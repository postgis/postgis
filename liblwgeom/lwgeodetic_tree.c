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
 * Copyright (C) 2012-2015 Paul Ramsey <pramsey@cleverelephant.ca>
 * Copyright (C) 2012-2015 Sandro Santilli <strk@kbt.io>
 *
 **********************************************************************/

#include "liblwgeom_internal.h"
#include "lwgeodetic_tree.h"
#include "lwgeom_log.h"


/* Internal prototype */
static CIRC_NODE* circ_nodes_merge(CIRC_NODE** nodes, int num_nodes);
static double circ_tree_distance_tree_internal(const CIRC_NODE* n1, const CIRC_NODE* n2, double threshold, double* min_dist, double* max_dist, GEOGRAPHIC_POINT* closest1, GEOGRAPHIC_POINT* closest2);


/**
* Internal nodes have their point references set to NULL.
*/
static inline int
circ_node_is_leaf(const CIRC_NODE* node)
{
	return (node->num_nodes == 0);
}

/**
* Recurse from top of node tree and free all children.
* does not free underlying point array.
*/
void
circ_tree_free(CIRC_NODE* node)
{
	uint32_t i;
	if ( ! node ) return;

	if (node->nodes)
	{
		for (i = 0; i < node->num_nodes; i++)
			circ_tree_free(node->nodes[i]);
		lwfree(node->nodes);
	}
	lwfree(node);
}


/**
* Create a new leaf node, storing pointers back to the end points for later.
*/
static CIRC_NODE*
circ_node_leaf_new(const POINTARRAY* pa, int i)
{
	POINT2D *p1, *p2;
	POINT3D q1, q2, c;
	GEOGRAPHIC_POINT g1, g2, gc;
	CIRC_NODE *node;
	double diameter;

	p1 = (POINT2D*)getPoint_internal(pa, i);
	p2 = (POINT2D*)getPoint_internal(pa, i+1);
	geographic_point_init(p1->x, p1->y, &g1);
	geographic_point_init(p2->x, p2->y, &g2);

	LWDEBUGF(3,"edge #%d (%g %g, %g %g)", i, p1->x, p1->y, p2->x, p2->y);

	diameter = sphere_distance(&g1, &g2);

	/* Zero length edge, doesn't get a node */
	if ( FP_EQUALS(diameter, 0.0) )
		return NULL;

	/* Allocate */
	node = lwalloc(sizeof(CIRC_NODE));
	node->p1 = p1;
	node->p2 = p2;

	/* Convert ends to X/Y/Z, sum, and normalize to get mid-point */
	geog2cart(&g1, &q1);
	geog2cart(&g2, &q2);
	vector_sum(&q1, &q2, &c);
	normalize(&c);
	cart2geog(&c, &gc);
	node->center = gc;
	node->radius = diameter / 2.0;

	LWDEBUGF(3,"edge #%d CENTER(%g %g) RADIUS=%g", i, gc.lon, gc.lat, node->radius);

	/* Leaf has no children */
	node->num_nodes = 0;
	node->nodes = NULL;
	node->edge_num = i;

	/* Zero out metadata */
	node->pt_outside.x = 0.0;
	node->pt_outside.y = 0.0;
	node->geom_type = 0;

	return node;
}

/**
* Return a point node (zero radius, referencing one point)
*/
static CIRC_NODE*
circ_node_leaf_point_new(const POINTARRAY* pa)
{
	CIRC_NODE* tree = lwalloc(sizeof(CIRC_NODE));
	tree->p1 = tree->p2 = (POINT2D*)getPoint_internal(pa, 0);
	geographic_point_init(tree->p1->x, tree->p1->y, &(tree->center));
	tree->radius = 0.0;
	tree->nodes = NULL;
	tree->num_nodes = 0;
	tree->edge_num = 0;
	tree->geom_type = POINTTYPE;
	tree->pt_outside.x = 0.0;
	tree->pt_outside.y = 0.0;
	return tree;
}

/**
* Comparing on geohash ensures that nearby nodes will be close
* to each other in the list.
*/
static int
circ_node_compare(const void* v1, const void* v2)
{
	POINT2D p1, p2;
	unsigned int u1, u2;
	CIRC_NODE *c1 = *((CIRC_NODE**)v1);
	CIRC_NODE *c2 = *((CIRC_NODE**)v2);
	p1.x = rad2deg((c1->center).lon);
	p1.y = rad2deg((c1->center).lat);
	p2.x = rad2deg((c2->center).lon);
	p2.y = rad2deg((c2->center).lat);
	u1 = geohash_point_as_int(&p1);
	u2 = geohash_point_as_int(&p2);
	if ( u1 < u2 ) return -1;
	if ( u1 > u2 ) return 1;
	return 0;
}

