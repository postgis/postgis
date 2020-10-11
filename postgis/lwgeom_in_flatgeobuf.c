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
 * Copyright (C) 2020 Björn Harrtell <bjorn@wololo.org>
 *
 **********************************************************************/


#include <assert.h>

#include "postgres.h"

#include "../postgis_config.h"
#include "lwgeom_pg.h"
#include "liblwgeom.h"
#include "lwgeom_cache.h"
#include "funcapi.h"
#include <executor/spi.h>
#include <utils/builtins.h>
#include "flatgeobuf.h"

PG_FUNCTION_INFO_V1(pgis_tablefromflatgeobuf);
Datum pgis_tablefromflatgeobuf(PG_FUNCTION_ARGS)
{
	text *schema_input;
	char *schema;
	text *table_input;
	char *table;
	char *format;
	char *sql;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();
	if (PG_ARGISNULL(1))
		PG_RETURN_NULL();

	schema_input = PG_GETARG_TEXT_P(0);
	schema = text_to_cstring(schema_input);

	table_input = PG_GETARG_TEXT_P(1);
	table = text_to_cstring(table_input);

	// TODO: parameterize temp

	format = "create temp table if not exists %s (id int, geom geometry) on commit drop";
	//format = "create table %s.%s (id int, geom geometry)";
	sql = palloc(strlen(format) + strlen(schema) + strlen(table));
	
	sprintf(sql, format, table);
	//sprintf(sql, format, schema, table);

	if (SPI_connect() != SPI_OK_CONNECT)
		elog(ERROR, "Failed to connect SPI");

	if (SPI_execute(sql, false, 0) != SPI_OK_UTILITY)
		elog(ERROR, "Failed to create table");

	if (SPI_finish() != SPI_OK_FINISH)
		elog(ERROR, "Failed to finish SPI");

	PG_RETURN_NULL();
}

// https://stackoverflow.com/questions/11740256/refactor-a-pl-pgsql-function-to-return-the-output-of-various-select-queries
PG_FUNCTION_INFO_V1(pgis_fromflatgeobuf);
Datum pgis_fromflatgeobuf(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;
	int                  call_cntr;
    int                  max_calls;
	
	TupleDesc tupdesc;
	bytea *data;
	HeapTuple tuple;
	//AttInMetadata *attinmeta;
	MemoryContext oldcontext;
	Datum result;

	struct flatgeobuf_decode_ctx *ctx;

	if (SRF_IS_FIRSTCALL()) {
		funcctx = SRF_FIRSTCALL_INIT();
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		funcctx->max_calls = 0;

		if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
            ereport(ERROR,
                    (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
                     errmsg("function returning record called in context "
                            "that cannot accept type record")));

		data = PG_GETARG_BYTEA_PP(1);

		ctx = palloc(sizeof(*ctx));
		ctx->tupdesc = tupdesc;
		ctx->size = VARSIZE_ANY_EXHDR(data);
		ctx->buf = (uint8_t *) VARDATA_ANY(data);
		ctx->offset = 0;
		ctx->done = false;
		ctx->fid = 0;

		funcctx->user_fctx = ctx;

		if (ctx->size == 0)
			SRF_RETURN_DONE(funcctx);

		flatgeobuf_check_magicbytes(ctx);
		flatgeobuf_decode_header(ctx);

		// no feature data
		if (ctx->size == ctx->offset)
			SRF_RETURN_DONE(funcctx);

		// TODO: get table and verify structure against header

		MemoryContextSwitchTo(oldcontext);
	}

	funcctx = SRF_PERCALL_SETUP();
	ctx = funcctx->user_fctx;

	if (!ctx->done) {
		flatgeobuf_decode_feature(ctx);
		SRF_RETURN_NEXT(funcctx, ctx->result);
	} else {
		SRF_RETURN_DONE(funcctx);
	}
}
