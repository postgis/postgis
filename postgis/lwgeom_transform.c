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
#include "fmgr.h"
#include "utils/builtins.h"

#include "../postgis_config.h"
#include "liblwgeom.h"
#include "lwgeom_transform.h"


Datum transform(PG_FUNCTION_ARGS);
Datum transform_geom(PG_FUNCTION_ARGS);
Datum postgis_proj_version(PG_FUNCTION_ARGS);
Datum LWGEOM_asKML(PG_FUNCTION_ARGS);

/**
 * transform( GEOMETRY, INT (output srid) )
 * tmpPts - if there is a nadgrid error (-38), we re-try the transform
 * on a copy of points.  The transformed points
 * are in an indeterminate state after the -38 error is thrown.
 */
PG_FUNCTION_INFO_V1(transform);
Datum transform(PG_FUNCTION_ARGS)
{
	GSERIALIZED* geom;
	GSERIALIZED* result=NULL;
	LWGEOM* lwgeom;
	LWPROJ *pj;
	int32 srid_to, srid_from;

	srid_to = PG_GETARG_INT32(1);
	if (srid_to == SRID_UNKNOWN)
	{
		elog(ERROR, "ST_Transform: %d is an invalid target SRID", SRID_UNKNOWN);
		PG_RETURN_NULL();
	}

	geom = PG_GETARG_GSERIALIZED_P_COPY(0);
	srid_from = gserialized_get_srid(geom);

	if ( srid_from == SRID_UNKNOWN )
	{
		PG_FREE_IF_COPY(geom, 0);
		elog(ERROR, "ST_Transform: Input geometry has unknown (%d) SRID", SRID_UNKNOWN);
		PG_RETURN_NULL();
	}

	/* Input SRID and output SRID are equal, noop */
	if ( srid_from == srid_to )
		PG_RETURN_POINTER(geom);

	if ( GetPJUsingFCInfo(fcinfo, srid_from, srid_to, &pj) == LW_FAILURE )
	{
		PG_FREE_IF_COPY(geom, 0);
		elog(ERROR, "ST_Transform: Failure reading projections from spatial_ref_sys.");
		PG_RETURN_NULL();
	}

	/* now we have a geometry, and input/output PJ structs. */
	lwgeom = lwgeom_from_gserialized(geom);
	lwgeom_transform(lwgeom, pj);
	lwgeom->srid = srid_to;

	/* Re-compute bbox if input had one (COMPUTE_BBOX TAINTING) */
	if ( lwgeom->bbox )
	{
		lwgeom_refresh_bbox(lwgeom);
	}

	result = geometry_serialize(lwgeom);
	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(result); /* new geometry */
}

/**
 * Transform_geom( GEOMETRY, TEXT (input proj4), TEXT (output proj4),
 *	INT (output srid)
 *
 * tmpPts - if there is a nadgrid error (-38), we re-try the transform
 * on a copy of points.  The transformed points
 * are in an indeterminate state after the -38 error is thrown.
 */
PG_FUNCTION_INFO_V1(transform_geom);
Datum transform_geom(PG_FUNCTION_ARGS)
{
	GSERIALIZED *gser, *gser_result=NULL;
	LWGEOM *geom;
	char *input_srs, *output_srs;
	int32 result_srid;
	int rv;

	/* Take a copy, since we will be altering the coordinates */
	gser = PG_GETARG_GSERIALIZED_P_COPY(0);

	/* Convert from text to cstring for libproj */
	input_srs = text_to_cstring(PG_GETARG_TEXT_P(1));
	output_srs = text_to_cstring(PG_GETARG_TEXT_P(2));
	result_srid = PG_GETARG_INT32(3);

	/* now we have a geometry, and input/output PJ structs. */
	geom = lwgeom_from_gserialized(gser);
	rv = lwgeom_transform_from_str(geom, input_srs, output_srs);
	pfree(input_srs);
	pfree(output_srs);

	if (rv == LW_FAILURE)
	{
		elog(ERROR, "coordinate transformation failed");
		PG_RETURN_NULL();
	}

	/* Re-compute bbox if input had one (COMPUTE_BBOX TAINTING) */
	geom->srid = result_srid;
	if (geom->bbox)
		lwgeom_refresh_bbox(geom);

	gser_result = geometry_serialize(geom);
	lwgeom_free(geom);
	PG_FREE_IF_COPY(gser, 0);

	PG_RETURN_POINTER(gser_result); /* new geometry */
}


PG_FUNCTION_INFO_V1(postgis_proj_version);
Datum postgis_proj_version(PG_FUNCTION_ARGS)
{
#if POSTGIS_PROJ_VERSION < 60
	const char *ver = pj_get_release();
	text *result = cstring_to_text(ver);
#else
	PJ_INFO pji = proj_info();
	text *result = 	cstring_to_text(pji.version);
#endif
	PG_RETURN_POINTER(result);
}


/**
 * Encode feature in KML
 */
PG_FUNCTION_INFO_V1(LWGEOM_asKML);
Datum LWGEOM_asKML(PG_FUNCTION_ARGS)
{
	LWGEOM *lwgeom;
	lwvarlena_t *kml;
	const char *default_prefix = ""; /* default prefix */
	char *prefixbuf;
	const char *prefix = default_prefix;
	int32_t srid_from;
	const int32_t srid_to = 4326;

	/* Get the geometry */
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P_COPY(0);
	int precision = PG_GETARG_INT32(1);
	text *prefix_text = PG_GETARG_TEXT_P(2);
	srid_from = gserialized_get_srid(geom);

	if ( srid_from == SRID_UNKNOWN )
	{
		PG_FREE_IF_COPY(geom, 0);
		elog(ERROR, "ST_AsKML: Input geometry has unknown (%d) SRID", SRID_UNKNOWN);
		PG_RETURN_NULL();
	}

	/* Condition precision */
	if (precision < 0)
		precision = 0;

	if (VARSIZE_ANY_EXHDR(prefix_text) > 0)
	{
		/* +2 is one for the ':' and one for term null */
		prefixbuf = palloc(VARSIZE_ANY_EXHDR(prefix_text)+2);
		memcpy(prefixbuf, VARDATA(prefix_text),
		       VARSIZE_ANY_EXHDR(prefix_text));
		/* add colon and null terminate */
		prefixbuf[VARSIZE_ANY_EXHDR(prefix_text)] = ':';
		prefixbuf[VARSIZE_ANY_EXHDR(prefix_text)+1] = '\0';
		prefix = prefixbuf;
	}

	lwgeom = lwgeom_from_gserialized(geom);

	if (srid_from != srid_to)
	{
		LWPROJ *pj;
		if (GetPJUsingFCInfo(fcinfo, srid_from, srid_to, &pj) == LW_FAILURE)
		{
			PG_FREE_IF_COPY(geom, 0);
			elog(ERROR, "ST_AsKML: Failure reading projections from spatial_ref_sys.");
			PG_RETURN_NULL();
		}
		lwgeom_transform(lwgeom, pj);
	}

	kml = lwgeom_to_kml2(lwgeom, precision, prefix);
	if (kml)
		PG_RETURN_TEXT_P(kml);
	PG_RETURN_NULL();
}
