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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "postgres.h"
#include "liblwgeom.h"
#include "fmgr.h"

#include "lwgeom_pg.h"


double interpolate_arc(double angle, double zm1, double a1, double zm2, double a2);
POINTARRAY *lwcircle_segmentize(POINT4D *p1, POINT4D *p2, POINT4D *p3, uint32 perQuad);
LWLINE *lwcurve_segmentize(LWCURVE *icurve, uint32 perQuad);
LWLINE *lwcompound_segmentize(LWCOMPOUND *icompound, uint32 perQuad);
LWPOLY *lwcurvepoly_segmentize(LWCURVEPOLY *curvepoly, uint32 perQuad);
LWMLINE *lwmcurve_segmentize(LWMCURVE *mcurve, uint32 perQuad);
LWMPOLY *lwmsurface_segmentize(LWMSURFACE *msurface, uint32 perQuad);
LWCOLLECTION *lwcollection_segmentize(LWCOLLECTION *collection, uint32 perQuad);
LWGEOM *append_segment(LWGEOM *geom, POINTARRAY *pts, int type, int SRID);
LWGEOM *pta_desegmentize(POINTARRAY *points, int type, int SRID);
LWGEOM *lwline_desegmentize(LWLINE *line);
LWGEOM *lwpolygon_desegmentize(LWPOLY *poly);
LWGEOM *lwmline_desegmentize(LWMLINE *mline);
LWGEOM *lwmpolygon_desegmentize(LWMPOLY *mpoly);
LWGEOM *lwgeom_desegmentize(LWGEOM *geom);
Datum LWGEOM_has_arc(PG_FUNCTION_ARGS);
Datum LWGEOM_curve_segmentize(PG_FUNCTION_ARGS);
Datum LWGEOM_line_desegmentize(PG_FUNCTION_ARGS);


/*
 * Tolerance used to determine equality.
 */
#define EPSILON_SQLMM 1e-8

/*
 * Determines (recursively in the case of collections) whether the geometry
 * contains at least on arc geometry or segment.
 */
uint32
has_arc(LWGEOM *geom)
{
        LWCOLLECTION *col;
        int i;

        LWDEBUG(2, "has_arc called.");

        switch(lwgeom_getType(geom->type)) 
        {
        case POINTTYPE:
        case LINETYPE:
        case POLYGONTYPE:
        case MULTIPOINTTYPE:
        case MULTILINETYPE:
        case MULTIPOLYGONTYPE:
                return 0;
        case CURVETYPE:
                return 1;
        /* It's a collection that MAY contain an arc */
        default:
                col = (LWCOLLECTION *)geom;
                for(i=0; i<col->ngeoms; i++)
                {
                        if(has_arc(col->geoms[i]) == 1) return 1;
                }
                return 0;
        }
}

/*
 * Determines the center of the circle defined by the three given points.
 * In the event the circle is complete, the midpoint of the segment defined
 * by the first and second points is returned.  If the points are colinear,
 * as determined by equal slopes, then NULL is returned.  If the interior
 * point is coincident with either end point, they are taken as colinear.
 */
double
lwcircle_center(POINT4D *p1, POINT4D *p2, POINT4D *p3, POINT4D **result)
{
        POINT4D *c;
        double cx, cy, cr;
        double temp, bc, cd, det;
        
        LWDEBUGF(2, "lwcircle_center called (%.16f, %.16f), (%.16f, %.16f), (%.16f, %.16f).", p1->x, p1->y, p2->x, p2->y, p3->x, p3->y);

        /* Closed circle */
        if(fabs(p1->x - p3->x) < EPSILON_SQLMM
                        && fabs(p1->y - p3->y) < EPSILON_SQLMM)
        {
                cx = p1->x + (p2->x - p1->x) / 2.0;
                cy = p1->y + (p2->y - p1->y) / 2.0;
                c = lwalloc(sizeof(POINT2D));
                c->x = cx;
                c->y = cy;
                *result = c;
                cr = sqrt((cx-p1->x)*(cx-p1->x)+(cy-p1->y)*(cy-p1->y));
                return cr;
        }

        temp = p2->x*p2->x + p2->y*p2->y;
        bc = (p1->x*p1->x + p1->y*p1->y - temp) / 2.0;
        cd = (temp - p3->x*p3->x - p3->y*p3->y) / 2.0;
        det = (p1->x - p2->x)*(p2->y - p3->y)-(p2->x - p3->x)*(p1->y - p2->y);

        /* Check colinearity */
        if(fabs(det) < EPSILON_SQLMM)
        {
                *result = NULL;
                return -1.0;
        }

        det = 1.0 / det;
        cx = (bc*(p2->y - p3->y)-cd*(p1->y - p2->y))*det;
        cy = ((p1->x - p2->x)*cd-(p2->x - p3->x)*bc)*det;
        c = lwalloc(sizeof(POINT4D));
        c->x = cx;
        c->y = cy;
        *result = c;
        cr = sqrt((cx-p1->x)*(cx-p1->x)+(cy-p1->y)*(cy-p1->y));
        return cr;
}

