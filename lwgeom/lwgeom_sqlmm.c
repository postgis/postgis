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
#include "wktparse.h"
#include "lwgeom_pg.h"

/*
 * Tolerance used to determine equality.
 */
#define EPSILON_SQLMM 1e-8

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
        
#ifdef PGIS_DEBUG_CALLS
        lwnotice("lwcircle_center called (%.16f, %.16f), (%.16f, %.16f), (%.16f, %.16f).", p1->x, p1->y, p2->x, p2->y, p3->x, p3->y);
#endif

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

/*******************************************************************************
 * End PG_FUNCTIONs
 ******************************************************************************/
