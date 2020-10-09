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
	/*flatbuffers_string_ref_t name = flatbuffers_string_create_str(B, "");
	flatbuffers_double_vec_ref_t envelope;
	GeometryType_enum_t geometry_type = GeometryType_Unknown;
	flatbuffers_bool_t hasZ;
	flatbuffers_bool_t hasM;
	flatbuffers_bool_t hasT;
	flatbuffers_bool_t hasTM;
	Column_vec_start(B);
	Column_vec_t columns = Column_vec_start(B);
	uint16_t index_node_size = 16;
	Crs_vec_start(B);
	Crs_vec_t crs = Crs_vec_end(B);
	flatbuffers_string_ref_t title = flatbuffers_string_create_str(B, "");
	flatbuffers_string_ref_t description = flatbuffers_string_create_str(B, "");
	flatbuffers_string_ref_t metadata = flatbuffers_string_create_str(B, "");*/
	Header_start_as_root_with_size(B);
	Header_end_as_root(B);

	header = flatcc_builder_finalize_buffer(B, &size);
	ctx->buf = repalloc(ctx->buf, ctx->offset + size);
	memcpy(ctx->buf + ctx->offset, header, size);
	free(header);
	ctx->offset += size;
}

static void encode_geometry(struct flatgeobuf_encode_ctx *ctx, flatcc_builder_t *B)
{	
	LWPOINT *lwpt = (LWPOINT *) ctx->lwgeom;
	POINT4D pt;
	getPoint4d_p(lwpt->point, 0, &pt);

	Geometry_ref_t geometry;
	Geometry_start(B);
	Geometry_xy_start(B);
	Geometry_xy_push_create(B, pt.x);
	Geometry_xy_push_create(B, pt.y);
	Geometry_xy_end(B);
	geometry = Geometry_end(B);
	Feature_geometry_add(B, geometry);
}

static void encode_feature(struct flatgeobuf_encode_ctx *ctx)
{
	size_t size;
	uint8_t *feature;
	flatcc_builder_t builder, *B;

	B = &builder;
	flatcc_builder_init(B);
	Feature_start_as_root_with_size(B);
	encode_geometry(ctx, B);
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
	Column_vec_t columns;
	size_t size;
	
	flatbuffers_read_size_prefix(ctx->buf + ctx->offset, &size);
	ctx->offset += sizeof(flatbuffers_uoffset_t);
	
	ctx->header = Header_as_root(ctx->buf + ctx->offset);
	ctx->offset += size;
	
	columns = Header_columns(ctx->header);
	
	ctx->geometry_type = Header_geometry_type(ctx->header);
	ctx->columns_len = Column_vec_len(columns);
	if (ctx->tupdesc->natts != ctx->columns_len + 2)
		elog(ERROR, "Mismatched column structure");
}

void flatgeobuf_decode_feature(struct flatgeobuf_decode_ctx *ctx)
{
	size_t size;

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

	Feature_table_t feature = Feature_as_root(ctx->buf + ctx->offset);
	ctx->offset += size;

	Geometry_table_t geometry = Feature_geometry(feature);
	flatbuffers_double_vec_t xy = Geometry_xy(geometry);
	size_t l = flatbuffers_double_vec_len(xy);
	double x = flatbuffers_double_vec_at(xy, 0);
	double y = flatbuffers_double_vec_at(xy, 1);
	pt.x = x;
	pt.y = y;

	pa = ptarray_construct_empty(0, 0, 1);
	ptarray_append_point(pa, &pt, LW_TRUE);
	lwgeom = (LWGEOM *) lwpoint_construct(0, NULL, pa);
	geom = geometry_serialize(lwgeom);
	values[1] = (Datum) geom;

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

	if (ctx->lwgeom == NULL)
		elog(ERROR, "ctx->lwgeom is null");
	
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
