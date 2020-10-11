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

#include <math.h>
#include "flatgeobuf.h"
#include "pgsql_compat.h"
#include "funcapi.h"
#include "parser/parse_type.h"

uint8_t magicbytes[] = { 0x66, 0x67, 0x62, 0x03, 0x66, 0x67, 0x62, 0x00 };
size_t MAGICBYTES_LEN = (sizeof(magicbytes) / sizeof((magicbytes)[0]));

static uint8_t get_geometrytype(LWGEOM *lwgeom) {
	int type = lwgeom->type;
	switch (type)
	{
	case POINTTYPE:
		return GeometryType_Point;
	case LINETYPE:
		return GeometryType_LineString;
	case TRIANGLETYPE:
		return GeometryType_Triangle;
	case POLYGONTYPE:
		return GeometryType_Polygon;
	case MULTIPOINTTYPE:
		return GeometryType_MultiPoint;
	case MULTILINETYPE:
		return GeometryType_MultiLineString;
	case MULTIPOLYGONTYPE:
		return GeometryType_MultiPolygon;
	//case COLLECTIONTYPE:
	//case TINTYPE:
	//	return encode_collection(ctx, (LWCOLLECTION*)lwgeom);
	default:
		elog(ERROR, "get_geometrytype: '%s' geometry type not supported",
				lwtype_name(type));
	}
	return 0;
}

static void encode_header(struct flatgeobuf_encode_ctx *ctx)
{
	size_t size;
	uint8_t *header;
	flatcc_builder_t builder, *B;

	Oid tupType = HeapTupleHeaderGetTypeId(ctx->row);
	int32 tupTypmod = HeapTupleHeaderGetTypMod(ctx->row);
	TupleDesc tupdesc = lookup_rowtype_tupdesc(tupType, tupTypmod);
	ctx->tupdesc = tupdesc;

	// inspect first geometry
	if (ctx->lwgeom != NULL) {
		ctx->hasZ = lwgeom_has_z(ctx->lwgeom);
		ctx->hasM = lwgeom_has_m(ctx->lwgeom);
		ctx->lwgeom_type = ctx->lwgeom->type;
		ctx->geometry_type = get_geometrytype(ctx->lwgeom);
	} else {
		ctx->geometry_type = GeometryType_Unknown;
	}

	B = &builder;
	flatcc_builder_init(B);
	//flatbuffers_double_vec_ref_t envelope;
	//Column_vec_start(B);
	//Column_vec_t columns = Column_vec_start(B);
	//uint16_t index_node_size = 16;
	//Crs_vec_start(B);
	//Crs_vec_t crs = Crs_vec_end(B);
	Header_start_as_root_with_size(B);
	Header_name_create_str(B, "");
	Header_geometry_type_add(B, ctx->geometry_type);
	Header_hasZ_add(B, ctx->hasZ);
	Header_hasM_add(B, ctx->hasM);
	//Header_hasT_add(B, false);
	//Header_hasTM_add(B, false);

	// inspect columns

	Header_end_as_root(B);

	header = flatcc_builder_finalize_buffer(B, &size);
	ctx->buf = repalloc(ctx->buf, ctx->offset + size);
	memcpy(ctx->buf + ctx->offset, header, size);
	free(header);
	ReleaseTupleDesc(tupdesc);

	ctx->offset += size;
}

static void encode_point(struct flatgeobuf_encode_ctx *ctx, LWPOINT *lwpt)
{
	POINT4D pt;
	flatcc_builder_t *B = ctx->B;
	getPoint4d_p(lwpt->point, 0, &pt);
	Geometry_start(B);
	Geometry_xy_start(B);
	Geometry_xy_push_create(B, pt.x);
	Geometry_xy_push_create(B, pt.y);
	Geometry_xy_end(B);
	if (ctx->hasZ) {
		Geometry_z_start(B);
		Geometry_z_push_create(B, pt.z);
		Geometry_z_end(B);
	}
	if (ctx->hasM) {
		Geometry_m_start(B);
		Geometry_m_push_create(B, pt.m);
		Geometry_m_end(B);
	}
	Feature_geometry_add(B, Geometry_end(B));
}

