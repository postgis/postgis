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

uint8_t magicbytes[] = { 0x66, 0x67, 0x62, 0x03, 0x66, 0x67, 0x62, 0x00 };

void encode_header(struct flatgeobuf_agg_context *ctx)
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

void encode_feature(struct flatgeobuf_agg_context *ctx)
{
	size_t size;
	uint8_t *feature;
	flatcc_builder_t builder, *B;

	B = &builder;
	flatcc_builder_init(B);
	Feature_start_as_root_with_size(B);
	// TODO: encode feature properties
	Feature_end_as_root(B);
	feature = flatcc_builder_finalize_buffer(B, &size);

	ctx->buf = repalloc(ctx->buf, ctx->offset + size);
	memcpy(ctx->buf, feature, size);
	free(feature);
	ctx->offset += size;
	ctx->features_count++;
}

/**
 * Initialize aggregation context.
 */
void flatgeobuf_agg_init_context(struct flatgeobuf_agg_context *ctx)
{
	size_t size = VARHDRSZ + sizeof(magicbytes);
	ctx->buf = palloc(size);
	memcpy(ctx->buf + VARHDRSZ, magicbytes, sizeof(magicbytes));
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
void flatgeobuf_agg_transfn(struct flatgeobuf_agg_context *ctx)
{
	size_t size;
	flatcc_builder_t builder, *B;
	uint8_t *feature;
	
	// TODO: if first feature, inspect and create header
	if (ctx->features_count == 0)
		encode_header(ctx);

	encode_feature(ctx);

	/*LWGEOM *lwgeom;
	bool isnull = false;
	Datum datum;
	GSERIALIZED *gs;
	*/

	/* inspect row and encode keys assuming static schema */
	//if (fc->n_features == 0)
	//	encode_keys(ctx);

	// TODO: move to encode geometry
	/*datum = GetAttributeByNum(ctx->row, ctx->geom_index + 1, &isnull);
	if (isnull)
		return;

	gs = (GSERIALIZED *) PG_DETOAST_DATUM_COPY(datum);
	lwgeom = lwgeom_from_gserialized(gs);*/
}

/**
 * Finalize aggregation.
 *
 * Encode into Data message and return it packed as a bytea.
 */
uint8_t *flatgeobuf_agg_finalfn(struct flatgeobuf_agg_context *ctx)
{
	SET_VARSIZE(ctx->buf, ctx->offset);
	return ctx->buf;
}
