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
 * Copyright 2001-2006 Refractions Research Inc.
 * Copyright 2017-2018 Daniel Baston <dbaston@gmail.com>
 *
 **********************************************************************/

#include "postgres.h"
#include "fmgr.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/elog.h"
#include "utils/geo_decls.h"

#include "../postgis_config.h"
#include "liblwgeom.h"
#include "liblwgeom_internal.h"
#include "lwgeom_pg.h"

#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>

#define xstr(s) str(s)
#define str(s) #s

Datum LWGEOM_mem_size(PG_FUNCTION_ARGS);
Datum LWGEOM_summary(PG_FUNCTION_ARGS);
Datum LWGEOM_npoints(PG_FUNCTION_ARGS);
Datum LWGEOM_nrings(PG_FUNCTION_ARGS);
Datum ST_Area(PG_FUNCTION_ARGS);
Datum postgis_scripts_released(PG_FUNCTION_ARGS);
Datum postgis_version(PG_FUNCTION_ARGS);
Datum postgis_liblwgeom_version(PG_FUNCTION_ARGS);
Datum postgis_lib_version(PG_FUNCTION_ARGS);
Datum postgis_svn_version(PG_FUNCTION_ARGS);
Datum postgis_lib_revision(PG_FUNCTION_ARGS);
Datum postgis_libxml_version(PG_FUNCTION_ARGS);
Datum postgis_lib_build_date(PG_FUNCTION_ARGS);
Datum LWGEOM_length2d_linestring(PG_FUNCTION_ARGS);
Datum LWGEOM_length_linestring(PG_FUNCTION_ARGS);
Datum LWGEOM_perimeter2d_poly(PG_FUNCTION_ARGS);
Datum LWGEOM_perimeter_poly(PG_FUNCTION_ARGS);

Datum LWGEOM_maxdistance2d_linestring(PG_FUNCTION_ARGS);
Datum ST_Distance(PG_FUNCTION_ARGS);
Datum LWGEOM_closestpoint(PG_FUNCTION_ARGS);
Datum LWGEOM_shortestline2d(PG_FUNCTION_ARGS);
Datum LWGEOM_longestline2d(PG_FUNCTION_ARGS);
Datum LWGEOM_dwithin(PG_FUNCTION_ARGS);
Datum LWGEOM_dfullywithin(PG_FUNCTION_ARGS);

Datum LWGEOM_maxdistance3d(PG_FUNCTION_ARGS);
Datum LWGEOM_mindistance3d(PG_FUNCTION_ARGS);
Datum LWGEOM_closestpoint3d(PG_FUNCTION_ARGS);
Datum LWGEOM_shortestline3d(PG_FUNCTION_ARGS);
Datum LWGEOM_longestline3d(PG_FUNCTION_ARGS);
Datum LWGEOM_dwithin3d(PG_FUNCTION_ARGS);
Datum LWGEOM_dfullywithin3d(PG_FUNCTION_ARGS);

Datum LWGEOM_inside_circle_point(PG_FUNCTION_ARGS);
Datum LWGEOM_collect(PG_FUNCTION_ARGS);
Datum LWGEOM_collect_garray(PG_FUNCTION_ARGS);
Datum LWGEOM_expand(PG_FUNCTION_ARGS);
Datum LWGEOM_to_BOX(PG_FUNCTION_ARGS);
Datum LWGEOM_envelope(PG_FUNCTION_ARGS);
Datum LWGEOM_isempty(PG_FUNCTION_ARGS);
Datum LWGEOM_segmentize2d(PG_FUNCTION_ARGS);
Datum LWGEOM_reverse(PG_FUNCTION_ARGS);
Datum LWGEOM_force_clockwise_poly(PG_FUNCTION_ARGS);
Datum LWGEOM_force_sfs(PG_FUNCTION_ARGS);
Datum LWGEOM_noop(PG_FUNCTION_ARGS);
Datum LWGEOM_zmflag(PG_FUNCTION_ARGS);
Datum LWGEOM_hasz(PG_FUNCTION_ARGS);
Datum LWGEOM_hasm(PG_FUNCTION_ARGS);
Datum LWGEOM_ndims(PG_FUNCTION_ARGS);
Datum LWGEOM_makepoint(PG_FUNCTION_ARGS);
Datum LWGEOM_makepoint3dm(PG_FUNCTION_ARGS);
Datum LWGEOM_makeline_garray(PG_FUNCTION_ARGS);
Datum LWGEOM_makeline(PG_FUNCTION_ARGS);
Datum LWGEOM_makepoly(PG_FUNCTION_ARGS);
Datum LWGEOM_line_from_mpoint(PG_FUNCTION_ARGS);
Datum LWGEOM_addpoint(PG_FUNCTION_ARGS);
Datum LWGEOM_removepoint(PG_FUNCTION_ARGS);
Datum LWGEOM_setpoint_linestring(PG_FUNCTION_ARGS);
Datum LWGEOM_asEWKT(PG_FUNCTION_ARGS);
Datum LWGEOM_hasBBOX(PG_FUNCTION_ARGS);
Datum LWGEOM_azimuth(PG_FUNCTION_ARGS);
Datum LWGEOM_angle(PG_FUNCTION_ARGS);
Datum LWGEOM_affine(PG_FUNCTION_ARGS);
Datum LWGEOM_longitude_shift(PG_FUNCTION_ARGS);
Datum optimistic_overlap(PG_FUNCTION_ARGS);
Datum ST_GeoHash(PG_FUNCTION_ARGS);
Datum ST_MakeEnvelope(PG_FUNCTION_ARGS);
Datum ST_TileEnvelope(PG_FUNCTION_ARGS);
Datum ST_CollectionExtract(PG_FUNCTION_ARGS);
Datum ST_CollectionHomogenize(PG_FUNCTION_ARGS);
Datum ST_IsCollection(PG_FUNCTION_ARGS);
Datum ST_QuantizeCoordinates(PG_FUNCTION_ARGS);
Datum ST_WrapX(PG_FUNCTION_ARGS);
Datum LWGEOM_FilterByM(PG_FUNCTION_ARGS);

/*------------------------------------------------------------------*/

/** find the size of geometry */
PG_FUNCTION_INFO_V1(LWGEOM_mem_size);
Datum LWGEOM_mem_size(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	size_t size = VARSIZE(geom);
	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_INT32(size);
}

/** get summary info on a GEOMETRY */
PG_FUNCTION_INFO_V1(LWGEOM_summary);
Datum LWGEOM_summary(PG_FUNCTION_ARGS)
{
	text *summary;
	GSERIALIZED *g = PG_GETARG_GSERIALIZED_P(0);
	LWGEOM *lwg = lwgeom_from_gserialized(g);
	char *lwresult = lwgeom_summary(lwg, 0);
	uint32_t gver = gserialized_get_version(g);
	size_t result_sz = strlen(lwresult) + 8;
	char *result;
	if (gver == 0)
	{
		result = lwalloc(result_sz + 2);
		snprintf(result, result_sz, "0:%s", lwresult);
	}
	else
	{
		result = lwalloc(result_sz);
		snprintf(result, result_sz, "%s", lwresult);
	}
	lwgeom_free(lwg);
	lwfree(lwresult);

	/* create a text obj to return */
	summary = cstring_to_text(result);
	lwfree(result);

	PG_FREE_IF_COPY(g, 0);
	PG_RETURN_TEXT_P(summary);
}

PG_FUNCTION_INFO_V1(postgis_version);
Datum postgis_version(PG_FUNCTION_ARGS)
{
	char *ver = POSTGIS_VERSION;
	text *result = cstring_to_text(ver);
	PG_RETURN_TEXT_P(result);
}

PG_FUNCTION_INFO_V1(postgis_liblwgeom_version);
Datum postgis_liblwgeom_version(PG_FUNCTION_ARGS)
{
	const char *ver = lwgeom_version();
	text *result = cstring_to_text(ver);
	PG_RETURN_TEXT_P(result);
}

PG_FUNCTION_INFO_V1(postgis_lib_version);
Datum postgis_lib_version(PG_FUNCTION_ARGS)
{
	char *ver = POSTGIS_LIB_VERSION;
	text *result = cstring_to_text(ver);
	PG_RETURN_TEXT_P(result);
}

PG_FUNCTION_INFO_V1(postgis_lib_revision);
Datum postgis_lib_revision(PG_FUNCTION_ARGS)
{
	static char *rev = xstr(POSTGIS_REVISION);
	char ver[32];
	if (rev && rev[0] != '\0')
	{
		snprintf(ver, 32, "%s", rev);
		ver[31] = '\0';
		PG_RETURN_TEXT_P(cstring_to_text(ver));
	}
	else PG_RETURN_NULL();
}

PG_FUNCTION_INFO_V1(postgis_lib_build_date);
Datum postgis_lib_build_date(PG_FUNCTION_ARGS)
{
	char *ver = POSTGIS_BUILD_DATE;
	text *result = cstring_to_text(ver);
	PG_RETURN_TEXT_P(result);
}

PG_FUNCTION_INFO_V1(postgis_scripts_released);
Datum postgis_scripts_released(PG_FUNCTION_ARGS)
{
	char ver[64];
	text *result;

	snprintf(ver, 64, "%s %s", POSTGIS_LIB_VERSION, xstr(POSTGIS_REVISION));
	ver[63] = '\0';

	result = cstring_to_text(ver);
	PG_RETURN_TEXT_P(result);
}

PG_FUNCTION_INFO_V1(postgis_libxml_version);
Datum postgis_libxml_version(PG_FUNCTION_ARGS)
{
	char *ver = POSTGIS_LIBXML2_VERSION;
	text *result = cstring_to_text(ver);
	PG_RETURN_TEXT_P(result);
}

/** number of points in an object */
PG_FUNCTION_INFO_V1(LWGEOM_npoints);
Datum LWGEOM_npoints(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	LWGEOM *lwgeom = lwgeom_from_gserialized(geom);
	int npoints = 0;

	npoints = lwgeom_count_vertices(lwgeom);
	lwgeom_free(lwgeom);

	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_INT32(npoints);
}

/** number of rings in an object */
PG_FUNCTION_INFO_V1(LWGEOM_nrings);
Datum LWGEOM_nrings(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	LWGEOM *lwgeom = lwgeom_from_gserialized(geom);
	int nrings = 0;

	nrings = lwgeom_count_rings(lwgeom);
	lwgeom_free(lwgeom);

	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_INT32(nrings);
}

/**
 * @brief Calculate the area of all the subobj in a polygon
 * 		area(point) = 0
 * 		area (line) = 0
 * 		area(polygon) = find its 2d area
 */
PG_FUNCTION_INFO_V1(ST_Area);
Datum ST_Area(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	LWGEOM *lwgeom = lwgeom_from_gserialized(geom);
	double area = 0.0;

	area = lwgeom_area(lwgeom);

	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_FLOAT8(area);
}

/**
 * @brief find the "length of a geometry"
 *  	length2d(point) = 0
 *  	length2d(line) = length of line
 *  	length2d(polygon) = 0  -- could make sense to return sum(ring perimeter)
 *  	uses euclidian 2d length (even if input is 3d)
 */
PG_FUNCTION_INFO_V1(LWGEOM_length2d_linestring);
Datum LWGEOM_length2d_linestring(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	LWGEOM *lwgeom = lwgeom_from_gserialized(geom);
	double dist = lwgeom_length_2d(lwgeom);
	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_FLOAT8(dist);
}

/**
 * @brief find the "length of a geometry"
 *  	length(point) = 0
 *  	length(line) = length of line
 *  	length(polygon) = 0  -- could make sense to return sum(ring perimeter)
 *  	uses euclidian 3d/2d length depending on input dimensions.
 */
PG_FUNCTION_INFO_V1(LWGEOM_length_linestring);
Datum LWGEOM_length_linestring(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	LWGEOM *lwgeom = lwgeom_from_gserialized(geom);
	double dist = lwgeom_length(lwgeom);
	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_FLOAT8(dist);
}

/**
 *  @brief find the "perimeter of a geometry"
 *  	perimeter(point) = 0
 *  	perimeter(line) = 0
 *  	perimeter(polygon) = sum of ring perimeters
 *  	uses euclidian 3d/2d computation depending on input dimension.
 */
PG_FUNCTION_INFO_V1(LWGEOM_perimeter_poly);
Datum LWGEOM_perimeter_poly(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	LWGEOM *lwgeom = lwgeom_from_gserialized(geom);
	double perimeter = 0.0;

	perimeter = lwgeom_perimeter(lwgeom);
	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_FLOAT8(perimeter);
}

/**
 *  @brief find the "perimeter of a geometry"
 *  	perimeter(point) = 0
 *  	perimeter(line) = 0
 *  	perimeter(polygon) = sum of ring perimeters
 *  	uses euclidian 2d computation even if input is 3d
 */
PG_FUNCTION_INFO_V1(LWGEOM_perimeter2d_poly);
Datum LWGEOM_perimeter2d_poly(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	LWGEOM *lwgeom = lwgeom_from_gserialized(geom);
	double perimeter = 0.0;

	perimeter = lwgeom_perimeter_2d(lwgeom);
	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_FLOAT8(perimeter);
}

