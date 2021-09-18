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
 * Copyright (C) 2021 Björn Harrtell <bjorn@wololo.org>
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
#include "flatgeobuf_c.h"

typedef struct flatgeobuf_agg_ctx
{
	flatgeobuf_ctx *ctx;
	const char *geom_name;
	uint32_t geom_index;
	TupleDesc tupdesc;
	HeapTupleHeader row;
} flatgeobuf_agg_ctx;


flatgeobuf_agg_ctx *flatgeobuf_agg_ctx_init(const char *geom_name, const bool create_index);
void flatgeobuf_agg_transfn(flatgeobuf_agg_ctx *ctx);
uint8_t *flatgeobuf_agg_finalfn(flatgeobuf_agg_ctx *ctx);

typedef struct flatgeobuf_decode_ctx
{
	flatgeobuf_ctx *ctx;
	TupleDesc tupdesc;
	Datum result;
	Datum geom;
	int fid;
	bool done;
} flatgeobuf_decode_ctx;

void flatgeobuf_check_magicbytes(struct flatgeobuf_decode_ctx *ctx);
void flatgeobuf_decode_row(struct flatgeobuf_decode_ctx *ctx);

#endif
