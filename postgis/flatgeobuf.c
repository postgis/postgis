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

#include <math.h>
#include "flatgeobuf.h"
#include "pgsql_compat.h"
#include "funcapi.h"
#include "parser/parse_type.h"
#include "pgtime.h"
#include "utils/timestamp.h"
#include "miscadmin.h"
#include "utils/date.h"
#include "utils/datetime.h"
#include "utils/jsonb.h"

static uint8_t get_column_type(Oid typoid) {
	switch (typoid)
	{
	case BOOLOID:
		return flatgeobuf_column_type_bool;
	case INT2OID:
		return flatgeobuf_column_type_short;
	case INT4OID:
		return flatgeobuf_column_type_int;
	case INT8OID:
		return flatgeobuf_column_type_long;
	case FLOAT4OID:
		return flatgeobuf_column_type_float;
	case FLOAT8OID:
		return flatgeobuf_column_type_double;
	case TEXTOID:
	case VARCHAROID:
		return flatgeobuf_column_type_string;
	case JSONBOID:
		return flatgeobuf_column_type_json;
	case BYTEAOID:
		return flatgeobuf_column_type_binary;
	case DATEOID:
	case TIMEOID:
	case TIMESTAMPOID:
	case TIMESTAMPTZOID:
		return flatgeobuf_column_type_datetime;
	}
	elog(ERROR, "flatgeobuf: get_column_type: '%d' column type not supported",
		typoid);
}

static void inspect_table(struct flatgeobuf_agg_ctx *ctx)
{
	flatgeobuf_column *c;
	flatgeobuf_column **columns;
	uint32_t columns_size = 0;
	Oid tupType = HeapTupleHeaderGetTypeId(ctx->row);
	int32 tupTypmod = HeapTupleHeaderGetTypMod(ctx->row);
	TupleDesc tupdesc = lookup_rowtype_tupdesc(tupType, tupTypmod);
	int natts = tupdesc->natts;
	bool geom_found = false;

	POSTGIS_DEBUG(2, "calling inspect_table");

	columns = palloc(sizeof(flatgeobuf_column *) * natts);
	ctx->tupdesc = tupdesc;

	// inspect columns
	// NOTE: last element will be unused if geom attr is found
	for (int i = 0; i < natts; i++) {
		Oid typoid = getBaseType(TupleDescAttr(tupdesc, i)->atttypid);
		const char *key = TupleDescAttr(tupdesc, i)->attname.data;
		POSTGIS_DEBUGF(2, "inspecting column definition for %s with oid %d", key, typoid);
		if (ctx->geom_name == NULL) {
			if (!geom_found && typoid == postgis_oid(GEOMETRYOID)) {
				ctx->geom_index = i;
				geom_found = true;
				continue;
			}
		} else {
			if (!geom_found && strcmp(key, ctx->geom_name) == 0) {
				ctx->geom_index = i;
				geom_found = true;
				continue;
			}
		}
		POSTGIS_DEBUGF(2, "creating column definition for %s with oid %d", key, typoid);

		c = (flatgeobuf_column *) palloc0(sizeof(flatgeobuf_column));
		c->name = pstrdup(key);
		c->type = get_column_type(typoid);
		columns[columns_size] = c;
		columns_size++;
	}

	if (!geom_found)
		elog(ERROR, "no geom column found");

	if (columns_size > 0) {
		ctx->ctx->columns = columns;
		ctx->ctx->columns_size = columns_size;
	}
}

// ensure properties has room for at least size
static void ensure_properties_size(struct flatgeobuf_agg_ctx *ctx, size_t size)
{
	if (ctx->ctx->properties_size == 0) {
		ctx->ctx->properties_size = 1024 * 4;
		POSTGIS_DEBUGF(2, "flatgeobuf: properties buffer to size %d", ctx->ctx->properties_size);
		ctx->ctx->properties = palloc(ctx->ctx->properties_size);
	}
	if (ctx->ctx->properties_size < size) {
		ctx->ctx->properties_size = ctx->ctx->properties_size * 2;
		POSTGIS_DEBUGF(2, "flatgeobuf: reallocating properties buffer to size %d", ctx->ctx->properties_size);
		ctx->ctx->properties = repalloc(ctx->ctx->properties, ctx->ctx->properties_size);
		ensure_properties_size(ctx, size);
	}
}

