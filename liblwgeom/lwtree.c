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
 * Copyright (C) 2009-2012 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 **********************************************************************/

#include "liblwgeom_internal.h"
#include "lwgeom_log.h"
#include "lwtree.h"
#include "measures.h"

static inline int
rect_node_is_leaf(const RECT_NODE *node)
{
	return node->type == RECT_NODE_LEAF_TYPE;
}

/*
* Support qsort of nodes for collection/multi types so nodes
* are in "spatial adjacent" order prior to merging.
*/
static int
rect_node_cmp(const void *pn1, const void *pn2)
{
	GBOX b1, b2;
	RECT_NODE *n1 = *((RECT_NODE**)pn1);
	RECT_NODE *n2 = *((RECT_NODE**)pn2);
	uint64_t h1, h2;
	b1.flags = 0;
	b1.xmin = n1->xmin;
	b1.xmax = n1->xmax;
	b1.ymin = n1->ymin;
	b1.ymax = n1->ymax;

	b2.flags = 0;
	b2.xmin = n2->xmin;
	b2.xmax = n2->xmax;
	b2.ymin = n2->ymin;
	b2.ymax = n2->ymax;

	h1 = gbox_get_sortable_hash(&b1, 0);
	h2 = gbox_get_sortable_hash(&b2, 0);
	return h1 < h2 ? -1 : (h1 > h2 ? 1 : 0);
}

/**
* Recurse from top of node tree and free all children.
* does not free underlying point array.
*/
void
rect_tree_free(RECT_NODE *node)
{
	int i;
	if (!node) return;
	if (!rect_node_is_leaf(node))
	{
		for (i = 0; i < node->i.num_nodes; i++)
		{
			rect_tree_free(node->i.nodes[i]);
			node->i.nodes[i] = NULL;
		}
	}
	lwfree(node);
}

static int
rect_leaf_node_intersects(RECT_NODE_LEAF *n1, RECT_NODE_LEAF *n2)
{
	const POINT2D *p1, *p2, *p3, *q1, *q2, *q3;
	DISTPTS dl;
	lw_dist2d_distpts_init(&dl, 1);
	switch (n1->seg_type)
	{
		case RECT_NODE_SEG_POINT:
		{
			p1 = getPoint2d_cp(n1->pa, n1->seg_num);

			switch (n2->seg_type)
			{
				case RECT_NODE_SEG_POINT:
					q1 = getPoint2d_cp(n2->pa, n2->seg_num);
					lw_dist2d_pt_pt(q1, p1, &dl);
					return dl.distance == 0.0;

				case RECT_NODE_SEG_LINEAR:
					q1 = getPoint2d_cp(n2->pa, n2->seg_num);
					q2 = getPoint2d_cp(n2->pa, n2->seg_num+1);
					lw_dist2d_pt_seg(p1, q1, q2, &dl);
					return dl.distance == 0.0;

				case RECT_NODE_SEG_CIRCULAR:
					q1 = getPoint2d_cp(n2->pa, n2->seg_num*2);
					q2 = getPoint2d_cp(n2->pa, n2->seg_num*2+1);
					q3 = getPoint2d_cp(n2->pa, n2->seg_num*2+2);
					lw_dist2d_pt_arc(p1, q1, q2, q3, &dl);
					return dl.distance == 0.0;

				default:
					lwerror("%s: unsupported segment type", __func__);
					break;
			}

			break;
		}

		case RECT_NODE_SEG_LINEAR:
		{
			p1 = getPoint2d_cp(n1->pa, n1->seg_num);
			p2 = getPoint2d_cp(n1->pa, n1->seg_num+1);

			switch (n2->seg_type)
			{
				case RECT_NODE_SEG_POINT:
					q1 = getPoint2d_cp(n2->pa, n2->seg_num);
					lw_dist2d_pt_seg(q1, p1, p2, &dl);
					return dl.distance == 0.0;

				case RECT_NODE_SEG_LINEAR:
					q1 = getPoint2d_cp(n2->pa, n2->seg_num);
					q2 = getPoint2d_cp(n2->pa, n2->seg_num+1);
					return lw_segment_intersects(p1, p2, q1, q2) > 0;

				case RECT_NODE_SEG_CIRCULAR:
					q1 = getPoint2d_cp(n2->pa, n2->seg_num*2);
					q2 = getPoint2d_cp(n2->pa, n2->seg_num*2+1);
					q3 = getPoint2d_cp(n2->pa, n2->seg_num*2+2);
					lw_dist2d_seg_arc(p1, p2, q1, q2, q3, &dl);
					return dl.distance == 0.0;

				default:
					lwerror("%s: unsupported segment type", __func__);
					break;
			}

			break;
		}
		case RECT_NODE_SEG_CIRCULAR:
		{
			p1 = getPoint2d_cp(n1->pa, n1->seg_num*2);
			p2 = getPoint2d_cp(n1->pa, n1->seg_num*2+1);
			p3 = getPoint2d_cp(n1->pa, n1->seg_num*2+2);

			switch (n2->seg_type)
			{
				case RECT_NODE_SEG_POINT:
					q1 = getPoint2d_cp(n2->pa, n2->seg_num);
					lw_dist2d_pt_arc(q1, p1, p2, p3, &dl);
					return dl.distance == 0.0;

				case RECT_NODE_SEG_LINEAR:
					q1 = getPoint2d_cp(n2->pa, n2->seg_num);
					q2 = getPoint2d_cp(n2->pa, n2->seg_num+1);
					lw_dist2d_seg_arc(q1, q2, p1, p2, p3, &dl);
					return dl.distance == 0.0;

				case RECT_NODE_SEG_CIRCULAR:
					q1 = getPoint2d_cp(n2->pa, n2->seg_num*2);
					q2 = getPoint2d_cp(n2->pa, n2->seg_num*2+1);
					q3 = getPoint2d_cp(n2->pa, n2->seg_num*2+2);
					lw_dist2d_arc_arc(p1, p2, p3, q1, q2, q3, &dl);
					return dl.distance == 0.0;

				default:
					lwerror("%s: unsupported segment type", __func__);
					break;
			}

			break;
		}
		default:
			return LW_FALSE;
	}
	return LW_FALSE;
}