/**
* Given the centers of two circles, and the offset distance we want to put the new center between them
* (calculated as the distance implied by the radii of the inputs and the distance between the centers)
* figure out where the new center point is, by getting the direction from c1 to c2 and projecting
* from c1 in that direction by the offset distance.
*/
static int
circ_center_spherical(const GEOGRAPHIC_POINT* c1, const GEOGRAPHIC_POINT* c2, double distance, double offset, GEOGRAPHIC_POINT* center)
{
	/* Direction from c1 to c2 */
	double dir = sphere_direction(c1, c2, distance);

	LWDEBUGF(4,"calculating spherical center", dir);

	LWDEBUGF(4,"dir is %g", dir);

	/* Catch sphere_direction when it barfs */
	if ( isnan(dir) )
		return LW_FAILURE;

	/* Center of new circle is projection from start point, using offset distance*/
	return sphere_project(c1, offset, dir, center);
}

/**
* Where the circ_center_spherical() function fails, we need a fall-back. The failures
* happen in short arcs, where the spherical distance between two points is practically
* the same as the straight-line distance, so our fallback will be to use the straight-line
* between the two to calculate the new projected center. For proportions far from 0.5
* this will be increasingly more incorrect.
*/
static int
circ_center_cartesian(const GEOGRAPHIC_POINT* c1, const GEOGRAPHIC_POINT* c2, double distance, double offset, GEOGRAPHIC_POINT* center)
{
	POINT3D p1, p2;
	POINT3D p1p2, pc;
	double proportion = offset/distance;

	LWDEBUG(4,"calculating cartesian center");

	geog2cart(c1, &p1);
	geog2cart(c2, &p2);

	/* Difference between p2 and p1 */
	p1p2.x = p2.x - p1.x;
	p1p2.y = p2.y - p1.y;
	p1p2.z = p2.z - p1.z;

	/* Scale difference to proportion */
	p1p2.x *= proportion;
	p1p2.y *= proportion;
	p1p2.z *= proportion;

	/* Add difference to p1 to get approximate center point */
	pc.x = p1.x + p1p2.x;
	pc.y = p1.y + p1p2.y;
	pc.z = p1.z + p1p2.z;
	normalize(&pc);

	/* Convert center point to geographics */
	cart2geog(&pc, center);

	return LW_SUCCESS;
}


