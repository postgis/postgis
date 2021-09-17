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
 * Copyright (C) 2021 Björn Harrtell <bjorn@wololo.org>
 *
 **********************************************************************/

/**
 * @file
 * Geobuf export functions
 */

#include "postgres.h"
#include "utils/builtins.h"
#include "executor/spi.h"

#include "../postgis_config.h"
#include "lwgeom_pg.h"
#include "lwgeom_log.h"
#include "liblwgeom.h"
#include "flatgeobuf.h"

/**
 * Process input parameters and row data into state
 */
PG_FUNCTION_INFO_V1(pgis_asflatgeobuf_transfn);
Datum pgis_asflatgeobuf_transfn(PG_FUNCTION_ARGS)
{
	MemoryContext aggcontext, oldcontext;
	char *geom_name = NULL;
	bool create_index = false;
	flatgeobuf_agg_ctx *ctx;

	POSTGIS_DEBUG(2, "calling pgis_asflatgeobuf_transfn");

	/* We need to initialize the internal cache to access it later via postgis_oid() */
	postgis_initialize_cache();

	if (!AggCheckCallContext(fcinfo, &aggcontext))
		elog(ERROR, "pgis_asflatgeobuf_transfn: called in non-aggregate context");
	oldcontext = MemoryContextSwitchTo(aggcontext);

	if (PG_ARGISNULL(0)) {
		if (PG_NARGS() > 2 && !PG_ARGISNULL(2))
			create_index = PG_GETARG_BOOL(2);
		if (PG_NARGS() > 3 && !PG_ARGISNULL(3))
			geom_name = text_to_cstring(PG_GETARG_TEXT_P(3));
		ctx = flatgeobuf_agg_ctx_init(geom_name, create_index);
	} else {
		ctx = (flatgeobuf_agg_ctx *) PG_GETARG_POINTER(0);
	}

	if (!type_is_rowtype(get_fn_expr_argtype(fcinfo->flinfo, 1)))
		elog(ERROR, "pgis_asflatgeobuf_transfn: parameter row cannot be other than a rowtype");
	ctx->row = PG_GETARG_HEAPTUPLEHEADER(1);

	flatgeobuf_agg_transfn(ctx);
	MemoryContextSwitchTo(oldcontext);
	PG_RETURN_POINTER(ctx);
}

/**
 * Encode final state to FlatGeobuf
 */
PG_FUNCTION_INFO_V1(pgis_asflatgeobuf_finalfn);
Datum pgis_asflatgeobuf_finalfn(PG_FUNCTION_ARGS)
{
	uint8_t *buf;
	flatgeobuf_agg_ctx *ctx;
	if (!AggCheckCallContext(fcinfo, NULL))
		elog(ERROR, "pgis_asflatgeobuf_finalfn called in non-aggregate context");

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	ctx = (flatgeobuf_agg_ctx *) PG_GETARG_POINTER(0);
	buf = flatgeobuf_agg_finalfn(ctx);
	PG_RETURN_BYTEA_P(buf);
}