/*
* Returns 1 if segment is to the right of point.
*/
static inline int
rect_leaf_node_segment_side(RECT_NODE_LEAF *node, const POINT2D *q, int *on_boundary)
{
	const POINT2D *p1, *p2, *p3;
	switch (node->seg_type)
	{
		case RECT_NODE_SEG_LINEAR:
		{
			int side;
			p1 = getPoint2d_cp(node->pa, node->seg_num);
			p2 = getPoint2d_cp(node->pa, node->seg_num+1);

			side = lw_segment_side(p1, p2, q);

			/* Always note case where we're on boundary */
			if (side == 0 && lw_pt_in_seg(q, p1, p2))
			{
				*on_boundary = LW_TRUE;
				return 0;
			}

			/* Segment points up and point is on left */
			if (p1->y < p2->y && side == -1 && q->y != p2->y)
			{
				return 1;
			}

			/* Segment points down and point is on right */
			if (p1->y > p2->y && side == 1 && q->y != p2->y)
			{
				return 1;
			}

			/* Segment is horizontal, do we cross first point? */
			if (p1->y == p2->y && q->x < p1->x)
			{
				return 1;
			}

			return 0;
		}
		case RECT_NODE_SEG_CIRCULAR:
		{
			int arc_side, seg_side;

			p1 = getPoint2d_cp(node->pa, node->seg_num*2);
			p2 = getPoint2d_cp(node->pa, node->seg_num*2+1);
			p3 = getPoint2d_cp(node->pa, node->seg_num*2+2);

			/* Always note case where we're on boundary */
			arc_side = lw_arc_side(p1, p2, p3, q);
			if (arc_side == 0)
			{
				*on_boundary = LW_TRUE;
				return 0;
			}

			seg_side = lw_segment_side(p1, p3, q);
			if (seg_side == arc_side)
			{
				/* Segment points up and point is on left */
				if (p1->y < p3->y && seg_side == -1 && q->y != p3->y)
				{
					return 1;
				}

				/* Segment points down and point is on right */
				if (p1->y > p3->y && seg_side == 1 && q->y != p3->y)
				{
					return 1;
				}
			}
			else
			{
				/* Segment points up and point is on left */
				if (p1->y < p3->y && seg_side == 1 && q->y != p3->y)
				{
					return 1;
				}

				/* Segment points down and point is on right */
				if (p1->y > p3->y && seg_side == -1 && q->y != p3->y)
				{
					return 1;
				}

				/* Segment is horizontal, do we cross first point? */
				if (p1->y == p3->y)
				{
					return 1;
				}
			}

			return 0;

		}
		default:
		{
			lwerror("%s: unsupported seg_type - %d", __func__, node->seg_type);
			return 0;
		}
	}

	return 0;
}

/*
* Only pass the head of a ring. Either a LinearRing from a polygon
* or a CompoundCurve from a CurvePolygon.
* Takes a horizontal line through the ring, and adds up the
* crossing directions. An "inside" point will be on the same side
* ("both left" or "both right") of the edges the line crosses,
* so it will have a non-zero return sum. An "outside" point will
* be on both sides, and will have a zero return sum.
*/
static int
rect_tree_ring_contains_point(RECT_NODE *node, const POINT2D *pt, int *on_boundary)
{
	/* Only test nodes that straddle our stabline vertically */
	/* and might be to the right horizontally */
	if (node->ymin <= pt->y && pt->y <= node->ymax && pt->x <= node->xmax)
	{
		if (rect_node_is_leaf(node))
		{
			return rect_leaf_node_segment_side(&node->l, pt, on_boundary);
		}
		else
		{
			int i, r = 0;
			for (i = 0; i < node->i.num_nodes; i++)
			{
				r += rect_tree_ring_contains_point(node->i.nodes[i], pt, on_boundary);
			}
			return r;
		}
	}
	return 0;
}

/*
* Only pass in the head of an "area" type. Polygon or CurvePolygon.
* Sums up containment of exterior (+1) and interior (-1) rings, so
* that zero is uncontained, +1 is contained and negative is an error
* (multiply contained by interior rings?)
*/
static int
rect_tree_area_contains_point(RECT_NODE *node, const POINT2D *pt)
{
	/* Can't do anything with a leaf node, makes no sense */
	if (rect_node_is_leaf(node))
		return 0;

	/* Iterate into area until we find ring heads */
	if (node->i.ring_type == RECT_NODE_RING_NONE)
	{
		int i, sum = 0;
		for (i = 0; i < node->i.num_nodes; i++)
			sum += rect_tree_area_contains_point(node->i.nodes[i], pt);
		return sum;
	}
	/* See if the ring encloses the point */
	else
	{
		int on_boundary = 0;
		int edge_crossing_count = rect_tree_ring_contains_point(node, pt, &on_boundary);
		/* Odd number of crossings => contained */
		int contained = (edge_crossing_count % 2 == 1);
		/* External rings return positive containment, interior ones negative, */
		/* so that a point-in-hole case nets out to zero (contained by both */
		/* interior and exterior rings. */
		if (node->i.ring_type == RECT_NODE_RING_INTERIOR)
		{
			return on_boundary ? 0 : -1 * contained;
		}
		else
		{
			return contained || on_boundary;
		}

	}
}

