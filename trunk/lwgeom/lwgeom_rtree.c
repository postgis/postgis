/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2005 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 * 
 **********************************************************************/

#include "lwgeom_pg.h"
#include "liblwgeom.h"
#include "lwgeom_rtree.h"

/*
 * Creates an rtree given a pointer to the point array.
 * Must copy the point array.
 */
RTREE_NODE *createTree(POINTARRAY *pointArray)
{
        RTREE_NODE *root;
        RTREE_NODE *nodes[pointArray->npoints];
        int i, nodeCount;
        int childNodes, parentNodes;

#ifdef PGIS_DEBUG_CALLS
        lwnotice("createTree called with pointarray %p", pointArray);
#endif

        nodeCount = pointArray->npoints - 1;

#ifdef PGIS_DEBUG
        lwnotice("Total leaf nodes: %d", nodeCount);
#endif

        /*
         * Create a leaf node for every line segment.
         */
        for(i = 0; i < nodeCount; i++)
        {
                nodes[i] = createLeafNode(pointArray, i);
        }

        /*
         * Next we group nodes by pairs.  If there's an odd number of nodes,
         * we bring the last node up a level as is.  Continue until we have
         * a single top node.
         */
        childNodes = nodeCount;
        parentNodes = nodeCount / 2;
        while(parentNodes > 0)
        {
#ifdef PGIS_DEBUG
                lwnotice("Merging %d children into %d parents.", childNodes, parentNodes);
#endif
                i = 0;
                while(i < parentNodes) 
                {
                        nodes[i] = createInteriorNode(nodes[i*2], nodes[i*2+1]);
                        i++;
                }
                /*
                 * Check for an odd numbered final node.
                 */
                if(parentNodes * 2 < childNodes)
                {
#ifdef PGIS_DEBUG
                        lwnotice("Shuffling child %d to parent %d", childNodes - 1, i);
#endif
                        nodes[i] = nodes[childNodes - 1];
                        parentNodes++;
                }
                childNodes = parentNodes;
                parentNodes = parentNodes / 2;
        }

        root = nodes[0];

#ifdef PGIS_DEBUG
        lwnotice("createTree returning %p", root);
#endif
        return root;
}

/* 
 * Creates an interior node given the children. 
 */
RTREE_NODE *createInteriorNode(RTREE_NODE *left, RTREE_NODE *right){
        RTREE_NODE *parent;

#ifdef PGIS_DEBUG_CALLS
        lwnotice("createInteriorNode called for children %p, %p", left, right);
#endif
        parent = lwalloc(sizeof(RTREE_NODE));
        parent->leftNode = left;
        parent->rightNode = right;
        parent->interval = mergeIntervals(left->interval, right->interval);
        parent->segment = NULL;
#ifdef PGIS_DEBUG_CALLS
        lwnotice("createInteriorNode returning %p", parent);
#endif
        return parent;
}

/*
 * Creates a leaf node given the pointer to the start point of the segment.
 */
RTREE_NODE *createLeafNode(POINTARRAY *pa, int startPoint)
{
        RTREE_NODE *parent;
        LWLINE *line;
        double value1;
        double value2;
        POINT4D tmp;
        POINTARRAY *npa;

#ifdef PGIS_DEBUG_CALLS
        lwnotice("createLeafNode called for point %d of %p", startPoint, pa);
#endif
        if(pa->npoints < startPoint + 2)
        {
                lwerror("createLeafNode: npoints = %d, startPoint = %d", pa->npoints, startPoint);
        }

        /*
         * The given point array will be part of a geometry that will be freed
         * independently of the index.  Since we may want to cache the index,
         * we must create independent arrays.
         */         
        npa = lwalloc(sizeof(POINTARRAY));
        npa->dims = 0;
        npa->npoints = 2;
        TYPE_SETZM(npa->dims, 0, 0);
        npa->serialized_pointlist = lwalloc(pointArray_ptsize(pa) * 2);

        getPoint4d_p(pa, startPoint, &tmp);

        setPoint4d(npa, 0, &tmp);
        value1 = tmp.y;

        getPoint4d_p(pa, startPoint + 1, &tmp);

        setPoint4d(npa, 1, &tmp);
        value2 = tmp.y;
        
        line = lwline_construct(-1, NULL, npa);
        
        parent = lwalloc(sizeof(RTREE_NODE));
        parent->interval = createInterval(value1, value2);
        parent->segment = line;
        parent->leftNode = NULL;
        parent->rightNode = NULL;

#ifdef PGIS_DEBUG_CALLS
        lwnotice("createLeafNode returning %p", parent);
#endif
        return parent;
}

/*
 * Creates an interval with the total extents of the two given intervals.
 */
INTERVAL *mergeIntervals(INTERVAL *inter1, INTERVAL *inter2)
{
        INTERVAL *interval;

#ifdef PGIS_DEBUG_CALLS
        lwnotice("mergeIntervals called with %p, %p", inter1, inter2);
#endif
        interval = lwalloc(sizeof(INTERVAL));
        interval->max = FP_MAX(inter1->max, inter2->max);
        interval->min = FP_MIN(inter1->min, inter2->min);
#ifdef PGIS_DEBUG
        lwnotice("interval min = %8.3f, max = %8.3f", interval->min, interval->max);
#endif
        return interval;
}