double
interpolate_arc(double angle, double zm1, double a1, double zm2, double a2)
{
        double frac = fabs((angle - a1) / (a2 - a1));
        double result = frac * (zm2 - zm1) + zm1;

        LWDEBUG(2, "interpolate_arc called.");

        LWDEBUGF(3, "interpolate_arc: angle=%.16f, a1=%.16f, a2=%.16f, z1=%.16f, z2=%.16f, frac=%.16f, result=%.16f", angle, a1, a2, zm1, zm2, frac, result);

        return result;
}

/*******************************************************************************
 * Begin curve segmentize functions
 ******************************************************************************/

POINTARRAY *
lwcircle_segmentize(POINT4D *p1, POINT4D *p2, POINT4D *p3, uint32 perQuad)
{
        POINTARRAY *result;
        POINT4D pbuf;
        size_t ptsize = sizeof(POINT4D);
        unsigned int ptcount;
        uchar *pt;

        POINT4D *center;
        double radius = 0.0, 
               sweep = 0.0,
               angle = 0.0,
               increment = 0.0;
        double a1, a2, a3, i;

        LWDEBUG(2, "lwcircle_segmentize called. ");

        radius = lwcircle_center(p1, p2, p3, &center);

        LWDEBUGF(3, "lwcircle_segmentize, (%.16f, %.16f) radius=%.16f", center->x, center->y, radius);

        if(radius < 0)
        {
                return NULL;
        }

        a1 = atan2(p1->y - center->y, p1->x - center->x);
        a2 = atan2(p2->y - center->y, p2->x - center->x);
        a3 = atan2(p3->y - center->y, p3->x - center->x);

        LWDEBUGF(3, "a1 = %.16f, a2 = %.16f, a3 = %.16f", a1, a2, a3);

        if(fabs(p1->x - p3->x) < EPSILON_SQLMM
                        && fabs(p1->y - p3->y) < EPSILON_SQLMM)
        {
                sweep = 2*M_PI;
        }
        /* Clockwise */
        else if(a1 > a2 && a2 > a3) 
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
         
        ptcount = ceil(fabs(perQuad * sweep / M_PI_2));
        
        result = ptarray_construct(1, 1, ptcount);

        increment = M_PI_2 / perQuad;
        if(sweep < 0) increment *= -1.0;
        angle = a1;

        LWDEBUGF(3, "ptcount: %d, perQuad: %d, sweep: %.16f, increment: %.16f", ptcount, perQuad, sweep, increment);

        for(i = 0; i < ptcount - 1; i++)
        {
            pt = getPoint_internal(result, i);
            angle += increment;
            if(increment > 0.0 && angle > M_PI) angle -= 2*M_PI;
            if(increment < 0.0 && angle < -1*M_PI) angle -= 2*M_PI;
            pbuf.x = center->x + radius*cos(angle);
            pbuf.y = center->y + radius*sin(angle);
            if((sweep > 0 && angle < a2) || (sweep < 0 && angle > a2))
            {
                if((sweep > 0 && a1 < a2) || (sweep < 0 && a1 > a2))
                {
                        pbuf.z = interpolate_arc(angle, p1->z, a1, p2->z, a2);
                        pbuf.m = interpolate_arc(angle, p1->m, a1, p2->m, a2);
                }
                else
                {
                    if(sweep > 0)
                    {
                        pbuf.z = interpolate_arc(angle, p1->z, a1-(2*M_PI), p2->z, a2);
                        pbuf.m = interpolate_arc(angle, p1->m, a1-(2*M_PI), p2->m, a2);
                    }
                    else
                    {
                        pbuf.z = interpolate_arc(angle, p1->z, a1+(2*M_PI), p2->z, a2);
                        pbuf.m = interpolate_arc(angle, p1->m, a1+(2*M_PI), p2->m, a2);
                    }
                }
            }
            else
            {
                if((sweep > 0 && a2 < a3) || (sweep < 0 && a2 > a3))
                {
                    pbuf.z = interpolate_arc(angle, p2->z, a2, p3->z, a3);
                    pbuf.m = interpolate_arc(angle, p2->m, a2, p3->m, a3);
                }
                else
                {
                    if(sweep > 0)
                    {
                        pbuf.z = interpolate_arc(angle, p2->z, a2-(2*M_PI), p3->z, a3);
                        pbuf.m = interpolate_arc(angle, p2->m, a2-(2*M_PI), p3->m, a3);
                    }
                    else
                    {
                        pbuf.z = interpolate_arc(angle, p2->z, a2+(2*M_PI), p3->z, a3);
                        pbuf.m = interpolate_arc(angle, p2->m, a2+(2*M_PI), p3->m, a3);
                    }
                }
            }
            memcpy(pt, (uchar *)&pbuf, ptsize);
        }

        pt = getPoint_internal(result, ptcount - 1);
        memcpy(pt, (uchar *)p3, ptsize);

        lwfree(center);

        return result;
}

