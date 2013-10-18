#ifndef _LWTREE_H
#define _LWTREE_H 1

#define RECT_NODE_SIZE 2

/**
* Note that pa is a pointer into an independent POINTARRAY, do not free it.
*/
typedef struct rect_node
{
	double xmin;
	double xmax;
	double ymin;
	double ymax;
	int num_nodes;
	struct rect_node **nodes;
	const POINTARRAY *pa;
	int first_point; /* what's the first point on the edge this node represents */
	int num_points; /* 1 for points, 2 for linear edges, 3 for curved edges */
} RECT_NODE;

int rect_tree_contains_point(const RECT_NODE *tree, const POINT2D *pt, int *on_boundary);
int rect_tree_intersects_tree(const RECT_NODE *tree1, const RECT_NODE *tree2);
double rect_tree_distance_tree(const RECT_NODE *tree1, const RECT_NODE *tree2, double threshold, double *min_dist, double *max_dist, POINT2D *closest1, POINT2D *closest2);
void rect_tree_free(RECT_NODE *node);
RECT_NODE* rect_tree_new(const POINTARRAY *pa);
RECT_NODE* lwgeom_calculate_rect_tree(const LWGEOM *lwgeom);

#endif /* _LWTREE_H */