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

#ifndef FLATGEOBUF_H_
#define FLATGEOBUF_H_ 1

#include <stdlib.h>
#include "postgres.h"
#include "utils/builtins.h"
#include "utils/array.h"
#include "utils/typcache.h"
#include "utils/lsyscache.h"
#include "catalog/pg_type.h"
#include "catalog/namespace.h"
#include "executor/spi.h"
#include "executor/executor.h"
#include "access/htup_details.h"
#include "access/htup.h"
#include "../postgis_config.h"
#include "liblwgeom.h"
#include "lwgeom_pg.h"
#include "lwgeom_log.h"
#include "header_builder.h"
#include "feature_builder.h"

struct flatgeobuf_encode_ctx
{
        flatcc_builder_t *B;

        char *geom_name;
        uint32_t geom_index;
        TupleDesc tupdesc;
        HeapTupleHeader row;
        LWGEOM *lwgeom;
        uint64_t features_count;
        uint8_t *buf;
        uint64_t offset;
        GeometryType_enum_t geometry_type;
        bool hasZ;
        bool hasM;
        bool hasT;
        bool hasTM;
};

void flatgeobuf_agg_init_context(struct flatgeobuf_encode_ctx *ctx);
void flatgeobuf_agg_transfn(struct flatgeobuf_encode_ctx *ctx);
uint8_t *flatgeobuf_agg_finalfn(struct flatgeobuf_encode_ctx *ctx);

struct flatgeobuf_decode_ctx
{
	TupleDesc tupdesc;
        uint8_t *buf;
        uint64_t size;
	uint64_t offset;
	Datum result;
        uint64_t fid;
	bool done;
        Header_table_t header;
        GeometryType_enum_t geometry_type;
        bool hasZ;
        bool hasM;
        bool hasT;
        bool hasTM;
        size_t columns_len;
        Datum geom;
};

void flatgeobuf_check_magicbytes(struct flatgeobuf_decode_ctx *ctx);
void flatgeobuf_decode_header(struct flatgeobuf_decode_ctx *ctx);
void flatgeobuf_decode_feature(struct flatgeobuf_decode_ctx *ctx);

#endif
