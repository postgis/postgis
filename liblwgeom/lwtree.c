#include "liblwgeom_internal.h"
#include "lwgeom_log.h"
#include "lwtree.h"
#include "measures.h"


/**
* Internal nodes have their point references set to NULL.
*/
static inline int 
rect_node_is_leaf(const RECT_NODE *node)
{
	return (node->pa != NULL);
}

static inline int 
rect_node_is_point(const RECT_NODE *n)
{
	return (n->num_points == 1);
}

static inline int 
rect_node_is_line(const RECT_NODE *n)
{
	return (n->num_points == 2);
}

static inline int 
rect_node_is_arc(const RECT_NODE *n)
{
	return (n->num_points == 3);
}

/**
* Recurse from top of node tree and free all children.
* does not free underlying point array.
*/
void 
rect_tree_free(RECT_NODE *node)
{
	int i;

	if ( node->nodes )
	{
		for ( i = 0; i < node->num_nodes; i++ )
		{
			rect_tree_free(node->nodes[i]);
		}
		lwfree(node->nodes);
	}
	lwfree(node);
}

/* Point-in-polygon test. 0 => no containment */
int 
rect_tree_contains_point(const RECT_NODE *node, const POINT2D *pt, int *on_boundary)
{
	if ( FP_CONTAINS_INCL(node->ymin, pt->y, node->ymax) )
	{
		if ( rect_node_is_leaf(node) )
		{
			POINT2D p1, p2, p3;
			double side = 0.0;

			/* Point edge. Skip it. Its neighbors will pick up the crossing. */
			if ( rect_node_is_point(node) )
			{
				return 0;
			}
			
			getPoint2d_p(node->pa, node->first_point + 1, &p2);
			getPoint2d_p(node->pa, node->first_point, &p1);
			
			/* Linear edge. Test it. */
			if ( rect_node_is_line(node) )
			{
				side = lw_segment_side(&p1, &p2, pt);
			}
			/* Circular arc edge. Test it. */
			else if ( rect_node_is_arc(node) )
			{
				getPoint2d_p(node->pa, node->first_point + 2, &p3);
				lwerror("rect_tree_contains_point: support for arcs not implemented yet!");
				/* TODO: handle arcs (node->num_points == 3) */
				/* this function doesn't exist yet */
				/* side = lw_arc_side(&p1, &p2, &p3, pt); */
			}
			/* WFT?!? This shouldn't happen. Freak out. */
			else
			{
				lwerror("rect_tree_contains_point: node references wrong number of points!");
			}
			
			if ( side == 0 )
				*on_boundary = LW_TRUE;
			return (side < 0 ? -1 : 1 );
		}
		else
		{
			int i, c = 0;
			for ( i = 0; i < node->num_nodes; i++ )
			{
				c += rect_tree_contains_point(node->nodes[i], pt, on_boundary);
			}
			return  c;
		}
	}

	return 0;
}


