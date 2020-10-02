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

#include <math.h>
#include "geobuf.h"
#include "pgsql_compat.h"

#if defined HAVE_LIBPROTOBUF

#define FEATURES_CAPACITY_INITIAL 50
#define MAX_PRECISION 1e6

static Data__Geometry *encode_geometry(struct geobuf_agg_context *ctx,
	LWGEOM *lwgeom);

static Data__Geometry *galloc(Data__Geometry__Type type) {
	Data__Geometry *geometry;
	geometry = palloc (sizeof (Data__Geometry));
	data__geometry__init(geometry);
	geometry->type = type;
	return geometry;
}

static TupleDesc get_tuple_desc(struct geobuf_agg_context *ctx)
{
	Oid tupType = HeapTupleHeaderGetTypeId(ctx->row);
	int32 tupTypmod = HeapTupleHeaderGetTypMod(ctx->row);
	TupleDesc tupdesc = lookup_rowtype_tupdesc(tupType, tupTypmod);
	return tupdesc;
}

static void encode_keys(struct geobuf_agg_context *ctx)
{
	TupleDesc tupdesc = get_tuple_desc(ctx);
	uint32_t natts = (uint32_t) tupdesc->natts;
	char **keys = palloc(natts * sizeof(*keys));
	uint32_t i, k = 0;
	bool geom_found = false;
	for (i = 0; i < natts; i++) {
		Oid typoid = getBaseType(TupleDescAttr(tupdesc, i)->atttypid);
		char *tkey = TupleDescAttr(tupdesc, i)->attname.data;
		char *key = pstrdup(tkey);
		if (ctx->geom_name == NULL) {
			if (!geom_found && typoid == postgis_oid(GEOMETRYOID)) {
				ctx->geom_index = i;
				geom_found = 1;
				continue;
			}
		} else {
			if (!geom_found && strcmp(key, ctx->geom_name) == 0) {
				ctx->geom_index = i;
				geom_found = 1;
				continue;
			}
		}
		keys[k++] = key;
	}
	if (!geom_found)
		elog(ERROR, "encode_keys: no geometry column found");
	ctx->data->n_keys = k;
	ctx->data->keys = keys;
	ReleaseTupleDesc(tupdesc);
}


static void set_int_value(Data__Value *value, int64 intval) {
	if (intval >= 0) {
		value->value_type_case = DATA__VALUE__VALUE_TYPE_POS_INT_VALUE;
		value->pos_int_value = (uint64_t) intval;
	} else {
		value->value_type_case = DATA__VALUE__VALUE_TYPE_NEG_INT_VALUE;
		value->neg_int_value = (uint64_t)llabs(intval);
	}
}

static void encode_properties(struct geobuf_agg_context *ctx,
	Data__Feature *feature)
{
	uint32_t *properties;
	Data__Value **values;
	uint32_t i, k = 0, c = 0;
	TupleDesc tupdesc = get_tuple_desc(ctx);
	uint32_t natts = (uint32_t) tupdesc->natts;
	properties = palloc(sizeof (*properties) * (natts - 1) * 2);
	values = palloc (sizeof (*values) * (natts - 1));

	for (i = 0; i < natts; i++) {
		Data__Value *value;
		char *type, *string_value;
		Datum datum;
		bool isnull;
		Oid typoid;

		if (i == ctx->geom_index)
			continue;
		k++;

		value = palloc (sizeof (*value));
		data__value__init(value);

		type = SPI_gettype(tupdesc, i + 1);
		datum = GetAttributeByNum(ctx->row, i + 1, &isnull);
		if (isnull)
			continue;

		typoid = getBaseType(TupleDescAttr(tupdesc, i)->atttypid);

		if (strcmp(type, "int2") == 0) {
			set_int_value(value, DatumGetInt16(datum));
		} else if (strcmp(type, "int4") == 0) {
			set_int_value(value, DatumGetInt32(datum));
		} else if (strcmp(type, "int8") == 0) {
			set_int_value(value, DatumGetInt64(datum));
		} else if (strcmp(type, "float4") == 0) {
			value->value_type_case = DATA__VALUE__VALUE_TYPE_DOUBLE_VALUE;
			value->double_value = DatumGetFloat4(datum);
		} else if (strcmp(type, "float8") == 0) {
			value->value_type_case = DATA__VALUE__VALUE_TYPE_DOUBLE_VALUE;
			value->double_value = DatumGetFloat8(datum);
		} else {
			Oid foutoid;
			bool typisvarlena;
			getTypeOutputInfo(typoid, &foutoid, &typisvarlena);
			string_value = OidOutputFunctionCall(foutoid, datum);
			value->value_type_case = DATA__VALUE__VALUE_TYPE_STRING_VALUE;
			value->string_value = string_value;
		}
		properties[c * 2] = k - 1;
		properties[c * 2 + 1] = c;
		values[c++] = value;
	}

	ReleaseTupleDesc(tupdesc);

	feature->n_values = c;
	feature->values = values;
	feature->n_properties = c * 2;
	feature->properties = properties;
}

