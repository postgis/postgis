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

#include <string.h>

#include "mvt.h"

#ifdef HAVE_LIBPROTOBUF

#if POSTGIS_PGSQL_VERSION >= 94
#include "utils/jsonb.h"
#endif

#if POSTGIS_PGSQL_VERSION < 110
/* See trac ticket #3867 */
# define DatumGetJsonbP DatumGetJsonb
#endif

#include "uthash.h"

#define FEATURES_CAPACITY_INITIAL 50

enum mvt_cmd_id
{
	CMD_MOVE_TO = 1,
	CMD_LINE_TO = 2,
	CMD_CLOSE_PATH = 7
};

enum mvt_type
{
	MVT_POINT = 1,
	MVT_LINE = 2,
	MVT_RING = 3
};

struct mvt_kv_key
{
	char *name;
	uint32_t id;
	UT_hash_handle hh;
};

struct mvt_kv_string_value
{
	char *string_value;
	uint32_t id;
	UT_hash_handle hh;
};

struct mvt_kv_float_value
{
	float float_value;
	uint32_t id;
	UT_hash_handle hh;
};

struct mvt_kv_double_value
{
	double double_value;
	uint32_t id;
	UT_hash_handle hh;
};

struct mvt_kv_uint_value
{
	uint64_t uint_value;
	uint32_t id;
	UT_hash_handle hh;
};

struct mvt_kv_sint_value
{
	int64_t sint_value;
	uint32_t id;
	UT_hash_handle hh;
};

struct mvt_kv_bool_value
{
	protobuf_c_boolean bool_value;
	uint32_t id;
	UT_hash_handle hh;
};

static inline uint32_t c_int(enum mvt_cmd_id id, uint32_t count)
{
	return (id & 0x7) | (count << 3);
}

static inline uint32_t p_int(int32_t value)
{
	return (value << 1) ^ (value >> 31);
}

static uint32_t encode_ptarray(__attribute__((__unused__)) mvt_agg_context *ctx,
			       enum mvt_type type, POINTARRAY *pa, uint32_t *buffer,
			       int32_t *px, int32_t *py)
{
	uint32_t offset = 0;
	uint32_t i, c = 0;
	int32_t dx, dy, x, y;
	const POINT2D *p;

	/* loop points and add to buffer */
	for (i = 0; i < pa->npoints; i++)
	{
		/* move offset for command */
		if (i == 0 || (i == 1 && type > MVT_POINT))
			offset++;
		/* skip closing point for rings */
		if (type == MVT_RING && i == pa->npoints - 1)
			break;
		p = getPoint2d_cp(pa, i);
		x = p->x;
		y = p->y;
		dx = x - *px;
		dy = y - *py;
		buffer[offset++] = p_int(dx);
		buffer[offset++] = p_int(dy);
		*px = x;
		*py = y;
		c++;
	}

	/* determine initial move and eventual line command */
	if (type == MVT_POINT)
	{
		/* point or multipoint, use actual number of point count */
		buffer[0] = c_int(CMD_MOVE_TO, c);
	}
	else
	{
		/* line or polygon, assume count 1 */
		buffer[0] = c_int(CMD_MOVE_TO, 1);
		/* line command with move point subtracted from count */
		buffer[3] = c_int(CMD_LINE_TO, c - 1);
	}

	/* add close command if ring */
	if (type == MVT_RING)
		buffer[offset++] = c_int(CMD_CLOSE_PATH, 1);

	return offset;
}

static uint32_t encode_ptarray_initial(mvt_agg_context *ctx,
				       enum mvt_type type,
				       POINTARRAY *pa, uint32_t *buffer)
{
	int32_t px = 0, py = 0;
	return encode_ptarray(ctx, type, pa, buffer, &px, &py);
}

static void encode_point(mvt_agg_context *ctx, LWPOINT *point)
{
	VectorTile__Tile__Feature *feature = ctx->feature;
	feature->type = VECTOR_TILE__TILE__GEOM_TYPE__POINT;
	feature->has_type = 1;
	feature->n_geometry = 3;
	feature->geometry = palloc(sizeof(*feature->geometry) * 3);
	encode_ptarray_initial(ctx, MVT_POINT, point->point, feature->geometry);
}

static void encode_mpoint(mvt_agg_context *ctx, LWMPOINT *mpoint)
{
	size_t c;
	VectorTile__Tile__Feature *feature = ctx->feature;
	// NOTE: inefficient shortcut LWMPOINT->LWLINE
	LWLINE *lwline = lwline_from_lwmpoint(mpoint->srid, mpoint);
	feature->type = VECTOR_TILE__TILE__GEOM_TYPE__POINT;
	feature->has_type = 1;
	c = 1 + lwline->points->npoints * 2;
	feature->geometry = palloc(sizeof(*feature->geometry) * c);
	feature->n_geometry = encode_ptarray_initial(ctx, MVT_POINT,
		lwline->points, feature->geometry);
}

static void encode_line(mvt_agg_context *ctx, LWLINE *lwline)
{
	size_t c;
	VectorTile__Tile__Feature *feature = ctx->feature;
	feature->type = VECTOR_TILE__TILE__GEOM_TYPE__LINESTRING;
	feature->has_type = 1;
	c = 2 + lwline->points->npoints * 2;
	feature->geometry = palloc(sizeof(*feature->geometry) * c);
	feature->n_geometry = encode_ptarray_initial(ctx, MVT_LINE,
		lwline->points, feature->geometry);
}