/* Returns true if the boundaries represented by the trees intersect. */
int 
rect_tree_intersects_tree(const RECT_NODE *n1, const RECT_NODE *n2)
{
	LWDEBUGF(4,"n1 (%.9g %.9g,%.9g %.9g) vs n2 (%.9g %.9g,%.9g %.9g)",n1->xmin,n1->ymin,n1->xmax,n1->ymax,n2->xmin,n2->ymin,n2->xmax,n2->ymax);
	/* There can only be an edge intersection if the rectangles overlap */
	if ( ! ((n1->xmin > n2->xmax) || (n2->xmin > n1->xmax) || (n1->ymin > n2->ymax) || (n2->ymin > n1->ymax)) )
	{
		LWDEBUG(4," interaction found");
		/* We can only test for a true intersection if the nodes are both leaf nodes */
		if ( rect_node_is_leaf(n1) && rect_node_is_leaf(n2) )
		{
			LWDEBUG(4,"  leaf node test");

			/* Skip the one-point edges and let the neighbors catch that case */
			if ( rect_node_is_point(n1) || rect_node_is_point(n2) )
			{
				return LW_FALSE;
			}
			/* Edge/edge intersection */
			if ( rect_node_is_line(n1) && rect_node_is_line(n2) )
			{
				POINT2D n1p1, n1p2, n2p1, n2p2;
				getPoint2d_p(n1->pa, n1->first_point, &n1p1);
				getPoint2d_p(n1->pa, n1->first_point + 1, &n1p2);
				getPoint2d_p(n2->pa, n2->first_point, &n2p1);
				getPoint2d_p(n2->pa, n2->first_point + 1, &n2p2);
				
				/* Check for true intersection */
				if ( lw_segment_intersects(&n1p1, &n1p2, &n2p1, &n2p2) )
					return LW_TRUE;
				else
					return LW_FALSE;
			}
			/* Arc/arc intersection */
			else if ( rect_node_is_arc(n1) || rect_node_is_arc(n2) )
			{
				/* lw_arc_intersects() */
				lwerror("rect_tree_intersects_tree: arc support is not implemented yet!");
			}
			/* WFT?!? Unhandled case */
			else
			{
				lwerror("rect_tree_intersects_tree: Unsupported number of edge points referenced!");
			}
		}
		else
		{
			int i;
			LWDEBUG(4,"  internal node found, recursing");

			/* Make sure n1 has children */
			if ( rect_node_is_leaf(n1) )
			{
				const RECT_NODE *tmp;
				tmp = n1;
				n1 = n2;
				n2 = tmp;
			}
			
			/* Recurse to children */
			for ( i = 0; i < n1->num_nodes; i++ )
			{
				if ( rect_tree_intersects_tree( n1->nodes[i], n2 ) )
				{
					LWDEBUGF(4,"    internal node interaction found @ node %d", i);
					return LW_TRUE;
				}
			}
			LWDEBUG(4,"    internal node no interaction found");
			return LW_FALSE;
		}
	}
	else
	{
		LWDEBUG(4," no interaction found");
		return LW_FALSE;
	}
	
	return LW_FALSE;
}

static inline double p2d_distance(double x1, double y1, double x2, double y2)
{
	return sqrt( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) );
}

static double 
rect_node_min_distance(const RECT_NODE *n1, const RECT_NODE *n2)
{

	int left  = n1->xmax < n2->xmin;
	int right = n1->xmin > n2->xmax;
	int above = n1->ymin > n2->ymax;
	int below = n1->ymax < n2->ymin;

	if ( left ) 
	{
		if ( above )
		{
			return p2d_distance(n1->xmax, n1->ymin, n2->xmin, n2->ymax);
		}
		else if ( below )
		{
			return p2d_distance(n1->xmax, n1->ymax, n2->xmin, n2->ymin);
		}
		else
		{
			return n2->xmin - n1->xmax;
		}
	}
	
	if ( right ) 
	{
		if ( above )
		{
			return p2d_distance(n1->xmin, n1->ymin, n2->xmax, n2->ymax);
		}
		else if ( below )
		{
			return p2d_distance(n1->xmin, n1->ymax, n2->xmax, n2->ymin);
		}
		else
		{
			return n1->xmin - n2->xmax;
		}
	}

	if ( above ) 
	{
		return n1->ymin - n2->ymax;
	}
	else if ( below )
	{
		return n2->ymin - n1->ymax;
	}
	/* Overlaps! */
	else
	{
		return 0.0;
	}

}

static double 
rect_node_max_distance(const RECT_NODE *n1, const RECT_NODE *n2)
{
	double minxmin = FP_MIN(n1->xmin, n2->xmin);
	double maxxmax = FP_MAX(n1->xmax, n2->xmax);
	double minymin = FP_MIN(n1->ymin, n2->ymin);
	double maxymax = FP_MAX(n1->ymax, n2->ymax);
	return FP_MAX(maxxmax - minxmin, maxymax - minymin);
}

