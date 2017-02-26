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

#include "uthash.h"

#define FEATURES_CAPACITY_INITIAL 50

enum mvd_cmd_id {
	CMD_MOVE_TO = 1,
	CMD_LINE_TO = 2,
	CMD_CLOSE_PATH = 7
};

enum mvt_type {
	MVT_POINT = 1,
	MVT_LINE = 2,
	MVT_RING = 3
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

struct mvt_kv_int_value {
	int64_t int_value;
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
	bool bool_value;
	uint32_t id;
	UT_hash_handle hh;
};

static inline uint32_t c_int(enum mvd_cmd_id id, uint32_t count)
{
	return (id & 0x7) | (count << 3);
}

static inline uint32_t p_int(int32_t value)
{
	return (value << 1) ^ (value >> 31);
}

static inline int32_t scale_translate(double value, double res,
				      double offset) {
	return (value - offset) / res;
}

static uint32_t encode_ptarray(struct mvt_agg_context *ctx, enum mvt_type type,
			       POINTARRAY *pa, uint32_t *buffer,
			       int32_t *px, int32_t *py)
{
	POINT2D p;
	uint32_t offset = 0;
	uint32_t i, c = 0;
	int32_t dx, dy, x, y;
	uint32_t cmd_offset;
	double xres = ctx->xres;
	double yres = ctx->yres;
	double xmin = ctx->bounds->xmin;
	double ymin = ctx->bounds->ymin;

	/* loop points, scale to extent and add to buffer if not repeated */
	for (i = 0; i < pa->npoints; i++) {
		/* move offset for command and remember the latest */
		if (i == 0 || (i == 1 && type > MVT_POINT))
			cmd_offset = offset++;
		getPoint2d_p(pa, i, &p);
		x = scale_translate(p.x, xres, xmin);
		y = ctx->extent - scale_translate(p.y, yres, ymin);
		dx = x - *px;
		dy = y - *py;
		/* skip point if repeated */
		if (i > type && dx == 0 && dy == 0)
			continue;
		buffer[offset++] = p_int(dx);
		buffer[offset++] = p_int(dy);
		*px = x;
		*py = y;
		c++;
	}

	/* determine initial move and eventual line command */
	if (type == MVT_POINT) {
		/* point or multipoint, use actual number of point count */
		buffer[cmd_offset] = c_int(CMD_MOVE_TO, c);
	} else {
		/* line or polygon, assume count 1 */
		buffer[cmd_offset - 3] = c_int(CMD_MOVE_TO, 1);
		/* line command with move point subtracted from count */ 
		buffer[cmd_offset] = c_int(CMD_LINE_TO, c - 1);
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
		c += 3 + lwpoly->rings[i]->npoints * 2;
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
			c += 3 + poly->rings[j]->npoints * 2;
	feature->geometry = palloc(sizeof(*feature->geometry) * c);
	for (i = 0; i < lwmpoly->ngeoms; i++)
		for (j = 0; poly = lwmpoly->geoms[i], j < poly->nrings; j++)
			offset += encode_ptarray(ctx, MVT_LINE,
				poly->rings[j],	feature->geometry + offset,
				&px, &py);
	feature->n_geometry = offset;
}

static bool check_geometry_size(LWGEOM *lwgeom, struct mvt_agg_context *ctx)
{
	GBOX bbox;
	lwgeom_calculate_gbox(lwgeom, &bbox);
	double w = bbox.xmax - bbox.xmin;
	double h = bbox.ymax - bbox.ymin;
	return w >= ctx->xres * 2 && h >= ctx->yres * 2;
}

static bool clip_geometry(struct mvt_agg_context *ctx)
{
	LWGEOM *lwgeom = ctx->lwgeom;
	GBOX *bounds = ctx->bounds;
	int type = lwgeom->type;
	double buffer_map_xunits = ctx->xres * ctx->buffer;
	double buffer_map_yunits = ctx->yres * ctx->buffer;
	double x0 = bounds->xmin - buffer_map_xunits;
	double y0 = bounds->ymin - buffer_map_yunits;
	double x1 = bounds->xmax + buffer_map_xunits;
	double y1 = bounds->ymax + buffer_map_yunits;
#if POSTGIS_GEOS_VERSION < 35
	LWPOLY *lwenv = lwpoly_construct_envelope(0, x0, y0, x1, y1);
	lwgeom = lwgeom_intersection(lwgeom, lwpoly_as_lwgeom(lwenv));
#else
	lwgeom = lwgeom_clip_by_rect(lwgeom, x0, y0, x1, y1);
#endif
	if (lwgeom_is_empty(lwgeom))
		return false;
	if (lwgeom->type == COLLECTIONTYPE) {
		if (type == MULTIPOLYGONTYPE)
			type = POLYGONTYPE;
		else if (type == MULTILINETYPE)
			type = LINETYPE;
		else if (type == MULTIPOINTTYPE)
			type = POINTTYPE;
		lwgeom = lwcollection_as_lwgeom(lwcollection_extract((LWCOLLECTION*)lwgeom, type));
	}
	ctx->lwgeom = lwgeom;
	return true;
}

static bool coerce_geometry(struct mvt_agg_context *ctx)
{
	LWGEOM *lwgeom;

	if (ctx->clip_geoms && !clip_geometry(ctx))
		return false;

	lwgeom = ctx->lwgeom;

	switch (lwgeom->type) {
	case POINTTYPE:
		return true;
	case LINETYPE:
	case POLYGONTYPE:
	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
		if (!check_geometry_size(lwgeom, ctx))
			ctx->lwgeom = lwgeom_centroid(lwgeom);
		return true;
	default: lwerror("encode_geometry: '%s' geometry type not supported",
		lwtype_name(lwgeom->type));
	}

	return true;
}

static void encode_geometry(struct mvt_agg_context *ctx)
{
	LWGEOM *lwgeom = ctx->lwgeom;
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
	default: lwerror("encode_geometry: '%s' geometry type not supported",
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

static void encode_keys(struct mvt_agg_context *ctx)
{
	TupleDesc tupdesc = get_tuple_desc(ctx);
	int natts = tupdesc->natts;
	char **keys = palloc(natts * sizeof(*keys));
	uint32_t i, k = 0;
	bool geom_name_found = false;
	for (i = 0; i < natts; i++) {
		char *key = tupdesc->attrs[i]->attname.data;
		if (strcmp(key, ctx->geom_name) == 0) {
			ctx->geom_index = i;
			geom_name_found = true;
			continue;
		}
		keys[k++] = key;
	}
	if (!geom_name_found)
		lwerror("encode_keys: no column with specificed geom_name found");
	ctx->layer->n_keys = k;
	ctx->layer->keys = keys;
	ReleaseTupleDesc(tupdesc);
}

static VectorTile__Tile__Value *create_value() {
	VectorTile__Tile__Value *value = palloc(sizeof(*value));
	vector_tile__tile__value__init(value);
	return value;
}

#define MVT_CREATE_VALUES(kvtype, hash, hasfield, valuefield) \
{ \
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
	MVT_CREATE_VALUES(mvt_kv_int_value,
		int_values_hash, has_int_value, int_value);
	MVT_CREATE_VALUES(mvt_kv_uint_value,
		uint_values_hash, has_uint_value, uint_value);
	MVT_CREATE_VALUES(mvt_kv_sint_value,
		sint_values_hash, has_sint_value, sint_value);
	MVT_CREATE_VALUES(mvt_kv_bool_value,
		bool_values_hash, has_bool_value, bool_value);

	ctx->layer->n_values = ctx->values_hash_i;
	ctx->layer->values = values;
}


#define MVT_PARSE_VALUE(type, kvtype, hash, valuefield, datumfunc, size) \
{ \
	struct kvtype *kv; \
	type value = datumfunc(datum); \
	HASH_FIND(hh, ctx->hash, &value, size, kv); \
	if (!kv) { \
		kv = palloc(sizeof(*kv)); \
		kv->id = ctx->values_hash_i++; \
		kv->valuefield = value; \
		HASH_ADD(hh, ctx->hash, valuefield, size, kv); \
	} \
	tags[c*2] = k - 1; \
	tags[c*2+1] = kv->id; \
}

static void parse_value_as_string(struct mvt_agg_context *ctx, Oid typoid,
		Datum datum, uint32_t *tags, uint32_t c, uint32_t k) {
	struct mvt_kv_string_value *kv;
	Oid foutoid;
	bool typisvarlena;
	getTypeOutputInfo(typoid, &foutoid, &typisvarlena);
	char *value = OidOutputFunctionCall(foutoid, datum);
	size_t size = strlen(value);
	HASH_FIND(hh, ctx->string_values_hash, &value, size, kv);
	if (!kv) {
		kv = palloc(sizeof(*kv));
		kv->id = ctx->values_hash_i++;
		kv->string_value = value;
		HASH_ADD(hh, ctx->string_values_hash, string_value, size, kv);
	}
	tags[c*2] = k - 1;
	tags[c*2+1] = kv->id;
}

static void parse_values(struct mvt_agg_context *ctx)
{
	uint32_t n_keys = ctx->layer->n_keys;
	uint32_t *tags = palloc(n_keys * 2 * sizeof(*tags));
	bool isnull;
	uint32_t i, k = 0, c = 0;
	TupleDesc tupdesc = get_tuple_desc(ctx);
	int natts = tupdesc->natts;

	for (i = 0; i < natts; i++) {
		if (i == ctx->geom_index)
			continue;
		k++;
		Datum datum = GetAttributeByNum(ctx->row, i+1, &isnull);
		if (isnull)
			continue;
		Oid typoid = getBaseType(tupdesc->attrs[i]->atttypid);
		switch (typoid) {
		case BOOLOID:
			MVT_PARSE_VALUE(bool, mvt_kv_bool_value,
				bool_values_hash, bool_value,
				DatumGetBool, sizeof(bool));
			break;
		case INT2OID:
			MVT_PARSE_VALUE(int16_t, mvt_kv_int_value,
				int_values_hash, int_value,
				DatumGetInt16, sizeof(int64_t));
			break;
		case INT4OID:
			MVT_PARSE_VALUE(int32_t, mvt_kv_int_value,
				int_values_hash, int_value,
				DatumGetInt32, sizeof(int64_t));
			break;
		case INT8OID:
			MVT_PARSE_VALUE(int64_t, mvt_kv_int_value,
				int_values_hash, int_value,
				DatumGetInt64, sizeof(int64_t));
			break;
		case FLOAT4OID:
			MVT_PARSE_VALUE(float, mvt_kv_float_value,
				float_values_hash, float_value,
				DatumGetFloat4, sizeof(float));
			break;
		case FLOAT8OID:
			MVT_PARSE_VALUE(double, mvt_kv_double_value,
				double_values_hash, double_value,
				DatumGetFloat8, sizeof(double));
			break;
		case TEXTOID:
			MVT_PARSE_VALUE(char *, mvt_kv_string_value,
				string_values_hash, string_value,
				TextDatumGetCString, strlen(value));
			break;
		default:
			parse_value_as_string(ctx, typoid, datum, tags, c, k);
			break;
		}
		c++;
	}

	ReleaseTupleDesc(tupdesc);

	ctx->feature->n_tags = c * 2;
	ctx->feature->tags = tags;
}

/**
 * Initialize aggregation context.
 */
void mvt_agg_init_context(struct mvt_agg_context *ctx) 
{
	VectorTile__Tile__Layer *layer;
	double width, height;

	width = ctx->bounds->xmax - ctx->bounds->xmin;
	height = ctx->bounds->ymax - ctx->bounds->ymin;

	if (width == 0 || height == 0)
		lwerror("mvt_agg_init_context: bounds width or height cannot be 0");
	if (ctx->extent == 0)
		lwerror("mvt_agg_init_context: extent cannot be 0");

	ctx->xres = width / ctx->extent;
	ctx->yres = height / ctx->extent;
	ctx->features_capacity = FEATURES_CAPACITY_INITIAL;
	ctx->string_values_hash = NULL;
	ctx->float_values_hash = NULL;
	ctx->double_values_hash = NULL;
	ctx->int_values_hash = NULL;
	ctx->uint_values_hash = NULL;
	ctx->sint_values_hash = NULL;
	ctx->bool_values_hash = NULL;
	ctx->values_hash_i = 0;

	layer = palloc(sizeof(*layer));
	vector_tile__tile__layer__init(layer);
	layer->version = 2;
	layer->name = ctx->name;
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
	VectorTile__Tile__Layer *layer = ctx->layer;
	VectorTile__Tile__Feature **features = layer->features;
	if (layer->n_features >= ctx->features_capacity) {
		size_t new_capacity = ctx->features_capacity * 2;
		layer->features = repalloc(layer->features, new_capacity *
			sizeof(*layer->features));
		ctx->features_capacity = new_capacity;
	}

	VectorTile__Tile__Feature *feature = palloc(sizeof(*feature));
	vector_tile__tile__feature__init(feature);

	ctx->feature = feature;
	if (layer->n_features == 0)
		encode_keys(ctx);

	bool isnull;
	Datum datum = GetAttributeByNum(ctx->row, ctx->geom_index + 1, &isnull);
	if (!datum)
		lwerror("mvt_agg_transfn: geometry column cannot be null");
	GSERIALIZED *gs = (GSERIALIZED *) PG_DETOAST_DATUM(datum);
	ctx->lwgeom = lwgeom_from_gserialized(gs);

	if (!coerce_geometry(ctx))
		return;

	layer->features[layer->n_features++] = feature;

	encode_geometry(ctx);
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
	encode_values(ctx);

	VectorTile__Tile__Layer *layers[1];
	layers[0] = ctx->layer;

	VectorTile__Tile tile = VECTOR_TILE__TILE__INIT;
	tile.n_layers = 1;
	tile.layers = layers;

	size_t len = vector_tile__tile__get_packed_size(&tile);
	uint8_t *buf = palloc(sizeof(*buf) * (len + VARHDRSZ));
	vector_tile__tile__pack(&tile, buf + VARHDRSZ);

	SET_VARSIZE(buf, VARHDRSZ + len);

	return buf;
}

#endif