/*
 * Creates an interval given the min and max values, in arbitrary order.
 */
INTERVAL *createInterval(double value1, double value2)
{
        INTERVAL *interval;

#ifdef PGIS_DEBUG_CALLS
        lwnotice("createInterval called with %8.3f, %8.3f", value1, value2);
#endif
        interval = lwalloc(sizeof(INTERVAL));
        interval->max = FP_MAX(value1, value2);
        interval->min = FP_MIN(value1, value2);
#ifdef PGIS_DEBUG
        lwnotice("interval min = %8.3f, max = %8.3f", interval->min, interval->max);
#endif
        return interval;
}

/*
 * Recursively frees the child nodes, the interval and the line before 
 * freeing the root node.
 */
void freeTree(RTREE_NODE *root)
{

#ifdef PGIS_DEBUG_CALLS
        lwnotice("freeTree called for %p", root);
#endif
        if(root->leftNode)
                freeTree(root->leftNode);
        if(root->rightNode)
                freeTree(root->rightNode);
        lwfree(root->interval);
        if(root->segment)
                lwfree(root->segment);
        lwfree(root);
}

/*
 * Retrieves a collection of line segments given the root and crossing value.
 * The collection is a multilinestring consisting of two point lines 
 * representing the segments of the ring that may be crossed by the 
 * horizontal projection line at the given y value.
 */
LWMLINE *findLineSegments(RTREE_NODE *root, double value)
{
        LWMLINE *tmp, *result;
        LWGEOM **lwgeoms;
        
#ifdef PGIS_DEBUG_CALLS
        lwnotice("findLineSegments called for tree %p and value %8.3f", root, value);
#endif
        result = NULL;

        if(!isContained(root->interval, value))
        {
#ifdef PGIS_DEBUG
                lwnotice("findLineSegments %p: not contained.", root);
#endif
                return NULL;
        }

        /* If there is a segment defined for this node, include it. */
        if(root->segment)
        {
#ifdef PGIS_DEBUG
                lwnotice("findLineSegments %p: adding segment %p %d.", root, root->segment, TYPE_GETTYPE(root->segment->type));
#endif
                lwgeoms = lwalloc(sizeof(LWGEOM *));
                lwgeoms[0] = (LWGEOM *)root->segment;

#ifdef PGIS_DEBUG
                lwnotice("Found geom %p, type %d, dim %d", root->segment, TYPE_GETTYPE(root->segment->type), TYPE_GETZM(root->segment->type));
#endif
                result = (LWMLINE *)lwcollection_construct(lwgeom_makeType_full(0, 0, 0, MULTILINETYPE, 0), -1, NULL, 1, lwgeoms);
        }

        /* If there is a left child node, recursively include its results. */
        if(root->leftNode)
        {
#ifdef PGIS_DEBUG
                lwnotice("findLineSegments %p: recursing left.", root);
#endif
                tmp = findLineSegments(root->leftNode, value);
                if(tmp)
                {
#ifdef PGIS_DEBUG
                        lwnotice("Found geom %p, type %d, dim %d", tmp, TYPE_GETTYPE(tmp->type), TYPE_GETZM(tmp->type));
#endif
                        if(result)
                                result = mergeMultiLines(result, tmp);
                        else
                                result = tmp;
                }
        }

        /* Same for any right child. */
        if(root->rightNode)
        {
#ifdef PGIS_DEBUG
                lwnotice("findLineSegments %p: recursing right.", root);
#endif
                tmp = findLineSegments(root->rightNode, value);
                if(tmp)
                {
#ifdef PGIS_DEBUG
                        lwnotice("Found geom %p, type %d, dim %d", tmp, TYPE_GETTYPE(tmp->type), TYPE_GETZM(tmp->type));
#endif
                        if(result)
                                result = mergeMultiLines(result, tmp);
                        else
                                result = tmp;
                }
        }
                        
        return result;
}

/* Merges two multilinestrings into a single multilinestring. */
LWMLINE *mergeMultiLines(LWMLINE *line1, LWMLINE *line2)
{
        LWGEOM **geoms;
        LWCOLLECTION *col;
        int i, j, ngeoms;
        
#ifdef PGIS_DEBUG_CALLS
        lwnotice("mergeMultiLines called on %p, %d, %d; %p, %d, %d", line1, line1->ngeoms, TYPE_GETTYPE(line1->type), line2, line2->ngeoms, TYPE_GETTYPE(line2->type));
#endif
        ngeoms = line1->ngeoms + line2->ngeoms;
        geoms = lwalloc(sizeof(LWGEOM *) * ngeoms);

        j = 0;
        for(i = 0; i < line1->ngeoms; i++, j++)
        {
                geoms[j] = lwgeom_clone((LWGEOM *)line1->geoms[i]);
        }
        for(i = 0; i < line2->ngeoms; i++, j++)
        {
                geoms[j] = lwgeom_clone((LWGEOM *)line2->geoms[i]);
        }
        col = lwcollection_construct(MULTILINETYPE, -1, NULL, ngeoms, geoms);

#ifdef PGIS_DEBUG_CALLS
        lwnotice("mergeMultiLines returning %p, %d, %d", col, col->ngeoms, TYPE_GETTYPE(col->type));
#endif

        return (LWMLINE *)col;
}