static void encode_mline(mvt_agg_context *ctx, LWMLINE *lwmline)
{
	uint32_t i;
	int32_t px = 0, py = 0;
	size_t c = 0, offset = 0;
	VectorTile__Tile__Feature *feature = ctx->feature;
	feature->type = VECTOR_TILE__TILE__GEOM_TYPE__LINESTRING;
	feature->has_type = 1;
	for (i = 0; i < lwmline->ngeoms; i++)
		c += 2 + lwmline->geoms[i]->points->npoints * 2;
	feature->geometry = palloc(sizeof(*feature->geometry) * c);
	for (i = 0; i < lwmline->ngeoms; i++)
		offset += encode_ptarray(ctx, MVT_LINE,
			lwmline->geoms[i]->points,
			feature->geometry + offset, &px, &py);
	feature->n_geometry = offset;
}

static void encode_poly(mvt_agg_context *ctx, LWPOLY *lwpoly)
{
	uint32_t i;
	int32_t px = 0, py = 0;
	size_t c = 0, offset = 0;
	VectorTile__Tile__Feature *feature = ctx->feature;
	feature->type = VECTOR_TILE__TILE__GEOM_TYPE__POLYGON;
	feature->has_type = 1;
	for (i = 0; i < lwpoly->nrings; i++)
		c += 3 + ((lwpoly->rings[i]->npoints - 1) * 2);
	feature->geometry = palloc(sizeof(*feature->geometry) * c);
	for (i = 0; i < lwpoly->nrings; i++)
		offset += encode_ptarray(ctx, MVT_RING,
			lwpoly->rings[i],
			feature->geometry + offset, &px, &py);
	feature->n_geometry = offset;
}

static void encode_mpoly(mvt_agg_context *ctx, LWMPOLY *lwmpoly)
{
	uint32_t i, j;
	int32_t px = 0, py = 0;
	size_t c = 0, offset = 0;
	LWPOLY *poly;
	VectorTile__Tile__Feature *feature = ctx->feature;
	feature->type = VECTOR_TILE__TILE__GEOM_TYPE__POLYGON;
	feature->has_type = 1;
	for (i = 0; i < lwmpoly->ngeoms; i++)
		for (j = 0; poly = lwmpoly->geoms[i], j < poly->nrings; j++)
			c += 3 + ((poly->rings[j]->npoints - 1) * 2);
	feature->geometry = palloc(sizeof(*feature->geometry) * c);
	for (i = 0; i < lwmpoly->ngeoms; i++)
		for (j = 0; poly = lwmpoly->geoms[i], j < poly->nrings; j++)
			offset += encode_ptarray(ctx, MVT_RING,
				poly->rings[j],	feature->geometry + offset,
				&px, &py);
	feature->n_geometry = offset;
}

static void encode_geometry(mvt_agg_context *ctx, LWGEOM *lwgeom)
{
	int type = lwgeom->type;

	switch (type)
	{
	case POINTTYPE:
		return encode_point(ctx, (LWPOINT*)lwgeom);
	case LINETYPE:
		return encode_line(ctx, (LWLINE*)lwgeom);
	case POLYGONTYPE:
		return encode_poly(ctx, (LWPOLY*)lwgeom);
	case MULTIPOINTTYPE:
		return encode_mpoint(ctx, (LWMPOINT*)lwgeom);
	case MULTILINETYPE:
		return encode_mline(ctx, (LWMLINE*)lwgeom);
	case MULTIPOLYGONTYPE:
		return encode_mpoly(ctx, (LWMPOLY*)lwgeom);
	default: elog(ERROR, "encode_geometry: '%s' geometry type not supported",
		lwtype_name(type));
	}
}

static TupleDesc get_tuple_desc(mvt_agg_context *ctx)
{
	Oid tupType = HeapTupleHeaderGetTypeId(ctx->row);
	int32 tupTypmod = HeapTupleHeaderGetTypMod(ctx->row);
	TupleDesc tupdesc = lookup_rowtype_tupdesc(tupType, tupTypmod);
	return tupdesc;
}

static uint32_t get_key_index_with_size(mvt_agg_context *ctx, const char *name, size_t size)
{
	struct mvt_kv_key *kv;
	HASH_FIND(hh, ctx->keys_hash, name, size, kv);
	if (!kv)
		return UINT32_MAX;
	return kv->id;
}

static uint32_t add_key(mvt_agg_context *ctx, char *name)
{
	struct mvt_kv_key *kv;
	size_t size = strlen(name);
	kv = palloc(sizeof(*kv));
	kv->id = ctx->keys_hash_i++;
	kv->name = name;
	HASH_ADD_KEYPTR(hh, ctx->keys_hash, name, size, kv);
	return kv->id;
}

