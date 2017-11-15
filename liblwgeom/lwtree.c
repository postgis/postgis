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

static inline int
rect_node_is_leaf(const RECT_NODE *node)
{
	return node->type == RECT_NODE_LEAF;
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

static const char *
rect_node_to_str(const RECT_NODE *n)
{
	char buf[256];
	snprintf(buf, 256, "(%.9g %.9g,%.9g %.9g)",
		n->xmin, n->ymin, n->xmax, n->ymax);
	return buf;
}

/*
* Support qsort of nodes for collection/multi types so nodes
* are in "spatial adjacent" order prior to merging.
*/
static int
rect_node_cmp(const void *pn1, const void *pn2)
{
	GBOX b1, b2;
	RECT_NODE *n1 = *pn1;
	RECT_NODE *n2 = *pn2;
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

	h1 = gbox_get_sortable_hash(&b1);
	h2 = gbox_get_sortable_hash(&b2);
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
rect_leaf_node_intersects(const RECT_NODE_LEAF *n1, const RECT_NODE_LEAF *n2)
{
	const POINT2D *p1, *p2, *p3, *q1, *q2, *q3;
	DISTPTS dl;

	switch (n1->seg_type)
	{
		case RECT_NODE_SEG_LINEAR:
		{
			p1 = getPoint2d_cp(n1->pa, n1->seg_num);
			p2 = getPoint2d_cp(n1->pa, n1->seg_num+1);

			switch (n2->seg_type)
			{
				case RECT_NODE_SEG_LINEAR:
					q1 = getPoint2d_cp(n2->pa, n2->seg_num);
					q2 = getPoint2d_cp(n2->pa, n2->seg_num+1);
					return lw_segment_intersects(p1, p2, q1, q2)) > 0;

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
		}
		case RECT_NODE_SEG_CIRCULAR:
		{
			p1 = getPoint2d_cp(n1->pa, n1->seg_num*2);
			p2 = getPoint2d_cp(n1->pa, n1->seg_num*2+1);
			p3 = getPoint2d_cp(n1->pa, n1->seg_num*2+2);

			switch (n2->seg_type)
			{
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
		}
	}
	return LW_FALSE;
}

/*
* Assumes that we have already tested that q is inside the
* ymin/ymax range of the node
*/
static inline int
rect_leaf_node_segment_side(const RECT_NODE_LEAF *node, const POINT2D *q, int *on_boundary)
{
	const POINT2D *p1, *p2, *p3;
	int side = 0;
	switch (node->seg_type)
	{
		case RECT_NODE_SEG_LINEAR:
		{
			p1 = getPoint2d_cp(node->pa, node->seg_num);
			p2 = getPoint2d_cp(node->pa, node->seg_num+1);
			/* Avoid vertex-grazing and through-vertex issues */
			/* by only counting grazings on first vertex */
			if (q->y != p2->y)
			{
				side = lw_segment_side(p1, p2, q);
				if (side == 0 && lw_pt_in_seg(q, p1, p2))
					*on_boundary = LW_TRUE;
			}
			break;
		}
		case RECT_NODE_SEG_CIRCULAR:
		{
			p1 = getPoint2d_cp(node->pa, node->seg_num*2);
			p2 = getPoint2d_cp(node->pa, node->seg_num*2+1);
			p3 = getPoint2d_cp(node->pa, node->seg_num*2+2);
			double ymin = FP_MIN(p1->y, p3->y);
			double ymax = FP_MAX(p1->y, p3->y);
			/* Avoid vertex-grazing and through-vertex issues */
			/* by only counting grazings on first vertex */
			if (ymin <= q->y && q->y <= ymax && q->y != p3->y)
			{
				side = lw_arc_side(p1, p2, p3, q);
				if (side == 0)
					*on_boundary = LW_TRUE;
			}
			break;
		}
		default:
		{
			lwerror("%s: unsupported seg_type - %d", __func__, leaf_node->seg_type);
		}
	}
	LWDEBUGF(2, "%s: seg_type=%d, side=%d, on_boundary=%d",
		__func__, node->seg_type, side, *on_boundary);
	return side;
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
int
rect_tree_ring_contains_point(const RECT_NODE *node, const POINT2D *pt, int *on_boundary)
{
	/* Only test nodes that straddle our stabline */
	if (node->ymin <= pt->y && pt->y <= node->ymax)
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
int
rect_tree_area_contains_point(const RECT_NODE *node, const POINT2D *pt, int *on_boundary)
{

}

/*
* Pass in arbitrary tree, get back true if point is contained or on boundary,
* and false otherwise.
*/
int
rect_tree_contains_point(const RECT_NODE *node, const POINT2D *pt, int *on_boundary)
{

}

static RECT_NODE_SEG_TYPE lwgeomTypeArc[] =
{
	RECT_NODE_SEG_UNKNOWN // "Unknown",
	RECT_NODE_SEG_LINEAR, // "Point",
	RECT_NODE_SEG_LINEAR, // "LineString",
	RECT_NODE_SEG_LINEAR, // "Polygon",
	RECT_NODE_SEG_LINEAR, // "MultiPoint",
	RECT_NODE_SEG_LINEAR, // "MultiLineString",
	RECT_NODE_SEG_LINEAR, // "MultiPolygon",
	RECT_NODE_SEG_UNKNOWN, // "GeometryCollection",
	RECT_NODE_SEG_CIRCULAR, // "CircularString",
	RECT_NODE_SEG_UNKNOWN, // "CompoundCurve",
	RECT_NODE_SEG_UNKNOWN, // "CurvePolygon",
	RECT_NODE_SEG_UNKNOWN, // "MultiCurve",
	RECT_NODE_SEG_UNKNOWN, // "MultiSurface",
	RECT_NODE_SEG_LINEAR, // "PolyhedralSurface",
	RECT_NODE_SEG_LINEAR, // "Triangle",
	RECT_NODE_SEG_LINEAR, // "Tin"
};

/*
* Create a new leaf node.
*/
static RECT_NODE *
rect_node_leaf_new(const POINTARRAY *pa, int seg_num, int geom_type)
{
	POINT2D *p1, *p2, *p3;
	RECT_NODE *node;
	GBOX gbox;
	RECT_NODE_SEG_TYPE seg_type = lwgeomTypeArc[geom_type];

	switch (seg_type)
	{
		case RECT_NODE_SEG_LINEAR:
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
		case RECT_NODE_SEG_CIRCULAR:
			GBOX gbox;
			p1 = getPoint2d_cp(pa, 2*seg_num);
			p2 = getPoint2d_cp(pa, 2*seg_num+1);
			p3 = getPoint2d_cp(pa, 2*seg_num+2);
			/* Zero length edge, doesn't get a node */
			if ((p1->x == p2->x) && (p2->x == p3->x) &&
				(p1->y == p2->y) && (p2->y == p3->y))
				return NULL;
			lw_arc_calculate_gbox_cartesian_2d(p1, p2, p3, &gbox);
			break;
		default:
			lwerror("%s: unsupported seg_type - %d", __func__, seg_type);
			return NULL;
	}

	node = lwalloc(sizeof(RECT_NODE));
	node->type = RECT_NODE_LEAF;
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


static int
rect_node_consistent_type(int type1, int type2)
{
	int mtype1 = lwtype_multitype(type1);
	int mtype2 = lwtype_multitype(type2);
	/* Same? Done. */
	if (type1 == type2) return type1;
	/* promote geom to mgeom */
	if (type1 == mtype2) return mtype2;
	if (type2 == mtype1) return mtype1;
	return COLLECTIONTYPE;
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
	node->type = RECT_NODE_INTERNAL;
	node->i.num_nodes = 0;
	return node;
}


static RECT_NODE *
rect_nodes_merge(RECT_NODE ** nodes, int num_nodes)
{
	if (num_nodes < 1) return NULL;
	while (num_nodes > 1)
	{
		int i, k = 0;
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
		node[k++] = node;
		num_nodes = k;
	}
	return nodes[0];
}

/*
* Build a tree of nodes from a point array, one node per edge.
*/
RECT_NODE *
ptarray_to_rect_tree(const POINTARRAY *pa, int geom_type)
{
	int num_edges = 0, i = 0, j = 0;
	RECT_NODE_SEG_TYPE seg_type = lwgeomTypeArc[geom_type];
	RECT_NODE **nodes = NULL;
	RECT_NODE *tree = NULL;

	/* No-op on empty ring/line/pt */
	if ( pa->npoints < 1 )
		return NULL;

	/* Special handling for POINT */
	if (pa->npoints == 1)
	{
		const POINT2D *pt = getPoint2d_cp(pa, 0);
		RECT_NODE *node = lwalloc(sizeof(RECT_NODE));
		node->geom_type = geom_type;
		node->type = RECT_NODE_LEAF;
		node->xmin = node->xmax = pt->x;
		node->ymin = node->ymax = pt->y;
		node->l.seg_num = 0;
		node->l.seg_type = RECT_NODE_SEG_LINEAR;
		node->l.pa = pa;
		return node;
	}

	/* For arcs, 3 points per edge, for lines, 2 per edge */
	switch(seg_type)
	{
		case RECT_NODE_SEG_LINEAR;
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


static RECT_NODE *
lwpoint_to_rect_tree(LWGEOM *lwgeom)
{
	LWPOINT *lwpt = (LWPOINT*)lwgeom;
	return ptarray_to_rect_tree(lwpt->point, lwgeom->type);
}

static RECT_NODE *
lwline_to_rect_tree(LWGEOM *lwgeom)
{
	LWLINE *lwline = (LWLINE*)lwgeom;
	return ptarray_to_rect_tree(lwline->points, lwgeom->type);
}

static RECT_NODE *
lwpoly_to_rect_tree(LWGEOM *lwgeom)
{
	RECT_NODE **nodes;
	RECT_NODE *tree;
	int i, j = 0;
	LWPOLY *lwpoly = (LWPOLY*)lwgeom;

	if (lwpoly->nrings < 1)
		return NULL;

	nodes = lwalloc(sizeof(RECT_NODE*) * lwpoly->nrings);
	for (i = 0; i < lwpoly->nrings; i++)
	{
		RECT_NODE *node = ptarray_to_rect_tree(lwpoly->rings[i], lwgeom->type);
		if (node)
			nodes[j++] = node;
	}
	tree = rect_nodes_merge(nodes, j);
	tree->geom_type = lwgeom->type;
	lwfree(nodes);
	return tree;
}

static RECT_NODE *
lwcollection_to_rect_tree(LWGEOM *lwgeom)
{
	RECT_NODE **nodes;
	RECT_NODE *tree;
	int i, j = 0;
	LWCOLLECTION *lwcol = (LWCOLLECTION*)lwgeom;

	if (lwcol->ngeoms < 1)
		return NULL;

	nodes = lwalloc(sizeof(RECT_NODE*) * lwcol->ngeoms);
	for (i = 0; i < lwcol->ngeoms; i++)
	{
		RECT_NODE *node = lwgeom_to_rect_tree(lwcol->geoms[i]);
		if (node)
			nodes[j++] = node;
	}
	/* CompoundCurve has edges already spatially organized */
	if (lwgeom->type != COMPOUNDTYPE)
		qsort(nodes, j, sizeof(RECT_NODE*), rect_node_cmp);

	tree = rect_nodes_merge(nodes, j);
	tree->geom_type = lwgeom->type;
	lwfree(nodes);
	return tree;
}


RECT_NODE *
lwgeom_to_rect_tree(const LWGEOM *lwgeom)
{
	switch(lwgeom_type(lwgeom))
	{
		case POINTTYPE:
			return lwpoint_to_rect_tree(lwgeom);
		case TRIANGLETYPE:
		case CIRCSTRINGTYPE:
		case LINETYPE:
			return lwline_to_rect_tree(lwgeom);
		case POLYGONTYPE:
			return lwpoly_to_rect_tree(lwgeom);
		case COMPOUNDTYPE:
		case CURVEPOLYTYPE:
		case MULTICURVETYPE:
		case MULTISURFACETYPE:
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case POLYHEDRALSURFACETYPE:
		case TINTYPE:
		case COLLECTIONTYPE:
			return lwcollection_to_rect_tree(lwgeom);
		default:
			lwerror("%s: Unknown geometry type: %s", __func__, lwtype_name(lwgeom->type));
			return NULL;
	}
	return NULL;
}



static const POINT2D *
rect_tree_get_point(const RECT_NODE *node)
{
	if (!node) return NULL;
	if (rect_node_is_leaf(node))
		return getPoint2d_cp(node->l.pa, 0);
	else
		return rect_tree_get_point(node->i.nodes[0]);
}


int
rect_tree_intersects_tree(const RECT_NODE *n1, const RECT_NODE *n2)
{
	int i;
	LWDEBUGF(4,"n1 %s  n2 %s", rect_node_to_str(n1), rect_node_to_str(n2));
	/* There can only be an edge intersection if the rectangles overlap */
	if (rect_node_intersects(n1, n2))
	{
		LWDEBUG(4," interaction found");
		/* We can only test for a true intersection if the nodes are both leaf nodes */
		if (rect_node_is_leaf(n1) && rect_node_is_leaf(n2))
		{
			LWDEBUG(4,"  leaf node test");
			/* Check for true intersection */
			return rect_leaf_node_intersects(n1, n2);
		}
		else
		{
			LWDEBUG(4,"  internal node found, recursing");
			/* Recurse to children */
			if (rect_node_is_leaf(n1))
			{
				for (i = 0; i < n2->num_nodes; i++)
				{
					if (rect_tree_intersects_tree(n2->nodes[i], n1))
						return LW_TRUE;
				}
				return LW_FALSE;
			}
			else
			{
				for (i = 0; i < n1->num_nodes; i++)
				{
					if (rect_tree_intersects_tree(n1->nodes[i], n2))
						return LW_TRUE;
				}
				return LW_FALSE;
			}
		}
	}
	else
	{
		LWDEBUG(4," no interaction found");
		return LW_FALSE;
	}
}


/* TO DO XXXX
- re-read everything from top
- update point-in-tree to handle compoundcurve rings
- update rect-intersects-rect to handle polygonal shapes w/ p-i-p pre-test
*/



#if 0

/**
* Leaf nodes have an empty nodes list
*/
static inline int
rect_node_is_leaf(const RECT_NODE *node)
{
	return (node->num_nodes == 0);
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
	for (i = 0; i < node->num_nodes; i++)
	{
		rect_tree_free(node->nodes[i]);
		node->nodes[i] = NULL;
	}
	lwfree(node);
}


/* 0 => no containment */
int
rect_tree_contains_point(const RECT_NODE *node, const POINT2D *pt, int *on_boundary)
{
	/* Only test nodes that straddle our stabline */
	if ((node->ymin <= pt->y) && (pt->y <= node->ymax))
	{
		if (rect_node_is_leaf(node))
		{
			const POINT2D *p1 = node->p1;
			const POINT2D *p2 = node->p2;
			/* Avoid vertex-grazing and through-vertex issues */
			/* by only counting *one* of the crossings */
			if (pt->y != p2->y)
			{
				double side = lw_segment_side(p1, node->p2, pt);
				if ( side == 0 )
					*on_boundary = LW_TRUE;
				return (side < 0 ? -1 : 1 );
			}
		}
		else
		{
			int i, r = 0;
			for (i = 0; i < node->num_nodes; i++)
			{
				r += rect_tree_contains_point(node->nodes[i], pt, on_boundary);
			}
			return r;
		}
	}

	return 0;
}



/**
* Create a new leaf node, calculating a measure value for each point on the
* edge and storing pointers back to the end points for later.
*/
static RECT_NODE *
rect_node_leaf_new(const POINTARRAY *pa, int i)
{
	POINT2D *p1, *p2;
	RECT_NODE *node;

	p1 = getPoint_cp(pa, i);
	p2 = getPoint_cp(pa, i+1);

	/* Zero length edge, doesn't get a node */
	if ((p1->x == p2->x) && (p1->y == p2->y))
		return NULL;

	node = lwalloc(sizeof(RECT_NODE));
	node->p1 = p1;
	node->p2 = p2;
	node->xmin = FP_MIN(p1->x, p2->x);
	node->xmax = FP_MAX(p1->x, p2->x);
	node->ymin = FP_MIN(p1->y, p2->y);
	node->ymax = FP_MAX(p1->y, p2->y);
	node->num_nodes = 0;
	node->geom_type = 0;
	return node;
}

static int
rect_node_consistent_type(int type1, int type2)
{
	int mtype1 = lwtype_multitype(type1);
	int mtype2 = lwtype_multitype(type2);
	/* Same? Done. */
	if (type1 == type2) return type1;
	/* promote geom to mgeom */
	if (type1 == mtype2) return mtype2;
	if (type2 == mtype1) return mtype1;
	return COLLECTIONTYPE;
}

static void
rect_node_internal_add_node(RECT_NODE *node, RECT_NODE *add)
{
	node->xmin = FP_MIN(node->xmin, add->xmin);
	node->xmax = FP_MAX(node->xmax, add->xmax);
	node->ymin = FP_MIN(node->ymin, add->ymin);
	node->ymax = FP_MAX(node->ymax, add->ymax);
	node->nodes[node->num_nodes++] = add;
	node->geom_type = rect_node_consistent_type(node->geom_type, add->geom_type);
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
	node->num_nodes = 0;
	node->geom_type = seed->geom_type;
	return node;
}


static RECT_NODE *
rect_nodes_merge(RECT_NODE ** nodes, int num_nodes)
{
	while (num_nodes > 1)
	{
		int i, k = 0;
		RECT_NODE *node = NULL;
		for (i = 0; i < num_nodes; i++)
		{
			if (!node)
				node = rect_node_internal_new(nodes[i]);

			rect_node_internal_add_node(node, nodes[i]);

			if (node->num_nodes == CIRC_NODE_SIZE)
			{
				nodes[k++] = node;
				node = NULL;
			}
		}
		node[k++] = node;
		num_nodes = k;
	}
	return nodes[0];
}


/**
* Build a tree of nodes from a point array, one node per edge, and each
* with an associated measure range along a one-dimensional space. We
* can then search that space as a range tree.
*/
RECT_NODE* rect_tree_new(const POINTARRAY *pa)
{
	int num_edges, num_children, num_parents;
	int i, j = 0;
	RECT_NODE **nodes;
	RECT_NODE *node;
	RECT_NODE *tree;

	if ( pa->npoints < 2 )
	{
		return NULL;
	}

	/* First create a flat list of nodes, one per edge. */
	num_edges = pa->npoints - 1;
	nodes = lwalloc(sizeof(RECT_NODE*) * pa->npoints);
	for (i = 0; i < num_edges; i++)
	{
		node = rect_node_leaf_new(pa, i);
		if ( node ) /* Not zero length? */
		{
			nodes[j++] = node;
		}
	}

	/* Merge the list into a tree */
	tree = rect_nodes_merge(nodes, j);

	/* Free the old list structure, leaving the tree in place */
	lwfree(nodes);

	return tree;
}



int
rect_tree_intersects_tree(const RECT_NODE *n1, const RECT_NODE *n2)
{
	int i;
	LWDEBUGF(4,"n1 %s  n2 %s", rect_node_to_str(n1), rect_node_to_str(n2));
	/* There can only be an edge intersection if the rectangles overlap */
	if (rect_node_intersects(n1, n2))
	{
		LWDEBUG(4," interaction found");
		/* We can only test for a true intersection if the nodes are both leaf nodes */
		if (rect_node_is_leaf(n1) && rect_node_is_leaf(n2))
		{
			LWDEBUG(4,"  leaf node test");
			/* Check for true intersection */
			if (lw_segment_intersects(n1->p1, n1->p2, n2->p1, n2->p2))
				return LW_TRUE;
			else
				return LW_FALSE;
		}
		else
		{
			LWDEBUG(4,"  internal node found, recursing");
			/* Recurse to children */
			if (rect_node_is_leaf(n1))
			{
				for (i = 0; i < n2->num_nodes; i++)
				{
					if (rect_tree_intersects_tree(n2->nodes[i], n1))
						return LW_TRUE;
				}
				return LW_FALSE;
			}
			else
			{
				for (i = 0; i < n1->num_nodes; i++)
				{
					if (rect_tree_intersects_tree(n1->nodes[i], n2))
						return LW_TRUE;
				}
				return LW_FALSE;
			}
		}
	}
	else
	{
		LWDEBUG(4," no interaction found");
		return LW_FALSE;
	}
}

#endif