double 
rect_tree_distance_tree(const RECT_NODE *n1, const RECT_NODE *n2, double threshold, double *min_dist, double *max_dist, POINT2D *closest1, POINT2D *closest2)
{	
	double max;
	double d, d_min;
	int i;
	const RECT_NODE *tmp;
	
	LWDEBUGF(4, "entered, min_dist %.8g max_dist %.8g", *min_dist, *max_dist);
	
	/* Short circuit if we've already hit the minimum */
	if( FP_LT(*min_dist, threshold) )
		return *min_dist;
	
	/* If your minimum is greater than anyone's maximum, you can't hold the winner */
	if( rect_node_min_distance(n1, n2) > *max_dist )
	{
		LWDEBUGF(4, "pruning pair %p, %p", n1, n2);		
		return MAXFLOAT;
	}
	
	/* If your maximum is a new low, we'll use that as our new global tolerance */
	max = rect_node_max_distance(n1, n2);
	LWDEBUGF(5, "max %.8g", max);
	if( max < *max_dist )
		*max_dist = max;

	/* Both leaf nodes, do a real distance calculation */
	if( rect_node_is_leaf(n1) && rect_node_is_leaf(n2) )
	{
		DISTPTS	dp;
		dp.tolerance = threshold;
		dp.mode = 1; /* mindistance */
		const POINT2D *a, *b, *c, *d, *p;

		/* Both nodes are points */
		if ( rect_node_is_point(n1) && rect_node_is_point(n2) )
		{
			a = getPoint2d_cp(n1->pa, n1->first_point);
			c = getPoint2d_cp(n2->pa, n2->first_point);
			lw_dist2d_pt_pt(a, c, &dp);
		}
		/* Both nodes are edges */
		else if ( rect_node_is_line(n1) && rect_node_is_line(n2) )
		{
			a = getPoint2d_cp(n1->pa, n1->first_point);
			b = getPoint2d_cp(n1->pa, n1->first_point+1);
			c = getPoint2d_cp(n2->pa, n2->first_point);
			d = getPoint2d_cp(n2->pa, n2->first_point+1);
			lw_dist2d_seg_seg(a, b, c, d, &dp);
		}
		/* Both nodes are arcs */
		else if ( rect_node_is_arc(n1) && rect_node_is_arc(n2) )
		{
			lwerror("rect_tree_distance_tree_internal cannot do arcs yet");
		}
		/* One node is a line */
		else if ( rect_node_is_line(n1) || rect_node_is_line(n2) )
		{
			/* Force n1 to be the line node */
			if ( ! rect_node_is_line(n1) )
			{
				tmp = n1; n1 = n2; n2 = tmp;
			}

			a = getPoint2d_cp(n1->pa, n1->first_point);
			b = getPoint2d_cp(n1->pa, n1->first_point+1);
			
			/* Line vs arc case */
			if ( rect_node_is_arc(n2) )
			{
				lwerror("rect_tree_distance_tree_internal cannot do arcs yet");
			}
			/* Line vs point case */
			else if ( rect_node_is_point(n2) )
			{
				p = getPoint2d_cp(n2->pa, n2->first_point);
				lw_dist2d_pt_seg(p, a, b, &dp);
			}
			else
			{
				lwerror("rect_tree_distance_tree_internal wtf");
			}	
		}
		/* One of the nodes is an arc... */
		else if ( rect_node_is_arc(n1) || rect_node_is_arc(n2) )
		{
			lwerror("rect_tree_distance_tree_internal cannot do arcs yet");
		}
		else
		{
			lwerror("rect_tree_distance_tree_internal wtf");
		}

		
		if ( dp.distance < *min_dist )
		{
			*min_dist = dp.distance;
			*closest1 = dp.p1;
			*closest2 = dp.p2;
		}
		return dp.distance;
	}
	/* At least one node is still internal, recurse into it... */
	else
	{
		/* Force n1 to be a non-leaf node */
		if ( rect_node_is_leaf(n1) )
		{
			tmp = n1; n1 = n2; n2 = tmp;
		}

		d_min = MAXFLOAT;
		for ( i = 0; i < n1->num_nodes; i++ )
		{
			d = rect_tree_distance_tree(n2, n1->nodes[i], threshold, min_dist, max_dist, closest1, closest2);
			d_min = FP_MIN(d_min, d);
		}
		return d_min;
	}
}

/**
* Create a new leaf node, calculating a measure value for each point on the
* edge and storing pointers back to the end points for later.
*/
static RECT_NODE* 
rect_node_leaf_new(const POINTARRAY *pa, int i)
{
	POINT2D p1, p2;
	RECT_NODE *node;

	/* Read point values */
	getPoint2d_p(pa, i, &p1);
	getPoint2d_p(pa, i+1, &p2);

	/* Zero length edge, doesn't get a node */
	if ( FP_EQUALS(p1.x, p2.x) && FP_EQUALS(p1.y, p2.y) )
	{
		return NULL;
	}

	/* Allocate */
	node = lwalloc(sizeof(RECT_NODE));

	/* Bounding box of node */
	node->xmin = FP_MIN(p1.x,p2.x);
	node->xmax = FP_MAX(p1.x,p2.x);
	node->ymin = FP_MIN(p1.y,p2.y);
	node->ymax = FP_MAX(p1.y,p2.y);

	/* Leaf node has no children */
	node->nodes = NULL;
	node->num_nodes = 0;

	/* Reference to source array */
	node->pa = pa;
	node->first_point = i;
	node->num_points = 2;

	return node;
}

