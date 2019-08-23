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
 * Copyright 2009 Mark Cave-Ayland <mark.cave-ayland@siriusit.co.uk>
 * Copyright 2009-2017 Paul Ramsey <pramsey@cleverelephant.ca>
 * Copyright 2018 Darafei Praliaskouski <me@komzpa.net>
 *
 **********************************************************************/

#include "postgres.h"
#include "fmgr.h"
#include "funcapi.h"
#include "access/htup_details.h"

#include "../postgis_config.h"
#include "lwgeom_pg.h"
#include "liblwgeom.h"

#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>


Datum ST_Hexagon(PG_FUNCTION_ARGS);
Datum ST_Square(PG_FUNCTION_ARGS);
Datum ST_ShapeGrid(PG_FUNCTION_ARGS);

/* ********* ********* ********* ********* ********* ********* ********* ********* */

/* XXX TO DO: what about negative sizes? */

/* Ratio of hexagon edge length to height = 2*cos(pi/6) */
#define HXR 1.7320508075688774

static const double hex_x[] = {0.0, 1.0, 1.5,     1.0, 0.0, -0.5,     0.0};
static const double hex_y[] = {0.0, 0.0, 0.5*HXR, HXR, HXR,  0.5*HXR, 0.0};

static LWGEOM *
hexagon(double origin_x, double origin_y, double edge, int cell_i, int cell_j, uint32_t srid)
{
	double height = edge * HXR;
	POINT4D pt;
	POINTARRAY **ppa = lwalloc(sizeof(POINTARRAY*));
	POINTARRAY *pa = ptarray_construct(0, 0, 7);
	uint32_t i;

	double offset_x = origin_x + (1.5 * edge * cell_i);
	double offset_y = origin_y + (height * cell_j) + (0.5 * height * (cell_i % 2));
	pt.z = pt.m = 0.0;

	for (i = 0; i < 7; ++i)
	{
		pt.x = edge * hex_x[i] + offset_x;
		pt.y = edge * hex_y[i] + offset_y;
		ptarray_set_point4d(pa, i, &pt);
	}

	ppa[0] = pa;
	return lwpoly_as_lwgeom(lwpoly_construct(srid, NULL, 1 /* nrings */, ppa));
}

static int
hexagon_grid_size(double edge, const GBOX *gbox, int *cell_width, int *cell_height)
{
	double hex_height = edge * HXR;
	double bounds_width  = gbox->xmax - gbox->xmin;
	double bounds_height = gbox->ymax - gbox->ymin;
	int width  = (int)floor(bounds_width/(1.5*edge)) + 1;
	int height = (int)floor(bounds_height/hex_height) + 1;
	if (cell_width) *cell_width = width;
	if (cell_height) *cell_height = height;
	return width * height;
}

/* ********* ********* ********* ********* ********* ********* ********* ********* */

static const double sqr_x[] = {0.0, 1.0, 1.0, 0.0, 0.0};
static const double sqr_y[] = {0.0, 0.0, 1.0, 1.0, 0.0};

static LWGEOM *
square(double origin_x, double origin_y, double E, int cell_i, int cell_j, uint32_t srid)
{
	POINT4D pt;
	POINTARRAY **ppa = lwalloc(sizeof(POINTARRAY*));
	POINTARRAY *pa = ptarray_construct(0, 0, 5);
	uint32_t i;

	double offset_x = origin_x + (E * cell_i);
	double offset_y = origin_y + (E * cell_j);
	pt.z = pt.m = 0.0;

	for (i = 0; i < 5; ++i)
	{
		pt.x = E * sqr_x[i] + offset_x;
		pt.y = E * sqr_y[i] + offset_y;
		ptarray_set_point4d(pa, i, &pt);
	}

	ppa[0] = pa;
	return lwpoly_as_lwgeom(lwpoly_construct(srid, NULL, 1 /* nrings */, ppa));
}

static int
square_grid_size(double edge, const GBOX *gbox, int *cell_width, int *cell_height)
{
	int width  = (int)floor((gbox->xmax - gbox->xmin)/edge) + 1;
	int height = (int)floor((gbox->ymax - gbox->ymin)/edge) + 1;
	if (cell_width) *cell_width = width;
	if (cell_height) *cell_height = height;
	return width * height;
}

