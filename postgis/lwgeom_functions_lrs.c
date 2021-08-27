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
 * Copyright (C) 2001-2005 Refractions Research Inc.
 *
 **********************************************************************/


#include <math.h>

#include "postgres.h"
#include "fmgr.h"

#include "../postgis_config.h"
#include "liblwgeom.h"
#include "lwgeom_pg.h"

/*
* Add a measure dimension to a line, interpolating linearly from the
* start value to the end value.
* ST_AddMeasure(Geometry, StartMeasure, EndMeasure) returns Geometry
*/
Datum ST_AddMeasure(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_AddMeasure);
Datum ST_AddMeasure(PG_FUNCTION_ARGS)
{
	GSERIALIZED *gin = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *gout;
	double start_measure = PG_GETARG_FLOAT8(1);
	double end_measure = PG_GETARG_FLOAT8(2);
	LWGEOM *lwin, *lwout;
	int type = gserialized_get_type(gin);

	/* Raise an error if input is not a linestring or multilinestring */
	if ( type != LINETYPE && type != MULTILINETYPE )
	{
		lwpgerror("Only LINESTRING and MULTILINESTRING are supported");
		PG_RETURN_NULL();
	}

	lwin = lwgeom_from_gserialized(gin);
	if ( type == LINETYPE )
		lwout = (LWGEOM*)lwline_measured_from_lwline((LWLINE*)lwin, start_measure, end_measure);
	else
		lwout = (LWGEOM*)lwmline_measured_from_lwmline((LWMLINE*)lwin, start_measure, end_measure);

	lwgeom_free(lwin);

	if ( lwout == NULL )
		PG_RETURN_NULL();

	gout = geometry_serialize(lwout);
	lwgeom_free(lwout);

	PG_RETURN_POINTER(gout);
}


/*
* Locate a point along a feature based on a measure value.
* ST_LocateAlong(Geometry, Measure, [Offset]) returns Geometry
*/
Datum ST_LocateAlong(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_LocateAlong);
Datum ST_LocateAlong(PG_FUNCTION_ARGS)
{
	GSERIALIZED *gin = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *gout;
	LWGEOM *lwin = NULL, *lwout = NULL;
	double measure = PG_GETARG_FLOAT8(1);
	double offset = PG_GETARG_FLOAT8(2);

	lwin = lwgeom_from_gserialized(gin);
	lwout = lwgeom_locate_along(lwin, measure, offset);
	lwgeom_free(lwin);
	PG_FREE_IF_COPY(gin, 0);

	if ( ! lwout )
		PG_RETURN_NULL();

	gout = geometry_serialize(lwout);
	lwgeom_free(lwout);

	PG_RETURN_POINTER(gout);
}


/*
* Locate the portion of a line between the specified measures
*/
Datum ST_LocateBetween(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_LocateBetween);
Datum ST_LocateBetween(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom_in = PG_GETARG_GSERIALIZED_P(0);
	double from = PG_GETARG_FLOAT8(1);
	double to = PG_GETARG_FLOAT8(2);
	double offset = PG_GETARG_FLOAT8(3);
	LWCOLLECTION *geom_out = NULL;
	LWGEOM *line_in = NULL;
	static char ordinate = 'M'; /* M */

	if ( ! gserialized_has_m(geom_in) )
	{
		elog(ERROR,"This function only accepts geometries that have an M dimension.");
		PG_RETURN_NULL();
	}

	/* This should be a call to ST_LocateAlong! */
	if ( to == from )
	{
		PG_RETURN_DATUM(DirectFunctionCall3(ST_LocateAlong, PG_GETARG_DATUM(0), PG_GETARG_DATUM(1), PG_GETARG_DATUM(3)));
	}

	line_in = lwgeom_from_gserialized(geom_in);
	geom_out = lwgeom_clip_to_ordinate_range(line_in,  ordinate, from, to, offset);
	lwgeom_free(line_in);
	PG_FREE_IF_COPY(geom_in, 0);

	if ( ! geom_out )
	{
		elog(ERROR,"lwline_clip_to_ordinate_range returned null");
		PG_RETURN_NULL();
	}

	PG_RETURN_POINTER(geometry_serialize((LWGEOM*)geom_out));
}

