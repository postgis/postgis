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

#include "geobuf.h"

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
#include "geobuf.h"

/**
 * Process input parameters and row data into state
 */
PG_FUNCTION_INFO_V1(pgis_asgeobuf_transfn);
Datum pgis_asgeobuf_transfn(PG_FUNCTION_ARGS)
{
#if !(defined HAVE_LIBPROTOBUF)
	elog(ERROR, "ST_AsGeobuf: Compiled without protobuf-c support");
	PG_RETURN_NULL();
#else
	MemoryContext aggcontext;
	struct geobuf_agg_context *ctx;

	/* We need to initialize the internal cache to access it later via postgis_oid() */
	postgis_initialize_cache(fcinfo);

	if (!AggCheckCallContext(fcinfo, &aggcontext))
		elog(ERROR, "pgis_asgeobuf_transfn: called in non-aggregate context");
	MemoryContextSwitchTo(aggcontext);

	if (PG_ARGISNULL(0)) {
		ctx = palloc(sizeof(*ctx));

		ctx->geom_name = NULL;
		if (PG_NARGS() > 2 && !PG_ARGISNULL(2))
			ctx->geom_name = text_to_cstring(PG_GETARG_TEXT_P(2));
		geobuf_agg_init_context(ctx);
	} else {
		ctx = (struct geobuf_agg_context *) PG_GETARG_POINTER(0);
	}

	if (!type_is_rowtype(get_fn_expr_argtype(fcinfo->flinfo, 1)))
		elog(ERROR, "pgis_asgeobuf_transfn: parameter row cannot be other than a rowtype");
	ctx->row = PG_GETARG_HEAPTUPLEHEADER(1);

	geobuf_agg_transfn(ctx);
	PG_RETURN_POINTER(ctx);
#endif
}

/**
 * Encode final state to Geobuf
 */
PG_FUNCTION_INFO_V1(pgis_asgeobuf_finalfn);
Datum pgis_asgeobuf_finalfn(PG_FUNCTION_ARGS)
{
#if !(defined HAVE_LIBPROTOBUF)
	elog(ERROR, "ST_AsGeobuf: Compiled without protobuf-c support");
	PG_RETURN_NULL();
#else
	uint8_t *buf;
	struct geobuf_agg_context *ctx;
	if (!AggCheckCallContext(fcinfo, NULL))
		elog(ERROR, "pgis_asmvt_finalfn called in non-aggregate context");

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	ctx = (struct geobuf_agg_context *) PG_GETARG_POINTER(0);
	buf = geobuf_agg_finalfn(ctx);
	PG_RETURN_BYTEA_P(buf);
#endif
}