LWLINE *
lwcurve_segmentize(LWCURVE *icurve, uint32 perQuad)
{
        LWLINE *oline;
        DYNPTARRAY *ptarray;
        POINTARRAY *tmp;
        uint32 i, j;
        POINT4D *p1 = lwalloc(sizeof(POINT4D));
        POINT4D *p2 = lwalloc(sizeof(POINT4D));
        POINT4D *p3 = lwalloc(sizeof(POINT4D));
        POINT4D *p4 = lwalloc(sizeof(POINT4D));


        LWDEBUGF(2, "lwcurve_segmentize called., dim = %d", icurve->points->dims);

        ptarray = dynptarray_create(icurve->points->npoints, icurve->points->dims);
        if(!getPoint4d_p(icurve->points, 0, p4))
        {
                elog(ERROR, "curve_segmentize: Cannot extract point.");
        }
        dynptarray_addPoint4d(ptarray, p4, 1);

        for(i = 2; i < icurve->points->npoints; i+=2) 
        {
                LWDEBUGF(3, "lwcurve_segmentize: arc ending at point %d", i);

                getPoint4d_p(icurve->points, i - 2, p1);
                getPoint4d_p(icurve->points, i - 1, p2);
                getPoint4d_p(icurve->points, i, p3);
                tmp = lwcircle_segmentize(p1, p2, p3, perQuad);

                LWDEBUGF(3, "lwcurve_segmentize: generated %d points", tmp->npoints);

                for(j = 0; j < tmp->npoints; j++)
                {
                        getPoint4d_p(tmp, j, p4);
                        dynptarray_addPoint4d(ptarray, p4, 1);
                }
                lwfree(tmp);
        }
        oline = lwline_construct(icurve->SRID, NULL, ptarray_clone(ptarray->pa));

        lwfree(p1);
        lwfree(p2);
        lwfree(p3);
        lwfree(p4);
        lwfree(ptarray);
        return oline;
}

LWLINE *
lwcompound_segmentize(LWCOMPOUND *icompound, uint32 perQuad)
{
        LWLINE *oline;
        LWGEOM *geom;
        DYNPTARRAY *ptarray = NULL;
        LWLINE *tmp = NULL;
        uint32 i, j;
        POINT4D *p = NULL;

        LWDEBUG(2, "lwcompound_segmentize called.");

        p = lwalloc(sizeof(POINT4D));

        ptarray = dynptarray_create(2, ((POINTARRAY *)icompound->geoms[0]->data)->dims);

        for(i = 0; i < icompound->ngeoms; i++)
        {
                geom = icompound->geoms[i];
                if(lwgeom_getType(geom->type) == CURVETYPE)
                {
                        tmp = lwcurve_segmentize((LWCURVE *)geom, perQuad);
                        for(j = 0; j < tmp->points->npoints; j++)
                        {
                                getPoint4d_p(tmp->points, j, p);
                                dynptarray_addPoint4d(ptarray, p, 0);
                        }
                        lwfree(tmp);
                }
                else if(lwgeom_getType(geom->type) == LINETYPE)
                {
                        tmp = (LWLINE *)geom;
                        for(j = 0; j < tmp->points->npoints; j++)
                        {
                                getPoint4d_p(tmp->points, j, p);
                                dynptarray_addPoint4d(ptarray, p, 0);
                        }
                }
                else
                {
                        lwerror("Unsupported geometry type %d found.", lwgeom_getType(geom->type));
                        return NULL;
                }
        }
        oline = lwline_construct(icompound->SRID, NULL, ptarray_clone(ptarray->pa));
        lwfree(ptarray);
        lwfree(p);
        return oline;
}

LWPOLY *
lwcurvepoly_segmentize(LWCURVEPOLY *curvepoly, uint32 perQuad)
{
        LWPOLY *ogeom;
        LWGEOM *tmp;
        LWLINE *line;
        POINTARRAY **ptarray;
        int i;

        LWDEBUG(2, "lwcurvepoly_segmentize called.");

        ptarray = lwalloc(sizeof(POINTARRAY *)*curvepoly->nrings);

        for(i = 0; i < curvepoly->nrings; i++)
        {
                tmp = curvepoly->rings[i];
                if(lwgeom_getType(tmp->type) == CURVETYPE)
                {
                        line = lwcurve_segmentize((LWCURVE *)tmp, perQuad);
                        ptarray[i] = ptarray_clone(line->points);
                        lwfree(line);
                }
                else if(lwgeom_getType(tmp->type) == LINETYPE)
                {
                        line = (LWLINE *)tmp;
                        ptarray[i] = ptarray_clone(line->points);
                }
                else
                {
                        lwerror("Invalid ring type found in CurvePoly.");
                        return NULL;
                }
        }

        ogeom = lwpoly_construct(curvepoly->SRID, NULL, curvepoly->nrings, ptarray);
        return ogeom;
}

