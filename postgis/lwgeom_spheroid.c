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
 * Copyright (C) 2001-2003 Refractions Research Inc.
 *
 **********************************************************************/


#include "postgres.h"


#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>

#include "access/gist.h"
#include "access/itup.h"

#include "fmgr.h"
#include "utils/elog.h"

#include "../postgis_config.h"
#include "liblwgeom.h"
#include "lwgeom_pg.h"

#define SHOW_DIGS_DOUBLE 15
#define MAX_DIGS_DOUBLE (SHOW_DIGS_DOUBLE + 6 + 1 + 3 +1)


/* PG-exposed  */
Datum ellipsoid_in(PG_FUNCTION_ARGS);
Datum ellipsoid_out(PG_FUNCTION_ARGS);
Datum LWGEOM_length2d_ellipsoid(PG_FUNCTION_ARGS);
Datum LWGEOM_length_ellipsoid_linestring(PG_FUNCTION_ARGS);
Datum LWGEOM_distance_ellipsoid(PG_FUNCTION_ARGS);
Datum LWGEOM_distance_sphere(PG_FUNCTION_ARGS);
Datum geometry_distance_spheroid(PG_FUNCTION_ARGS);


/*
 * Use the WKT definition of an ellipsoid
 * ie. SPHEROID["name",A,rf] or SPHEROID("name",A,rf)
 *	  SPHEROID["GRS_1980",6378137,298.257222101]
 * wkt says you can use "(" or "["
 */
PG_FUNCTION_INFO_V1(ellipsoid_in);
Datum ellipsoid_in(PG_FUNCTION_ARGS)
{
	char *str = PG_GETARG_CSTRING(0);
	SPHEROID *sphere = (SPHEROID *) palloc(sizeof(SPHEROID));
	int nitems;
	double rf;

	memset(sphere,0, sizeof(SPHEROID));

	if (strstr(str,"SPHEROID") !=  str )
	{
		elog(ERROR,"SPHEROID parser - doesn't start with SPHEROID");
		pfree(sphere);
		PG_RETURN_NULL();
	}

	nitems = sscanf(str,"SPHEROID[\"%19[^\"]\",%lf,%lf]",
	                sphere->name, &sphere->a, &rf);

	if (nitems != 3)
		nitems = sscanf(str,"SPHEROID(\"%19[^\"]\",%lf,%lf)",
		                sphere->name, &sphere->a, &rf);

	if (nitems != 3)
	{
		elog(ERROR,"SPHEROID parser - couldn't parse the spheroid");
		pfree(sphere);
		PG_RETURN_NULL();
	}

	sphere->f = 1.0/rf;
	sphere->b = sphere->a - (1.0/rf)*sphere->a;
	sphere->e_sq = ((sphere->a*sphere->a) - (sphere->b*sphere->b)) /
	               (sphere->a*sphere->a);
	sphere->e = sqrt(sphere->e_sq);

	PG_RETURN_POINTER(sphere);

}

PG_FUNCTION_INFO_V1(ellipsoid_out);
Datum ellipsoid_out(PG_FUNCTION_ARGS)
{
	SPHEROID *sphere = (SPHEROID *) PG_GETARG_POINTER(0);
	char *result;
	size_t sz = MAX_DIGS_DOUBLE + MAX_DIGS_DOUBLE + 20 + 9 + 2;
	result = palloc(sz);

	snprintf(result, sz, "SPHEROID(\"%s\",%.15g,%.15g)",
	        sphere->name, sphere->a, 1.0/sphere->f);

	PG_RETURN_CSTRING(result);
}



/*
 * Find the "length of a geometry"
 * length2d_spheroid(point, sphere) = 0
 * length2d_spheroid(line, sphere) = length of line
 * length2d_spheroid(polygon, sphere) = 0
 *	-- could make sense to return sum(ring perimeter)
 * uses ellipsoidal math to find the distance
 * x's are longitude, and y's are latitude - both in decimal degrees
 */
PG_FUNCTION_INFO_V1(LWGEOM_length2d_ellipsoid);
Datum LWGEOM_length2d_ellipsoid(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	SPHEROID *sphere = (SPHEROID *) PG_GETARG_POINTER(1);
	LWGEOM *lwgeom = lwgeom_from_gserialized(geom);
	double dist = lwgeom_length_spheroid(lwgeom, sphere);
	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_FLOAT8(dist);
}


/*
 * Find the "length of a geometry"
 *
 * length2d_spheroid(point, sphere) = 0
 * length2d_spheroid(line, sphere) = length of line
 * length2d_spheroid(polygon, sphere) = 0
 *	-- could make sense to return sum(ring perimeter)
 * uses ellipsoidal math to find the distance
 * x's are longitude, and y's are latitude - both in decimal degrees
 */