static void parse_column_keys(mvt_agg_context *ctx)
{
	uint32_t i, natts;
	bool geom_found = false;

	POSTGIS_DEBUG(2, "parse_column_keys called");

	ctx->column_cache.tupdesc = get_tuple_desc(ctx);
	natts = ctx->column_cache.tupdesc->natts;

	ctx->column_cache.column_keys_index = palloc(sizeof(uint32_t) * natts);
	ctx->column_cache.column_oid = palloc(sizeof(uint32_t) * natts);
	ctx->column_cache.values = palloc(sizeof(Datum) * natts);
	ctx->column_cache.nulls = palloc(sizeof(bool) * natts);

	for (i = 0; i < natts; i++)
	{
#if POSTGIS_PGSQL_VERSION < 110
		Oid typoid = getBaseType(ctx->column_cache.tupdesc->attrs[i]->atttypid);
		char *tkey = ctx->column_cache.tupdesc->attrs[i]->attname.data;
#else
		Oid typoid = getBaseType(ctx->column_cache.tupdesc->attrs[i].atttypid);
		char *tkey = ctx->column_cache.tupdesc->attrs[i].attname.data;
#endif

		ctx->column_cache.column_oid[i] = typoid;
#if POSTGIS_PGSQL_VERSION >= 94
		if (typoid == JSONBOID)
		{
			ctx->column_cache.column_keys_index[i] = UINT32_MAX;
			continue;
		}
#endif

		if (ctx->geom_name == NULL)
		{
			if (!geom_found && typoid == TypenameGetTypid("geometry"))
			{
				ctx->geom_index = i;
				geom_found = true;
				continue;
			}
		}
		else
		{
			if (!geom_found && strcmp(tkey, ctx->geom_name) == 0)
			{
				ctx->geom_index = i;
				geom_found = true;
				continue;
			}
		}

		if (ctx->id_name &&
			(ctx->id_index == UINT32_MAX) &&
			(strcmp(tkey, ctx->id_name) == 0) &&
			(typoid == INT2OID || typoid == INT4OID || typoid == INT8OID))
		{
			ctx->id_index = i;
		}
		else
		{
			ctx->column_cache.column_keys_index[i] = add_key(ctx, pstrdup(tkey));
		}
	}

	if (!geom_found)
		elog(ERROR, "parse_column_keys: no geometry column found");

	if (ctx->id_name != NULL && ctx->id_index == UINT32_MAX)
		elog(ERROR, "mvt_agg_transfn: Could not find column '%s' of integer type", ctx->id_name);
}

static void encode_keys(mvt_agg_context *ctx)
{
	struct mvt_kv_key *kv;
	size_t n_keys = ctx->keys_hash_i;
	char **keys = palloc(n_keys * sizeof(*keys));
	for (kv = ctx->keys_hash; kv != NULL; kv=kv->hh.next)
		keys[kv->id] = kv->name;
	ctx->layer->n_keys = n_keys;
	ctx->layer->keys = keys;

	HASH_CLEAR(hh, ctx->keys_hash);
}

static VectorTile__Tile__Value *create_value()
{
	VectorTile__Tile__Value *value = palloc(sizeof(*value));
	vector_tile__tile__value__init(value);
	return value;
}

#define MVT_CREATE_VALUES(kvtype, hash, hasfield, valuefield) \
{ \
	POSTGIS_DEBUG(2, "MVT_CREATE_VALUES called"); \
	{ \
		struct kvtype *kv; \
		for (kv = ctx->hash; kv != NULL; kv=kv->hh.next) \
		{ \
			VectorTile__Tile__Value *value = create_value(); \
			value->hasfield = 1; \
			value->valuefield = kv->valuefield; \
			values[kv->id] = value; \
		} \
	} \
}

static void encode_values(mvt_agg_context *ctx)
{
	VectorTile__Tile__Value **values;
	struct mvt_kv_string_value *kv;

	POSTGIS_DEBUG(2, "encode_values called");

	values = palloc(ctx->values_hash_i * sizeof(*values));
	for (kv = ctx->string_values_hash; kv != NULL; kv=kv->hh.next)
	{
		VectorTile__Tile__Value *value = create_value();
		value->string_value = kv->string_value;
		values[kv->id] = value;
	}
	MVT_CREATE_VALUES(mvt_kv_float_value,
		float_values_hash, has_float_value, float_value);
	MVT_CREATE_VALUES(mvt_kv_double_value,
		double_values_hash, has_double_value, double_value);
	MVT_CREATE_VALUES(mvt_kv_uint_value,
		uint_values_hash, has_uint_value, uint_value);
	MVT_CREATE_VALUES(mvt_kv_sint_value,
		sint_values_hash, has_sint_value, sint_value);
	MVT_CREATE_VALUES(mvt_kv_bool_value,
		bool_values_hash, has_bool_value, bool_value);

	POSTGIS_DEBUGF(3, "encode_values n_values: %d", ctx->values_hash_i);
	ctx->layer->n_values = ctx->values_hash_i;
	ctx->layer->values = values;

	HASH_CLEAR(hh, ctx->string_values_hash);
	HASH_CLEAR(hh, ctx->float_values_hash);
	HASH_CLEAR(hh, ctx->double_values_hash);
	HASH_CLEAR(hh, ctx->uint_values_hash);
	HASH_CLEAR(hh, ctx->sint_values_hash);
	HASH_CLEAR(hh, ctx->bool_values_hash);

	pfree(ctx->column_cache.column_keys_index);
	pfree(ctx->column_cache.column_oid);
	pfree(ctx->column_cache.values);
	pfree(ctx->column_cache.nulls);
	ReleaseTupleDesc(ctx->column_cache.tupdesc);
	memset(&ctx->column_cache, 0, sizeof(ctx->column_cache));

}

#define MVT_PARSE_VALUE(value, kvtype, hash, valuefield, size) \
{ \
	POSTGIS_DEBUG(2, "MVT_PARSE_VALUE called"); \
	{ \
		struct kvtype *kv; \
		HASH_FIND(hh, ctx->hash, &value, size, kv); \
		if (!kv) \
		{ \
			POSTGIS_DEBUG(4, "MVT_PARSE_VALUE value not found"); \
			kv = palloc(sizeof(*kv)); \
			POSTGIS_DEBUGF(4, "MVT_PARSE_VALUE new hash key: %d", \
				ctx->values_hash_i); \
			kv->id = ctx->values_hash_i++; \
			kv->valuefield = value; \
			HASH_ADD(hh, ctx->hash, valuefield, size, kv); \
		} \
		tags[ctx->row_columns*2] = k; \
		tags[ctx->row_columns*2+1] = kv->id; \
	} \
}

