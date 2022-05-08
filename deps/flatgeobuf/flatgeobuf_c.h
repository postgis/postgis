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
 * Copyright 2021 Björn Harrtell
 *
 **********************************************************************/

#ifndef FLATGEOBUF_C_H
#define FLATGEOBUF_C_H

#ifdef __cplusplus
extern "C" {
#endif

#include "liblwgeom.h"
#include "lwgeom_log.h"

extern uint8_t flatgeobuf_magicbytes[];
extern uint8_t FLATGEOBUF_MAGICBYTES_SIZE;

// need c compatible variant of this enum
#define flatgeobuf_column_type_byte UINT8_C(0)
#define flatgeobuf_column_type_ubyte UINT8_C(1)
#define flatgeobuf_column_type_bool UINT8_C(2)
#define flatgeobuf_column_type_short UINT8_C(3)
#define flatgeobuf_column_type_ushort UINT8_C(4)
#define flatgeobuf_column_type_int UINT8_C(5)
#define flatgeobuf_column_type_uint UINT8_C(6)
#define flatgeobuf_column_type_long UINT8_C(7)
#define flatgeobuf_column_type_ulong UINT8_C(8)
#define flatgeobuf_column_type_float UINT8_C(9)
#define flatgeobuf_column_type_double UINT8_C(10)
#define flatgeobuf_column_type_string UINT8_C(11)
#define flatgeobuf_column_type_json UINT8_C(12)
#define flatgeobuf_column_type_datetime UINT8_C(13)
#define flatgeobuf_column_type_binary UINT8_C(14)

typedef struct flatgeobuf_column
{
	const char *name;
	uint8_t type;
	const char *title;
	const char *description;
	uint32_t width;
	uint32_t precision;
	uint32_t scale;
	bool nullable;
	bool unique;
	bool primary_key;
	const char * metadata;
} flatgeobuf_column;

typedef struct flatgeobuf_item
{
	double xmin;
	double xmax;
	double ymin;
	double ymax;
	uint32_t size;
	uint64_t offset;
} flatgeobuf_item;

typedef struct flatgeobuf_ctx
{
	// header contents
	const char *name;
	uint64_t features_count;
	uint8_t geometry_type;
	bool has_extent;
	double xmin;
	double xmax;
	double ymin;
	double ymax;
	bool has_z;
	bool has_m;
	bool has_t;
	bool has_tm;
	uint16_t index_node_size;
	int32_t srid;
	flatgeobuf_column **columns;
	uint16_t columns_size;

	// encode/decode buffers
	uint8_t *buf;
	uint64_t offset;
	uint64_t size;

	LWGEOM *lwgeom;
	uint8_t lwgeom_type;
	uint8_t *properties;
	uint32_t properties_len;
	uint32_t properties_size;

	// encode input
	const char *geom_name;
	uint32_t geom_index;

	// encode spatial index bookkeeping
	bool create_index;
	flatgeobuf_item **items;
	uint64_t items_len;
} flatgeobuf_ctx;

int flatgeobuf_encode_header(flatgeobuf_ctx *ctx);
int flatgeobuf_encode_feature(flatgeobuf_ctx *ctx);
void flatgeobuf_create_index(flatgeobuf_ctx *ctx);

int flatgeobuf_decode_header(flatgeobuf_ctx *ctx);
int flatgeobuf_decode_feature(flatgeobuf_ctx *ctx);

#ifdef __cplusplus
}
#endif

#endif /* FLATGEOBUF_C_H */