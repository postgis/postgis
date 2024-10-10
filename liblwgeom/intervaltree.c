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

#include "intervaltree.h"

void
itree_free(IntervalTree *itree)
{
	if (itree->nodes) lwfree(itree->nodes);
	if (itree->ringCounts) lwfree(itree->ringCounts);
	if (itree->indexArrays)
	{
		for (uint32_t i = 0; i < itree->numIndexes; i++)
		{
			if (itree->indexArrays[i])
				ptarray_free(itree->indexArrays[i]);
		}
	}
	if (itree->indexes) lwfree(itree->indexes);
	if (itree->indexArrays) lwfree(itree->indexArrays);
	lwfree(itree);
}

static uint32_t
itree_num_nodes_pointarray(const POINTARRAY *pa)
{
	uint32_t num_nodes = 0;
	uint32_t level_nodes = 0;

	/* not a closed polygon */
	if (!pa || pa->npoints < 4) return 0;

	/* one leaf node per edge */
	level_nodes = pa->npoints - 1;

	while(level_nodes > 1)
	{
		uint32_t next_level_nodes = level_nodes / ITREE_MAX_NODES;
		next_level_nodes += ((level_nodes % ITREE_MAX_NODES) > 0);
		num_nodes += level_nodes;
		level_nodes = next_level_nodes;
	}
	/* and the root node */
	return num_nodes + 1;
}

static uint32_t
itree_num_nodes_polygon(const LWPOLY *poly)
{
	uint32_t num_nodes = 0;
	for (uint32_t i = 0; i < poly->nrings; i++)
	{
		const POINTARRAY *pa = poly->rings[i];
		num_nodes += itree_num_nodes_pointarray(pa);
	}
	return num_nodes;
}

static uint32_t
itree_num_nodes_multipolygon(const LWMPOLY *mpoly)
{
	uint32_t num_nodes = 0;
	for (uint32_t i = 0; i < mpoly->ngeoms; i++)
	{
		const LWPOLY *poly = mpoly->geoms[i];
		num_nodes += itree_num_nodes_polygon(poly);
	}
	return num_nodes;
}

static uint32_t
itree_num_rings(const LWMPOLY *mpoly)
{
	return lwgeom_count_rings((const LWGEOM*)mpoly);
}

static IntervalTreeNode *
itree_new_node(IntervalTree *itree)
{
	IntervalTreeNode *node = NULL;
	if (itree->numNodes >= itree->maxNodes)
		lwerror("%s ran out of node space", __func__);

	node = &(itree->nodes[itree->numNodes++]);
	node->min = FLT_MAX;
	node->max = FLT_MIN;
	node->edgeIndex = UINT32_MAX;
	node->numChildren = 0;
	memset(node->children, 0, sizeof(node->children));
	return node;
}


static uint32_t // nodes_remaining for after latest merge, stop at 1
itree_merge_nodes(IntervalTree *itree, uint32_t nodes_remaining)
 {
	/*
	 * store the starting state of the tree which gives
	 * us the array of nodes we are going to have to merge
	 */
	uint32_t end_node = itree->numNodes;
	uint32_t start_node = end_node - nodes_remaining;

	/*
	 * one parent for every ITREE_MAX_NODES children
	 * plus one for the remainder
	 */
	uint32_t num_parents = nodes_remaining / ITREE_MAX_NODES;
	num_parents += ((nodes_remaining % ITREE_MAX_NODES) > 0);

	/*
	 * each parent is composed by merging four adjacent children
	 * this generates a useful index structure in O(n) time
	 * because the edges are from a ring, and thus are
	 * spatially autocorrelated, so the pre-sorting step of
	 * building a packed index is already done for us before we even start
	 */
	for (uint32_t i = 0; i < num_parents; i++)
	{
		uint32_t children_start = start_node + i * ITREE_MAX_NODES;
		uint32_t children_end = start_node + (i+1) * ITREE_MAX_NODES;
		children_end = children_end > end_node ? end_node : children_end;

		/*
		 * put pointers to the children we are merging onto
		 * the new parent node
		 */
		IntervalTreeNode *parent_node = itree_new_node(itree);
		for (uint32_t j = children_start; j < children_end; j++)
		{
			IntervalTreeNode *child_node = &(itree->nodes[j]);
			parent_node->min = FP_MIN(child_node->min, parent_node->min);
			parent_node->max = FP_MAX(child_node->max, parent_node->max);
			parent_node->edgeIndex = FP_MIN(child_node->edgeIndex, parent_node->edgeIndex);
			parent_node->children[parent_node->numChildren++] = child_node;
		}
	}

	/*
	 * keep going until num_parents gets down to one and
	 * we are at the root of the tree
	 */
	return num_parents;
}