#define MVT_PARSE_INT_VALUE(value) \
{ \
	if (value >= 0) \
	{ \
		uint64_t cvalue = value; \
		MVT_PARSE_VALUE(cvalue, mvt_kv_uint_value, \
				uint_values_hash, uint_value, \
				sizeof(uint64_t)) \
	} \
	else \
	{ \
		int64_t cvalue = value; \
		MVT_PARSE_VALUE(cvalue, mvt_kv_sint_value, \
				sint_values_hash, sint_value, \
				sizeof(int64_t)) \
	} \
}

#define MVT_PARSE_DATUM(type, kvtype, hash, valuefield, datumfunc, size) \
{ \
	type value = datumfunc(datum); \
	MVT_PARSE_VALUE(value, kvtype, hash, valuefield, size); \
}

#define MVT_PARSE_INT_DATUM(type, datumfunc) \
{ \
	type value = datumfunc(datum); \
	MVT_PARSE_INT_VALUE(value); \
}

static void add_value_as_string_with_size(mvt_agg_context *ctx,
	char *value, size_t size, uint32_t *tags, uint32_t k)
{
	struct mvt_kv_string_value *kv;
	POSTGIS_DEBUG(2, "add_value_as_string called");
	HASH_FIND(hh, ctx->string_values_hash, value, size, kv);
	if (!kv)
	{
		POSTGIS_DEBUG(4, "add_value_as_string value not found");
		kv = palloc(sizeof(*kv));
		POSTGIS_DEBUGF(4, "add_value_as_string new hash key: %d",
			ctx->values_hash_i);
		kv->id = ctx->values_hash_i++;
		kv->string_value = value;
		HASH_ADD_KEYPTR(hh, ctx->string_values_hash, kv->string_value,
			size, kv);
	}
	tags[ctx->row_columns*2] = k;
	tags[ctx->row_columns*2+1] = kv->id;
}

static void add_value_as_string(mvt_agg_context *ctx,
	char *value, uint32_t *tags, uint32_t k)
{
	return add_value_as_string_with_size(ctx, value, strlen(value), tags, k);
}

static void parse_datum_as_string(mvt_agg_context *ctx, Oid typoid,
	Datum datum, uint32_t *tags, uint32_t k)
{
	Oid foutoid;
	bool typisvarlena;
	char *value;
	POSTGIS_DEBUG(2, "parse_value_as_string called");
	getTypeOutputInfo(typoid, &foutoid, &typisvarlena);
	value = OidOutputFunctionCall(foutoid, datum);
	POSTGIS_DEBUGF(4, "parse_value_as_string value: %s", value);
	add_value_as_string(ctx, value, tags, k);
}

#if POSTGIS_PGSQL_VERSION >= 94
static uint32_t *parse_jsonb(mvt_agg_context *ctx, Jsonb *jb,
	uint32_t *tags)
{
	JsonbIterator *it;
	JsonbValue v;
	bool skipNested = false;
	JsonbIteratorToken r;
	uint32_t k;

	if (!JB_ROOT_IS_OBJECT(jb))
		return tags;

	it = JsonbIteratorInit(&jb->root);

	while ((r = JsonbIteratorNext(&it, &v, skipNested)) != WJB_DONE)
	{
		skipNested = true;

		if (r == WJB_KEY && v.type != jbvNull)
		{

			k = get_key_index_with_size(ctx, v.val.string.val, v.val.string.len);
			if (k == UINT32_MAX)
			{
				char *key;
				uint32_t newSize = ctx->keys_hash_i + 1;

				key = palloc(v.val.string.len + 1);
				memcpy(key, v.val.string.val, v.val.string.len);
				key[v.val.string.len] = '\0';

				tags = repalloc(tags, newSize * 2 * sizeof(*tags));
				k = add_key(ctx, key);
			}

			r = JsonbIteratorNext(&it, &v, skipNested);

			if (v.type == jbvString)
			{
				char *value;
				value = palloc(v.val.string.len + 1);
				memcpy(value, v.val.string.val, v.val.string.len);
				value[v.val.string.len] = '\0';
				add_value_as_string(ctx, value, tags, k);
				ctx->row_columns++;
			}
			else if (v.type == jbvBool)
			{
				MVT_PARSE_VALUE(v.val.boolean, mvt_kv_bool_value,
					bool_values_hash, bool_value, sizeof(protobuf_c_boolean));
				ctx->row_columns++;
			}
			else if (v.type == jbvNumeric)
			{
				char *str;
				double d;
				long l;
				str = DatumGetCString(DirectFunctionCall1(numeric_out,
					PointerGetDatum(v.val.numeric)));
				d = strtod(str, NULL);
				l = strtol(str, NULL, 10);
				if ((long) d != l)
				{
					MVT_PARSE_VALUE(d, mvt_kv_double_value, double_values_hash,
						double_value, sizeof(double));
				}
				else
				{
					MVT_PARSE_INT_VALUE(l);
				}
				ctx->row_columns++;
			}
		}
	}

	return tags;
}
#endif

/**
 * Sets the feature id. Ignores Nulls and negative values
 */
static void set_feature_id(mvt_agg_context *ctx, Datum datum, bool isNull)
{
	Oid typoid = ctx->column_cache.column_oid[ctx->id_index];
	int64_t value = INT64_MIN;

	if (isNull)
	{
		POSTGIS_DEBUG(3, "set_feature_id: Ignored null value");
		return;
	}

	switch (typoid)
	{
	case INT2OID:
		value = DatumGetInt16(datum);
		break;
	case INT4OID:
		value = DatumGetInt32(datum);
		break;
	case INT8OID:
		value = DatumGetInt64(datum);
		break;
	default:
		elog(ERROR, "set_feature_id: Feature id type does not match");
	}

	if (value < 0)
	{
		POSTGIS_DEBUG(3, "set_feature_id: Ignored negative value");
		return;
	}

	ctx->feature->has_id = true;
	ctx->feature->id = (uint64_t) value;
}