// ensure items have room for at least ctx->ctx->features_count + 1
static void ensure_items_len(struct flatgeobuf_agg_ctx *ctx)
{
	if (ctx->ctx->features_count == 0) {
		ctx->ctx->items_len = 32;
		ctx->ctx->items = palloc(sizeof(flatgeobuf_item *) * ctx->ctx->items_len);
	}
	if (ctx->ctx->items_len < (ctx->ctx->features_count + 1)) {
		ctx->ctx->items_len = ctx->ctx->items_len * 2;
		POSTGIS_DEBUGF(2, "flatgeobuf: reallocating items to len %ld", ctx->ctx->items_len);
		ctx->ctx->items = repalloc(ctx->ctx->items, sizeof(flatgeobuf_item *) * ctx->ctx->items_len);
		ensure_items_len(ctx);
	}
}

static void encode_properties(flatgeobuf_agg_ctx *ctx)
{
	uint16_t ci = 0;
	size_t offset = 0;
	Datum datum;
	bool isnull;
	Oid typoid;
	uint32_t len;
	uint32_t i;

	uint8_t byte_value;
	int16_t short_value;
	int32_t int_value;
	int64_t long_value;
	float float_value;
	double double_value;
	char *string_value;

	//Jsonb *jb;

	for (i = 0; i < (uint32_t) ctx->tupdesc->natts; i++) {
		if (ctx->geom_index == i)
			continue;
		datum = GetAttributeByNum(ctx->row, i + 1, &isnull);
		if (isnull)
			continue;
		ensure_properties_size(ctx, offset + sizeof(ci));
		memcpy(ctx->ctx->properties + offset, &ci, sizeof(ci));
		offset += sizeof(ci);
		typoid = getBaseType(TupleDescAttr(ctx->tupdesc, i)->atttypid);
		switch (typoid) {
		case BOOLOID:
			byte_value = DatumGetBool(datum) ? 1 : 0;
			ensure_properties_size(ctx, offset + sizeof(byte_value));
			memcpy(ctx->ctx->properties + offset, &byte_value, sizeof(byte_value));
			offset += sizeof(byte_value);
			break;
		case INT2OID:
			short_value = DatumGetInt16(datum);
			ensure_properties_size(ctx, offset + sizeof(short_value));
			memcpy(ctx->ctx->properties + offset, &short_value, sizeof(short_value));
			offset += sizeof(short_value);
			break;
		case INT4OID:
			int_value = DatumGetInt32(datum);
			ensure_properties_size(ctx, offset + sizeof(int_value));
			memcpy(ctx->ctx->properties + offset, &int_value, sizeof(int_value));
			offset += sizeof(int_value);
			break;
		case INT8OID:
			long_value = DatumGetInt64(datum);
			ensure_properties_size(ctx, offset + sizeof(long_value));
			memcpy(ctx->ctx->properties + offset, &long_value, sizeof(long_value));
			offset += sizeof(long_value);
			break;
		case FLOAT4OID:
			float_value = DatumGetFloat4(datum);
			ensure_properties_size(ctx, offset + sizeof(float_value));
			memcpy(ctx->ctx->properties + offset, &float_value, sizeof(float_value));
			offset += sizeof(float_value);
			break;
		case FLOAT8OID:
			double_value = DatumGetFloat8(datum);
			ensure_properties_size(ctx, offset + sizeof(double_value));
			memcpy(ctx->ctx->properties + offset, &double_value, sizeof(double_value));
			offset += sizeof(double_value);
			break;
		case TEXTOID:
			string_value = text_to_cstring(DatumGetTextP(datum));
			len = strlen(string_value);
			ensure_properties_size(ctx, offset + sizeof(len));
			memcpy(ctx->ctx->properties + offset, &len, sizeof(len));
			offset += sizeof(len);
			ensure_properties_size(ctx, offset + len);
			memcpy(ctx->ctx->properties + offset, string_value, len);
			offset += len;
			break;
		case TIMESTAMPTZOID: {
			struct pg_tm tm;
			fsec_t fsec;
			int tz;
			const char *tzn = NULL;
			TimestampTz timestamp;
			timestamp = DatumGetTimestampTz(datum);
			timestamp2tm(timestamp, &tz, &tm, &fsec, &tzn, NULL);
			string_value = palloc(MAXDATELEN + 1);
			EncodeDateTime(&tm, fsec, true, tz, tzn, USE_ISO_DATES, string_value);
			len = strlen(string_value);
			ensure_properties_size(ctx, offset + sizeof(len));
			memcpy(ctx->ctx->properties + offset, &len, sizeof(len));
			offset += sizeof(len);
			ensure_properties_size(ctx, offset + len);
			memcpy(ctx->ctx->properties + offset, string_value, len);
			offset += len;
			break;
		}
		// TODO: handle date/time types
		// case JSONBOID:
		// 	jb = DatumGetJsonbP(datum);
		// 	string_value = JsonbToCString(NULL, &jb->root, VARSIZE(jb));
		// 	len = strlen(string_value);
		// 	memcpy(data + offset, &len, sizeof(len));
		// 	offset += sizeof(len);
		// 	memcpy(data + offset, string_value, len);
		// 	offset += len;
		// 	break;
		}
		ci++;
	}

	if (offset > 0) {
		POSTGIS_DEBUGF(3, "offset %ld", offset);
		ctx->ctx->properties_len = offset;
	}
}