/* transform input geometry to 2d if not 2d already */
PG_FUNCTION_INFO_V1(LWGEOM_force_2d);
Datum LWGEOM_force_2d(PG_FUNCTION_ARGS)
{
	GSERIALIZED *pg_geom_in = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *pg_geom_out;
	LWGEOM *lwg_in, *lwg_out;

	/* already 2d */
	if (gserialized_ndims(pg_geom_in) == 2)
		PG_RETURN_POINTER(pg_geom_in);

	lwg_in = lwgeom_from_gserialized(pg_geom_in);
	lwg_out = lwgeom_force_2d(lwg_in);
	pg_geom_out = geometry_serialize(lwg_out);
	lwgeom_free(lwg_out);
	lwgeom_free(lwg_in);

	PG_FREE_IF_COPY(pg_geom_in, 0);
	PG_RETURN_POINTER(pg_geom_out);
}

/* transform input geometry to 3dz if not 3dz already */
PG_FUNCTION_INFO_V1(LWGEOM_force_3dz);
Datum LWGEOM_force_3dz(PG_FUNCTION_ARGS)
{
	GSERIALIZED *pg_geom_in = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *pg_geom_out;
	LWGEOM *lwg_in, *lwg_out;
	double z = PG_NARGS() < 2 ? 0 : PG_GETARG_FLOAT8(1);

	/* already 3d */
	if (gserialized_ndims(pg_geom_in) == 3 && gserialized_has_z(pg_geom_in))
		PG_RETURN_POINTER(pg_geom_in);

	lwg_in = lwgeom_from_gserialized(pg_geom_in);
	lwg_out = lwgeom_force_3dz(lwg_in, z);
	pg_geom_out = geometry_serialize(lwg_out);
	lwgeom_free(lwg_out);
	lwgeom_free(lwg_in);

	PG_FREE_IF_COPY(pg_geom_in, 0);
	PG_RETURN_POINTER(pg_geom_out);
}

/** transform input geometry to 3dm if not 3dm already */
PG_FUNCTION_INFO_V1(LWGEOM_force_3dm);
Datum LWGEOM_force_3dm(PG_FUNCTION_ARGS)
{
	GSERIALIZED *pg_geom_in = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *pg_geom_out;
	LWGEOM *lwg_in, *lwg_out;
	double m = PG_NARGS() < 2 ? 0 : PG_GETARG_FLOAT8(1);

	/* already 3d */
	if (gserialized_ndims(pg_geom_in) == 3 && gserialized_has_m(pg_geom_in))
		PG_RETURN_POINTER(pg_geom_in);

	lwg_in = lwgeom_from_gserialized(pg_geom_in);
	lwg_out = lwgeom_force_3dm(lwg_in, m);
	pg_geom_out = geometry_serialize(lwg_out);
	lwgeom_free(lwg_out);
	lwgeom_free(lwg_in);

	PG_FREE_IF_COPY(pg_geom_in, 0);
	PG_RETURN_POINTER(pg_geom_out);
}

/* transform input geometry to 4d if not 4d already */
PG_FUNCTION_INFO_V1(LWGEOM_force_4d);
Datum LWGEOM_force_4d(PG_FUNCTION_ARGS)
{
	GSERIALIZED *pg_geom_in = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *pg_geom_out;
	LWGEOM *lwg_in, *lwg_out;
	double z = PG_NARGS() < 3 ? 0 : PG_GETARG_FLOAT8(1);
	double m = PG_NARGS() < 3 ? 0 : PG_GETARG_FLOAT8(2);

	/* already 4d */
	if (gserialized_ndims(pg_geom_in) == 4)
		PG_RETURN_POINTER(pg_geom_in);

	lwg_in = lwgeom_from_gserialized(pg_geom_in);
	lwg_out = lwgeom_force_4d(lwg_in, z, m);
	pg_geom_out = geometry_serialize(lwg_out);
	lwgeom_free(lwg_out);
	lwgeom_free(lwg_in);

	PG_FREE_IF_COPY(pg_geom_in, 0);
	PG_RETURN_POINTER(pg_geom_out);
}

/** transform input geometry to a collection type */
PG_FUNCTION_INFO_V1(LWGEOM_force_collection);
Datum LWGEOM_force_collection(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *result;
	LWGEOM **lwgeoms;
	LWGEOM *lwgeom;
	int32_t srid;
	GBOX *bbox;

	POSTGIS_DEBUG(2, "LWGEOM_force_collection called");

	/*
	 * This funx is a no-op only if a bbox cache is already present
	 * in input. If bbox cache is not there we'll need to handle
	 * automatic bbox addition FOR_COMPLEX_GEOMS.
	 */
	if (gserialized_get_type(geom) == COLLECTIONTYPE && gserialized_has_bbox(geom))
	{
		PG_RETURN_POINTER(geom);
	}

	/* deserialize into lwgeoms[0] */
	lwgeom = lwgeom_from_gserialized(geom);

	/* alread a multi*, just make it a collection */
	if (lwgeom_is_collection(lwgeom))
	{
		lwgeom->type = COLLECTIONTYPE;
	}

	/* single geom, make it a collection */
	else
	{
		srid = lwgeom->srid;
		/* We transfer bbox ownership from input to output */
		bbox = lwgeom->bbox;
		lwgeom->srid = SRID_UNKNOWN;
		lwgeom->bbox = NULL;
		lwgeoms = palloc(sizeof(LWGEOM *));
		lwgeoms[0] = lwgeom;
		lwgeom = (LWGEOM *)lwcollection_construct(COLLECTIONTYPE, srid, bbox, 1, lwgeoms);
	}

	result = geometry_serialize(lwgeom);
	lwgeom_free(lwgeom);

	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_POINTER(result);
}

/** transform input geometry to a multi* type */
PG_FUNCTION_INFO_V1(LWGEOM_force_multi);
Datum LWGEOM_force_multi(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *result;
	LWGEOM *lwgeom;
	LWGEOM *ogeom;

	POSTGIS_DEBUG(2, "LWGEOM_force_multi called");

	/*
	** This funx is a no-op only if a bbox cache is already present
	** in input. If bbox cache is not there we'll need to handle
	** automatic bbox addition FOR_COMPLEX_GEOMS.
	*/
	if (gserialized_has_bbox(geom))
	{
		switch (gserialized_get_type(geom))
		{
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case COLLECTIONTYPE:
		case MULTICURVETYPE:
		case MULTISURFACETYPE:
		case TINTYPE:
			PG_RETURN_POINTER(geom);
		default:
			break;
		}
	}

	/* deserialize into lwgeoms[0] */
	lwgeom = lwgeom_from_gserialized(geom);
	ogeom = lwgeom_as_multi(lwgeom);

	result = geometry_serialize(ogeom);

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(result);
}

/** transform input geometry to a curved type */
PG_FUNCTION_INFO_V1(LWGEOM_force_curve);
Datum LWGEOM_force_curve(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *result;
	LWGEOM *lwgeom;
	LWGEOM *ogeom;

	POSTGIS_DEBUG(2, "LWGEOM_force_curve called");

	/* TODO: early out if input is already a curve */

	lwgeom = lwgeom_from_gserialized(geom);
	ogeom = lwgeom_as_curve(lwgeom);

	result = geometry_serialize(ogeom);

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(result);
}

/** transform input geometry to a SFS 1.1 geometry type compliant */
PG_FUNCTION_INFO_V1(LWGEOM_force_sfs);
Datum LWGEOM_force_sfs(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *result;
	LWGEOM *lwgeom;
	LWGEOM *ogeom;
	text *ver;
	int version = 110; /* default version is SFS 1.1 */

	POSTGIS_DEBUG(2, "LWGEOM_force_sfs called");

	/* If user specified version, respect it */
	if ((PG_NARGS() > 1) && (!PG_ARGISNULL(1)))
	{
		ver = PG_GETARG_TEXT_P(1);

		if (!strncmp(VARDATA(ver), "1.2", 3))
		{
			version = 120;
		}
	}

	lwgeom = lwgeom_from_gserialized(geom);
	ogeom = lwgeom_force_sfs(lwgeom, version);

	result = geometry_serialize(ogeom);

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(result);
}

/**
Returns the point in first input geometry that is closest to the second input geometry in 2d
*/

PG_FUNCTION_INFO_V1(LWGEOM_closestpoint);
Datum LWGEOM_closestpoint(PG_FUNCTION_ARGS)
{
	GSERIALIZED *result;
	GSERIALIZED *geom1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *geom2 = PG_GETARG_GSERIALIZED_P(1);
	LWGEOM *point;
	LWGEOM *lwgeom1 = lwgeom_from_gserialized(geom1);
	LWGEOM *lwgeom2 = lwgeom_from_gserialized(geom2);
	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	point = lwgeom_closest_point(lwgeom1, lwgeom2);

	if (lwgeom_is_empty(point))
		PG_RETURN_NULL();

	result = geometry_serialize(point);
	lwgeom_free(point);
	lwgeom_free(lwgeom1);
	lwgeom_free(lwgeom2);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);
	PG_RETURN_POINTER(result);
}

/**
Returns the shortest 2d line between two geometries
*/
PG_FUNCTION_INFO_V1(LWGEOM_shortestline2d);
Datum LWGEOM_shortestline2d(PG_FUNCTION_ARGS)
{
	GSERIALIZED *result;
	GSERIALIZED *geom1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *geom2 = PG_GETARG_GSERIALIZED_P(1);
	LWGEOM *theline;
	LWGEOM *lwgeom1 = lwgeom_from_gserialized(geom1);
	LWGEOM *lwgeom2 = lwgeom_from_gserialized(geom2);
	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	theline = lwgeom_closest_line(lwgeom1, lwgeom2);

	if (lwgeom_is_empty(theline))
		PG_RETURN_NULL();

	result = geometry_serialize(theline);
	lwgeom_free(theline);
	lwgeom_free(lwgeom1);
	lwgeom_free(lwgeom2);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);
	PG_RETURN_POINTER(result);
}

/**
Returns the longest 2d line between two geometries
*/
PG_FUNCTION_INFO_V1(LWGEOM_longestline2d);
Datum LWGEOM_longestline2d(PG_FUNCTION_ARGS)
{
	GSERIALIZED *result;
	GSERIALIZED *geom1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *geom2 = PG_GETARG_GSERIALIZED_P(1);
	LWGEOM *theline;
	LWGEOM *lwgeom1 = lwgeom_from_gserialized(geom1);
	LWGEOM *lwgeom2 = lwgeom_from_gserialized(geom2);
	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	theline = lwgeom_furthest_line(lwgeom1, lwgeom2);

	if (lwgeom_is_empty(theline))
		PG_RETURN_NULL();

	result = geometry_serialize(theline);
	lwgeom_free(theline);
	lwgeom_free(lwgeom1);
	lwgeom_free(lwgeom2);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);
	PG_RETURN_POINTER(result);
}
/**
 Minimum 2d distance between objects in geom1 and geom2.
 */
PG_FUNCTION_INFO_V1(ST_Distance);
Datum ST_Distance(PG_FUNCTION_ARGS)
{
	double mindist;
	GSERIALIZED *geom1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *geom2 = PG_GETARG_GSERIALIZED_P(1);
	LWGEOM *lwgeom1 = lwgeom_from_gserialized(geom1);
	LWGEOM *lwgeom2 = lwgeom_from_gserialized(geom2);
	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	mindist = lwgeom_mindistance2d(lwgeom1, lwgeom2);

	lwgeom_free(lwgeom1);
	lwgeom_free(lwgeom2);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	/* if called with empty geometries the ingoing mindistance is untouched, and makes us return NULL*/
	if (mindist < FLT_MAX)
		PG_RETURN_FLOAT8(mindist);

	PG_RETURN_NULL();
}

/**
Returns boolean describing if
mininimum 2d distance between objects in
geom1 and geom2 is shorter than tolerance
*/
PG_FUNCTION_INFO_V1(LWGEOM_dwithin);
Datum LWGEOM_dwithin(PG_FUNCTION_ARGS)
{
	double mindist;
	GSERIALIZED *geom1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *geom2 = PG_GETARG_GSERIALIZED_P(1);
	double tolerance = PG_GETARG_FLOAT8(2);
	LWGEOM *lwgeom1 = lwgeom_from_gserialized(geom1);
	LWGEOM *lwgeom2 = lwgeom_from_gserialized(geom2);

	if (tolerance < 0)
	{
		elog(ERROR, "Tolerance cannot be less than zero\n");
		PG_RETURN_NULL();
	}

	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	mindist = lwgeom_mindistance2d_tolerance(lwgeom1, lwgeom2, tolerance);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);
	/*empty geometries cases should be right handled since return from underlying
	 functions should be FLT_MAX which causes false as answer*/
	PG_RETURN_BOOL(tolerance >= mindist);
}

/**
Returns boolean describing if
maximum 2d distance between objects in
geom1 and geom2 is shorter than tolerance
*/
PG_FUNCTION_INFO_V1(LWGEOM_dfullywithin);
Datum LWGEOM_dfullywithin(PG_FUNCTION_ARGS)
{
	double maxdist;
	GSERIALIZED *geom1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *geom2 = PG_GETARG_GSERIALIZED_P(1);
	double tolerance = PG_GETARG_FLOAT8(2);
	LWGEOM *lwgeom1 = lwgeom_from_gserialized(geom1);
	LWGEOM *lwgeom2 = lwgeom_from_gserialized(geom2);

	if (tolerance < 0)
	{
		elog(ERROR, "Tolerance cannot be less than zero\n");
		PG_RETURN_NULL();
	}

	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	maxdist = lwgeom_maxdistance2d_tolerance(lwgeom1, lwgeom2, tolerance);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	/*If function is feed with empty geometries we should return false*/
	if (maxdist > -1)
		PG_RETURN_BOOL(tolerance >= maxdist);

	PG_RETURN_BOOL(LW_FALSE);
}

