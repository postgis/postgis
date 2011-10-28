/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2006 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "postgres.h"
#include "liblwgeom.h"
#include "fmgr.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "lwgeom_pg.h"


Datum LWGEOM_has_arc(PG_FUNCTION_ARGS);
Datum LWGEOM_curve_segmentize(PG_FUNCTION_ARGS);
Datum LWGEOM_line_desegmentize(PG_FUNCTION_ARGS);



/*******************************************************************************
 * Begin PG_FUNCTIONs
 ******************************************************************************/

PG_FUNCTION_INFO_V1(LWGEOM_has_arc);
Datum LWGEOM_has_arc(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = (GSERIALIZED *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *lwgeom = pglwgeom_deserialize(geom);
	uint32 result = lwgeom_has_arc(lwgeom);
	lwgeom_free(lwgeom);
	PG_RETURN_BOOL(result == 1);
}

/*
 * Converts any curve segments of the geometry into a linear approximation.
 * Curve centers are determined by projecting the defining points into the 2d
 * plane.  Z and M values are assigned by linear interpolation between
 * defining points.
 */
PG_FUNCTION_INFO_V1(LWGEOM_curve_segmentize);
Datum LWGEOM_curve_segmentize(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = (GSERIALIZED *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	uint32 perQuad = PG_GETARG_INT32(1);
	GSERIALIZED *ret;
	LWGEOM *igeom = NULL, *ogeom = NULL;

	POSTGIS_DEBUG(2, "LWGEOM_curve_segmentize called.");

	if (perQuad < 0)
	{
		elog(ERROR, "2nd argument must be positive.");
		PG_RETURN_NULL();
	}
#if POSTGIS_DEBUG_LEVEL > 0
	else
	{
		POSTGIS_DEBUGF(3, "perQuad = %d", perQuad);
	}
#endif
	igeom = pglwgeom_deserialize(geom);
	if ( ! lwgeom_has_arc(igeom) )
	{
		PG_RETURN_POINTER(geom);
	}
	ogeom = lwgeom_segmentize(igeom, perQuad);
	if (ogeom == NULL) PG_RETURN_NULL();
	ret = pglwgeom_serialize(ogeom);
	lwgeom_release(igeom);
	lwgeom_release(ogeom);
	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_POINTER(ret);
}

PG_FUNCTION_INFO_V1(LWGEOM_line_desegmentize);
Datum LWGEOM_line_desegmentize(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = (GSERIALIZED *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GSERIALIZED *ret;
	LWGEOM *igeom = NULL, *ogeom = NULL;

	POSTGIS_DEBUG(2, "LWGEOM_line_desegmentize.");

	igeom = pglwgeom_deserialize(geom);
	ogeom = lwgeom_desegmentize(igeom);
	if (ogeom == NULL)
	{
		lwgeom_release(igeom);
		PG_RETURN_NULL();
	}
	ret = pglwgeom_serialize(ogeom);
	lwgeom_release(igeom);
	lwgeom_release(ogeom);
	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_POINTER(ret);
}

/*******************************************************************************
 * End PG_FUNCTIONs
 ******************************************************************************/