/**
* Create a new internal node, calculating the new measure range for the node,
* and storing pointers to the child nodes.
*/
static CIRC_NODE*
circ_node_internal_new(CIRC_NODE** c, uint32_t num_nodes)
{
	CIRC_NODE *node = NULL;
	GEOGRAPHIC_POINT new_center, c1;
	double new_radius;
	double offset1, dist, D, r1, ri;
	uint32_t i, new_geom_type;

	LWDEBUGF(3, "called with %d nodes --", num_nodes);

	/* Can't do anything w/ empty input */
	if ( num_nodes < 1 )
		return node;

	/* Initialize calculation with values of the first circle */
	new_center = c[0]->center;
	new_radius = c[0]->radius;
	new_geom_type = c[0]->geom_type;

	/* Merge each remaining circle into the new circle */
	for ( i = 1; i < num_nodes; i++ )
	{
		c1 = new_center;
		r1 = new_radius;

		dist = sphere_distance(&c1, &(c[i]->center));
		ri = c[i]->radius;

		/* Promote geometry types up the tree, getting more and more collected */
		/* Go until we find a value */
		if ( ! new_geom_type )
		{
			new_geom_type = c[i]->geom_type;
		}
		/* Promote singleton to a multi-type */
		else if ( ! lwtype_is_collection(new_geom_type) )
		{
			/* Anonymous collection if types differ */
			if ( new_geom_type != c[i]->geom_type )
			{
				new_geom_type = COLLECTIONTYPE;
			}
			else
			{
				new_geom_type = lwtype_get_collectiontype(new_geom_type);
			}
		}
		/* If we can't add next feature to this collection cleanly, promote again to anonymous collection */
		else if ( new_geom_type != lwtype_get_collectiontype(c[i]->geom_type) )
		{
			new_geom_type = COLLECTIONTYPE;
		}


		LWDEBUGF(3, "distance between new (%g %g) and %i (%g %g) is %g", c1.lon, c1.lat, i, c[i]->center.lon, c[i]->center.lat, dist);

		if ( FP_EQUALS(dist, 0) )
		{
			LWDEBUG(3, "  distance between centers is zero");
			new_radius = r1 + 2*dist;
			new_center = c1;
		}
		else if ( dist < fabs(r1 - ri) )
		{
			/* new contains next */
			if ( r1 > ri )
			{
				LWDEBUG(3, "  c1 contains ci");
				new_center = c1;
				new_radius = r1;
			}
			/* next contains new */
			else
			{
				LWDEBUG(3, "  ci contains c1");
				new_center = c[i]->center;
				new_radius = ri;
			}
		}
		else
		{
			LWDEBUG(3, "  calculating new center");
			/* New circle diameter */
			D = dist + r1 + ri;
			LWDEBUGF(3,"    D is %g", D);

			/* New radius */
			new_radius = D / 2.0;

			/* Distance from cn1 center to the new center */
			offset1 = ri + (D - (2.0*r1 + 2.0*ri)) / 2.0;
			LWDEBUGF(3,"    offset1 is %g", offset1);

			/* Sometimes the sphere_direction function fails... this causes the center calculation */
			/* to fail too. In that case, we're going to fall back to a cartesian calculation, which */
			/* is less exact, so we also have to pad the radius by (hack alert) an arbitrary amount */
			/* which is hopefully always big enough to contain the input edges */
			if ( circ_center_spherical(&c1, &(c[i]->center), dist, offset1, &new_center) == LW_FAILURE )
			{
				circ_center_cartesian(&c1, &(c[i]->center), dist, offset1, &new_center);
				new_radius *= 1.1;
			}
		}
		LWDEBUGF(3, " new center is (%g %g) new radius is %g", new_center.lon, new_center.lat, new_radius);
	}

	node = lwalloc(sizeof(CIRC_NODE));
	node->p1 = NULL;
	node->p2 = NULL;
	node->center = new_center;
	node->radius = new_radius;
	node->num_nodes = num_nodes;
	node->nodes = c;
	node->edge_num = -1;
	node->geom_type = new_geom_type;
	node->pt_outside.x = 0.0;
	node->pt_outside.y = 0.0;
	return node;
}

/**
* Build a tree of nodes from a point array, one node per edge.
*/
CIRC_NODE*
circ_tree_new(const POINTARRAY* pa)
{
	int num_edges;
	int i, j;
	CIRC_NODE **nodes;
	CIRC_NODE *node;
	CIRC_NODE *tree;

	/* Can't do anything with no points */
	if ( pa->npoints < 1 )
		return NULL;

	/* Special handling for a single point */
	if ( pa->npoints == 1 )
		return circ_node_leaf_point_new(pa);

	/* First create a flat list of nodes, one per edge. */
	num_edges = pa->npoints - 1;
	nodes = lwalloc(sizeof(CIRC_NODE*) * pa->npoints);
	j = 0;
	for ( i = 0; i < num_edges; i++ )
	{
		node = circ_node_leaf_new(pa, i);
		if ( node ) /* Not zero length? */
			nodes[j++] = node;
	}

	/* Special case: only zero-length edges. Make a point node. */
	if ( j == 0 ) {
		lwfree(nodes);
		return circ_node_leaf_point_new(pa);
	}

	/* Merge the node list pairwise up into a tree */
	tree = circ_nodes_merge(nodes, j);

	/* Free the old list structure, leaving the tree in place */
	lwfree(nodes);

	return tree;
}

/**
* Given a list of nodes, sort them into a spatially consistent
* order, then pairwise merge them up into a tree. Should make
* handling multipoints and other collections more efficient
*/
static void
circ_nodes_sort(CIRC_NODE** nodes, int num_nodes)
{
	qsort(nodes, num_nodes, sizeof(CIRC_NODE*), circ_node_compare);
}


