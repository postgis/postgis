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

#define RECT_NODE_SIZE 4

typedef struct rect_node
{
	/* boundings of the node */
	double xmin;
	double xmax;
	double ymin;
	double ymax;
	/* interior nodes only */
    int geom_type; /* for interior nodes, the geometry type it is proxying for */
	unsigned int num_nodes; /* how many children? */
	struct rect_node *nodes[RECT_NODE_SIZE];
	/* leaf nodes only */
	const POINT2D *p1;
	const POINT2D *p2;
} RECT_NODE;

int rect_tree_contains_point(const RECT_NODE *tree, const POINT2D *pt, int *on_boundary);
int rect_tree_intersects_tree(const RECT_NODE *tree1, const RECT_NODE *tree2);
void rect_tree_free(RECT_NODE *node);
RECT_NODE* rect_node_leaf_new(const POINTARRAY *pa, int i);
RECT_NODE* rect_node_internal_new(RECT_NODE *left_node, RECT_NODE *right_node);
RECT_NODE* rect_tree_from_ptarray(const POINTARRAY *pa);
RECT_NODE* rect_tree_from_lwgeom(const LWGOEM *geom);