static int64_t *encode_coords(struct geobuf_agg_context *ctx, POINTARRAY *pa,
	int64_t *coords, int len, int offset)
{
	int i, c;
	POINT4D pt;
	int64_t sum[] = { 0, 0, 0, 0 };

	if (offset == 0)
		coords = palloc(sizeof (int64_t) * len * ctx->dimensions);
	else
		coords = repalloc(coords, sizeof (int64_t) *
			((len * ctx->dimensions) + offset));

	c = offset;
	for (i = 0; i < len; i++) {
		getPoint4d_p(pa, i, &pt);
		sum[0] += coords[c++] = (int64_t) (ceil(pt.x * ctx->e) - sum[0]);
		sum[1] += coords[c++] = (int64_t) (ceil(pt.y * ctx->e) - sum[1]);
		if (ctx->dimensions == 3)
			sum[2] += coords[c++] = (int64_t) (ceil(pt.z * ctx->e) - sum[2]);
		else if (ctx->dimensions == 4)
			sum[3] += coords[c++] = (int64_t) (ceil(pt.m * ctx->e) - sum[3]);
	}
	return coords;
}

static Data__Geometry *encode_point(struct geobuf_agg_context *ctx,
	LWPOINT *lwpoint)
{
	int npoints;
	Data__Geometry *geometry;
	POINTARRAY *pa;

	geometry = galloc(DATA__GEOMETRY__TYPE__POINT);

	pa = lwpoint->point;
	npoints = pa->npoints;

	if (npoints == 0)
		return geometry;

	geometry->n_coords = npoints * ctx->dimensions;
	geometry->coords = encode_coords(ctx, pa, NULL, 1, 0);

	return geometry;
}

static Data__Geometry *encode_mpoint(struct geobuf_agg_context *ctx,
	LWMPOINT *lwmpoint)
{
	int i, ngeoms;
	POINTARRAY *pa;
	Data__Geometry *geometry;

	geometry = galloc(DATA__GEOMETRY__TYPE__MULTIPOINT);

	ngeoms = lwmpoint->ngeoms;

	if (ngeoms == 0)
		return geometry;

	pa = ptarray_construct_empty(0, 0, ngeoms);

	for (i = 0; i < ngeoms; i++) {
		POINT4D pt;
		getPoint4d_p(lwmpoint->geoms[i]->point, 0, &pt);
		ptarray_append_point(pa, &pt, 0);
	}

	geometry->n_coords = ngeoms * ctx->dimensions;
	geometry->coords = encode_coords(ctx, pa, NULL, ngeoms, 0);

	return geometry;
}

static Data__Geometry *encode_line(struct geobuf_agg_context *ctx,
	LWLINE *lwline)
{
	POINTARRAY *pa;
	Data__Geometry *geometry;

	geometry = galloc(DATA__GEOMETRY__TYPE__LINESTRING);

	pa = lwline->points;

	if (pa->npoints == 0)
		return geometry;

	geometry->n_coords = pa->npoints * ctx->dimensions;
	geometry->coords = encode_coords(ctx, pa, NULL, pa->npoints, 0);

	return geometry;
}

