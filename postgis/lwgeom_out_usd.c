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
 * Copyright (C) 2023 ^Bo Zhou^
 *
 **********************************************************************/

#include "postgres.h"
#include "utils/builtins.h"

#include "../postgis_config.h"
#include "lwgeom_pg.h"
#include "lwgeom_log.h"
#include "liblwgeom.h"

#include "usd_c.h"

PG_FUNCTION_INFO_V1(ST_AsUSDA);

Datum
ST_AsUSDA(PG_FUNCTION_ARGS)
{
#ifndef HAVE_USD
	elog(ERROR, "ST_AsUSDA: Compiled without USD support");
	PG_RETURN_NULL();
#else
	GSERIALIZED *gs = NULL;
	int32_t srid = 0;
	LWGEOM *lwgeom = NULL;
	struct usd_write_context *ctx = NULL;
    size_t size = 0;
    void *data = NULL;
    text *result = NULL;

	gs = PG_GETARG_GSERIALIZED_P(0);
	srid = gserialized_get_srid(gs);

	postgis_initialize_cache();

	lwgeom = lwgeom_from_gserialized(gs);

	ctx = usd_write_create();
	usd_write_convert(ctx, lwgeom);
    usd_write_save(ctx, USDA);
    usd_write_data(ctx, &size, &data);

    result = cstring_to_text((char *)data);

	usd_write_destroy(ctx);

	PG_RETURN_TEXT_P(result);
#endif
}
