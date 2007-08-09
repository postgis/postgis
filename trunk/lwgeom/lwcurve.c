/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2006 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 * 
 **********************************************************************/

/* basic LWCURVE functions */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "liblwgeom.h"

/*#define PGIS_DEBUG_CALLS 1 */
/*#define PGIS_DEBUG 1 */

#ifndef MAXFLOAT
  #define MAXFLOAT      3.40282347e+38F
#endif

/*
 * Construct a new LWCURVE.  points will *NOT* be copied
 * use SRID=-1 for unknown SRID (will have 8bit type's S = 0)
 */
LWCURVE *
lwcurve_construct(int SRID, BOX2DFLOAT4 *bbox, POINTARRAY *points)
{
        /*
         * The first arc requires three points.  Each additional
         * arc requires two more points.  Thus the minimum point count
         * is three, and the count must be odd.
         */
        if(points->npoints % 2 != 1 || points->npoints < 3) 
        {
                lwerror("lwcurve_construct: invalid point count %d", points->npoints);
                return NULL;
        }
        
        LWCURVE *result;
        result = (LWCURVE*) lwalloc(sizeof(LWCURVE));

        result->type = lwgeom_makeType_full(
                TYPE_HASZ(points->dims),
                TYPE_HASM(points->dims),
                (SRID!=-1), CURVETYPE, 0);
        result->SRID = SRID;
        result->points = points;
        result->bbox = bbox;
         
        return result;
}

/*
 * given the LWGEOM serialized form (or a point into a multi* one)
 * construct a propert LWCURVE.
 * serialized_form should point to the 8bit type format (with type = 8)
 * See serialized form doc
 */
LWCURVE *
lwcurve_deserialize(uchar *serialized_form)
{
        uchar type;
        LWCURVE *result;
        uchar *loc=NULL;
        uint32 npoints;
        POINTARRAY *pa;

        type = (uchar)serialized_form[0];
        if(lwgeom_getType(type) != CURVETYPE)
        {
                lwerror("lwcurve_deserialize: attempt to deserialize a curve which is really a %s", lwgeom_typename(type));
                return NULL;
        }

        result = (LWCURVE*) lwalloc(sizeof(LWCURVE));
        result->type = type;

        loc = serialized_form + 1;

        if(lwgeom_hasBBOX(type))
        {
#ifdef PGIS_DEBUG
                lwnotice("lwcurve_deserialize: input has bbox");
#endif
                result->bbox = lwalloc(sizeof(BOX2DFLOAT4));
                memcpy(result->bbox, loc, sizeof(BOX2DFLOAT4));
                loc += sizeof(BOX2DFLOAT4);               
         }
         else
         {
#ifdef PGIS_DEBUG
                lwnotice("lwcurve_deserialize: input lacks bbox");           
#endif               
                result->bbox = NULL;
         }

         if(lwgeom_hasSRID(type))
         {
#ifdef PGIS_DEBUG
                lwnotice("lwcurve_deserialize: input has srid");
#endif
                result->SRID = get_int32(loc);               
                loc += 4; /* type + SRID */
         }
         else
         {
#ifdef PGIS_DEBUG
                lwnotice("lwcurve_deserialize: input lacks srid");
#endif
                result->SRID = -1;                
         }

         /* we've read the type (1 byte) and SRID (4 bytes, if present) */

         npoints = get_uint32(loc);
#ifdef PGIS_DEBUG
         lwnotice("curve npoints = %d", npoints);
#endif         
         loc += 4;
         pa = pointArray_construct(loc, TYPE_HASZ(type), TYPE_HASM(type), npoints);
         result->points = pa;
         return result;
}

/*
 * convert this curve into its serialized form
 * result's first char will be the 8bit type. See serialized form doc
 */
