#ifndef _LWGEOM_RTREE_H
#define _LWGEOM_RTREE_H

#include "liblwgeom.h"

typedef struct
{
	double min;
	double max;
}
INTERVAL;

/* Returns 1 if min < value <= max, 0 otherwise */
uint32 isContained(INTERVAL *interval, double value);
/* Creates an interval given the min and max values, in whatever order. */
INTERVAL *createInterval(double value1, double value2);
/* Creates an interval with the total extents of the two given intervals. */
INTERVAL *mergeIntervals(INTERVAL *inter1, INTERVAL *inter2);

/*
 * The following struct and methods are used for a 1D RTree implementation,
 * described at:
 *  http://lin-ear-th-inking.blogspot.com/2007/06/packed-1-dimensional-r-tree.html
 */
typedef struct rtree_node
{
	INTERVAL *interval;
	struct rtree_node *leftNode;
	struct rtree_node *rightNode;
	LWLINE *segment;
}
RTREE_NODE;

/* Creates an interior node given the children. */
RTREE_NODE *createInteriorNode(RTREE_NODE *left, RTREE_NODE *right);
/* Creates a leaf node given the pointer to the start point of the segment. */
RTREE_NODE *createLeafNode(POINTARRAY *pa, int startPoint);
/*
 * Creates an rtree given a pointer to the point array.
 * Must copy the point array.
 */
RTREE_NODE *createTree(POINTARRAY *pointArray);
/* Frees the tree. */
void freeTree(RTREE_NODE *root);
/* Retrieves a collection of line segments given the root and crossing value. */
LWMLINE *findLineSegments(RTREE_NODE *root, double value);
/* Merges two multilinestrings into a single multilinestring. */
LWMLINE *mergeMultiLines(LWMLINE *line1, LWMLINE *line2);

typedef struct
{
	char type;
	RTREE_NODE **ringIndices;
	int* ringCounts;
	int polyCount;
	GSERIALIZED *poly;
}
RTREE_POLY_CACHE;

/*
 * Creates a new cachable index if needed, or returns the current cache if
 * it is applicable to the current polygon.
 */
RTREE_POLY_CACHE *retrieveCache(LWGEOM *lwgeom, GSERIALIZED *serializedPoly, RTREE_POLY_CACHE *currentCache);
RTREE_POLY_CACHE *createCache(void);
/* Frees the cache. */
void populateCache(RTREE_POLY_CACHE *cache, LWGEOM *lwgeom, GSERIALIZED *serializedPoly);
void clearCache(RTREE_POLY_CACHE *cache);



#endif /* !defined _LIBLWGEOM_H */
