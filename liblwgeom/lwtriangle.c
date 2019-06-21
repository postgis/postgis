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
 * Copyright (C) 2010 - Oslandia
 *
 **********************************************************************/


/* basic LWTRIANGLE manipulation */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "liblwgeom_internal.h"
#include "lwgeom_log.h"



/* construct a new LWTRIANGLE.
 * use SRID=SRID_UNKNOWN for unknown SRID (will have 8bit type's S = 0)
 */
LWTRIANGLE *
lwtriangle_construct(int32_t srid, GBOX *bbox, POINTARRAY *points)
{
	LWTRIANGLE *result;

	result = (LWTRIANGLE*) lwalloc(sizeof(LWTRIANGLE));
	result->type = TRIANGLETYPE;

	result->flags = points->flags;
	FLAGS_SET_BBOX(result->flags, bbox?1:0);

	result->srid = srid;
	result->points = points;
	result->bbox = bbox;

	return result;
}

LWTRIANGLE *
lwtriangle_construct_empty(int32_t srid, char hasz, char hasm)
{
	LWTRIANGLE *result = lwalloc(sizeof(LWTRIANGLE));
	result->type = TRIANGLETYPE;
	result->flags = lwflags(hasz,hasm,0);
	result->srid = srid;
	result->points = ptarray_construct_empty(hasz, hasm, 1);
	result->bbox = NULL;
	return result;
}

void lwtriangle_free(LWTRIANGLE  *triangle)
{
	if ( ! triangle ) return;

	if (triangle->bbox)
		lwfree(triangle->bbox);

	if (triangle->points)
		ptarray_free(triangle->points);

	lwfree(triangle);
}

void printLWTRIANGLE(LWTRIANGLE *triangle)
{
	if (triangle->type != TRIANGLETYPE)
                lwerror("printLWTRIANGLE called with something else than a Triangle");

	lwnotice("LWTRIANGLE {");
	lwnotice("    ndims = %i", (int)FLAGS_NDIMS(triangle->flags));
	lwnotice("    SRID = %i", (int)triangle->srid);
	printPA(triangle->points);
	lwnotice("}");
}

/* @brief Clone LWTRIANGLE object. Serialized point lists are not copied.
 *
 * @see ptarray_clone
 */
LWTRIANGLE *
lwtriangle_clone(const LWTRIANGLE *g)
{
	LWDEBUGF(2, "lwtriangle_clone called with %p", g);
	return (LWTRIANGLE *)lwline_clone((const LWLINE *)g);
}

void
lwtriangle_force_clockwise(LWTRIANGLE *triangle)
{
	if ( ptarray_isccw(triangle->points) )
		ptarray_reverse_in_place(triangle->points);
}

int
lwtriangle_is_clockwise(LWTRIANGLE *triangle)
{
	return !ptarray_isccw(triangle->points);
}

void
lwtriangle_release(LWTRIANGLE *lwtriangle)
{
	lwgeom_release(lwtriangle_as_lwgeom(lwtriangle));
}

/* check coordinate equality  */
char
lwtriangle_same(const LWTRIANGLE *t1, const LWTRIANGLE *t2)
{
	char r = ptarray_same(t1->points, t2->points);
	LWDEBUGF(5, "returning %d", r);
	return r;
}

static char
lwtriangle_is_repeated_points(LWTRIANGLE *triangle)
{
	char ret;
	POINTARRAY *pa;

	pa = ptarray_remove_repeated_points(triangle->points, 0.0);
	ret = ptarray_same(pa, triangle->points);
	ptarray_free(pa);

	return ret;
}

/*
 * Construct a triangle from a LWLINE being
 * the shell
 * Pointarray from input geom is cloned.
 * Input line must have 4 points, and be closed.
 */
LWTRIANGLE *
lwtriangle_from_lwline(const LWLINE *shell)
{
	LWTRIANGLE *ret;
	POINTARRAY *pa;

	if ( shell->points->npoints != 4 )
		lwerror("lwtriangle_from_lwline: shell must have exactly 4 points");

	if (   (!FLAGS_GET_Z(shell->flags) && !ptarray_is_closed_2d(shell->points)) ||
	        (FLAGS_GET_Z(shell->flags) && !ptarray_is_closed_3d(shell->points)) )
		lwerror("lwtriangle_from_lwline: shell must be closed");

	pa = ptarray_clone_deep(shell->points);
	ret = lwtriangle_construct(shell->srid, NULL, pa);

	if (lwtriangle_is_repeated_points(ret))
		lwerror("lwtriangle_from_lwline: some points are repeated in triangle");

	return ret;
}

/**
 * Find the area of the outer ring
 */
double
lwtriangle_area(const LWTRIANGLE *triangle)
{
	double area=0.0;
	uint32_t i;
	POINT2D p1;
	POINT2D p2;

	if (! triangle->points->npoints) return area; /* empty triangle */

	for (i=0; i < triangle->points->npoints-1; i++)
	{
		getPoint2d_p(triangle->points, i, &p1);
		getPoint2d_p(triangle->points, i+1, &p2);
		area += ( p1.x * p2.y ) - ( p1.y * p2.x );
	}

	area  /= 2.0;

	return fabs(area);
}


double
lwtriangle_perimeter(const LWTRIANGLE *triangle)
{
	if( triangle->points )
		return ptarray_length(triangle->points);
	else
		return 0.0;
}

double
lwtriangle_perimeter_2d(const LWTRIANGLE *triangle)
{
	if( triangle->points )
		return ptarray_length_2d(triangle->points);
	else
		return 0.0;
}
