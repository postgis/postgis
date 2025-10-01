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
#include "liblwgeom_internal.h"
#include "lwgeom_log.h"


uint32_t
lwcompound_num_curves(const LWCOMPOUND *compound)
{
	if ( compound->type != COMPOUNDTYPE )
		lwerror("%s only supports compound curves", __func__);
	return compound->ngeoms;
}

const LWGEOM *
lwcollection_getsubcurve(const LWCOMPOUND *compound, uint32_t curvenum)
{
	return (const LWGEOM *)compound->geoms[curvenum];
}

int
lwcompound_is_closed(const LWCOMPOUND *compound)
{
	int hasz = lwgeom_has_z(lwcompound_as_lwgeom(compound));
	if (lwgeom_is_empty(lwcompound_as_lwgeom(compound)))
		return LW_FALSE;

	for (uint32_t i = 0; i < compound->ngeoms; i++)
	{
		uint32_t i_end = i == 0 ? compound->ngeoms - 1 : i - 1;
		const LWLINE *geom_start = (LWLINE *)(compound->geoms[i]);
		const LWLINE *geom_end = (LWLINE *)(compound->geoms[i_end]);
		const POINTARRAY *pa_start = geom_start->points;
		const POINTARRAY *pa_end = geom_end->points;
		if (hasz)
		{
			const POINT3D *pt_start = getPoint3d_cp(pa_start, 0);
			const POINT3D *pt_end = getPoint3d_cp(pa_end, pa_end->npoints-1);
			if (!p3d_same(pt_start, pt_end))
				return LW_FALSE;
		}
		else
		{
			const POINT2D *pt_start = getPoint2d_cp(pa_start, 0);
			const POINT2D *pt_end = getPoint2d_cp(pa_end, pa_end->npoints-1);
			if (!p2d_same(pt_start, pt_end))
				return LW_FALSE;
		}
	}

	return LW_TRUE;
}

int
lwcompound_is_valid(const LWCOMPOUND *compound)
{
	int hasz = lwgeom_has_z(lwcompound_as_lwgeom(compound));
	if (lwgeom_is_empty(lwcompound_as_lwgeom(compound)))
		return LW_TRUE;

	for (uint32_t i = 1; i < compound->ngeoms; i++)
	{
		const LWLINE *geom_start = (LWLINE *)(compound->geoms[i]);
		const LWLINE *geom_end = (LWLINE *)(compound->geoms[i-1]);
		const POINTARRAY *pa_start = geom_start->points;
		const POINTARRAY *pa_end = geom_end->points;
		if (hasz)
		{
			const POINT3D *pt_start = getPoint3d_cp(pa_start, 0);
			const POINT3D *pt_end = getPoint3d_cp(pa_end, pa_end->npoints-1);
			if (!p3d_same(pt_start, pt_end))
				return LW_FALSE;
		}
		else
		{
			const POINT2D *pt_start = getPoint2d_cp(pa_start, 0);
			const POINT2D *pt_end = getPoint2d_cp(pa_end, pa_end->npoints-1);
			if (!p2d_same(pt_start, pt_end))
				return LW_FALSE;
		}
	}

	return LW_TRUE;
}

double lwcompound_length(const LWCOMPOUND *comp)
{
	return lwcompound_length_2d(comp);
}

double lwcompound_length_2d(const LWCOMPOUND *comp)
{
	uint32_t i;
	double length = 0.0;
	if ( lwgeom_is_empty((LWGEOM*)comp) )
		return 0.0;

	for (i = 0; i < comp->ngeoms; i++)
	{
		length += lwgeom_length_2d(comp->geoms[i]);
	}
	return length;
}

int lwcompound_add_lwgeom(LWCOMPOUND *comp, LWGEOM *geom)
{
	LWCOLLECTION *col = (LWCOLLECTION*)comp;

	/* Empty things can't continuously join up with other things */
	if ( lwgeom_is_empty(geom) )
	{
		LWDEBUG(4, "Got an empty component for a compound curve!");
		return LW_FAILURE;
	}

	if( col->ngeoms > 0 )
	{
		POINT4D last, first;
		/* First point of the component we are adding */
		LWLINE *newline = (LWLINE*)geom;
		/* Last point of the previous component */
		LWLINE *prevline = (LWLINE*)(col->geoms[col->ngeoms-1]);

		getPoint4d_p(newline->points, 0, &first);
		getPoint4d_p(prevline->points, prevline->points->npoints-1, &last);

		if ( !(FP_EQUALS(first.x,last.x) && FP_EQUALS(first.y,last.y)) )
		{
			LWDEBUG(4, "Components don't join up end-to-end!");
			LWDEBUGF(4, "first pt (%g %g %g %g) last pt (%g %g %g %g)", first.x, first.y, first.z, first.m, last.x, last.y, last.z, last.m);
			return LW_FAILURE;
		}
	}

	col = lwcollection_add_lwgeom(col, geom);
	return LW_SUCCESS;
}

