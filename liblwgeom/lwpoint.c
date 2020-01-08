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
 * Copyright (C) 2001-2006 Refractions Research Inc.
 *
 **********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../postgis_config.h"
/*#define POSTGIS_DEBUG_LEVEL 4*/
#include "liblwgeom_internal.h"
#include "lwgeom_log.h"


/*
 * Convenience functions to hide the POINTARRAY
 * TODO: obsolete this
 */
int
lwpoint_getPoint2d_p(const LWPOINT *point, POINT2D *out)
{
	return lwpoint_is_empty(point) ? 0 : getPoint2d_p(point->point, 0, out);
}

/* convenience functions to hide the POINTARRAY */
int
lwpoint_getPoint3dz_p(const LWPOINT *point, POINT3DZ *out)
{
	return lwpoint_is_empty(point) ? 0 : getPoint3dz_p(point->point,0,out);
}
int
lwpoint_getPoint3dm_p(const LWPOINT *point, POINT3DM *out)
{
	return lwpoint_is_empty(point) ? 0 : getPoint3dm_p(point->point,0,out);
}
int
lwpoint_getPoint4d_p(const LWPOINT *point, POINT4D *out)
{
	return lwpoint_is_empty(point) ? 0 : getPoint4d_p(point->point,0,out);
}

double
lwpoint_get_x(const LWPOINT *point)
{
	POINT4D pt;
	if ( lwpoint_is_empty(point) )
	{
		lwerror("lwpoint_get_x called with empty geometry");
		return 0;
	}
	getPoint4d_p(point->point, 0, &pt);
	return pt.x;
}

double
lwpoint_get_y(const LWPOINT *point)
{
	POINT4D pt;
	if ( lwpoint_is_empty(point) )
	{
		lwerror("lwpoint_get_y called with empty geometry");
		return 0;
	}
	getPoint4d_p(point->point, 0, &pt);
	return pt.y;
}

double
lwpoint_get_z(const LWPOINT *point)
{
	POINT4D pt;
	if ( lwpoint_is_empty(point) )
	{
		lwerror("lwpoint_get_z called with empty geometry");
		return 0;
	}
	if ( ! FLAGS_GET_Z(point->flags) )
	{
		lwerror("lwpoint_get_z called without z dimension");
		return 0;
	}
	getPoint4d_p(point->point, 0, &pt);
	return pt.z;
}

double
lwpoint_get_m(const LWPOINT *point)
{
	POINT4D pt;
	if ( lwpoint_is_empty(point) )
	{
		lwerror("lwpoint_get_m called with empty geometry");
		return 0;
	}
	if ( ! FLAGS_GET_M(point->flags) )
	{
		lwerror("lwpoint_get_m called without m dimension");
		return 0;
	}
	getPoint4d_p(point->point, 0, &pt);
	return pt.m;
}

/*
 * Construct a new point.  point will not be copied
 * use SRID=SRID_UNKNOWN for unknown SRID (will have 8bit type's S = 0)
 */
LWPOINT *
lwpoint_construct(int32_t srid, GBOX *bbox, POINTARRAY *point)
{
	LWPOINT *result;
	lwflags_t flags = 0;

	if (point == NULL)
		return NULL; /* error */

	result = lwalloc(sizeof(LWPOINT));
	result->type = POINTTYPE;
	FLAGS_SET_Z(flags, FLAGS_GET_Z(point->flags));
	FLAGS_SET_M(flags, FLAGS_GET_M(point->flags));
	FLAGS_SET_BBOX(flags, bbox?1:0);
	result->flags = flags;
	result->srid = srid;
	result->point = point;
	result->bbox = bbox;

	return result;
}

LWPOINT *
lwpoint_construct_empty(int32_t srid, char hasz, char hasm)
{
	LWPOINT *result = lwalloc(sizeof(LWPOINT));
	result->type = POINTTYPE;
	result->flags = lwflags(hasz, hasm, 0);
	result->srid = srid;
	result->point = ptarray_construct(hasz, hasm, 0);
	result->bbox = NULL;
	return result;
}

