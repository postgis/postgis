/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * PostGIS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * PostGIS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PostGIS.  If not, see <http://www.gnu.org/licenses/>.
 *
 **********************************************************************
 *
 * Copyright (C) 2001-2005 Refractions Research Inc.
 *
 **********************************************************************/


#include <assert.h>

#include "../postgis_config.h"
#include "lwgeom_pg.h"
#include "liblwgeom.h"
#include "liblwgeom_internal.h" /* For FP comparators. */
#include "lwgeom_cache.h"
#include "lwgeom_rtree.h"
#include "lwgeom_functions_analytic.h"


/* Prototypes */
static void RTreeFree(RTREE_NODE* root);

/**
* Allocate a fresh clean RTREE_POLY_CACHE
*/
static RTREE_POLY_CACHE*
RTreeCacheCreate()
{
	RTREE_POLY_CACHE* result;
	result = lwalloc(sizeof(RTREE_POLY_CACHE));
	memset(result, 0, sizeof(RTREE_POLY_CACHE));
	return result;
}

/**
* Recursively frees the child nodes, the interval and the line before
* freeing the root node.
*/
static void
RTreeFree(RTREE_NODE* root)
{
	POSTGIS_DEBUGF(2, "RTreeFree called for %p", root);

	if (root->leftNode)
		RTreeFree(root->leftNode);
	if (root->rightNode)
		RTreeFree(root->rightNode);
	if (root->segment)
	{
		lwline_free(root->segment);
	}
	lwfree(root);
}

/**
* Free the cache object and all the sub-objects properly.
*/
static void
RTreeCacheClear(RTREE_POLY_CACHE* cache)
{
	int g, r, i;
	POSTGIS_DEBUGF(2, "RTreeCacheClear called for %p", cache);
	i = 0;
	for (g = 0; g < cache->polyCount; g++)
	{
		for (r = 0; r < cache->ringCounts[g]; r++)
		{
			RTreeFree(cache->ringIndices[i]);
			i++;
		}
	}
	lwfree(cache->ringIndices);
	lwfree(cache->ringCounts);
	cache->ringIndices = 0;
	cache->ringCounts = 0;
	cache->polyCount = 0;
}


/**
 * Returns 1 if min < value <= max, 0 otherwise.
*/
static uint32
IntervalIsContained(const RTREE_INTERVAL* interval, double value)
{
	return FP_CONTAINS_INCL(interval->min, value, interval->max) ? 1 : 0;
}

/**
* Creates an interval with the total extents of the two given intervals.
*/
static void
RTreeMergeIntervals(
	const RTREE_INTERVAL *inter1,
	const RTREE_INTERVAL *inter2,
	RTREE_INTERVAL *result)
{
	result->max = FP_MAX(inter1->max, inter2->max);
	result->min = FP_MIN(inter1->min, inter2->min);
	return;
}


/**
* Creates an interior node given the children.
*/
static RTREE_NODE*
RTreeCreateInteriorNode(RTREE_NODE* left, RTREE_NODE* right)
{
	RTREE_NODE *parent;

	POSTGIS_DEBUGF(2, "RTreeCreateInteriorNode called for children %p, %p", left, right);

	parent = lwalloc(sizeof(RTREE_NODE));
	parent->leftNode = left;
	parent->rightNode = right;
	parent->segment = NULL;

	RTreeMergeIntervals(
		&(left->interval), &(right->interval),
		&(parent->interval));

	POSTGIS_DEBUGF(3, "RTreeCreateInteriorNode returning %p", parent);

	return parent;
}

