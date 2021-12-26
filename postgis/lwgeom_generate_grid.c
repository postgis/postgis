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
 * Copyright 2020 Paul Ramsey <pramsey@cleverelephant.ca>
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

typedef enum
{
	SHAPE_SQUARE,
	SHAPE_HEXAGON,
	SHAPE_TRIANGLE
} GeometryShape;

typedef struct GeometryGridState
{
	GeometryShape cell_shape;
	bool done;
	GBOX bounds;
	int32_t srid;
	double size;
	int32_t i, j;
}
GeometryGridState;

/* ********* ********* ********* ********* ********* ********* ********* ********* */

/* Ratio of hexagon edge length to edge->center vertical distance = cos(M_PI/6.0) */
#define H 0.8660254037844387

/* Build origin hexagon centered around origin point */
static const double hex_x[] = {-1.0, -0.5,  0.5, 1.0, 0.5, -0.5, -1.0};
static const double hex_y[] = {0.0, -0.5, -0.5, 0.0, 0.5, 0.5, 0.0};

static LWGEOM *
hexagon(double origin_x, double origin_y, double size, int cell_i, int cell_j, int32_t srid)
{
	POINTARRAY **ppa = lwalloc(sizeof(POINTARRAY*));
	POINTARRAY *pa = ptarray_construct(0, 0, 7);

	for (uint32_t i = 0; i < 7; ++i)
	{
		double height = size * 2 * H;
		POINT4D pt;
		pt.x = origin_x + size * (1.5 * cell_i + hex_x[i]);
		pt.y = origin_y + height * (cell_j + 0.5 * (abs(cell_i) % 2) + hex_y[i]);
		ptarray_set_point4d(pa, i, &pt);
	}

	ppa[0] = pa;
	return lwpoly_as_lwgeom(lwpoly_construct(srid, NULL, 1 /* nrings */, ppa));
}


typedef struct HexagonGridState
{
	GeometryShape cell_shape;
	bool done;
	GBOX bounds;
	int32_t srid;
	double size;
	int32_t i, j;
	int32_t column_min, column_max;
	int32_t row_min_odd, row_max_odd;
	int32_t row_min_even, row_max_even;
}
HexagonGridState;

static HexagonGridState *
hexagon_grid_state(double size, const GBOX *gbox, int32_t srid)
{
	HexagonGridState *state = palloc0(sizeof(HexagonGridState));
	double col_width = 1.5 * size;
	double row_height = size * 2 * H;

	/* fill in state */
	state->cell_shape = SHAPE_HEXAGON;
	state->size = size;
	state->srid = srid;
	state->done = false;
	state->bounds = *gbox;

	/* Column address is just width / column width, with an */
	/* adjustment to account for partial overlaps */
	state->column_min = floor(gbox->xmin / col_width);
	if(gbox->xmin - state->column_min * col_width > size)
		state->column_min++;

	state->column_max = ceil(gbox->xmax / col_width);
	if(state->column_max * col_width - gbox->xmax > size)
		state->column_max--;

	/* Row address range depends on the odd/even column we're on */
	state->row_min_even = floor(gbox->ymin/row_height + 0.5);
	state->row_max_even = floor(gbox->ymax/row_height + 0.5);
	state->row_min_odd  = floor(gbox->ymin/row_height);
	state->row_max_odd  = floor(gbox->ymax/row_height);

	/* Set initial state */
	state->i = state->column_min;
	state->j = (state->i % 2) ? state->row_min_odd : state->row_min_even;

	return state;
}

static void
hexagon_state_next(HexagonGridState *state)
{
	if (!state || state->done) return;
	/* Move up one row */
	state->j++;
	/* Off the end, increment column counter, reset row counter back to (appropriate) minimum */
	if (state->j > ((state->i % 2) ? state->row_max_odd : state->row_max_even))
	{
		state->i++;
		state->j = ((state->i % 2) ? state->row_min_odd : state->row_min_even);
	}
	/* Column counter over max means we have used up all combinations */
	if (state->i > state->column_max)
	{
		state->done = true;
	}
}

/* ********* ********* ********* ********* ********* ********* ********* ********* */

static LWGEOM *
square(double origin_x, double origin_y, double size, int cell_i, int cell_j, int32_t srid)
{
	double ll_x = origin_x + (size * cell_i);
	double ll_y = origin_y + (size * cell_j);
	double ur_x = origin_x + (size * (cell_i + 1));
	double ur_y = origin_y + (size * (cell_j + 1));
	return (LWGEOM*)lwpoly_construct_envelope(srid, ll_x, ll_y, ur_x, ur_y);
}

typedef struct SquareGridState
{
	GeometryShape cell_shape;
	bool done;
	GBOX bounds;
	int32_t srid;
	double size;
	int32_t i, j;
	int32_t column_min, column_max;
	int32_t row_min, row_max;
}
SquareGridState;

static SquareGridState *
square_grid_state(double size, const GBOX *gbox, int32_t srid)
{
	SquareGridState *state = palloc0(sizeof(SquareGridState));

	/* fill in state */
	state->cell_shape = SHAPE_SQUARE;
	state->size = size;
	state->srid = srid;
	state->done = false;
	state->bounds = *gbox;

	state->column_min = floor(gbox->xmin / size);
	state->column_max = floor(gbox->xmax / size);
	state->row_min = floor(gbox->ymin / size);
	state->row_max = floor(gbox->ymax / size);
	state->i = state->column_min;
	state->j = state->row_min;
	return state;
}

