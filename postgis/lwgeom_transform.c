/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 *
 * Copyright (C) 2001-2003 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "postgres.h"
#include "fmgr.h"
#include "proj_api.h"

#include "liblwgeom.h"
#include "lwgeom_transform.h"


Datum transform(PG_FUNCTION_ARGS);
Datum transform_geom(PG_FUNCTION_ARGS);
Datum postgis_proj_version(PG_FUNCTION_ARGS);

/**
 * transform( GEOMETRY, INT (output srid) )
 * tmpPts - if there is a nadgrid error (-38), we re-try the transform
 * on a copy of points.  The transformed points
 * are in an indeterminate state after the -38 error is thrown.
 */
PG_FUNCTION_INFO_V1(transform);
Datum transform(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	PG_LWGEOM *result=NULL;
	LWGEOM *lwgeom;
	projPJ input_pj, output_pj;
	int32 result_srid ;

	Proj4Cache *proj_cache = NULL;


	result_srid = PG_GETARG_INT32(1);
	if (result_srid == SRID_UNKNOWN)
	{
		elog(ERROR,"%d is an invalid target SRID",SRID_UNKNOWN);
		PG_RETURN_NULL();
	}

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));
	if (pglwgeom_get_srid(geom) == SRID_UNKNOWN)
	{
		PG_FREE_IF_COPY(geom, 0);
		elog(ERROR,"Input geometry has unknown (%d) SRID",SRID_UNKNOWN);
		PG_RETURN_NULL();
	}

	/* Set the search path if we haven't already */
	SetPROJ4LibPath();

	/*
	 * If input SRID and output SRID are equal, return geometry
	 * without transform it
	 */
	if (pglwgeom_get_srid(geom) == result_srid)
	{
		pfree(geom);
		PG_RETURN_POINTER(PG_GETARG_DATUM(0));
	}

	/* get or initialize the cache for this round */
	proj_cache = GetPROJ4Cache(fcinfo) ;

	/* Add the output srid to the cache if it's not already there */
	if (!IsInPROJ4Cache(proj_cache, result_srid))
		AddToPROJ4Cache(proj_cache, result_srid, pglwgeom_get_srid(geom));

	/* Get the output projection */
	output_pj = GetProjectionFromPROJ4Cache(proj_cache, result_srid);

	/* Add the input srid to the cache if it's not already there */
	if (!IsInPROJ4Cache(proj_cache, pglwgeom_get_srid(geom)))
		AddToPROJ4Cache(proj_cache, pglwgeom_get_srid(geom), result_srid);

	/* Get the input projection	 */
	input_pj = GetProjectionFromPROJ4Cache(proj_cache, pglwgeom_get_srid(geom));


	/* now we have a geometry, and input/output PJ structs. */
	lwgeom = pglwgeom_deserialize(geom);
	lwgeom_transform(lwgeom, input_pj, output_pj);
	lwgeom->srid = result_srid;

	/* Re-compute bbox if input had one (COMPUTE_BBOX TAINTING) */
	if ( lwgeom->bbox )
	{
		lwgeom_drop_bbox(lwgeom);
		lwgeom_add_bbox(lwgeom);
	}

	result = pglwgeom_serialize(lwgeom);
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
	PG_LWGEOM *geom;
	PG_LWGEOM *result=NULL;
	LWGEOM *lwgeom;
	projPJ input_pj, output_pj;
	char *input_proj4, *output_proj4;
	text *input_proj4_text;
	text *output_proj4_text;
	int32 result_srid ;
	int* pj_errno_ref;


	result_srid = PG_GETARG_INT32(3);
	if (result_srid == SRID_UNKNOWN)
	{
		elog(ERROR,"tranform: destination SRID = %d",SRID_UNKNOWN);
		PG_RETURN_NULL();
	}

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));
	if (pglwgeom_get_srid(geom) == SRID_UNKNOWN)
	{
		pfree(geom);
		elog(ERROR,"tranform: source SRID = %d",SRID_UNKNOWN);
		PG_RETURN_NULL();
	}

	/* Set the search path if we haven't already */
	SetPROJ4LibPath();

	/* Read the arguments */
	input_proj4_text  = (PG_GETARG_TEXT_P(1));
	output_proj4_text = (PG_GETARG_TEXT_P(2));

	/* Convert from text to cstring for libproj */
	input_proj4 = text2cstring(input_proj4_text);
	output_proj4 = text2cstring(output_proj4_text);

	/* make input and output projection objects */
	input_pj = lwproj_from_string(input_proj4);

	pj_errno_ref = pj_get_errno_ref();
	if ( (input_pj == NULL) || (*pj_errno_ref))
	{
		/* we need this for error reporting */
		/* pfree(input_proj4); */

		pfree(output_proj4);
		pfree(geom);
		elog(ERROR, "transform: couldn't parse proj4 input string: '%s': %s", input_proj4, pj_strerrno(*pj_errno_ref));
		PG_RETURN_NULL();
	}
	pfree(input_proj4);

	output_pj = lwproj_from_string(output_proj4);

	pj_errno_ref = pj_get_errno_ref();
	if ((output_pj == NULL)|| (*pj_errno_ref))
	{
		/* we need this for error reporting */
		/* pfree(output_proj4); */
		pj_free(input_pj);
		pfree(geom);
		elog(ERROR, "transform: couldn't parse proj4 output string: '%s': %s", output_proj4, pj_strerrno(*pj_errno_ref));
		PG_RETURN_NULL();
	}
	pfree(output_proj4);

	/* now we have a geometry, and input/output PJ structs. */
	lwgeom = pglwgeom_deserialize(geom);
	lwgeom_transform(lwgeom, input_pj, output_pj);
	lwgeom->srid = result_srid;

	/* clean up */
	pj_free(input_pj);
	pj_free(output_pj);

	/* Re-compute bbox if input had one (COMPUTE_BBOX TAINTING) */
	if ( lwgeom->bbox )
	{
		lwgeom_drop_bbox(lwgeom);
		lwgeom_add_bbox(lwgeom);
	}

	result = pglwgeom_serialize(lwgeom);

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
