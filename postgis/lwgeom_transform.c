/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright (C) 2001-2003 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "postgres.h"
#include "fmgr.h"

#include "../postgis_config.h"
#include "liblwgeom.h"
#include "lwgeom_transform.h"


Datum transform(PG_FUNCTION_ARGS);
Datum postgis_proj_version(PG_FUNCTION_ARGS);

/* Availability: 2.2.0 */
Datum transform_to_proj(PG_FUNCTION_ARGS);
Datum transform_from_proj_to_proj(PG_FUNCTION_ARGS);
Datum transform_from_proj_to_srid(PG_FUNCTION_ARGS);

/**
 * transform( GEOMETRY, INT (output srid) )
 * tmpPts - if there is a nadgrid error (-38), we re-try the transform
 * on a copy of points.  The transformed points
 * are in an indeterminate state after the -38 error is thrown.
 */
PG_FUNCTION_INFO_V1(transform);
Datum transform(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom;
	GSERIALIZED *result=NULL;
	LWGEOM *lwgeom;
	projPJ input_pj, output_pj;
	int32 output_srid, input_srid;

	output_srid = PG_GETARG_INT32(1);
	if (output_srid == SRID_UNKNOWN)
	{
		elog(ERROR,"%d is an invalid target SRID",SRID_UNKNOWN);
		PG_RETURN_NULL();
	}

	geom = PG_GETARG_GSERIALIZED_P_COPY(0);
	input_srid = gserialized_get_srid(geom);
	if ( input_srid == SRID_UNKNOWN )
	{
		PG_FREE_IF_COPY(geom, 0);
		elog(ERROR,"Input geometry has unknown (%d) SRID",SRID_UNKNOWN);
		PG_RETURN_NULL();
	}

	/*
	 * If input SRID and output SRID are equal, return geometry
	 * without transform it
	 */
	if ( input_srid == output_srid )
		PG_RETURN_POINTER(PG_GETARG_DATUM(0));

	if ( GetProjectionsUsingFCInfo(fcinfo, input_srid, output_srid, &input_pj, &output_pj) == LW_FAILURE )
	{
		PG_FREE_IF_COPY(geom, 0);
		elog(ERROR,"Failure reading projections from spatial_ref_sys.");
		PG_RETURN_NULL();
	}
	
	/* now we have a geometry, and input/output PJ structs. */
	lwgeom = lwgeom_from_gserialized(geom);
	lwgeom_transform(lwgeom, input_pj, output_pj);
	lwgeom->srid = output_srid;

	/* Re-compute bbox if input had one (COMPUTE_BBOX TAINTING) */
	if ( lwgeom->bbox )
	{
		lwgeom_drop_bbox(lwgeom);
		lwgeom_add_bbox(lwgeom);
	}

	result = geometry_serialize(lwgeom);
	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(result); /* new geometry */
}

PG_FUNCTION_INFO_V1(postgis_proj_version);
Datum postgis_proj_version(PG_FUNCTION_ARGS)
{
	const char *ver = pj_get_release();
	text *result = cstring2text(ver);
	PG_RETURN_POINTER(result);
}

/**
 * ST_Transform(geom geometry, to_proj text)
 *
 * Availability: 2.2.0
 */
