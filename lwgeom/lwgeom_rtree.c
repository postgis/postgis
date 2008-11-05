/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2005 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 * 
 **********************************************************************/

#include "lwgeom_pg.h"
#include "liblwgeom.h"
#include "lwgeom_rtree.h"

/*
 * Creates an rtree given a pointer to the point array.
 * Must copy the point array.
 */
RTREE_NODE *createTree(POINTARRAY *pointArray)
{
        RTREE_NODE *root;
	RTREE_NODE** nodes = lwalloc(sizeof(RTREE_NODE*) * pointArray->npoints);
	int i, nodeCount;
        int childNodes, parentNodes;

#ifdef PGIS_DEBUG_CALLS
        lwnotice("createTree called with pointarray %p", pointArray);
#endif

        nodeCount = pointArray->npoints - 1;

#ifdef PGIS_DEBUG
        lwnotice("Total leaf nodes: %d", nodeCount);
#endif

        /*
         * Create a leaf node for every line segment.
         */
        for(i = 0; i < nodeCount; i++)
        {
                nodes[i] = createLeafNode(pointArray, i);
        }

        /*
         * Next we group nodes by pairs.  If there's an odd number of nodes,
         * we bring the last node up a level as is.  Continue until we have
         * a single top node.
         */
        childNodes = nodeCount;
        parentNodes = nodeCount / 2;
        while(parentNodes > 0)
        {
#ifdef PGIS_DEBUG
                lwnotice("Merging %d children into %d parents.", childNodes, parentNodes);
#endif
                i = 0;
                while(i < parentNodes) 
                {
                        nodes[i] = createInteriorNode(nodes[i*2], nodes[i*2+1]);
                        i++;
                }
                /*
                 * Check for an odd numbered final node.
                 */
                if(parentNodes * 2 < childNodes)
                {
#ifdef PGIS_DEBUG
                        lwnotice("Shuffling child %d to parent %d", childNodes - 1, i);
#endif
                        nodes[i] = nodes[childNodes - 1];
                        parentNodes++;
                }
                childNodes = parentNodes;
                parentNodes = parentNodes / 2;
        }

        root = nodes[0];
		lwfree(nodes);

#ifdef PGIS_DEBUG
        lwnotice("createTree returning %p", root);
#endif
        return root;
}

/* 
 * Creates an interior node given the children. 
 */
RTREE_NODE *createInteriorNode(RTREE_NODE *left, RTREE_NODE *right){
        RTREE_NODE *parent;

#ifdef PGIS_DEBUG_CALLS
        lwnotice("createInteriorNode called for children %p, %p", left, right);
#endif
        parent = lwalloc(sizeof(RTREE_NODE));
        parent->leftNode = left;
        parent->rightNode = right;
        parent->interval = mergeIntervals(left->interval, right->interval);
        parent->segment = NULL;
#ifdef PGIS_DEBUG_CALLS
        lwnotice("createInteriorNode returning %p", parent);
#endif
        return parent;
}

/*
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

#ifdef PGIS_DEBUG_CALLS
        lwnotice("createLeafNode called for point %d of %p", startPoint, pa);
#endif
        if(pa->npoints < startPoint + 2)
        {
                lwerror("createLeafNode: npoints = %d, startPoint = %d", pa->npoints, startPoint);
        }

        /*
         * The given point array will be part of a geometry that will be freed
         * independently of the index.  Since we may want to cache the index,
         * we must create independent arrays.
         */         
        npa = lwalloc(sizeof(POINTARRAY));
        npa->dims = 0;
        npa->npoints = 2;
        TYPE_SETZM(npa->dims, 0, 0);
        npa->serialized_pointlist = lwalloc(pointArray_ptsize(pa) * 2);

        getPoint4d_p(pa, startPoint, &tmp);

        setPoint4d(npa, 0, &tmp);
        value1 = tmp.y;

        getPoint4d_p(pa, startPoint + 1, &tmp);

        setPoint4d(npa, 1, &tmp);
        value2 = tmp.y;
        
        line = lwline_construct(-1, NULL, npa);
        
        parent = lwalloc(sizeof(RTREE_NODE));
        parent->interval = createInterval(value1, value2);
        parent->segment = line;
        parent->leftNode = NULL;
        parent->rightNode = NULL;