LWPOINT *
lwpoint_make2d(int32_t srid, double x, double y)
{
	POINT4D p = {x, y, 0.0, 0.0};
	POINTARRAY *pa = ptarray_construct_empty(0, 0, 1);

	ptarray_append_point(pa, &p, LW_TRUE);
	return lwpoint_construct(srid, NULL, pa);
}

LWPOINT *
lwpoint_make3dz(int32_t srid, double x, double y, double z)
{
	POINT4D p = {x, y, z, 0.0};
	POINTARRAY *pa = ptarray_construct_empty(1, 0, 1);

	ptarray_append_point(pa, &p, LW_TRUE);

	return lwpoint_construct(srid, NULL, pa);
}

LWPOINT *
lwpoint_make3dm(int32_t srid, double x, double y, double m)
{
	POINT4D p = {x, y, 0.0, m};
	POINTARRAY *pa = ptarray_construct_empty(0, 1, 1);

	ptarray_append_point(pa, &p, LW_TRUE);

	return lwpoint_construct(srid, NULL, pa);
}

LWPOINT *
lwpoint_make4d(int32_t srid, double x, double y, double z, double m)
{
	POINT4D p = {x, y, z, m};
	POINTARRAY *pa = ptarray_construct_empty(1, 1, 1);

	ptarray_append_point(pa, &p, LW_TRUE);

	return lwpoint_construct(srid, NULL, pa);
}

LWPOINT *
lwpoint_make(int32_t srid, int hasz, int hasm, const POINT4D *p)
{
	POINTARRAY *pa = ptarray_construct_empty(hasz, hasm, 1);
	ptarray_append_point(pa, p, LW_TRUE);
	return lwpoint_construct(srid, NULL, pa);
}

void lwpoint_free(LWPOINT *pt)
{
	if ( ! pt ) return;

	if ( pt->bbox )
		lwfree(pt->bbox);
	if ( pt->point )
		ptarray_free(pt->point);
	lwfree(pt);
}

void printLWPOINT(LWPOINT *point)
{
	lwnotice("LWPOINT {");
	lwnotice("    ndims = %i", (int)FLAGS_NDIMS(point->flags));
	lwnotice("    BBOX = %i", FLAGS_GET_BBOX(point->flags) ? 1 : 0 );
	lwnotice("    SRID = %i", (int)point->srid);
	printPA(point->point);
	lwnotice("}");
}

/* @brief Clone LWPOINT object. Serialized point lists are not copied.
 *
 * @see ptarray_clone
 */
LWPOINT *
lwpoint_clone(const LWPOINT *g)
{
	LWPOINT *ret = lwalloc(sizeof(LWPOINT));

	LWDEBUG(2, "lwpoint_clone called");

	memcpy(ret, g, sizeof(LWPOINT));

	ret->point = ptarray_clone(g->point);

	if ( g->bbox ) ret->bbox = gbox_copy(g->bbox);
	return ret;
}



void
lwpoint_release(LWPOINT *lwpoint)
{
	lwgeom_release(lwpoint_as_lwgeom(lwpoint));
}


/* check coordinate equality  */
char
lwpoint_same(const LWPOINT *p1, const LWPOINT *p2)
{
	return ptarray_same(p1->point, p2->point);
}


LWPOINT*
lwpoint_force_dims(const LWPOINT *point, int hasz, int hasm, double zval, double mval)
{
	POINTARRAY *pdims = NULL;
	LWPOINT *pointout;

	/* Return 2D empty */
	if( lwpoint_is_empty(point) )
	{
		pointout = lwpoint_construct_empty(point->srid, hasz, hasm);
	}
	else
	{
		/* Always we duplicate the ptarray and return */
		pdims = ptarray_force_dims(point->point, hasz, hasm, zval, mval);
		pointout = lwpoint_construct(point->srid, NULL, pdims);
	}
	pointout->type = point->type;
	return pointout;
}