static void parse_values(mvt_agg_context *ctx)
{
	uint32_t n_keys = ctx->keys_hash_i;
	uint32_t *tags = palloc(n_keys * 2 * sizeof(*tags));
	uint32_t i;
	mvt_column_cache cc = ctx->column_cache;
	uint32_t natts = (uint32_t) cc.tupdesc->natts;

	HeapTupleData tuple;

	POSTGIS_DEBUG(2, "parse_values called");
	ctx->row_columns = 0;

	/* Build a temporary HeapTuple control structure */
	tuple.t_len = HeapTupleHeaderGetDatumLength(ctx->row);
	ItemPointerSetInvalid(&(tuple.t_self));
	tuple.t_tableOid = InvalidOid;
	tuple.t_data = ctx->row;

	/* We use heap_deform_tuple as it costs only O(N) vs O(N^2) of GetAttributeByNum */
	heap_deform_tuple(&tuple, cc.tupdesc, cc.values, cc.nulls);

	POSTGIS_DEBUGF(3, "parse_values natts: %d", natts);

	for (i = 0; i < natts; i++)
	{
		char *key;
		Oid typoid;
		uint32_t k;
		Datum datum = cc.values[i];

		if (i == ctx->geom_index)
			continue;

		if (i == ctx->id_index)
		{
			set_feature_id(ctx, datum, cc.nulls[i]);
			continue;
		}

		if (cc.nulls[i])
		{
			POSTGIS_DEBUG(3, "parse_values isnull detected");
			continue;
		}

#if POSTGIS_PGSQL_VERSION < 110
		key = cc.tupdesc->attrs[i]->attname.data;
#else
		key = cc.tupdesc->attrs[i].attname.data;
#endif
		k = cc.column_keys_index[i];
		typoid = cc.column_oid[i];

#if POSTGIS_PGSQL_VERSION >= 94
		if (k == UINT32_MAX && typoid != JSONBOID)
			elog(ERROR, "parse_values: unexpectedly could not find parsed key name '%s'", key);
		if (typoid == JSONBOID)
		{
			tags = parse_jsonb(ctx, DatumGetJsonbP(datum), tags);
			continue;
		}
#else
		if (k == UINT32_MAX)
			elog(ERROR, "parse_values: unexpectedly could not find parsed key name '%s'", key);
#endif

		switch (typoid)
		{
		case BOOLOID:
			MVT_PARSE_DATUM(protobuf_c_boolean, mvt_kv_bool_value,
				bool_values_hash, bool_value,
				DatumGetBool, sizeof(protobuf_c_boolean));
			break;
		case INT2OID:
			MVT_PARSE_INT_DATUM(int16_t, DatumGetInt16);
			break;
		case INT4OID:
			MVT_PARSE_INT_DATUM(int32_t, DatumGetInt32);
			break;
		case INT8OID:
			MVT_PARSE_INT_DATUM(int64_t, DatumGetInt64);
			break;
		case FLOAT4OID:
			MVT_PARSE_DATUM(float, mvt_kv_float_value,
				float_values_hash, float_value,
				DatumGetFloat4, sizeof(float));
			break;
		case FLOAT8OID:
			MVT_PARSE_DATUM(double, mvt_kv_double_value,
				double_values_hash, double_value,
				DatumGetFloat8, sizeof(double));
			break;
		default:
			parse_datum_as_string(ctx, typoid, datum, tags, k);
			break;
		}

		ctx->row_columns++;
	}


	ctx->feature->n_tags = ctx->row_columns * 2;
	ctx->feature->tags = tags;

	POSTGIS_DEBUGF(3, "parse_values n_tags %zd", ctx->feature->n_tags);
}

/**
 * In place process a collection to find a concrete geometry
 * object and expose that as the actual object. Will some
 * geom be lost? Sure, but your MVT renderer couldn't
 * draw it anyways.
 */
static void
lwgeom_to_basic_type(LWGEOM *geom)
{
	if (lwgeom_get_type(geom) == COLLECTIONTYPE)
	{
		/* MVT doesn't handle generic collections, so we */
		/* need to strip them down to a typed collection */
		/* by finding the largest basic type available and */
		/* using that as the basis of a typed collection. */
		LWCOLLECTION *g = (LWCOLLECTION*)geom;
		LWCOLLECTION *gc;
		uint32_t i, maxtype = 0;
		for (i = 0; i < g->ngeoms; i++)
		{
			LWGEOM *sg = g->geoms[i];
			if (sg->type > maxtype && sg->type < COLLECTIONTYPE)
				maxtype = sg->type;
		}
		if (maxtype > 3) maxtype -= 3;
		/* Force the working geometry to be a simpler version */
		/* of itself */
		gc = lwcollection_extract(g, maxtype);
		*g = *gc;
	}
}

/**
 * Transform a geometry into vector tile coordinate space.
 *
 * Makes best effort to keep validity. Might collapse geometry into lower
 * dimension.
 *
 * NOTE: modifies in place if possible (not currently possible for polygons)
 */