void flatgeobuf_check_magicbytes(struct flatgeobuf_decode_ctx *ctx)
{
	uint8_t *buf = ctx->ctx->buf + ctx->ctx->offset;
	uint32_t i;

	for (i = 0; i < FLATGEOBUF_MAGICBYTES_SIZE / 2; i++)
		if (buf[i] != flatgeobuf_magicbytes[i])
			elog(ERROR, "Data is not FlatGeobuf");
	ctx->ctx->offset += FLATGEOBUF_MAGICBYTES_SIZE;
}

static void decode_properties(struct flatgeobuf_decode_ctx *ctx, Datum *values, bool *isnull)
{
	uint16_t i, ci;
	flatgeobuf_column *column;
	uint8_t type;
	uint32_t offset = 0;
	uint8_t *data = ctx->ctx->properties;
	uint32_t size = ctx->ctx->properties_len;

	POSTGIS_DEBUGF(3, "flatgeobuf: decode_properties from byte array with length %d at offset %d", size, offset);

	// TODO: init isnull

	if (size > 0 && size < (sizeof(uint16_t) + sizeof(uint8_t)))
		elog(ERROR, "flatgeobuf: decode_properties: Unexpected properties data size %d", size);
	while (offset + 1 < size) {
		if (offset + sizeof(uint16_t) > size)
			elog(ERROR, "flatgeobuf: decode_properties: Unexpected offset %d", offset);
		memcpy(&i, data + offset, sizeof(uint16_t));
		ci = i + 2;
		offset += sizeof(uint16_t);
		if (i >= ctx->ctx->columns_size)
			elog(ERROR, "flatgeobuf: decode_properties: Column index %hu out of range", i);
		column = ctx->ctx->columns[i];
		type = column->type;
		isnull[ci] = false;
		switch (type) {
		case flatgeobuf_column_type_bool: {
			uint8_t value;
			if (offset + sizeof(uint8_t) > size)
				elog(ERROR, "flatgeobuf: decode_properties: Invalid size for bool value");
			memcpy(&value, data + offset, sizeof(uint8_t));
			values[ci] = BoolGetDatum(value);
			offset += sizeof(uint8_t);
			break;
		}
		case flatgeobuf_column_type_byte: {
			int8_t value;
			if (offset + sizeof(int8_t) > size)
				elog(ERROR, "flatgeobuf: decode_properties: Invalid size for byte value");
			memcpy(&value, data + offset, sizeof(int8_t));
			values[ci] = Int8GetDatum(value);
			offset += sizeof(int8_t);
			break;
		}
		case flatgeobuf_column_type_ubyte: {
			uint8_t value;
			if (offset + sizeof(uint8_t) > size)
				elog(ERROR, "flatgeobuf: decode_properties: Invalid size for ubyte value");
			memcpy(&value, data + offset, sizeof(uint8_t));
			values[ci] = UInt8GetDatum(value);
			offset += sizeof(uint8_t);
			break;
		}
		case flatgeobuf_column_type_short: {
			int16_t value;
			if (offset + sizeof(int16_t) > size)
				elog(ERROR, "flatgeobuf: decode_properties: Invalid size for short value");
			memcpy(&value, data + offset, sizeof(int16_t));
			values[ci] = Int16GetDatum(value);
			offset += sizeof(int16_t);
			break;
		}
		case flatgeobuf_column_type_ushort: {
			uint16_t value;
			if (offset + sizeof(uint16_t) > size)
				elog(ERROR, "flatgeobuf: decode_properties: Invalid size for ushort value");
			memcpy(&value, data + offset, sizeof(uint16_t));
			values[ci] = UInt16GetDatum(value);
			offset += sizeof(uint16_t);
			break;
		}
		case flatgeobuf_column_type_int: {
			int32_t value;
			if (offset + sizeof(int32_t) > size)
				elog(ERROR, "flatgeobuf: decode_properties: Invalid size for int value");
			memcpy(&value, data + offset, sizeof(int32_t));
			values[ci] = Int32GetDatum(value);
			offset += sizeof(int32_t);
			break;
		}
		case flatgeobuf_column_type_uint: {
			uint32_t value;
			if (offset + sizeof(uint32_t) > size)
				elog(ERROR, "flatgeobuf: decode_properties: Invalid size for uint value");
			memcpy(&value, data + offset, sizeof(uint32_t));
			values[ci] = UInt32GetDatum(value);
			offset += sizeof(uint32_t);
			break;
		}
		case flatgeobuf_column_type_long: {
			int64_t value;
			if (offset + sizeof(int64_t) > size)
				elog(ERROR, "flatgeobuf: decode_properties: Invalid size for long value");
			memcpy(&value, data + offset, sizeof(int64_t));
			values[ci] = Int64GetDatum(value);
			offset += sizeof(int64_t);
			break;
		}
		case flatgeobuf_column_type_ulong: {
			uint64_t value;
			if (offset + sizeof(uint64_t) > size)
				elog(ERROR, "flatgeobuf: decode_properties: Invalid size for ulong value");
			memcpy(&value, data + offset, sizeof(uint64_t));
			values[ci] = UInt64GetDatum(value);
			offset += sizeof(uint64_t);
			break;
		}
		case flatgeobuf_column_type_float: {
			float value;
			if (offset + sizeof(float) > size)
				elog(ERROR, "flatgeobuf: decode_properties: Invalid size for float value");
			memcpy(&value, data + offset, sizeof(float));
			values[ci] = Float4GetDatum(value);
			offset += sizeof(float);
			break;
		}
		case flatgeobuf_column_type_double: {
			double value;
			if (offset + sizeof(double) > size)
				elog(ERROR, "flatgeobuf: decode_properties: Invalid size for double value");
			memcpy(&value, data + offset, sizeof(double));
			values[ci] = Float8GetDatum(value);
			offset += sizeof(double);
			break;
		}
		case flatgeobuf_column_type_string: {
			uint32_t len;
			if (offset + sizeof(len) > size)
				elog(ERROR, "flatgeobuf: decode_properties: Invalid size for string value");
			memcpy(&len, data + offset, sizeof(uint32_t));
			offset += sizeof(len);
			values[ci] = PointerGetDatum(cstring_to_text_with_len((const char *) data + offset, len));
			offset += len;
			break;
		}
		case flatgeobuf_column_type_datetime: {
			uint32_t len;
			char *buf;
			char workbuf[MAXDATELEN + MAXDATEFIELDS];
			char *field[MAXDATEFIELDS];
			int ftype[MAXDATEFIELDS];
			int dtype;
			int nf;
			struct pg_tm tt, *tm = &tt;
			fsec_t fsec;
			int tzp;
			TimestampTz dttz;
#if POSTGIS_PGSQL_VERSION >= 160
			DateTimeErrorExtra extra;
#endif
			if (offset + sizeof(len) > size)
				elog(ERROR, "flatgeobuf: decode_properties: Invalid size for string value");
			memcpy(&len, data + offset, sizeof(uint32_t));
			offset += sizeof(len);
			buf = palloc0(len + 1);
			memcpy(buf, (const char *) data + offset, len);
			ParseDateTime((const char *) buf, workbuf, sizeof(workbuf), field, ftype, MAXDATEFIELDS, &nf);

#if POSTGIS_PGSQL_VERSION >= 160
			DecodeDateTime(field, ftype, nf, &dtype, tm, &fsec, &tzp, &extra);
#else
			DecodeDateTime(field, ftype, nf, &dtype, tm, &fsec, &tzp);
#endif
			tm2timestamp(tm, fsec, &tzp, &dttz);
			values[ci] = TimestampTzGetDatum(dttz);
			offset += len;
			break;
		}
		// TODO: find out how to handle string to jsonb Datum
		// case FlatGeobuf_ColumnType_Json:
		// 	if (offset + sizeof(len) > size)
		// 		elog(ERROR, "flatgeobuf: decode_properties: Invalid size for json value");
		// 	len = *((uint32_t *)(data + offset));
		// 	offset += sizeof(len);
		// 	values[ci] = jsonb_from_cstring((const char *) data + offset, len);
		// 	offset += len;
		// 	break;
		default:
			elog(ERROR, "flatgeobuf: decode_properties: Unknown type %d", type);
		}
	}

}