static CIRC_NODE*
circ_nodes_merge(CIRC_NODE** nodes, int num_nodes)
{
	CIRC_NODE **inodes = NULL;
	int num_children = num_nodes;
	int inode_num = 0;
	int num_parents = 0;
	int j;

	/* TODO, roll geom_type *up* as tree is built, changing to collection types as simple types are merged
	 * TODO, change the distance algorithm to drive down to simple types first, test pip on poly/other cases, then test edges
	 */

	while( num_children > 1 )
	{
		for ( j = 0; j < num_children; j++ )
		{
			inode_num = (j % CIRC_NODE_SIZE);
			if ( inode_num == 0 )
				inodes = lwalloc(sizeof(CIRC_NODE*)*CIRC_NODE_SIZE);

			inodes[inode_num] = nodes[j];

			if ( inode_num == CIRC_NODE_SIZE-1 )
				nodes[num_parents++] = circ_node_internal_new(inodes, CIRC_NODE_SIZE);
		}

		/* Clean up any remaining nodes... */
		if ( inode_num == 0 )
		{
			/* Promote solo nodes without merging */
			nodes[num_parents++] = inodes[0];
			lwfree(inodes);
		}
		else if ( inode_num < CIRC_NODE_SIZE-1 )
		{
			/* Merge spare nodes */
			nodes[num_parents++] = circ_node_internal_new(inodes, inode_num+1);
		}

		num_children = num_parents;
		num_parents = 0;
	}

	/* Return a reference to the head of the tree */
	return nodes[0];
}


/**
* Returns a #POINT2D that is a vertex of the input shape
*/
int circ_tree_get_point(const CIRC_NODE* node, POINT2D* pt)
{
	if ( circ_node_is_leaf(node) )
	{
		pt->x = node->p1->x;
		pt->y = node->p1->y;
		return LW_SUCCESS;
	}
	else
	{
		return circ_tree_get_point(node->nodes[0], pt);
	}
}

int circ_tree_get_point_outside(const CIRC_NODE* node, POINT2D* pt)
{
	POINT3D center3d;
	GEOGRAPHIC_POINT g;
	// if (node->radius >= M_PI) return LW_FAILURE;
	geog2cart(&(node->center), &center3d);
	vector_scale(&center3d, -1.0);
	cart2geog(&center3d, &g);
	pt->x = rad2deg(g.lon);
	pt->y = rad2deg(g.lat);
	return LW_SUCCESS;
}


/**
* Walk the tree and count intersections between the stab line and the edges.
* odd => containment, even => no containment.
* KNOWN PROBLEM: Grazings (think of a sharp point, just touching the
*   stabline) will be counted for one, which will throw off the count.
*/
int circ_tree_contains_point(const CIRC_NODE* node, const POINT2D* pt, const POINT2D* pt_outside, int level, int* on_boundary)
{
	GEOGRAPHIC_POINT closest;
	GEOGRAPHIC_EDGE stab_edge, edge;
	POINT3D S1, S2, E1, E2;
	double d;
	uint32_t i, c;

	/* Construct a stabline edge from our "inside" to our known outside point */
	geographic_point_init(pt->x, pt->y, &(stab_edge.start));
	geographic_point_init(pt_outside->x, pt_outside->y, &(stab_edge.end));
	geog2cart(&(stab_edge.start), &S1);
	geog2cart(&(stab_edge.end), &S2);

	LWDEBUGF(3, "%*s entered", level, "");

	/*
	* If the stabline doesn't cross within the radius of a node, there's no
	* way it can cross.
	*/

	LWDEBUGF(3, "%*s :working on node %p, edge_num %d, radius %g, center POINT(%.12g %.12g)", level, "", node, node->edge_num, node->radius, rad2deg(node->center.lon), rad2deg(node->center.lat));
	d = edge_distance_to_point(&stab_edge, &(node->center), &closest);
	LWDEBUGF(3, "%*s :edge_distance_to_point=%g, node_radius=%g", level, "", d, node->radius);
	if ( FP_LTEQ(d, node->radius) )
	{
		LWDEBUGF(3,"%*s :entering this branch (%p)", level, "", node);

		/* Return the crossing number of this leaf */
		if ( circ_node_is_leaf(node) )
		{
			int inter;
			LWDEBUGF(3, "%*s :leaf node calculation (edge %d)", level, "", node->edge_num);
			geographic_point_init(node->p1->x, node->p1->y, &(edge.start));
			geographic_point_init(node->p2->x, node->p2->y, &(edge.end));
			geog2cart(&(edge.start), &E1);
			geog2cart(&(edge.end), &E2);

			inter = edge_intersects(&S1, &S2, &E1, &E2);
			LWDEBUGF(3, "%*s :inter = %d", level, "", inter);

			if ( inter & PIR_INTERSECTS )
			{
				LWDEBUGF(3,"%*s ::got stab line edge_intersection with this edge!", level, "");
				/* To avoid double counting crossings-at-a-vertex, */
				/* always ignore crossings at "lower" ends of edges*/
				GEOGRAPHIC_POINT e1, e2;
				cart2geog(&E1,&e1); cart2geog(&E2,&e2);

				LWDEBUGF(3,"%*s LINESTRING(%.15g %.15g,%.15g %.15g)", level, "",
					pt->x, pt->y,
					pt_outside->x, pt_outside->y
					);

				LWDEBUGF(3,"%*s LINESTRING(%.15g %.15g,%.15g %.15g)", level, "",
					rad2deg(e1.lon), rad2deg(e1.lat),
					rad2deg(e2.lon), rad2deg(e2.lat)
					);

				if ( inter & PIR_B_TOUCH_RIGHT || inter & PIR_COLINEAR )
				{
					LWDEBUGF(3,"%*s ::rejecting stab line grazing by left-side edge", level, "");
					return 0;
				}
				else
				{
					LWDEBUGF(3,"%*s ::accepting stab line intersection", level, "");
					return 1;
				}
			}
			else
			{
				LWDEBUGF(3,"%*s edge does not intersect", level, "");
			}
		}
		/* Or, add up the crossing numbers of all children of this node. */
		else
		{
			c = 0;
			for ( i = 0; i < node->num_nodes; i++ )
			{
				LWDEBUGF(3,"%*s calling circ_tree_contains_point on child %d!", level, "", i);
				c += circ_tree_contains_point(node->nodes[i], pt, pt_outside, level + 1, on_boundary);
			}
			return c % 2;
		}
	}
	else
	{
		LWDEBUGF(3,"%*s skipping this branch (%p)", level, "", node);
	}
	return 0;
}