LWGEOM *mvt_geom(LWGEOM *lwgeom, const GBOX *gbox, uint32_t extent, uint32_t buffer,
	bool clip_geom)
{
	AFFINE affine;
	gridspec grid;
	double width = gbox->xmax - gbox->xmin;
	double height = gbox->ymax - gbox->ymin;
	double resx, resy, res, fx, fy;
	int preserve_collapsed = LW_TRUE;
	POSTGIS_DEBUG(2, "mvt_geom called");

	/* Short circuit out on EMPTY */
	if (lwgeom_is_empty(lwgeom))
		return NULL;

	if (width == 0 || height == 0)
		elog(ERROR, "mvt_geom: bounds width or height cannot be 0");

	if (extent == 0)
		elog(ERROR, "mvt_geom: extent cannot be 0");

	resx = width / extent;
	resy = height / extent;
	res = (resx < resy ? resx : resy)/2;
	fx = extent / width;
	fy = -(extent / height);

	if (FLAGS_GET_BBOX(lwgeom->flags) && lwgeom->bbox &&
		(lwgeom->type == LINETYPE || lwgeom->type == MULTILINETYPE ||
		lwgeom->type == POLYGONTYPE || lwgeom->type == MULTIPOLYGONTYPE))
	{
		// Shortcut to drop geometries smaller than the resolution
		double bbox_width = lwgeom->bbox->xmax - lwgeom->bbox->xmin;
		double bbox_height = lwgeom->bbox->ymax - lwgeom->bbox->ymin;
		if (bbox_height * bbox_height + bbox_width * bbox_width < res * res)
			return NULL;
	}

	/* Remove all non-essential points (under the output resolution) */
	lwgeom_remove_repeated_points_in_place(lwgeom, res);
	lwgeom_simplify_in_place(lwgeom, res, preserve_collapsed);

	/* If geometry has disappeared, you're done */
	if (lwgeom_is_empty(lwgeom))
		return NULL;

	if (clip_geom)
	{
		// We need to add an extra half pixel to include the points that
		// fall into the bbox only after the coordinate transformation
		double buffer_map_xunits = nextafterf(res, 0.0) + resx * buffer;
		GBOX bgbox;
		const GBOX *lwgeom_gbox = lwgeom_get_bbox(lwgeom);
		bgbox = *gbox;
		gbox_expand(&bgbox, buffer_map_xunits);
		if (!gbox_overlaps_2d(lwgeom_gbox, &bgbox))
		{
			POSTGIS_DEBUG(3, "mvt_geom: geometry outside clip box");
			return NULL;
		}
		if (!gbox_contains_2d(&bgbox, lwgeom_gbox))
		{
			double x0 = bgbox.xmin;
			double y0 = bgbox.ymin;
			double x1 = bgbox.xmax;
			double y1 = bgbox.ymax;
			lwgeom = lwgeom_clip_by_rect(lwgeom, x0, y0, x1, y1);
			POSTGIS_DEBUG(3, "mvt_geom: no geometry after clip");
			if (lwgeom == NULL || lwgeom_is_empty(lwgeom))
				return NULL;
		}
	}

	/* transform to tile coordinate space */
	memset(&affine, 0, sizeof(affine));
	affine.afac = fx;
	affine.efac = fy;
	affine.ifac = 1;
	affine.xoff = -gbox->xmin * fx;
	affine.yoff = -gbox->ymax * fy;
	lwgeom_affine(lwgeom, &affine);

	/* snap to integer precision, removing duplicate points */
	memset(&grid, 0, sizeof(gridspec));
	grid.ipx = 0;
	grid.ipy = 0;
	grid.xsize = 1;
	grid.ysize = 1;
	lwgeom_grid_in_place(lwgeom, &grid);

	if (lwgeom == NULL || lwgeom_is_empty(lwgeom))
		return NULL;

	/* if polygon(s) make valid and force clockwise as per MVT spec */
	if (lwgeom->type == POLYGONTYPE ||
		lwgeom->type == MULTIPOLYGONTYPE ||
		lwgeom->type == COLLECTIONTYPE)
	{
		lwgeom = lwgeom_make_valid(lwgeom);
		/* In image coordinates CW actually comes out a CCW, so */
		/* we also reverse. ¯\_(ツ)_/¯ */
		lwgeom_force_clockwise(lwgeom);
		lwgeom_reverse_in_place(lwgeom);
	}

	/* if geometry collection extract highest dimensional geometry type */
	if (lwgeom->type == COLLECTIONTYPE)
		lwgeom_to_basic_type(lwgeom);

	if (lwgeom == NULL || lwgeom_is_empty(lwgeom))
		return NULL;

	return lwgeom;
}

/**
 * Initialize aggregation context.
 */
void mvt_agg_init_context(mvt_agg_context *ctx)
{
	VectorTile__Tile__Layer *layer;

	POSTGIS_DEBUG(2, "mvt_agg_init_context called");

	if (ctx->extent == 0)
		elog(ERROR, "mvt_agg_init_context: extent cannot be 0");

	ctx->tile = NULL;
	ctx->features_capacity = FEATURES_CAPACITY_INITIAL;
	ctx->keys_hash = NULL;
	ctx->string_values_hash = NULL;
	ctx->float_values_hash = NULL;
	ctx->double_values_hash = NULL;
	ctx->uint_values_hash = NULL;
	ctx->sint_values_hash = NULL;
	ctx->bool_values_hash = NULL;
	ctx->values_hash_i = 0;
	ctx->keys_hash_i = 0;
	ctx->id_index = UINT32_MAX;
	ctx->geom_index = UINT32_MAX;

	memset(&ctx->column_cache, 0, sizeof(ctx->column_cache));

	layer = palloc(sizeof(*layer));
	vector_tile__tile__layer__init(layer);
	layer->version = 2;
	layer->name = ctx->name;
	layer->has_extent = 1;
	layer->extent = ctx->extent;
	layer->features = palloc (ctx->features_capacity *
		sizeof(*layer->features));

	ctx->layer = layer;
}

/**
 * Aggregation step.
 *
 * Expands features array if needed by a factor of 2.
 * Allocates a new feature, increment feature counter and
 * encode geometry and properties into it.
 */
