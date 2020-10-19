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

static char *get_pgtype(ColumnType_enum_t column_type) {
	switch (column_type) {
	case ColumnType_Int:
		return "int";
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
	char *format;
	char *sql;
	bytea *data;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();
	if (PG_ARGISNULL(1))
		PG_RETURN_NULL();

	schema_input = PG_GETARG_TEXT_P(0);
	schema = text_to_cstring(schema_input);

	table_input = PG_GETARG_TEXT_P(1);
	table = text_to_cstring(table_input);

	data = PG_GETARG_BYTEA_PP(2);
	
	ctx = palloc(sizeof(*ctx));
	ctx->size = VARSIZE_ANY_EXHDR(data);
	ctx->buf = (uint8_t *) VARDATA_ANY(data);
	ctx->offset = 0;

	flatgeobuf_check_magicbytes(ctx);
	flatgeobuf_decode_header(ctx);

	char **column_defs = palloc(sizeof(char *) * ctx->columns_len);
	size_t column_defs_total_len = 0;
	POSTGIS_DEBUGF(2, "flatgeobuf: pgis_tablefromflatgeobuf found %ld columns", ctx->columns_len);
	for (int i = 0; i < ctx->columns_len; i++) {
		Column_table_t column = Column_vec_at(ctx->columns, i);
		flatbuffers_string_t name = Column_name(column);
		ColumnType_enum_t column_type = Column_type(column);
		char *pgtype = get_pgtype(column_type);
		size_t len = strlen(name) + strlen(pgtype) + 1; 
		column_defs[i] = palloc0(sizeof(char) * len);
		strcat(column_defs[i], name);
		strcat(column_defs[i], " ");
		strcat(column_defs[i], pgtype);
		column_defs_total_len += len;
	}
	char *column_defs_str = palloc0(sizeof(char) * column_defs_total_len + (ctx->columns_len * 2) + 2);
	if (ctx->columns_len > 0)
		strcat(column_defs_str, ", ");
	for (int i = 0; i < ctx->columns_len; i++) {
		strcat(column_defs_str, column_defs[i]);
		if (i < ctx->columns_len - 1)
			strcat(column_defs_str, ", ");
	}

	POSTGIS_DEBUGF(2, "flatgeobuf: pgis_tablefromflatgeobuf column_defs_str %s", column_defs_str);

	// TODO: parameterize temp

	format = "create temp table if not exists %s (id int, geom geometry%s) on commit drop";
	//format = "create table %s.%s (id int, geom geometry)";
	sql = palloc(strlen(format) + strlen(schema) + strlen(table));
	
	sprintf(sql, format, table, column_defs_str);
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
