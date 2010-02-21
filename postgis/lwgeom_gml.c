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
* @file GML output routines.
*
**********************************************************************/


#include "postgres.h"

#include "lwgeom_pg.h"
#include "liblwgeom.h"
#include "lwgeom_export.h"


Datum LWGEOM_asGML(PG_FUNCTION_ARGS);

/**
 * Encode feature in GML
 */
PG_FUNCTION_INFO_V1(LWGEOM_asGML);
Datum LWGEOM_asGML(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	char *gml;
	text *result;
	int len;
	int version;
	char *srs;
	int SRID;
	int option = 0;
	int is_deegree = 0;
	int precision = OUT_MAX_DOUBLE_PRECISION;

	/* Get the version */
	version = PG_GETARG_INT32(0);
	if ( version != 2 && version != 3 )
	{
		elog(ERROR, "Only GML 2 and GML 3 are supported");
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

	/* retrieve option */
	if (PG_NARGS() >3 && !PG_ARGISNULL(3))
		option = PG_GETARG_INT32(3);

	SRID = lwgeom_getsrid(SERIALIZED_FORM(geom));
	if (SRID == -1)      srs = NULL;
	else if (option & 1) srs = getSRSbySRID(SRID, false);
	else                 srs = getSRSbySRID(SRID, true);

	if (option & 16) is_deegree = 1;

	if (version == 2)
		gml = lwgeom_to_gml2(SERIALIZED_FORM(geom), srs, precision);
	else
		gml = lwgeom_to_gml3(SERIALIZED_FORM(geom), srs, precision, is_deegree);

	PG_FREE_IF_COPY(geom, 1);

	len = strlen(gml) + VARHDRSZ;

	result = palloc(len);
	SET_VARSIZE(result, len);

	memcpy(VARDATA(result), gml, len-VARHDRSZ);

	lwfree(gml);

	PG_RETURN_POINTER(result);
}