/**
 Maximum 2d distance between objects in geom1 and geom2.
 */
PG_FUNCTION_INFO_V1(LWGEOM_maxdistance2d_linestring);
Datum LWGEOM_maxdistance2d_linestring(PG_FUNCTION_ARGS)
{
	double maxdist;
	GSERIALIZED *geom1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *geom2 = PG_GETARG_GSERIALIZED_P(1);
	LWGEOM *lwgeom1 = lwgeom_from_gserialized(geom1);
	LWGEOM *lwgeom2 = lwgeom_from_gserialized(geom2);
	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	maxdist = lwgeom_maxdistance2d(lwgeom1, lwgeom2);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	/*if called with empty geometries the ingoing mindistance is untouched, and makes us return NULL*/
	if (maxdist > -1)
		PG_RETURN_FLOAT8(maxdist);

	PG_RETURN_NULL();
}

/**
Returns the point in first input geometry that is closest to the second input geometry in 3D
*/

PG_FUNCTION_INFO_V1(LWGEOM_closestpoint3d);
Datum LWGEOM_closestpoint3d(PG_FUNCTION_ARGS)
{
	GSERIALIZED *result;
	GSERIALIZED *geom1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *geom2 = PG_GETARG_GSERIALIZED_P(1);
	LWGEOM *point;
	LWGEOM *lwgeom1 = lwgeom_from_gserialized(geom1);
	LWGEOM *lwgeom2 = lwgeom_from_gserialized(geom2);
	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	point = lwgeom_closest_point_3d(lwgeom1, lwgeom2);
	// point = lw_dist3d_distancepoint(lwgeom1, lwgeom2, lwgeom1->srid, DIST_MIN);

	if (lwgeom_is_empty(point))
		PG_RETURN_NULL();

	result = geometry_serialize(point);

	lwgeom_free(point);
	lwgeom_free(lwgeom1);
	lwgeom_free(lwgeom2);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);
	PG_RETURN_POINTER(result);
}

/**
Returns the shortest line between two geometries in 3D
*/
PG_FUNCTION_INFO_V1(LWGEOM_shortestline3d);
Datum LWGEOM_shortestline3d(PG_FUNCTION_ARGS)
{
	GSERIALIZED *result;
	GSERIALIZED *geom1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *geom2 = PG_GETARG_GSERIALIZED_P(1);
	LWGEOM *theline;
	LWGEOM *lwgeom1 = lwgeom_from_gserialized(geom1);
	LWGEOM *lwgeom2 = lwgeom_from_gserialized(geom2);
	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	theline = lwgeom_closest_line_3d(lwgeom1, lwgeom2);
	// theline = lw_dist3d_distanceline(lwgeom1, lwgeom2, lwgeom1->srid, DIST_MIN);

	if (lwgeom_is_empty(theline))
		PG_RETURN_NULL();

	result = geometry_serialize(theline);

	lwgeom_free(theline);
	lwgeom_free(lwgeom1);
	lwgeom_free(lwgeom2);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);
	PG_RETURN_POINTER(result);
}

/**
Returns the longest line between two geometries in 3D
*/
PG_FUNCTION_INFO_V1(LWGEOM_longestline3d);
Datum LWGEOM_longestline3d(PG_FUNCTION_ARGS)
{
	GSERIALIZED *result;
	GSERIALIZED *geom1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *geom2 = PG_GETARG_GSERIALIZED_P(1);
	LWGEOM *theline;
	LWGEOM *lwgeom1 = lwgeom_from_gserialized(geom1);
	LWGEOM *lwgeom2 = lwgeom_from_gserialized(geom2);
	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	theline = lwgeom_furthest_line_3d(lwgeom1, lwgeom2);
	// theline = lw_dist3d_distanceline(lwgeom1, lwgeom2, lwgeom1->srid, DIST_MAX);

	if (lwgeom_is_empty(theline))
		PG_RETURN_NULL();

	result = geometry_serialize(theline);

	lwgeom_free(theline);
	lwgeom_free(lwgeom1);
	lwgeom_free(lwgeom2);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);
	PG_RETURN_POINTER(result);
}
/**
 Minimum 2d distance between objects in geom1 and geom2 in 3D
 */
PG_FUNCTION_INFO_V1(ST_3DDistance);
Datum ST_3DDistance(PG_FUNCTION_ARGS)
{
	double mindist;
	GSERIALIZED *geom1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *geom2 = PG_GETARG_GSERIALIZED_P(1);
	LWGEOM *lwgeom1 = lwgeom_from_gserialized(geom1);
	LWGEOM *lwgeom2 = lwgeom_from_gserialized(geom2);
	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	mindist = lwgeom_mindistance3d(lwgeom1, lwgeom2);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	/*if called with empty geometries the ingoing mindistance is untouched, and makes us return NULL*/
	if (mindist < FLT_MAX)
		PG_RETURN_FLOAT8(mindist);

	PG_RETURN_NULL();
}

/* intersects3d through dwithin */
PG_FUNCTION_INFO_V1(ST_3DIntersects);
Datum ST_3DIntersects(PG_FUNCTION_ARGS)
{
	double mindist;
	GSERIALIZED *geom1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *geom2 = PG_GETARG_GSERIALIZED_P(1);
	LWGEOM *lwgeom1 = lwgeom_from_gserialized(geom1);
	LWGEOM *lwgeom2 = lwgeom_from_gserialized(geom2);
	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	mindist = lwgeom_mindistance3d_tolerance(lwgeom1, lwgeom2, 0.0);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);
	/*empty geometries cases should be right handled since return from underlying
	  functions should be FLT_MAX which causes false as answer*/
	PG_RETURN_BOOL(0.0 == mindist);
}


/**
Returns boolean describing if
mininimum 3d distance between objects in
geom1 and geom2 is shorter than tolerance
*/
PG_FUNCTION_INFO_V1(LWGEOM_dwithin3d);
Datum LWGEOM_dwithin3d(PG_FUNCTION_ARGS)
{
	double mindist;
	GSERIALIZED *geom1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *geom2 = PG_GETARG_GSERIALIZED_P(1);
	double tolerance = PG_GETARG_FLOAT8(2);
	LWGEOM *lwgeom1 = lwgeom_from_gserialized(geom1);
	LWGEOM *lwgeom2 = lwgeom_from_gserialized(geom2);

	if (tolerance < 0)
	{
		elog(ERROR, "Tolerance cannot be less than zero\n");
		PG_RETURN_NULL();
	}

	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	mindist = lwgeom_mindistance3d_tolerance(lwgeom1, lwgeom2, tolerance);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	/*empty geometries cases should be right handled since return from underlying
	 functions should be FLT_MAX which causes false as answer*/
	PG_RETURN_BOOL(tolerance >= mindist);
}

/**
Returns boolean describing if
maximum 3d distance between objects in
geom1 and geom2 is shorter than tolerance
*/
PG_FUNCTION_INFO_V1(LWGEOM_dfullywithin3d);
Datum LWGEOM_dfullywithin3d(PG_FUNCTION_ARGS)
{
	double maxdist;
	GSERIALIZED *geom1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *geom2 = PG_GETARG_GSERIALIZED_P(1);
	double tolerance = PG_GETARG_FLOAT8(2);
	LWGEOM *lwgeom1 = lwgeom_from_gserialized(geom1);
	LWGEOM *lwgeom2 = lwgeom_from_gserialized(geom2);

	if (tolerance < 0)
	{
		elog(ERROR, "Tolerance cannot be less than zero\n");
		PG_RETURN_NULL();
	}

	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);
	maxdist = lwgeom_maxdistance3d_tolerance(lwgeom1, lwgeom2, tolerance);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	/*If function is feed with empty geometries we should return false*/
	if (maxdist > -1)
		PG_RETURN_BOOL(tolerance >= maxdist);

	PG_RETURN_BOOL(LW_FALSE);
}

/**
 Maximum 3d distance between objects in geom1 and geom2.
 */
PG_FUNCTION_INFO_V1(LWGEOM_maxdistance3d);
Datum LWGEOM_maxdistance3d(PG_FUNCTION_ARGS)
{
	double maxdist;
	GSERIALIZED *geom1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *geom2 = PG_GETARG_GSERIALIZED_P(1);
	LWGEOM *lwgeom1 = lwgeom_from_gserialized(geom1);
	LWGEOM *lwgeom2 = lwgeom_from_gserialized(geom2);

	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	maxdist = lwgeom_maxdistance3d(lwgeom1, lwgeom2);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	/*if called with empty geometries the ingoing mindistance is untouched, and makes us return NULL*/
	if (maxdist > -1)
		PG_RETURN_FLOAT8(maxdist);

	PG_RETURN_NULL();
}

PG_FUNCTION_INFO_V1(LWGEOM_longitude_shift);
Datum LWGEOM_longitude_shift(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom;
	LWGEOM *lwgeom;
	GSERIALIZED *ret;

	POSTGIS_DEBUG(2, "LWGEOM_longitude_shift called.");

	geom = PG_GETARG_GSERIALIZED_P_COPY(0);
	lwgeom = lwgeom_from_gserialized(geom);

	/* Drop bbox, will be recomputed */
	lwgeom_drop_bbox(lwgeom);

	/* Modify geometry */
	lwgeom_longitude_shift(lwgeom);

	/* Construct GSERIALIZED */
	ret = geometry_serialize(lwgeom);

	/* Release deserialized geometry */
	lwgeom_free(lwgeom);

	/* Release detoasted geometry */
	pfree(geom);

	PG_RETURN_POINTER(ret);
}

PG_FUNCTION_INFO_V1(ST_WrapX);
Datum ST_WrapX(PG_FUNCTION_ARGS)
{
	Datum gdatum;
	GSERIALIZED *geom_in;
	LWGEOM *lwgeom_in, *lwgeom_out;
	GSERIALIZED *geom_out;
	double cutx;
	double amount;

	POSTGIS_DEBUG(2, "ST_WrapX called.");

	gdatum = PG_GETARG_DATUM(0);
	cutx = PG_GETARG_FLOAT8(1);
	amount = PG_GETARG_FLOAT8(2);

	// if ( ! amount ) PG_RETURN_DATUM(gdatum);

	geom_in = ((GSERIALIZED *)PG_DETOAST_DATUM(gdatum));
	lwgeom_in = lwgeom_from_gserialized(geom_in);

	lwgeom_out = lwgeom_wrapx(lwgeom_in, cutx, amount);
	geom_out = geometry_serialize(lwgeom_out);

	lwgeom_free(lwgeom_in);
	lwgeom_free(lwgeom_out);
	PG_FREE_IF_COPY(geom_in, 0);

	PG_RETURN_POINTER(geom_out);
}

PG_FUNCTION_INFO_V1(LWGEOM_inside_circle_point);
Datum LWGEOM_inside_circle_point(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom;
	double cx = PG_GETARG_FLOAT8(1);
	double cy = PG_GETARG_FLOAT8(2);
	double rr = PG_GETARG_FLOAT8(3);
	LWPOINT *lwpoint;
	LWGEOM *lwgeom;
	int inside;

	geom = PG_GETARG_GSERIALIZED_P(0);
	lwgeom = lwgeom_from_gserialized(geom);
	lwpoint = lwgeom_as_lwpoint(lwgeom);
	if (lwpoint == NULL || lwgeom_is_empty(lwgeom))
	{
		PG_FREE_IF_COPY(geom, 0);
		PG_RETURN_NULL(); /* not a point */
	}

	inside = lwpoint_inside_circle(lwpoint, cx, cy, rr);
	lwpoint_free(lwpoint);

	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_BOOL(inside);
}

/**
 *  @brief collect( geom, geom ) returns a geometry which contains
 *  		all the sub_objects from both of the argument geometries
 *  @return geometry is the simplest possible, based on the types
 *  	of the collected objects
 *  	ie. if all are of either X or multiX, then a multiX is returned.
 */
PG_FUNCTION_INFO_V1(LWGEOM_collect);
Datum LWGEOM_collect(PG_FUNCTION_ARGS)
{
	GSERIALIZED *gser1, *gser2, *result;
	LWGEOM *lwgeoms[2], *outlwg;
	uint32 type1, type2;
	uint8_t outtype;
	int32_t srid;

	POSTGIS_DEBUG(2, "LWGEOM_collect called.");

	/* return null if both geoms are null */
	if (PG_ARGISNULL(0) && PG_ARGISNULL(1))
		PG_RETURN_NULL();

	/* Return the second geom if the first geom is null */
	if (PG_ARGISNULL(0))
		PG_RETURN_DATUM(PG_GETARG_DATUM(1));

	/* Return the first geom if the second geom is null */
	if (PG_ARGISNULL(1))
		PG_RETURN_DATUM(PG_GETARG_DATUM(0));

	gser1 = PG_GETARG_GSERIALIZED_P(0);
	gser2 = PG_GETARG_GSERIALIZED_P(1);
	gserialized_error_if_srid_mismatch(gser1, gser2, __func__);

	POSTGIS_DEBUGF(3,
		       "LWGEOM_collect(%s, %s): call",
		       lwtype_name(gserialized_get_type(gser1)),
		       lwtype_name(gserialized_get_type(gser2)));

	if ((gserialized_has_z(gser1) != gserialized_has_z(gser2)) ||
		(gserialized_has_m(gser1) != gserialized_has_m(gser2)))
	{
		elog(ERROR, "Cannot ST_Collect geometries with differing dimensionality.");
		PG_RETURN_NULL();
	}

	srid = gserialized_get_srid(gser1);

	lwgeoms[0] = lwgeom_from_gserialized(gser1);
	lwgeoms[1] = lwgeom_from_gserialized(gser2);

	type1 = lwgeoms[0]->type;
	type2 = lwgeoms[1]->type;

	if ((type1 == type2) && (!lwgeom_is_collection(lwgeoms[0])))
		outtype = lwtype_get_collectiontype(type1);
	else
		outtype = COLLECTIONTYPE;

	POSTGIS_DEBUGF(3, " outtype = %d", outtype);

	/* Drop input geometries bbox and SRID */
	lwgeom_drop_bbox(lwgeoms[0]);
	lwgeom_drop_srid(lwgeoms[0]);
	lwgeom_drop_bbox(lwgeoms[1]);
	lwgeom_drop_srid(lwgeoms[1]);

	outlwg = (LWGEOM *)lwcollection_construct(outtype, srid, NULL, 2, lwgeoms);
	result = geometry_serialize(outlwg);

	lwgeom_free(lwgeoms[0]);
	lwgeom_free(lwgeoms[1]);

	PG_FREE_IF_COPY(gser1, 0);
	PG_FREE_IF_COPY(gser2, 1);

	PG_RETURN_POINTER(result);
}