uchar *
lwcurve_serialize(LWCURVE *curve)
{
        size_t size, retsize;
        uchar * result;

        if(curve == NULL) {
                lwerror("lwcurve_serialize:: given null curve");
                return NULL;
        }

        size = lwcurve_serialize_size(curve);
        result = lwalloc(size);
        lwcurve_serialize_buf(curve, result, &retsize);
        if(retsize != size)
                lwerror("lwcurve_serialize_size returned %d, ..selialize_buf returned %d", size, retsize);
        return result;
}

/*
 * convert this curve into its serialized form writing it into 
 * the given buffer, and returning number of bytes written into 
 * the given int pointer.
 * result's first char will be the 8bit type.  See serialized form doc
 */
void lwcurve_serialize_buf(LWCURVE *curve, uchar *buf, size_t *retsize)
{
        char hasSRID;
        uchar *loc;
        int ptsize;
        size_t size;

#ifdef PGIS_DEBUG_CALLS
        lwnotice("lwcurve_serialize_buf(%p, %p, %p) called",
                curve, buf, retsize);
#endif

        if(curve == NULL) 
        {
                lwerror("lwcurve_serialize:: given null curve");
                return;
        }

        if(TYPE_GETZM(curve->type) != TYPE_GETZM(curve->points->dims))
        {
                lwerror("Dimensions mismatch in lwcurve");
                return;
        }

        ptsize = pointArray_ptsize(curve->points);

        hasSRID = (curve->SRID != -1);

        buf[0] = (uchar)lwgeom_makeType_full(
                TYPE_HASZ(curve->type), TYPE_HASM(curve->type),
                hasSRID, CURVETYPE, curve->bbox ? 1 : 0);
        loc = buf+1;

#ifdef PGIS_DEBUG
        lwnotice("lwcurve_serialize_buf added type (%d)", curve->type);
#endif

        if(curve->bbox)
        {
                memcpy(loc, curve->bbox, sizeof(BOX2DFLOAT4));
                loc += sizeof(BOX2DFLOAT4);

#ifdef PGIS_DEBUG
                lwnotice("lwcurve_serialize_buf added BBOX");
#endif

        }                

        if(hasSRID)
        {
                memcpy(loc, &curve->SRID, sizeof(int32));
                loc += sizeof(int32);

#ifdef PGIS_DEBUG
                lwnotice("lwcurve_serialize_buf added SRID");
#endif

        }

        memcpy(loc, &curve->points->npoints, sizeof(uint32));
        loc += sizeof(uint32);

#ifdef PGIS_DEBUG
        lwnotice("lwcurve_serialize_buf added npoints (%d)",
            curve->points->npoints);
#endif

        /* copy in points */
        size = curve->points->npoints * ptsize;
        memcpy(loc, getPoint_internal(curve->points, 0), size);
        loc += size;

#ifdef PGIS_DEBUG
        lwnotice("lwcurve_serialize_buf copied serialized_pointlist (%d bytes)",
                ptsize * curve->points->npoints);        
#endif

        if(retsize) *retsize = loc-buf;

#ifdef PGIS_DEBUG_CALLS
        lwnotice("lwcurve_serialize_buf returning (loc: %p, size: %d)",
                loc, loc-buf);
#endif
}

/* find length of this deserialized curve */
size_t
lwcurve_serialize_size(LWCURVE *curve)
{
        size_t size = 1; /* type */

#ifdef PGIS_DEBUG_CALLS
        lwnotice("lwcurve_serialize_size called");        
#endif

        if(curve->SRID != -1) size += 4; /* SRID */
        if(curve->bbox) size += sizeof(BOX2DFLOAT4);

        size += 4; /* npoints */
        size += pointArray_ptsize(curve->points) * curve->points->npoints;

#ifdef PGIS_DEBUG_CALLS
        lwnotice("lwcurve_serialize_size returning %d", size);
#endif

        return size;
}

