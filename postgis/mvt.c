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
	bool bool_value;
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
			geom_name_found = 1;
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
	MVT_CREATE_VALUES(mvt_kv_uint_value,
		uint_values_hash, has_uint_value, uint_value);
	MVT_CREATE_VALUES(mvt_kv_sint_value,
		sint_values_hash, has_sint_value, sint_value);
	MVT_CREATE_VALUES(mvt_kv_bool_value,
		bool_values_hash, has_bool_value, bool_value);

	ctx->layer->n_values = ctx->values_hash_i;
	ctx->layer->values = values;
}

#define MVT_PARSE_VALUE(value, kvtype, hash, valuefield, size) \
{ \
	struct kvtype *kv; \
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

#define MVT_PARSE_DATUM(type, kvtype, hash, valuefield, datumfunc, size) \
{ \
	type value = datumfunc(datum); \
	MVT_PARSE_VALUE(value, kvtype, hash, valuefield, size); \
}

#define MVT_PARSE_INT_DATUM(type, datumfunc) \
{ \
	type value = datumfunc(datum); \
	if (value >= 0) { \
		MVT_PARSE_VALUE(value, mvt_kv_uint_value, \
				uint_values_hash, uint_value, \
				sizeof(uint64_t)) \
	} else { \
		MVT_PARSE_VALUE(value, mvt_kv_sint_value, \
				sint_values_hash, sint_value, \
				sizeof(int64_t)) \
	} \
}

static void parse_value_as_string(struct mvt_agg_context *ctx, Oid typoid,
		Datum datum, uint32_t *tags, uint32_t c, uint32_t k)
{
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
			MVT_PARSE_DATUM(bool, mvt_kv_bool_value,
				bool_values_hash, bool_value,
				DatumGetBool, sizeof(bool));
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
		case TEXTOID:
			MVT_PARSE_DATUM(char *, mvt_kv_string_value,
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

static void ptarray_mirror_y(POINTARRAY *pa, uint32_t extent)
{
	int i;
	POINT2D *p;

	for (i = 0; i < pa->npoints; i++) {
		p = (POINT2D *) getPoint_internal(pa, i);
		p->y = extent - p->y;
	}
}

static void lwgeom_mirror_y(LWGEOM *in, uint32_t extent)
{
	LWCOLLECTION *col;
	LWPOLY *poly;
	int i;

	if ( (!in) || lwgeom_is_empty(in) ) return;

	switch (in->type) {
	case POINTTYPE:
		ptarray_mirror_y(lwgeom_as_lwpoint(in)->point, extent);
		break;
	case LINETYPE:
		ptarray_mirror_y(lwgeom_as_lwline(in)->points, extent);
		break;
	case POLYGONTYPE:
		poly = (LWPOLY *) in;
		for (i=0; i<poly->nrings; i++)
			ptarray_mirror_y(poly->rings[i], extent);
		break;
	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case COLLECTIONTYPE:
		col = (LWCOLLECTION *) in;
		for (i=0; i<col->ngeoms; i++)
			lwgeom_mirror_y(col->geoms[i], extent);
		break;
	default:
		lwerror("lwgeom_mirror_y: unsupported geometry type: %s",
		        lwtype_name(in->type));
		return;
	}

	lwgeom_drop_bbox(in);
	lwgeom_add_bbox(in);
}

/**
 * Transform a geometry into vector tile coordinate space.
 *
 * Makes best effort to keep validity. Might collapse geometry into lower
 * dimension.
 */
LWGEOM *mvt_geom(LWGEOM *lwgeom, GBOX *gbox, uint32_t extent, uint32_t buffer, 
		 bool clip_geom)
{
	double width = gbox->xmax - gbox->xmin;
	double height = gbox->ymax - gbox->ymin;
	double resx = width / extent;
	double resy = height / extent;

	if (width == 0 || height == 0)
		lwerror("mvt_geom: bounds width or height cannot be 0");

	if (extent == 0)
		lwerror("mvt_geom: extent cannot be 0");

	if (clip_geom) {
		double buffer_map_xunits = resx * buffer;
		double buffer_map_yunits = resy * buffer;
		double x0 = gbox->xmin - buffer_map_xunits;
		double y0 = gbox->ymin - buffer_map_yunits;
		double x1 = gbox->xmax + buffer_map_xunits;
		double y1 = gbox->ymax + buffer_map_yunits;
#if POSTGIS_GEOS_VERSION < 35
		LWPOLY *lwenv = lwpoly_construct_envelope(0, x0, y0, x1, y1);
		lwgeom = lwgeom_intersection(lwgeom, lwpoly_as_lwgeom(lwenv));
		lwpoly_free(lwenv);
#else
		lwgeom = lwgeom_clip_by_rect(lwgeom, x0, y0, x1, y1);
#endif
	}

	POINT4D factors;
	factors.x = resx;
	factors.y = resy;
	factors.z = 1;
	factors.m = 1;

	lwgeom_scale(lwgeom, &factors);

	gridspec grid;
	memset(&grid, 0, sizeof(gridspec));
	grid.ipx = 0;
	grid.ipy = 0;
	grid.xsize = 1;
	grid.ysize = 1;

	LWGEOM *lwgeom_out = lwgeom_grid(lwgeom, &grid);

	if (lwgeom_out == NULL)
		lwgeom_out = lwgeom_grid(lwgeom_centroid(lwgeom), &grid);

	lwgeom_force_clockwise(lwgeom_out);
	lwgeom_mirror_y(lwgeom_out, extent);
	lwgeom_out = lwgeom_make_valid(lwgeom_out);

	return lwgeom_out;
}

/**
 * Initialize aggregation context.
 */
void mvt_agg_init_context(struct mvt_agg_context *ctx) 
{
	VectorTile__Tile__Layer *layer;

	if (ctx->extent == 0)
		lwerror("mvt_agg_init_context: extent cannot be 0");

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
	LWGEOM *lwgeom = lwgeom_from_gserialized(gs);

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