/**
* Create a new internal node, calculating the new measure range for the node,
* and storing pointers to the child nodes.
*/
static RECT_NODE* 
rect_node_internal_new(RECT_NODE **r, int num_nodes)
{
	int i;
	RECT_NODE *node = NULL;
	double xmin, ymin, xmax, ymax;
	
	if ( num_nodes < 1 ) 
		return NULL;
	
	xmin = r[0]->xmin;
	ymin = r[0]->ymin;
	xmax = r[0]->xmax;
	ymax = r[0]->ymax;

	for ( i = 1; i < num_nodes; i++ )
	{
		xmin = FP_MIN(xmin, r[i]->xmin);
		ymin = FP_MIN(ymin, r[i]->ymin);
		xmax = FP_MAX(xmax, r[i]->xmax);
		ymax = FP_MAX(ymax, r[i]->ymax);
	}
	
	/* Box is union of boxes */
	node = lwalloc(sizeof(RECT_NODE));
	node->xmin = xmin;
	node->ymin = ymin;
	node->xmax = xmax;
	node->ymax = ymax;
	
	/* Point array references are null */
	node->pa = NULL;
	node->num_points = 0;
	node->first_point = r[0]->first_point;
	
	/* Child nodes are references */
	node->nodes = r;
	node->num_nodes = num_nodes;
	
	return node;
}

static RECT_NODE*
rect_nodes_merge(RECT_NODE **nodes, int num_nodes)
{
	RECT_NODE **inodes = NULL;
	int num_children = num_nodes;
	int inode_num = 0;
	int num_parents = 0;
	int j;

	while( num_children > 1 )
	{
		for ( j = 0; j < num_children; j++ )
		{
			inode_num = (j % RECT_NODE_SIZE);
			if ( inode_num == 0 )
				inodes = lwalloc(sizeof(RECT_NODE*)*RECT_NODE_SIZE);

			inodes[inode_num] = nodes[j];
			
			if ( inode_num == RECT_NODE_SIZE-1 )
				nodes[num_parents++] = rect_node_internal_new(inodes, RECT_NODE_SIZE);
		}
		
		/* Clean up any remaining nodes... */
		if ( inode_num == 0 )
		{
			/* Promote solo nodes without merging */
			nodes[num_parents++] = inodes[0];
			lwfree(inodes);
		}
		else if ( inode_num < RECT_NODE_SIZE-1 )
		{
			/* Merge spare nodes */
			nodes[num_parents++] = rect_node_internal_new(inodes, inode_num+1);
		}
		
		num_children = num_parents;	
		num_parents = 0;
	}
	
	/* Return a reference to the head of the tree */
	return nodes[0];
}


/**
* Build a tree of nodes from a point array, one node per edge, and each
* with an associated box. We can search those boxes for intersections,
* point-in-polygon and distance.
*/
RECT_NODE* 
rect_tree_new(const POINTARRAY *pa)
{
	int num_edges;
	int i, j;
	RECT_NODE **nodes;
	RECT_NODE *node;
	RECT_NODE *tree;

	if ( pa->npoints < 1 )
	{
		return NULL;
	}

	/* Special case for a point */
	if ( pa->npoints == 1 )
	{
		POINT2D p;
		getPoint2d_p(pa, 0, &p);
		node = lwalloc(sizeof(RECT_NODE));
		node->xmin = node->xmax = p.x;
		node->ymin = node->ymax = p.y;
		node->nodes = NULL;
		node->num_nodes = 0;
		node->pa = pa;
		node->first_point = 0;
		node->num_points = 1;
		return node;
	}
	
	/* First create a flat list of nodes, one per edge. */
	num_edges = pa->npoints - 1;
	nodes = lwalloc(sizeof(RECT_NODE*) * pa->npoints);
	j = 0;
	for ( i = 0; i < num_edges; i++ )
	{
		node = rect_node_leaf_new(pa, i);
		if ( node ) /* Not zero length? */
			nodes[j++] = node;
	}

	/* Merge the node list pairwise up into a tree */
	tree = rect_nodes_merge(nodes, j);

	/* Free the old list structure, leaving the tree in place */
	lwfree(nodes);

	return tree;
}

