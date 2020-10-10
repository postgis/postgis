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

uint8_t magicbytes[] = { 0x66, 0x67, 0x62, 0x03, 0x66, 0x67, 0x62, 0x00 };
size_t MAGICBYTES_LEN = (sizeof(magicbytes) / sizeof((magicbytes)[0]));

static void encode_header(struct flatgeobuf_encode_ctx *ctx)
{
	size_t size;
	uint8_t *header;
	flatcc_builder_t builder, *B;

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
	Header_geometry_type_add(B, GeometryType_Point);
	Header_hasZ_add(B, false);
	Header_hasM_add(B, false);
	Header_hasT_add(B, false);
	Header_hasTM_add(B, false);
	Header_end_as_root(B);

	header = flatcc_builder_finalize_buffer(B, &size);
	ctx->buf = repalloc(ctx->buf, ctx->offset + size);
	memcpy(ctx->buf + ctx->offset, header, size);
	free(header);
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

static void encode_geometry(struct flatgeobuf_encode_ctx *ctx)
{
	LWGEOM *lwgeom = ctx->lwgeom;

	if (lwgeom == NULL)
		return;

	switch (lwgeom->type) {
	case POINTTYPE:
		return encode_point(ctx, (LWPOINT *) lwgeom);
	/*case LINETYPE:
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

	if (ctx->hasZ) {
		flatbuffers_double_vec_t z = Geometry_z(geometry);
	} else {
		pt = (POINT4D) { x, y, 0, 0};
	}
	
	pa = ptarray_construct_empty(ctx->hasZ, ctx->hasM, 1);
	ptarray_append_point(pa, &pt, LW_TRUE);
	lwgeom = (LWGEOM *) lwpoint_construct(0, NULL, pa);
	return (Datum) geometry_serialize(lwgeom);
}

static Datum decode_geometry(struct flatgeobuf_decode_ctx *ctx, Geometry_table_t geometry)
{
	flatbuffers_double_vec_t xy = Geometry_xy(geometry);
	if (ctx->geometry_type == GeometryType_Point)
		return decode_point(ctx, geometry);
	elog(ERROR, "Unknown geometry type");
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
	
	Datum *values = palloc(ctx->tupdesc->natts * sizeof(Datum *));
	bool *isnull = palloc(ctx->tupdesc->natts * sizeof(bool *));

	values[0] = Int32GetDatum(ctx->fid);
	
	flatbuffers_read_size_prefix(ctx->buf + ctx->offset, &size);
	ctx->offset += sizeof(flatbuffers_uoffset_t);

	feature = Feature_as_root(ctx->buf + ctx->offset);
	ctx->offset += size;

	values[1] = (Datum) decode_geometry(ctx, Feature_geometry(feature));

	heapTuple = heap_form_tuple(ctx->tupdesc, values, isnull);
	ctx->result = HeapTupleGetDatum(heapTuple);
	ctx->fid++;

	ctx->done = true;
}

/**
 * Initialize aggregation context.
 */
void flatgeobuf_agg_init_context(struct flatgeobuf_encode_ctx *ctx)
{
	size_t size = VARHDRSZ + sizeof(magicbytes);
	ctx->buf = palloc(size);
	memcpy(ctx->buf + VARHDRSZ, magicbytes, sizeof(magicbytes));
	ctx->geom_index = 0;
	ctx->features_count = 0;
	ctx->offset = size;
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
	
	if (ctx->features_count == 0)
		encode_header(ctx);

	datum = GetAttributeByNum(ctx->row, ctx->geom_index + 1, &isnull);
	if (!isnull) {
		gs = (GSERIALIZED *) PG_DETOAST_DATUM_COPY(datum);
		lwgeom = lwgeom_from_gserialized(gs);
	}

	ctx->lwgeom = lwgeom;

	encode_feature(ctx);
}

/**
 * Finalize aggregation.
 *
 * Encode into Data message and return it packed as a bytea.
 */
uint8_t *flatgeobuf_agg_finalfn(struct flatgeobuf_encode_ctx *ctx)
{
	SET_VARSIZE(ctx->buf, ctx->offset);
	return ctx->buf;
}