/* ********* ********* ********* ********* ********* ********* ********* ********* */

typedef enum
{
	SHAPE_SQUARE,
	SHAPE_HEXAGON,
	SHAPE_TRIANGLE
} GeometryShape;

typedef struct GeometryGridState
{
	int cell_current;
	int cell_count;
	int cell_width;
	int cell_height;
	GBOX bounds;
	double size;
	uint32_t srid;
	GeometryShape cell_shape;
}
GeometryGridState;

/**
* ST_HexagonGrid(size double default 1.0,
*            bounds geometry default 'LINESTRING(0 0, 100 100)')
*/
PG_FUNCTION_INFO_V1(ST_ShapeGrid);
Datum ST_ShapeGrid(PG_FUNCTION_ARGS)
{
	int i, j;
	FuncCallContext *funcctx;
	MemoryContext oldcontext, newcontext;

	GSERIALIZED *gbounds;
	GeometryGridState *state;

	bool isnull[3] = {0,0,0}; /* needed to say no value is null */
	Datum tuple_arr[3]; /* used to construct the composite return value */
	HeapTuple tuple;
	Datum result; /* the actual composite return value */

	if (SRF_IS_FIRSTCALL())
	{
		const char *func_name;
		double bounds_width, bounds_height;
		int gbounds_is_empty;
		GBOX bounds;
		double size;
		funcctx = SRF_FIRSTCALL_INIT();

		/* get a local copy of what we're doing a dump points on */
		gbounds = PG_GETARG_GSERIALIZED_P(1);
		size = PG_GETARG_FLOAT8(0);

		gbounds_is_empty = (gserialized_get_gbox_p(gbounds, &bounds) == LW_FAILURE);
		bounds_width = bounds.xmax - bounds.xmin;
		bounds_height = bounds.ymax - bounds.ymin;

		/* quick opt-out if we get nonsensical inputs  */
		if (size <= 0.0 || gbounds_is_empty ||
		    bounds_width <= 0.0 || bounds_height <= 0.0)
		{
			funcctx = SRF_PERCALL_SETUP();
			SRF_RETURN_DONE(funcctx);
		}

		newcontext = funcctx->multi_call_memory_ctx;
		oldcontext = MemoryContextSwitchTo(newcontext);

		/* Create function state */
		state = lwalloc(sizeof(GeometryGridState));
		state->cell_current = 0;
		state->bounds = bounds;
		state->srid = gserialized_get_srid(gbounds);
		state->size = size;

		func_name = get_func_name(fcinfo->flinfo->fn_oid);
		if (strcmp(func_name, "st_hexagongrid") == 0)
		{
			state->cell_shape = SHAPE_HEXAGON;
			state->cell_count = hexagon_grid_size(size, &state->bounds,
			                                      &state->cell_width, &state->cell_height);
		}
		else if (strcmp(func_name, "st_squaregrid") == 0)
		{
			state->cell_shape = SHAPE_SQUARE;
			state->cell_count = square_grid_size(size, &state->bounds,
			                                     &state->cell_width, &state->cell_height);
		}
		else
		{
			ereport(ERROR, (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				errmsg("%s called from unsupported functional context '%s'", __func__, func_name)));
		}

		funcctx->user_fctx = state;

		/* get tuple description for return type */
		if (get_call_result_type(fcinfo, 0, &funcctx->tuple_desc) != TYPEFUNC_COMPOSITE)
		{
			ereport(ERROR, (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				errmsg("set-valued function called in context that cannot accept a set")));
		}

		BlessTupleDesc(funcctx->tuple_desc);
		MemoryContextSwitchTo(oldcontext);
	}

	/* stuff done on every call of the function */
	funcctx = SRF_PERCALL_SETUP();
	newcontext = funcctx->multi_call_memory_ctx;

	/* get state */
	state = funcctx->user_fctx;

	/* Stop when we've used up all the grid squares */
	if (state->cell_current >= state->cell_count)
	{
		SRF_RETURN_DONE(funcctx);
	}

	i = state->cell_current % state->cell_width;
	j = state->cell_current / state->cell_width;

	/* Generate geometry */
	LWGEOM *lwgeom;
	switch (state->cell_shape)
	{
		case SHAPE_HEXAGON:
			lwgeom = hexagon(state->bounds.xmin, state->bounds.ymin, state->size, i, j, state->srid);
			break;
		case SHAPE_SQUARE:
			lwgeom = square(state->bounds.xmin, state->bounds.ymin, state->size, i, j, state->srid);
			break;
		default:
			ereport(ERROR, (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				errmsg("%s called from with unsupported shape '%d'", __func__, state->cell_shape)));
	}

	tuple_arr[0] = PointerGetDatum(geometry_serialize(lwgeom));
	lwfree(lwgeom);

	/* Store grid cell coordinates */
	tuple_arr[1] = Int32GetDatum(i);
	tuple_arr[2] = Int32GetDatum(j);

	tuple = heap_form_tuple(funcctx->tuple_desc, tuple_arr, isnull);
	result = HeapTupleGetDatum(tuple);
	state->cell_current++;
	SRF_RETURN_NEXT(funcctx, result);
}