PG_FUNCTION_INFO_V1(transform_to_proj);
Datum transform_to_proj(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom;
	GSERIALIZED *result=NULL;
	LWGEOM *lwgeom;
	projPJ input_pj, output_pj;
	char *input_proj4, *output_proj4;
	text *output_proj4_text;
	int32 input_srid;

	char *pj_errstr;

	/* Set the search path if we haven't already */
	SetPROJ4LibPath();

	/* Read the arguments */
	geom = PG_GETARG_GSERIALIZED_P_COPY(0);
	input_srid = gserialized_get_srid(geom);
	if ( input_srid == SRID_UNKNOWN )
	{
		PG_FREE_IF_COPY(geom, 0);
		elog(ERROR, "Input geometry has unknown (%d) SRID", SRID_UNKNOWN);
		PG_RETURN_NULL();
	}
	input_proj4 = GetProj4StringSPI(input_srid);

	output_proj4_text = PG_GETARG_TEXT_P(1);
	output_proj4 = text2cstring(output_proj4_text);

	/* make input and output projection objects */
	input_pj = lwproj_from_string(input_proj4);
	if ( input_pj == NULL )
	{
		pj_errstr = pj_strerrno(*pj_get_errno_ref());
		if ( ! pj_errstr ) pj_errstr = "";
		PG_FREE_IF_COPY(geom, 0);
		elog(ERROR,
			"transform_to_proj: could not parse SRID=%d for PROJ.4: '%s' %s",
			input_srid, input_proj4, pj_errstr);
		pfree(input_proj4);
		PG_RETURN_NULL();
	}
	pfree(input_proj4);

	output_pj = lwproj_from_string(output_proj4);
	if ( output_pj == NULL )
	{
		pj_errstr = pj_strerrno(*pj_get_errno_ref());
		if ( ! pj_errstr ) pj_errstr = "";
		pj_free(input_pj);
		PG_FREE_IF_COPY(geom, 0);
		elog(ERROR,
			"transform_to_proj: could not parse 'to_proj' for PROJ.4: '%s': %s",
			output_proj4, pj_errstr);
		pfree(output_proj4);
		PG_RETURN_NULL();
	}
	pfree(output_proj4);

	/* now we have a geometry, and input/output PJ structs. */
	lwgeom = lwgeom_from_gserialized(geom);
	lwgeom_transform(lwgeom, input_pj, output_pj);
	/* the geometry does not have an SRID */
	lwgeom->srid = SRID_UNKNOWN;

	/* clean up */
	pj_free(input_pj);
	pj_free(output_pj);

	/* Re-compute bbox if input had one (COMPUTE_BBOX TAINTING) */
	if ( lwgeom->bbox )
	{
		lwgeom_drop_bbox(lwgeom);
		lwgeom_add_bbox(lwgeom);
	}

	result = geometry_serialize(lwgeom);

	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(result); /* new geometry */
}

/**
 * ST_Transform(geom geometry, from_proj text, to_proj text)
 *
 * Availability: 2.2.0
 */
PG_FUNCTION_INFO_V1(transform_from_proj_to_proj);
Datum transform_from_proj_to_proj(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom;
	GSERIALIZED *result=NULL;
	LWGEOM *lwgeom;
	projPJ input_pj, output_pj;
	char *input_proj4, *output_proj4;
	text *input_proj4_text;
	text *output_proj4_text;
	int32 geom_srid;
	char *pj_errstr;


	geom = PG_GETARG_GSERIALIZED_P_COPY(0);
	input_proj4_text = PG_GETARG_TEXT_P(1);
	output_proj4_text = PG_GETARG_TEXT_P(2);

	/* Convert from text to cstring for libproj */

	geom_srid = gserialized_get_srid(geom);
	if (geom_srid != SRID_UNKNOWN)
		elog(WARNING,
				"transform_from_proj_to_proj: ignoring geometry SRID=%d",
				geom_srid);

	/* make input and output projection objects */
	SetPROJ4LibPath();
	input_proj4 = text2cstring(input_proj4_text);
	input_pj = lwproj_from_string(input_proj4);
	if ( input_pj == NULL )
	{
		pj_errstr = pj_strerrno(*pj_get_errno_ref());
		if ( ! pj_errstr ) pj_errstr = "";
		PG_FREE_IF_COPY(geom, 0);
		elog(ERROR,
			"transform_from_proj_to_proj: could not parse 'from_proj' for PROJ.4: '%s' %s",
			input_proj4, pj_errstr);
		pfree(input_proj4);
		PG_RETURN_NULL();
	}
	pfree(input_proj4);

	output_proj4 = text2cstring(output_proj4_text);
	output_pj = lwproj_from_string(output_proj4);
	if ( output_pj == NULL )
	{
		pj_errstr = pj_strerrno(*pj_get_errno_ref());
		if ( ! pj_errstr ) pj_errstr = "";
		pj_free(input_pj);
		PG_FREE_IF_COPY(geom, 0);
		elog(ERROR,
			"transform_from_proj_to_proj: could not parse 'to_proj' for PROJ.4: '%s': %s",
			output_proj4, pj_errstr);
		pfree(output_proj4);
		PG_RETURN_NULL();
	}
	pfree(output_proj4);

	/* now we have a geometry, and input/output PJ structs. */
	lwgeom = lwgeom_from_gserialized(geom);
	lwgeom_transform(lwgeom, input_pj, output_pj);
	/* the geometry does not have an SRID */
	lwgeom->srid = SRID_UNKNOWN;

	/* clean up */
	pj_free(input_pj);
	pj_free(output_pj);

	/* Re-compute bbox if input had one (COMPUTE_BBOX TAINTING) */
	if ( lwgeom->bbox )
	{
		lwgeom_drop_bbox(lwgeom);
		lwgeom_add_bbox(lwgeom);
	}

	result = geometry_serialize(lwgeom);

	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(result); /* new geometry */
}