static Data__Geometry *
encode_triangle(struct geobuf_agg_context *ctx, LWTRIANGLE *lwtri)
{
	Data__Geometry *geometry = galloc(DATA__GEOMETRY__TYPE__POLYGON);
	POINTARRAY *pa = lwtri->points;
	uint32_t len;

	if (pa->npoints == 0)
		return geometry;

	len = pa->npoints - 1;
	geometry->n_coords = len * ctx->dimensions;
	geometry->coords = encode_coords(ctx, pa, NULL, len, 0);

	return geometry;
}

static Data__Geometry *encode_mline(struct geobuf_agg_context *ctx,
	LWMLINE *lwmline)
{
	int i, offset, ngeoms;
	POINTARRAY *pa;
	Data__Geometry *geometry;
	uint32_t *lengths;
	int64_t *coords = NULL;

	geometry = galloc(DATA__GEOMETRY__TYPE__MULTILINESTRING);

	ngeoms = lwmline->ngeoms;

	if (ngeoms == 0)
		return geometry;

	lengths = palloc (sizeof (uint32_t) * ngeoms);

	offset = 0;
	for (i = 0; i < ngeoms; i++) {
		pa = lwmline->geoms[i]->points;
		coords = encode_coords(ctx, pa, coords, pa->npoints, offset);
		offset += pa->npoints * ctx->dimensions;
		lengths[i] = pa->npoints;
	}

	if (ngeoms > 1) {
		geometry->n_lengths = ngeoms;
		geometry->lengths = lengths;
	}

	geometry->n_coords = offset;
	geometry->coords = coords;

	return geometry;
}

static Data__Geometry *encode_poly(struct geobuf_agg_context *ctx,
	LWPOLY *lwpoly)
{
	int i, len, nrings, offset;
	POINTARRAY *pa;
	Data__Geometry *geometry;
	uint32_t *lengths;
	int64_t *coords = NULL;

	geometry = galloc(DATA__GEOMETRY__TYPE__POLYGON);

	nrings = lwpoly->nrings;

	if (nrings == 0)
		return geometry;

	lengths = palloc (sizeof (uint32_t) * nrings);

	offset = 0;
	for (i = 0; i < nrings; i++) {
		pa = lwpoly->rings[i];
		len = pa->npoints - 1;
		coords = encode_coords(ctx, pa, coords, len, offset);
		offset += len * ctx->dimensions;
		lengths[i] = len;
	}

	if (nrings > 1) {
		geometry->n_lengths = nrings;
		geometry->lengths = lengths;
	}

	geometry->n_coords = offset;
	geometry->coords = coords;

	return geometry;
}

static Data__Geometry *encode_mpoly(struct geobuf_agg_context *ctx,
	LWMPOLY* lwmpoly)
{
	int i, j, c, len, offset, n_lengths, ngeoms, nrings;
	POINTARRAY *pa;
	Data__Geometry *geometry;
	uint32_t *lengths;
	int64_t *coords = NULL;

	geometry = galloc(DATA__GEOMETRY__TYPE__MULTIPOLYGON);

	ngeoms = lwmpoly->ngeoms;

	if (ngeoms == 0) return geometry;

	n_lengths = 1;
	for (i = 0; i < ngeoms; i++) {
		nrings = lwmpoly->geoms[i]->nrings;
		n_lengths++;
		for (j = 0; j < nrings; j++)
			n_lengths++;
	}

	lengths = palloc (sizeof (uint32_t) * n_lengths);

	c = 0;
	offset = 0;
	lengths[c++] = ngeoms;
	for (i = 0; i < ngeoms; i++) {
		nrings = lwmpoly->geoms[i]->nrings;
		lengths[c++] = nrings;
		for (j = 0; j < nrings; j++) {
			pa = lwmpoly->geoms[i]->rings[j];
			len = pa->npoints - 1;
			coords = encode_coords(ctx, pa, coords, len, offset);
			offset += len * ctx->dimensions;
			lengths[c++] = len;
		}
	}

	if (c > 1) {
		geometry->n_lengths = n_lengths;
		geometry->lengths = lengths;
	}

	geometry->n_coords = offset;
	geometry->coords = coords;

	return geometry;
}