LWCOMPOUND *
lwcompound_construct_empty(int32_t srid, char hasz, char hasm)
{
	LWCOMPOUND *ret = (LWCOMPOUND*)lwcollection_construct_empty(COMPOUNDTYPE, srid, hasz, hasm);
	return ret;
}

int lwgeom_contains_point(const LWGEOM *geom, const POINT2D *pt)
{
	switch( geom->type )
	{
		case LINETYPE:
			return ptarray_contains_point(((LWLINE*)geom)->points, pt);
		case CIRCSTRINGTYPE:
			return ptarrayarc_contains_point(((LWCIRCSTRING*)geom)->points, pt);
		case COMPOUNDTYPE:
			return lwcompound_contains_point((LWCOMPOUND*)geom, pt);
	}
	lwerror("lwgeom_contains_point failed");
	return LW_FAILURE;
}

/*
 * Use a ray-casting count to determine if the point
 * is inside or outside of the compound curve. Ray-casting
 * is run against each component of the overall arc, and
 * the even/odd test run against the total of all components.
 * Returns LW_INSIDE / LW_BOUNDARY / LW_OUTSIDE
 */
int
lwcompound_contains_point(const LWCOMPOUND *comp, const POINT2D *pt)
{
	int intersections = 0;

	if (lwgeom_is_empty(lwcompound_as_lwgeom(comp)))
		return LW_OUTSIDE;

	for (uint32_t j = 0; j < comp->ngeoms; j++)
	{
		int on_boundary = LW_FALSE;
		const LWGEOM *sub = comp->geoms[j];
		if (sub->type == LINETYPE)
		{
			LWLINE *lwline = lwgeom_as_lwline(sub);
			intersections += ptarray_raycast_intersections(lwline->points, pt, &on_boundary);
		}
		else if (sub->type == CIRCSTRINGTYPE)
		{
			LWCIRCSTRING *lwcirc = lwgeom_as_lwcircstring(sub);
			intersections += ptarrayarc_raycast_intersections(lwcirc->points, pt, &on_boundary);
		}
		else
		{
			lwerror("%s: unsupported type %s", __func__, lwtype_name(sub->type));
		}
		if (on_boundary)
			return LW_BOUNDARY;
	}

	/*
	 * Odd number of intersections means inside.
	 * Even means outside.
	 */
	return (intersections % 2) ? LW_INSIDE : LW_OUTSIDE;
}

LWCOMPOUND *
lwcompound_construct_from_lwline(const LWLINE *lwline)
{
  LWCOMPOUND* ogeom = lwcompound_construct_empty(lwline->srid, FLAGS_GET_Z(lwline->flags), FLAGS_GET_M(lwline->flags));
  lwcompound_add_lwgeom(ogeom, lwgeom_clone((LWGEOM*)lwline));
	/* ogeom->bbox = lwline->bbox; */
  return ogeom;
}

LWPOINT*
lwcompound_get_lwpoint(const LWCOMPOUND *lwcmp, uint32_t where)
{
	uint32_t i;
	uint32_t count = 0;
	uint32_t npoints = 0;
	if ( lwgeom_is_empty((LWGEOM*)lwcmp) )
		return NULL;

	npoints = lwgeom_count_vertices((LWGEOM*)lwcmp);
	if ( where >= npoints )
	{
		lwerror("%s: index %d is not in range of number of vertices (%d) in input", __func__, where, npoints);
		return NULL;
	}

	for ( i = 0; i < lwcmp->ngeoms; i++ )
	{
		LWGEOM* part = lwcmp->geoms[i];
		uint32_t npoints_part = lwgeom_count_vertices(part);
		if ( where >= count && where < count + npoints_part )
		{
			return lwline_get_lwpoint((LWLINE*)part, where - count);
		}
		else
		{
			count += npoints_part;
		}
	}

	return NULL;
}



LWPOINT *
lwcompound_get_startpoint(const LWCOMPOUND *lwcmp)
{
	return lwcompound_get_lwpoint(lwcmp, 0);
}

LWPOINT *
lwcompound_get_endpoint(const LWCOMPOUND *lwcmp)
{
	LWLINE *lwline;
	if ( lwcmp->ngeoms < 1 )
	{
		return NULL;
	}

	lwline = (LWLINE*)(lwcmp->geoms[lwcmp->ngeoms-1]);

	if ( (!lwline) || (!lwline->points) || (lwline->points->npoints < 1) )
	{
		return NULL;
	}

	return lwline_get_lwpoint(lwline, lwline->points->npoints-1);
}

