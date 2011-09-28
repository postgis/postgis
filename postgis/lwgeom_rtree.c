/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright (C) 2001-2005 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "lwgeom_pg.h"
#include "liblwgeom.h"
#include "liblwgeom_internal.h"         /* For FP comparators. */
#include "lwgeom_rtree.h"


Datum LWGEOM_polygon_index(PG_FUNCTION_ARGS);


/**
 * Creates an rtree given a pointer to the point array.
 * Must copy the point array.
 */
RTREE_NODE *createTree(POINTARRAY *pointArray)
{
	RTREE_NODE *root;
	RTREE_NODE** nodes = lwalloc(sizeof(RTREE_NODE*) * pointArray->npoints);
	int i, nodeCount;
	int childNodes, parentNodes;

	POSTGIS_DEBUGF(2, "createTree called with pointarray %p", pointArray);

	nodeCount = pointArray->npoints - 1;

	POSTGIS_DEBUGF(3, "Total leaf nodes: %d", nodeCount);

	/*
	 * Create a leaf node for every line segment.
	 */
	for (i = 0; i < nodeCount; i++)
	{
		nodes[i] = createLeafNode(pointArray, i);
	}

	/*
	 * Next we group nodes by pairs.  If there's an odd number of nodes,
	 * we bring the last node up a level as is.	 Continue until we have
	 * a single top node.
	 */
	childNodes = nodeCount;
	parentNodes = nodeCount / 2;
	while (parentNodes > 0)
	{
		POSTGIS_DEBUGF(3, "Merging %d children into %d parents.", childNodes, parentNodes);

		i = 0;
		while (i < parentNodes)
		{
			nodes[i] = createInteriorNode(nodes[i*2], nodes[i*2+1]);
			i++;
		}
		/*
		 * Check for an odd numbered final node.
		 */
		if (parentNodes * 2 < childNodes)
		{
			POSTGIS_DEBUGF(3, "Shuffling child %d to parent %d", childNodes - 1, i);

			nodes[i] = nodes[childNodes - 1];
			parentNodes++;
		}
		childNodes = parentNodes;
		parentNodes = parentNodes / 2;
	}

	root = nodes[0];
	lwfree(nodes);
	POSTGIS_DEBUGF(3, "createTree returning %p", root);

	return root;
}

/**
 * Creates an interior node given the children.
 */
RTREE_NODE *createInteriorNode(RTREE_NODE *left, RTREE_NODE *right)
{
	RTREE_NODE *parent;

	POSTGIS_DEBUGF(2, "createInteriorNode called for children %p, %p", left, right);

	parent = lwalloc(sizeof(RTREE_NODE));
	parent->leftNode = left;
	parent->rightNode = right;
	parent->interval = mergeIntervals(left->interval, right->interval);
	parent->segment = NULL;

	POSTGIS_DEBUGF(3, "createInteriorNode returning %p", parent);

	return parent;
}

/**
 * Creates a leaf node given the pointer to the start point of the segment.
 */
RTREE_NODE *createLeafNode(POINTARRAY *pa, int startPoint)
{
	RTREE_NODE *parent;
	LWLINE *line;
	double value1;
	double value2;
	POINT4D tmp;
	POINTARRAY *npa;

	POSTGIS_DEBUGF(2, "createLeafNode called for point %d of %p", startPoint, pa);

	if (pa->npoints < startPoint + 2)
	{
		lwerror("createLeafNode: npoints = %d, startPoint = %d", pa->npoints, startPoint);
	}

	/*
	 * The given point array will be part of a geometry that will be freed
	 * independently of the index.	Since we may want to cache the index,
	 * we must create independent arrays.
	 */
	npa = ptarray_construct_empty(0,0,2);

	getPoint4d_p(pa, startPoint, &tmp);
	value1 = tmp.y;
	ptarray_append_point(npa,&tmp,LW_TRUE);
	
	getPoint4d_p(pa, startPoint+1, &tmp);
	value2 = tmp.y;
	ptarray_append_point(npa,&tmp,LW_TRUE);

	line = lwline_construct(-1, NULL, npa);
	
	parent = lwalloc(sizeof(RTREE_NODE));
	parent->interval = createInterval(value1, value2);
	parent->segment = line;
	parent->leftNode = NULL;
	parent->rightNode = NULL;

	POSTGIS_DEBUGF(3, "createLeafNode returning %p", parent);

	return parent;
}