static Data__Geometry *encode_collection(struct geobuf_agg_context *ctx,
	LWCOLLECTION* lwcollection)
{
	int i, ngeoms;
	Data__Geometry *geometry, **geometries;

	geometry = galloc(DATA__GEOMETRY__TYPE__GEOMETRYCOLLECTION);

	ngeoms = lwcollection->ngeoms;

	if (ngeoms == 0)
		return geometry;

	geometries = palloc (sizeof (Data__Geometry *) * ngeoms);
	for (i = 0; i < ngeoms; i++) {
		LWGEOM *lwgeom = lwcollection->geoms[i];
		Data__Geometry *geom = encode_geometry(ctx, lwgeom);
		geometries[i] = geom;
	}

	geometry->n_geometries = ngeoms;
	geometry->geometries = geometries;

	return geometry;
}

static Data__Geometry *encode_geometry(struct geobuf_agg_context *ctx,
	LWGEOM *lwgeom)
{
	int type = lwgeom->type;
	switch (type)
	{
	case POINTTYPE:
		return encode_point(ctx, (LWPOINT*)lwgeom);
	case LINETYPE:
		return encode_line(ctx, (LWLINE*)lwgeom);
	case TRIANGLETYPE:
		return encode_triangle(ctx, (LWTRIANGLE *)lwgeom);
	case POLYGONTYPE:
		return encode_poly(ctx, (LWPOLY*)lwgeom);
	case MULTIPOINTTYPE:
		return encode_mpoint(ctx, (LWMPOINT*)lwgeom);
	case MULTILINETYPE:
		return encode_mline(ctx, (LWMLINE*)lwgeom);
	case MULTIPOLYGONTYPE:
		return encode_mpoly(ctx, (LWMPOLY*)lwgeom);
	case COLLECTIONTYPE:
	case TINTYPE:
		return encode_collection(ctx, (LWCOLLECTION*)lwgeom);
	default:
		elog(ERROR, "encode_geometry: '%s' geometry type not supported",
				lwtype_name(type));
	}
	return NULL;
}

static void analyze_val(struct geobuf_agg_context *ctx, double val)
{
	if (fabs((round(val * ctx->e) / ctx->e) - val) >= EPSILON &&
		ctx->e < MAX_PRECISION)
		ctx->e *= 10;
}

static void analyze_pa(struct geobuf_agg_context *ctx, POINTARRAY *pa)
{
	uint32_t i;
	POINT4D pt;
	for (i = 0; i < pa->npoints; i++) {
		getPoint4d_p(pa, i, &pt);
		analyze_val(ctx, pt.x);
		analyze_val(ctx, pt.y);
		if (ctx->dimensions == 3)
			analyze_val(ctx, pt.z);
		if (ctx->dimensions == 4)
			analyze_val(ctx, pt.m);
	}
}

static void analyze_geometry(struct geobuf_agg_context *ctx, LWGEOM *lwgeom)
{
	uint32_t i, type;
	LWLINE *lwline;
	LWPOLY *lwpoly;
	LWCOLLECTION *lwcollection;
	type = lwgeom->type;
	switch (type)
	{
	case POINTTYPE:
	case LINETYPE:
	case TRIANGLETYPE:
		lwline = (LWLINE*) lwgeom;
		analyze_pa(ctx, lwline->points);
		break;
	case POLYGONTYPE:
		lwpoly = (LWPOLY*) lwgeom;
		for (i = 0; i < lwpoly->nrings; i++)
			analyze_pa(ctx, lwpoly->rings[i]);
		break;
	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case COLLECTIONTYPE:
	case TINTYPE:
		lwcollection = (LWCOLLECTION*) lwgeom;
		for (i = 0; i < lwcollection->ngeoms; i++)
			analyze_geometry(ctx, lwcollection->geoms[i]);
		break;
	default:
		elog(ERROR, "analyze_geometry: '%s' geometry type not supported",
			lwtype_name(type));
	}
}

static void analyze_geometry_flags(struct geobuf_agg_context *ctx,
	LWGEOM *lwgeom)
{
	if (!ctx->has_dimensions)
	{
		if (lwgeom_has_z(lwgeom) && lwgeom_has_m(lwgeom))
			ctx->dimensions = 4;
		else if (lwgeom_has_z(lwgeom) || lwgeom_has_m(lwgeom))
			ctx->dimensions = 3;
		else
			ctx->dimensions = 2;
		ctx->has_dimensions = 1;
	}
}

