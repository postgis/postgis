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
 * ^copyright^
 *
 **********************************************************************/

#ifndef _LWGEOM_RTREE_H
#define _LWGEOM_RTREE_H 1

#include "liblwgeom.h"
#include "lwgeom_cache.h"

/**
* Representation for the y-axis interval spanned by an edge.
*/
typedef struct
{
	double min;
	double max;
}
RTREE_INTERVAL;

/**
* The following struct and methods are used for a 1D RTree implementation,
* described at:
*  http://lin-ear-th-inking.blogspot.com/2007/06/packed-1-dimensional-r-tree.html
*/
typedef struct rtree_node
{
	RTREE_INTERVAL *interval;
	struct rtree_node *leftNode;
	struct rtree_node *rightNode;
	LWLINE *segment;
}
RTREE_NODE;

/**
* The tree structure used for fast P-i-P tests by point_in_multipolygon_rtree()
*/
typedef struct
{
	RTREE_NODE **ringIndices;
	int* ringCounts;
	int polyCount;
} RTREE_POLY_CACHE;


typedef struct
{
	GeomCache             gcache;
	RTREE_POLY_CACHE      *index;
} RTreeGeomCache;

/**
* Retrieves a collection of line segments given the root and crossing value.
*/
LWMLINE *RTreeFindLineSegments(RTREE_NODE *root, double value);


/**
* Checks for a cache hit against the provided geometry and returns
* a pre-built index structure (RTREE_POLY_CACHE) if one exists. Otherwise
* builds a new one and returns that.
*/
RTREE_POLY_CACHE *GetRtreeCache(FunctionCallInfo fcinfo, SHARED_GSERIALIZED *g1);

#endif /* !defined _LWGEOM_RTREE_H */