static int
itree_edge_invalid(const POINT2D *pt1, const POINT2D *pt2)
{
	/* zero length */
	if (pt1->x == pt2->x && pt1->y == pt2->y)
		return 1;

	/* nan/inf coordinates */
	if (isfinite(pt1->x) &&
	    isfinite(pt1->y) &&
	    isfinite(pt2->x) &&
	    isfinite(pt2->y))
		return 0;

	return 1;
}

static void
itree_add_pointarray(IntervalTree *itree, const POINTARRAY *pa)
{
	uint32_t nodes_remaining = 0;
	uint32_t leaf_nodes = 0;
	IntervalTreeNode *root = NULL;

	/* EMPTY/unusable ring */
	if (!pa || pa->npoints < 4)
		lwerror("%s called with unusable ring", __func__);

	/* fill in the leaf nodes */
	for (uint32_t i = 0; i < pa->npoints-1; i++)
	{
		const POINT2D *pt1 = getPoint2d_cp(pa, i);
		const POINT2D *pt2 = getPoint2d_cp(pa, i+1);

		/* Do not add nodes for zero length segments */
		if (itree_edge_invalid(pt1, pt2))
			continue;

		/* get a fresh node for each segment of the ring */
		IntervalTreeNode *node = itree_new_node(itree);
		node->min = FP_MIN(pt1->y, pt2->y);
		node->max = FP_MAX(pt1->y, pt2->y);
		node->edgeIndex = i;
		leaf_nodes++;
	}

	/* merge leaf nodes up to parents */
	nodes_remaining = leaf_nodes;
	while (nodes_remaining > 1)
		nodes_remaining = itree_merge_nodes(itree, nodes_remaining);

	/* final parent is the root */
	if (leaf_nodes > 0)
		root = &(itree->nodes[itree->numNodes - 1]);
	else
		root = NULL;

	/*
	 * take a copy of the point array we built this
	 * tree on top of so we can reference it to get
	 * segment information later
	 */
	itree->indexes[itree->numIndexes] = root;
	itree->indexArrays[itree->numIndexes] = ptarray_clone(pa);
	itree->numIndexes += 1;

	return;
}


static void
itree_add_polygon(IntervalTree *itree, const LWPOLY *poly)
{
	if (poly->nrings == 0) return;

	itree->maxNodes = itree_num_nodes_polygon(poly);
	itree->nodes = lwalloc0(itree->maxNodes * sizeof(IntervalTreeNode));
	itree->numNodes = 0;

	itree->ringCounts = lwalloc0(sizeof(uint32_t));
	itree->indexes = lwalloc0(poly->nrings * sizeof(IntervalTreeNode*));
	itree->indexArrays = lwalloc0(poly->nrings * sizeof(POINTARRAY*));

	for (uint32_t j = 0; j < poly->nrings; j++)
	{
		const POINTARRAY *pa = poly->rings[j];

		/* skip empty/unclosed/invalid rings */
		if (!pa || pa->npoints < 4)
			continue;

		itree_add_pointarray(itree, pa);

		itree->ringCounts[itree->numPolys] += 1;
	}
	itree->numPolys = 1;
	return;
}


