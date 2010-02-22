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
*
* SVG output routines.
* Originally written by: Klaus Förster <klaus@svg.cc>
* Refactored by: Olivier Courtin (Camptocamp)
*
* BNF SVG Path: <http://www.w3.org/TR/SVG/paths.html#PathDataBNF>
**********************************************************************/


#include "postgres.h"
#include "liblwgeom.h"
#include "lwgeom_pg.h"
#include "lwgeom_export.h"

Datum assvg_geometry(PG_FUNCTION_ARGS);

/**
 * SVG features
 */
PG_FUNCTION_INFO_V1(assvg_geometry);
Datum assvg_geometry(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	char *svg;
	text *result;
	int len;
	int relative = 0;
	int precision=OUT_MAX_DOUBLE_PRECISION;

	if ( PG_ARGISNULL(0) ) PG_RETURN_NULL();

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* check for relative path notation */
	if ( PG_NARGS() > 1 && ! PG_ARGISNULL(1) )
		relative = PG_GETARG_INT32(1) ? 1:0;

	if ( PG_NARGS() > 2 && ! PG_ARGISNULL(2) )
	{
		precision = PG_GETARG_INT32(2);
		if ( precision > OUT_MAX_DOUBLE_PRECISION )
			precision = OUT_MAX_DOUBLE_PRECISION;
		else if ( precision < 0 ) precision = 0;
	}

	svg = lwgeom_to_svg(SERIALIZED_FORM(geom), precision, relative);
	PG_FREE_IF_COPY(geom, 0);

	len = strlen(svg) + VARHDRSZ;
	result = palloc(len);
	SET_VARSIZE(result, len);
	memcpy(VARDATA(result), svg, len-VARHDRSZ);

	lwfree(svg);

	PG_RETURN_POINTER(result);
}