BOX3D *
lwcircle_compute_box3d(POINT4D *p1, POINT4D *p2, POINT4D *p3)
{
        double x1, x2, y1, y2, z1, z2;
        double angle, radius, sweep;
        /*
        double top, left;
        */
        double a1, a2, a3;
        double xe = 0.0, ye = 0.0;
        POINT4D *center;
        int i;
        BOX3D *box;

#ifdef PGIS_DEBUG_CALLS
        lwnotice("lwcircle_compute_box3d called.");
#endif

        center = lwalloc(sizeof(POINT4D));
        radius = lwcircle_center(p1, p2, p3, &center);
        if(radius < 0.0) return NULL;

        /*
        top = center->y + radius;
        left = center->x - radius;

#ifdef PGIS_DEBUG_CALLS
        lwnotice("lwcircle_compute_box3d: top=%.16f, left=%.16f", top, left);
#endif
        */

        x1 = MAXFLOAT;
        x2 = -1 * MAXFLOAT;
        y1 = MAXFLOAT;
        y2 = -1 * MAXFLOAT;

        a1 = atan2(p1->y - center->y, p1->x - center->x);
        a2 = atan2(p2->y - center->y, p2->x - center->x);
        a3 = atan2(p3->y - center->y, p3->x - center->x);

        /* Determine sweep angle */
        if(a1 > a2 && a2 > a3) 
        {
                sweep = a3 - a1;
        }
        /* Counter-clockwise */
        else if(a1 < a2 && a2 < a3)
        {
                sweep = a3 - a1;
        }
        /* Clockwise, wrap */
        else if((a1 < a2 && a1 > a3) || (a2 < a3 && a1 > a3))
        {
                sweep = a3 - a1 + 2*M_PI;
        }
        /* Counter-clockwise, wrap */
        else if((a1 > a2 && a1 < a3) || (a2 > a3 && a1 < a3))
        {
                sweep = a3 - a1 - 2*M_PI;
        } 
        else 
        {
                sweep = 0.0;
        }

#ifdef PGIS_DEBUG
        lwnotice("a1 %.16f, a2 %.16f, a3 %.16f, sweep %.16f", a1, a2, a3, sweep);
#endif

        angle = 0.0;
        for(i=0; i < 6; i++)
        {
                switch(i) {
                case 0:
                        angle = 0.0;
                        xe = center->x + radius;
                        ye = center->y;
                        break;
                case 1:
                        angle = M_PI_2;
                        xe = center->x;
                        ye = center->y + radius;
                        break;
                case 2:
                        angle = M_PI;
                        xe = center->x - radius;
                        ye = center->y;
                        break;
                case 3:
                        angle = -1 * M_PI_2;
                        xe = center->x;
                        ye = center->y - radius;
                        break;
                case 4:
                        angle = a1;
                        xe = p1->x;
                        ye = p1->y;
                        break;
                case 5:
                        angle = a3;
                        xe = p3->x;
                        ye = p3->y;
                        break;
                }
                if(i < 4) 
                {
                        if(sweep > 0.0 && (angle > a3 || angle < a1)) continue;
                        if(sweep < 0.0 && (angle < a3 || angle > a1)) continue;
                }

#ifdef PGIS_DEBUG
                lwnotice("lwcircle_compute_box3d: potential extreame %d (%.16f, %.16f)", i, xe, ye);
#endif
                x1 = (x1 < xe) ? x1 : xe;
                y1 = (y1 < ye) ? y1 : ye;
                x2 = (x2 > xe) ? x2 : xe;
                y2 = (y2 > ye) ? y2 : ye;
        }
#ifdef PGIS_DEBUG
        lwnotice("lwcircle_compute_box3d: extreames found (%.16f %.16f, %.16f %.16f)", x1, y1, x2, y2);
#endif

        /*
        x1 = center->x + x1 * radius;
        x2 = center->x + x2 * radius;
        y1 = center->y + y1 * radius;
        y2 = center->y + y2 * radius;
        */
        z1 = (p1->z < p2->z) ? p1->z : p2->z;
        z1 = (z1 < p3->z) ? z1 : p3->z;
        z2 = (p1->z > p2->z) ? p1->z : p2->z;
        z2 = (z2 > p3->z) ? z2 : p3->z;

        box = lwalloc(sizeof(BOX3D));
        box->xmin = x1; box->xmax = x2;
        box->ymin = y1; box->ymax = y2;
        box->zmin = z1; box->zmax = z2;

        lwfree(center);

        return box;
}

