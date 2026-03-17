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

static char *get_pgtype(uint8_t column_type) {
	switch (column_type) {
	case flatgeobuf_column_type_bool:
		return "boolean";
	case flatgeobuf_column_type_byte:
	case flatgeobuf_column_type_ubyte:
		return "smallint";
	case flatgeobuf_column_type_short:
		return "smallint";
	case flatgeobuf_column_type_int:
		return "integer";
	case flatgeobuf_column_type_uint:
	case flatgeobuf_column_type_long:
	case flatgeobuf_column_type_ulong:
		return "bigint";
	case flatgeobuf_column_type_float:
		return "real";
	case flatgeobuf_column_type_double:
		return "double precision";
	case flatgeobuf_column_type_datetime:
		return "timestamptz";
	case flatgeobuf_column_type_string:
		return "text";
	case flatgeobuf_column_type_binary:
		return "bytea";
	case flatgeobuf_column_type_json:
		return "jsonb";
	}
	elog(ERROR, "unknown column_type %d", column_type);
}

PG_FUNCTION_INFO_V1(pgis_tablefromflatgeobuf);
Datum pgis_tablefromflatgeobuf(PG_FUNCTION_ARGS)
{
	struct flatgeobuf_decode_ctx *ctx;
	text *schema_input;
	char *schema;
	text *table_input;
	char *table;
	bytea *data;
	uint16_t i;
	StringInfoData sql;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();
	if (PG_ARGISNULL(1))
		PG_RETURN_NULL();

	schema_input = PG_GETARG_TEXT_P(0);
	schema = text_to_cstring(schema_input);

	table_input = PG_GETARG_TEXT_P(1);
	table = text_to_cstring(table_input);

	data = PG_GETARG_BYTEA_PP(2);

	ctx = palloc0(sizeof(*ctx));
	ctx->ctx = palloc0(sizeof(flatgeobuf_ctx));
	ctx->ctx->size = VARSIZE_ANY_EXHDR(data);
	POSTGIS_DEBUGF(3, "bytea data size is %lld", ctx->ctx->size);
	ctx->ctx->buf = lwalloc(ctx->ctx->size);
	memcpy(ctx->ctx->buf, VARDATA_ANY(data), ctx->ctx->size);
	ctx->ctx->offset = 0;

	flatgeobuf_check_magicbytes(ctx);
	flatgeobuf_decode_header(ctx->ctx);

	initStringInfo(&sql);
	appendStringInfo(&sql, "create table %s.%s (id int, geom geometry",
		quote_identifier(schema), quote_identifier(table));

	POSTGIS_DEBUGF(2, "found %d columns", ctx->ctx->columns_size);
	for (i = 0; i < ctx->ctx->columns_size; i++) {
		flatgeobuf_column *column = ctx->ctx->columns[i];
		const char *name = column->name;
		uint8_t column_type = column->type;
		char *pgtype = get_pgtype(column_type);
		appendStringInfo(&sql, ", %s %s", quote_identifier(name), pgtype);
	}

	appendStringInfoChar(&sql, ')');

	POSTGIS_DEBUGF(3, "sql: %s", sql.data);

	if (SPI_connect() != SPI_OK_CONNECT)
		elog(ERROR, "Failed to connect SPI");

	if (SPI_execute(sql.data, false, 0) != SPI_OK_UTILITY)
		elog(ERROR, "Failed to create table");

	if (SPI_finish() != SPI_OK_FINISH)
		elog(ERROR, "Failed to finish SPI");

	pfree(sql.data);

	POSTGIS_DEBUG(3, "finished");

	PG_RETURN_NULL();
}

// https://stackoverflow.com/questions/11740256/refactor-a-pl-pgsql-function-to-return-the-output-of-various-select-queries
PG_FUNCTION_INFO_V1(pgis_fromflatgeobuf);
Datum pgis_fromflatgeobuf(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;

	TupleDesc tupdesc;
	bytea *data;
	MemoryContext oldcontext;

	struct flatgeobuf_decode_ctx *ctx;

	if (SRF_IS_FIRSTCALL()) {
		funcctx = SRF_FIRSTCALL_INIT();
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		funcctx->max_calls = 0;

		if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("first argument of function must be composite type")));

		data = PG_GETARG_BYTEA_PP(1);

		ctx = palloc0(sizeof(*ctx));
		ctx->tupdesc = tupdesc;
		ctx->ctx = palloc0(sizeof(flatgeobuf_ctx));
		ctx->ctx->size = VARSIZE_ANY_EXHDR(data);
		POSTGIS_DEBUGF(3, "VARSIZE_ANY_EXHDR %lld", ctx->ctx->size);
		ctx->ctx->buf = palloc(ctx->ctx->size);
		memcpy(ctx->ctx->buf, VARDATA_ANY(data), ctx->ctx->size);
		ctx->ctx->offset = 0;
		ctx->done = false;
		ctx->fid = 0;

		funcctx->user_fctx = ctx;

		if (ctx->ctx->size == 0) {
			POSTGIS_DEBUG(2, "no data");
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}

		flatgeobuf_check_magicbytes(ctx);
		flatgeobuf_decode_header(ctx->ctx);

		POSTGIS_DEBUGF(2, "header decoded now at offset %lld", ctx->ctx->offset);

		if (ctx->ctx->size == ctx->ctx->offset) {
			POSTGIS_DEBUGF(2, "no feature data offset %lld", ctx->ctx->offset);
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}

		// TODO: get table and verify structure against header
		MemoryContextSwitchTo(oldcontext);
	}

	funcctx = SRF_PERCALL_SETUP();
	ctx = funcctx->user_fctx;

	if (!ctx->done) {
		flatgeobuf_decode_row(ctx);
		POSTGIS_DEBUG(2, "Calling SRF_RETURN_NEXT");
		SRF_RETURN_NEXT(funcctx, ctx->result);
	} else {
		POSTGIS_DEBUG(2, "Calling SRF_RETURN_DONE");
		SRF_RETURN_DONE(funcctx);
	}
}