static void
square_state_next(SquareGridState *state)
{
	if (!state || state->done) return;
	/* Move up one row */
	state->j++;
	/* Off the end, increment column counter, reset row counter back to (appropriate) minimum */
	if (state->j > state->row_max)
	{
		state->i++;
		state->j = state->row_min;
	}
	/* Column counter over max means we have used up all combinations */
	if (state->i > state->column_max)
	{
		state->done = true;
	}
}

/**
* ST_HexagonGrid(size float8, bounds geometry)
* ST_SquareGrid(size float8, bounds geometry)
*/
PG_FUNCTION_INFO_V1(ST_ShapeGrid);
Datum ST_ShapeGrid(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;

	GSERIALIZED *gbounds;
	GeometryGridState *state;

	LWGEOM *lwgeom;
	bool isnull[3] = {0,0,0}; /* needed to say no value is null */
	Datum tuple_arr[3]; /* used to construct the composite return value */
	HeapTuple tuple;
	Datum result; /* the actual composite return value */

	if (SRF_IS_FIRSTCALL())
	{
		MemoryContext oldcontext;
		const char *func_name;
		char gbounds_is_empty;
		GBOX bounds;
		double size;
		funcctx = SRF_FIRSTCALL_INIT();

		/* Get a local copy of the bounds we are going to fill with shapes */
		gbounds = PG_GETARG_GSERIALIZED_P(1);
		size = PG_GETARG_FLOAT8(0);

		gbounds_is_empty = (gserialized_get_gbox_p(gbounds, &bounds) == LW_FAILURE);

		/* quick opt-out if we get nonsensical inputs  */
		if (size <= 0.0 || gbounds_is_empty)
		{
			funcctx = SRF_PERCALL_SETUP();
			SRF_RETURN_DONE(funcctx);
		}

		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		/*
		* Support both hexagon and square grids with one function,
		* by checking the calling signature up front.
		*/
		func_name = get_func_name(fcinfo->flinfo->fn_oid);
		if (strcmp(func_name, "st_hexagongrid") == 0)
		{
			state = (GeometryGridState*)hexagon_grid_state(size, &bounds, gserialized_get_srid(gbounds));
		}
		else if (strcmp(func_name, "st_squaregrid") == 0)
		{
			state = (GeometryGridState*)square_grid_state(size, &bounds, gserialized_get_srid(gbounds));
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

	/* get state */
	state = funcctx->user_fctx;

	/* Stop when we've used up all the grid squares */
	if (state->done)
	{
		SRF_RETURN_DONE(funcctx);
	}

	/* Store grid cell coordinates */
	tuple_arr[1] = Int32GetDatum(state->i);
	tuple_arr[2] = Int32GetDatum(state->j);

	/* Generate geometry */
	switch (state->cell_shape)
	{
		case SHAPE_HEXAGON:
			lwgeom = hexagon(0.0, 0.0, state->size, state->i, state->j, state->srid);
			/* Increment to next cell */
			hexagon_state_next((HexagonGridState*)state);
			break;
		case SHAPE_SQUARE:
			lwgeom = square(0.0, 0.0, state->size, state->i, state->j, state->srid);
			/* Increment to next cell */
			square_state_next((SquareGridState*)state);
			break;
		default:
			ereport(ERROR, (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				errmsg("%s called from with unsupported shape '%d'", __func__, state->cell_shape)));
	}

	tuple_arr[0] = PointerGetDatum(geometry_serialize(lwgeom));
	lwfree(lwgeom);

	/* Form tuple and return */
	tuple = heap_form_tuple(funcctx->tuple_desc, tuple_arr, isnull);
	result = HeapTupleGetDatum(tuple);
	SRF_RETURN_NEXT(funcctx, result);
}

/**
* ST_Hexagon(size double, cell_i integer default 0, cell_j integer default 0, origin geometry default 'POINT(0 0)')
*/
PG_FUNCTION_INFO_V1(ST_Hexagon);
Datum ST_Hexagon(PG_FUNCTION_ARGS)
{
	LWPOINT *lwpt;
	double size = PG_GETARG_FLOAT8(0);
	GSERIALIZED *ghex;
	int cell_i = PG_GETARG_INT32(1);
	int cell_j = PG_GETARG_INT32(2);
	GSERIALIZED *gorigin = PG_GETARG_GSERIALIZED_P(3);
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
	PG_FREE_IF_COPY(gorigin, 3);
	PG_RETURN_POINTER(ghex);
}


/**
* ST_Square(size double, cell_i integer, cell_j integer, origin geometry default 'POINT(0 0)')
*/
PG_FUNCTION_INFO_V1(ST_Square);
Datum ST_Square(PG_FUNCTION_ARGS)
{
	LWPOINT *lwpt;
	double size = PG_GETARG_FLOAT8(0);
	GSERIALIZED *gsqr;
	int cell_i = PG_GETARG_INT32(1);
	int cell_j = PG_GETARG_INT32(2);
	GSERIALIZED *gorigin = PG_GETARG_GSERIALIZED_P(3);
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
	PG_FREE_IF_COPY(gorigin, 3);
	PG_RETURN_POINTER(gsqr);
}
