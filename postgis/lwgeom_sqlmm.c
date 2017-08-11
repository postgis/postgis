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
 *
 **********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "postgres.h"
#include "fmgr.h"

#include "../postgis_config.h"
#include "liblwgeom.h"
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
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	LWGEOM *lwgeom = lwgeom_from_gserialized(geom);
	uint32 result = lwgeom_has_arc(lwgeom);
	lwgeom_free(lwgeom);
	PG_RETURN_BOOL(result == 1);
}

/*
 * Converts any curve segments of the geometry into a linear approximation.
 * Curve centers are determined by projecting the defining points into the 2d
 * plane.  Z and M values are assigned by linear interpolation between
 * defining points.
 *
 * TODO: drop, use ST_CurveToLine instead
 */
PG_FUNCTION_INFO_V1(LWGEOM_curve_segmentize);
Datum LWGEOM_curve_segmentize(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	int32 perQuad = PG_GETARG_INT32(1);
	GSERIALIZED *ret;
	LWGEOM *igeom = NULL, *ogeom = NULL;

	POSTGIS_DEBUG(2, "LWGEOM_curve_segmentize called.");

	if (perQuad < 0)
	{
		elog(ERROR, "2nd argument must be positive.");
		PG_RETURN_NULL();
	}

	POSTGIS_DEBUGF(3, "perQuad = %d", perQuad);

	igeom = lwgeom_from_gserialized(geom);
	ogeom = lwgeom_stroke(igeom, perQuad);
	lwgeom_free(igeom);

	if (ogeom == NULL)
		PG_RETURN_NULL();

	ret = geometry_serialize(ogeom);
	lwgeom_free(ogeom);
	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_POINTER(ret);
}

PG_FUNCTION_INFO_V1(ST_CurveToLine);
Datum ST_CurveToLine(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	double tol = PG_GETARG_FLOAT8(1);
	int toltype = PG_GETARG_INT32(2);
	int flags = PG_GETARG_INT32(3);
	GSERIALIZED *ret;
	LWGEOM *igeom = NULL, *ogeom = NULL;

	POSTGIS_DEBUG(2, "ST_CurveToLine called.");

	POSTGIS_DEBUGF(3, "tol = %g, typ = %d, flg = %d", tol, toltype, flags);

	igeom = lwgeom_from_gserialized(geom);
	ogeom = lwcurve_linearize(igeom, tol, toltype, flags);
	lwgeom_free(igeom);

	if (ogeom == NULL)
		PG_RETURN_NULL();

	ret = geometry_serialize(ogeom);
	lwgeom_free(ogeom);
	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_POINTER(ret);
}

PG_FUNCTION_INFO_V1(LWGEOM_line_desegmentize);
Datum LWGEOM_line_desegmentize(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *ret;
	LWGEOM *igeom = NULL, *ogeom = NULL;

	POSTGIS_DEBUG(2, "LWGEOM_line_desegmentize.");

	igeom = lwgeom_from_gserialized(geom);
	ogeom = lwgeom_unstroke(igeom);
	lwgeom_free(igeom);

	if (ogeom == NULL)
	{
		PG_FREE_IF_COPY(geom, 0);
		PG_RETURN_NULL();
	}

	ret = geometry_serialize(ogeom);
	lwgeom_free(ogeom);
	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_POINTER(ret);
}

/*******************************************************************************
 * End PG_FUNCTIONs
 ******************************************************************************/