static void encode_line_pa(struct flatgeobuf_encode_ctx *ctx, POINTARRAY *pa)
{
	POINT4D pt;
	flatcc_builder_t *B = ctx->B;
	Geometry_start(B);
	Geometry_xy_start(B);
	for (int i = 0; i < pa->npoints; i++) {
		getPoint4d_p(pa, i, &pt);
		Geometry_xy_push_create(B, pt.x);
		Geometry_xy_push_create(B, pt.y);
	}
	Geometry_xy_end(B);
	if (ctx->hasZ) {
		Geometry_z_start(B);
		for (int i = 0; i < pa->npoints; i++) {
			getPoint4d_p(pa, i, &pt);
			Geometry_z_push_create(B, pt.z);
		}
		Geometry_z_end(B);
	}
	if (ctx->hasM) {
		Geometry_m_start(B);
		for (int i = 0; i < pa->npoints; i++) {
			getPoint4d_p(pa, i, &pt);
			Geometry_m_push_create(B, pt.m);
		}
		Geometry_m_end(B);
	}
	Feature_geometry_add(B, Geometry_end(B));
}

static void encode_line_ppa(struct flatgeobuf_encode_ctx *ctx, POINTARRAY **ppa, size_t len)
{
	POINT4D pt;
	flatcc_builder_t *B = ctx->B;
	uint32_t *ends = lwalloc(sizeof(uint32_t) * len);
	uint32_t offset = 0;

	Geometry_start(B);
	Geometry_xy_start(B);
	for (int n = 0; n < len; n++) {
		POINTARRAY *pa = ppa[n];
		for (int i = 0; i < pa->npoints; i++) {
			getPoint4d_p(pa, i, &pt);
			Geometry_xy_push_create(B, pt.x);
			Geometry_xy_push_create(B, pt.y);
		}
		offset += pa->npoints;
		ends[n] = offset;
	}
	Geometry_xy_end(B);
	Geometry_ends_create(B, ends, len);
	if (ctx->hasZ) {
		Geometry_z_start(B);
		for (int n = 0; n < len; n++) {
			POINTARRAY *pa = ppa[n];
			for (int i = 0; i < pa->npoints; i++) {
				getPoint4d_p(pa, i, &pt);
				Geometry_z_push_create(B, pt.z);
			}
		}
		Geometry_z_end(B);
	}
	if (ctx->hasM) {
		Geometry_m_start(B);
		for (int n = 0; n < len; n++) {
			POINTARRAY *pa = ppa[n];
			for (int i = 0; i < pa->npoints; i++) {
				getPoint4d_p(pa, i, &pt);
				Geometry_m_push_create(B, pt.m);
			}
		}
		Geometry_m_end(B);
	}
	Feature_geometry_add(B, Geometry_end(B));
}

static void encode_line(struct flatgeobuf_encode_ctx *ctx, LWLINE *lwline)
{
	encode_line_pa(ctx, lwline->points);
}

static void encode_triangle(struct flatgeobuf_encode_ctx *ctx, LWTRIANGLE *lwtriangle)
{
	encode_line_pa(ctx, lwtriangle->points);
}

static void encode_poly(struct flatgeobuf_encode_ctx *ctx, LWPOLY *lwpoly)
{
	uint32_t nrings = lwpoly->nrings;
	POINTARRAY **ppa = lwpoly->rings;
	if (nrings == 0)
		return;
	if (nrings == 1)
		encode_line_pa(ctx, ppa[0]);
	else
		encode_line_ppa(ctx, ppa, nrings);
}

static void encode_mpoint(struct flatgeobuf_encode_ctx *ctx, LWMPOINT *lwmpoint)
{
	LWLINE *lwline = lwline_from_lwmpoint(0, lwmpoint);
	encode_line_pa(ctx, lwline->points);
}