/**
 * @brief collect_garray ( GEOMETRY[] ) returns a geometry which contains
 * 		all the sub_objects from all of the geometries in given array.
 *
 * @return geometry is the simplest possible, based on the types
 * 		of the collected objects
 * 		ie. if all are of either X or multiX, then a multiX is returned
 * 		bboxonly types are treated as null geometries (no sub_objects)
 */
PG_FUNCTION_INFO_V1(LWGEOM_collect_garray);
Datum LWGEOM_collect_garray(PG_FUNCTION_ARGS)
{
	ArrayType *array;
	int nelems;
	/*GSERIALIZED **geoms; */
	GSERIALIZED *result = NULL;
	LWGEOM **lwgeoms, *outlwg;
	uint32 outtype;
	int count;
	int32_t srid = SRID_UNKNOWN;
	GBOX *box = NULL;

	ArrayIterator iterator;
	Datum value;
	bool isnull;

	POSTGIS_DEBUG(2, "LWGEOM_collect_garray called.");

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	/* Get actual ArrayType */
	array = PG_GETARG_ARRAYTYPE_P(0);
	nelems = ArrayGetNItems(ARR_NDIM(array), ARR_DIMS(array));

	POSTGIS_DEBUGF(3,
		       " array is %d-bytes in size, %ld w/out header",
		       ARR_SIZE(array),
		       ARR_SIZE(array) - ARR_OVERHEAD_NONULLS(ARR_NDIM(array)));

	POSTGIS_DEBUGF(3, "LWGEOM_collect_garray: array has %d elements", nelems);

	/* Return null on 0-elements input array */
	if (nelems == 0)
		PG_RETURN_NULL();

	/*
	 * Deserialize all geometries in array into the lwgeoms pointers
	 * array. Check input types to form output type.
	 */
	lwgeoms = palloc(sizeof(LWGEOM *) * nelems);
	count = 0;
	outtype = 0;

	iterator = array_create_iterator(array, 0, NULL);

	while (array_iterate(iterator, &value, &isnull))
	{
		GSERIALIZED *geom;
		uint8_t intype;

		/* Don't do anything for NULL values */
		if (isnull)
			continue;

		geom = (GSERIALIZED *)DatumGetPointer(value);
		intype = gserialized_get_type(geom);

		lwgeoms[count] = lwgeom_from_gserialized(geom);

		POSTGIS_DEBUGF(3, "%s: geom %d deserialized", __func__, count);

		if (!count)
		{
			/* Get first geometry SRID */
			srid = lwgeoms[count]->srid;

			/* COMPUTE_BBOX WHEN_SIMPLE */
			if (lwgeoms[count]->bbox)
				box = gbox_copy(lwgeoms[count]->bbox);
		}
		else
		{
			/* Check SRID homogeneity */
			gserialized_error_if_srid_mismatch_reference(geom, srid, __func__);

			/* COMPUTE_BBOX WHEN_SIMPLE */
			if (box)
			{
				if (lwgeoms[count]->bbox)
					gbox_merge(lwgeoms[count]->bbox, box);
				else
				{
					pfree(box);
					box = NULL;
				}
			}
		}

		lwgeom_drop_srid(lwgeoms[count]);
		lwgeom_drop_bbox(lwgeoms[count]);

		/* Output type not initialized */
		if (!outtype)
		{
			outtype = lwtype_get_collectiontype(intype);
		}
		/* Input type not compatible with output */
		/* make output type a collection */
		else if (outtype != COLLECTIONTYPE && lwtype_get_collectiontype(intype) != outtype)
		{
			outtype = COLLECTIONTYPE;
		}

		count++;
	}
	array_free_iterator(iterator);

	POSTGIS_DEBUGF(3, "LWGEOM_collect_garray: outtype = %d", outtype);

	/* If we have been passed a complete set of NULLs then return NULL */
	if (!outtype)
	{
		PG_RETURN_NULL();
	}
	else
	{
		outlwg = (LWGEOM *)lwcollection_construct(outtype, srid, box, count, lwgeoms);

		result = geometry_serialize(outlwg);

		PG_RETURN_POINTER(result);
	}
}

/**
 * LineFromMultiPoint ( GEOMETRY ) returns a LINE formed by
 * 		all the points in the in given multipoint.
 */
PG_FUNCTION_INFO_V1(LWGEOM_line_from_mpoint);
Datum LWGEOM_line_from_mpoint(PG_FUNCTION_ARGS)
{
	GSERIALIZED *ingeom, *result;
	LWLINE *lwline;
	LWMPOINT *mpoint;

	POSTGIS_DEBUG(2, "LWGEOM_line_from_mpoint called");

	/* Get input GSERIALIZED and deserialize it */
	ingeom = PG_GETARG_GSERIALIZED_P(0);

	if (gserialized_get_type(ingeom) != MULTIPOINTTYPE)
	{
		elog(ERROR, "makeline: input must be a multipoint");
		PG_RETURN_NULL(); /* input is not a multipoint */
	}

	mpoint = lwgeom_as_lwmpoint(lwgeom_from_gserialized(ingeom));
	lwline = lwline_from_lwmpoint(mpoint->srid, mpoint);
	if (!lwline)
	{
		PG_FREE_IF_COPY(ingeom, 0);
		elog(ERROR, "makeline: lwline_from_lwmpoint returned NULL");
		PG_RETURN_NULL();
	}

	result = geometry_serialize(lwline_as_lwgeom(lwline));

	PG_FREE_IF_COPY(ingeom, 0);
	lwline_free(lwline);

	PG_RETURN_POINTER(result);
}

/**
 * @brief makeline_garray ( GEOMETRY[] ) returns a LINE formed by
 * 		all the point geometries in given array.
 * 		array elements that are NOT points are discarded..
 */
PG_FUNCTION_INFO_V1(LWGEOM_makeline_garray);
Datum LWGEOM_makeline_garray(PG_FUNCTION_ARGS)
{
	ArrayType *array;
	int nelems;
	GSERIALIZED *result = NULL;
	LWGEOM **geoms;
	LWGEOM *outlwg;
	uint32 ngeoms;
	int32_t srid = SRID_UNKNOWN;

	ArrayIterator iterator;
	Datum value;
	bool isnull;

	POSTGIS_DEBUGF(2, "%s called", __func__);

	/* Return null on null input */
	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	/* Get actual ArrayType */
	array = PG_GETARG_ARRAYTYPE_P(0);

	/* Get number of geometries in array */
	nelems = ArrayGetNItems(ARR_NDIM(array), ARR_DIMS(array));

	POSTGIS_DEBUGF(3, "%s: array has %d elements", __func__, nelems);

	/* Return null on 0-elements input array */
	if (nelems == 0)
		PG_RETURN_NULL();

	/*
	 * Deserialize all point geometries in array into the
	 * geoms pointers array.
	 * Count actual number of points.
	 */

	/* possibly more then required */
	geoms = palloc(sizeof(LWGEOM *) * nelems);
	ngeoms = 0;

	iterator = array_create_iterator(array, 0, NULL);

	while (array_iterate(iterator, &value, &isnull))
	{
		GSERIALIZED *geom;

		if (isnull)
			continue;

		geom = (GSERIALIZED *)DatumGetPointer(value);

		if (gserialized_get_type(geom) != POINTTYPE && gserialized_get_type(geom) != LINETYPE &&
		    gserialized_get_type(geom) != MULTIPOINTTYPE)
		{
			continue;
		}

		geoms[ngeoms++] = lwgeom_from_gserialized(geom);

		/* Check SRID homogeneity */
		if (ngeoms == 1)
		{
			/* Get first geometry SRID */
			srid = geoms[ngeoms - 1]->srid;
			/* TODO: also get ZMflags */
		}
		else
			gserialized_error_if_srid_mismatch_reference(geom, srid, __func__);

		POSTGIS_DEBUGF(3, "%s: element %d deserialized", __func__, ngeoms);
	}
	array_free_iterator(iterator);

	/* Return null on 0-points input array */
	if (ngeoms == 0)
	{
		/* TODO: should we return LINESTRING EMPTY here ? */
		elog(NOTICE, "No points or linestrings in input array");
		PG_RETURN_NULL();
	}

	POSTGIS_DEBUGF(3, "LWGEOM_makeline_garray: elements: %d", ngeoms);

	outlwg = (LWGEOM *)lwline_from_lwgeom_array(srid, ngeoms, geoms);

	result = geometry_serialize(outlwg);

	PG_RETURN_POINTER(result);
}

/**
 * makeline ( GEOMETRY, GEOMETRY ) returns a LINESTRIN segment
 * formed by the given point geometries.
 */
PG_FUNCTION_INFO_V1(LWGEOM_makeline);
Datum LWGEOM_makeline(PG_FUNCTION_ARGS)
{
	GSERIALIZED *pglwg1, *pglwg2;
	GSERIALIZED *result = NULL;
	LWGEOM *lwgeoms[2];
	LWLINE *outline;

	POSTGIS_DEBUG(2, "LWGEOM_makeline called.");

	/* Get input datum */
	pglwg1 = PG_GETARG_GSERIALIZED_P(0);
	pglwg2 = PG_GETARG_GSERIALIZED_P(1);

	if ((gserialized_get_type(pglwg1) != POINTTYPE && gserialized_get_type(pglwg1) != LINETYPE) ||
	    (gserialized_get_type(pglwg2) != POINTTYPE && gserialized_get_type(pglwg2) != LINETYPE))
	{
		elog(ERROR, "Input geometries must be points or lines");
		PG_RETURN_NULL();
	}

	gserialized_error_if_srid_mismatch(pglwg1, pglwg2, __func__);

	lwgeoms[0] = lwgeom_from_gserialized(pglwg1);
	lwgeoms[1] = lwgeom_from_gserialized(pglwg2);

	outline = lwline_from_lwgeom_array(lwgeoms[0]->srid, 2, lwgeoms);

	result = geometry_serialize((LWGEOM *)outline);

	PG_FREE_IF_COPY(pglwg1, 0);
	PG_FREE_IF_COPY(pglwg2, 1);
	lwgeom_free(lwgeoms[0]);
	lwgeom_free(lwgeoms[1]);

	PG_RETURN_POINTER(result);
}

/**
 * makepoly( GEOMETRY, GEOMETRY[] ) returns a POLYGON
 * 		formed by the given shell and holes geometries.
 */
PG_FUNCTION_INFO_V1(LWGEOM_makepoly);
Datum LWGEOM_makepoly(PG_FUNCTION_ARGS)
{
	GSERIALIZED *pglwg1;
	ArrayType *array = NULL;
	GSERIALIZED *result = NULL;
	const LWLINE *shell = NULL;
	const LWLINE **holes = NULL;
	LWPOLY *outpoly;
	uint32 nholes = 0;
	uint32 i;
	size_t offset = 0;

	POSTGIS_DEBUG(2, "LWGEOM_makepoly called.");

	/* Get input shell */
	pglwg1 = PG_GETARG_GSERIALIZED_P(0);
	if (gserialized_get_type(pglwg1) != LINETYPE)
	{
		lwpgerror("Shell is not a line");
	}
	shell = lwgeom_as_lwline(lwgeom_from_gserialized(pglwg1));

	/* Get input holes if any */
	if (PG_NARGS() > 1)
	{
		array = PG_GETARG_ARRAYTYPE_P(1);
		nholes = ArrayGetNItems(ARR_NDIM(array), ARR_DIMS(array));
		holes = lwalloc(sizeof(LWLINE *) * nholes);
		for (i = 0; i < nholes; i++)
		{
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#endif
			GSERIALIZED *g = (GSERIALIZED *)(ARR_DATA_PTR(array) + offset);
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
			LWLINE *hole;
			offset += INTALIGN(VARSIZE(g));
			if (gserialized_get_type(g) != LINETYPE)
			{
				lwpgerror("Hole %d is not a line", i);
			}
			hole = lwgeom_as_lwline(lwgeom_from_gserialized(g));
			holes[i] = hole;
		}
	}

	outpoly = lwpoly_from_lwlines(shell, nholes, holes);
	POSTGIS_DEBUGF(3, "%s", lwgeom_summary((LWGEOM *)outpoly, 0));
	result = geometry_serialize((LWGEOM *)outpoly);

	lwline_free((LWLINE *)shell);
	PG_FREE_IF_COPY(pglwg1, 0);

	for (i = 0; i < nholes; i++)
	{
		lwline_free((LWLINE *)holes[i]);
	}

	PG_RETURN_POINTER(result);
}