/*
* Simple containment test for node/point inputs
*/
static int
rect_node_bounds_point(RECT_NODE *node, const POINT2D *pt)
{
	if (pt->y < node->ymin || pt->y > node->ymax ||
		pt->x < node->xmin || pt->x > node->xmax)
		return LW_FALSE;
	else
		return LW_TRUE;
}

/*
* Pass in arbitrary tree, get back true if point is contained or on boundary,
* and false otherwise.
*/
int
rect_tree_contains_point(RECT_NODE *node, const POINT2D *pt)
{
	int i, c;

	/* Object cannot contain point if bounds don't */
	if (!rect_node_bounds_point(node, pt))
		return 0;

	switch (node->geom_type)
	{
		case POLYGONTYPE:
		case CURVEPOLYTYPE:
			return rect_tree_area_contains_point(node, pt) > 0;

		case MULTIPOLYGONTYPE:
		case MULTISURFACETYPE:
		case COLLECTIONTYPE:
		{
			for (i = 0; i < node->i.num_nodes; i++)
			{
				c = rect_tree_contains_point(node->i.nodes[i], pt);
				if (c) return LW_TRUE;
			}
			return LW_FALSE;
		}

		default:
			return LW_FALSE;
	}
}

/*
* For area types, doing intersects and distance, we will
* need to do a point-in-poly test first to find the full-contained
* case where an intersection exists without any edges actually
* intersecting.
*/
static int
rect_tree_is_area(const RECT_NODE *node)
{
	switch (node->geom_type)
	{
		case POLYGONTYPE:
		case CURVEPOLYTYPE:
		case MULTISURFACETYPE:
			return LW_TRUE;

		case COLLECTIONTYPE:
		{
			if (rect_node_is_leaf(node))
				return LW_FALSE;
			else
			{
				int i;
				for (i = 0; i < node->i.num_nodes; i++)
				{
					if (rect_tree_is_area(node->i.nodes[i]))
						return LW_TRUE;
				}
				return LW_FALSE;
			}
		}

		default:
			return LW_FALSE;
	}
}

static RECT_NODE_SEG_TYPE lwgeomTypeArc[] =
{
	RECT_NODE_SEG_UNKNOWN,  // "Unknown"
	RECT_NODE_SEG_POINT,    // "Point"
	RECT_NODE_SEG_LINEAR,   // "LineString"
	RECT_NODE_SEG_LINEAR,   // "Polygon"
	RECT_NODE_SEG_UNKNOWN,  // "MultiPoint"
	RECT_NODE_SEG_LINEAR,   // "MultiLineString"
	RECT_NODE_SEG_LINEAR,   // "MultiPolygon"
	RECT_NODE_SEG_UNKNOWN,  // "GeometryCollection"
	RECT_NODE_SEG_CIRCULAR, // "CircularString"
	RECT_NODE_SEG_UNKNOWN,  // "CompoundCurve"
	RECT_NODE_SEG_UNKNOWN,  // "CurvePolygon"
	RECT_NODE_SEG_UNKNOWN,  // "MultiCurve"
	RECT_NODE_SEG_UNKNOWN,  // "MultiSurface"
	RECT_NODE_SEG_LINEAR,   // "PolyhedralSurface"
	RECT_NODE_SEG_LINEAR,   // "Triangle"
	RECT_NODE_SEG_LINEAR    // "Tin"
};

/*
* Create a new leaf node.
*/
static RECT_NODE *
rect_node_leaf_new(const POINTARRAY *pa, int seg_num, int geom_type)
{
	const POINT2D *p1, *p2, *p3;
	RECT_NODE *node;
	GBOX gbox;
	RECT_NODE_SEG_TYPE seg_type = lwgeomTypeArc[geom_type];

	switch (seg_type)
	{
		case RECT_NODE_SEG_POINT:
		{
			p1 = getPoint2d_cp(pa, seg_num);
			gbox.xmin = gbox.xmax = p1->x;
			gbox.ymin = gbox.ymax = p1->y;
			break;
		}

		case RECT_NODE_SEG_LINEAR:
		{
			p1 = getPoint2d_cp(pa, seg_num);
			p2 = getPoint2d_cp(pa, seg_num+1);
			/* Zero length edge, doesn't get a node */
			if ((p1->x == p2->x) && (p1->y == p2->y))
				return NULL;
			gbox.xmin = FP_MIN(p1->x, p2->x);
			gbox.xmax = FP_MAX(p1->x, p2->x);
			gbox.ymin = FP_MIN(p1->y, p2->y);
			gbox.ymax = FP_MAX(p1->y, p2->y);
			break;
		}

		case RECT_NODE_SEG_CIRCULAR:
		{
			p1 = getPoint2d_cp(pa, 2*seg_num);
			p2 = getPoint2d_cp(pa, 2*seg_num+1);
			p3 = getPoint2d_cp(pa, 2*seg_num+2);
			/* Zero length edge, doesn't get a node */
			if ((p1->x == p2->x) && (p2->x == p3->x) &&
				(p1->y == p2->y) && (p2->y == p3->y))
				return NULL;
			lw_arc_calculate_gbox_cartesian_2d(p1, p2, p3, &gbox);
			break;
		}

		default:
		{
			lwerror("%s: unsupported seg_type - %d", __func__, seg_type);
			return NULL;
		}
	}

	node = lwalloc(sizeof(RECT_NODE));
	node->type = RECT_NODE_LEAF_TYPE;
	node->geom_type = geom_type;
	node->xmin = gbox.xmin;
	node->xmax = gbox.xmax;
	node->ymin = gbox.ymin;
	node->ymax = gbox.ymax;
	node->l.seg_num = seg_num;
	node->l.seg_type = seg_type;
	node->l.pa = pa;
	return node;
}