/**
* Creates a leaf node given the pointer to the start point of the segment.
*/
static RTREE_NODE*
RTreeCreateLeafNode(POINTARRAY* pa, uint32_t startPoint)
{
	RTREE_NODE *parent;
	LWLINE *line;
	double value1;
	double value2;
	POINT4D tmp;
	POINTARRAY *npa;

	POSTGIS_DEBUGF(2, "RTreeCreateLeafNode called for point %d of %p", startPoint, pa);

	if (pa->npoints < startPoint + 2)
	{
		lwpgerror("RTreeCreateLeafNode: npoints = %d, startPoint = %d", pa->npoints, startPoint);
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

	line = lwline_construct(SRID_UNKNOWN, NULL, npa);

	parent = lwalloc(sizeof(RTREE_NODE));
	parent->interval.min = FP_MIN(value1, value2);
	parent->interval.max = FP_MAX(value1, value2);
	parent->segment = line;
	parent->leftNode = NULL;
	parent->rightNode = NULL;

	POSTGIS_DEBUGF(3, "RTreeCreateLeafNode returning %p", parent);

	return parent;
}

/**
* Creates an rtree given a pointer to the point array.
* Must copy the point array.
*/
static RTREE_NODE*
RTreeCreate(POINTARRAY* pointArray)
{
	RTREE_NODE* root;
	RTREE_NODE** nodes = lwalloc(pointArray->npoints * sizeof(RTREE_NODE*));
	uint32_t i, nodeCount;
	uint32_t childNodes, parentNodes;

	POSTGIS_DEBUGF(2, "RTreeCreate called with pointarray %p", pointArray);

	nodeCount = pointArray->npoints - 1;

	POSTGIS_DEBUGF(3, "Total leaf nodes: %d", nodeCount);

	/*
	 * Create a leaf node for every line segment.
	 */
	for (i = 0; i < nodeCount; i++)
	{
		nodes[i] = RTreeCreateLeafNode(pointArray, i);
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
			nodes[i] = RTreeCreateInteriorNode(nodes[i*2], nodes[i*2+1]);
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
	POSTGIS_DEBUGF(3, "RTreeCreate returning %p", root);

	return root;
}


/**
* Merges two multilinestrings into a single multilinestring.
*/
static LWMLINE*
RTreeMergeMultiLines(LWMLINE *line1, LWMLINE *line2)
{
	LWGEOM **geoms;
	LWCOLLECTION *col;
	uint32_t i, j, ngeoms;

	POSTGIS_DEBUGF(2, "RTreeMergeMultiLines called on %p, %d, %d; %p, %d, %d", line1, line1->ngeoms, line1->type, line2, line2->ngeoms, line2->type);

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
	col = lwcollection_construct(MULTILINETYPE, SRID_UNKNOWN, NULL, ngeoms, geoms);

	POSTGIS_DEBUGF(3, "RTreeMergeMultiLines returning %p, %d, %d", col, col->ngeoms, col->type);

	return (LWMLINE *)col;
}


/**
* Callback function sent into the GetGeomCache generic caching system. Given a
* LWGEOM* this function builds and stores an RTREE_POLY_CACHE into the provided
* GeomCache object.
*/
static int
RTreeBuilder(const LWGEOM* lwgeom, GeomCache* cache)
{
	uint32_t i, p, r;
	LWMPOLY *mpoly;
	LWPOLY *poly;
	int nrings;
	RTreeGeomCache* rtree_cache = (RTreeGeomCache*)cache;
	RTREE_POLY_CACHE* currentCache;

	if ( ! cache )
		return LW_FAILURE;

	if ( rtree_cache->index )
	{
		lwpgerror("RTreeBuilder asked to build index where one already exists.");
		return LW_FAILURE;
	}

	if (lwgeom->type == MULTIPOLYGONTYPE)
	{
		POSTGIS_DEBUG(2, "RTreeBuilder MULTIPOLYGON");
		mpoly = (LWMPOLY *)lwgeom;
		nrings = 0;
		/*
		** Count the total number of rings.
		*/
		currentCache = RTreeCacheCreate();
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
				currentCache->ringIndices[i] = RTreeCreate(mpoly->geoms[p]->rings[r]);
				i++;
			}
		}
		rtree_cache->index = currentCache;
	}
	else if ( lwgeom->type == POLYGONTYPE )
	{
		POSTGIS_DEBUG(2, "RTreeBuilder POLYGON");
		poly = (LWPOLY *)lwgeom;
		currentCache = RTreeCacheCreate();
		currentCache->polyCount = 1;
		currentCache->ringCounts = lwalloc(sizeof(int));
		currentCache->ringCounts[0] = poly->nrings;
		/*
		** Just load the rings on in order
		*/
		currentCache->ringIndices = lwalloc(sizeof(RTREE_NODE *) * poly->nrings);
		for ( i = 0; i < poly->nrings; i++ )
		{
			currentCache->ringIndices[i] = RTreeCreate(poly->rings[i]);
		}
		rtree_cache->index = currentCache;
	}
	else
	{
		/* Uh oh, shouldn't be here. */
		lwpgerror("RTreeBuilder got asked to build index on non-polygon");
		return LW_FAILURE;
	}
	return LW_SUCCESS;
}

/**
* Callback function sent into the GetGeomCache generic caching system. On a
* cache miss, this function clears the cached index object.
*/
static int
RTreeFreer(GeomCache* cache)
{
	RTreeGeomCache* rtree_cache = (RTreeGeomCache*)cache;

	if ( ! cache )
		return LW_FAILURE;

	if ( rtree_cache->index )
	{
		RTreeCacheClear(rtree_cache->index);
		lwfree(rtree_cache->index);
		rtree_cache->index = 0;
		rtree_cache->gcache.argnum = 0;
	}
	return LW_SUCCESS;
}

static GeomCache*
RTreeAllocator(void)
{
	RTreeGeomCache* cache = palloc(sizeof(RTreeGeomCache));
	memset(cache, 0, sizeof(RTreeGeomCache));
	return (GeomCache*)cache;
}