static void encode_geometry(struct flatgeobuf_encode_ctx *ctx)
{
	LWGEOM *lwgeom = ctx->lwgeom;

	if (lwgeom == NULL)
		return;

	if (ctx->geometry_type != GeometryType_Unknown && lwgeom->type != ctx->geometry_type)
		elog(ERROR, "encode_geometry: unexpected geometry type '%s'", lwtype_name(lwgeom->type));

	switch (lwgeom->type) {
	case POINTTYPE:
		return encode_point(ctx, (LWPOINT *) lwgeom);
	case LINETYPE:
		return encode_line(ctx, (LWLINE*)lwgeom);
	case TRIANGLETYPE:
		return encode_triangle(ctx, (LWTRIANGLE *)lwgeom);
	case POLYGONTYPE:
		return encode_poly(ctx, (LWPOLY*)lwgeom);
	case MULTIPOINTTYPE:
		return encode_mpoint(ctx, (LWMPOINT*)lwgeom);
	/*case MULTILINETYPE:
		return encode_mline(ctx, (LWMLINE*)lwgeom);
	case MULTIPOLYGONTYPE:
		return encode_mpoly(ctx, (LWMPOLY*)lwgeom);
	case COLLECTIONTYPE:
	case TINTYPE:
		return encode_collection(ctx, (LWCOLLECTION*)lwgeom);*/
	default:
		elog(ERROR, "encode_geometry: '%s' geometry type not supported",
				lwtype_name(lwgeom->type));
	}
}

static void encode_feature(struct flatgeobuf_encode_ctx *ctx)
{
	size_t size;
	uint8_t *feature;
	flatcc_builder_t builder, *B;

	B = &builder;
	flatcc_builder_init(B);
	ctx->B = B;
	
	Feature_start_as_root_with_size(B);
	encode_geometry(ctx);
	// TODO: encode feature properties
	Feature_end_as_root(B);
	feature = flatcc_builder_finalize_buffer(B, &size);

	ctx->buf = repalloc(ctx->buf, ctx->offset + size);
	memcpy(ctx->buf + ctx->offset, feature, size);
	free(feature);
	ctx->offset += size;
	ctx->features_count++;
}

void flatgeobuf_check_magicbytes(struct flatgeobuf_decode_ctx *ctx)
{
	uint8_t *buf = ctx->buf + ctx->offset;
	for (int i = 0; i < MAGICBYTES_LEN; i++)
		if (ctx->buf[i] != magicbytes[i])
			elog(ERROR, "Data is not FlatGeobuf");
	ctx->offset += MAGICBYTES_LEN;
}

void flatgeobuf_decode_header(struct flatgeobuf_decode_ctx *ctx)
{
	Header_table_t header;
	Column_vec_t columns;
	size_t size;
	
	flatbuffers_read_size_prefix(ctx->buf + ctx->offset, &size);
	ctx->offset += sizeof(flatbuffers_uoffset_t);
	
	header = Header_as_root(ctx->buf + ctx->offset);
	ctx->offset += size;
	
	columns = Header_columns(header);
	
	ctx->geometry_type = Header_geometry_type(header);
	ctx->hasZ = Header_hasZ(header);
	ctx->hasM = Header_hasM(header);
	ctx->hasT = Header_hasT(header);
	ctx->hasTM = Header_hasTM(header);
	ctx->columns_len = Column_vec_len(columns);
	if (ctx->tupdesc->natts != ctx->columns_len + 2)
		elog(ERROR, "Mismatched column structure");

	ctx->header = header;
}

static Datum decode_point(struct flatgeobuf_decode_ctx *ctx, Geometry_table_t geometry)
{
	POINTARRAY *pa;
	LWGEOM *lwgeom;
	POINT4D pt;

	flatbuffers_double_vec_t xy = Geometry_xy(geometry);

	double x = flatbuffers_double_vec_at(xy, 0);
	double y = flatbuffers_double_vec_at(xy, 1);

	if (ctx->hasZ && ctx->hasM) {
		flatbuffers_double_vec_t za = Geometry_z(geometry);
		flatbuffers_double_vec_t ma = Geometry_m(geometry);
		double z = flatbuffers_double_vec_at(za, 0);
		double m = flatbuffers_double_vec_at(ma, 0);
		pt = (POINT4D) { x, y, z, m };
	} else if (ctx->hasZ) {
		flatbuffers_double_vec_t za = Geometry_z(geometry);
		double z = flatbuffers_double_vec_at(za, 0);
		pt = (POINT4D) { x, y, z, 0 };
	} else if (ctx->hasM) {
		flatbuffers_double_vec_t ma = Geometry_m(geometry);
		double m = flatbuffers_double_vec_at(ma, 0);
		pt = (POINT4D) { x, y, 0, m };
	} else {
		pt = (POINT4D) { x, y, 0, 0 };
	}
	
	pa = ptarray_construct_empty(ctx->hasZ, ctx->hasM, 1);
	ptarray_append_point(pa, &pt, LW_TRUE);
	lwgeom = (LWGEOM *) lwpoint_construct(0, NULL, pa);
	return (Datum) geometry_serialize(lwgeom);
}