static double
circ_node_min_distance(const CIRC_NODE* n1, const CIRC_NODE* n2)
{
	double d = sphere_distance(&(n1->center), &(n2->center));
	double r1 = n1->radius;
	double r2 = n2->radius;

	if ( d < r1 + r2 )
		return 0.0;

	return d - r1 - r2;
}

static double
circ_node_max_distance(const CIRC_NODE *n1, const CIRC_NODE *n2)
{
	return sphere_distance(&(n1->center), &(n2->center)) + n1->radius + n2->radius;
}

double
circ_tree_distance_tree(const CIRC_NODE* n1, const CIRC_NODE* n2, const SPHEROID* spheroid, double threshold)
{
	double min_dist = FLT_MAX;
	double max_dist = FLT_MAX;
	GEOGRAPHIC_POINT closest1, closest2;
	/* Quietly decrease the threshold just a little to avoid cases where */
	/* the actual spheroid distance is larger than the sphere distance */
	/* causing the return value to be larger than the threshold value */
	double threshold_radians = 0.95 * threshold / spheroid->radius;

	circ_tree_distance_tree_internal(n1, n2, threshold_radians, &min_dist, &max_dist, &closest1, &closest2);

	/* Spherical case */
	if ( spheroid->a == spheroid->b )
	{
		return spheroid->radius * sphere_distance(&closest1, &closest2);
	}
	else
	{
		return spheroid_distance(&closest1, &closest2, spheroid);
	}
}


/***********************************************************************
* Internal node sorting routine to make distance calculations faster?
*/

struct sort_node {
	CIRC_NODE *node;
	double d;
};

static int
circ_nodes_sort_cmp(const void *a, const void *b)
{
	struct sort_node *node_a = (struct sort_node *)(a);
	struct sort_node *node_b = (struct sort_node *)(b);
	if (node_a->d < node_b->d) return -1;
	else if (node_a->d > node_b->d) return 1;
	else return 0;
}

static void
circ_internal_nodes_sort(CIRC_NODE **nodes, uint32_t num_nodes, const CIRC_NODE *target_node)
{
	uint32_t i;
	struct sort_node sort_nodes[CIRC_NODE_SIZE];

	/* Copy incoming nodes into sorting array and calculate */
	/* distance to the target node */
	for (i = 0; i < num_nodes; i++)
	{
		sort_nodes[i].node = nodes[i];
		sort_nodes[i].d = sphere_distance(&(nodes[i]->center), &(target_node->center));
	}

	/* Sort the nodes and copy the result back into the input array */
	qsort(sort_nodes, num_nodes, sizeof(struct sort_node), circ_nodes_sort_cmp);
	for (i = 0; i < num_nodes; i++)
	{
		nodes[i] = sort_nodes[i].node;
	}
	return;
}