/*
 * Find bounding box (standard one)
 * zmin=zmax=NO_Z_VALUE if 2d
 * TODO: This ignores curvature, which should be taken into account.
 */
BOX3D *
lwcurve_compute_box3d(LWCURVE *curve)
{
        BOX3D *box, *tmp; 
        int i;
        POINT4D *p1 = lwalloc(sizeof(POINT4D));
        POINT4D *p2 = lwalloc(sizeof(POINT4D));
        POINT4D *p3 = lwalloc(sizeof(POINT4D));

#ifdef PGIS_DEBUG_CALLS
        lwnotice("lwcurve_compute_box3d called.");
#endif

        /* initialize box values */
        box = lwalloc(sizeof(BOX3D));
        box->xmin = MAXFLOAT; box->xmax = -1 * MAXFLOAT;
        box->ymin = MAXFLOAT; box->ymax = -1 * MAXFLOAT;
        box->zmin = MAXFLOAT; box->zmax = -1 * MAXFLOAT;
        
        for(i = 2; i < curve->points->npoints; i+=2)
        {
                getPoint4d_p(curve->points, i-2, p1);
                getPoint4d_p(curve->points, i-1, p2);
                getPoint4d_p(curve->points, i, p3);
                tmp = lwcircle_compute_box3d(p1, p2, p3);
                if(tmp == NULL) continue;
                box->xmin = (box->xmin < tmp->xmin) ? box->xmin : tmp->xmin;
                box->xmax = (box->xmax > tmp->xmax) ? box->xmax : tmp->xmax;
                box->ymin = (box->ymin < tmp->ymin) ? box->ymin : tmp->ymin;
                box->ymax = (box->ymax > tmp->ymax) ? box->ymax : tmp->ymax;
                box->zmin = (box->zmin < tmp->zmin) ? box->zmin : tmp->zmin;
                box->zmax = (box->zmax > tmp->zmax) ? box->zmax : tmp->zmax;
#ifdef PGIS_DEBUG_CALLS
                lwnotice("curve %d x=(%.16f,%.16f) y=(%.16f,%.16f) z=(%.16f,%.16f)", i/2, box->xmin, box->xmax, box->ymin, box->ymax, box->zmin, box->zmax);
#endif
        }

        
        return box;
}

int
lwcurve_compute_box2d_p(LWCURVE *curve, BOX2DFLOAT4 *result)
{

#ifdef PGIS_DEBUG_CALLS
        lwnotice("lwcurve_compute_box2d_p called.");
#endif

        BOX3D *box = lwcurve_compute_box3d(curve);
        if(box == NULL) return 0;
        box3d_to_box2df_p(box, result);
        return 1;
}

void pfree_curve(LWCURVE *curve)
{
        lwfree(curve->points);
        lwfree(curve);
}