void mvt_agg_transfn(mvt_agg_context *ctx)
{
	bool isnull = false;
	Datum datum;
	GSERIALIZED *gs;
	LWGEOM *lwgeom;
	VectorTile__Tile__Feature *feature;
	VectorTile__Tile__Layer *layer = ctx->layer;
/*	VectorTile__Tile__Feature **features = layer->features; */
	POSTGIS_DEBUG(2, "mvt_agg_transfn called");

	if (layer->n_features >= ctx->features_capacity)
	{
		size_t new_capacity = ctx->features_capacity * 2;
		layer->features = repalloc(layer->features, new_capacity *
			sizeof(*layer->features));
		ctx->features_capacity = new_capacity;
		POSTGIS_DEBUGF(3, "mvt_agg_transfn new_capacity: %zd", new_capacity);
	}

	if (ctx->geom_index == UINT32_MAX)
		parse_column_keys(ctx);

	datum = GetAttributeByNum(ctx->row, ctx->geom_index + 1, &isnull);
	POSTGIS_DEBUGF(3, "mvt_agg_transfn ctx->geom_index: %d", ctx->geom_index);
	POSTGIS_DEBUGF(3, "mvt_agg_transfn isnull: %u", isnull);
	POSTGIS_DEBUGF(3, "mvt_agg_transfn datum: %lu", datum);
	if (isnull) /* Skip rows that have null geometry */
	{
		POSTGIS_DEBUG(3, "mvt_agg_transfn got null geom");
		return;
	}

	feature = palloc(sizeof(*feature));
	vector_tile__tile__feature__init(feature);

	ctx->feature = feature;

	gs = (GSERIALIZED *) PG_DETOAST_DATUM(datum);
	lwgeom = lwgeom_from_gserialized(gs);

	POSTGIS_DEBUGF(3, "mvt_agg_transfn encoded feature count: %zd", layer->n_features);
	layer->features[layer->n_features++] = feature;

	encode_geometry(ctx, lwgeom);
	lwgeom_free(lwgeom);
	// TODO: free detoasted datum?
	parse_values(ctx);
}

static VectorTile__Tile * mvt_ctx_to_tile(mvt_agg_context *ctx)
{
	int n_layers = 1;
	VectorTile__Tile *tile;
	encode_keys(ctx);
	encode_values(ctx);

	tile = palloc(sizeof(VectorTile__Tile));
	vector_tile__tile__init(tile);
	tile->layers = palloc(sizeof(VectorTile__Tile__Layer*) * n_layers);
	tile->layers[0] = ctx->layer;
	tile->n_layers = n_layers;
	return tile;
}

static bytea *mvt_ctx_to_bytea(mvt_agg_context *ctx)
{
	/* Fill out the file slot, if it's not already filled. */
	/* We should only have a filled slow when all the work of building */
	/* out the data is complete, so after a serialize/deserialize cycle */
	/* or after a context combine */
	size_t len;
	bytea *ba;

	if (!ctx->tile)
	{
		ctx->tile = mvt_ctx_to_tile(ctx);
	}

	/* Zero features => empty bytea output */
	if (ctx && ctx->layer && ctx->layer->n_features == 0)
	{
		bytea *ba = palloc(VARHDRSZ);
		SET_VARSIZE(ba, VARHDRSZ);
		return ba;
	}

	/* Serialize the Tile */
	len = VARHDRSZ + vector_tile__tile__get_packed_size(ctx->tile);
	ba = palloc(len);
	vector_tile__tile__pack(ctx->tile, (uint8_t*)VARDATA(ba));
	SET_VARSIZE(ba, len);
	return ba;
}


bytea * mvt_ctx_serialize(mvt_agg_context *ctx)
{
	return mvt_ctx_to_bytea(ctx);
}

static void * mvt_allocator(__attribute__((__unused__)) void *data, size_t size)
{
	return palloc(size);
}

static void mvt_deallocator(__attribute__((__unused__)) void *data, void *ptr)
{
	return pfree(ptr);
}

mvt_agg_context * mvt_ctx_deserialize(const bytea *ba)
{
	ProtobufCAllocator allocator =
	{
		mvt_allocator,
		mvt_deallocator,
		NULL
	};

	size_t len = VARSIZE(ba) - VARHDRSZ;
	VectorTile__Tile *tile = vector_tile__tile__unpack(&allocator, len, (uint8_t*)VARDATA(ba));
	mvt_agg_context *ctx = palloc(sizeof(mvt_agg_context));
	memset(ctx, 0, sizeof(mvt_agg_context));
	ctx->tile = tile;
	return ctx;
}

static VectorTile__Tile__Value *
tile_value_copy(const VectorTile__Tile__Value *value)
{
	VectorTile__Tile__Value *nvalue = palloc(sizeof(VectorTile__Tile__Value));
	memcpy(nvalue, value, sizeof(VectorTile__Tile__Value));
	if (value->string_value)
		nvalue->string_value = pstrdup(value->string_value);
	return nvalue;
}