/**
 * Creates an interval with the total extents of the two given intervals.
 */
INTERVAL *mergeIntervals(INTERVAL *inter1, INTERVAL *inter2)
{
	INTERVAL *interval;

	POSTGIS_DEBUGF(2, "mergeIntervals called with %p, %p", inter1, inter2);

	interval = lwalloc(sizeof(INTERVAL));
	interval->max = FP_MAX(inter1->max, inter2->max);
	interval->min = FP_MIN(inter1->min, inter2->min);

	POSTGIS_DEBUGF(3, "interval min = %8.3f, max = %8.3f", interval->min, interval->max);

	return interval;
}

/**
 * Creates an interval given the min and max values, in arbitrary order.
 */
INTERVAL *createInterval(double value1, double value2)
{
	INTERVAL *interval;

	POSTGIS_DEBUGF(2, "createInterval called with %8.3f, %8.3f", value1, value2);

	interval = lwalloc(sizeof(INTERVAL));
	interval->max = FP_MAX(value1, value2);
	interval->min = FP_MIN(value1, value2);

	POSTGIS_DEBUGF(3, "interval min = %8.3f, max = %8.3f", interval->min, interval->max);

	return interval;
}

/**
 * Recursively frees the child nodes, the interval and the line before
 * freeing the root node.
 */
void freeTree(RTREE_NODE *root)
{
	POSTGIS_DEBUGF(2, "freeTree called for %p", root);

	if (root->leftNode)
		freeTree(root->leftNode);
	if (root->rightNode)
		freeTree(root->rightNode);
	lwfree(root->interval);
	if (root->segment)
	{
		lwfree(root->segment->points->serialized_pointlist);
		lwfree(root->segment->points);
		lwgeom_release((LWGEOM *)root->segment);
	}
	lwfree(root);
}


/**
 * Free the cache object and all the sub-objects properly.
 */
void clearCache(RTREE_POLY_CACHE *cache)
{
	int g, r, i;
	POSTGIS_DEBUGF(2, "clearCache called for %p", cache);
        i = 0;
        for (g = 0; g < cache->polyCount; g++)
        {
	        for (r = 0; r < cache->ringCounts[g]; r++)
	        {
		        freeTree(cache->ringIndices[i]);
                        i++;
                }
	}
	lwfree(cache->ringIndices);
        lwfree(cache->ringCounts);
	lwfree(cache->poly);
	cache->poly = 0;
	cache->ringIndices = 0;
	cache->ringCounts = 0;
	cache->polyCount = 0;
}


/**
 * Retrieves a collection of line segments given the root and crossing value.
 * The collection is a multilinestring consisting of two point lines
 * representing the segments of the ring that may be crossed by the
 * horizontal projection line at the given y value.
 */
LWMLINE *findLineSegments(RTREE_NODE *root, double value)
{
	LWMLINE *tmp, *result;
	LWGEOM **lwgeoms;

	POSTGIS_DEBUGF(2, "findLineSegments called for tree %p and value %8.3f", root, value);

	result = NULL;

	if (!isContained(root->interval, value))
	{
		POSTGIS_DEBUGF(3, "findLineSegments %p: not contained.", root);

		return NULL;
	}

	/* If there is a segment defined for this node, include it. */
	if (root->segment)
	{
		POSTGIS_DEBUGF(3, "findLineSegments %p: adding segment %p %d.", root, root->segment, root->segment->type);

		lwgeoms = lwalloc(sizeof(LWGEOM *));
		lwgeoms[0] = (LWGEOM *)root->segment;

		POSTGIS_DEBUGF(3, "Found geom %p, type %d, dim %d", root->segment, root->segment->type, FLAGS_GET_Z(root->segment->flags));

		result = (LWMLINE *)lwcollection_construct(lwgeom_makeType_full(0, 0, 0, MULTILINETYPE, 0), -1, NULL, 1, lwgeoms);
	}

	/* If there is a left child node, recursively include its results. */
	if (root->leftNode)
	{
		POSTGIS_DEBUGF(3, "findLineSegments %p: recursing left.", root);

		tmp = findLineSegments(root->leftNode, value);
		if (tmp)
		{
			POSTGIS_DEBUGF(3, "Found geom %p, type %d, dim %d", tmp, tmp->type, FLAGS_GET_Z(tmp->flags));

			if (result)
				result = mergeMultiLines(result, tmp);
			else
				result = tmp;
		}
	}

	/* Same for any right child. */
	if (root->rightNode)
	{
		POSTGIS_DEBUGF(3, "findLineSegments %p: recursing right.", root);

		tmp = findLineSegments(root->rightNode, value);
		if (tmp)
		{
			POSTGIS_DEBUGF(3, "Found geom %p, type %d, dim %d", tmp, tmp->type, FLAGS_GET_Z(tmp->flags));

			if (result)
				result = mergeMultiLines(result, tmp);
			else
				result = tmp;
		}
	}

	return result;
}