void flatgeobuf_decode_row(struct flatgeobuf_decode_ctx *ctx)
{
	HeapTuple heapTuple;
	uint32_t natts = ctx->tupdesc->natts;

	Datum *values = palloc0(natts * sizeof(Datum *));
	bool *isnull = palloc0(natts * sizeof(bool *));

	values[0] = Int32GetDatum(ctx->fid);

	if (flatgeobuf_decode_feature(ctx->ctx))
		elog(ERROR, "flatgeobuf_decode_feature: unsuccessful");

	if (ctx->ctx->lwgeom != NULL) {
		values[1] = PointerGetDatum(geometry_serialize(ctx->ctx->lwgeom));
	} else {
		POSTGIS_DEBUG(3, "geometry is null");
		isnull[1] = true;
	}

	if (natts > 2 && ctx->ctx->properties_len > 0)
		decode_properties(ctx, values, isnull);

	heapTuple = heap_form_tuple(ctx->tupdesc, values, isnull);
	ctx->result = HeapTupleGetDatum(heapTuple);
	ctx->fid++;

	POSTGIS_DEBUGF(3, "fid now %d", ctx->fid);

	if (ctx->ctx->offset == ctx->ctx->size) {
		POSTGIS_DEBUGF(3, "reached end at %ld", ctx->ctx->offset);
		ctx->done = true;
	}
}