/**
* Comparing on geohash ensures that nearby nodes will be close 
* to each other in the list.
*/  
static int
rect_node_compare(const void *v1, const void *v2)
{
	POINT2D p1, p2;
	unsigned int u1, u2;
	RECT_NODE *c1 = (RECT_NODE*)v1;
	RECT_NODE *c2 = (RECT_NODE*)v2;
	p1.x = (c1->xmin + c1->xmax)/2.0;
	p1.y = (c1->ymin + c1->ymax)/2.0;
	p2.x = (c2->xmin + c2->xmax)/2.0;
	p2.y = (c2->ymin + c2->ymax)/2.0;
	u1 = geohash_point_as_int(&p1);
	u2 = geohash_point_as_int(&p2);
	if ( u1 < u2 ) return -1;
	if ( u1 > u2 ) return 1;
	return 0;
}

/**
* Given a list of nodes, sort them into a spatially consistent
* order, then pairwise merge them up into a tree. Should make
* handling multipoints and other collections more efficient
*/
static void
rect_nodes_sort(RECT_NODE **nodes, int num_nodes)
{
	qsort(nodes, num_nodes, sizeof(RECT_NODE*), rect_node_compare);
}


static RECT_NODE*
lwpoint_calculate_rect_tree(const LWPOINT *lwpoint)
{
	return rect_tree_new(lwpoint->point);
}

static RECT_NODE*
lwline_calculate_rect_tree(const LWLINE *lwline)
{
	return rect_tree_new(lwline->points);
}

static RECT_NODE*
lwpoly_calculate_rect_tree(const LWPOLY *lwpoly)
{
	int i = 0, j = 0;
	RECT_NODE **nodes;
	RECT_NODE *node;

	/* One ring? Handle it like a line. */
	if ( lwpoly->nrings == 1 )
		return rect_tree_new(lwpoly->rings[0]);	
	
	/* Calculate a tree for each non-trivial ring of the polygon */
	nodes = lwalloc(lwpoly->nrings * sizeof(RECT_NODE*));
	for ( i = 0; i < lwpoly->nrings; i++ )
	{
		node = rect_tree_new(lwpoly->rings[i]);
		if ( node )
			nodes[j++] = node;
	}
	/* Put the trees into a spatially correlated order */
	rect_nodes_sort(nodes, j);
	/* Merge the trees pairwise up to a parent node and return */
	node = rect_nodes_merge(nodes, j);
	/* Don't need the working list any more */
	lwfree(nodes);
	
	return node;
}

static RECT_NODE*
lwcollection_calculate_rect_tree(const LWCOLLECTION *lwcol)
{
	int i = 0, j = 0;
	RECT_NODE **nodes;
	RECT_NODE *node;

	/* One geometry? Done! */
	if ( lwcol->ngeoms == 1 )
		return lwgeom_calculate_rect_tree(lwcol->geoms[0]);	
	
	/* Calculate a tree for each sub-geometry*/
	nodes = lwalloc(lwcol->ngeoms * sizeof(RECT_NODE*));
	for ( i = 0; i < lwcol->ngeoms; i++ )
	{
		node = lwgeom_calculate_rect_tree(lwcol->geoms[i]);
		if ( node )
			nodes[j++] = node;
	}
	/* Put the trees into a spatially correlated order */
	rect_nodes_sort(nodes, j);
	/* Merge the trees pairwise up to a parent node and return */
	node = rect_nodes_merge(nodes, j);
	/* Don't need the working list any more */
	lwfree(nodes);
	
	return node;
}

RECT_NODE*
lwgeom_calculate_rect_tree(const LWGEOM *lwgeom)
{
	if ( lwgeom_is_empty(lwgeom) )
		return NULL;
		
	switch ( lwgeom->type )
	{
		case POINTTYPE:
			return lwpoint_calculate_rect_tree((LWPOINT*)lwgeom);
		case LINETYPE:
			return lwline_calculate_rect_tree((LWLINE*)lwgeom);
		case POLYGONTYPE:
			return lwpoly_calculate_rect_tree((LWPOLY*)lwgeom);
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case COLLECTIONTYPE:
			return lwcollection_calculate_rect_tree((LWCOLLECTION*)lwgeom);
		default:
			lwerror("Unable to calculate rect index tree for type %s", lwtype_name(lwgeom->type));
			return NULL;
	}
	
}
	