/***********************************************************************/

static double
circ_tree_distance_tree_internal(const CIRC_NODE* n1, const CIRC_NODE* n2, double threshold, double* min_dist, double* max_dist, GEOGRAPHIC_POINT* closest1, GEOGRAPHIC_POINT* closest2)
{
	double max;
	double d, d_min;
	uint32_t i;

	LWDEBUGF(4, "entered, min_dist=%.8g max_dist=%.8g, type1=%d, type2=%d", *min_dist, *max_dist, n1->geom_type, n2->geom_type);

	// printf("-==-\n");
	// circ_tree_print(n1, 0);
	// printf("--\n");
	// circ_tree_print(n2, 0);

	/* Short circuit if we've already hit the minimum */
	if( *min_dist < threshold || *min_dist == 0.0 )
		return *min_dist;

	/* If your minimum is greater than anyone's maximum, you can't hold the winner */
	if( circ_node_min_distance(n1, n2) > *max_dist )
	{
		LWDEBUGF(4, "pruning pair %p, %p", n1, n2);
		return FLT_MAX;
	}

	/* If your maximum is a new low, we'll use that as our new global tolerance */
	max = circ_node_max_distance(n1, n2);
	LWDEBUGF(5, "max %.8g", max);
	if( max < *max_dist )
		*max_dist = max;

	/* Polygon on one side, primitive type on the other. Check for point-in-polygon */
	/* short circuit. */
	if ( n1->geom_type == POLYGONTYPE && n2->geom_type && ! lwtype_is_collection(n2->geom_type) )
	{
		POINT2D pt;
		circ_tree_get_point(n2, &pt);
		LWDEBUGF(4, "n1 is polygon, testing if contains (%.5g,%.5g)", pt.x, pt.y);
		if ( circ_tree_contains_point(n1, &pt, &(n1->pt_outside), 0, NULL) )
		{
			LWDEBUG(4, "it does");
			*min_dist = 0.0;
			geographic_point_init(pt.x, pt.y, closest1);
			geographic_point_init(pt.x, pt.y, closest2);
			return *min_dist;
		}
	}
	/* Polygon on one side, primitive type on the other. Check for point-in-polygon */
	/* short circuit. */
	if ( n2->geom_type == POLYGONTYPE && n1->geom_type && ! lwtype_is_collection(n1->geom_type) )
	{
		POINT2D pt;
		circ_tree_get_point(n1, &pt);
		LWDEBUGF(4, "n2 is polygon, testing if contains (%.5g,%.5g)", pt.x, pt.y);
		if ( circ_tree_contains_point(n2, &pt, &(n2->pt_outside), 0, NULL) )
		{
			LWDEBUG(4, "it does");
			geographic_point_init(pt.x, pt.y, closest1);
			geographic_point_init(pt.x, pt.y, closest2);
			*min_dist = 0.0;
			return *min_dist;
		}
	}

	/* Both leaf nodes, do a real distance calculation */
	if( circ_node_is_leaf(n1) && circ_node_is_leaf(n2) )
	{
		double d;
		GEOGRAPHIC_POINT close1, close2;
		LWDEBUGF(4, "testing leaf pair [%d], [%d]", n1->edge_num, n2->edge_num);
		/* One of the nodes is a point */
		if ( n1->p1 == n1->p2 || n2->p1 == n2->p2 )
		{
			GEOGRAPHIC_EDGE e;
			GEOGRAPHIC_POINT gp1, gp2;

			/* Both nodes are points! */
			if ( n1->p1 == n1->p2 && n2->p1 == n2->p2 )
			{
				geographic_point_init(n1->p1->x, n1->p1->y, &gp1);
				geographic_point_init(n2->p1->x, n2->p1->y, &gp2);
				close1 = gp1; close2 = gp2;
				d = sphere_distance(&gp1, &gp2);
			}
			/* Node 1 is a point */
			else if ( n1->p1 == n1->p2 )
			{
				geographic_point_init(n1->p1->x, n1->p1->y, &gp1);
				geographic_point_init(n2->p1->x, n2->p1->y, &(e.start));
				geographic_point_init(n2->p2->x, n2->p2->y, &(e.end));
				close1 = gp1;
				d = edge_distance_to_point(&e, &gp1, &close2);
			}
			/* Node 2 is a point */
			else
			{
				geographic_point_init(n2->p1->x, n2->p1->y, &gp1);
				geographic_point_init(n1->p1->x, n1->p1->y, &(e.start));
				geographic_point_init(n1->p2->x, n1->p2->y, &(e.end));
				close1 = gp1;
				d = edge_distance_to_point(&e, &gp1, &close2);
			}
			LWDEBUGF(4, "  got distance %g", d);
		}
		/* Both nodes are edges */
		else
		{
			GEOGRAPHIC_EDGE e1, e2;
			GEOGRAPHIC_POINT g;
			POINT3D A1, A2, B1, B2;
			geographic_point_init(n1->p1->x, n1->p1->y, &(e1.start));
			geographic_point_init(n1->p2->x, n1->p2->y, &(e1.end));
			geographic_point_init(n2->p1->x, n2->p1->y, &(e2.start));
			geographic_point_init(n2->p2->x, n2->p2->y, &(e2.end));
			geog2cart(&(e1.start), &A1);
			geog2cart(&(e1.end), &A2);
			geog2cart(&(e2.start), &B1);
			geog2cart(&(e2.end), &B2);
			if ( edge_intersects(&A1, &A2, &B1, &B2) )
			{
				d = 0.0;
				edge_intersection(&e1, &e2, &g);
				close1 = close2 = g;
			}
			else
			{
				d = edge_distance_to_edge(&e1, &e2, &close1, &close2);
			}
			LWDEBUGF(4, "edge_distance_to_edge returned %g", d);
		}
		if ( d < *min_dist )
		{
			*min_dist = d;
			*closest1 = close1;
			*closest2 = close2;
		}
		return d;
	}
	else
	{
		d_min = FLT_MAX;
		/* Drive the recursion into the COLLECTION types first so we end up with */
		/* pairings of primitive geometries that can be forced into the point-in-polygon */
		/* tests above. */
		if ( n1->geom_type && lwtype_is_collection(n1->geom_type) )
		{
			circ_internal_nodes_sort(n1->nodes, n1->num_nodes, n2);
			for ( i = 0; i < n1->num_nodes; i++ )
			{
				d = circ_tree_distance_tree_internal(n1->nodes[i], n2, threshold, min_dist, max_dist, closest1, closest2);
				d_min = FP_MIN(d_min, d);
			}
		}
		else if ( n2->geom_type && lwtype_is_collection(n2->geom_type) )
		{
			circ_internal_nodes_sort(n2->nodes, n2->num_nodes, n1);
			for ( i = 0; i < n2->num_nodes; i++ )
			{
				d = circ_tree_distance_tree_internal(n1, n2->nodes[i], threshold, min_dist, max_dist, closest1, closest2);
				d_min = FP_MIN(d_min, d);
			}
		}
		else if ( ! circ_node_is_leaf(n1) )
		{
			circ_internal_nodes_sort(n1->nodes, n1->num_nodes, n2);
			for ( i = 0; i < n1->num_nodes; i++ )
			{
				d = circ_tree_distance_tree_internal(n1->nodes[i], n2, threshold, min_dist, max_dist, closest1, closest2);
				d_min = FP_MIN(d_min, d);
			}
		}
		else if ( ! circ_node_is_leaf(n2) )
		{
			circ_internal_nodes_sort(n2->nodes, n2->num_nodes, n1);
			for ( i = 0; i < n2->num_nodes; i++ )
			{
				d = circ_tree_distance_tree_internal(n1, n2->nodes[i], threshold, min_dist, max_dist, closest1, closest2);
				d_min = FP_MIN(d_min, d);
			}
		}
		else
		{
			/* Never get here */
		}

		return d_min;
	}
}