LWMLINE *
lwmcurve_segmentize(LWMCURVE *mcurve, uint32 perQuad)
{
        LWMLINE *ogeom;
        LWGEOM *tmp;
        LWGEOM **lines;
        int i;
        
        LWDEBUGF(2, "lwmcurve_segmentize called, geoms=%d, dim=%d.", mcurve->ngeoms, TYPE_NDIMS(mcurve->type));

        lines = lwalloc(sizeof(LWGEOM *)*mcurve->ngeoms);

        for(i = 0; i < mcurve->ngeoms; i++)
        {
                tmp = mcurve->geoms[i];
                if(lwgeom_getType(tmp->type) == CURVETYPE)
                {
                        lines[i] = (LWGEOM *)lwcurve_segmentize((LWCURVE *)tmp, perQuad);
                }
                else if(lwgeom_getType(tmp->type) == LINETYPE)
                {
                        lines[i] = (LWGEOM *)lwline_construct(mcurve->SRID, NULL, ptarray_clone(((LWLINE *)tmp)->points));
                }
                else
                {
                        lwerror("Unsupported geometry found in MultiCurve.");
                        return NULL;
                }
        }

        ogeom = (LWMLINE *)lwcollection_construct(MULTILINETYPE, mcurve->SRID, NULL, mcurve->ngeoms, lines);
        return ogeom;
}

LWMPOLY *
lwmsurface_segmentize(LWMSURFACE *msurface, uint32 perQuad)
{
        LWMPOLY *ogeom;
        LWGEOM *tmp;
        LWPOLY *poly;
        LWGEOM **polys;
        POINTARRAY **ptarray;
        int i, j;

        LWDEBUG(2, "lwmsurface_segmentize called.");

        polys = lwalloc(sizeof(LWGEOM *)*msurface->ngeoms);

        for(i = 0; i < msurface->ngeoms; i++)
        {
                tmp = msurface->geoms[i];
                if(lwgeom_getType(tmp->type) == CURVEPOLYTYPE)
                {
                        polys[i] = (LWGEOM *)lwcurvepoly_segmentize((LWCURVEPOLY *)tmp, perQuad);
                }
                else if(lwgeom_getType(tmp->type) == POLYGONTYPE)
                {
                        poly = (LWPOLY *)tmp;
                        ptarray = lwalloc(sizeof(POINTARRAY *)*poly->nrings);
                        for(j = 0; j < poly->nrings; j++)
                        {
                                ptarray[j] = ptarray_clone(poly->rings[j]);
                        }
                        polys[i] = (LWGEOM *)lwpoly_construct(msurface->SRID, NULL, poly->nrings, ptarray);
                }
        }
        ogeom = (LWMPOLY *)lwcollection_construct(MULTIPOLYGONTYPE, msurface->SRID, NULL, msurface->ngeoms, polys);
        return ogeom;
}

LWCOLLECTION *
lwcollection_segmentize(LWCOLLECTION *collection, uint32 perQuad)
{
        LWCOLLECTION *ocol;
        LWGEOM *tmp;
        LWGEOM **geoms;
        int i;

        LWDEBUG(2, "lwcollection_segmentize called.");

        if(has_arc((LWGEOM *)collection) == 0)
        {
                return collection;
        }

        geoms = lwalloc(sizeof(LWGEOM *)*collection->ngeoms);

        for(i=0; i<collection->ngeoms; i++)
        {
                tmp = collection->geoms[i];
                switch(lwgeom_getType(tmp->type)) {
                case CURVETYPE:
                        geoms[i] = (LWGEOM *)lwcurve_segmentize((LWCURVE *)tmp, perQuad);
                        break;
                case COMPOUNDTYPE:
                        geoms[i] = (LWGEOM *)lwcompound_segmentize((LWCOMPOUND *)tmp, perQuad);
                        break;
                case CURVEPOLYTYPE:
                        geoms[i] = (LWGEOM *)lwcurvepoly_segmentize((LWCURVEPOLY *)tmp, perQuad);
                        break;
                default:
                        geoms[i] = lwgeom_clone(tmp);
                        break;
                }
        }
        ocol = lwcollection_construct(COLLECTIONTYPE, collection->SRID, NULL, collection->ngeoms, geoms);
        return ocol;
}

LWGEOM *
lwgeom_segmentize(LWGEOM *geom, uint32 perQuad)
{
        LWGEOM * ogeom = NULL;
        switch(lwgeom_getType(geom->type)) {
        case CURVETYPE:
                ogeom = (LWGEOM *)lwcurve_segmentize((LWCURVE *)geom, perQuad);
                break;
        case COMPOUNDTYPE:
                ogeom = (LWGEOM *)lwcompound_segmentize((LWCOMPOUND *)geom, perQuad);
                break;
        case CURVEPOLYTYPE:
                ogeom = (LWGEOM *)lwcurvepoly_segmentize((LWCURVEPOLY *)geom, perQuad);
                break;
        case MULTICURVETYPE:
                ogeom = (LWGEOM *)lwmcurve_segmentize((LWMCURVE *)geom, perQuad);
                break;
        case MULTISURFACETYPE:
                ogeom = (LWGEOM *)lwmsurface_segmentize((LWMSURFACE *)geom, perQuad);
                break;
        case COLLECTIONTYPE:
                ogeom = (LWGEOM *)lwcollection_segmentize((LWCOLLECTION *)geom, perQuad);
                break;
        default:
                ogeom = lwgeom_clone(geom);
        }
        return ogeom;
}