static void
itree_add_multipolygon(IntervalTree *itree, const LWMPOLY *mpoly)
{
	if (mpoly->ngeoms == 0) return;

	itree->maxNodes = itree_num_nodes_multipolygon(mpoly);
	itree->nodes = lwalloc0(itree->maxNodes * sizeof(IntervalTreeNode));
	itree->numNodes = 0;

	itree->ringCounts = lwalloc0(mpoly->ngeoms * sizeof(uint32_t));
	itree->indexes = lwalloc0(itree_num_rings(mpoly) * sizeof(IntervalTreeNode*));
	itree->indexArrays = lwalloc0(itree_num_rings(mpoly) * sizeof(POINTARRAY*));

	for (uint32_t i = 0; i < mpoly->ngeoms; i++)
	{
		const LWPOLY *poly = mpoly->geoms[i];

		/* skip empty polygons */
		if (! poly || lwpoly_is_empty(poly))
			continue;

		for (uint32_t j = 0; j < poly->nrings; j++)
		{
			const POINTARRAY *pa = poly->rings[j];

			/* skip empty/unclosed/invalid rings */
			if (!pa || pa->npoints < 4)
				continue;

			itree_add_pointarray(itree, pa);

			itree->ringCounts[itree->numPolys] += 1;
		}

		itree->numPolys += 1;
	}
	return;
}


IntervalTree *
itree_from_lwgeom(const LWGEOM *geom)
{
	if (!geom) lwerror("%s called with null geometry", __func__);
	if (lwgeom_get_type(geom) == MULTIPOLYGONTYPE)
	{
		IntervalTree *itree = lwalloc0(sizeof(IntervalTree));
		itree_add_multipolygon(itree, lwgeom_as_lwmpoly(geom));
		return itree;
	}
	else if (lwgeom_get_type(geom) == POLYGONTYPE)
	{
		IntervalTree *itree = lwalloc0(sizeof(IntervalTree));
		itree_add_polygon(itree, lwgeom_as_lwpoly(geom));
		return itree;
	}
	else
	{
		lwerror("%s got asked to build index on non-polygon", __func__);
		return NULL;
	}
}


/*******************************************************************************
 * The following is based on the "Fast Winding Number Inclusion of a Point
 * in a Polygon" algorithm by Dan Sunday.
 * http://softsurfer.com/Archive/algorithm_0103/algorithm_0103.htm#Winding%20Number
 *
 * returns: >0 for a point to the left of the segment,
 *          <0 for a point to the right of the segment,
 *          0 for a point on the segment
 */
static inline double
itree_segment_side(const POINT2D *seg1, const POINT2D *seg2, const POINT2D *point)
{
	return ((seg2->x - seg1->x) * (point->y - seg1->y) -
		    (point->x - seg1->x) * (seg2->y - seg1->y));
}

/*
 * This function doesn't test that the point falls on the line defined by
 * the two points.  It assumes that that has already been determined
 * by having itree_segment_side return within the tolerance.  It simply checks
 * that if the point is on the line, it is within the endpoints.
 *
 * returns: 1 if the point is inside the segment bounds
 *          0 if the point is outside the segment bounds
 */
static int
itree_point_on_segment(const POINT2D *seg1, const POINT2D *seg2, const POINT2D *point)
{
	double maxX = FP_MAX(seg1->x, seg2->x);
	double maxY = FP_MAX(seg1->y, seg2->y);
	double minX = FP_MIN(seg1->x, seg2->x);
	double minY = FP_MIN(seg1->y, seg2->y);

	return point->x >= minX && point->x <= maxX &&
	       point->y >= minY && point->y <= maxY;
}