static void
rect_node_internal_add_node(RECT_NODE *node, RECT_NODE *add)
{
	if (rect_node_is_leaf(node))
		lwerror("%s: call on leaf node", __func__);
	node->xmin = FP_MIN(node->xmin, add->xmin);
	node->xmax = FP_MAX(node->xmax, add->xmax);
	node->ymin = FP_MIN(node->ymin, add->ymin);
	node->ymax = FP_MAX(node->ymax, add->ymax);
	node->i.nodes[node->i.num_nodes++] = add;
	return;
}


static RECT_NODE *
rect_node_internal_new(const RECT_NODE *seed)
{
	RECT_NODE *node = lwalloc(sizeof(RECT_NODE));
	node->xmin = seed->xmin;
	node->xmax = seed->xmax;
	node->ymin = seed->ymin;
	node->ymax = seed->ymax;
	node->geom_type = seed->geom_type;
	node->type = RECT_NODE_INTERNAL_TYPE;
	node->i.num_nodes = 0;
	node->i.ring_type = RECT_NODE_RING_NONE;
	node->i.sorted = 0;
	return node;
}

/*
* We expect the incoming nodes to be in a spatially coherent
* order. For incoming nodes derived from point arrays,
* the very fact that they are
* a vertex list implies a reasonable ordering: points nearby in
* the list will be nearby in space. For incoming nodes from higher
* level structures (collections, etc) the caller should sort the
* nodes using a z-order first, so that this merge step results in a
* spatially coherent structure.
*/
static RECT_NODE *
rect_nodes_merge(RECT_NODE ** nodes, uint32_t num_nodes)
{
	if (num_nodes < 1)
	{
		return NULL;
	}

	while (num_nodes > 1)
	{
		uint32_t i, k = 0;
		RECT_NODE *node = NULL;
		for (i = 0; i < num_nodes; i++)
		{
			if (!node)
				node = rect_node_internal_new(nodes[i]);

			rect_node_internal_add_node(node, nodes[i]);

			if (node->i.num_nodes == RECT_NODE_SIZE)
			{
				nodes[k++] = node;
				node = NULL;
			}
		}
		if (node)
			nodes[k++] = node;
		num_nodes = k;
	}

	return nodes[0];
}

/*
* Build a tree of nodes from a point array, one node per edge.
*/
RECT_NODE *
rect_tree_from_ptarray(const POINTARRAY *pa, int geom_type)
{
	int num_edges = 0, i = 0, j = 0;
	RECT_NODE_SEG_TYPE seg_type = lwgeomTypeArc[geom_type];
	RECT_NODE **nodes = NULL;
	RECT_NODE *tree = NULL;

	/* No-op on empty ring/line/pt */
	if ( pa->npoints < 1 )
		return NULL;

	/* For arcs, 3 points per edge, for lines, 2 per edge */
	switch(seg_type)
	{
		case RECT_NODE_SEG_POINT:
			return rect_node_leaf_new(pa, 0, geom_type);
			break;
		case RECT_NODE_SEG_LINEAR:
			num_edges = pa->npoints - 1;
			break;
		case RECT_NODE_SEG_CIRCULAR:
			num_edges = (pa->npoints - 1)/2;
			break;
		default:
			lwerror("%s: unsupported seg_type - %d", __func__, seg_type);
	}

	/* First create a flat list of nodes, one per edge. */
	nodes = lwalloc(sizeof(RECT_NODE*) * num_edges);
	for (i = 0; i < num_edges; i++)
	{
		RECT_NODE *node = rect_node_leaf_new(pa, i, geom_type);
		if (node) /* Not zero length? */
			nodes[j++] = node;
	}

	/* Merge the list into a tree */
	tree = rect_nodes_merge(nodes, j);

	/* Free the old list structure, leaving the tree in place */
	lwfree(nodes);

	/* Return top of tree */
	return tree;
}

LWGEOM *
rect_tree_to_lwgeom(const RECT_NODE *node)
{
	LWGEOM *poly = (LWGEOM*)lwpoly_construct_envelope(0, node->xmin, node->ymin, node->xmax, node->ymax);
	if (rect_node_is_leaf(node))
	{
		return poly;
	}
	else
	{
		int i;
		LWCOLLECTION *col = lwcollection_construct_empty(COLLECTIONTYPE, 0, 0, 0);
		lwcollection_add_lwgeom(col, poly);
		for (i = 0; i < node->i.num_nodes; i++)
		{
			lwcollection_add_lwgeom(col, rect_tree_to_lwgeom(node->i.nodes[i]));
		}
		return (LWGEOM*)col;
	}
}

char *
rect_tree_to_wkt(const RECT_NODE *node)
{
	LWGEOM *geom = rect_tree_to_lwgeom(node);
	char *wkt = lwgeom_to_wkt(geom, WKT_ISO, 12, 0);
	lwgeom_free(geom);
	return wkt;
}

void
rect_tree_printf(const RECT_NODE *node, int depth)
{
	printf("%*s----\n", depth, "");
	printf("%*stype: %d\n", depth, "", node->type);
	printf("%*sgeom_type: %d\n", depth, "", node->geom_type);
	printf("%*sbox: %g %g, %g %g\n", depth, "", node->xmin, node->ymin, node->xmax, node->ymax);
	if (node->type == RECT_NODE_LEAF_TYPE)
	{
		printf("%*sseg_type: %d\n", depth, "", node->l.seg_type);
		printf("%*sseg_num: %d\n", depth, "", node->l.seg_num);
	}
	else
	{
		int i;
		for (i = 0; i < node->i.num_nodes; i++)
		{
			rect_tree_printf(node->i.nodes[i], depth+2);
		}
	}
}