/** Merges two multilinestrings into a single multilinestring. */
LWMLINE *mergeMultiLines(LWMLINE *line1, LWMLINE *line2)
{
	LWGEOM **geoms;
	LWCOLLECTION *col;
	int i, j, ngeoms;

	POSTGIS_DEBUGF(2, "mergeMultiLines called on %p, %d, %d; %p, %d, %d", line1, line1->ngeoms, line1->type, line2, line2->ngeoms, line2->type);

	ngeoms = line1->ngeoms + line2->ngeoms;
	geoms = lwalloc(sizeof(LWGEOM *) * ngeoms);

	j = 0;
	for (i = 0; i < line1->ngeoms; i++, j++)
	{
		geoms[j] = lwgeom_clone((LWGEOM *)line1->geoms[i]);
	}
	for (i = 0; i < line2->ngeoms; i++, j++)
	{
		geoms[j] = lwgeom_clone((LWGEOM *)line2->geoms[i]);
	}
	col = lwcollection_construct(MULTILINETYPE, -1, NULL, ngeoms, geoms);

	POSTGIS_DEBUGF(3, "mergeMultiLines returning %p, %d, %d", col, col->ngeoms, col->type);

	return (LWMLINE *)col;
}

/**
 * Returns 1 if min < value <= max, 0 otherwise. */
uint32 isContained(INTERVAL *interval, double value)
{
	return FP_CONTAINS_INCL(interval->min, value, interval->max) ? 1 : 0;
}

PG_FUNCTION_INFO_V1(LWGEOM_polygon_index);
Datum LWGEOM_polygon_index(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *igeom, *result;
	LWGEOM *geom;
	LWPOLY *poly;
	LWMLINE *mline;
	RTREE_NODE *root;
	double yval;
#if POSTGIS_DEBUG_LEVEL >= 3
	int i = 0;
#endif

	POSTGIS_DEBUG(2, "polygon_index called.");

	result = NULL;
	igeom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	yval = PG_GETARG_FLOAT8(1);
	geom = pglwgeom_deserialize(igeom);
	if (geom->type != POLYGONTYPE)
	{
		lwgeom_release(geom);
		PG_FREE_IF_COPY(igeom, 0);
		PG_RETURN_NULL();
	}
	poly = (LWPOLY *)geom;
	root = createTree(poly->rings[0]);

	mline = findLineSegments(root, yval);

#if POSTGIS_DEBUG_LEVEL >= 3
	POSTGIS_DEBUGF(3, "mline returned %p %d", mline, mline->type);
	for (i = 0; i < mline->ngeoms; i++)
	{
		POSTGIS_DEBUGF(3, "geom[%d] %p %d", i, mline->geoms[i], mline->geoms[i]->type);
	}
#endif

	if (mline)
		result = pglwgeom_serialize((LWGEOM *)mline);

	POSTGIS_DEBUGF(3, "returning result %p", result);

	lwfree(root);

	PG_FREE_IF_COPY(igeom, 0);
	lwgeom_release((LWGEOM *)poly);
	lwgeom_release((LWGEOM *)mline);
	PG_RETURN_POINTER(result);

}

RTREE_POLY_CACHE * createCache()
{
	RTREE_POLY_CACHE *result;
	result = lwalloc(sizeof(RTREE_POLY_CACHE));
	result->polyCount = 0;
	result->ringCounts = 0;
	result->ringIndices = 0;
	result->poly = 0;
	result->type = 1;
	return result;
}