/*
 * Returns 1 if min < value <= max, 0 otherwise. */
uint32 isContained(INTERVAL *interval, double value)
{
        return FP_CONTAINS_INCL(interval->min, value, interval->max) ? 1 : 0;
}

PG_FUNCTION_INFO_V1(LWGEOM_polygon_index);
Datum LWGEOM_polygon_index(PG_FUNCTION_ARGS)
{
        PG_LWGEOM *igeom, *result;
        LWGEOM *geom;
        LWPOLY *poly;
        LWMLINE *mline;
        RTREE_NODE *root;
        double yval;
        
#ifdef PGIS_DEBUG_CALLS
        int i;
        lwnotice("polygon_index called.");
#endif
        result = NULL;
        igeom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
        yval = PG_GETARG_FLOAT8(1);
        geom = lwgeom_deserialize(SERIALIZED_FORM(igeom));
        if(TYPE_GETTYPE(geom->type) != POLYGONTYPE)
        {
                lwgeom_release(geom);
                PG_FREE_IF_COPY(igeom, 0);
                PG_RETURN_NULL();
        }
        poly = (LWPOLY *)geom;
        root = createTree(poly->rings[0]);

        mline = findLineSegments(root, yval);

#ifdef PGIS_DEBUG
        lwnotice("mline returned %p %d", mline, TYPE_GETTYPE(mline->type));
        for(i = 0; i < mline->ngeoms; i++)
        {
                lwnotice("geom[%d] %p %d", i, mline->geoms[i], TYPE_GETTYPE(mline->geoms[i]->type));
        }
#endif

        if(mline)
                result = pglwgeom_serialize((LWGEOM *)mline);

#ifdef PGIS_DEBUG
        lwnotice("returning result %p", result);
#endif
        PG_FREE_IF_COPY(igeom, 0);
        lwgeom_release((LWGEOM *)poly);
        lwgeom_release((LWGEOM *)mline);
        PG_RETURN_POINTER(result);
        
}

RTREE_POLY_CACHE *createNewCache(LWPOLY *poly, uchar *serializedPoly)
{
        RTREE_POLY_CACHE *result;
        int i, length;

#ifdef PGIS_DEBUG_CALLS
        lwnotice("createNewCache called with %p", poly);
#endif
        result = lwalloc(sizeof(RTREE_POLY_CACHE));
        result->ringIndices = lwalloc(sizeof(RTREE_NODE *) * poly->nrings);
        result->ringCount = poly->nrings;
        length = lwgeom_size_poly(serializedPoly);
        result->poly = lwalloc(length);
        memcpy(result->poly, serializedPoly, length); 
        for(i = 0; i < result->ringCount; i++)
        {
                result->ringIndices[i] = createTree(poly->rings[i]);
        }
#ifdef PGIS_DEBUG
        lwnotice("createNewCache returning %p", result);
#endif
        return result;
}

/* 
 * Creates a new cachable index if needed, or returns the current cache if
 * it is applicable to the current polygon.
 * The memory context must be changed to function scope before calling this
 * method.  The method will allocate memory for the cache it creates,
 * as well as freeing the memory of any cache that is no longer applicable.
 */
RTREE_POLY_CACHE *retrieveCache(LWPOLY *poly, uchar *serializedPoly, 
                RTREE_POLY_CACHE *currentCache)
{
        int i, length;

#ifdef PGIS_DEBUG_CALLS
        lwnotice("retrieveCache called with %p %p %p", poly, serializedPoly, currentCache);
#endif
        if(!currentCache)
        {
#ifdef PGIS_DEBUG
                lwnotice("No existing cache, create one.");
#endif
                return createNewCache(poly, serializedPoly);
        }
        if(!(currentCache->poly))
        {
#ifdef PGIS_DEBUG
                lwnotice("Cache contains no polygon, creating new cache.");
#endif
                return createNewCache(poly, serializedPoly);
        }

        length = lwgeom_size_poly(serializedPoly);

        if(lwgeom_size_poly(currentCache->poly) != length)
        {
#ifdef PGIS_DEBUG
                lwnotice("Polygon size mismatch, creating new cache.");
#endif
                lwfree(currentCache->poly);
                lwfree(currentCache);
                return createNewCache(poly, serializedPoly);
        }
        for(i = 0; i < length; i++) 
        {
                uchar a = serializedPoly[i];
                uchar b = currentCache->poly[i];
                if(a != b) 
                {
#ifdef PGIS_DEBUG
                        lwnotice("Polygon mismatch, creating new cache. %c, %c", a, b);
#endif
                        lwfree(currentCache->poly);
                        lwfree(currentCache);
                        return createNewCache(poly, serializedPoly);
                }
        }

#ifdef PGIS_DEBUG
        lwnotice("Polygon match, retaining current cache, %p.", currentCache);
#endif
        return currentCache;
}