static RECT_NODE *
rect_tree_from_lwpoint(const LWGEOM *lwgeom)
{
	const LWPOINT *lwpt = (const LWPOINT*)lwgeom;
	return rect_tree_from_ptarray(lwpt->point, lwgeom->type);
}

static RECT_NODE *
rect_tree_from_lwline(const LWGEOM *lwgeom)
{
	const LWLINE *lwline = (const LWLINE*)lwgeom;
	return rect_tree_from_ptarray(lwline->points, lwgeom->type);
}

static RECT_NODE *
rect_tree_from_lwpoly(const LWGEOM *lwgeom)
{
	RECT_NODE **nodes;
	RECT_NODE *tree;
	uint32_t i, j = 0;
	const LWPOLY *lwpoly = (const LWPOLY*)lwgeom;

	if (lwpoly->nrings < 1)
		return NULL;

	nodes = lwalloc(sizeof(RECT_NODE*) * lwpoly->nrings);
	for (i = 0; i < lwpoly->nrings; i++)
	{
		RECT_NODE *node = rect_tree_from_ptarray(lwpoly->rings[i], lwgeom->type);
		if (node)
		{
			node->i.ring_type = i ? RECT_NODE_RING_INTERIOR : RECT_NODE_RING_EXTERIOR;
			nodes[j++] = node;
		}
	}
	tree = rect_nodes_merge(nodes, j);
	tree->geom_type = lwgeom->type;
	lwfree(nodes);
	return tree;
}

static RECT_NODE *
rect_tree_from_lwcurvepoly(const LWGEOM *lwgeom)
{
	RECT_NODE **nodes;
	RECT_NODE *tree;
	uint32_t i, j = 0;
	const LWCURVEPOLY *lwcol = (const LWCURVEPOLY*)lwgeom;

	if (lwcol->nrings < 1)
		return NULL;

	nodes = lwalloc(sizeof(RECT_NODE*) * lwcol->nrings);
	for (i = 0; i < lwcol->nrings; i++)
	{
		RECT_NODE *node = rect_tree_from_lwgeom(lwcol->rings[i]);
		if (node)
		{
			/*
			* In the case of arc circle, it's possible for a ring to consist
			* of a single closed edge. That will arrive as a leaf node. We
			* need to wrap that node in an internal node with an appropriate
			* ring type so all the other code can try and make sense of it.
			*/
			if (node->type == RECT_NODE_LEAF_TYPE)
			{
				RECT_NODE *internal = rect_node_internal_new(node);
				rect_node_internal_add_node(internal, node);
				node = internal;
			}
			/* Each subcomponent is a ring */
			node->i.ring_type = i ? RECT_NODE_RING_INTERIOR : RECT_NODE_RING_EXTERIOR;
			nodes[j++] = node;
		}
	}
	/* Put the top nodes in a z-order curve for a spatially coherent */
	/* tree after node merge */
	qsort(nodes, j, sizeof(RECT_NODE*), rect_node_cmp);

	tree = rect_nodes_merge(nodes, j);

	tree->geom_type = lwgeom->type;
	lwfree(nodes);
	return tree;

}

static RECT_NODE *
rect_tree_from_lwcollection(const LWGEOM *lwgeom)
{
	RECT_NODE **nodes;
	RECT_NODE *tree;
	uint32_t i, j = 0;
	const LWCOLLECTION *lwcol = (const LWCOLLECTION*)lwgeom;

	if (lwcol->ngeoms < 1)
		return NULL;

	/* Build one tree for each sub-geometry, then below */
	/* we merge the root notes of those trees to get a single */
	/* top node for the collection */
	nodes = lwalloc(sizeof(RECT_NODE*) * lwcol->ngeoms);
	for (i = 0; i < lwcol->ngeoms; i++)
	{
		RECT_NODE *node = rect_tree_from_lwgeom(lwcol->geoms[i]);
		if (node)
		{
			/* Curvepolygons are collections where the sub-geometries */
			/* are the rings, and will need to doint point-in-poly */
			/* tests in order to do intersects and distance calculations */
			/* correctly */
			if (lwgeom->type == CURVEPOLYTYPE)
				node->i.ring_type = i ? RECT_NODE_RING_INTERIOR : RECT_NODE_RING_EXTERIOR;
			nodes[j++] = node;
		}
	}
	/* Sort the nodes using a z-order curve, so that merging the nodes */
	/* gives a spatially coherent tree (near things are in near nodes) */
	/* Note: CompoundCurve has edges already spatially organized, no */
	/* sorting needed */
	if (lwgeom->type != COMPOUNDTYPE)
		qsort(nodes, j, sizeof(RECT_NODE*), rect_node_cmp);

	tree = rect_nodes_merge(nodes, j);

	tree->geom_type = lwgeom->type;
	lwfree(nodes);
	return tree;
}

RECT_NODE *
rect_tree_from_lwgeom(const LWGEOM *lwgeom)
{
	switch(lwgeom->type)
	{
		case POINTTYPE:
			return rect_tree_from_lwpoint(lwgeom);
		case TRIANGLETYPE:
		case CIRCSTRINGTYPE:
		case LINETYPE:
			return rect_tree_from_lwline(lwgeom);
		case POLYGONTYPE:
			return rect_tree_from_lwpoly(lwgeom);
		case CURVEPOLYTYPE:
			return rect_tree_from_lwcurvepoly(lwgeom);
		case COMPOUNDTYPE:
		case MULTICURVETYPE:
		case MULTISURFACETYPE:
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case POLYHEDRALSURFACETYPE:
		case TINTYPE:
		case COLLECTIONTYPE:
			return rect_tree_from_lwcollection(lwgeom);
		default:
			lwerror("%s: Unknown geometry type: %s", __func__, lwtype_name(lwgeom->type));
			return NULL;
	}
	return NULL;
}