/**
* ST_Hexagon(size double,
*            origin geometry default 'POINT(0 0)',
*            int cellx default 0, int celly default 0)
*/
PG_FUNCTION_INFO_V1(ST_Hexagon);
Datum ST_Hexagon(PG_FUNCTION_ARGS)
{
	LWPOINT *lwpt;
	double size = PG_GETARG_FLOAT8(0);
	GSERIALIZED *gorigin = PG_GETARG_GSERIALIZED_P(1);
	GSERIALIZED *ghex;
	int cell_i = PG_GETARG_INT32(2);
	int cell_j = PG_GETARG_INT32(3);
	LWGEOM *lwhex;
	LWGEOM *lworigin = lwgeom_from_gserialized(gorigin);

	if (lwgeom_is_empty(lworigin))
	{
		elog(ERROR, "%s: origin point is empty", __func__);
		PG_RETURN_NULL();
	}

	lwpt = lwgeom_as_lwpoint(lworigin);
	if (!lwpt)
	{
		elog(ERROR, "%s: origin argument is not a point", __func__);
		PG_RETURN_NULL();
	}

	lwhex = hexagon(lwpoint_get_x(lwpt), lwpoint_get_y(lwpt),
	                size, cell_i, cell_j,
		            lwgeom_get_srid(lworigin));

	ghex = geometry_serialize(lwhex);
	PG_FREE_IF_COPY(gorigin, 1);
	PG_RETURN_POINTER(ghex);
}


/**
* ST_Square(size double,
*           origin geometry default 'POINT(0 0)',
*           int cellx default 0, int celly default 0)
*/
PG_FUNCTION_INFO_V1(ST_Square);
Datum ST_Square(PG_FUNCTION_ARGS)
{
	LWPOINT *lwpt;
	double size = PG_GETARG_FLOAT8(0);
	GSERIALIZED *gorigin = PG_GETARG_GSERIALIZED_P(1);
	GSERIALIZED *gsqr;
	int cell_i = PG_GETARG_INT32(2);
	int cell_j = PG_GETARG_INT32(3);
	LWGEOM *lwsqr;
	LWGEOM *lworigin = lwgeom_from_gserialized(gorigin);

	if (lwgeom_is_empty(lworigin))
	{
		elog(ERROR, "%s: origin point is empty", __func__);
		PG_RETURN_NULL();
	}

	lwpt = lwgeom_as_lwpoint(lworigin);
	if (!lwpt)
	{
		elog(ERROR, "%s: origin argument is not a point", __func__);
		PG_RETURN_NULL();
	}

	lwsqr = square(lwpoint_get_x(lwpt), lwpoint_get_y(lwpt),
	               size, cell_i, cell_j,
		           lwgeom_get_srid(lworigin));

	gsqr = geometry_serialize(lwsqr);
	PG_FREE_IF_COPY(gorigin, 1);
	PG_RETURN_POINTER(gsqr);
}