void circ_tree_print(const CIRC_NODE* node, int depth)
{
	uint32_t i;

	if (circ_node_is_leaf(node))
	{
		printf("%*s[%d] C(%.5g %.5g) R(%.5g) ((%.5g %.5g),(%.5g,%.5g))",
		  3*depth + 6, "NODE", node->edge_num,
		  node->center.lon, node->center.lat,
		  node->radius,
		  node->p1->x, node->p1->y,
		  node->p2->x, node->p2->y
		);
  		if ( node->geom_type )
  		{
  			printf(" %s", lwtype_name(node->geom_type));
  		}
  		if ( node->geom_type == POLYGONTYPE )
  		{
  			printf(" O(%.5g %.5g)", node->pt_outside.x, node->pt_outside.y);
  		}
  		printf("\n");

	}
	else
	{
		printf("%*s C(%.5g %.5g) R(%.5g)",
		  3*depth + 6, "NODE",
		  node->center.lon, node->center.lat,
		  node->radius
		);
		if ( node->geom_type )
		{
			printf(" %s", lwtype_name(node->geom_type));
		}
  		if ( node->geom_type == POLYGONTYPE )
  		{
  			printf(" O(%.15g %.15g)", node->pt_outside.x, node->pt_outside.y);
  		}
		printf("\n");
	}
	for ( i = 0; i < node->num_nodes; i++ )
	{
		circ_tree_print(node->nodes[i], depth + 1);
	}
	return;
}