/*******************************************************************************
 * End curve segmentize functions
 ******************************************************************************/
LWGEOM *
append_segment(LWGEOM *geom, POINTARRAY *pts, int type, int SRID)
{
        LWGEOM *result; 
        int currentType, i;

        LWDEBUGF(2, "append_segment called %p, %p, %d, %d", geom, pts, type, SRID);

        if(geom == NULL)
        {
                if(type == LINETYPE)
                {
                        LWDEBUG(3, "append_segment: line to NULL");

                        return (LWGEOM *)lwline_construct(SRID, NULL, pts);
                }
                else if(type == CURVETYPE)
                {
#if POSTGIS_DEBUG_LEVEL >= 4
                        POINT4D tmp;
                        
			LWDEBUGF(4, "append_segment: curve to NULL %d", pts->npoints);
 
                        for(i=0; i<pts->npoints; i++)
                        {
                                getPoint4d_p(pts, i, &tmp);
                                LWDEBUGF(4, "new point: (%.16f,%.16f)",tmp.x,tmp.y);
                        }
#endif
                        return (LWGEOM *)lwcurve_construct(SRID, NULL, pts);
                }
                else
                {
                        lwerror("Invalid segment type %d.", type);
                }
        }
        
        currentType = lwgeom_getType(geom->type);
        if(currentType == LINETYPE && type == LINETYPE)
        {
                POINTARRAY *newPoints;
                POINT4D pt;
                LWLINE *line = (LWLINE *)geom;

                LWDEBUG(3, "append_segment: line to line");

                newPoints = ptarray_construct(TYPE_HASZ(pts->dims), TYPE_HASM(pts->dims), pts->npoints + line->points->npoints - 1);
                for(i=0; i<line->points->npoints; i++)
                {
                        getPoint4d_p(pts, i, &pt);
                        setPoint4d(newPoints, i, &pt);
                }
                for(i=1; i<pts->npoints; i++)
                {
                        getPoint4d_p(line->points, i, &pt);
                        setPoint4d(newPoints, i + pts->npoints, &pt);
                }
                result = (LWGEOM *)lwline_construct(SRID, NULL, newPoints);
                lwgeom_release(geom);
                return result;
        }
        else if(currentType == CURVETYPE && type == CURVETYPE)
        {
                POINTARRAY *newPoints;
                POINT4D pt;
                LWCURVE *curve = (LWCURVE *)geom;

                LWDEBUG(3, "append_segment: curve to curve");

                newPoints = ptarray_construct(TYPE_HASZ(pts->dims), TYPE_HASM(pts->dims), pts->npoints + curve->points->npoints - 1);

                LWDEBUGF(3, "New array length: %d", pts->npoints + curve->points->npoints - 1);

                for(i=0; i<curve->points->npoints; i++)
                {
                        getPoint4d_p(curve->points, i, &pt);

                        LWDEBUGF(3, "orig point %d: (%.16f,%.16f)", i, pt.x, pt.y);

                        setPoint4d(newPoints, i, &pt);
                }
                for(i=1; i<pts->npoints;i++)
                {
                        getPoint4d_p(pts, i, &pt);

                        LWDEBUGF(3, "new point %d: (%.16f,%.16f)", i + curve->points->npoints - 1, pt.x, pt.y);

                        setPoint4d(newPoints, i + curve->points->npoints - 1, &pt);
                }
                result = (LWGEOM *)lwcurve_construct(SRID, NULL, newPoints);
                lwgeom_release(geom);
                return result;
        }
        else if(currentType == CURVETYPE && type == LINETYPE)
        {
                LWLINE *line;
                LWGEOM **geomArray;

                LWDEBUG(3, "append_segment: line to curve");

                geomArray = lwalloc(sizeof(LWGEOM *)*2);
                geomArray[0] = lwgeom_clone(geom);
                
                line = lwline_construct(SRID, NULL, pts);
                geomArray[1] = lwgeom_clone((LWGEOM *)line);

                result = (LWGEOM *)lwcollection_construct(COMPOUNDTYPE, SRID, NULL, 2, geomArray);
                lwfree((LWGEOM *)line);
                lwgeom_release(geom);
                return result;
        }
        else if(currentType == LINETYPE && type == CURVETYPE)
        {
                LWCURVE *curve;
                LWGEOM **geomArray;

                LWDEBUG(3, "append_segment: curve to line");

                geomArray = lwalloc(sizeof(LWGEOM *)*2);
                geomArray[0] = lwgeom_clone(geom);

                curve = lwcurve_construct(SRID, NULL, pts);
                geomArray[1] = lwgeom_clone((LWGEOM *)curve);

                result = (LWGEOM *)lwcollection_construct(COMPOUNDTYPE, SRID, NULL, 2, geomArray);
                lwfree((LWGEOM *)curve);
                lwgeom_release(geom);
                return result;
        }
        else if(currentType == COMPOUNDTYPE)
        {
                LWGEOM *newGeom;
                LWCOMPOUND *compound;
                int count;
                LWGEOM **geomArray;
                
                compound = (LWCOMPOUND *)geom;
                count = compound->ngeoms + 1;
                geomArray = lwalloc(sizeof(LWGEOM *)*count);
                for(i=0; i<compound->ngeoms; i++)
                {
                        geomArray[i] = lwgeom_clone(compound->geoms[i]);
                }
                if(type == LINETYPE)
                {
                        LWDEBUG(3, "append_segment: line to compound");

                        newGeom = (LWGEOM *)lwline_construct(SRID, NULL, pts);
                }
                else if(type == CURVETYPE)
                {
                        LWDEBUG(3, "append_segment: curve to compound");

                        newGeom = (LWGEOM *)lwcurve_construct(SRID, NULL, pts);
                }
                else
                {
                        lwerror("Invalid segment type %d.", type);
                        return NULL;
                }
                geomArray[compound->ngeoms] = lwgeom_clone(newGeom);

                result = (LWGEOM *)lwcollection_construct(COMPOUNDTYPE, SRID, NULL, count, geomArray);
                lwfree(newGeom);
                lwgeom_release(geom);
                return result;
        }
        lwerror("Invalid state %d-%d", currentType, type);
        return NULL;
}