static GeomCacheMethods RTreeCacheMethods =
{
	RTREE_CACHE_ENTRY,
	RTreeBuilder,
	RTreeFreer,
	RTreeAllocator
};

RTREE_POLY_CACHE *
GetRtreeCache(FunctionCallInfo fcinfo, SHARED_GSERIALIZED *g1)
{
	RTreeGeomCache* cache = (RTreeGeomCache*)GetGeomCache(fcinfo, &RTreeCacheMethods, g1, NULL);
	RTREE_POLY_CACHE* index = NULL;

	if ( cache )
		index = cache->index;

	return index;
}


/**
* Retrieves a collection of line segments given the root and crossing value.
* The collection is a multilinestring consisting of two point lines
* representing the segments of the ring that may be crossed by the
* horizontal projection line at the given y value.
*/
LWMLINE *RTreeFindLineSegments(RTREE_NODE *root, double value)
{
	LWMLINE *tmp, *result;
	LWGEOM **lwgeoms;

	POSTGIS_DEBUGF(2, "RTreeFindLineSegments called for tree %p and value %8.3f", root, value);

	result = NULL;

	if (!IntervalIsContained(&(root->interval), value))
	{
		POSTGIS_DEBUGF(3, "RTreeFindLineSegments %p: not contained.", root);

		return NULL;
	}

	/* If there is a segment defined for this node, include it. */
	if (root->segment)
	{
		POSTGIS_DEBUGF(3, "RTreeFindLineSegments %p: adding segment %p %d.", root, root->segment, root->segment->type);

		lwgeoms = lwalloc(sizeof(LWGEOM *));
		lwgeoms[0] = (LWGEOM *)root->segment;

		POSTGIS_DEBUGF(3, "Found geom %p, type %d, dim %d", root->segment, root->segment->type, lwgeom_ndims((LWGEOM *)(root->segment)));

		result = (LWMLINE *)lwcollection_construct(MULTILINETYPE, SRID_UNKNOWN, NULL, 1, lwgeoms);
	}

	/* If there is a left child node, recursively include its results. */
	if (root->leftNode)
	{
		POSTGIS_DEBUGF(3, "RTreeFindLineSegments %p: recursing left.", root);

		tmp = RTreeFindLineSegments(root->leftNode, value);
		if (tmp)
		{
			POSTGIS_DEBUGF(3, "Found geom %p, type %d, dim %d", tmp, tmp->type, lwgeom_ndims((LWGEOM *)tmp));

			if (result)
				result = RTreeMergeMultiLines(result, tmp);
			else
				result = tmp;
		}
	}

	/* Same for any right child. */
	if (root->rightNode)
	{
		POSTGIS_DEBUGF(3, "RTreeFindLineSegments %p: recursing right.", root);

		tmp = RTreeFindLineSegments(root->rightNode, value);
		if (tmp)
		{
			POSTGIS_DEBUGF(3, "Found geom %p, type %d, dim %d", tmp, tmp->type, lwgeom_ndims((LWGEOM *)tmp));

			if (result)
				result = RTreeMergeMultiLines(result, tmp);
			else
				result = tmp;
		}
	}

	return result;
}



/*
 * return -1 iff point is outside ring pts
 * return 1 iff point is inside ring pts
 * return 0 iff point is on ring pts
 */
static int point_in_ring_rtree(RTREE_NODE *root, const POINT2D *point)
{
	int wn = 0;
	uint32_t i;
	double side;
	const POINT2D *seg1;
	const POINT2D *seg2;
	LWMLINE *lines;

	POSTGIS_DEBUG(2, "point_in_ring called.");

	lines = RTreeFindLineSegments(root, point->y);
	if (!lines)
		return -1;

	for (i=0; i<lines->ngeoms; i++)
	{
		seg1 = getPoint2d_cp(lines->geoms[i]->points, 0);
		seg2 = getPoint2d_cp(lines->geoms[i]->points, 1);

		side = determineSide(seg1, seg2, point);

		POSTGIS_DEBUGF(3, "segment: (%.8f, %.8f),(%.8f, %.8f)", seg1->x, seg1->y, seg2->x, seg2->y);
		POSTGIS_DEBUGF(3, "side result: %.8f", side);
		POSTGIS_DEBUGF(3, "counterclockwise wrap %d, clockwise wrap %d", FP_CONTAINS_BOTTOM(seg1->y, point->y, seg2->y), FP_CONTAINS_BOTTOM(seg2->y, point->y, seg1->y));

		/* zero length segments are ignored. */
		if (((seg2->x - seg1->x)*(seg2->x - seg1->x) + (seg2->y - seg1->y)*(seg2->y - seg1->y)) < 1e-12*1e-12)
		{
			POSTGIS_DEBUG(3, "segment is zero length... ignoring.");
			continue;
		}

		/* a point on the boundary of a ring is not contained. */
		/* WAS: if (fabs(side) < 1e-12), see #852 */
		if (side == 0.0)
		{
			if (isOnSegment(seg1, seg2, point) == 1)
			{
				POSTGIS_DEBUGF(3, "point on ring boundary between points %d, %d", i, i+1);
				return 0;
			}
		}

		/*
		 * If the point is to the left of the line, and it's rising,
		 * then the line is to the right of the point and
		 * circling counter-clockwise, so increment.
		 */
		if ((seg1->y <= point->y) && (point->y < seg2->y) && (side > 0))
		{
			POSTGIS_DEBUG(3, "incrementing winding number.");
			++wn;
		}
		/*
		 * If the point is to the right of the line, and it's falling,
		 * then the line is to the right of the point and circling
		 * clockwise, so decrement.
		 */
		else if ((seg2->y <= point->y) && (point->y < seg1->y) && (side < 0))
		{
			POSTGIS_DEBUG(3, "decrementing winding number.");
			--wn;
		}
	}

	POSTGIS_DEBUGF(3, "winding number %d", wn);

	if (wn == 0)
		return -1;
	return 1;
}

