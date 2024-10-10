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
 * Copyright 2023 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 **********************************************************************/

#pragma once

#include "liblwgeom_internal.h"

#define ITREE_MAX_NODES 4

typedef enum
{
	ITREE_INSIDE = 1,
	ITREE_BOUNDARY = 0, /* any boundary case stops calculations */
	ITREE_OUTSIDE = -1,
	ITREE_OK = 2        /* calculations may continue */
} IntervalTreeResult;

typedef struct IntervalTreeNode
{
	double min;
	double max;
	struct IntervalTreeNode *children[ITREE_MAX_NODES];
	uint32_t edgeIndex;
	uint32_t numChildren;
} IntervalTreeNode;

/*
 * A multi-polygon is converted into an interval tree for
 * searching, with one indexed ring in the index for each
 * (non-empty, non-degenerate) ring in the input.
 * The indexes are arranged in order they are read off
 * the multipolygon, so (P0R0, P0R1, P1R0, P1R1) etc.
 * The ringCounts allow hopping to the exterior rings.
 */
typedef struct IntervalTree
{
	struct IntervalTreeNode *nodes;    /* store nodes here in flat array */
	struct IntervalTreeNode **indexes; /* store pointers to index root nodes here */
	POINTARRAY **indexArrays;          /* store original ring data here */
	uint32_t numIndexes;               /* utilization of the indexes/indexArrays */
	uint32_t *ringCounts;              /* number of rings in each polygon */
	uint32_t numPolys;                 /* number of polygons */
	uint32_t numNodes;                 /* utilization of the nodes array */
	uint32_t maxNodes;                 /* capacity of the nodes array */
} IntervalTree;

/*
 * Public prototypes
 */
IntervalTree *itree_from_lwgeom(const LWGEOM *geom);
void itree_free(IntervalTree *itree);
IntervalTreeResult itree_point_in_multipolygon(const IntervalTree *itree, const LWPOINT *point);