#ifdef PGIS_DEBUG_CALLS
        lwnotice("createLeafNode returning %p", parent);
#endif
        return parent;
}

/*
 * Creates an interval with the total extents of the two given intervals.
 */
INTERVAL *mergeIntervals(INTERVAL *inter1, INTERVAL *inter2)
{
        INTERVAL *interval;

#ifdef PGIS_DEBUG_CALLS
        lwnotice("mergeIntervals called with %p, %p", inter1, inter2);
#endif
        interval = lwalloc(sizeof(INTERVAL));
        interval->max = FP_MAX(inter1->max, inter2->max);
        interval->min = FP_MIN(inter1->min, inter2->min);
#ifdef PGIS_DEBUG
        lwnotice("interval min = %8.3f, max = %8.3f", interval->min, interval->max);
#endif
        return interval;
}

/*
 * Creates an interval given the min and max values, in arbitrary order.
 */
INTERVAL *createInterval(double value1, double value2)
{
        INTERVAL *interval;

#ifdef PGIS_DEBUG_CALLS
        lwnotice("createInterval called with %8.3f, %8.3f", value1, value2);
#endif
        interval = lwalloc(sizeof(INTERVAL));
        interval->max = FP_MAX(value1, value2);
        interval->min = FP_MIN(value1, value2);
#ifdef PGIS_DEBUG
        lwnotice("interval min = %8.3f, max = %8.3f", interval->min, interval->max);
#endif
        return interval;
}

/*
 * Recursively frees the child nodes, the interval and the line before 
 * freeing the root node.
 */
void freeTree(RTREE_NODE *root)
{

#ifdef PGIS_DEBUG_CALLS
        lwnotice("freeTree called for %p", root);
#endif
		if(root->leftNode)
				freeTree(root->leftNode);
		if(root->rightNode)
				freeTree(root->rightNode);
		lwfree(root->interval);
		if(root->segment) {
			lwfree(root->segment->points->serialized_pointlist);
			lwfree(root->segment->points);
			lwgeom_release((LWGEOM *)root->segment);
		}
		lwfree(root);
}

/*
 * Free the cache object and all the sub-objects properly.
 */
void clearCache(RTREE_POLY_CACHE *cache)
{
	int i;
#ifdef PGIS_DEBUG_CALLS
    lwnotice("clearCache called for %p", cache);
#endif
	for(i = 0; i < cache->ringCount; i++)
	{ 
		freeTree(cache->ringIndices[i]);
	}
	lwfree(cache->ringIndices);
	lwfree(cache->poly);
	cache->poly = 0;
	cache->ringIndices = 0;
	cache->ringCount = 0;
	cache->polyCount = 0;
}


/*
 * Retrieves a collection of line segments given the root and crossing value.
 * The collection is a multilinestring consisting of two point lines 
 * representing the segments of the ring that may be crossed by the 
 * horizontal projection line at the given y value.
 */
LWMLINE *findLineSegments(RTREE_NODE *root, double value)
{
        LWMLINE *tmp, *result;
        LWGEOM **lwgeoms;
        
#ifdef PGIS_DEBUG_CALLS
        lwnotice("findLineSegments called for tree %p and value %8.3f", root, value);
#endif
        result = NULL;

        if(!isContained(root->interval, value))
        {
#ifdef PGIS_DEBUG
                lwnotice("findLineSegments %p: not contained.", root);
#endif
                return NULL;
        }

        /* If there is a segment defined for this node, include it. */
        if(root->segment)
        {
#ifdef PGIS_DEBUG
                lwnotice("findLineSegments %p: adding segment %p %d.", root, root->segment, TYPE_GETTYPE(root->segment->type));
#endif
                lwgeoms = lwalloc(sizeof(LWGEOM *));
                lwgeoms[0] = (LWGEOM *)root->segment;

#ifdef PGIS_DEBUG
                lwnotice("Found geom %p, type %d, dim %d", root->segment, TYPE_GETTYPE(root->segment->type), TYPE_GETZM(root->segment->type));
#endif
                result = (LWMLINE *)lwcollection_construct(lwgeom_makeType_full(0, 0, 0, MULTILINETYPE, 0), -1, NULL, 1, lwgeoms);
        }

        /* If there is a left child node, recursively include its results. */
        if(root->leftNode)
        {
#ifdef PGIS_DEBUG
                lwnotice("findLineSegments %p: recursing left.", root);
#endif
                tmp = findLineSegments(root->leftNode, value);
                if(tmp)
                {
#ifdef PGIS_DEBUG
                        lwnotice("Found geom %p, type %d, dim %d", tmp, TYPE_GETTYPE(tmp->type), TYPE_GETZM(tmp->type));
#endif
                        if(result)
                                result = mergeMultiLines(result, tmp);
                        else
                                result = tmp;
                }
        }

        /* Same for any right child. */
        if(root->rightNode)
        {
#ifdef PGIS_DEBUG
                lwnotice("findLineSegments %p: recursing right.", root);
#endif
                tmp = findLineSegments(root->rightNode, value);
                if(tmp)
                {
#ifdef PGIS_DEBUG
                        lwnotice("Found geom %p, type %d, dim %d", tmp, TYPE_GETTYPE(tmp->type), TYPE_GETZM(tmp->type));
#endif
                        if(result)
                                result = mergeMultiLines(result, tmp);
                        else
                                result = tmp;
                }
        }
                        
        return result;
}

