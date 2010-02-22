/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2003 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of hte GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/
/** @file
* GeoJSON output routines.
* Originally written by Olivier Courtin (Camptocamp)
* 
**********************************************************************/

#include "postgres.h"

#include "lwgeom_pg.h"
#include "liblwgeom.h"
#include "lwgeom_export.h"

Datum LWGEOM_asGeoJson(PG_FUNCTION_ARGS);

/**
 * Encode Feature in GeoJson
 */
PG_FUNCTION_INFO_V1(LWGEOM_asGeoJson);
Datum LWGEOM_asGeoJson(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	char *geojson;
	text *result;
	int SRID;
	int len;
	int version;
	int option = 0;
	int has_bbox = 0;
	int precision = OUT_MAX_DOUBLE_PRECISION;
	char * srs = NULL;

	/* Get the version */
	version = PG_GETARG_INT32(0);
	if ( version != 1)
	{
		elog(ERROR, "Only GeoJSON 1 is supported");
		PG_RETURN_NULL();
	}

	/* Get the geometry */
	if (PG_ARGISNULL(1) ) PG_RETURN_NULL();
	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	/* Retrieve precision if any (default is max) */
	if (PG_NARGS() >2 && !PG_ARGISNULL(2))
	{
		precision = PG_GETARG_INT32(2);
		if ( precision > OUT_MAX_DOUBLE_PRECISION )
			precision = OUT_MAX_DOUBLE_PRECISION;
		else if ( precision < 0 ) precision = 0;
	}

	/* Retrieve output option
	 * 0 = without option (default)
	 * 1 = bbox
	 * 2 = short crs
	 * 4 = long crs
	 */
	if (PG_NARGS() >3 && !PG_ARGISNULL(3))
		option = PG_GETARG_INT32(3);

	if (option & 2 || option & 4)
	{
		SRID = lwgeom_getsrid(SERIALIZED_FORM(geom));
		if ( SRID != -1 )
		{
			if (option & 2) srs = getSRSbySRID(SRID, true);
			if (option & 4) srs = getSRSbySRID(SRID, false);
			if (!srs)
			{
				elog(	ERROR, 
					"SRID %i unknown in spatial_ref_sys table",
					SRID);
				PG_RETURN_NULL();
			}
		}
	}

	if (option & 1) has_bbox = 1;

	geojson = lwgeom_to_geojson(SERIALIZED_FORM(geom), srs, precision, has_bbox);

	PG_FREE_IF_COPY(geom, 1);
	if (srs) pfree(srs);

	len = strlen(geojson) + VARHDRSZ;
	result = palloc(len);
	SET_VARSIZE(result, len);
	memcpy(VARDATA(result), geojson, len-VARHDRSZ);

	lwfree(geojson);

	PG_RETURN_POINTER(result);
}