/**
 *  makes a polygon of the expanded features bvol - 1st point = LL 3rd=UR
 *  2d only. (3d might be worth adding).
 *  create new geometry of type polygon, 1 ring, 5 points
 */
PG_FUNCTION_INFO_V1(LWGEOM_expand);
Datum LWGEOM_expand(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	LWGEOM *lwgeom = lwgeom_from_gserialized(geom);
	int32_t srid = lwgeom_get_srid(lwgeom);
	LWPOLY *poly;
	GSERIALIZED *result;
	GBOX gbox;

	POSTGIS_DEBUG(2, "LWGEOM_expand called.");

	/* Can't expand an empty */
	if (lwgeom_is_empty(lwgeom))
	{
		lwgeom_free(lwgeom);
		PG_RETURN_POINTER(geom);
	}

	/* Can't expand something with no gbox! */
	if (LW_FAILURE == lwgeom_calculate_gbox(lwgeom, &gbox))
	{
		lwgeom_free(lwgeom);
		PG_RETURN_POINTER(geom);
	}

	if (PG_NARGS() == 2)
	{
		/* Expand the box the same amount in all directions */
		double d = PG_GETARG_FLOAT8(1);
		gbox_expand(&gbox, d);
	}
	else
	{
		double dx = PG_GETARG_FLOAT8(1);
		double dy = PG_GETARG_FLOAT8(2);
		double dz = PG_GETARG_FLOAT8(3);
		double dm = PG_GETARG_FLOAT8(4);

		gbox_expand_xyzm(&gbox, dx, dy, dz, dm);
	}

	{
		POINT4D p1 = {gbox.xmin, gbox.ymin, gbox.zmin, gbox.mmin};
		POINT4D p2 = {gbox.xmin, gbox.ymax, gbox.zmin, gbox.mmin};
		POINT4D p3 = {gbox.xmax, gbox.ymax, gbox.zmax, gbox.mmax};
		POINT4D p4 = {gbox.xmax, gbox.ymin, gbox.zmax, gbox.mmax};

		poly = lwpoly_construct_rectangle(lwgeom_has_z(lwgeom), lwgeom_has_m(lwgeom), &p1, &p2, &p3, &p4);
	}

	lwgeom_add_bbox(lwpoly_as_lwgeom(poly));
	lwgeom_set_srid(lwpoly_as_lwgeom(poly), srid);

	/* Construct GSERIALIZED  */
	result = geometry_serialize(lwpoly_as_lwgeom(poly));

	lwgeom_free(lwpoly_as_lwgeom(poly));
	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(result);
}

/** Convert geometry to BOX (internal postgres type) */
PG_FUNCTION_INFO_V1(LWGEOM_to_BOX);
Datum LWGEOM_to_BOX(PG_FUNCTION_ARGS)
{
	GSERIALIZED *pg_lwgeom = PG_GETARG_GSERIALIZED_P(0);
	LWGEOM *lwgeom = lwgeom_from_gserialized(pg_lwgeom);
	GBOX gbox;
	int result;
	BOX *out = NULL;

	/* Zero out flags */
	gbox_init(&gbox);

	/* Calculate the GBOX of the geometry */
	result = lwgeom_calculate_gbox(lwgeom, &gbox);

	/* Clean up memory */
	lwfree(lwgeom);
	PG_FREE_IF_COPY(pg_lwgeom, 0);

	/* Null on failure */
	if (!result)
		PG_RETURN_NULL();

	out = lwalloc(sizeof(BOX));
	out->low.x = gbox.xmin;
	out->low.y = gbox.ymin;
	out->high.x = gbox.xmax;
	out->high.y = gbox.ymax;
	PG_RETURN_POINTER(out);
}

/**
 *  makes a polygon of the features bvol - 1st point = LL 3rd=UR
 *  2d only. (3d might be worth adding).
 *  create new geometry of type polygon, 1 ring, 5 points
 */