static IntervalTreeResult
itree_point_in_ring_recursive(
	const IntervalTreeNode *node,
	const POINTARRAY *pa,
	const POINT2D *pt,
	int *winding_number)
{
	if (!node) return ITREE_OUTSIDE;
	/*
	 * If Y value is not within range of node, we can
	 * learn nothing from this node or its children, so
	 * we exit early.
	 */
	uint8_t node_contains_value = FP_CONTAINS_INCL(node->min, pt->y, node->max) ? 1 : 0;
	if (!node_contains_value)
		return ITREE_OK;

	/* This is a leaf node, so evaluate winding number */
	if (node->numChildren == 0)
	{
		const POINT2D *seg1 = getPoint2d_cp(pa, node->edgeIndex);
		const POINT2D *seg2 = getPoint2d_cp(pa, node->edgeIndex + 1);
		double side = itree_segment_side(seg1, seg2, pt);

		/* Zero length segments are ignored. */
		// xxxx need a unit test, what about really really short segments?
		// if (distance2d_sqr_pt_pt(seg1, seg2) < FP_EPS*FP_EPS)
		// 	return ITREE_OK;

		/* A point on the boundary of a ring is not contained. */
		/* WAS: if (fabs(side) < 1e-12), see ticket #852 */
		if (side == 0.0 && itree_point_on_segment(seg1, seg2, pt) == 1)
			return ITREE_BOUNDARY;

		/*
		 * If the point is to the left of the line, and it's rising,
		 * then the line is to the right of the point and
		 * circling counter-clockwise, so increment.
		 */
		if ((seg1->y <= pt->y) && (pt->y < seg2->y) && (side > 0))
		{
			*winding_number = *winding_number + 1;
		}

		/*
		 * If the point is to the right of the line, and it's falling,
		 * then the line is to the right of the point and circling
		 * clockwise, so decrement.
		 */
		else if ((seg2->y <= pt->y) && (pt->y < seg1->y) && (side < 0))
		{
			*winding_number = *winding_number - 1;
		}

		return ITREE_OK;
	}
	/* This is an interior node, so recurse downwards */
	else
	{
		for (uint32_t i = 0; i < node->numChildren; i++)
		{
			IntervalTreeResult rslt = itree_point_in_ring_recursive(node->children[i], pa, pt, winding_number);
			/* Short circuit and send back boundary result */
			if (rslt == ITREE_BOUNDARY)
				return rslt;
		}
	}
	return ITREE_OK;
}

static IntervalTreeResult
itree_point_in_ring(const IntervalTree *itree, uint32_t ringNumber, const POINT2D *pt)
{
	int winding_number = 0;
	const IntervalTreeNode *node = itree->indexes[ringNumber];
	const POINTARRAY *pa = itree->indexArrays[ringNumber];
	IntervalTreeResult rslt = itree_point_in_ring_recursive(node, pa, pt, &winding_number);

	/* Boundary case is separate from winding number */
	if (rslt == ITREE_BOUNDARY) return rslt;

	/* Not boundary, so evaluate winding number */
	if (winding_number == 0)
		return ITREE_OUTSIDE;
	else
		return ITREE_INSIDE;
}


/*
 * Test if the given point falls within the given multipolygon.
 * Assume bbox short-circuit has already been attempted.
 * First check if point is within any of the outer rings.
 * If not, it is outside. If so, check if the point is
 * within the relevant inner rings. If not, it is inside.
 */
IntervalTreeResult
itree_point_in_multipolygon(const IntervalTree *itree, const LWPOINT *point)
{
	uint32_t i = 0;
	const POINT2D *pt;
	IntervalTreeResult result = ITREE_OUTSIDE;

	/* Empty is not within anything */
	if (lwpoint_is_empty(point))
		return ITREE_OUTSIDE;

	/* Non-finite point is within anything */
	pt = getPoint2d_cp(point->point, 0);
	if (!pt || !(isfinite(pt->x) && isfinite(pt->y)))
		return ITREE_OUTSIDE;

	/* Is the point inside any of the exterior rings of the sub-polygons? */
	for (uint32_t p = 0; p < itree->numPolys; p++)
	{
		uint32_t ringCount = itree->ringCounts[p];

		/* Skip empty polygons */
		if (ringCount == 0) continue;

		/* Check result against exterior ring. */
		result = itree_point_in_ring(itree, i, pt);

		/* Boundary condition is a hard stop */
		if (result == ITREE_BOUNDARY)
			return ITREE_BOUNDARY;

		/* We are inside an exterior ring! Are we outside all the holes? */
		if (result == ITREE_INSIDE)
		{
			for(uint32_t r = 1; r < itree->ringCounts[p]; r++)
			{
				result = itree_point_in_ring(itree, i+r, pt);

				/* Boundary condition is a hard stop */
				if (result == ITREE_BOUNDARY)
					return ITREE_BOUNDARY;

				/*
				 * Inside a hole => Outside the polygon!
				 * But there might be other polygons lurking
				 * inside this hole so we have to continue
				 * and check all the exterior rings.
				 */
				if (result == ITREE_INSIDE)
					goto holes_done;
			}
			return ITREE_INSIDE;
		}

		holes_done:
			/* Move to first ring of next polygon */
			i += ringCount;
	}

	/* Not in any rings */
	return ITREE_OUTSIDE;
}