LWGEOM *
pta_desegmentize(POINTARRAY *points, int type, int SRID)
{
        int i, j, commit, isline, count;
        double last_angle, last_length;
        double dxab, dyab, dxbc, dybc, theta, length;
        POINT4D a, b, c, tmp;
        POINTARRAY *pts;
        LWGEOM *geom = NULL;

        LWDEBUG(2, "pta_desegmentize called.");

        getPoint4d_p(points, 0, &a);
        getPoint4d_p(points, 1, &b);
        getPoint4d_p(points, 2, &c);

        dxab = b.x - a.x;
        dyab = b.y - a.y;
        dxbc = c.x - b.x;
        dybc = c.y - b.y;

        theta = atan2(dyab, dxab);
        last_angle = theta - atan2(dybc, dxbc);
        last_length = sqrt(dxbc*dxbc+dybc*dybc);
        length = sqrt(dxab*dxab+dyab*dyab);
        if((last_length - length) < EPSILON_SQLMM) 
        {
                isline = -1;
                LWDEBUG(3, "Starting as unknown.");
        }
        else 
        {
                isline = 1;        
                LWDEBUG(3, "Starting as line.");
        }

        commit = 0;
        for(i=3; i<points->npoints; i++)
        {
                getPoint4d_p(points, i-2, &a);
                getPoint4d_p(points, i-1, &b);
                getPoint4d_p(points, i, &c);
        
                dxab = b.x - a.x;
                dyab = b.y - a.y;
                dxbc = c.x - b.x;
                dybc = c.y - b.y;

                LWDEBUGF(3, "(dxab, dyab, dxbc, dybc) (%.16f, %.16f, %.16f, %.16f)", dxab, dyab, dxbc, dybc);

                theta = atan2(dyab, dxab);
                theta = theta - atan2(dybc, dxbc);
                length = sqrt(dxbc*dxbc+dybc*dybc);

                LWDEBUGF(3, "Last/current length and angle %.16f/%.16f, %.16f/%.16f", last_angle, theta, last_length, length);

                /* Found a line segment */
                if(fabs(length - last_length) > EPSILON_SQLMM || 
                        fabs(theta - last_angle) > EPSILON_SQLMM)
                {
                        last_length = length;
                        last_angle = theta;
                        /* We are tracking a line, keep going */
                        if(isline > 0)
                        {
                        }
                        /* We were tracking a curve, commit it and start line*/
                        else if(isline == 0)
                        {
                                LWDEBUGF(3, "Building curve, %d - %d", commit, i);

                                count = i - commit;
                                pts = ptarray_construct(
                                        TYPE_HASZ(type),
                                        TYPE_HASM(type),
                                        3);
                                getPoint4d_p(points, commit, &tmp);
                                setPoint4d(pts, 0, &tmp);
                                getPoint4d_p(points, 
                                        commit + count/2, &tmp);
                                setPoint4d(pts, 1, &tmp);
                                getPoint4d_p(points, i - 1, &tmp);
                                setPoint4d(pts, 2, &tmp);

                                commit = i-1;
                                geom = append_segment(geom, pts, CURVETYPE, SRID);
                                isline = -1;

                                /* 
                                 * We now need to move ahead one point to 
                                 * determine if it's a potential new curve, 
                                 * since the last_angle value is corrupt.
                                 */
        i++;
        getPoint4d_p(points, i-2, &a);
        getPoint4d_p(points, i-1, &b);
        getPoint4d_p(points, i, &c);

        dxab = b.x - a.x;
        dyab = b.y - a.y;
        dxbc = c.x - b.x;
        dybc = c.y - b.y;

        theta = atan2(dyab, dxab);
        last_angle = theta - atan2(dybc, dxbc);
        last_length = sqrt(dxbc*dxbc+dybc*dybc);
        length = sqrt(dxab*dxab+dyab*dyab);
        if((last_length - length) < EPSILON_SQLMM) 
        {
                isline = -1;
                LWDEBUG(3, "Restarting as unknown.");
        }
        else 
        {
                isline = 1;        
                LWDEBUG(3, "Restarting as line.");
        }


                        }
                        /* We didn't know what we were tracking, now we do. */
                        else
                        {
                                LWDEBUG(3, "It's a line");
                                isline = 1;
                        }
                }
                /* Found a curve segment */
                else
                {
                        /* We were tracking a curve, commit it and start line */
                        if(isline > 0)
                        {
                                LWDEBUGF(3, "Building line, %d - %d", commit, i-2);

                                count = i - commit - 2;

                                pts = ptarray_construct(
                                        TYPE_HASZ(type),
                                        TYPE_HASM(type),
                                        count);
                                for(j=commit;j<i-2;j++)
                                {
                                        getPoint4d_p(points, j, &tmp);
                                        setPoint4d(pts, j-commit, &tmp);
                                }

                                commit = i-3;
                                geom = append_segment(geom, pts, LINETYPE, SRID);
                                isline = -1;
                        }
                        /* We are tracking a curve, keep going */
                        else if(isline == 0)
                        {
                                ;
                        }
                        /* We didn't know what we were tracking, now we do */
                        else
                        {
                                LWDEBUG(3, "It's a curve");
                                isline = 0;
                        }
                }
        }
        count = i - commit;
        if(isline == 0 && count > 2)
        {
                LWDEBUGF(3, "Finishing curve %d,%d.", commit, i);

                pts = ptarray_construct(
                        TYPE_HASZ(type),
                        TYPE_HASM(type),
                        3);
                getPoint4d_p(points, commit, &tmp);
                setPoint4d(pts, 0, &tmp);
                getPoint4d_p(points, commit + count/2, &tmp);
                setPoint4d(pts, 1, &tmp);
                getPoint4d_p(points, i - 1, &tmp);
                setPoint4d(pts, 2, &tmp);

                geom = append_segment(geom, pts, CURVETYPE, SRID);
        }
        else 
        {
                LWDEBUGF(3, "Finishing line %d,%d.", commit, i);

                pts = ptarray_construct(
                        TYPE_HASZ(type),
                        TYPE_HASM(type),
                        count);
                for(j=commit;j<i;j++)
                {
                        getPoint4d_p(points, j, &tmp);
                        setPoint4d(pts, j-commit, &tmp);
                }
                geom = append_segment(geom, pts, LINETYPE, SRID);
        }
        return geom;
}

