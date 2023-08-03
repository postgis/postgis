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

PG_FUNCTION_INFO_V1(LWGEOM_to_USD);

Datum
LWGEOM_to_USD(PG_FUNCTION_ARGS)
{
#ifndef HAVE_USD
	elog(ERROR, "LWGEOM_to_USD: Compiled without USD support");
	PG_RETURN_NULL();
#else
	usd_format format = USDA;
	GSERIALIZED *gs = NULL;
	LWGEOM *geom = NULL;
	text *root_name = NULL;
	char *root_name_cstr = NULL;
	text *geom_name = NULL;
	char *geom_name_cstr = NULL;
	struct usd_write_context *ctx = NULL;
	size_t size = 0;
	void *data = NULL;
	bytea *ret = NULL;

	format = PG_GETARG_INT32(0);
	if (format != USDA && format != USDC)
	{
		elog(ERROR, "LWGEOM_to_USD: Unknown format %d", format);
		PG_RETURN_NULL();
	}

	gs = PG_GETARG_GSERIALIZED_P(1);
	geom = lwgeom_from_gserialized(gs);

	root_name = PG_GETARG_TEXT_P(2);
	root_name_cstr = text_to_cstring(root_name);
	if (!root_name_cstr || !strlen(root_name_cstr))
	{
		root_name_cstr = USD_ROOT_NAME;
	}

	geom_name = PG_GETARG_TEXT_P(3);
	geom_name_cstr = text_to_cstring(geom_name);
	if (!geom_name_cstr || !strlen(geom_name_cstr))
	{
		geom_name_cstr = USD_GEOM_NAME;
	}

	postgis_initialize_cache();

	ctx = usd_write_create(format, root_name_cstr, geom_name_cstr);
	usd_write_convert(ctx, geom);
	usd_write_save(ctx);
	usd_write_data(ctx, &size, &data);

	ret = (bytea *)palloc(VARHDRSZ + size);
	SET_VARSIZE(ret, VARHDRSZ + size);
	memcpy(VARDATA(ret), data, size);
	usd_write_destroy(ctx);

	PG_RETURN_BYTEA_P(ret);
#endif
}
