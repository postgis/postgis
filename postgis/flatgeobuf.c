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
#include "utils/jsonb.h"

uint8_t magicbytes[] = { 0x66, 0x67, 0x62, 0x03, 0x66, 0x67, 0x62, 0x00 };
uint8_t MAGICBYTES_LEN = (sizeof(magicbytes) / sizeof((magicbytes)[0]));

static GeometryType_enum_t get_geometrytype(LWGEOM *lwgeom) {
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
	case TINTYPE:
	case COLLECTIONTYPE:
		return GeometryType_GeometryCollection;
	default:
		elog(ERROR, "get_geometrytype: '%s' geometry type not supported",
				lwtype_name(type));
	}
	return GeometryType_Unknown;
}

static ColumnType_enum_t get_column_type(Oid typoid) {
	switch (typoid)
	{
	case BOOLOID:
		return ColumnType_Bool;
	case INT2OID:
		return ColumnType_Short;
	case INT4OID:
		return ColumnType_Int;
	case INT8OID:
		return ColumnType_Long;
	case FLOAT4OID:
		return ColumnType_Float;
	case FLOAT8OID:
		return ColumnType_Double;
	case TEXTOID:
	case VARCHAROID:
		return ColumnType_String;
	/*case JSONBOID:
		return ColumnType_Json;*/
	case BYTEAOID:
		return ColumnType_Binary;
	case DATEOID:
	case TIMEOID:
	case TIMESTAMPOID:
	case TIMESTAMPTZOID:
		return ColumnType_DateTime;
	}
	elog(ERROR, "get_column_type: '%d' column type not supported",
		typoid);
}

static void encode_header(struct flatgeobuf_encode_ctx *ctx)
{
	size_t size;
	uint8_t *header;
	flatcc_builder_t builder, *B;
	uint32_t columns_len = 0;
	Oid tupType = HeapTupleHeaderGetTypeId(ctx->row);
	int32 tupTypmod = HeapTupleHeaderGetTypMod(ctx->row);
	TupleDesc tupdesc = lookup_rowtype_tupdesc(tupType, tupTypmod);
	int natts = tupdesc->natts;
	bool geom_found = false;
	Column_ref_t *columns = lwalloc(sizeof(Column_ref_t) * natts);

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

	// inspect columns
	// NOTE: last element will be unused if geom attr is found
	for (int i = 0; i < natts; i++) {
		Oid typoid = getBaseType(TupleDescAttr(tupdesc, i)->atttypid);
		const char *key = TupleDescAttr(tupdesc, i)->attname.data;
		POSTGIS_DEBUGF(3, "flatgeobuf: inspecting column definition for %s with oid %d", key, typoid);
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
		POSTGIS_DEBUGF(2, "flatgeobuf: creating column definition for %s with oid %d", key, typoid);
		Column_start(B);
		Column_name_create_str(B, pstrdup(key));
		Column_type_add(B, get_column_type(typoid));
		columns[columns_len] = Column_end(B);
		columns_len++;
	}

	//flatbuffers_double_vec_ref_t envelope;
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
	Header_columns_create(B, columns, columns_len);
	Header_end_as_root(B);

	header = flatcc_builder_finalize_buffer(B, &size);
	POSTGIS_DEBUGF(3, "flatgeobuf: created header with size %ld", size);
	POSTGIS_DEBUGF(3, "flatgeobuf: reallocating buf to size %ld", ctx->offset + size);
	ctx->buf = repalloc(ctx->buf, ctx->offset + size);
	memcpy(ctx->buf + ctx->offset, header, size);
	free(header);

	ctx->offset += size;
}

static Geometry_ref_t encode_point(struct flatgeobuf_encode_ctx *ctx, LWPOINT *lwpt, bool with_type)
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
	if (with_type)
		Geometry_type_add(B, GeometryType_Point);
	return Geometry_end(B);
}