LWGEOM *
lwline_desegmentize(LWLINE *line)
{
        LWDEBUG(2, "lwline_desegmentize called.");

        return pta_desegmentize(line->points, line->type, line->SRID);
}

LWGEOM *
lwpolygon_desegmentize(LWPOLY *poly)
{
        LWGEOM **geoms;
        int i, hascurve = 0;
        
        LWDEBUG(2, "lwpolygon_desegmentize called.");

        geoms = lwalloc(sizeof(LWGEOM *)*poly->nrings);
        for(i=0; i<poly->nrings; i++)
        {
                geoms[i] = pta_desegmentize(poly->rings[i], poly->type, poly->SRID);
                if(lwgeom_getType(geoms[i]->type) == CURVETYPE ||
                        lwgeom_getType(geoms[i]->type) == COMPOUNDTYPE)
                {
                        hascurve = 1;
                }
        }
        if(hascurve == 0)
        {
                for(i=0; i<poly->nrings; i++)
                {
                        lwfree(geoms[i]);
                }
                return lwgeom_clone((LWGEOM *)poly);
        }

        return (LWGEOM *)lwcollection_construct(CURVEPOLYTYPE, poly->SRID, NULL, poly->nrings, geoms);
}

LWGEOM *
lwmline_desegmentize(LWMLINE *mline)
{
        LWGEOM **geoms;
        int i, hascurve = 0;

        LWDEBUG(2, "lwmline_desegmentize called.");

        geoms = lwalloc(sizeof(LWGEOM *)*mline->ngeoms);
        for(i=0; i<mline->ngeoms; i++)
        {
                geoms[i] = lwline_desegmentize((LWLINE *)mline->geoms[i]);
                if(lwgeom_getType(geoms[i]->type) == CURVETYPE ||
                        lwgeom_getType(geoms[i]->type) == COMPOUNDTYPE)
                {
                        hascurve = 1;
                }
        }
        if(hascurve == 0)
        {
                for(i=0; i<mline->ngeoms; i++)
                {
                        lwfree(geoms[i]);
                }
                return lwgeom_clone((LWGEOM *)mline);
        }
        return (LWGEOM *)lwcollection_construct(MULTICURVETYPE, mline->SRID, NULL, mline->ngeoms, geoms);
}

