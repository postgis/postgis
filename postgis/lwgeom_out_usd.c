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
 * Copyright 2023 J CUBE Inc and Jadason Technology Ltd
 *
 **********************************************************************/

#include "postgres.h"
#include "utils/builtins.h"

#include "../postgis_config.h"
#include "lwgeom_pg.h"
#include "lwgeom_log.h"
#include "liblwgeom.h"

#include "usd_c.h"

PG_FUNCTION_INFO_V1(LWGEOM_to_USDA);

Datum
LWGEOM_to_USDA(PG_FUNCTION_ARGS)
{
#ifndef HAVE_USD
	elog(ERROR, "LWGEOM_to_USDA: Compiled without USD support");
	PG_RETURN_NULL();
#else
	GSERIALIZED *gs = NULL;
	LWGEOM *geom = NULL;
	struct usd_write_context *ctx = NULL;
	size_t size = 0;
	void *data = NULL;
	text *ret = NULL;

	gs = PG_GETARG_GSERIALIZED_P(0);
	geom = lwgeom_from_gserialized(gs);

	postgis_initialize_cache();

	ctx = usd_write_create();
	usd_write_convert(ctx, geom);
	usd_write_save(ctx, USDA);
	usd_write_data(ctx, &size, &data);

	ret = cstring_to_text_with_len((char *)data, (int)size);
	usd_write_destroy(ctx);

	PG_RETURN_TEXT_P(ret);
#endif
}

PG_FUNCTION_INFO_V1(LWGEOM_to_USDC);

Datum
LWGEOM_to_USDC(PG_FUNCTION_ARGS)
{
#ifndef HAVE_USD
	elog(ERROR, "LWGEOM_to_USDC: Compiled without USD support");
	PG_RETURN_NULL();
#else
	GSERIALIZED *gs = NULL;
	LWGEOM *geom = NULL;
	struct usd_write_context *ctx = NULL;
	size_t size = 0;
	void *data = NULL;
	bytea *ret = NULL;

	gs = PG_GETARG_GSERIALIZED_P(0);
	geom = lwgeom_from_gserialized(gs);

	postgis_initialize_cache();

	ctx = usd_write_create();
	usd_write_convert(ctx, geom);
	usd_write_save(ctx, USDC);
	usd_write_data(ctx, &size, &data);

	ret = (bytea *)palloc(VARHDRSZ + size);
	SET_VARSIZE(ret, VARHDRSZ + size);
	memcpy(VARDATA(ret), data, size);
	usd_write_destroy(ctx);

	PG_RETURN_BYTEA_P(ret);
#endif
}