static void encode_line_pa(struct flatgeobuf_encode_ctx *ctx, POINTARRAY *pa)
{
	POINT4D pt;
	flatcc_builder_t *B = ctx->B;
	uint32_t i;

	Geometry_xy_start(B);
	for (i = 0; i < pa->npoints; i++) {
		getPoint4d_p(pa, i, &pt);
		Geometry_xy_push_create(B, pt.x);
		Geometry_xy_push_create(B, pt.y);
	}
	Geometry_xy_end(B);
	if (ctx->hasZ) {
		Geometry_z_start(B);
		for (i = 0; i < pa->npoints; i++) {
			getPoint4d_p(pa, i, &pt);
			Geometry_z_push_create(B, pt.z);
		}
		Geometry_z_end(B);
	}
	if (ctx->hasM) {
		Geometry_m_start(B);
		for (i = 0; i < pa->npoints; i++) {
			getPoint4d_p(pa, i, &pt);
			Geometry_m_push_create(B, pt.m);
		}
		Geometry_m_end(B);
	}
}

static void encode_line_ppa(struct flatgeobuf_encode_ctx *ctx, POINTARRAY **ppa, uint32_t len)
{
	POINT4D pt;
	flatcc_builder_t *B = ctx->B;
	uint32_t *ends = lwalloc(sizeof(uint32_t) * len);
	uint32_t offset = 0;
	uint32_t n, i;

	Geometry_xy_start(B);
	for (n = 0; n < len; n++) {
		POINTARRAY *pa = ppa[n];
		for (i = 0; i < pa->npoints; i++) {
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
		for (n = 0; n < len; n++) {
			POINTARRAY *pa = ppa[n];
			for (i = 0; i < pa->npoints; i++) {
				getPoint4d_p(pa, i, &pt);
				Geometry_z_push_create(B, pt.z);
			}
		}
		Geometry_z_end(B);
	}
	if (ctx->hasM) {
		Geometry_m_start(B);
		for (n = 0; n < len; n++) {
			POINTARRAY *pa = ppa[n];
			for (i = 0; i < pa->npoints; i++) {
				getPoint4d_p(pa, i, &pt);
				Geometry_m_push_create(B, pt.m);
			}
		}
		Geometry_m_end(B);
	}
}

static Geometry_ref_t encode_line(struct flatgeobuf_encode_ctx *ctx, LWLINE *lwline, bool with_type)
{
	flatcc_builder_t *B = ctx->B;

	Geometry_start(B);
	encode_line_pa(ctx, lwline->points);
	if (with_type)
		Geometry_type_add(B, GeometryType_LineString);
	return Geometry_end(B);
}

static Geometry_ref_t encode_triangle(struct flatgeobuf_encode_ctx *ctx, LWTRIANGLE *lwtriangle, bool with_type)
{
	flatcc_builder_t *B = ctx->B;

	Geometry_start(B);
	encode_line_pa(ctx, lwtriangle->points);
	if (with_type)
		Geometry_type_add(B, GeometryType_Triangle);
	return Geometry_end(B);
}

static Geometry_ref_t encode_poly(struct flatgeobuf_encode_ctx *ctx, LWPOLY *lwpoly, bool with_type)
{
	flatcc_builder_t *B = ctx->B;
	uint32_t nrings = lwpoly->nrings;
	POINTARRAY **ppa = lwpoly->rings;

	Geometry_start(B);
	if (nrings == 1)
		encode_line_pa(ctx, ppa[0]);
	else
		encode_line_ppa(ctx, ppa, nrings);
	if (with_type)
		Geometry_type_add(B, GeometryType_Polygon);
	return Geometry_end(B);
}

static Geometry_ref_t encode_mpoint(struct flatgeobuf_encode_ctx *ctx, LWMPOINT *lwmpoint, bool with_type)
{
	flatcc_builder_t *B = ctx->B;
	LWLINE *lwline = lwline_from_lwmpoint(0, lwmpoint);

	Geometry_start(B);
	encode_line_pa(ctx, lwline->points);
	if (with_type)
		Geometry_type_add(B, GeometryType_MultiPoint);
	return Geometry_end(B);
}

static Geometry_ref_t encode_mline(struct flatgeobuf_encode_ctx *ctx, LWMLINE *lwmline, bool with_type)
{
	flatcc_builder_t *B = ctx->B;
	uint32_t ngeoms = lwmline->ngeoms;
	POINTARRAY **ppa;
	uint32_t i;

	Geometry_start(B);
	if (ngeoms == 1) {
		encode_line_pa(ctx, lwmline->geoms[0]->points);
	} else {
		ppa = lwalloc(sizeof(POINTARRAY *) * ngeoms);
		for (i = 0; i < ngeoms; i++)
			ppa[i] = lwmline->geoms[i]->points;
		encode_line_ppa(ctx, ppa, ngeoms);
	}
	if (with_type)
		Geometry_type_add(B, GeometryType_MultiLineString);
	return Geometry_end(B);
}

static Geometry_ref_t encode_mpoly(struct flatgeobuf_encode_ctx *ctx, LWMPOLY *lwmpoly, bool with_type)
{
	flatcc_builder_t *B = ctx->B;
	uint32_t ngeoms = lwmpoly->ngeoms;
	Geometry_ref_t *parts = lwalloc(sizeof(Geometry_ref_t) * ngeoms);
	uint32_t i;

	for (i = 0; i < ngeoms; i++)
		parts[i] = encode_poly(ctx, lwmpoly->geoms[i], false);
	Geometry_start(B);
	Geometry_parts_create(B, parts, ngeoms);
	if (with_type)
		Geometry_type_add(B, GeometryType_MultiPolygon);
	return Geometry_end(B);
}

static Geometry_ref_t encode_geometry_part(struct flatgeobuf_encode_ctx *ctx, LWGEOM *lwgeom, bool with_type);

static Geometry_ref_t encode_collection(struct flatgeobuf_encode_ctx *ctx, LWCOLLECTION *lwcollection, bool with_type)
{
	flatcc_builder_t *B = ctx->B;
	uint32_t ngeoms = lwcollection->ngeoms;
	Geometry_ref_t *parts = lwalloc(sizeof(Geometry_ref_t) * ngeoms);
	uint32_t i;

	for (i = 0; i < ngeoms; i++)
		parts[i] = encode_geometry_part(ctx, lwcollection->geoms[i], true);
	Geometry_start(B);
	Geometry_parts_create(B, parts, ngeoms);
	if (with_type)
		Geometry_type_add(B, GeometryType_GeometryCollection);
	return Geometry_end(B);
}

static Geometry_ref_t encode_geometry_part(struct flatgeobuf_encode_ctx *ctx, LWGEOM *lwgeom, bool with_type)
{
	if (lwgeom == NULL)
		return 0;

	switch (lwgeom->type) {
	case POINTTYPE:
		return encode_point(ctx, (LWPOINT *) lwgeom, with_type);
	case LINETYPE:
		return encode_line(ctx, (LWLINE *) lwgeom, with_type);
	case TRIANGLETYPE:
		return encode_triangle(ctx, (LWTRIANGLE *) lwgeom, with_type);
	case POLYGONTYPE:
		return encode_poly(ctx, (LWPOLY *) lwgeom, with_type);
	case MULTIPOINTTYPE:
		return encode_mpoint(ctx, (LWMPOINT *) lwgeom, with_type);
	case MULTILINETYPE:
		return encode_mline(ctx, (LWMLINE *) lwgeom, with_type);
	case MULTIPOLYGONTYPE:
		return encode_mpoly(ctx, (LWMPOLY *) lwgeom, with_type);
	case COLLECTIONTYPE:
	case TINTYPE:
		return encode_collection(ctx, (LWCOLLECTION *) lwgeom, with_type);
	default:
		elog(ERROR, "encode_geometry: '%s' geometry type not supported",
				lwtype_name(lwgeom->type));
	}
}

static Geometry_ref_t encode_geometry(struct flatgeobuf_encode_ctx *ctx)
{
	bool geometry_type_is_unknown = ctx->geometry_type == GeometryType_Unknown;

	if (!geometry_type_is_unknown && ctx->lwgeom->type != ctx->geometry_type)
		elog(ERROR, "encode_geometry: unexpected geometry type '%s'", lwtype_name(ctx->lwgeom->type));

	return encode_geometry_part(ctx, ctx->lwgeom, geometry_type_is_unknown);
}

static void ensure_tmp_buf_size(struct flatgeobuf_encode_ctx *ctx, size_t size)
{
	if (ctx->tmp_buf_size == 0) {
		ctx->tmp_buf_size = 1024 * 4;
		ctx->tmp_buf = palloc(sizeof(uint8_t) * ctx->tmp_buf_size);
	}
	if (ctx->tmp_buf_size < size) {
		ctx->tmp_buf_size = ctx->tmp_buf_size * 2;
		ctx->tmp_buf = repalloc(ctx->tmp_buf, sizeof(uint8_t) * ctx->tmp_buf_size);
		ensure_tmp_buf_size(ctx, size);
	}
}

static void encode_properties(struct flatgeobuf_encode_ctx *ctx)
{
	uint16_t ci = 0;
	uint32_t offset = 0;
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

	Jsonb *jb;

	for (i = 0; i < (uint32_t) ctx->tupdesc->natts; i++) {
		if (ctx->geom_index == i)
			continue;
		datum = GetAttributeByNum(ctx->row, i + 1, &isnull);
		if (isnull)
			continue;
		ensure_tmp_buf_size(ctx, offset + sizeof(ci));
		memcpy(ctx->tmp_buf + offset, &ci, sizeof(ci));
		offset += sizeof(ci);
		typoid = getBaseType(TupleDescAttr(ctx->tupdesc, i)->atttypid);
		switch (typoid) {
		case BOOLOID:
			byte_value = DatumGetBool(datum) ? 1 : 0;
			ensure_tmp_buf_size(ctx, offset + sizeof(byte_value));
			memcpy(ctx->tmp_buf + offset, &byte_value, sizeof(byte_value));
			offset += sizeof(byte_value);
			break;
		case INT2OID:
			short_value = DatumGetInt16(datum);
			ensure_tmp_buf_size(ctx, offset + sizeof(short_value));
			memcpy(ctx->tmp_buf + offset, &short_value, sizeof(short_value));
			offset += sizeof(short_value);
			break;
		case INT4OID:
			int_value = DatumGetInt32(datum);
			ensure_tmp_buf_size(ctx, offset + sizeof(int_value));
			memcpy(ctx->tmp_buf + offset, &int_value, sizeof(int_value));
			offset += sizeof(int_value);
			break;
		case INT8OID:
			long_value = DatumGetInt64(datum);
			ensure_tmp_buf_size(ctx, offset + sizeof(long_value));
			memcpy(ctx->tmp_buf + offset, &long_value, sizeof(long_value));
			offset += sizeof(long_value);
			break;
		case FLOAT4OID:
			float_value = DatumGetFloat4(datum);
			ensure_tmp_buf_size(ctx, offset + sizeof(float_value));
			memcpy(ctx->tmp_buf + offset, &float_value, sizeof(float_value));
			offset += sizeof(float_value);
			break;
		case FLOAT8OID:
			double_value = DatumGetFloat8(datum);
			ensure_tmp_buf_size(ctx, offset + sizeof(double_value));
			memcpy(ctx->tmp_buf + offset, &double_value, sizeof(double_value));
			offset += sizeof(double_value);
			break;
		case TEXTOID:
			string_value = text_to_cstring(DatumGetTextP(datum));
			len = strlen(string_value);
			ensure_tmp_buf_size(ctx, offset + sizeof(len));
			memcpy(ctx->tmp_buf + offset, &len, sizeof(len));
			offset += sizeof(len);
			ensure_tmp_buf_size(ctx, offset + len);
			memcpy(ctx->tmp_buf + offset, string_value, len);
			offset += len;
			break;
		// TODO: handle date/time types
		/*case JSONBOID:
			jb = DatumGetJsonbP(datum);
			string_value = JsonbToCString(NULL, &jb->root, VARSIZE(jb));
			len = strlen(string_value);
			memcpy(data + offset, &len, sizeof(len));
			offset += sizeof(len);
			memcpy(data + offset, string_value, len);
			offset += len;
			break;*/
		}
		ci++;
	}

	Feature_properties_create(ctx->B, ctx->tmp_buf, offset);
}

static void encode_feature(struct flatgeobuf_encode_ctx *ctx)
{
	size_t size;
	uint8_t *feature;
	flatcc_builder_t builder, *B;
	Geometry_ref_t geometry;

	B = &builder;
	flatcc_builder_init(B);
	ctx->B = B;

	Feature_start_as_root_with_size(B);
	geometry = encode_geometry(ctx);
	if (geometry != 0)
		Feature_geometry_add(B, encode_geometry(ctx));
	encode_properties(ctx);
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
	uint32_t i;

	for (i = 0; i < MAGICBYTES_LEN; i++)
		if (buf[i] != magicbytes[i])
			elog(ERROR, "Data is not FlatGeobuf");
	ctx->offset += MAGICBYTES_LEN;
}

void flatgeobuf_decode_header(struct flatgeobuf_decode_ctx *ctx)
{
	Header_table_t header;
	Column_vec_t columns;
	size_t size;

	POSTGIS_DEBUGF(3, "flatgeobuf: reading feature prefix at %ld", ctx->offset);
	flatbuffers_read_size_prefix(ctx->buf + ctx->offset, &size);
	POSTGIS_DEBUGF(3, "flatgeobuf: feature size is %ld", size);
	ctx->offset += sizeof(flatbuffers_uoffset_t);

	header = Header_as_root(ctx->buf + ctx->offset);
	ctx->offset += size;
	ctx->geometry_type = Header_geometry_type(header);
	ctx->hasZ = Header_hasZ(header);
	ctx->hasM = Header_hasM(header);
	ctx->hasT = Header_hasT(header);
	ctx->hasTM = Header_hasTM(header);
	ctx->columns = Header_columns_get(header);
	ctx->columns_len = Column_vec_len(ctx->columns);

	ctx->header = header;
}

static LWPOINT *decode_point(struct flatgeobuf_decode_ctx *ctx, Geometry_table_t geometry)
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
	return lwpoint_construct(0, NULL, pa);
}