static CIRC_NODE*
lwpoint_calculate_circ_tree(const LWPOINT* lwpoint)
{
	CIRC_NODE* node;
	node = circ_tree_new(lwpoint->point);
	node->geom_type = lwgeom_get_type((LWGEOM*)lwpoint);;
	return node;
}

static CIRC_NODE*
lwline_calculate_circ_tree(const LWLINE* lwline)
{
	CIRC_NODE* node;
	node = circ_tree_new(lwline->points);
	node->geom_type = lwgeom_get_type((LWGEOM*)lwline);
	return node;
}

static CIRC_NODE*
lwpoly_calculate_circ_tree(const LWPOLY* lwpoly)
{
	uint32_t i = 0, j = 0;
	CIRC_NODE** nodes;
	CIRC_NODE* node;

	/* One ring? Handle it like a line. */
	if ( lwpoly->nrings == 1 )
	{
		node = circ_tree_new(lwpoly->rings[0]);
	}
	else
	{
		/* Calculate a tree for each non-trivial ring of the polygon */
		nodes = lwalloc(lwpoly->nrings * sizeof(CIRC_NODE*));
		for ( i = 0; i < lwpoly->nrings; i++ )
		{
			node = circ_tree_new(lwpoly->rings[i]);
			if ( node )
				nodes[j++] = node;
		}
		/* Put the trees into a spatially correlated order */
		circ_nodes_sort(nodes, j);
		/* Merge the trees pairwise up to a parent node and return */
		node = circ_nodes_merge(nodes, j);
		/* Don't need the working list any more */
		lwfree(nodes);
	}

	/* Metadata about polygons, we need this to apply P-i-P tests */
	/* selectively when doing distance calculations */
	node->geom_type = lwgeom_get_type((LWGEOM*)lwpoly);
	lwpoly_pt_outside(lwpoly, &(node->pt_outside));

	return node;
}

static CIRC_NODE*
lwcollection_calculate_circ_tree(const LWCOLLECTION* lwcol)
{
	uint32_t i = 0, j = 0;
	CIRC_NODE** nodes;
	CIRC_NODE* node;

	/* One geometry? Done! */
	if ( lwcol->ngeoms == 1 )
		return lwgeom_calculate_circ_tree(lwcol->geoms[0]);

	/* Calculate a tree for each sub-geometry*/
	nodes = lwalloc(lwcol->ngeoms * sizeof(CIRC_NODE*));
	for ( i = 0; i < lwcol->ngeoms; i++ )
	{
		node = lwgeom_calculate_circ_tree(lwcol->geoms[i]);
		if ( node )
			nodes[j++] = node;
	}
	/* Put the trees into a spatially correlated order */
	circ_nodes_sort(nodes, j);
	/* Merge the trees pairwise up to a parent node and return */
	node = circ_nodes_merge(nodes, j);
	/* Don't need the working list any more */
	lwfree(nodes);
	node->geom_type = lwgeom_get_type((LWGEOM*)lwcol);
	return node;
}

CIRC_NODE*
lwgeom_calculate_circ_tree(const LWGEOM* lwgeom)
{
	if ( lwgeom_is_empty(lwgeom) )
		return NULL;

	switch ( lwgeom->type )
	{
		case POINTTYPE:
			return lwpoint_calculate_circ_tree((LWPOINT*)lwgeom);
		case LINETYPE:
			return lwline_calculate_circ_tree((LWLINE*)lwgeom);
		case POLYGONTYPE:
			return lwpoly_calculate_circ_tree((LWPOLY*)lwgeom);
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case COLLECTIONTYPE:
			return lwcollection_calculate_circ_tree((LWCOLLECTION*)lwgeom);
		default:
			lwerror("Unable to calculate spherical index tree for type %s", lwtype_name(lwgeom->type));
			return NULL;
	}

}
