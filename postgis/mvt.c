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

enum mvt_cmd_id {
	CMD_MOVE_TO = 1,
	CMD_LINE_TO = 2,
	CMD_CLOSE_PATH = 7
};

enum mvt_type {
	MVT_POINT = 1,
	MVT_LINE = 2,
	MVT_RING = 3
};

struct mvt_kv_key {
	char *name;
	uint32_t id;
	UT_hash_handle hh;
};

struct mvt_kv_string_value {
	char *string_value;
	uint32_t id;
	UT_hash_handle hh;
};

struct mvt_kv_float_value {
	float float_value;
	uint32_t id;
	UT_hash_handle hh;
};

struct mvt_kv_double_value {
	double double_value;
	uint32_t id;
	UT_hash_handle hh;
};

struct mvt_kv_uint_value {
	uint64_t uint_value;
	uint32_t id;
	UT_hash_handle hh;
};

struct mvt_kv_sint_value {
	int64_t sint_value;
	uint32_t id;
	UT_hash_handle hh;
};

struct mvt_kv_bool_value {
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

static uint32_t encode_ptarray(struct mvt_agg_context *ctx, enum mvt_type type,
			       POINTARRAY *pa, uint32_t *buffer,
			       int32_t *px, int32_t *py)
{
	uint32_t offset = 0;
	uint32_t i, c = 0;
	int32_t dx, dy, x, y;

	/* loop points and add to buffer */
	for (i = 0; i < pa->npoints; i++) {
		/* move offset for command */
		if (i == 0 || (i == 1 && type > MVT_POINT))
			offset++;
		/* skip closing point for rings */
		if (type == MVT_RING && i == pa->npoints - 1)
			break;
		const POINT2D *p = getPoint2d_cp(pa, i);
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
	if (type == MVT_POINT) {
		/* point or multipoint, use actual number of point count */
		buffer[0] = c_int(CMD_MOVE_TO, c);
	} else {
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

static uint32_t encode_ptarray_initial(struct mvt_agg_context *ctx,
				       enum mvt_type type,
				       POINTARRAY *pa, uint32_t *buffer)
{
	int32_t px = 0, py = 0;
	return encode_ptarray(ctx, type, pa, buffer, &px, &py);
}

static void encode_point(struct mvt_agg_context *ctx, LWPOINT *point)
{
	VectorTile__Tile__Feature *feature = ctx->feature;
	feature->type = VECTOR_TILE__TILE__GEOM_TYPE__POINT;
	feature->has_type = 1;
	feature->n_geometry = 3;
	feature->geometry = palloc(sizeof(*feature->geometry) * 3);
	encode_ptarray_initial(ctx, MVT_POINT, point->point, feature->geometry);
}

static void encode_mpoint(struct mvt_agg_context *ctx, LWMPOINT *mpoint)
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

static void encode_line(struct mvt_agg_context *ctx, LWLINE *lwline)
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

static void encode_mline(struct mvt_agg_context *ctx, LWMLINE *lwmline)
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

static void encode_poly(struct mvt_agg_context *ctx, LWPOLY *lwpoly)
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

static void encode_mpoly(struct mvt_agg_context *ctx, LWMPOLY *lwmpoly)
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

static void encode_geometry(struct mvt_agg_context *ctx, LWGEOM *lwgeom)
{
	int type = lwgeom->type;

	switch (type) {
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

static TupleDesc get_tuple_desc(struct mvt_agg_context *ctx)
{
	Oid tupType = HeapTupleHeaderGetTypeId(ctx->row);
	int32 tupTypmod = HeapTupleHeaderGetTypMod(ctx->row);
	TupleDesc tupdesc = lookup_rowtype_tupdesc(tupType, tupTypmod);
	return tupdesc;
}

static uint32_t get_key_index(struct mvt_agg_context *ctx, char *name)
{
	struct mvt_kv_key *kv;
	size_t size = strlen(name);
	HASH_FIND(hh, ctx->keys_hash, name, size, kv);
	if (!kv)
		return -1;
	return kv->id;
}

static uint32_t add_key(struct mvt_agg_context *ctx, char *name)
{
	struct mvt_kv_key *kv;
	size_t size = strlen(name);
	kv = palloc(sizeof(*kv));
	kv->id = ctx->keys_hash_i++;
	kv->name = name;
	HASH_ADD_KEYPTR(hh, ctx->keys_hash, name, size, kv);
	return kv->id;
}

static void parse_column_keys(struct mvt_agg_context *ctx)
{
	TupleDesc tupdesc = get_tuple_desc(ctx);
	int natts = tupdesc->natts;
	uint32_t i;
	bool geom_found = false;
	char *key;
	POSTGIS_DEBUG(2, "parse_column_keys called");

	for (i = 0; i < natts; i++) {
#if POSTGIS_PGSQL_VERSION < 110
		Oid typoid = getBaseType(tupdesc->attrs[i]->atttypid);
		char *tkey = tupdesc->attrs[i]->attname.data;
#else
		Oid typoid = getBaseType(tupdesc->attrs[i].atttypid);
		char *tkey = tupdesc->attrs[i].attname.data;
#endif
#if POSTGIS_PGSQL_VERSION >= 94
		if (typoid == JSONBOID)
			continue;
#endif
		key = pstrdup(tkey);
		if (ctx->geom_name == NULL) {
			if (!geom_found && typoid == TypenameGetTypid("geometry")) {
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
		add_key(ctx, key);
	}
	if (!geom_found)
		elog(ERROR, "parse_column_keys: no geometry column found");
	ReleaseTupleDesc(tupdesc);
}

static void encode_keys(struct mvt_agg_context *ctx)
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
	struct kvtype *kv; \
	for (kv = ctx->hash; kv != NULL; kv=kv->hh.next) { \
		VectorTile__Tile__Value *value = create_value(); \
		value->hasfield = 1; \
		value->valuefield = kv->valuefield; \
		values[kv->id] = value; \
	} \
}

static void encode_values(struct mvt_agg_context *ctx)
{
	POSTGIS_DEBUG(2, "encode_values called");
	VectorTile__Tile__Value **values;
	values = palloc(ctx->values_hash_i * sizeof(*values));
	struct mvt_kv_string_value *kv;
	for (kv = ctx->string_values_hash; kv != NULL; kv=kv->hh.next) {
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
}

#define MVT_PARSE_VALUE(value, kvtype, hash, valuefield, size) \
{ \
	POSTGIS_DEBUG(2, "MVT_PARSE_VALUE called"); \
	struct kvtype *kv; \
	HASH_FIND(hh, ctx->hash, &value, size, kv); \
	if (!kv) { \
		POSTGIS_DEBUG(4, "MVT_PARSE_VALUE value not found"); \
		kv = palloc(sizeof(*kv)); \
		POSTGIS_DEBUGF(4, "MVT_PARSE_VALUE new hash key: %d", \
			ctx->values_hash_i); \
		kv->id = ctx->values_hash_i++; \
		kv->valuefield = value; \
		HASH_ADD(hh, ctx->hash, valuefield, size, kv); \
	} \
	tags[ctx->c*2] = k; \
	tags[ctx->c*2+1] = kv->id; \
}

#define MVT_PARSE_INT_VALUE(value) \
{ \
	if (value >= 0) { \
		uint64_t cvalue = value; \
		MVT_PARSE_VALUE(cvalue, mvt_kv_uint_value, \
				uint_values_hash, uint_value, \
				sizeof(uint64_t)) \
	} else { \
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

static void add_value_as_string(struct mvt_agg_context *ctx,
	char *value, uint32_t *tags, uint32_t k)
{
	struct mvt_kv_string_value *kv;
	size_t size = strlen(value);
	POSTGIS_DEBUG(2, "add_value_as_string called");
	HASH_FIND(hh, ctx->string_values_hash, value, size, kv);
	if (!kv) {
		POSTGIS_DEBUG(4, "add_value_as_string value not found");
		kv = palloc(sizeof(*kv));
		POSTGIS_DEBUGF(4, "add_value_as_string new hash key: %d",
			ctx->values_hash_i);
		kv->id = ctx->values_hash_i++;
		kv->string_value = value;
		HASH_ADD_KEYPTR(hh, ctx->string_values_hash, kv->string_value,
			size, kv);
	}
	tags[ctx->c*2] = k;
	tags[ctx->c*2+1] = kv->id;
}

static void parse_datum_as_string(struct mvt_agg_context *ctx, Oid typoid,
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
static uint32_t *parse_jsonb(struct mvt_agg_context *ctx, Jsonb *jb,
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

	while ((r = JsonbIteratorNext(&it, &v, skipNested)) != WJB_DONE) {
		skipNested = true;

		if (r == WJB_KEY && v.type != jbvNull) {
			char *key;
			key = palloc(v.val.string.len + 1 * sizeof(char));
			memcpy(key, v.val.string.val, v.val.string.len);
			key[v.val.string.len] = '\0';

			k = get_key_index(ctx, key);
			if (k == -1) {
				uint32_t newSize = ctx->keys_hash_i + 1;
				tags = repalloc(tags, newSize * 2 * sizeof(*tags));
				k = add_key(ctx, key);
			}

			r = JsonbIteratorNext(&it, &v, skipNested);

			if (v.type == jbvString) {
				char *value;
				value = palloc(v.val.string.len + 1 * sizeof(char));
				memcpy(value, v.val.string.val, v.val.string.len);
				value[v.val.string.len] = '\0';
				add_value_as_string(ctx, value, tags, k);
				ctx->c++;
			} else if (v.type == jbvBool) {
				MVT_PARSE_VALUE(v.val.boolean, mvt_kv_bool_value,
					bool_values_hash, bool_value, sizeof(protobuf_c_boolean));
				ctx->c++;
			} else if (v.type == jbvNumeric) {
				char *str;
				str = DatumGetCString(DirectFunctionCall1(numeric_out,
					PointerGetDatum(v.val.numeric)));
				double d = strtod(str, NULL);
				long l = strtol(str, NULL, 10);
				if ((long) d != l) {
					MVT_PARSE_VALUE(d, mvt_kv_double_value, double_values_hash,
						double_value, sizeof(double));
				} else {
					MVT_PARSE_INT_VALUE(l);
				}
				ctx->c++;
			}
		}
	}

	return tags;
}
#endif

static void parse_values(struct mvt_agg_context *ctx)
{
	POSTGIS_DEBUG(2, "parse_values called");
	uint32_t n_keys = ctx->keys_hash_i;
	uint32_t *tags = palloc(n_keys * 2 * sizeof(*tags));
	bool isnull;
	uint32_t i, k;
	TupleDesc tupdesc = get_tuple_desc(ctx);
	int natts = tupdesc->natts;
	ctx->c = 0;

	POSTGIS_DEBUGF(3, "parse_values natts: %d", natts);

	for (i = 0; i < natts; i++) {
		char *key;
		Oid typoid;
		Datum datum;

		if (i == ctx->geom_index)
			continue;

#if POSTGIS_PGSQL_VERSION < 110
		key = tupdesc->attrs[i]->attname.data;
		typoid = getBaseType(tupdesc->attrs[i]->atttypid);
#else
		key = tupdesc->attrs[i].attname.data;
		typoid = getBaseType(tupdesc->attrs[i].atttypid);
#endif
		datum = GetAttributeByNum(ctx->row, i+1, &isnull);
		k = get_key_index(ctx, key);
		if (isnull) {
			POSTGIS_DEBUG(3, "parse_values isnull detected");
			continue;
		}
#if POSTGIS_PGSQL_VERSION >= 94
		if (k == -1 && typoid != JSONBOID)
			elog(ERROR, "parse_values: unexpectedly could not find parsed key name '%s'", key);
		if (typoid == JSONBOID) {
			tags = parse_jsonb(ctx, DatumGetJsonbP(datum), tags);
			continue;
		}
#else
		if (k == -1)
			elog(ERROR, "parse_values: unexpectedly could not find parsed key name '%s'", key);
#endif

		switch (typoid) {
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
		ctx->c++;
	}

	ReleaseTupleDesc(tupdesc);

	ctx->feature->n_tags = ctx->c * 2;
	ctx->feature->tags = tags;

	POSTGIS_DEBUGF(3, "parse_values n_tags %zd", ctx->feature->n_tags);
}
static int max_type(LWCOLLECTION *lwcoll)
{
	int i, max = POINTTYPE;
	for (i = 0; i < lwcoll->ngeoms; i++) {
		uint8_t type = lwcoll->geoms[i]->type;
		if (type == POLYGONTYPE || type == MULTIPOLYGONTYPE)
			return POLYGONTYPE;
		else if (type == LINETYPE || type == MULTILINETYPE)
			max = LINETYPE;
	}
	return max;
}

/**
 * Transform a geometry into vector tile coordinate space.
 *
 * Makes best effort to keep validity. Might collapse geometry into lower
 * dimension.
 */
LWGEOM *mvt_geom(const LWGEOM *lwgeom, const GBOX *gbox, uint32_t extent, uint32_t buffer,
	bool clip_geom)
{
	AFFINE affine;
	gridspec grid;
	LWGEOM *lwgeom_out = NULL;
	double width = gbox->xmax - gbox->xmin;
	double height = gbox->ymax - gbox->ymin;
	double resx = width / extent;
	double fx = extent / width;
	double fy = -(extent / height);
	double buffer_map_xunits = resx * buffer;
	const GBOX *ggbox;
	POSTGIS_DEBUG(2, "mvt_geom called");

	/* Short circuit out on EMPTY */
	if (lwgeom_is_empty(lwgeom))
		return NULL;

	ggbox = lwgeom_get_bbox(lwgeom);
	if (width == 0 || height == 0)
		elog(ERROR, "mvt_geom: bounds width or height cannot be 0");

	if (extent == 0)
		elog(ERROR, "mvt_geom: extent cannot be 0");

	if (clip_geom) {
		GBOX *bgbox = gbox_copy(gbox);
		gbox_expand(bgbox, buffer_map_xunits);
		if (!gbox_overlaps_2d(ggbox, bgbox)) {
			POSTGIS_DEBUG(3, "mvt_geom: geometry outside clip box");
			return NULL;
		}
		if (!gbox_contains_2d(bgbox, ggbox)) {
			double x0 = bgbox->xmin;
			double y0 = bgbox->ymin;
			double x1 = bgbox->xmax;
			double y1 = bgbox->ymax;
#if POSTGIS_GEOS_VERSION < 35
			LWPOLY *lwenv = lwpoly_construct_envelope(0, x0, y0, x1, y1);
			lwgeom_out = lwgeom_intersection(lwgeom, lwpoly_as_lwgeom(lwenv));
			lwpoly_free(lwenv);
#else
			lwgeom_out = lwgeom_clip_by_rect(lwgeom, x0, y0, x1, y1);
#endif
			POSTGIS_DEBUG(3, "mvt_geom: no geometry after clip");
			if (lwgeom_out == NULL || lwgeom_is_empty(lwgeom_out))
				return NULL;
		}
	}

	/* if no clip output deep clone original to avoid mutation */
	if (lwgeom_out == NULL)
		lwgeom_out = lwgeom_clone_deep(lwgeom);

	/* transform to tile coordinate space */
	memset(&affine, 0, sizeof(affine));
	affine.afac = fx;
	affine.efac = fy;
	affine.ifac = 1;
	affine.xoff = -gbox->xmin * fx;
	affine.yoff = -gbox->ymax * fy;
	lwgeom_affine(lwgeom_out, &affine);

	/* snap to integer precision, removing duplicate points */
	memset(&grid, 0, sizeof(gridspec));
	grid.ipx = 0;
	grid.ipy = 0;
	grid.xsize = 1;
	grid.ysize = 1;
	lwgeom_out = lwgeom_grid(lwgeom_out, &grid);

	if (lwgeom_out == NULL || lwgeom_is_empty(lwgeom_out))
		return NULL;

	/* if polygon(s) make valid and force clockwise as per MVT spec */
	if (lwgeom_out->type == POLYGONTYPE ||
		lwgeom_out->type == MULTIPOLYGONTYPE) {
		lwgeom_out = lwgeom_make_valid(lwgeom_out);
		lwgeom_force_clockwise(lwgeom_out);
	}

	/* if geometry collection extract highest dimensional geometry type */
	if (lwgeom_out->type == COLLECTIONTYPE) {
		LWCOLLECTION *lwcoll = lwgeom_as_lwcollection(lwgeom_out);
		lwgeom_out = lwcollection_as_lwgeom(
			lwcollection_extract(lwcoll, max_type(lwcoll)));
		lwgeom_out = lwgeom_homogenize(lwgeom_out);
		/* if polygon(s) make valid and force clockwise as per MVT spec */
		if (lwgeom_out->type == POLYGONTYPE ||
			lwgeom_out->type == MULTIPOLYGONTYPE) {
			lwgeom_out = lwgeom_make_valid(lwgeom_out);
			lwgeom_force_clockwise(lwgeom_out);
		}
	}

	if (lwgeom_out == NULL || lwgeom_is_empty(lwgeom_out))
		return NULL;

	return lwgeom_out;
}

/**
 * Initialize aggregation context.
 */
void mvt_agg_init_context(struct mvt_agg_context *ctx)
{
	VectorTile__Tile__Layer *layer;

	POSTGIS_DEBUG(2, "mvt_agg_init_context called");

	if (ctx->extent == 0)
		elog(ERROR, "mvt_agg_init_context: extent cannot be 0");

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
void mvt_agg_transfn(struct mvt_agg_context *ctx)
{
	bool isnull = false;
	Datum datum;
	GSERIALIZED *gs;
	LWGEOM *lwgeom;
	VectorTile__Tile__Feature *feature;
	VectorTile__Tile__Layer *layer = ctx->layer;
	POSTGIS_DEBUG(2, "mvt_agg_transfn called");

	if (layer->n_features >= ctx->features_capacity) {
		size_t new_capacity = ctx->features_capacity * 2;
		layer->features = repalloc(layer->features, new_capacity *
			sizeof(*layer->features));
		ctx->features_capacity = new_capacity;
		POSTGIS_DEBUGF(3, "mvt_agg_transfn new_capacity: %zd", new_capacity);
	}

	if (layer->n_features == 0)
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

/**
 * Finalize aggregation.
 *
 * Encode keys and values and put the aggregated Layer message into
 * a Tile message and returns it packed as a bytea.
 */
uint8_t *mvt_agg_finalfn(struct mvt_agg_context *ctx)
{
	VectorTile__Tile__Layer *layers[1];
	VectorTile__Tile tile = VECTOR_TILE__TILE__INIT;
	size_t len;
	uint8_t *buf;

	POSTGIS_DEBUG(2, "mvt_agg_finalfn called");
	POSTGIS_DEBUGF(2, "mvt_agg_finalfn n_features == %zd", ctx->layer->n_features);

	/* Zero features => empty bytea output */
	if (ctx->layer->n_features == 0)
	{
		buf = palloc(VARHDRSZ);
		SET_VARSIZE(buf, VARHDRSZ);
		return buf;
	}

	encode_keys(ctx);
	encode_values(ctx);

	layers[0] = ctx->layer;

	tile.n_layers = 1;
	tile.layers = layers;

	len = vector_tile__tile__get_packed_size(&tile);
	buf = palloc(sizeof(*buf) * (len + VARHDRSZ));
	vector_tile__tile__pack(&tile, buf + VARHDRSZ);

	SET_VARSIZE(buf, VARHDRSZ + len);

	return buf;
}

#endif
