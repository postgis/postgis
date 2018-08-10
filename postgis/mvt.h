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

#ifndef MVT_H_
#define MVT_H_ 1

#include <stdlib.h>
#include "postgres.h"
#include "utils/builtins.h"
#include "utils/array.h"
#include "utils/typcache.h"
#include "utils/lsyscache.h"
#include "catalog/pg_type.h"
#include "catalog/namespace.h"
#include "executor/executor.h"
#include "access/htup_details.h"
#include "access/htup.h"
#include "../postgis_config.h"
#include "liblwgeom.h"
#include "liblwgeom_internal.h"
#include "lwgeom_pg.h"
#include "lwgeom_log.h"

#ifdef HAVE_LIBPROTOBUF

#include "vector_tile.pb-c.h"

typedef struct mvt_column_cache
{
    uint32_t *column_keys_index;
    uint32_t *column_oid;
    Datum *values;
    bool *nulls;
    TupleDesc tupdesc;
} mvt_column_cache;

typedef struct mvt_agg_context
{
	char *name;
	uint32_t extent;
	char *geom_name;
	uint32_t geom_index;
	HeapTupleHeader row;
	VectorTile__Tile__Feature *feature;
	VectorTile__Tile__Layer *layer;
	VectorTile__Tile *tile;
	size_t features_capacity;
	struct mvt_kv_key *keys_hash;
	struct mvt_kv_string_value *string_values_hash;
	struct mvt_kv_float_value *float_values_hash;
	struct mvt_kv_double_value *double_values_hash;
	struct mvt_kv_uint_value *uint_values_hash;
	struct mvt_kv_sint_value *sint_values_hash;
	struct mvt_kv_bool_value *bool_values_hash;
	uint32_t values_hash_i;
	uint32_t keys_hash_i;
	uint32_t row_columns;
	mvt_column_cache column_cache;
} mvt_agg_context;

/* Prototypes */
LWGEOM *mvt_geom(LWGEOM *geom, const GBOX *bounds, uint32_t extent, uint32_t buffer, bool clip_geom);
void mvt_agg_init_context(mvt_agg_context *ctx);
void mvt_agg_transfn(mvt_agg_context *ctx);
bytea *mvt_agg_finalfn(mvt_agg_context *ctx);
bytea *mvt_ctx_serialize(mvt_agg_context *ctx);
mvt_agg_context * mvt_ctx_deserialize(const bytea *ba);
mvt_agg_context * mvt_ctx_combine(mvt_agg_context *ctx1, mvt_agg_context *ctx2);


#endif  /* HAVE_LIBPROTOBUF */

#endif