PG_FUNCTION_INFO_V1(LWGEOM_length_ellipsoid_linestring);
Datum LWGEOM_length_ellipsoid_linestring(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	LWGEOM *lwgeom = lwgeom_from_gserialized(geom);
	SPHEROID *sphere = (SPHEROID *) PG_GETARG_POINTER(1);
	double length = 0.0;

	/* EMPTY things have no length */
	if ( lwgeom_is_empty(lwgeom) )
	{
		lwgeom_free(lwgeom);
		PG_RETURN_FLOAT8(0.0);
	}

	length = lwgeom_length_spheroid(lwgeom, sphere);
	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(geom, 0);

	/* Something went wrong... */
	if ( length < 0.0 )
	{
		elog(ERROR, "lwgeom_length_spheroid returned length < 0.0");
		PG_RETURN_NULL();
	}

	/* Clean up */
	PG_RETURN_FLOAT8(length);
}


/*
 * distance (geometry,geometry, sphere)
 */
PG_FUNCTION_INFO_V1(geometry_distance_spheroid);
Datum geometry_distance_spheroid(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *geom2 = PG_GETARG_GSERIALIZED_P(1);
	SPHEROID *sphere = (SPHEROID *)PG_GETARG_POINTER(2);
	int type1 = gserialized_get_type(geom1);
	int type2 = gserialized_get_type(geom2);
	bool use_spheroid = PG_GETARG_BOOL(3);
	LWGEOM *lwgeom1, *lwgeom2;
	double distance;
	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	/* Calculate some other parameters on the spheroid */
	spheroid_init(sphere, sphere->a, sphere->b);

	/* Catch sphere special case and re-jig spheroid appropriately */
	if ( ! use_spheroid )
	{
		sphere->a = sphere->b = sphere->radius;
	}

	if ( ! ( type1 == POINTTYPE || type1 == LINETYPE || type1 == POLYGONTYPE ||
	         type1 == MULTIPOINTTYPE || type1 == MULTILINETYPE || type1 == MULTIPOLYGONTYPE ))
	{
		elog(ERROR, "geometry_distance_spheroid: Only point/line/polygon supported.\n");
		PG_RETURN_NULL();
	}

	if ( ! ( type2 == POINTTYPE || type2 == LINETYPE || type2 == POLYGONTYPE ||
	         type2 == MULTIPOINTTYPE || type2 == MULTILINETYPE || type2 == MULTIPOLYGONTYPE ))
	{
		elog(ERROR, "geometry_distance_spheroid: Only point/line/polygon supported.\n");
		PG_RETURN_NULL();
	}

	/* Get #LWGEOM structures */
	lwgeom1 = lwgeom_from_gserialized(geom1);
	lwgeom2 = lwgeom_from_gserialized(geom2);

	/* We are going to be calculating geodetic distances */
	lwgeom_set_geodetic(lwgeom1, LW_TRUE);
	lwgeom_set_geodetic(lwgeom2, LW_TRUE);
	lwgeom_refresh_bbox(lwgeom1);
	lwgeom_refresh_bbox(lwgeom2);

	distance = lwgeom_distance_spheroid(lwgeom1, lwgeom2, sphere, 0.0);

	PG_RETURN_FLOAT8(distance);

}

PG_FUNCTION_INFO_V1(LWGEOM_distance_ellipsoid);
Datum LWGEOM_distance_ellipsoid(PG_FUNCTION_ARGS)
{
	SPHEROID s;

	/* No spheroid provided */
	if (PG_NARGS() == 2) {
		/* Init to WGS84 */
		spheroid_init(&s, 6378137.0, 6356752.314245179498);
		PG_RETURN_DATUM(DirectFunctionCall4(geometry_distance_spheroid,
			PG_GETARG_DATUM(0),
			PG_GETARG_DATUM(1),
			PointerGetDatum(&s),
			BoolGetDatum(true)));
	}

	PG_RETURN_DATUM(DirectFunctionCall4(geometry_distance_spheroid,
		PG_GETARG_DATUM(0),
		PG_GETARG_DATUM(1),
		PG_GETARG_DATUM(2),
		BoolGetDatum(true)));
}

PG_FUNCTION_INFO_V1(LWGEOM_distance_sphere);
Datum LWGEOM_distance_sphere(PG_FUNCTION_ARGS)
{
	SPHEROID s;
	/* Init to WGS84 */
	spheroid_init(&s, 6378137.0, 6356752.314245179498);

	if (PG_NARGS() == 3) {
		s.radius = PG_GETARG_FLOAT8(2);
	}
	s.a = s.b = s.radius;

	PG_RETURN_DATUM(DirectFunctionCall4(geometry_distance_spheroid,
	                                    PG_GETARG_DATUM(0), PG_GETARG_DATUM(1), PointerGetDatum(&s), BoolGetDatum(false)));
}