static POINTARRAY *decode_line_pa(struct flatgeobuf_decode_ctx *ctx, Geometry_table_t geometry, uint32 end, uint32 offset)
{
	POINTARRAY *pa;
	LWGEOM *lwgeom;
	POINT4D pt;

	flatbuffers_double_vec_t xy = Geometry_xy(geometry);

	size_t npoints;
	size_t xy_len;

	if (end != 0) {
		npoints = end - offset;
		xy_len = npoints * 2;
	} else {
		xy_len = flatbuffers_double_vec_len(xy);
		npoints = xy_len / 2;
		end = npoints;
	}

	pa = ptarray_construct_empty(ctx->hasZ, ctx->hasM, npoints);

	if (ctx->hasZ && ctx->hasM) {
		for (size_t i = offset; i < end; i++) {
			double x = flatbuffers_double_vec_at(xy, (i * 2));
			double y = flatbuffers_double_vec_at(xy, (i * 2) + 1);
			flatbuffers_double_vec_t za = Geometry_z(geometry);
			flatbuffers_double_vec_t ma = Geometry_m(geometry);
			double z = flatbuffers_double_vec_at(za, i);
			double m = flatbuffers_double_vec_at(ma, i);
			pt = (POINT4D) { x, y, z, m };
			ptarray_append_point(pa, &pt, LW_TRUE);
		}
	} else if (ctx->hasZ) {
		for (size_t i = offset; i < end; i++) {
			double x = flatbuffers_double_vec_at(xy, (i * 2));
			double y = flatbuffers_double_vec_at(xy, (i * 2) + 1);
			flatbuffers_double_vec_t za = Geometry_z(geometry);
			double z = flatbuffers_double_vec_at(za, i);
			pt = (POINT4D) { x, y, z,0 };
			ptarray_append_point(pa, &pt, LW_TRUE);
		}
	} else if (ctx->hasM) {
		for (size_t i = offset; i < end; i++) {
			double x = flatbuffers_double_vec_at(xy, (i * 2));
			double y = flatbuffers_double_vec_at(xy, (i * 2) + 1);
			flatbuffers_double_vec_t ma = Geometry_m(geometry);
			double m = flatbuffers_double_vec_at(ma, i);
			pt = (POINT4D) { x, y, 0, m };
			ptarray_append_point(pa, &pt, LW_TRUE);
		}
	} else {
		for (size_t i = offset; i < end; i++) {
			double x = flatbuffers_double_vec_at(xy, (i * 2));
			double y = flatbuffers_double_vec_at(xy, (i * 2) + 1);
			pt = (POINT4D) { x, y, 0, 0 };
			ptarray_append_point(pa, &pt, LW_TRUE);
		}
	}

	return pa;
}

static Datum decode_line(struct flatgeobuf_decode_ctx *ctx, Geometry_table_t geometry)
{
	POINTARRAY *pa = decode_line_pa(ctx, geometry, 0, 0);
	LWGEOM *lwgeom = (LWGEOM *) lwline_construct(0, NULL, pa);
	return (Datum) geometry_serialize(lwgeom);
}

static Datum decode_poly(struct flatgeobuf_decode_ctx *ctx, Geometry_table_t geometry)
{
	LWGEOM *lwgeom;
	POINTARRAY **ppa;

	flatbuffers_uint32_vec_t ends = Geometry_ends(geometry);

	size_t nrings = 1;
	uint32 end = 0;
	uint32 offset = 0;
	
	if (ends != NULL)
		nrings = flatbuffers_uint32_vec_len(ends);
	
	ppa = lwalloc(sizeof(POINTARRAY *) * nrings);
	for (size_t i = 0; i < nrings; i++) {
		if (ends != NULL)
			end = flatbuffers_uint32_vec_at(ends, i);
		ppa[i] = decode_line_pa(ctx, geometry, end, offset);
		offset = end;
	}

	lwgeom = (LWGEOM *) lwpoly_construct(0, NULL, nrings, ppa);
	return (Datum) geometry_serialize(lwgeom);
}