/*
* Locate the portion of a line between the specified elevations
*/
Datum ST_LocateBetweenElevations(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_LocateBetweenElevations);
Datum ST_LocateBetweenElevations(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom_in = PG_GETARG_GSERIALIZED_P(0);
	double from = PG_GETARG_FLOAT8(1);
	double to = PG_GETARG_FLOAT8(2);
	LWCOLLECTION *geom_out = NULL;
	LWGEOM *line_in = NULL;
	static char ordinate = 'Z'; /* Z */
	static double offset = 0.0;

	if ( ! gserialized_has_z(geom_in) )
	{
		elog(ERROR, "This function only accepts geometries with Z dimensions.");
		PG_RETURN_NULL();
	}

	line_in = lwgeom_from_gserialized(geom_in);
	geom_out = lwgeom_clip_to_ordinate_range(line_in,  ordinate, from, to, offset);
	lwgeom_free(line_in);
	PG_FREE_IF_COPY(geom_in, 0);

	if ( ! geom_out )
	{
		elog(ERROR,"lwline_clip_to_ordinate_range returned null");
		PG_RETURN_NULL();
	}

	PG_RETURN_POINTER(geometry_serialize((LWGEOM*)geom_out));
}


Datum ST_InterpolatePoint(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_InterpolatePoint);
Datum ST_InterpolatePoint(PG_FUNCTION_ARGS)
{
	GSERIALIZED *gser_line = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *gser_point = PG_GETARG_GSERIALIZED_P(1);
	LWGEOM *lwline;
	LWPOINT *lwpoint;

	if ( gserialized_get_type(gser_line) != LINETYPE )
	{
		elog(ERROR,"ST_InterpolatePoint: 1st argument isn't a line");
		PG_RETURN_NULL();
	}
	if ( gserialized_get_type(gser_point) != POINTTYPE )
	{
		elog(ERROR,"ST_InterpolatePoint: 2st argument isn't a point");
		PG_RETURN_NULL();
	}

	gserialized_error_if_srid_mismatch(gser_line, gser_point, __func__);

	if ( ! gserialized_has_m(gser_line) )
	{
		elog(ERROR,"ST_InterpolatePoint only accepts geometries that have an M dimension");
		PG_RETURN_NULL();
	}

	lwpoint = lwgeom_as_lwpoint(lwgeom_from_gserialized(gser_point));
	lwline = lwgeom_from_gserialized(gser_line);

	PG_RETURN_FLOAT8(lwgeom_interpolate_point(lwline, lwpoint));
}


Datum LWGEOM_line_locate_point(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(LWGEOM_line_locate_point);
Datum LWGEOM_line_locate_point(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *geom2 = PG_GETARG_GSERIALIZED_P(1);
	LWLINE *lwline;
	LWPOINT *lwpoint;
	POINTARRAY *pa;
	POINT4D p, p_proj;
	double ret;

	if ( gserialized_get_type(geom1) != LINETYPE )
	{
		elog(ERROR,"line_locate_point: 1st arg isn't a line");
		PG_RETURN_NULL();
	}
	if ( gserialized_get_type(geom2) != POINTTYPE )
	{
		elog(ERROR,"line_locate_point: 2st arg isn't a point");
		PG_RETURN_NULL();
	}

	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	lwline = lwgeom_as_lwline(lwgeom_from_gserialized(geom1));
	lwpoint = lwgeom_as_lwpoint(lwgeom_from_gserialized(geom2));

	pa = lwline->points;
	lwpoint_getPoint4d_p(lwpoint, &p);

	ret = ptarray_locate_point(pa, &p, NULL, &p_proj);

	PG_RETURN_FLOAT8(ret);
}
