/**********************************************************************
 * $Id: lwgeom_geos.c 5258 2010-02-17 21:02:49Z strk $
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 *
 * Copyright 2009-2010 Sandro Santilli <strk@keybit.net>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************
 *
 * ST_SplitGeometry
 *
 * Split polygon by line, line by line, line by point.
 * Returns components as a collection
 *
 * Author: Sandro Santilli <strk@keybit.net>
 *
 * Work done for Faunalia (http://www.faunalia.it) with fundings
 * from Regione Toscana - Sistema Informativo per il Governo
 * del Territorio e dell'Ambiente (RT-SIGTA).
 *
 * Thanks to the PostGIS community for sharing poly/line ideas [1]
 *
 * [1] http://trac.osgeo.org/postgis/wiki/UsersWikiSplitPolygonWithLineString
 *
 *
 **********************************************************************/

#include "lwgeom_geos.h"
#include "funcapi.h"

#include <string.h>
#include <assert.h>

/* #define POSTGIS_DEBUG_LEVEL 4 */

/* Initializes GEOS internally */
static LWGEOM* lwline_split_by_point(LWLINE* lwgeom_in, LWPOINT* blade_in);
static LWGEOM*
lwline_split_by_point(LWLINE* lwline_in, LWPOINT* blade_in)
{
	GEOSGeometry* line;
	GEOSGeometry* point;
	double loc, dist;
	int intersects;
	POINT2D pt;
	POINTARRAY* pa1;
	POINTARRAY* pa2;
	LWGEOM** components;
	LWCOLLECTION* out;

	/* Possible outcomes:
	 *
	 *  1. The point is not on the line
	 *      -> Return original geometry (cloned)
	 *  2. The point is on the line
	 *      -> Return a multiline with all elements
	 */

	getPoint2d_p(blade_in->point, 0, &pt);
	loc = ptarray_locate_point(lwline_in->points, &pt, &dist);

	/* lwnotice("Location: %g -- Distance: %g", loc, dist); */

	if ( dist > 0 ) { /* TODO: accept a tolerance ? */
		/* No intersection */
		return (LWGEOM*)lwline_clone(lwline_in);
	}

	/* There is an intersection, let's get two substrings */

	if ( loc == 0 || loc == 1 )
	{
		/* Intersection is on the boundary or outside */
		return (LWGEOM*)lwline_clone(lwline_in);
	}

	pa1 = ptarray_substring(lwline_in->points, 0, loc);
	pa2 = ptarray_substring(lwline_in->points, loc, 1);

	/* TODO: check if either pa1 or pa2 are empty ? */

	components = lwalloc(sizeof(LWGEOM*)*2);
	components[0] = (LWGEOM*)lwline_construct(-1, NULL, pa1);
	components[1] = (LWGEOM*)lwline_construct(-1, NULL, pa2);

	out = lwcollection_construct(COLLECTIONTYPE, lwline_in->SRID,
		NULL, 2, components);

	/* That's all folks */

	return (LWGEOM*)out;
}

static LWGEOM* lwline_split(LWLINE* lwgeom_in, LWGEOM* blade_in);
static LWGEOM*
lwline_split(LWLINE* lwline_in, LWGEOM* blade_in)
{
	switch (TYPE_GETTYPE(blade_in->type))
	{
	case POINTTYPE:
		return lwline_split_by_point(lwline_in, (LWPOINT*)blade_in);

	case LINETYPE:
	default:
		lwerror("Splitting a Line by a %s is unsupported",
			lwgeom_typename(TYPE_GETTYPE(blade_in->type)));
		return NULL;
	}
	return NULL;
}

static LWGEOM* lwpoly_split(LWPOLY* lwgeom_in, LWGEOM* blade_in);
static LWGEOM*
lwpoly_split(LWPOLY* lwgeom_in, LWGEOM* blade_in)
{
	switch (TYPE_GETTYPE(blade_in->type))
	{
	case LINETYPE:
	default:
		lwerror("Splitting a Polygon by a %s is unsupported",
			lwgeom_typename(TYPE_GETTYPE(blade_in->type)));
		return NULL;
	}
	return NULL;
}

static LWGEOM* lwgeom_split(LWGEOM* lwgeom_in, LWGEOM* blade_in);
static LWGEOM*
lwgeom_split(LWGEOM* lwgeom_in, LWGEOM* blade_in)
{
	switch (TYPE_GETTYPE(lwgeom_in->type))
	{
	case LINETYPE:
		return lwline_split((LWLINE*)lwgeom_in, blade_in);

	case POLYGONTYPE:
		return lwpoly_split((LWPOLY*)lwgeom_in, blade_in);

	default:
		lwerror("Splitting of %d geometries is unsupported",
			lwgeom_typename(TYPE_GETTYPE(lwgeom_in->type)));
		return NULL;
	}

}


Datum ST_SplitGeometry(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_SplitGeometry);
Datum ST_SplitGeometry(PG_FUNCTION_ARGS)
{
#if 0 && POSTGIS_GEOS_VERSION < 33
	elog(ERROR, "You need GEOS-3.3.0 or up for ST_CleanGeometry");
	PG_RETURN_NULL();
#else /* POSTGIS_GEOS_VERSION >= 33 */

	PG_LWGEOM *in, *blade_in, *out;
	LWGEOM *lwgeom_in, *lwblade_in, *lwgeom_out;

	in = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	lwgeom_in = lwgeom_deserialize(SERIALIZED_FORM(in));

	blade_in = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	lwblade_in = lwgeom_deserialize(SERIALIZED_FORM(blade_in));

	lwgeom_out = lwgeom_split(lwgeom_in, lwblade_in);
	if ( ! lwgeom_out ) {
		PG_FREE_IF_COPY(in, 0);
		PG_FREE_IF_COPY(blade_in, 1);
		PG_RETURN_NULL();
	}

	out = pglwgeom_serialize(lwgeom_out);

	PG_FREE_IF_COPY(in, 0);
	PG_FREE_IF_COPY(blade_in, 1);

	PG_RETURN_POINTER(out);

#endif /* POSTGIS_GEOS_VERSION >= 33 */
}