/*
 * return -1 if point outside polygon
 * return 0 if point on boundary
 * return 1 if point inside polygon
 *
 * Expected **root order is each exterior ring followed by its holes, eg. EIIEIIEI
 */
static int
point_in_multipolygon_rtree(RTREE_NODE **root, int polyCount, int *ringCounts, LWPOINT *point)
{
	int i, p, r, in_ring;
	const POINT2D *pt;
	int result = -1;

	POSTGIS_DEBUGF(2, "point_in_multipolygon_rtree called for %p %d %p.", root, polyCount, point);

	/* empty is not within anything */
	if (lwpoint_is_empty(point)) return -1;

	pt = getPoint2d_cp(point->point, 0);

	/* assume bbox short-circuit has already been attempted */

	i = 0; /* the current index into the root array */

	/* is the point inside any of the sub-polygons? */
	for ( p = 0; p < polyCount; p++ )
	{
		/* Skip empty polygons */
		if( ringCounts[p] == 0 ) continue;

		in_ring = point_in_ring_rtree(root[i], pt);
		POSTGIS_DEBUGF(4, "point_in_multipolygon_rtree: exterior ring (%d), point_in_ring returned %d", p, in_ring);
		if ( in_ring == -1 ) /* outside the exterior ring */
		{
			POSTGIS_DEBUG(3, "point_in_multipolygon_rtree: outside exterior ring.");
		}
		else if ( in_ring == 0 ) /* on the boundary */
		{
			POSTGIS_DEBUGF(3, "point_in_multipolygon_rtree: on edge of exterior ring %d", p);
			return 0;
		}
		else
		{
			result = in_ring;

			for(r = 1; r < ringCounts[p]; r++)
			{
				in_ring = point_in_ring_rtree(root[i+r], pt);
				POSTGIS_DEBUGF(4, "point_in_multipolygon_rtree: interior ring (%d), point_in_ring returned %d", r, in_ring);
				if (in_ring == 1) /* inside a hole => outside the polygon */
				{
					POSTGIS_DEBUGF(3, "point_in_multipolygon_rtree: within hole %d of exterior ring %d", r, p);
					result = -1;
					break;
				}
				if (in_ring == 0) /* on the edge of a hole */
				{
					POSTGIS_DEBUGF(3, "point_in_multipolygon_rtree: on edge of hole %d of exterior ring %d", r, p);
					return 0;
				}
			}
			/* if we have a positive result, we can short-circuit and return it */
			if (result != -1)
			{
				return result;
			}
		}
		/* increment the index by the total number of rings in the sub-poly */
		/* we do this here in case we short-cutted out of the poly before looking at all the rings */
		i += ringCounts[p];
	}

	return result; /* -1 = outside, 0 = boundary, 1 = inside */
}


/* Utility function that checks a LWPOINT and a GSERIALIZED poly against a cache.
 * Serialized poly may be a multipart.
 */
int
pip_short_circuit(RTREE_POLY_CACHE *poly_cache, LWPOINT *point, const GSERIALIZED *gpoly)
{
	int result;

	if (poly_cache && poly_cache->ringIndices)
	{
		result = point_in_multipolygon_rtree(poly_cache->ringIndices, poly_cache->polyCount, poly_cache->ringCounts, point);
	}
	else
	{
		LWGEOM* poly = lwgeom_from_gserialized(gpoly);
		if (lwgeom_get_type(poly) == POLYGONTYPE)
		{
			result = point_in_polygon(lwgeom_as_lwpoly(poly), point);
		}
		else
		{
			result = point_in_multipolygon(lwgeom_as_lwmpoly(poly), point);
		}
		lwgeom_free(poly);
	}
	return result;
}

