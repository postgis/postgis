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


/**
* Leaf nodes have an empty nodes list
*/
static inline int
rect_node_is_leaf(const RECT_NODE *node)
{
	return (node->num_nodes == 0);
}

static const char *
rect_node_to_str(const RECT_NODE *n)
{
	char buf[256];
	snprintf(buf, 256, "(%.9g %.9g,%.9g %.9g)",
		n->xmin, n->ymin, n->xmax, n->ymax);
	return buf;
}

static inline int
rect_node_intersects(const RECT_NODE *n1, const RECT_NODE *n2)
{
	if (FP_GT(n1->xmin, n2->xmax) ||
		FP_GT(n2->xmin, n1->xmax) ||
		FP_GT(n1->ymin, n2->ymax) ||
		FP_GT(n2->ymin, n1->ymax))
	{
		return 0;
	}
	return 1;
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
			double ymin = FP_MIN(p1->y, p2->y);
			double ymax = FP_MAX(p1->y, p2->y);
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

	/*
	** First create a flat list of nodes, one per edge.
	*/
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

	/*
	** If we sort the nodelist first, we'll get a more balanced tree
	** in the end, but at the cost of sorting. For now, we just
	** build the tree knowing that point arrays tend to have a
	** reasonable amount of sorting already.
	*/

	num_children = j;
	num_parents = num_children / RECT_NODE_SIZE;
	while ( num_parents > 0 )
	{
		j = 0;
		while (j < num_parents)
		{
			/*
			** Each new parent includes pointers to the children, so even though
			** we are over-writing their place in the list, we still have references
			** to them via the tree.
			*/
			nodes[j] = rect_node_internal_new(nodes[2*j], nodes[(2*j)+1]);
			j++;
		}
		/* Odd number of children, just copy the last node up a level */
		if ( num_children % 2 )
		{
			nodes[j] = nodes[num_children - 1];
			num_parents++;
		}
		num_children = num_parents;
		num_parents = num_children / 2;
	}

	/* Take a reference to the head of the tree*/
	tree = nodes[0];

	/* Free the old list structure, leaving the tree in place */
	lwfree(nodes);

	return tree;

}





int
rect_tree_intersects_tree(const RECT_NODE *n1, const RECT_NODE *n2)
{
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
				if (rect_tree_intersects_tree(n2->left_node, n1) ||
					rect_tree_intersects_tree(n2->right_node, n1))
					return LW_TRUE;
				else
					return LW_FALSE;
			}
			else
			{
				if (rect_tree_intersects_tree(n1->left_node, n2) ||
					rect_tree_intersects_tree(n1->right_node, n2))
					return LW_TRUE;
				else
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