/* find length of this serialized curve */
size_t
lwgeom_size_curve(const uchar *serialized_curve)
{
        int type = (uchar)serialized_curve[0];
        uint32 result = 1; /* type */
        const uchar *loc;
        uint32 npoints;

#ifdef PGIS_DEBUG_CALLS
        lwnotice("lwgeom_size_curve called");
#endif
        if(lwgeom_getType(type) != CURVETYPE)
                lwerror("lwgeom_size_curve::attempt to find the length of a non-curve");

        loc = serialized_curve + 1;
        if(lwgeom_hasBBOX(type))
        {
                loc += sizeof(BOX2DFLOAT4);
                result += sizeof(BOX2DFLOAT4);
        }

        if(lwgeom_hasSRID(type))
        {
                loc += 4; /* type + SRID */
                result += 4;
        }

        /* we've read the type (1 byte) and SRID (4 bytes, if present) */
        npoints = get_uint32(loc);
        result += sizeof(uint32); /* npoints */

        result += TYPE_NDIMS(type) * sizeof(double) * npoints;

#ifdef PGIS_DEBUG_CALLS
        lwnotice("lwgeom_size_curve returning %d", result);
#endif        

        return result;
}

void printLWCURVE(LWCURVE *curve)
{
        lwnotice("LWCURVE {");
        lwnotice("    ndims = %i", (int)TYPE_NDIMS(curve->type));
        lwnotice("    SRID = %i", (int)curve->SRID);
        printPA(curve->points);
        lwnotice("}");
}

/* Clone LWCURVE object.  POINTARRAY is not copied. */
LWCURVE *
lwcurve_clone(const LWCURVE *g)
{
        LWCURVE *ret = lwalloc(sizeof(LWCURVE));
        memcpy(ret, g, sizeof(LWCURVE));
        if(g->bbox) ret->bbox = box2d_clone(g->bbox);
        return ret;
}

/*
 * Add 'what' to this curve at position 'where'.
 * where=0 == prepend
 * where=-1 == append
 * Returns a MULTICURVE or a GEOMETRYCOLLECTION
 */
LWGEOM *
lwcurve_add(const LWCURVE *to, uint32 where, const LWGEOM *what)
{
        LWCOLLECTION *col;
        LWGEOM **geoms;
        int newtype;
        
        if(where != -1 && where != 0)
        {
                lwerror("lwcurve_add only supports 0 or -1 as second argument %d", where);
                return NULL;
        }

        /* dimensions compatibility are checked by caller */

        /* Construct geoms array */
        geoms = lwalloc(sizeof(LWGEOM *)*2);
        if(where == -1) /* append */
        {
                geoms[0] = lwgeom_clone((LWGEOM *)to);
                geoms[1] = lwgeom_clone(what);
        }
        else /* prepend */
        {
                geoms[0] = lwgeom_clone(what);
                geoms[1] = lwgeom_clone((LWGEOM *)to);
        }
        
        /* reset SRID and wantbbox flag from component types */
        geoms[0]->SRID = geoms[1]->SRID = -1;
        TYPE_SETHASSRID(geoms[0]->type, 0);
        TYPE_SETHASSRID(geoms[1]->type, 0);
        TYPE_SETHASBBOX(geoms[0]->type, 0);
        TYPE_SETHASBBOX(geoms[1]->type, 0);

        /* Find appropriate geom type */
        if(TYPE_GETTYPE(what->type) == CURVETYPE || TYPE_GETTYPE(what->type) == LINETYPE) newtype = MULTICURVETYPE;
        else newtype = COLLECTIONTYPE;

        col = lwcollection_construct(newtype, 
                to->SRID, NULL, 
                2, geoms);

        return (LWGEOM *)col;
}

void lwcurve_reverse(LWCURVE *curve)
{
        ptarray_reverse(curve->points);
}

/*
 * TODO: Invalid segmentization.  This should accomodate the curvature.
 */
LWCURVE *
lwcurve_segmentize2d(LWCURVE *curve, double dist)
{
        return lwcurve_construct(curve->SRID, NULL,
                ptarray_segmentize2d(curve->points, dist));
}
                    
/* check coordinate equality */
char
lwcurve_same(const LWCURVE *me, const LWCURVE *you)
{
        return ptarray_same(me->points, you->points);
}

/*
 * Construct a LWCURVE from an array of LWPOINTs
 * LWCURVE dimensions are large enough to host all input dimensions.
 */
