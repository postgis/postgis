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

typedef enum
{
	RECT_NODE_INTERNAL,
	RECT_NODE_LEAF
} RECT_NODE_TYPE;

typedef enum
{
	RECT_NODE_SEG_LINEAR,
	RECT_NODE_SEG_CIRCULAR,
	RECT_NODE_SEG_UNKNOWN
} RECT_NODE_SEG_TYPE;

typedef struct
{
	const POINTARRAY *pa;
	RECT_NODE_SEGMENT seg_type;
	int seg_num;
} RECT_NODE_LEAF;

typedef struct
{
	num_nodes;
	RECT_NODE *nodes[RECT_NODE_SIZE];
} RECT_NODE_INTERNAL;

struct rect_node
{
	RECT_NODE_TYPE type;
	unsigned int geom_type;
	double xmin;
	double xmax;
	double ymin;
	double ymax;
	union {
		RECT_NODE_INTERNAL i;
		RECT_NODE_LEAF l;
	}
} RECT_NODE;



int rect_tree_contains_point(const RECT_NODE *tree, const POINT2D *pt, int *on_boundary);
int rect_tree_intersects_tree(const RECT_NODE *tree1, const RECT_NODE *tree2);
void rect_tree_free(RECT_NODE *node);
RECT_NODE* rect_node_leaf_new(const POINTARRAY *pa, int i);
RECT_NODE* rect_node_internal_new(RECT_NODE *left_node, RECT_NODE *right_node);
RECT_NODE* rect_tree_from_ptarray(const POINTARRAY *pa);
RECT_NODE* rect_tree_from_lwgeom(const LWGOEM *geom);