/* Merges two multilinestrings into a single multilinestring. */
LWMLINE *mergeMultiLines(LWMLINE *line1, LWMLINE *line2)
{
        LWGEOM **geoms;
        LWCOLLECTION *col;
        int i, j, ngeoms;
        
#ifdef PGIS_DEBUG_CALLS
        lwnotice("mergeMultiLines called on %p, %d, %d; %p, %d, %d", line1, line1->ngeoms, TYPE_GETTYPE(line1->type), line2, line2->ngeoms, TYPE_GETTYPE(line2->type));
#endif
        ngeoms = line1->ngeoms + line2->ngeoms;
        geoms = lwalloc(sizeof(LWGEOM *) * ngeoms);

        j = 0;
        for(i = 0; i < line1->ngeoms; i++, j++)
        {
                geoms[j] = lwgeom_clone((LWGEOM *)line1->geoms[i]);
        }
        for(i = 0; i < line2->ngeoms; i++, j++)
        {
                geoms[j] = lwgeom_clone((LWGEOM *)line2->geoms[i]);
        }
        col = lwcollection_construct(MULTILINETYPE, -1, NULL, ngeoms, geoms);

#ifdef PGIS_DEBUG_CALLS
        lwnotice("mergeMultiLines returning %p, %d, %d", col, col->ngeoms, TYPE_GETTYPE(col->type));
#endif

        return (LWMLINE *)col;
}