LWGEOM *
lwmpolygon_desegmentize(LWMPOLY *mpoly)
{
        LWGEOM **geoms;
        int i, hascurve = 0;

        LWDEBUG(2, "lwmpoly_desegmentize called.");

        geoms = lwalloc(sizeof(LWGEOM *)*mpoly->ngeoms);
        for(i=0; i<mpoly->ngeoms; i++)
        {
                geoms[i] = lwpolygon_desegmentize((LWPOLY *)mpoly->geoms[i]);
                if(lwgeom_getType(geoms[i]->type) == CURVEPOLYTYPE)
                {
                        hascurve = 1;
                }
        }
        if(hascurve == 0)
        {
                for(i=0; i<mpoly->ngeoms; i++)
                {
                        lwfree(geoms[i]);
                }
                return lwgeom_clone((LWGEOM *)mpoly);
        }
        return (LWGEOM *)lwcollection_construct(MULTISURFACETYPE, mpoly->SRID, NULL, mpoly->ngeoms, geoms);
}

LWGEOM *
lwgeom_desegmentize(LWGEOM *geom)
{
        int type = lwgeom_getType(geom->type);

        LWDEBUG(2, "lwgeom_desegmentize called.");

        switch(type) {
        case LINETYPE:
                return lwline_desegmentize((LWLINE *)geom);
        case POLYGONTYPE:
                return lwpolygon_desegmentize((LWPOLY *)geom);
        case MULTILINETYPE:
                return lwmline_desegmentize((LWMLINE *)geom);
        case MULTIPOLYGONTYPE:
                return lwmpolygon_desegmentize((LWMPOLY *)geom);
        default:
                return lwgeom_clone(geom);
        }
}

/*******************************************************************************
 * Begin PG_FUNCTIONs
 ******************************************************************************/

PG_FUNCTION_INFO_V1(LWGEOM_has_arc);
Datum LWGEOM_has_arc(PG_FUNCTION_ARGS)
{
        PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
        uint32 result = has_arc(lwgeom_deserialize(SERIALIZED_FORM(geom)));
        PG_RETURN_BOOL(result == 1);
}

/*
 * Converts any curve segments of the geometry into a linear approximation.
 * Curve centers are determined by projecting the defining points into the 2d
 * plane.  Z and M values are assigned by linear interpolation between 
 * defining points.
 */
PG_FUNCTION_INFO_V1(LWGEOM_curve_segmentize);
Datum LWGEOM_curve_segmentize(PG_FUNCTION_ARGS)
{
        PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
        uint32 perQuad = PG_GETARG_INT32(1);
        PG_LWGEOM *ret;
        LWGEOM *igeom = NULL, *ogeom = NULL;

        POSTGIS_DEBUG(2, "LWGEOM_curve_segmentize called.");

        if(perQuad < 0) 
        {
                elog(ERROR, "2nd argument must be positive.");
                PG_RETURN_NULL();
        }
#if POSTGIS_DEBUG_LEVEL > 0
        else
        {
                POSTGIS_DEBUGF(3, "perQuad = %d", perQuad);
        }
#endif
        igeom = lwgeom_deserialize(SERIALIZED_FORM(geom));
        ogeom = lwgeom_segmentize(igeom, perQuad);
        if(ogeom == NULL) PG_RETURN_NULL();
        ret = pglwgeom_serialize(ogeom);
        lwgeom_release(igeom);
        lwgeom_release(ogeom);
        PG_FREE_IF_COPY(geom, 0);
        PG_RETURN_POINTER(ret);
}

PG_FUNCTION_INFO_V1(LWGEOM_line_desegmentize);
Datum LWGEOM_line_desegmentize(PG_FUNCTION_ARGS)
{
        PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
        PG_LWGEOM *ret;
        LWGEOM *igeom = NULL, *ogeom = NULL;

        POSTGIS_DEBUG(2, "LWGEOM_line_desegmentize.");

        igeom = lwgeom_deserialize(SERIALIZED_FORM(geom));
        ogeom = lwgeom_desegmentize(igeom);
        if(ogeom == NULL)
        {
                lwgeom_release(igeom);
                PG_RETURN_NULL();
        }
        ret = pglwgeom_serialize(ogeom);
        lwgeom_release(igeom);
        lwgeom_release(ogeom);
        PG_FREE_IF_COPY(geom, 0);
        PG_RETURN_POINTER(ret);
}

/*******************************************************************************
 * End PG_FUNCTIONs
 ******************************************************************************/
