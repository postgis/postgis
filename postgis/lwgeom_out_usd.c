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

PG_FUNCTION_INFO_V1(ST_AsUSDX);

Datum
ST_AsUSDX(PG_FUNCTION_ARGS)
{
#ifndef HAVE_USD
	elog(ERROR, "ST_AsUSDX: Compiled without USD support");
	PG_RETURN_NULL();
#else
	GSERIALIZED *gs = NULL;
	LWGEOM *lwgeom = NULL;
	int format = USDA;
	struct usd_write_context *ctx = NULL;
	size_t size = 0;
	void *data = NULL;
	text *result_text = NULL;
	bytea *result_bytea = NULL;

	gs = PG_GETARG_GSERIALIZED_P(0);
	format = PG_GETARG_INT32(1);

	if (format != USDA && format != USDC)
	{
		elog(ERROR, "ST_AsUSDX: Unknown USD format %d", format);
		PG_RETURN_NULL();
	}

	postgis_initialize_cache();

	lwgeom = lwgeom_from_gserialized(gs);

	ctx = usd_write_create();
	usd_write_convert(ctx, lwgeom);
	usd_write_save(ctx, format);
	usd_write_data(ctx, &size, &data);

	if (format == USDA)
	{
		result_text = cstring_to_text_with_len((char *)data, (int)size);
		usd_write_destroy(ctx);
		PG_RETURN_TEXT_P(result_text);
	}
	else if (format == USDC)
	{
		result_bytea = (bytea *)palloc(VARHDRSZ + size);
		SET_VARSIZE(result_bytea, VARHDRSZ + size);
		memcpy(VARDATA(result_bytea), data, size);
		PG_RETURN_BYTEA_P(result_bytea);
	}
	usd_write_destroy(ctx);
	PG_RETURN_NULL();
#endif
}