/**
 * Initialize aggregation context.
 */
struct flatgeobuf_agg_ctx *flatgeobuf_agg_ctx_init(const char *geom_name, const bool create_index)
{
	struct flatgeobuf_agg_ctx *ctx;
	size_t size = VARHDRSZ + FLATGEOBUF_MAGICBYTES_SIZE;
	ctx = palloc0(sizeof(*ctx));
	ctx->ctx = palloc0(sizeof(flatgeobuf_ctx));
	ctx->ctx->buf = lwalloc(size);
	memcpy(ctx->ctx->buf + VARHDRSZ, flatgeobuf_magicbytes, FLATGEOBUF_MAGICBYTES_SIZE);
	ctx->geom_name = geom_name;
	ctx->geom_index = 0;
	ctx->ctx->features_count = 0;
	ctx->ctx->offset = size;
	ctx->tupdesc = NULL;
	ctx->ctx->create_index = create_index;
	return ctx;
}

/**
 * Aggregation step.
 *
 * Expands features array if needed by a factor of 2.
 * Allocates a new feature, increment feature counter and
 * encode properties into it.
 */
void flatgeobuf_agg_transfn(struct flatgeobuf_agg_ctx *ctx)
{
	LWGEOM *lwgeom = NULL;
	bool isnull = false;
	Datum datum;
	GSERIALIZED *gs;

	if (ctx->ctx->features_count == 0)
		inspect_table(ctx);

	datum = GetAttributeByNum(ctx->row, ctx->geom_index + 1, &isnull);
	if (!isnull) {
		gs = (GSERIALIZED *) PG_DETOAST_DATUM_COPY(datum);
		lwgeom = lwgeom_from_gserialized(gs);
	}
	ctx->ctx->lwgeom = lwgeom;

	if (ctx->ctx->features_count == 0)
		flatgeobuf_encode_header(ctx->ctx);

	encode_properties(ctx);
	if (ctx->ctx->create_index)
		ensure_items_len(ctx);
	flatgeobuf_encode_feature(ctx->ctx);
}

/**
 * Finalize aggregation.
 *
 * Encode into Data message and return it packed as a bytea.
 */
uint8_t *flatgeobuf_agg_finalfn(struct flatgeobuf_agg_ctx *ctx)
{
	POSTGIS_DEBUGF(3, "called at offset %ld", ctx->ctx->offset);
	if (ctx == NULL)
		flatgeobuf_agg_ctx_init(NULL, false);
	// header only result
	if (ctx->ctx->features_count == 0) {
		flatgeobuf_encode_header(ctx->ctx);
	} else if (ctx->ctx->create_index) {
		ctx->ctx->index_node_size = 16;
		flatgeobuf_create_index(ctx->ctx);
	}
	if (ctx->tupdesc != NULL)
		ReleaseTupleDesc(ctx->tupdesc);
	SET_VARSIZE(ctx->ctx->buf, ctx->ctx->offset);
	POSTGIS_DEBUGF(3, "returning at offset %ld", ctx->ctx->offset);
	return ctx->ctx->buf;
}