static Data__Feature *encode_feature(struct geobuf_agg_context *ctx)
{
	Data__Feature *feature;

	feature = palloc (sizeof (Data__Feature));
	data__feature__init(feature);

	encode_properties(ctx, feature);
	return feature;
}

/**
 * Initialize aggregation context.
 */
void geobuf_agg_init_context(struct geobuf_agg_context *ctx)
{
	Data *data;
	Data__FeatureCollection *fc;

	ctx->has_dimensions = 0;
	ctx->dimensions = 2;
	ctx->has_precision = 0;
	ctx->precision = MAX_PRECISION;
	ctx->e = 1;
	ctx->features_capacity = FEATURES_CAPACITY_INITIAL;

	data = palloc(sizeof(*data));
	data__init(data);

	fc = palloc(sizeof(*fc));
	data__feature_collection__init(fc);

	fc->features = palloc (ctx->features_capacity *
		sizeof(*fc->features));

	ctx->lwgeoms = palloc (ctx->features_capacity *
		sizeof(*ctx->lwgeoms));

	data->data_type_case = DATA__DATA_TYPE_FEATURE_COLLECTION;
	data->feature_collection = fc;

	ctx->data = data;
}

/**
 * Aggregation step.
 *
 * Expands features array if needed by a factor of 2.
 * Allocates a new feature, increment feature counter and
 * encode properties into it.
 */
void geobuf_agg_transfn(struct geobuf_agg_context *ctx)
{
	LWGEOM *lwgeom;
	bool isnull = false;
	Datum datum;
	Data__FeatureCollection *fc = ctx->data->feature_collection;
/*	Data__Feature **features = fc->features; */
	Data__Feature *feature;
	GSERIALIZED *gs;
	if (fc->n_features >= ctx->features_capacity) {
		size_t new_capacity = ctx->features_capacity * 2;
		fc->features = repalloc(fc->features, new_capacity *
			sizeof(*fc->features));
		ctx->lwgeoms = repalloc(ctx->lwgeoms, new_capacity *
			sizeof(*ctx->lwgeoms));
		ctx->features_capacity = new_capacity;
	}

	/* inspect row and encode keys assuming static schema */
	if (fc->n_features == 0)
		encode_keys(ctx);

	datum = GetAttributeByNum(ctx->row, ctx->geom_index + 1, &isnull);
	if (isnull)
		return;

	gs = (GSERIALIZED *) PG_DETOAST_DATUM_COPY(datum);
	lwgeom = lwgeom_from_gserialized(gs);

	feature = encode_feature(ctx);

	/* inspect geometry flags assuming static schema */
	if (fc->n_features == 0)
		analyze_geometry_flags(ctx, lwgeom);

	analyze_geometry(ctx, lwgeom);

	ctx->lwgeoms[fc->n_features] = lwgeom;
	fc->features[fc->n_features++] = feature;
}

/**
 * Finalize aggregation.
 *
 * Encode into Data message and return it packed as a bytea.
 */
uint8_t *geobuf_agg_finalfn(struct geobuf_agg_context *ctx)
{
	size_t i, len;
	Data *data;
	Data__FeatureCollection *fc;
	uint8_t *buf;

	data = ctx->data;
	fc = data->feature_collection;

	/* check and set dimensions if not default */
	if (ctx->dimensions != 2) {
		data->has_dimensions = ctx->has_dimensions;
		data->dimensions = ctx->dimensions;
	}

	/* check and set precision if not default */
	if (ctx->e > MAX_PRECISION)
		ctx->e = MAX_PRECISION;
	ctx->precision = ceil(log(ctx->e) / log(10));
	if (ctx->precision != 6) {
		data->has_precision = 1;
		data->precision = ctx->precision;
	}

	for (i = 0; i < fc->n_features; i++)
		fc->features[i]->geometry = encode_geometry(ctx, ctx->lwgeoms[i]);

	len = data__get_packed_size(data);
	buf = palloc(sizeof(*buf) * (len + VARHDRSZ));
	data__pack(data, buf + VARHDRSZ);

	SET_VARSIZE(buf, VARHDRSZ + len);

	return buf;
}

#endif
