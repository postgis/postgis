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

/**
* @file
* KML output routines based on lwgeom_gml.c
* Written by: Eduin Carrillo <yecarrillo@cas.gov.co>
*             © 2006 Corporacion Autonoma Regional de Santander - CAS
*
**********************************************************************/


#include "postgres.h"
#include "executor/spi.h"

#include "lwgeom_pg.h"
#include "liblwgeom.h"
#include "lwgeom_export.h"

Datum LWGEOM_asKML(PG_FUNCTION_ARGS);

/**
 * Encode feature in KML
 */
PG_FUNCTION_INFO_V1(LWGEOM_asKML);
Datum LWGEOM_asKML(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	char *kml;
	text *result;
	int len;
	int version;
	int precision = OUT_MAX_DOUBLE_PRECISION;


	/* Get the version */
	version = PG_GETARG_INT32(0);
	if ( version != 2)
	{
		elog(ERROR, "Only KML 2 is supported");
		PG_RETURN_NULL();
	}

	/* Get the geometry */
	if ( PG_ARGISNULL(1) ) PG_RETURN_NULL();
	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	/* Retrieve precision if any (default is max) */
	if (PG_NARGS() >2 && !PG_ARGISNULL(2))
	{
		precision = PG_GETARG_INT32(2);
		if ( precision > OUT_MAX_DOUBLE_PRECISION )
			precision = OUT_MAX_DOUBLE_PRECISION;
		else if ( precision < 0 ) precision = 0;
	}

	kml = lwgeom_to_kml2(SERIALIZED_FORM(geom), precision);

	PG_FREE_IF_COPY(geom, 1);

	len = strlen(kml) + VARHDRSZ;

	result = palloc(len);
	SET_VARSIZE(result, len);

	memcpy(VARDATA(result), kml, len-VARHDRSZ);

	lwfree(kml);

	PG_RETURN_POINTER(result);
}