/**
 * ST_Transform(geom geometry, from_proj text, to_srid integer)
 *
 * Availability: 2.2.0
 */
PG_FUNCTION_INFO_V1(transform_from_proj_to_srid);
Datum transform_from_proj_to_srid(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom;
	GSERIALIZED *result=NULL;
	LWGEOM *lwgeom;
	projPJ input_pj, output_pj;
	char *input_proj4, *output_proj4;
	text *input_proj4_text;
	int32 output_srid, geom_srid;

	char *pj_errstr;

	output_srid = PG_GETARG_INT32(2);
	if (output_srid == SRID_UNKNOWN)
	{
		elog(ERROR, "%d is an invalid target SRID", SRID_UNKNOWN);
		PG_RETURN_NULL();
	}

	geom = PG_GETARG_GSERIALIZED_P_COPY(0);
	geom_srid = gserialized_get_srid(geom);
	if (geom_srid != SRID_UNKNOWN)
		elog(WARNING,
				"transform_from_proj_to_srid: ignoring geometry SRID=%d",
				geom_srid);

	input_proj4_text = PG_GETARG_TEXT_P(1);
	input_proj4 = text2cstring(input_proj4_text);

	SetPROJ4LibPath();
	output_proj4 = GetProj4StringSPI((int)output_srid);

	/* make input and output projection objects */
	input_pj = lwproj_from_string(input_proj4);
	if ( input_pj == NULL )
	{
		pj_errstr = pj_strerrno(*pj_get_errno_ref());
		if ( ! pj_errstr ) pj_errstr = "";
		PG_FREE_IF_COPY(geom, 0);
		elog(ERROR,
			"transform_from_proj_to_srid: could not parse 'from_proj' for PROJ.4: '%s' %s",
			input_proj4, pj_errstr);
		pfree(input_proj4);
		PG_RETURN_NULL();
	}
	pfree(input_proj4);

	output_pj = lwproj_from_string(output_proj4);
	if ( output_pj == NULL )
	{
		pj_errstr = pj_strerrno(*pj_get_errno_ref());
		if ( ! pj_errstr ) pj_errstr = "";
		pj_free(input_pj);
		PG_FREE_IF_COPY(geom, 0);
		elog(ERROR,
			"transform_from_proj_to_srid: could not parse SRID=%d 'to_proj' for PROJ.4: '%s': %s",
			output_srid, output_proj4, pj_errstr);
		pfree(output_proj4);
		PG_RETURN_NULL();
	}
	pfree(output_proj4);

	/* now we have a geometry, and input/output PJ structs. */
	lwgeom = lwgeom_from_gserialized(geom);
	lwgeom_transform(lwgeom, input_pj, output_pj);
	lwgeom->srid = output_srid;

	/* clean up */
	pj_free(input_pj);
	pj_free(output_pj);

	/* Re-compute bbox if input had one (COMPUTE_BBOX TAINTING) */
	if ( lwgeom->bbox )
	{
		lwgeom_drop_bbox(lwgeom);
		lwgeom_add_bbox(lwgeom);
	}

	result = geometry_serialize(lwgeom);

	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(result); /* new geometry */
}