void populateCache(RTREE_POLY_CACHE *currentCache, LWGEOM *lwgeom, PG_LWGEOM *serializedPoly)
{
	int i, p, r, length;
	LWMPOLY *mpoly;
	LWPOLY *poly;
	int nrings;

	POSTGIS_DEBUGF(2, "populateCache called with cache %p geom %p", currentCache, lwgeom);

	if (lwgeom->type == MULTIPOLYGONTYPE)
	{
		POSTGIS_DEBUG(2, "populateCache MULTIPOLYGON");
		mpoly = (LWMPOLY *)lwgeom;
		nrings = 0;
		/*
		** Count the total number of rings.
		*/
		currentCache->polyCount = mpoly->ngeoms;
		currentCache->ringCounts = lwalloc(sizeof(int) * mpoly->ngeoms);
		for ( i = 0; i < mpoly->ngeoms; i++ )
		{
                        currentCache->ringCounts[i] = mpoly->geoms[i]->nrings;
			nrings += mpoly->geoms[i]->nrings;
		}
		currentCache->ringIndices = lwalloc(sizeof(RTREE_NODE *) * nrings);
		/*
		** Load the array in geometry order, each outer ring followed by the inner rings
                ** associated with that outer ring
		*/
                i = 0;
		for ( p = 0; p < mpoly->ngeoms; p++ )
		{
			for ( r = 0; r < mpoly->geoms[p]->nrings; r++ )
			{
				currentCache->ringIndices[i] = createTree(mpoly->geoms[p]->rings[r]);
				i++;
			}
		}
	}
	else if ( lwgeom->type == POLYGONTYPE )
	{
		POSTGIS_DEBUG(2, "populateCache POLYGON");
		poly = (LWPOLY *)lwgeom;
		currentCache->polyCount = 1;
		currentCache->ringCounts = lwalloc(sizeof(int));
		currentCache->ringCounts[0] = poly->nrings;
		/*
		** Just load the rings on in order
		*/
		currentCache->ringIndices = lwalloc(sizeof(RTREE_NODE *) * poly->nrings);
		for ( i = 0; i < poly->nrings; i++ )
		{
			currentCache->ringIndices[i] = createTree(poly->rings[i]);
		}
	}
	else
	{
		/* Uh oh, shouldn't be here. */
		return;
	}

	/*
	** Copy the serialized form of the polygon into the cache so
	** we can test for equality against subsequent polygons.
	*/
	length = VARSIZE(serializedPoly);
	currentCache->poly = lwalloc(length);
	memcpy(currentCache->poly, serializedPoly, length);
	POSTGIS_DEBUGF(3, "populateCache returning %p", currentCache);
}

/**
 * Creates a new cachable index if needed, or returns the current cache if
 * it is applicable to the current polygon.
 * The memory context must be changed to function scope before calling this
 * method.	The method will allocate memory for the cache it creates,
 * as well as freeing the memory of any cache that is no longer applicable.
 */
RTREE_POLY_CACHE *retrieveCache(LWGEOM *lwgeom, PG_LWGEOM *serializedPoly, RTREE_POLY_CACHE *currentCache)
{
	int length;

	POSTGIS_DEBUGF(2, "retrieveCache called with %p %p %p", lwgeom, serializedPoly, currentCache);

	/* Make sure this isn't someone else's cache object. */
	if ( currentCache && currentCache->type != 1 ) currentCache = NULL;

	if (!currentCache)
	{
		POSTGIS_DEBUG(3, "No existing cache, create one.");
		return createCache();
	}
	if (!(currentCache->poly))
	{
		POSTGIS_DEBUG(3, "Cache contains no polygon, populating it.");
		populateCache(currentCache, lwgeom, serializedPoly);
		return currentCache;
	}

	length = VARSIZE(serializedPoly);

	if (VARSIZE(currentCache->poly) != length)
	{
		POSTGIS_DEBUG(3, "Polygon size mismatch, creating new cache.");
		clearCache(currentCache);
		return currentCache;
	}
	if ( memcmp(serializedPoly, currentCache->poly, length) )
	{
		POSTGIS_DEBUG(3, "Polygon mismatch, creating new cache.");
		clearCache(currentCache);
		return currentCache;
	}

	POSTGIS_DEBUGF(3, "Polygon match, retaining current cache, %p.",
	                  currentCache);

	return currentCache;
}