/*
* Get an actual coordinate point from a tree to use
* for point-in-polygon testing.
*/
static const POINT2D *
rect_tree_get_point(const RECT_NODE *node)
{
	if (!node) return NULL;
	if (rect_node_is_leaf(node))
		return getPoint2d_cp(node->l.pa, 0);
	else
		return rect_tree_get_point(node->i.nodes[0]);
}

static inline int
rect_node_intersects(const RECT_NODE *n1, const RECT_NODE *n2)
{
	if (n1->xmin > n2->xmax || n2->xmin > n1->xmax ||
		n1->ymin > n2->ymax || n2->ymin > n1->ymax)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

#if POSTGIS_DEBUG_LEVEL >= 4
static char *
rect_node_to_str(const RECT_NODE *n)
{
	char *buf = lwalloc(256);
	snprintf(buf, 256, "(%.9g %.9g,%.9g %.9g)",
		n->xmin, n->ymin, n->xmax, n->ymax);
   	return buf;
}
#endif

/*
* Work down to leaf nodes, until we find a pair of leaf nodes
* that intersect. Prune branches that do not intersect.
*/
static int
rect_tree_intersects_tree_recursive(RECT_NODE *n1, RECT_NODE *n2)
{
	int i, j;
#if POSTGIS_DEBUG_LEVEL >= 4
	char *n1_str = rect_node_to_str(n1);
	char *n2_str = rect_node_to_str(n2);
	LWDEBUGF(4,"n1 %s  n2 %s", n1, n2);
	lwfree(n1_str);
	lwfree(n2_str);
#endif
	/* There can only be an edge intersection if the rectangles overlap */
	if (rect_node_intersects(n1, n2))
	{
		LWDEBUG(4," interaction found");
		/* We can only test for a true intersection if the nodes are both leaf nodes */
		if (rect_node_is_leaf(n1) && rect_node_is_leaf(n2))
		{
			LWDEBUG(4,"  leaf node test");
			/* Check for true intersection */
			return rect_leaf_node_intersects(&n1->l, &n2->l);
		}
		else if (rect_node_is_leaf(n2) && !rect_node_is_leaf(n1))
		{
			for (i = 0; i < n1->i.num_nodes; i++)
			{
				if (rect_tree_intersects_tree_recursive(n1->i.nodes[i], n2))
					return LW_TRUE;
			}
		}
		else if (rect_node_is_leaf(n1) && !rect_node_is_leaf(n2))
		{
			for (i = 0; i < n2->i.num_nodes; i++)
			{
				if (rect_tree_intersects_tree_recursive(n2->i.nodes[i], n1))
					return LW_TRUE;
			}
		}
		else
		{
			for (j = 0; j < n1->i.num_nodes; j++)
			{
				for (i = 0; i < n2->i.num_nodes; i++)
				{
					if (rect_tree_intersects_tree_recursive(n2->i.nodes[i], n1->i.nodes[j]))
						return LW_TRUE;
				}
			}
		}
	}
	return LW_FALSE;
}


int
rect_tree_intersects_tree(RECT_NODE *n1, RECT_NODE *n2)
{
	/*
	* It is possible for an area to intersect another object
	* without any edges intersecting, if the object is fully contained.
	* If that is so, then any point in the object will be contained,
	* so we do a quick point-in-poly test first for those cases
	*/
	if (rect_tree_is_area(n1) &&
		rect_tree_contains_point(n1, rect_tree_get_point(n2)))
	{
		return LW_TRUE;
	}

	if (rect_tree_is_area(n2) &&
		rect_tree_contains_point(n2, rect_tree_get_point(n1)))
	{
		return LW_TRUE;
	}

	/*
	* Not contained, so intersection can only happen if
	* edges actually intersect.
	*/
	return rect_tree_intersects_tree_recursive(n1, n2);
}

/*
* For slightly more speed when doing distance comparisons,
* where we only care if d1 < d2 and not what the actual value
* of d1 or d2 is, we use the squared distance and avoid the
* sqrt()
*/
static inline double
distance_sq(double x1, double y1, double x2, double y2)
{
	double dx = x1-x2;
	double dy = y1-y2;
	return dx*dx + dy*dy;
}

static inline double
distance(double x1, double y1, double x2, double y2)
{
	return sqrt(distance_sq(x1, y1, x2, y2));
}

/*
* The closest any two objects in two nodes can be is the smallest
* distance between the nodes themselves.
*/
static inline double
rect_node_min_distance(const RECT_NODE *n1, const RECT_NODE *n2)
{
	int   left = n1->xmin > n2->xmax;
	int  right = n1->xmax < n2->xmin;
	int bottom = n1->ymin > n2->ymax;
	int    top = n1->ymax < n2->ymin;

	//lwnotice("rect_node_min_distance");

	if (top && left)
		return distance(n1->xmin, n1->ymax, n2->xmax, n2->ymin);
	else if (top && right)
		return distance(n1->xmax, n1->ymax, n2->xmin, n2->ymin);
	else if (bottom && left)
		return distance(n1->xmin, n1->ymin, n2->xmax, n2->ymax);
	else if (bottom && right)
		return distance(n1->xmax, n1->ymin, n2->xmin, n2->ymax);
	else if (left)
		return n1->xmin - n2->xmax;
	else if (right)
		return n2->xmin - n1->xmax;
	else if (bottom)
		return n1->ymin - n2->ymax;
	else if (top)
		return n2->ymin - n1->ymax;
	else
		return 0.0;

	return 0.0;
}

/*
* The furthest apart the objects in two nodes can be is if they
* are at opposite corners of the bbox that contains both nodes
*/
static inline double
rect_node_max_distance(const RECT_NODE *n1, const RECT_NODE *n2)
{
	double xmin = FP_MIN(n1->xmin, n2->xmin);
	double ymin = FP_MIN(n1->ymin, n2->ymin);
	double xmax = FP_MAX(n1->xmax, n2->xmax);
	double ymax = FP_MAX(n1->ymax, n2->ymax);
	double dx = xmax - xmin;
	double dy = ymax - ymin;
	//lwnotice("rect_node_max_distance");
	return sqrt(dx*dx + dy*dy);
}

/*
* Leaf nodes represent individual edges from the original shape.
* As such, they can be either points (if original was a (multi)point)
* two-point straight edges (for linear types), or
* three-point circular arcs (for curvilinear types).
* The distance calculations for each possible combination of primitive
* edges is different, so there's a big case switch in here to match
* up the right combination of inputs to the right distance calculation.
*/
static double
rect_leaf_node_distance(const RECT_NODE_LEAF *n1, const RECT_NODE_LEAF *n2, RECT_TREE_DISTANCE_STATE *state)
{
	const POINT2D *p1, *p2, *p3, *q1, *q2, *q3;
	DISTPTS dl;

	//lwnotice("rect_leaf_node_distance, %d<->%d", n1->seg_num, n2->seg_num);

	lw_dist2d_distpts_init(&dl, DIST_MIN);

	switch (n1->seg_type)
	{
		case RECT_NODE_SEG_POINT:
		{
			p1 = getPoint2d_cp(n1->pa, n1->seg_num);

			switch (n2->seg_type)
			{
				case RECT_NODE_SEG_POINT:
					q1 = getPoint2d_cp(n2->pa, n2->seg_num);
					lw_dist2d_pt_pt(q1, p1, &dl);
					break;

				case RECT_NODE_SEG_LINEAR:
					q1 = getPoint2d_cp(n2->pa, n2->seg_num);
					q2 = getPoint2d_cp(n2->pa, n2->seg_num+1);
					lw_dist2d_pt_seg(p1, q1, q2, &dl);
					break;

				case RECT_NODE_SEG_CIRCULAR:
				q1 = getPoint2d_cp(n2->pa, n2->seg_num*2);
					q2 = getPoint2d_cp(n2->pa, n2->seg_num*2+1);
					q3 = getPoint2d_cp(n2->pa, n2->seg_num*2+2);
					lw_dist2d_pt_arc(p1, q1, q2, q3, &dl);
					break;

				default:
					lwerror("%s: unsupported segment type", __func__);
			}
			break;
		}

		case RECT_NODE_SEG_LINEAR:
		{
			p1 = getPoint2d_cp(n1->pa, n1->seg_num);
			p2 = getPoint2d_cp(n1->pa, n1->seg_num+1);

			switch (n2->seg_type)
			{
				case RECT_NODE_SEG_POINT:
					q1 = getPoint2d_cp(n2->pa, n2->seg_num);
					lw_dist2d_pt_seg(q1, p1, p2, &dl);
					break;

				case RECT_NODE_SEG_LINEAR:
					q1 = getPoint2d_cp(n2->pa, n2->seg_num);
					q2 = getPoint2d_cp(n2->pa, n2->seg_num+1);
					lw_dist2d_seg_seg(q1, q2, p1, p2, &dl);
					// lwnotice(
					// 	"%d\tLINESTRING(%g %g,%g %g)\t%d\tLINESTRING(%g %g,%g %g)\t%g\t%g\t%g",
					// 	n1->seg_num,
					// 	p1->x, p1->y, p2->x, p2->y,
					// 	n2->seg_num,
					// 	q1->x, q1->y, q2->x, q2->y,
					// 	dl.distance, state->min_dist, state->max_dist);
					break;

				case RECT_NODE_SEG_CIRCULAR:
					q1 = getPoint2d_cp(n2->pa, n2->seg_num*2);
					q2 = getPoint2d_cp(n2->pa, n2->seg_num*2+1);
					q3 = getPoint2d_cp(n2->pa, n2->seg_num*2+2);
					lw_dist2d_seg_arc(p1, p2, q1, q2, q3, &dl);
					break;

				default:
					lwerror("%s: unsupported segment type", __func__);
			}
			break;
		}
		case RECT_NODE_SEG_CIRCULAR:
		{
			p1 = getPoint2d_cp(n1->pa, n1->seg_num*2);
			p2 = getPoint2d_cp(n1->pa, n1->seg_num*2+1);
			p3 = getPoint2d_cp(n1->pa, n1->seg_num*2+2);

			switch (n2->seg_type)
			{
				case RECT_NODE_SEG_POINT:
					q1 = getPoint2d_cp(n2->pa, n2->seg_num);
					lw_dist2d_pt_arc(q1, p1, p2, p3, &dl);
					break;

				case RECT_NODE_SEG_LINEAR:
					q1 = getPoint2d_cp(n2->pa, n2->seg_num);
					q2 = getPoint2d_cp(n2->pa, n2->seg_num+1);
					lw_dist2d_seg_arc(q1, q2, p1, p2, p3, &dl);
					break;

				case RECT_NODE_SEG_CIRCULAR:
					q1 = getPoint2d_cp(n2->pa, n2->seg_num*2);
					q2 = getPoint2d_cp(n2->pa, n2->seg_num*2+1);
					q3 = getPoint2d_cp(n2->pa, n2->seg_num*2+2);
					lw_dist2d_arc_arc(p1, p2, p3, q1, q2, q3, &dl);
					break;

				default:
					lwerror("%s: unsupported segment type", __func__);
			}
			break;
		}
		default:
			lwerror("%s: unsupported segment type", __func__);
	}

	/* If this is a new global minima, save it */
	if (dl.distance < state->min_dist)
	{
		state->min_dist = dl.distance;
		state->p1 = dl.p1;
		state->p2 = dl.p2;
	}

	return dl.distance;
}


static int
rect_tree_node_sort_cmp(const void *a, const void *b)
{
	RECT_NODE *n1 = *(RECT_NODE**)a;
	RECT_NODE *n2 = *(RECT_NODE**)b;
	if (n1->d < n2->d) return -1;
	else if (n1->d > n2->d) return 1;
	else return 0;
}

/* Calculate the center of a node */
static inline void
rect_node_to_p2d(const RECT_NODE *n, POINT2D *pt)
{
	pt->x = (n->xmin + n->xmax)/2;
	pt->y = (n->ymin + n->ymax)/2;
}

/*
* (If necessary), sort the children of each node in
* order of their distance from the enter of the other node.
*/
static void
rect_tree_node_sort(RECT_NODE *n1, RECT_NODE *n2)
{
	int i;
	RECT_NODE *n;
	POINT2D c1, c2, c;
	if (!rect_node_is_leaf(n1) && ! n1->i.sorted)
	{
		rect_node_to_p2d(n2, &c2);
		/* Distance of each child from center of other node */
		for (i = 0; i < n1->i.num_nodes; i++)
		{
			n = n1->i.nodes[i];
			rect_node_to_p2d(n, &c);
			n->d = distance2d_sqr_pt_pt(&c2, &c);
		}
		/* Sort the children by distance */
		n1->i.sorted = 1;
		qsort(n1->i.nodes,
		         n1->i.num_nodes,
		         sizeof(RECT_NODE*),
		         rect_tree_node_sort_cmp);
	}
	if (!rect_node_is_leaf(n2) && ! n2->i.sorted)
	{
		rect_node_to_p2d(n1, &c1);
		/* Distance of each child from center of other node */
		for (i = 0; i < n2->i.num_nodes; i++)
		{
			n = n2->i.nodes[i];
			rect_node_to_p2d(n, &c);
			n->d = distance2d_sqr_pt_pt(&c1, &c);
		}
		/* Sort the children by distance */
		n2->i.sorted = 1;
		qsort(n2->i.nodes,
		         n2->i.num_nodes,
		         sizeof(RECT_NODE*),
		         rect_tree_node_sort_cmp);
	}
}

static double
rect_tree_distance_tree_recursive(RECT_NODE *n1, RECT_NODE *n2, RECT_TREE_DISTANCE_STATE *state)
{
	double min, max;

	/* Short circuit if we've already hit the minimum */
	if (state->min_dist < state->threshold || state->min_dist == 0.0)
		return state->min_dist;

	/* If your minimum is greater than anyone's maximum, you can't hold the winner */
	min = rect_node_min_distance(n1, n2);
	if (min > state->max_dist)
	{
		//lwnotice("pruning pair %p, %p", n1, n2);
		LWDEBUGF(4, "pruning pair %p, %p", n1, n2);
		return FLT_MAX;
	}

	/* If your maximum is a new low, we'll use that as our new global tolerance */
	max = rect_node_max_distance(n1, n2);
	if (max < state->max_dist)
		state->max_dist = max;

	/* Both leaf nodes, do a real distance calculation */
	if (rect_node_is_leaf(n1) && rect_node_is_leaf(n2))
	{
		return rect_leaf_node_distance(&n1->l, &n2->l, state);
	}
	/* Recurse into nodes */
	else
	{
		int i, j;
		double d_min = FLT_MAX;
		rect_tree_node_sort(n1, n2);
		if (rect_node_is_leaf(n1) && !rect_node_is_leaf(n2))
		{
			for (i = 0; i < n2->i.num_nodes; i++)
			{
				min = rect_tree_distance_tree_recursive(n1, n2->i.nodes[i], state);
				d_min = FP_MIN(d_min, min);
			}
		}
		else if (rect_node_is_leaf(n2) && !rect_node_is_leaf(n1))
		{
			for (i = 0; i < n1->i.num_nodes; i++)
			{
				min = rect_tree_distance_tree_recursive(n1->i.nodes[i], n2, state);
				d_min = FP_MIN(d_min, min);
			}
		}
		else
		{
			for (i = 0; i < n1->i.num_nodes; i++)
			{
				for (j = 0; j < n2->i.num_nodes; j++)
				{
					min = rect_tree_distance_tree_recursive(n1->i.nodes[i], n2->i.nodes[j], state);
					d_min = FP_MIN(d_min, min);
				}
			}
		}
		return d_min;
	}
}

double rect_tree_distance_tree(RECT_NODE *n1, RECT_NODE *n2, double threshold)
{
	double distance;
	RECT_TREE_DISTANCE_STATE state;

	/*
	* It is possible for an area to intersect another object
	* without any edges intersecting, if the object is fully contained.
	* If that is so, then any point in the object will be contained,
	* so we do a quick point-in-poly test first for those cases
	*/
	if (rect_tree_is_area(n1) &&
		rect_tree_contains_point(n1, rect_tree_get_point(n2)))
	{
		return 0.0;
	}

	if (rect_tree_is_area(n2) &&
		rect_tree_contains_point(n2, rect_tree_get_point(n1)))
	{
		return 0.0;
	}

	state.threshold = threshold;
	state.min_dist = FLT_MAX;
	state.max_dist = FLT_MAX;
	distance = rect_tree_distance_tree_recursive(n1, n2, &state);
	// *p1 = state.p1;
	// *p2 = state.p2;
	return distance;
}