static Datum decode_mpoint(struct flatgeobuf_decode_ctx *ctx, Geometry_table_t geometry)
{
	POINTARRAY *pa = decode_line_pa(ctx, geometry, 0, 0);
	LWGEOM *lwgeom = (LWGEOM *) lwmpoint_construct(0, pa);
	return (Datum) geometry_serialize(lwgeom);
}

static Datum decode_geometry(struct flatgeobuf_decode_ctx *ctx, Geometry_table_t geometry)
{
	switch (ctx->geometry_type)
	{
		case GeometryType_Point:
			return decode_point(ctx, geometry);
		case GeometryType_LineString:
			return decode_line(ctx, geometry);
		case GeometryType_Polygon:
			return decode_poly(ctx, geometry);
		case GeometryType_MultiPoint:
			return decode_mpoint(ctx, geometry);
		default:
			elog(ERROR, "Unknown geometry type");
	}
	return 0;
}

void flatgeobuf_decode_feature(struct flatgeobuf_decode_ctx *ctx)
{
	size_t size;
	Feature_table_t feature;

	GSERIALIZED *geom;
	LWGEOM *lwgeom;
	HeapTuple heapTuple;
	POINTARRAY *pa;
	POINT4D pt = { 0, 0, 0, 0 };
	Geometry_table_t geometry;
	
	Datum *values = palloc(ctx->tupdesc->natts * sizeof(Datum *));
	bool *isnull = palloc(ctx->tupdesc->natts * sizeof(bool *));

	values[0] = Int32GetDatum(ctx->fid);
	isnull[0] = false;
	
	flatbuffers_read_size_prefix(ctx->buf + ctx->offset, &size);
	ctx->offset += sizeof(flatbuffers_uoffset_t);

	feature = Feature_as_root(ctx->buf + ctx->offset);
	ctx->offset += size;

	geometry = Feature_geometry(feature);
	if (geometry != NULL)
		values[1] = (Datum) decode_geometry(ctx, geometry);
	else
		isnull[1] = true;

	heapTuple = heap_form_tuple(ctx->tupdesc, values, isnull);
	ctx->result = HeapTupleGetDatum(heapTuple);
	ctx->fid++;

	ctx->done = true;
}

/**
 * Initialize aggregation context.
 */
struct flatgeobuf_encode_ctx *flatgeobuf_agg_init_context(const char *geom_name)
{
	struct flatgeobuf_encode_ctx *ctx;
	size_t size = VARHDRSZ + sizeof(magicbytes);
	ctx = palloc(sizeof(*ctx));
	ctx->buf = palloc(size);
	memcpy(ctx->buf + VARHDRSZ, magicbytes, sizeof(magicbytes));
	ctx->geom_name = geom_name;
	ctx->geom_index = 0;
	ctx->features_count = 0;
	ctx->offset = size;
	ctx->tupdesc = NULL;
	return ctx;
}

/**
 * Aggregation step.
 *
 * Expands features array if needed by a factor of 2.
 * Allocates a new feature, increment feature counter and
 * encode properties into it.
 */
void flatgeobuf_agg_transfn(struct flatgeobuf_encode_ctx *ctx)
{
	size_t size;
	flatcc_builder_t builder, *B;
	uint8_t *feature;

	LWGEOM *lwgeom = NULL;
	bool isnull = false;
	Datum datum;
	GSERIALIZED *gs;

	datum = GetAttributeByNum(ctx->row, ctx->geom_index + 1, &isnull);
	if (!isnull) {
		gs = (GSERIALIZED *) PG_DETOAST_DATUM_COPY(datum);
		lwgeom = lwgeom_from_gserialized(gs);
	}

	ctx->lwgeom = lwgeom;

	if (ctx->features_count == 0)
		encode_header(ctx);

	encode_feature(ctx);
}

/**
 * Finalize aggregation.
 *
 * Encode into Data message and return it packed as a bytea.
 */
uint8_t *flatgeobuf_agg_finalfn(struct flatgeobuf_encode_ctx *ctx)
{
	if (ctx == NULL)
		flatgeobuf_agg_init_context(NULL);
	if (ctx->features_count == 0)
		encode_header(ctx);
	SET_VARSIZE(ctx->buf, ctx->offset);
	return ctx->buf;
}