static POINTARRAY *decode_line_pa(struct flatgeobuf_decode_ctx *ctx, Geometry_table_t geometry, uint32_t end, uint32_t offset)
{
	POINTARRAY *pa;
	LWGEOM *lwgeom;
	POINT4D pt;
	flatbuffers_double_vec_t xy = Geometry_xy(geometry);
	uint32_t npoints;
	size_t xy_len;
	uint32_t i;

	if (end != 0) {
		npoints = end - offset;
	} else {
		npoints = flatbuffers_double_vec_len(xy) / 2;
		end = npoints;
	}

	pa = ptarray_construct_empty(ctx->hasZ, ctx->hasM, npoints);

	if (ctx->hasZ && ctx->hasM) {
		for (i = offset; i < end; i++) {
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
		for (i = offset; i < end; i++) {
			double x = flatbuffers_double_vec_at(xy, (i * 2));
			double y = flatbuffers_double_vec_at(xy, (i * 2) + 1);
			flatbuffers_double_vec_t za = Geometry_z(geometry);
			double z = flatbuffers_double_vec_at(za, i);
			pt = (POINT4D) { x, y, z,0 };
			ptarray_append_point(pa, &pt, LW_TRUE);
		}
	} else if (ctx->hasM) {
		for (i = offset; i < end; i++) {
			double x = flatbuffers_double_vec_at(xy, (i * 2));
			double y = flatbuffers_double_vec_at(xy, (i * 2) + 1);
			flatbuffers_double_vec_t ma = Geometry_m(geometry);
			double m = flatbuffers_double_vec_at(ma, i);
			pt = (POINT4D) { x, y, 0, m };
			ptarray_append_point(pa, &pt, LW_TRUE);
		}
	} else {
		for (i = offset; i < end; i++) {
			double x = flatbuffers_double_vec_at(xy, (i * 2));
			double y = flatbuffers_double_vec_at(xy, (i * 2) + 1);
			pt = (POINT4D) { x, y, 0, 0 };
			ptarray_append_point(pa, &pt, LW_TRUE);
		}
	}

	return pa;
}

static LWLINE *decode_line(struct flatgeobuf_decode_ctx *ctx, Geometry_table_t geometry)
{
	POINTARRAY *pa = decode_line_pa(ctx, geometry, 0, 0);
	return lwline_construct(0, NULL, pa);
}

static LWPOLY *decode_poly(struct flatgeobuf_decode_ctx *ctx, Geometry_table_t geometry)
{
	POINTARRAY **ppa;

	flatbuffers_uint32_vec_t ends = Geometry_ends(geometry);

	uint32_t nrings = 1;
	uint32_t end = 0;
	uint32_t offset = 0;
	uint32_t i;

	if (ends != NULL)
		nrings = flatbuffers_uint32_vec_len(ends);

	ppa = lwalloc(sizeof(POINTARRAY *) * nrings);
	for (i = 0; i < nrings; i++) {
		if (ends != NULL)
			end = flatbuffers_uint32_vec_at(ends, i);
		ppa[i] = decode_line_pa(ctx, geometry, end, offset);
		offset = end;
	}

	return lwpoly_construct(0, NULL, nrings, ppa);
}

static LWMPOINT *decode_mpoint(struct flatgeobuf_decode_ctx *ctx, Geometry_table_t geometry)
{
	POINTARRAY *pa = decode_line_pa(ctx, geometry, 0, 0);
	return lwmpoint_construct(0, pa);
}

static LWMLINE *decode_mline(struct flatgeobuf_decode_ctx *ctx, Geometry_table_t geometry)
{
	uint32_t ngeoms = 1;
	flatbuffers_uint32_vec_t ends = Geometry_ends(geometry);
	LWMLINE *lwmline = lwmline_construct_empty(0, ctx->hasZ, ctx->hasM);
	uint32_t i;

	if (ends != NULL)
		ngeoms = flatbuffers_uint32_vec_len(ends);

	if (ngeoms == 1) {
		POINTARRAY *pa = decode_line_pa(ctx, geometry, 0, 0);
		LWLINE *lwline = lwline_construct(0, NULL, pa);
		lwmline_add_lwline(lwmline, lwline);
	} else {
		for (i = 0; i < ngeoms; i++) {
			uint32 end = flatbuffers_uint32_vec_at(ends, i);
			uint32 offset = 0;
			POINTARRAY *pa = decode_line_pa(ctx, geometry, end, offset);
			LWLINE *lwline = lwline_construct(0, NULL, pa);
			lwmline_add_lwline(lwmline, lwline);
			offset = end;
		}
	}

	return lwmline;
}

static LWMPOLY *decode_mpoly(struct flatgeobuf_decode_ctx *ctx, Geometry_table_t geometry)
{
	Geometry_vec_t parts = Geometry_parts(geometry);
	uint32_t ngeoms = Geometry_vec_len(parts);
	LWMPOLY *lwmpoly = lwmpoly_construct_empty(0, ctx->hasZ, ctx->hasM);
	uint32_t i;

	for (i = 0; i < ngeoms; i++)
		lwmpoly_add_lwpoly(lwmpoly, decode_poly(ctx, Geometry_vec_at(parts, i)));
	return lwmpoly;
}

static LWGEOM *decode_geometry_part(struct flatgeobuf_decode_ctx *ctx, Geometry_table_t geometry, GeometryType_enum_t geometry_type);

static LWCOLLECTION *decode_collection(struct flatgeobuf_decode_ctx *ctx, Geometry_table_t geometry)
{
	Geometry_vec_t parts = Geometry_parts(geometry);
	uint32_t ngeoms = Geometry_vec_len(parts);
	LWCOLLECTION *lwcollection = lwcollection_construct_empty(COLLECTIONTYPE, 0, ctx->hasZ, ctx->hasM);
	uint32_t i;

	for (i = 0; i < ngeoms; i++) {
		Geometry_table_t part = Geometry_vec_at(parts, i);
		GeometryType_enum_t geometry_type = Geometry_type_get(part);
		lwcollection_add_lwgeom(lwcollection, decode_geometry_part(ctx, part, geometry_type));
	}
	return lwcollection;
}

static LWGEOM *decode_geometry_part(struct flatgeobuf_decode_ctx *ctx, Geometry_table_t geometry, GeometryType_enum_t geometry_type)
{
	LWGEOM *lwgeom;
	switch (geometry_type) {
	case GeometryType_Point:
		return (LWGEOM *) decode_point(ctx, geometry);
	case GeometryType_LineString:
		return (LWGEOM *) decode_line(ctx, geometry);
	case GeometryType_Polygon:
		return (LWGEOM *) decode_poly(ctx, geometry);
	case GeometryType_MultiPoint:
		return (LWGEOM *) decode_mpoint(ctx, geometry);
	case GeometryType_MultiLineString:
		return (LWGEOM *) decode_mline(ctx, geometry);
	case GeometryType_MultiPolygon:
		return (LWGEOM *) decode_mpoly(ctx, geometry);
	case GeometryType_GeometryCollection:
		return (LWGEOM *) decode_collection(ctx, geometry);
	default:
		elog(ERROR, "decode_geometry: Unknown geometry type %d", geometry_type);
	}
	return NULL;
}

static Datum decode_geometry(struct flatgeobuf_decode_ctx *ctx, Geometry_table_t geometry)
{
	LWGEOM *lwgeom = decode_geometry_part(ctx, geometry, ctx->geometry_type);
	return (Datum) geometry_serialize(lwgeom);
}

static void decode_properties(struct flatgeobuf_decode_ctx *ctx, Feature_table_t feature, Datum *values, bool *isnull)
{
	uint32_t natts = ctx->tupdesc->natts;
	uint16_t i, ci;
	uint32_t len, natti = 2;
	flatbuffers_uoffset_t offset = 0;
	flatbuffers_uint8_vec_t data = Feature_properties(feature);
	size_t size = flatbuffers_uint8_vec_len(data);
	Column_table_t column;
	ColumnType_enum_t type;

	// TODO: init isnull

	if (size > 0 && size < (sizeof(uint16_t) + sizeof(uint8_t)))
		elog(ERROR, "flatgeobuf: decode_properties: Unexpected properties data size %ld", size);
	while (offset + 1 < size) {
		if (offset + sizeof(uint16_t) > size)
			elog(ERROR, "flatgeobuf: decode_properties: Unexpected offset %d", offset);
		i = *((uint16_t *)(data + offset));
		ci = i + 2;
		offset += sizeof(uint16_t);
		if (i >= ctx->columns_len)
			elog(ERROR, "flatgeobuf: decode_properties: Column index %hu out of range", i);
		column = Column_vec_at(ctx->columns, i);
		type = Column_type(column);
		isnull[ci] = false;
		switch (type) {
		case ColumnType_Bool:
			if (offset + sizeof(uint8_t) > size)
				elog(ERROR, "flatgeobuf: decode_properties: Invalid size for bool value");
			values[ci] = BoolGetDatum(*((uint8_t *)(data + offset)));
			offset += sizeof(uint8_t);
			break;
		case ColumnType_Byte:
			if (offset + sizeof(int8_t) > size)
				elog(ERROR, "flatgeobuf: decode_properties: Invalid size for ubyte value");
			values[ci] = Int8GetDatum(*((int8_t *)(data + offset)));
			offset += sizeof(int8_t);
			break;
		case ColumnType_UByte:
			if (offset + sizeof(uint8_t) > size)
				elog(ERROR, "flatgeobuf: decode_properties: Invalid size for ubyte value");
			values[ci] = UInt8GetDatum(*((uint8_t *)(data + offset)));
			offset += sizeof(uint8_t);
			break;
		case ColumnType_Short:
			if (offset + sizeof(int16_t) > size)
				elog(ERROR, "flatgeobuf: decode_properties: Invalid size for short value");
			values[ci] = Int16GetDatum(*((int16_t *)(data + offset)));
			offset += sizeof(int16_t);
			break;
		case ColumnType_UShort:
			if (offset + sizeof(uint16_t) > size)
				elog(ERROR, "flatgeobuf: decode_properties: Invalid size for ushort value");
			values[ci] = UInt16GetDatum(*((uint16_t *)(data + offset)));
			offset += sizeof(uint16_t);
			break;
		case ColumnType_Int:
			if (offset + sizeof(int32_t) > size)
				elog(ERROR, "flatgeobuf: decode_properties: Invalid size for int value");
			values[ci] = Int64GetDatum(*((int32_t *)(data + offset)));
			offset += sizeof(int32_t);
			break;
		case ColumnType_UInt:
			if (offset + sizeof(uint32_t) > size)
				elog(ERROR, "flatgeobuf: decode_properties: Invalid size for uint value");
			values[ci] = UInt32GetDatum(*((uint32_t *)(data + offset)));
			offset += sizeof(uint32_t);
			break;
		case ColumnType_Long:
			if (offset + sizeof(int64_t) > size)
				elog(ERROR, "flatgeobuf: decode_properties: Invalid size for long value");
			values[ci] = Int64GetDatum(*((int64_t *)(data + offset)));
			offset += sizeof(int64_t);
			break;
		case ColumnType_ULong:
			if (offset + sizeof(uint64_t) > size)
				elog(ERROR, "flatgeobuf: decode_properties: Invalid size for ulong value");
			values[ci] = UInt64GetDatum(*((uint64_t *)(data + offset)));
			offset += sizeof(uint64_t);
			break;
		case ColumnType_Float:
			if (offset + sizeof(float) > size)
				elog(ERROR, "flatgeobuf: decode_properties: Invalid size for float value");
			values[ci] = Float4GetDatum(*((float *)(data + offset)));
			offset += sizeof(float);
			break;
		case ColumnType_Double:
			if (offset + sizeof(double) > size)
				elog(ERROR, "flatgeobuf: decode_properties: Invalid size for double value");
			values[ci] = Float8GetDatum(*((double *)(data + offset)));
			offset += sizeof(double);
			break;
		case ColumnType_String:
			if (offset + sizeof(len) > size)
				elog(ERROR, "flatgeobuf: decode_properties: Invalid size for string value");
			len = *((uint32_t *)(data + offset));
			offset += sizeof(len);
			values[ci] = PointerGetDatum(cstring_to_text_with_len((const char *) data + offset, len));
			offset += len;
			break;
		case ColumnType_DateTime:
			if (offset + sizeof(len) > size)
				elog(ERROR, "flatgeobuf: decode_properties: Invalid size for string value");
			len = *((uint32_t *)(data + offset));
			offset += sizeof(len);
			// TODO: find out how to parse an ISO datetime string into a Datum
			//values[ci] = PointerGetDatum(cstring_to_text_with_len((const char *) data + offset, len));
			offset += len;
			break;
		// TODO: find out how to handle string to jsonb Datum
		/*case ColumnType_Json:
			if (offset + sizeof(len) > size)
				elog(ERROR, "flatgeobuf: decode_properties: Invalid size for json value");
			len = *((uint32_t *)(data + offset));
			offset += sizeof(len);
			values[ci] = jsonb_from_cstring((const char *) data + offset, len);
			offset += len;
			break;*/
		default:
			elog(ERROR, "flatgeobuf: decode_properties: Unknown type %d", type);
		}
	}

}

void flatgeobuf_decode_feature(struct flatgeobuf_decode_ctx *ctx)
{
	size_t size;
	Feature_table_t feature;
	GSERIALIZED *geom;
	LWGEOM *lwgeom;
	HeapTuple heapTuple;
	uint32_t natts = ctx->tupdesc->natts;
	POINTARRAY *pa;
	POINT4D pt = { 0, 0, 0, 0 };
	Geometry_table_t geometry;
	Datum *values = palloc(natts * sizeof(Datum *));
	bool *isnull = palloc(natts * sizeof(bool *));

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

	if (natts > 2)
		decode_properties(ctx, feature, values, isnull);

	heapTuple = heap_form_tuple(ctx->tupdesc, values, isnull);
	ctx->result = HeapTupleGetDatum(heapTuple);
	ctx->fid++;

	if (ctx->offset == ctx->size)
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
	ctx->tmp_buf = NULL;
	ctx->tmp_buf_size = 0;
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
	if (ctx->tupdesc != NULL)
		ReleaseTupleDesc(ctx->tupdesc);
	SET_VARSIZE(ctx->buf, ctx->offset);
	return ctx->buf;
}
