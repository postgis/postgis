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

#ifndef GEOBUF_H_
#define GEOBUF_H_ 1

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

#if defined HAVE_LIBPROTOBUF

#include "geobuf.pb-c.h"

struct geobuf_agg_context {
	char *geom_name;
	uint32_t geom_index;
	HeapTupleHeader row;
	LWGEOM **lwgeoms;
        Data *data;
        Data__Feature *feature;
	size_t features_capacity;
        uint32_t e;
        protobuf_c_boolean has_precision;
        uint32_t precision;
        protobuf_c_boolean has_dimensions;
        uint32_t dimensions;
};

void geobuf_agg_init_context(struct geobuf_agg_context *ctx);
void geobuf_agg_transfn(struct geobuf_agg_context *ctx);
uint8_t *geobuf_agg_finalfn(struct geobuf_agg_context *ctx);

#endif  /* HAVE_LIBPROTOBUF */

#endif