PG_FUNCTION_INFO_V1(LWGEOM_envelope);
Datum LWGEOM_envelope(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	LWGEOM *lwgeom = lwgeom_from_gserialized(geom);
	int32_t srid = lwgeom->srid;
	POINT4D pt;
	GBOX box;
	POINTARRAY *pa;
	GSERIALIZED *result;

	if (lwgeom_is_empty(lwgeom))
	{
		/* must be the EMPTY geometry */
		PG_RETURN_POINTER(geom);
	}

	if (lwgeom_calculate_gbox(lwgeom, &box) == LW_FAILURE)
	{
		/* must be the EMPTY geometry */
		PG_RETURN_POINTER(geom);
	}

	/*
	 * Alter envelope type so that a valid geometry is always
	 * returned depending upon the size of the geometry. The
	 * code makes the following assumptions:
	 *     - If the bounding box is a single point then return a
	 *     POINT geometry
	 *     - If the bounding box represents either a horizontal or
	 *     vertical line, return a LINESTRING geometry
	 *     - Otherwise return a POLYGON
	 */

	if ((box.xmin == box.xmax) && (box.ymin == box.ymax))
	{
		/* Construct and serialize point */
		LWPOINT *point = lwpoint_make2d(srid, box.xmin, box.ymin);
		result = geometry_serialize(lwpoint_as_lwgeom(point));
		lwpoint_free(point);
	}
	else if ((box.xmin == box.xmax) || (box.ymin == box.ymax))
	{
		LWLINE *line;
		/* Construct point array */
		pa = ptarray_construct_empty(0, 0, 2);

		/* Assign coordinates to POINT2D array */
		pt.x = box.xmin;
		pt.y = box.ymin;
		ptarray_append_point(pa, &pt, LW_TRUE);
		pt.x = box.xmax;
		pt.y = box.ymax;
		ptarray_append_point(pa, &pt, LW_TRUE);

		/* Construct and serialize linestring */
		line = lwline_construct(srid, NULL, pa);
		result = geometry_serialize(lwline_as_lwgeom(line));
		lwline_free(line);
	}
	else
	{
		LWPOLY *poly;
		POINTARRAY **ppa = lwalloc(sizeof(POINTARRAY *));
		pa = ptarray_construct_empty(0, 0, 5);
		ppa[0] = pa;

		/* Assign coordinates to POINT2D array */
		pt.x = box.xmin;
		pt.y = box.ymin;
		ptarray_append_point(pa, &pt, LW_TRUE);
		pt.x = box.xmin;
		pt.y = box.ymax;
		ptarray_append_point(pa, &pt, LW_TRUE);
		pt.x = box.xmax;
		pt.y = box.ymax;
		ptarray_append_point(pa, &pt, LW_TRUE);
		pt.x = box.xmax;
		pt.y = box.ymin;
		ptarray_append_point(pa, &pt, LW_TRUE);
		pt.x = box.xmin;
		pt.y = box.ymin;
		ptarray_append_point(pa, &pt, LW_TRUE);

		/* Construct polygon  */
		poly = lwpoly_construct(srid, NULL, 1, ppa);
		result = geometry_serialize(lwpoly_as_lwgeom(poly));
		lwpoly_free(poly);
	}

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(LWGEOM_isempty);
Datum LWGEOM_isempty(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	PG_RETURN_BOOL(gserialized_is_empty(geom));
}

/**
 *  @brief Returns a modified geometry so that no segment is
 *  	longer then the given distance (computed using 2d).
 *  	Every input point is kept.
 *  	Z and M values for added points (if needed) are set to 0.
 */
PG_FUNCTION_INFO_V1(LWGEOM_segmentize2d);
Datum LWGEOM_segmentize2d(PG_FUNCTION_ARGS)
{
	GSERIALIZED *outgeom, *ingeom;
	double dist;
	LWGEOM *inlwgeom, *outlwgeom;
	int type;

	POSTGIS_DEBUG(2, "LWGEOM_segmentize2d called");

	ingeom = PG_GETARG_GSERIALIZED_P(0);
	dist = PG_GETARG_FLOAT8(1);
	type = gserialized_get_type(ingeom);

	/* Avoid types we cannot segmentize. */
	if ((type == POINTTYPE) || (type == MULTIPOINTTYPE) || (type == TRIANGLETYPE) || (type == TINTYPE) ||
	    (type == POLYHEDRALSURFACETYPE))
	{
		PG_RETURN_POINTER(ingeom);
	}

	if (dist <= 0)
	{
		/* Protect from knowingly infinite loops, see #1799 */
		/* Note that we'll end out of memory anyway for other small distances */
		elog(ERROR, "ST_Segmentize: invalid max_distance %g (must be >= 0)", dist);
		PG_RETURN_NULL();
	}

	LWGEOM_INIT();

	inlwgeom = lwgeom_from_gserialized(ingeom);
	if (lwgeom_is_empty(inlwgeom))
	{
		/* Should only happen on interruption */
		lwgeom_free(inlwgeom);
		PG_RETURN_POINTER(ingeom);
	}

	outlwgeom = lwgeom_segmentize2d(inlwgeom, dist);
	if (!outlwgeom)
	{
		/* Should only happen on interruption */
		PG_FREE_IF_COPY(ingeom, 0);
		PG_RETURN_NULL();
	}

	/* Copy input bounding box if any */
	if (inlwgeom->bbox)
		outlwgeom->bbox = gbox_copy(inlwgeom->bbox);

	outgeom = geometry_serialize(outlwgeom);

	// lwgeom_free(outlwgeom); /* TODO fix lwgeom_clone / ptarray_clone_deep for consistent semantics */
	lwgeom_free(inlwgeom);

	PG_FREE_IF_COPY(ingeom, 0);

	PG_RETURN_POINTER(outgeom);
}

/** Reverse vertex order of geometry */
PG_FUNCTION_INFO_V1(LWGEOM_reverse);
Datum LWGEOM_reverse(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom;
	LWGEOM *lwgeom;

	POSTGIS_DEBUG(2, "LWGEOM_reverse called");

	geom = PG_GETARG_GSERIALIZED_P_COPY(0);

	lwgeom = lwgeom_from_gserialized(geom);
	lwgeom_reverse_in_place(lwgeom);

	geom = geometry_serialize(lwgeom);

	PG_RETURN_POINTER(geom);
}

/** Force polygons of the collection to obey Right-Hand-Rule */
PG_FUNCTION_INFO_V1(LWGEOM_force_clockwise_poly);
Datum LWGEOM_force_clockwise_poly(PG_FUNCTION_ARGS)
{
	GSERIALIZED *ingeom, *outgeom;
	LWGEOM *lwgeom;

	POSTGIS_DEBUG(2, "LWGEOM_force_clockwise_poly called");

	ingeom = PG_GETARG_GSERIALIZED_P_COPY(0);

	lwgeom = lwgeom_from_gserialized(ingeom);
	lwgeom_force_clockwise(lwgeom);

	outgeom = geometry_serialize(lwgeom);

	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(ingeom, 0);
	PG_RETURN_POINTER(outgeom);
}

/** Test deserialize/serialize operations */
PG_FUNCTION_INFO_V1(LWGEOM_noop);
Datum LWGEOM_noop(PG_FUNCTION_ARGS)
{
	GSERIALIZED *in = PG_GETARG_GSERIALIZED_P(0);
	LWGEOM *lwgeom = lwgeom_from_gserialized(in);
	GSERIALIZED *out = geometry_serialize(lwgeom);
	PG_RETURN_POINTER(out);
}

Datum ST_Normalize(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_Normalize);
Datum ST_Normalize(PG_FUNCTION_ARGS)
{
	GSERIALIZED *in, *out;
	LWGEOM *lwgeom_in, *lwgeom_out;

	POSTGIS_DEBUG(2, "ST_Normalize called");

	in = PG_GETARG_GSERIALIZED_P_COPY(0);

	lwgeom_in = lwgeom_from_gserialized(in);
	POSTGIS_DEBUGF(3, "Deserialized: %s", lwgeom_summary(lwgeom_in, 0));

	lwgeom_out = lwgeom_normalize(lwgeom_in);
	POSTGIS_DEBUGF(3, "Normalized: %s", lwgeom_summary(lwgeom_out, 0));

	out = geometry_serialize(lwgeom_out);
	lwgeom_free(lwgeom_in);
	lwgeom_free(lwgeom_out);

	PG_FREE_IF_COPY(in, 0);

	PG_RETURN_POINTER(out);
}

/**
 *  @return:
 *   0==2d
 *   1==3dm
 *   2==3dz
 *   3==4d
 */
PG_FUNCTION_INFO_V1(LWGEOM_zmflag);
Datum LWGEOM_zmflag(PG_FUNCTION_ARGS)
{
	GSERIALIZED *in = PG_GETARG_GSERIALIZED_HEADER(0);
	int ret = 0;

	if (gserialized_has_z(in))
		ret += 2;
	if (gserialized_has_m(in))
		ret += 1;
	PG_FREE_IF_COPY(in, 0);
	PG_RETURN_INT16(ret);
}

PG_FUNCTION_INFO_V1(LWGEOM_hasz);
Datum LWGEOM_hasz(PG_FUNCTION_ARGS)
{
	GSERIALIZED *in = PG_GETARG_GSERIALIZED_HEADER(0);
	PG_RETURN_BOOL(gserialized_has_z(in));
}

PG_FUNCTION_INFO_V1(LWGEOM_hasm);
Datum LWGEOM_hasm(PG_FUNCTION_ARGS)
{
	GSERIALIZED *in = PG_GETARG_GSERIALIZED_HEADER(0);
	PG_RETURN_BOOL(gserialized_has_m(in));
}

PG_FUNCTION_INFO_V1(LWGEOM_hasBBOX);
Datum LWGEOM_hasBBOX(PG_FUNCTION_ARGS)
{
	GSERIALIZED *in = PG_GETARG_GSERIALIZED_HEADER(0);
	char res = gserialized_has_bbox(in);
	PG_FREE_IF_COPY(in, 0);
	PG_RETURN_BOOL(res);
}

/** Return: 2,3 or 4 */
PG_FUNCTION_INFO_V1(LWGEOM_ndims);
Datum LWGEOM_ndims(PG_FUNCTION_ARGS)
{
	GSERIALIZED *in = PG_GETARG_GSERIALIZED_HEADER(0);
	int ret = gserialized_ndims(in);
	PG_FREE_IF_COPY(in, 0);
	PG_RETURN_INT16(ret);
}

/** lwgeom_same(lwgeom1, lwgeom2) */
PG_FUNCTION_INFO_V1(LWGEOM_same);
Datum LWGEOM_same(PG_FUNCTION_ARGS)
{
	GSERIALIZED *g1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *g2 = PG_GETARG_GSERIALIZED_P(1);

	PG_RETURN_BOOL(gserialized_cmp(g1, g2) == 0);
}

PG_FUNCTION_INFO_V1(ST_MakeEnvelope);
Datum ST_MakeEnvelope(PG_FUNCTION_ARGS)
{
	LWPOLY *poly;
	GSERIALIZED *result;
	double x1, y1, x2, y2;
	int32_t srid = SRID_UNKNOWN;

	POSTGIS_DEBUG(2, "ST_MakeEnvelope called");

	x1 = PG_GETARG_FLOAT8(0);
	y1 = PG_GETARG_FLOAT8(1);
	x2 = PG_GETARG_FLOAT8(2);
	y2 = PG_GETARG_FLOAT8(3);
	if (PG_NARGS() > 4)
	{
		srid = PG_GETARG_INT32(4);
	}

	poly = lwpoly_construct_envelope(srid, x1, y1, x2, y2);

	result = geometry_serialize(lwpoly_as_lwgeom(poly));
	lwpoly_free(poly);

	PG_RETURN_POINTER(result);
}


PG_FUNCTION_INFO_V1(ST_TileEnvelope);
Datum ST_TileEnvelope(PG_FUNCTION_ARGS)
{
	GSERIALIZED *bounds;
	uint32_t zoomu;
	int32_t x, y, zoom;
	uint32_t worldTileSize;
	double tileGeoSizeX, tileGeoSizeY;
	double boundsWidth, boundsHeight;
	double x1, y1, x2, y2;
	double margin;
	/* This is broken, since 3857 doesn't mean "web mercator", it means
	   the contents of the row in spatial_ref_sys with srid = 3857.
	   For practical purposes this will work, but in good implementation
	   we should de-reference in spatial ref sys to confirm that the
	   srid of the object is EPSG:3857. */
	int32_t srid;
	GBOX bbox;
	LWGEOM *g = NULL;

	POSTGIS_DEBUG(2, "ST_TileEnvelope called");

	zoom = PG_GETARG_INT32(0);
	x = PG_GETARG_INT32(1);
	y = PG_GETARG_INT32(2);

	bounds = PG_GETARG_GSERIALIZED_P(3);
	/*
	 * We deserialize the geometry and recalculate the bounding box here to get
	 * 64b floating point precision. The serialized bbox has 32b float is not
	 * precise enough with big numbers such as the ones used in the default
	 * parameters, e.g: -20037508.3427892 is transformed into -20037510
	 */
	g = lwgeom_from_gserialized(bounds);
	if (lwgeom_calculate_gbox(g, &bbox) != LW_SUCCESS)
		elog(ERROR, "%s: Unable to compute bbox", __func__);
	srid = g->srid;
	lwgeom_free(g);

	/* Avoid crashing with old signature (old sql code with 3 args, new C code with 4) */
	margin = PG_NARGS() < 4 ? 0 : PG_GETARG_FLOAT8(4);
	/* shrinking by more than 50% would eliminate the tile outright */
	if (margin < -0.5)
		elog(ERROR, "%s: Margin must not be less than -50%%, margin=%f", __func__, margin);

	boundsWidth  = bbox.xmax - bbox.xmin;
	boundsHeight = bbox.ymax - bbox.ymin;
	if (boundsWidth <= 0 || boundsHeight <= 0)
		elog(ERROR, "%s: Geometric bounds are too small", __func__);

	if (zoom < 0 || zoom >= 32)
		elog(ERROR, "%s: Invalid tile zoom value, %d", __func__, zoom);

	zoomu = (uint32_t)zoom;
	worldTileSize = 0x01u << (zoomu > 31 ? 31 : zoomu);

	if (x < 0 || (uint32_t)x >= worldTileSize)
		elog(ERROR, "%s: Invalid tile x value, %d", __func__, x);
	if (y < 0 || (uint32_t)y >= worldTileSize)
		elog(ERROR, "%s: Invalid tile y value, %d", __func__, y);

	tileGeoSizeX = boundsWidth / worldTileSize;
	tileGeoSizeY = boundsHeight / worldTileSize;

	/*
	 * 1 margin (100%) is the same as a single tile width
	 * if the size of the tile with margins span more than the total number of tiles,
	 * reset x1/x2 to the bounds
	 */
	if ((1 + margin * 2) > worldTileSize)
	{
		x1 = bbox.xmin;
		x2 = bbox.xmax;
	}
	else
	{
		x1 = bbox.xmin + tileGeoSizeX * (x - margin);
		x2 = bbox.xmin + tileGeoSizeX * (x + 1 + margin);
	}

	y1 = bbox.ymax - tileGeoSizeY * (y + 1 + margin);
	y2 = bbox.ymax - tileGeoSizeY * (y - margin);

	/* Clip y-axis to the given bounds */
	if (y1 < bbox.ymin) y1 = bbox.ymin;
	if (y2 > bbox.ymax) y2 = bbox.ymax;

	PG_RETURN_POINTER(
		geometry_serialize(
		lwpoly_as_lwgeom(
		lwpoly_construct_envelope(
			srid, x1, y1, x2, y2))));
}


PG_FUNCTION_INFO_V1(ST_IsCollection);
Datum ST_IsCollection(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_HEADER(0);
	int type = gserialized_get_type(geom);
	PG_RETURN_BOOL(lwtype_is_collection(type));
}

PG_FUNCTION_INFO_V1(LWGEOM_makepoint);
Datum LWGEOM_makepoint(PG_FUNCTION_ARGS)
{
	double x, y, z, m;
	LWPOINT *point;
	GSERIALIZED *result;

	POSTGIS_DEBUG(2, "LWGEOM_makepoint called");

	x = PG_GETARG_FLOAT8(0);
	y = PG_GETARG_FLOAT8(1);

	if (PG_NARGS() == 2)
		point = lwpoint_make2d(SRID_UNKNOWN, x, y);
	else if (PG_NARGS() == 3)
	{
		z = PG_GETARG_FLOAT8(2);
		point = lwpoint_make3dz(SRID_UNKNOWN, x, y, z);
	}
	else if (PG_NARGS() == 4)
	{
		z = PG_GETARG_FLOAT8(2);
		m = PG_GETARG_FLOAT8(3);
		point = lwpoint_make4d(SRID_UNKNOWN, x, y, z, m);
	}
	else
	{
		elog(ERROR, "LWGEOM_makepoint: unsupported number of args: %d", PG_NARGS());
		PG_RETURN_NULL();
	}

	result = geometry_serialize((LWGEOM *)point);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(LWGEOM_makepoint3dm);
Datum LWGEOM_makepoint3dm(PG_FUNCTION_ARGS)
{
	double x, y, m;
	LWPOINT *point;
	GSERIALIZED *result;

	POSTGIS_DEBUG(2, "LWGEOM_makepoint3dm called.");

	x = PG_GETARG_FLOAT8(0);
	y = PG_GETARG_FLOAT8(1);
	m = PG_GETARG_FLOAT8(2);

	point = lwpoint_make3dm(SRID_UNKNOWN, x, y, m);
	result = geometry_serialize((LWGEOM *)point);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(LWGEOM_addpoint);
Datum LWGEOM_addpoint(PG_FUNCTION_ARGS)
{
	GSERIALIZED *pglwg1, *pglwg2, *result;
	LWPOINT *point;
	LWLINE *line, *linecopy;
	uint32_t uwhere = 0;

	POSTGIS_DEBUGF(2, "%s called.", __func__);

	pglwg1 = PG_GETARG_GSERIALIZED_P(0);
	pglwg2 = PG_GETARG_GSERIALIZED_P(1);

	if (gserialized_get_type(pglwg1) != LINETYPE)
	{
		elog(ERROR, "First argument must be a LINESTRING");
		PG_RETURN_NULL();
	}

	if (gserialized_get_type(pglwg2) != POINTTYPE)
	{
		elog(ERROR, "Second argument must be a POINT");
		PG_RETURN_NULL();
	}

	line = lwgeom_as_lwline(lwgeom_from_gserialized(pglwg1));

	if (PG_NARGS() <= 2)
	{
		uwhere = line->points->npoints;
	}
	else
	{
		int32 where = PG_GETARG_INT32(2);
		if (where == -1)
		{
			uwhere = line->points->npoints;
		}
		else if (where < 0 || where > (int32)line->points->npoints)
		{
			elog(ERROR, "%s: Invalid offset", __func__);
			PG_RETURN_NULL();
		}
		else
		{
			uwhere = where;
		}
	}

	point = lwgeom_as_lwpoint(lwgeom_from_gserialized(pglwg2));
	linecopy = lwgeom_as_lwline(lwgeom_clone_deep(lwline_as_lwgeom(line)));
	lwline_free(line);

	if (lwline_add_lwpoint(linecopy, point, uwhere) == LW_FAILURE)
	{
		elog(ERROR, "Point insert failed");
		PG_RETURN_NULL();
	}

	result = geometry_serialize(lwline_as_lwgeom(linecopy));

	/* Release memory */
	PG_FREE_IF_COPY(pglwg1, 0);
	PG_FREE_IF_COPY(pglwg2, 1);
	lwpoint_free(point);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(LWGEOM_removepoint);
Datum LWGEOM_removepoint(PG_FUNCTION_ARGS)
{
	GSERIALIZED *pglwg1, *result;
	LWLINE *line, *outline;
	int32 which;

	POSTGIS_DEBUG(2, "LWGEOM_removepoint called.");

	pglwg1 = PG_GETARG_GSERIALIZED_P(0);
	which = PG_GETARG_INT32(1);

	if (gserialized_get_type(pglwg1) != LINETYPE)
	{
		elog(ERROR, "First argument must be a LINESTRING");
		PG_RETURN_NULL();
	}

	line = lwgeom_as_lwline(lwgeom_from_gserialized(pglwg1));

	if (which < 0 || (uint32_t)which > line->points->npoints - 1)
	{
		elog(ERROR, "Point index out of range (%u..%u)", 0, line->points->npoints - 1);
		PG_RETURN_NULL();
	}

	if (line->points->npoints < 3)
	{
		elog(ERROR, "Can't remove points from a single segment line");
		PG_RETURN_NULL();
	}

	outline = lwline_removepoint(line, (uint32_t)which);
	/* Release memory */
	lwline_free(line);

	result = geometry_serialize((LWGEOM *)outline);
	lwline_free(outline);

	PG_FREE_IF_COPY(pglwg1, 0);
	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(LWGEOM_setpoint_linestring);
Datum LWGEOM_setpoint_linestring(PG_FUNCTION_ARGS)
{
	GSERIALIZED *pglwg1, *pglwg2, *result;
	LWGEOM *lwg;
	LWLINE *line;
	LWPOINT *lwpoint;
	POINT4D newpoint;
	int64_t which;

	POSTGIS_DEBUG(2, "LWGEOM_setpoint_linestring called.");

	/* we copy input as we're going to modify it */
	pglwg1 = PG_GETARG_GSERIALIZED_P_COPY(0);

	which = PG_GETARG_INT32(1);
	pglwg2 = PG_GETARG_GSERIALIZED_P(2);

	/* Extract a POINT4D from the point */
	lwg = lwgeom_from_gserialized(pglwg2);
	lwpoint = lwgeom_as_lwpoint(lwg);
	if (!lwpoint)
	{
		elog(ERROR, "Third argument must be a POINT");
		PG_RETURN_NULL();
	}
	getPoint4d_p(lwpoint->point, 0, &newpoint);
	lwpoint_free(lwpoint);
	PG_FREE_IF_COPY(pglwg2, 2);

	lwg = lwgeom_from_gserialized(pglwg1);
	line = lwgeom_as_lwline(lwg);
	if (!line)
	{
		elog(ERROR, "First argument must be a LINESTRING");
		PG_RETURN_NULL();
	}
	if (which < 0)
	{
		/* Use backward indexing for negative values */
		which += (int64_t)line->points->npoints;
	}
	if ((uint32_t)which > line->points->npoints - 1)
	{
		elog(ERROR, "abs(Point index) out of range (-)(%u..%u)", 0, line->points->npoints - 1);
		PG_RETURN_NULL();
	}

	/*
	 * This will change pointarray of the serialized pglwg1,
	 */
	lwline_setPoint4d(line, (uint32_t)which, &newpoint);
	result = geometry_serialize((LWGEOM *)line);

	/* Release memory */
	lwline_free(line);
	pfree(pglwg1); /* we forced copy, POINARRAY is released now */

	PG_RETURN_POINTER(result);
}

/* convert LWGEOM to ewkt (in TEXT format) */
PG_FUNCTION_INFO_V1(LWGEOM_asEWKT);
Datum LWGEOM_asEWKT(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	LWGEOM *lwgeom = lwgeom_from_gserialized(geom);

	int precision = OUT_DEFAULT_DECIMAL_DIGITS;
	if (PG_NARGS() > 1)
		precision = PG_GETARG_INT32(1);

	PG_RETURN_TEXT_P(lwgeom_to_wkt_varlena(lwgeom, WKT_EXTENDED, precision));
}

/**
 * Compute the azimuth of segment defined by the two
 * given Point geometries.
 * @return NULL on exception (same point).
 * 		Return radians otherwise.
 */
PG_FUNCTION_INFO_V1(LWGEOM_azimuth);
Datum LWGEOM_azimuth(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom;
	LWPOINT *lwpoint;
	POINT2D p1, p2;
	double result;
	int32_t srid;

	/* Extract first point */
	geom = PG_GETARG_GSERIALIZED_P(0);
	lwpoint = lwgeom_as_lwpoint(lwgeom_from_gserialized(geom));
	if (!lwpoint)
	{
		PG_FREE_IF_COPY(geom, 0);
		lwpgerror("Argument must be POINT geometries");
		PG_RETURN_NULL();
	}
	srid = lwpoint->srid;
	if (!getPoint2d_p(lwpoint->point, 0, &p1))
	{
		PG_FREE_IF_COPY(geom, 0);
		lwpgerror("Error extracting point");
		PG_RETURN_NULL();
	}
	lwpoint_free(lwpoint);
	PG_FREE_IF_COPY(geom, 0);

	/* Extract second point */
	geom = PG_GETARG_GSERIALIZED_P(1);
	lwpoint = lwgeom_as_lwpoint(lwgeom_from_gserialized(geom));
	if (!lwpoint)
	{
		PG_FREE_IF_COPY(geom, 1);
		lwpgerror("Argument must be POINT geometries");
		PG_RETURN_NULL();
	}
	if (lwpoint->srid != srid)
	{
		PG_FREE_IF_COPY(geom, 1);
		lwpgerror("Operation on mixed SRID geometries");
		PG_RETURN_NULL();
	}
	if (!getPoint2d_p(lwpoint->point, 0, &p2))
	{
		PG_FREE_IF_COPY(geom, 1);
		lwpgerror("Error extracting point");
		PG_RETURN_NULL();
	}
	lwpoint_free(lwpoint);
	PG_FREE_IF_COPY(geom, 1);

	/* Standard return value for equality case */
	if ((p1.x == p2.x) && (p1.y == p2.y))
	{
		PG_RETURN_NULL();
	}

	/* Compute azimuth */
	if (!azimuth_pt_pt(&p1, &p2, &result))
	{
		PG_RETURN_NULL();
	}

	PG_RETURN_FLOAT8(result);
}

/**
 * Compute the angle defined by 3 points or the angle between 2 vectors
 * defined by 4 points
 * given Point geometries.
 * @return NULL on exception (same point).
 * 		Return radians otherwise (always positive).
 */
PG_FUNCTION_INFO_V1(LWGEOM_angle);
Datum LWGEOM_angle(PG_FUNCTION_ARGS)
{
	GSERIALIZED *seri_geoms[4];
	LWGEOM *geom_unser;
	LWPOINT *lwpoint;
	POINT2D points[4];
	double az1, az2;
	double result;
	int32_t srids[4];
	int i = 0;
	int j = 0;
	int err_code = 0;
	int n_args = PG_NARGS();

	/* no deserialize, checking for common error first*/
	for (i = 0; i < n_args; i++)
	{
		seri_geoms[i] = PG_GETARG_GSERIALIZED_P(i);
		if (gserialized_is_empty(seri_geoms[i]))
		{ /* empty geom */
			if (i == 3)
			{
				n_args = 3;
			}
			else
			{
				err_code = 1;
				break;
			}
		}
		else
		{
			if (gserialized_get_type(seri_geoms[i]) != POINTTYPE)
			{ /* geom type */
				err_code = 2;
				break;
			}
			else
			{
				srids[i] = gserialized_get_srid(seri_geoms[i]);
				if (srids[0] != srids[i])
				{ /* error on srid*/
					err_code = 3;
					break;
				}
			}
		}
	}
	if (err_code > 0)
		switch (err_code)
		{
		default: /*always executed*/
			for (j = 0; j <= i; j++)
				PG_FREE_IF_COPY(seri_geoms[j], j);
		/*FALLTHROUGH*/
		case 1:
			lwpgerror("Empty geometry");
			PG_RETURN_NULL();
			break;

		case 2:
			lwpgerror("Argument must be POINT geometries");
			PG_RETURN_NULL();
			break;

		case 3:
			lwpgerror("Operation on mixed SRID geometries");
			PG_RETURN_NULL();
			break;
		}
	/* extract points */
	for (i = 0; i < n_args; i++)
	{
		geom_unser = lwgeom_from_gserialized(seri_geoms[i]);
		lwpoint = lwgeom_as_lwpoint(geom_unser);
		if (!lwpoint)
		{
			for (j = 0; j < n_args; j++)
				PG_FREE_IF_COPY(seri_geoms[j], j);
			lwpgerror("Error unserializing geometry");
			PG_RETURN_NULL();
		}

		if (!getPoint2d_p(lwpoint->point, 0, &points[i]))
		{
			/* // can't free serialized geom, it might be needed by lw
			for (j=0;j<n_args;j++)
				PG_FREE_IF_COPY(seri_geoms[j], j); */
			lwpgerror("Error extracting point");
			PG_RETURN_NULL();
		}
		/* lwfree(geom_unser);don't do, lw may rely on this memory
		lwpoint_free(lwpoint); dont do , this memory is needed ! */
	}
	/* // can't free serialized geom, it might be needed by lw
	for (j=0;j<n_args;j++)
		PG_FREE_IF_COPY(seri_geoms[j], j); */

	/* compute azimuth for the 2 pairs of points
	 * note that angle is not defined identically for 3 points or 4 points*/
	if (n_args == 3)
	{ /* we rely on azimuth to complain if points are identical */
		if (!azimuth_pt_pt(&points[0], &points[1], &az1))
			PG_RETURN_NULL();
		if (!azimuth_pt_pt(&points[2], &points[1], &az2))
			PG_RETURN_NULL();
	}
	else
	{
		if (!azimuth_pt_pt(&points[0], &points[1], &az1))
			PG_RETURN_NULL();
		if (!azimuth_pt_pt(&points[2], &points[3], &az2))
			PG_RETURN_NULL();
	}
	result = az2 - az1;
	result += (result < 0) * 2 * M_PI; /* we dont want negative angle*/
	PG_RETURN_FLOAT8(result);
}

/*
 * optimistic_overlap(Polygon P1, Multipolygon MP2, double dist)
 * returns true if P1 overlaps MP2
 *   method: bbox check -
 *   is separation < dist?  no - return false (quick)
 *                          yes  - return distance(P1,MP2) < dist
 */
PG_FUNCTION_INFO_V1(optimistic_overlap);
Datum optimistic_overlap(PG_FUNCTION_ARGS)
{
	GSERIALIZED *pg_geom1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *pg_geom2 = PG_GETARG_GSERIALIZED_P(1);
	double dist = PG_GETARG_FLOAT8(2);
	GBOX g1_bvol;
	double calc_dist;
	LWGEOM *geom1 = lwgeom_from_gserialized(pg_geom1);
	LWGEOM *geom2 = lwgeom_from_gserialized(pg_geom2);
	gserialized_error_if_srid_mismatch(pg_geom1, pg_geom2, __func__);

	if (geom1->type != POLYGONTYPE)
	{
		elog(ERROR, "optimistic_overlap: first arg isn't a polygon\n");
		PG_RETURN_NULL();
	}

	if (geom2->type != POLYGONTYPE && geom2->type != MULTIPOLYGONTYPE)
	{
		elog(ERROR, "optimistic_overlap: 2nd arg isn't a [multi-]polygon\n");
		PG_RETURN_NULL();
	}

	/*bbox check */
	gserialized_get_gbox_p(pg_geom1, &g1_bvol);

	g1_bvol.xmin = g1_bvol.xmin - dist;
	g1_bvol.ymin = g1_bvol.ymin - dist;
	g1_bvol.xmax = g1_bvol.xmax + dist;
	g1_bvol.ymax = g1_bvol.ymax + dist;

	if ((g1_bvol.xmin > geom2->bbox->xmax) || (g1_bvol.xmax < geom2->bbox->xmin) ||
	    (g1_bvol.ymin > geom2->bbox->ymax) || (g1_bvol.ymax < geom2->bbox->ymin))
	{
		PG_RETURN_BOOL(false); /*bbox not overlap */
	}

	/*
	 * compute distances
	 * should be a fast calc if they actually do intersect
	 */
	calc_dist =
	    DatumGetFloat8(DirectFunctionCall2(ST_Distance, PointerGetDatum(pg_geom1), PointerGetDatum(pg_geom2)));

	PG_RETURN_BOOL(calc_dist < dist);
}

/*affine transform geometry */
PG_FUNCTION_INFO_V1(LWGEOM_affine);
Datum LWGEOM_affine(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P_COPY(0);
	LWGEOM *lwgeom = lwgeom_from_gserialized(geom);
	GSERIALIZED *ret;
	AFFINE affine;

	affine.afac = PG_GETARG_FLOAT8(1);
	affine.bfac = PG_GETARG_FLOAT8(2);
	affine.cfac = PG_GETARG_FLOAT8(3);
	affine.dfac = PG_GETARG_FLOAT8(4);
	affine.efac = PG_GETARG_FLOAT8(5);
	affine.ffac = PG_GETARG_FLOAT8(6);
	affine.gfac = PG_GETARG_FLOAT8(7);
	affine.hfac = PG_GETARG_FLOAT8(8);
	affine.ifac = PG_GETARG_FLOAT8(9);
	affine.xoff = PG_GETARG_FLOAT8(10);
	affine.yoff = PG_GETARG_FLOAT8(11);
	affine.zoff = PG_GETARG_FLOAT8(12);

	POSTGIS_DEBUG(2, "LWGEOM_affine called.");

	lwgeom_affine(lwgeom, &affine);

	/* COMPUTE_BBOX TAINTING */
	if (lwgeom->bbox)
	{
		lwgeom_refresh_bbox(lwgeom);
	}
	ret = geometry_serialize(lwgeom);

	/* Release memory */
	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(ret);
}

PG_FUNCTION_INFO_V1(ST_GeoHash);
Datum ST_GeoHash(PG_FUNCTION_ARGS)
{

	GSERIALIZED *geom = NULL;
	int precision = 0;
	lwvarlena_t *geohash = NULL;

	if (PG_ARGISNULL(0))
	{
		PG_RETURN_NULL();
	}

	geom = PG_GETARG_GSERIALIZED_P(0);

	if (!PG_ARGISNULL(1))
	{
		precision = PG_GETARG_INT32(1);
	}

	geohash = lwgeom_geohash((LWGEOM *)(lwgeom_from_gserialized(geom)), precision);
	if (geohash)
		PG_RETURN_TEXT_P(geohash);
	PG_RETURN_NULL();
}

PG_FUNCTION_INFO_V1(_ST_SortableHash);
Datum _ST_SortableHash(PG_FUNCTION_ARGS)
{
	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();
	PG_RETURN_INT64(gserialized_get_sortable_hash(PG_GETARG_GSERIALIZED_P(0)));
}

PG_FUNCTION_INFO_V1(ST_CollectionExtract);
Datum ST_CollectionExtract(PG_FUNCTION_ARGS)
{
	GSERIALIZED *gser_in, *gser_out;
	LWGEOM *lwg_in = NULL;
	LWGEOM *lwg_out = NULL;
	int extype = 0;

	if (PG_NARGS() > 1)
		extype = PG_GETARG_INT32(1);

	/* Ensure the right type was input */
	if (!(extype == 0 || extype == POINTTYPE || extype == LINETYPE || extype == POLYGONTYPE))
	{
		elog(ERROR, "ST_CollectionExtract: only point, linestring and polygon may be extracted");
		PG_RETURN_NULL();
	}

	gser_in = PG_GETARG_GSERIALIZED_P(0);
	lwg_in = lwgeom_from_gserialized(gser_in);

	/* Mirror non-collections right back */
	if (!lwgeom_is_collection(lwg_in))
	{
		/* Non-collections of the matching type go back */
		if (lwg_in->type == extype || !extype)
		{
			lwgeom_free(lwg_in);
			PG_RETURN_POINTER(gser_in);
		}
		/* Others go back as EMPTY */
		else
		{
			lwg_out = lwgeom_construct_empty(extype, lwg_in->srid, lwgeom_has_z(lwg_in), lwgeom_has_m(lwg_in));
			PG_RETURN_POINTER(geometry_serialize(lwg_out));
		}
	}

	lwg_out = (LWGEOM*)lwcollection_extract((LWCOLLECTION*)lwg_in, extype);

	gser_out = geometry_serialize(lwg_out);
	lwgeom_free(lwg_in);
	lwgeom_free(lwg_out);
	PG_RETURN_POINTER(gser_out);
}

PG_FUNCTION_INFO_V1(ST_CollectionHomogenize);
Datum ST_CollectionHomogenize(PG_FUNCTION_ARGS)
{
	GSERIALIZED *input = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *output;
	LWGEOM *lwgeom = lwgeom_from_gserialized(input);
	LWGEOM *lwoutput = NULL;

	lwoutput = lwgeom_homogenize(lwgeom);
	lwgeom_free(lwgeom);

	if (!lwoutput)
	{
		PG_FREE_IF_COPY(input, 0);
		PG_RETURN_NULL();
	}

	output = geometry_serialize(lwoutput);
	lwgeom_free(lwoutput);

	PG_FREE_IF_COPY(input, 0);
	PG_RETURN_POINTER(output);
}

Datum ST_RemoveRepeatedPoints(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_RemoveRepeatedPoints);
Datum ST_RemoveRepeatedPoints(PG_FUNCTION_ARGS)
{
	GSERIALIZED *g_in = PG_GETARG_GSERIALIZED_P_COPY(0);
	uint32_t type = gserialized_get_type(g_in);
	GSERIALIZED *g_out;
	LWGEOM *lwgeom_in = NULL;
	double tolerance = 0.0;
	int modified = LW_FALSE;

	/* Don't even start to think about points */
	if (type == POINTTYPE)
		PG_RETURN_POINTER(g_in);

	if (PG_NARGS() > 1 && !PG_ARGISNULL(1))
		tolerance = PG_GETARG_FLOAT8(1);

	lwgeom_in = lwgeom_from_gserialized(g_in);
	modified = lwgeom_remove_repeated_points_in_place(lwgeom_in, tolerance);
	if (!modified)
	{
		/* Since there were no changes, we can return the input to avoid the serialization */
		PG_RETURN_POINTER(g_in);
	}

	g_out = geometry_serialize(lwgeom_in);

	pfree(g_in);
	PG_RETURN_POINTER(g_out);
}

Datum ST_FlipCoordinates(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_FlipCoordinates);
Datum ST_FlipCoordinates(PG_FUNCTION_ARGS)
{
	GSERIALIZED *in = PG_GETARG_GSERIALIZED_P_COPY(0);
	GSERIALIZED *out;
	LWGEOM *lwgeom = lwgeom_from_gserialized(in);

	lwgeom_swap_ordinates(lwgeom, LWORD_X, LWORD_Y);
	out = geometry_serialize(lwgeom);

	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(in, 0);

	PG_RETURN_POINTER(out);
}

static LWORD
ordname2ordval(char n)
{
	if (n == 'x' || n == 'X')
		return LWORD_X;
	if (n == 'y' || n == 'Y')
		return LWORD_Y;
	if (n == 'z' || n == 'Z')
		return LWORD_Z;
	if (n == 'm' || n == 'M')
		return LWORD_M;
	lwpgerror("Invalid ordinate name '%c'. Expected x,y,z or m", n);
	return (LWORD)-1;
}

Datum ST_SwapOrdinates(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_SwapOrdinates);
Datum ST_SwapOrdinates(PG_FUNCTION_ARGS)
{
	GSERIALIZED *in;
	GSERIALIZED *out;
	LWGEOM *lwgeom;
	const char *ospec;
	LWORD o1, o2;

	ospec = PG_GETARG_CSTRING(1);
	if (strlen(ospec) != 2)
	{
		lwpgerror(
		    "Invalid ordinate specification. "
		    "Need two letters from the set (x,y,z,m). "
		    "Got '%s'",
		    ospec);
		PG_RETURN_NULL();
	}
	o1 = ordname2ordval(ospec[0]);
	o2 = ordname2ordval(ospec[1]);

	in = PG_GETARG_GSERIALIZED_P_COPY(0);

	/* Check presence of given ordinates */
	if ((o1 == LWORD_M || o2 == LWORD_M) && !gserialized_has_m(in))
	{
		lwpgerror("Geometry does not have an M ordinate");
		PG_RETURN_NULL();
	}
	if ((o1 == LWORD_Z || o2 == LWORD_Z) && !gserialized_has_z(in))
	{
		lwpgerror("Geometry does not have a Z ordinate");
		PG_RETURN_NULL();
	}

	/* Nothing to do if swapping the same ordinate, pity for the copy... */
	if (o1 == o2)
		PG_RETURN_POINTER(in);

	lwgeom = lwgeom_from_gserialized(in);
	lwgeom_swap_ordinates(lwgeom, o1, o2);
	out = geometry_serialize(lwgeom);
	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(in, 0);
	PG_RETURN_POINTER(out);
}

/*
 * ST_BoundingDiagonal(inp geometry, fits boolean)
 */
Datum ST_BoundingDiagonal(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_BoundingDiagonal);
Datum ST_BoundingDiagonal(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom_out;
	bool fits = PG_GETARG_BOOL(1);
	LWGEOM *lwgeom_out = NULL;

	GBOX gbox = {0};
	int hasz;
	int hasm;
	int32_t srid;

	POINT4D pt;
	POINTARRAY *pa;

	if (fits)
	{
		GSERIALIZED *geom_in = PG_GETARG_GSERIALIZED_P(0);
		LWGEOM *lwgeom_in = lwgeom_from_gserialized(geom_in);
		lwgeom_calculate_gbox(lwgeom_in, &gbox);
		hasz = FLAGS_GET_Z(lwgeom_in->flags);
		hasm = FLAGS_GET_M(lwgeom_in->flags);
		srid = lwgeom_in->srid;
	}
	else
	{
		uint8_t type;
		lwflags_t flags;
		int res = gserialized_datum_get_internals_p(PG_GETARG_DATUM(0), &gbox, &flags, &type, &srid);
		hasz = FLAGS_GET_Z(flags);
		hasm = FLAGS_GET_M(flags);
		if (res == LW_FAILURE)
		{
			lwgeom_out = lwgeom_construct_empty(LINETYPE, srid, hasz, hasm);
		}
	}

	if (!lwgeom_out)
	{
		pa = ptarray_construct_empty(hasz, hasm, 2);
		pt.x = gbox.xmin;
		pt.y = gbox.ymin;
		pt.z = gbox.zmin;
		pt.m = gbox.mmin;
		ptarray_append_point(pa, &pt, LW_TRUE);
		pt.x = gbox.xmax;
		pt.y = gbox.ymax;
		pt.z = gbox.zmax;
		pt.m = gbox.mmax;
		ptarray_append_point(pa, &pt, LW_TRUE);
		lwgeom_out = lwline_as_lwgeom(lwline_construct(srid, NULL, pa));
	}

	geom_out = geometry_serialize(lwgeom_out);
	lwgeom_free(lwgeom_out);

	PG_RETURN_POINTER(geom_out);
}

Datum ST_Scale(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_Scale);
Datum ST_Scale(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom;
	GSERIALIZED *geom_scale = PG_GETARG_GSERIALIZED_P(1);
	GSERIALIZED *geom_origin = NULL;
	LWGEOM *lwg, *lwg_scale, *lwg_origin;
	LWPOINT *lwpt_scale, *lwpt_origin;
	POINT4D origin;
	POINT4D factors;
	bool translate = false;
	GSERIALIZED *ret;
	AFFINE aff;

	/* Make sure we have a valid scale input */
	lwg_scale = lwgeom_from_gserialized(geom_scale);
	lwpt_scale = lwgeom_as_lwpoint(lwg_scale);
	if (!lwpt_scale)
	{
		lwgeom_free(lwg_scale);
		PG_FREE_IF_COPY(geom_scale, 1);
		lwpgerror("Scale factor geometry parameter must be a point");
		PG_RETURN_NULL();
	}

	/* Geom Will be modified in place, so take a copy */
	geom = PG_GETARG_GSERIALIZED_P_COPY(0);
	lwg = lwgeom_from_gserialized(geom);

	/* Empty point, return input untouched */
	if (lwgeom_is_empty(lwg))
	{
		lwgeom_free(lwg_scale);
		lwgeom_free(lwg);
		PG_FREE_IF_COPY(geom_scale, 1);
		PG_RETURN_POINTER(geom);
	}

	/* Once we read the scale data into local static point, we can */
	/* free the lwgeom */
	lwpoint_getPoint4d_p(lwpt_scale, &factors);
	if (!lwgeom_has_z(lwg_scale))
		factors.z = 1.0;
	if (!lwgeom_has_m(lwg_scale))
		factors.m = 1.0;
	lwgeom_free(lwg_scale);

	/* Do we have the optional false origin? */
	if (PG_NARGS() > 2 && !PG_ARGISNULL(2))
	{
		geom_origin = PG_GETARG_GSERIALIZED_P(2);
		lwg_origin = lwgeom_from_gserialized(geom_origin);
		lwpt_origin = lwgeom_as_lwpoint(lwg_origin);
		if (lwpt_origin)
		{
			lwpoint_getPoint4d_p(lwpt_origin, &origin);
			translate = true;
		}
		/* Free the false origin inputs */
		lwgeom_free(lwg_origin);
		PG_FREE_IF_COPY(geom_origin, 2);
	}

	/* If we have false origin, translate to it before scaling */
	if (translate)
	{
		/* Initialize affine */
		memset(&aff, 0, sizeof(AFFINE));
		/* Set rotation/scale/sheer matrix to no-op */
		aff.afac = aff.efac = aff.ifac = 1.0;
		/* Strip false origin from all coordinates */
		aff.xoff = -1 * origin.x;
		aff.yoff = -1 * origin.y;
		aff.zoff = -1 * origin.z;
		lwgeom_affine(lwg, &aff);
	}

	lwgeom_scale(lwg, &factors);

	/* Return to original origin after scaling */
	if (translate)
	{
		aff.xoff *= -1;
		aff.yoff *= -1;
		aff.zoff *= -1;
		lwgeom_affine(lwg, &aff);
	}

	/* Cleanup and return */
	ret = geometry_serialize(lwg);
	lwgeom_free(lwg);
	PG_FREE_IF_COPY(geom, 0);
	PG_FREE_IF_COPY(geom_scale, 1);
	PG_RETURN_POINTER(ret);
}

Datum ST_Points(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_Points);
Datum ST_Points(PG_FUNCTION_ARGS)
{
	if (PG_ARGISNULL(0))
	{
		PG_RETURN_NULL();
	}
	else
	{
		GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
		GSERIALIZED *ret;
		LWGEOM *lwgeom = lwgeom_from_gserialized(geom);
		LWMPOINT *result = lwmpoint_from_lwgeom(lwgeom);

		lwgeom_free(lwgeom);

		ret = geometry_serialize(lwmpoint_as_lwgeom(result));
		lwmpoint_free(result);
		PG_RETURN_POINTER(ret);
	}
}

PG_FUNCTION_INFO_V1(ST_QuantizeCoordinates);
Datum ST_QuantizeCoordinates(PG_FUNCTION_ARGS)
{
	GSERIALIZED *input;
	GSERIALIZED *result;
	LWGEOM *g;
	int32_t prec_x;
	int32_t prec_y;
	int32_t prec_z;
	int32_t prec_m;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();
	if (PG_ARGISNULL(1))
	{
		lwpgerror("Must specify precision");
		PG_RETURN_NULL();
	}
	else
	{
		prec_x = PG_GETARG_INT32(1);
	}
	prec_y = PG_ARGISNULL(2) ? prec_x : PG_GETARG_INT32(2);
	prec_z = PG_ARGISNULL(3) ? prec_x : PG_GETARG_INT32(3);
	prec_m = PG_ARGISNULL(4) ? prec_x : PG_GETARG_INT32(4);

	input = PG_GETARG_GSERIALIZED_P_COPY(0);

	g = lwgeom_from_gserialized(input);

	lwgeom_trim_bits_in_place(g, prec_x, prec_y, prec_z, prec_m);

	result = geometry_serialize(g);
	lwgeom_free(g);
	PG_FREE_IF_COPY(input, 0);
	PG_RETURN_POINTER(result);
}

/*
 * ST_FilterByM(in geometry, val double precision)
 */
PG_FUNCTION_INFO_V1(LWGEOM_FilterByM);
Datum LWGEOM_FilterByM(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom_in;
	GSERIALIZED *geom_out;
	LWGEOM *lwgeom_in;
	LWGEOM *lwgeom_out;
	double min, max;
	int returnm;
	int hasm;

	if (PG_NARGS() > 0 && !PG_ARGISNULL(0))
	{
		geom_in = PG_GETARG_GSERIALIZED_P(0);
	}
	else
	{
		PG_RETURN_NULL();
	}

	if (PG_NARGS() > 1 && !PG_ARGISNULL(1))
		min = PG_GETARG_FLOAT8(1);
	else
	{
		min = DBL_MIN;
	}
	if (PG_NARGS() > 2 && !PG_ARGISNULL(2))
		max = PG_GETARG_FLOAT8(2);
	else
	{
		max = DBL_MAX;
	}
	if (PG_NARGS() > 3 && !PG_ARGISNULL(3) && PG_GETARG_BOOL(3))
		returnm = 1;
	else
	{
		returnm = 0;
	}

	if (min > max)
	{
		elog(ERROR, "Min-value cannot be larger than Max value\n");
		PG_RETURN_NULL();
	}

	lwgeom_in = lwgeom_from_gserialized(geom_in);

	hasm = lwgeom_has_m(lwgeom_in);

	if (!hasm)
	{
		elog(NOTICE, "No M-value, No vertex removed\n");
		PG_RETURN_POINTER(geom_in);
	}

	lwgeom_out = lwgeom_filter_m(lwgeom_in, min, max, returnm);

	geom_out = geometry_serialize(lwgeom_out);
	lwgeom_free(lwgeom_out);
	PG_RETURN_POINTER(geom_out);
}