/*
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
        
#ifdef PGIS_DEBUG_CALLS
        int i;
        lwnotice("polygon_index called.");
#endif
        result = NULL;
        igeom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
        yval = PG_GETARG_FLOAT8(1);
        geom = lwgeom_deserialize(SERIALIZED_FORM(igeom));
        if(TYPE_GETTYPE(geom->type) != POLYGONTYPE)
        {
                lwgeom_release(geom);
                PG_FREE_IF_COPY(igeom, 0);
                PG_RETURN_NULL();
        }
        poly = (LWPOLY *)geom;
        root = createTree(poly->rings[0]);

        mline = findLineSegments(root, yval);

#ifdef PGIS_DEBUG
        lwnotice("mline returned %p %d", mline, TYPE_GETTYPE(mline->type));
        for(i = 0; i < mline->ngeoms; i++)
        {
                lwnotice("geom[%d] %p %d", i, mline->geoms[i], TYPE_GETTYPE(mline->geoms[i]->type));
        }
#endif

        if(mline)
                result = pglwgeom_serialize((LWGEOM *)mline);

#ifdef PGIS_DEBUG
        lwnotice("returning result %p", result);
#endif
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
	result->ringCount = 0;
	result->ringIndices = 0;
	result->poly = 0;
    result->type = 1;
	return result;
}

void populateCache(RTREE_POLY_CACHE *currentCache, LWGEOM *lwgeom, uchar *serializedPoly)
{
	int i, j, k, length;
	LWMPOLY *mpoly;
	LWPOLY *poly;
	int nrings;
	
#ifdef PGIS_DEBUG_CALLS
    lwnotice("populateCache called with cache %p geom %p", currentCache, lwgeom);
#endif

	if(TYPE_GETTYPE(lwgeom->type) == MULTIPOLYGONTYPE) 
	{
#ifdef PGIS_DEBUG_CALLS
		lwnotice("populateCache MULTIPOLYGON");
#endif
		mpoly = (LWMPOLY *)lwgeom;
		nrings = 0;
		/*
		** Count the total number of rings.
		*/
		for( i = 0; i < mpoly->ngeoms; i++ ) 
		{
			nrings += mpoly->geoms[i]->nrings;
		}
		currentCache->polyCount = mpoly->ngeoms;
		currentCache->ringCount = nrings;
		currentCache->ringIndices = lwalloc(sizeof(RTREE_NODE *) * nrings);
		/*
		** Load the exterior rings onto the ringIndices array first
		*/
		for( i = 0; i < mpoly->ngeoms; i++ ) 
		{
			currentCache->ringIndices[i] = createTree(mpoly->geoms[i]->rings[0]);
		}
		/*
		** Load the interior rings (holes) onto ringIndices next
		*/
		for( j = 0; j < mpoly->ngeoms; j++ )
		{
			for( k = 1; k < mpoly->geoms[j]->nrings; k++ ) 
			{
				currentCache->ringIndices[i] = createTree(mpoly->geoms[j]->rings[k]);
				i++;
			}
		}
	}
	else if ( TYPE_GETTYPE(lwgeom->type) == POLYGONTYPE ) 
	{
#ifdef PGIS_DEBUG_CALLS
		lwnotice("populateCache POLYGON");
#endif
		poly = (LWPOLY *)lwgeom;
		currentCache->polyCount = 1;
		currentCache->ringCount = poly->nrings;
		/*
		** Just load the rings on in order
		*/
		currentCache->ringIndices = lwalloc(sizeof(RTREE_NODE *) * poly->nrings);
		for( i = 0; i < poly->nrings; i++ ) 
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
	length = lwgeom_size(serializedPoly);
	currentCache->poly = lwalloc(length);
	memcpy(currentCache->poly, serializedPoly, length); 
#ifdef PGIS_DEBUG_CALLS
	lwnotice("populateCache returning %p", currentCache);
#endif

}

/* 
 * Creates a new cachable index if needed, or returns the current cache if
 * it is applicable to the current polygon.
 * The memory context must be changed to function scope before calling this
 * method.  The method will allocate memory for the cache it creates,
 * as well as freeing the memory of any cache that is no longer applicable.
 */
RTREE_POLY_CACHE *retrieveCache(LWGEOM *lwgeom, uchar *serializedPoly, 
                RTREE_POLY_CACHE *currentCache)
{
        int length;

        /* Make sure this isn't someone else's cache object. */
        if( currentCache && currentCache->type != 1 ) currentCache = NULL;

#ifdef PGIS_DEBUG_CALLS
        lwnotice("retrieveCache called with %p %p %p", lwgeom, serializedPoly, currentCache);
#endif
        if(!currentCache)
        {
#ifdef PGIS_DEBUG
                lwnotice("No existing cache, create one.");
#endif
				return createCache();
        }
        if(!(currentCache->poly))
        {
#ifdef PGIS_DEBUG
                lwnotice("Cache contains no polygon, populating it.");
#endif
				populateCache(currentCache, lwgeom, serializedPoly);
				return currentCache;
        }

        length = lwgeom_size_poly(serializedPoly);

		if(lwgeom_size(currentCache->poly) != length)
        {
#ifdef PGIS_DEBUG
                lwnotice("Polygon size mismatch, creating new cache.");
#endif
				clearCache(currentCache);
				return currentCache;
		}
		if( memcmp(serializedPoly, currentCache->poly, length) ) 
		{
#ifdef PGIS_DEBUG
				//lwnotice("Polygon mismatch, creating new cache. %c, %c", a, b);
#endif
				clearCache(currentCache);
				return currentCache;
		}

#ifdef PGIS_DEBUG
        lwnotice("Polygon match, retaining current cache, %p.", currentCache);
#endif
        return currentCache;
}

