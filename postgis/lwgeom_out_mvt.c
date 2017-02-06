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
 * Copyright (C) 2016-2017 Björn Harrtell <bjorn@wololo.org>
 *
 **********************************************************************/

#include "postgres.h"
#include "utils/builtins.h"
#include "executor/spi.h"
#include "../postgis_config.h"
#include "lwgeom_pg.h"
#include "lwgeom_log.h"
#include "liblwgeom.h"
#include "mvt.h"

#ifdef HAVE_LIBPROTOBUF
#include "vector_tile.pb-c.h"
#endif  /* HAVE_LIBPROTOBUF */

/**
 * Process input parameters and row data into state
 */
PG_FUNCTION_INFO_V1(pgis_asmvt_transfn);
Datum pgis_asmvt_transfn(PG_FUNCTION_ARGS)
{
#ifndef HAVE_LIBPROTOBUF
	lwerror("Missing libprotobuf-c");
	PG_RETURN_NULL();
#else
	MemoryContext aggcontext;
	struct mvt_agg_context *ctx;

	if (!AggCheckCallContext(fcinfo, &aggcontext))
		lwerror("pgis_asmvt_transfn: called in non-aggregate context");
	MemoryContextSwitchTo(aggcontext);

	if (PG_ARGISNULL(0)) {
		ctx = palloc(sizeof(*ctx));
		if (PG_ARGISNULL(1))
			lwerror("pgis_asmvt_transfn: parameter name cannot be null");
		ctx->name = text_to_cstring(PG_GETARG_TEXT_P(1));
		if (PG_ARGISNULL(2))
			lwerror("pgis_asmvt_transfn: parameter bounds cannot be null");
		ctx->bounds = (GBOX *) PG_GETARG_POINTER(2);
		ctx->extent = PG_ARGISNULL(3) ? 4096 : PG_GETARG_INT32(3);
		ctx->buffer = PG_ARGISNULL(4) ? 0 : PG_GETARG_INT32(4);
		ctx->clip_geoms = PG_ARGISNULL(5) ? true : PG_GETARG_BOOL(5);
		if (PG_ARGISNULL(6))
			lwerror("pgis_asmvt_transfn: parameter geom_name cannot be null");
		ctx->geom_name = text_to_cstring(PG_GETARG_TEXT_P(6));
		mvt_agg_init_context(ctx);
	} else {
		ctx = (struct mvt_agg_context *) PG_GETARG_POINTER(0);
	}

	if (!type_is_rowtype(get_fn_expr_argtype(fcinfo->flinfo, 7)))
		lwerror("pgis_asmvt_transfn: parameter row cannot be other than a rowtype");
	ctx->row = PG_GETARG_HEAPTUPLEHEADER(7);

	mvt_agg_transfn(ctx);
	PG_RETURN_POINTER(ctx);
#endif
}

/**
 * Encode final state to Mapbox Vector Tile
 */
PG_FUNCTION_INFO_V1(pgis_asmvt_finalfn);
Datum pgis_asmvt_finalfn(PG_FUNCTION_ARGS)
{
#ifndef HAVE_LIBPROTOBUF
	lwerror("Missing libprotobuf-c");
	PG_RETURN_NULL();
#else
	struct mvt_agg_context *ctx;
	if (!AggCheckCallContext(fcinfo, NULL))
		lwerror("pgis_asmvt_finalfn called in non-aggregate context");

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	ctx = (struct mvt_agg_context *) PG_GETARG_POINTER(0);
	uint8_t *buf = mvt_agg_finalfn(ctx);
	PG_RETURN_BYTEA_P(buf);
#endif
}