static VectorTile__Tile__Feature *
tile_feature_copy(const VectorTile__Tile__Feature *feature, int key_offset, int value_offset)
{
	uint32_t i;
	VectorTile__Tile__Feature *nfeature;

	/* Null in => Null out */
	if (!feature) return NULL;

	/* Init object */
	nfeature = palloc(sizeof(VectorTile__Tile__Feature));
	vector_tile__tile__feature__init(nfeature);

	/* Copy settings straight over */
	nfeature->has_id = feature->has_id;
	nfeature->id = feature->id;
	nfeature->has_type = feature->has_type;
	nfeature->type = feature->type;

	/* Copy tags over, offsetting indexes so they match the dictionaries */
	/* at the Tile_Layer level */
	if (feature->n_tags > 0)
	{
		nfeature->n_tags = feature->n_tags;
		nfeature->tags = palloc(sizeof(uint32_t)*feature->n_tags);
		for (i = 0; i < feature->n_tags/2; i++)
		{
			nfeature->tags[2*i] = feature->tags[2*i] + key_offset;
			nfeature->tags[2*i+1] = feature->tags[2*i+1] + value_offset;
		}
	}

	/* Copy the raw geometry data over literally */
	if (feature->n_geometry > 0)
	{
		nfeature->n_geometry = feature->n_geometry;
		nfeature->geometry = palloc(sizeof(uint32_t)*feature->n_geometry);
		memcpy(nfeature->geometry, feature->geometry, sizeof(uint32_t)*feature->n_geometry);
	}

	/* Done */
	return nfeature;
}

static VectorTile__Tile__Layer *
vectortile_layer_combine(const VectorTile__Tile__Layer *layer1, const VectorTile__Tile__Layer *layer2)
{
	uint32_t i, j;
	int key2_offset, value2_offset;
	VectorTile__Tile__Layer *layer = palloc(sizeof(VectorTile__Tile__Layer));
	vector_tile__tile__layer__init(layer);

	/* Take globals from layer1 */
	layer->version = layer1->version;
	layer->name = pstrdup(layer1->name);
	layer->has_extent = layer1->has_extent;
	layer->extent = layer1->extent;

	/* Copy keys into new layer */
	j = 0;
	layer->n_keys = layer1->n_keys + layer2->n_keys;
	layer->keys = layer->n_keys ? palloc(layer->n_keys * sizeof(void*)) : NULL;
	for (i = 0; i < layer1->n_keys; i++)
		layer->keys[j++] = pstrdup(layer1->keys[i]);
	key2_offset = j;
	for (i = 0; i < layer2->n_keys; i++)
		layer->keys[j++] = pstrdup(layer2->keys[i]);

	/* Copy values into new layer */
	/* TODO, apply hash logic here too, so that merged tiles */
	/* retain unique value maps */
	layer->n_values = layer1->n_values + layer2->n_values;
	layer->values = layer->n_values ? palloc(layer->n_values * sizeof(void*)) : NULL;
	j = 0;
	for (i = 0; i < layer1->n_values; i++)
		layer->values[j++] = tile_value_copy(layer1->values[i]);
	value2_offset = j;
	for (i = 0; i < layer2->n_values; i++)
		layer->values[j++] = tile_value_copy(layer2->values[i]);


	layer->n_features = layer1->n_features + layer2->n_features;
	layer->features = layer->n_features ? palloc(layer->n_features * sizeof(void*)) : NULL;
	j = 0;
	for (i = 0; i < layer1->n_features; i++)
		layer->features[j++] = tile_feature_copy(layer1->features[i], 0, 0);
	for (i = 0; i < layer2->n_features; i++)
		layer->features[j++] = tile_feature_copy(layer2->features[i], key2_offset, value2_offset);

	return layer;
}


static VectorTile__Tile *
vectortile_tile_combine(VectorTile__Tile *tile1, VectorTile__Tile *tile2)
{
	uint32_t i, j;
	VectorTile__Tile *tile;

	/* Hopelessly messing up memory ownership here */
	if (tile1->n_layers == 0 && tile2->n_layers == 0)
		return tile1;
	else if (tile1->n_layers == 0)
		return tile2;
	else if (tile2->n_layers == 0)
		return tile1;

	tile = palloc(sizeof(VectorTile__Tile));
	vector_tile__tile__init(tile);
	tile->layers = palloc(sizeof(void*));
	tile->n_layers = 0;

	/* Merge all matching layers in the files (basically always only one) */
	for (i = 0; i < tile1->n_layers; i++)
	{
		for (j = 0; j < tile2->n_layers; j++)
		{
			VectorTile__Tile__Layer *l1 = tile1->layers[i];
			VectorTile__Tile__Layer *l2 = tile2->layers[j];
			if (strcmp(l1->name, l2->name)==0)
			{
				VectorTile__Tile__Layer *layer = vectortile_layer_combine(l1, l2);
				if (!layer)
					continue;
				tile->layers[tile->n_layers++] = layer;
				/* Add a spare slot at the end of the array */
				tile->layers = repalloc(tile->layers, (tile->n_layers+1) * sizeof(void*));
			}
		}
	}
	return tile;
}

mvt_agg_context * mvt_ctx_combine(mvt_agg_context *ctx1, mvt_agg_context *ctx2)
{
	if (ctx1 || ctx2)
	{
		if (ctx1 && ! ctx2) return ctx1;
		if (ctx2 && ! ctx1) return ctx2;
		if (ctx1 && ctx2 && ctx1->tile && ctx2->tile)
		{
			mvt_agg_context *ctxnew = palloc(sizeof(mvt_agg_context));
			memset(ctxnew, 0, sizeof(mvt_agg_context));
			ctxnew->tile = vectortile_tile_combine(ctx1->tile, ctx2->tile);
			return ctxnew;
		}
		else
		{
			elog(DEBUG2, "ctx1->tile = %p", ctx1->tile);
			elog(DEBUG2, "ctx2->tile = %p", ctx2->tile);
			elog(ERROR, "%s: unable to combine contexts where tile attribute is null", __func__);
			return NULL;
		}
	}
	else
	{
		return NULL;
	}
}

/**
 * Finalize aggregation.
 *
 * Encode keys and values and put the aggregated Layer message into
 * a Tile message and returns it packed as a bytea.
 */
bytea *mvt_agg_finalfn(mvt_agg_context *ctx)
{
	return mvt_ctx_to_bytea(ctx);
}


#endif