LWCURVE *
lwcurve_from_lwpointarray(int SRID, unsigned int npoints, LWPOINT **points)
{
        int zmflag=0;
        unsigned int i;
        POINTARRAY *pa;
        uchar *newpoints, *ptr;
        size_t ptsize, size;

        /*
         * Find output dimensions, check integrity
         */
        for(i = 0; i < npoints; i++)
        {
                if(TYPE_GETTYPE(points[i]->type) != POINTTYPE)
                {
                        lwerror("lwcurve_from_lwpointarray: invalid input type: %s",
                            lwgeom_typename(TYPE_GETTYPE(points[i]->type)));
                        return NULL;
                }
                if(TYPE_HASZ(points[i]->type)) zmflag |= 2;
                if(TYPE_HASM(points[i]->type)) zmflag |=1;
                if(zmflag == 3) break;
        }

        if(zmflag == 0) ptsize = 2 * sizeof(double);
        else if(zmflag == 3) ptsize = 4 * sizeof(double);
        else ptsize = 3 * sizeof(double);

        /*
         * Allocate output points array
         */
        size = ptsize * npoints;
        newpoints = lwalloc(size);
        memset(newpoints, 0, size);

        ptr = newpoints;
        for(i = 0; i < npoints; i++)
        {
                size = pointArray_ptsize(points[i]->point);
                memcpy(ptr, getPoint_internal(points[i]->point, 0), size);
                ptr += ptsize;
        }
        pa = pointArray_construct(newpoints, zmflag&2, zmflag&1, npoints);

        return lwcurve_construct(SRID, NULL, pa);
}

/*
 * Construct a LWCURVE from a LWMPOINT
 */
LWCURVE *
lwcurve_from_lwmpoint(int SRID, LWMPOINT *mpoint)
{
        unsigned int i;
        POINTARRAY *pa;
        char zmflag = TYPE_GETZM(mpoint->type);
        size_t ptsize, size;
        uchar *newpoints, *ptr;

        if(zmflag == 0) ptsize = 2 * sizeof(double);
        else if(zmflag == 3) ptsize = 4 * sizeof(double);
        else ptsize = 3 * sizeof(double);

        /* Allocate space for output points */
        size = ptsize * mpoint->ngeoms;
        newpoints = lwalloc(size);
        memset(newpoints, 0, size);

        ptr = newpoints;
        for(i = 0; i < mpoint->ngeoms; i++)
        {
                memcpy(ptr,
                        getPoint_internal(mpoint->geoms[i]->point, 0),
                        ptsize);
                ptr += ptsize;
        }

        pa = pointArray_construct(newpoints, zmflag&2, zmflag&1,
                mpoint->ngeoms);

#ifdef PGIS_DEBUG
        lwnotice("lwcurve_from_lwmpoint: constructed pointarray for %d points, %d zmflag", mpoint->ngeoms, zmflag);
#endif
        
        return lwcurve_construct(SRID, NULL, pa);
}

LWCURVE *
lwcurve_addpoint(LWCURVE *curve, LWPOINT *point, unsigned int where)
{
        POINTARRAY *newpa;
        LWCURVE *ret;

        newpa = ptarray_addPoint(curve->points, 
                getPoint_internal(point->point, 0),
                TYPE_NDIMS(point->type), where);
        ret = lwcurve_construct(curve->SRID, NULL, newpa);

        return ret;
}

LWCURVE *
lwcurve_removepoint(LWCURVE *curve, unsigned int index)
{
        POINTARRAY *newpa;
        LWCURVE *ret;

        newpa = ptarray_removePoint(curve->points, index);
        ret = lwcurve_construct(curve->SRID, NULL, newpa);

        return ret;
}

/*
 * Note: input will be changed, make sure you have permissions for this.
 * */
void
lwcurve_setPoint4d(LWCURVE *curve, unsigned int index, POINT4D *newpoint)
{
        setPoint4d(curve->points, index, newpoint);
}












