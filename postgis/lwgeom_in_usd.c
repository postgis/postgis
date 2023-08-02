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
#include "liblwgeom.h"
#include "lwgeom_transform.h"

#include "usd_c.h"

PG_FUNCTION_INFO_V1(LWGEOM_from_USD);

Datum
LWGEOM_from_USD(PG_FUNCTION_ARGS)
{
#ifndef HAVE_USD
	elog(ERROR, "LWGEOM_from_USD: Compiled without USD support");
	PG_RETURN_NULL();
#else
	usd_format format = USDA;
	bytea* array = NULL;
	char *data = NULL;
	size_t size = 0;
	struct usd_read_context *ctx = NULL;
	struct usd_read_iterator *itr = NULL;
	LWCOLLECTION *coll = NULL;
	LWGEOM *geom = NULL;
	GSERIALIZED *ret = NULL;

	format = PG_GETARG_INT32(0);
	if (format == USDA)
	{
		data = PG_GETARG_CSTRING(1);
		size = strlen(data);
	}
	else if (format == USDC)
	{
		bytea* array = PG_GETARG_BYTEA_P(0);
		data = VARDATA(array);
		size = VARSIZE(array);
	}
	else
	{
		elog(ERROR, "LWGEOM_from_USD: Unknown format %d", format);
		PG_RETURN_NULL();
	}

	ctx = usd_read_create();
	usd_read_convert(ctx, data, size, format);

	coll = lwcollection_construct_empty(COLLECTIONTYPE, SRID_UNKNOWN, 0, 0);
	itr = usd_read_iterator_create(ctx);
	while ((geom = usd_read_iterator_next(itr)))
	{
		coll = lwcollection_add_lwgeom(coll, geom);
	}
	usd_read_iterator_destroy(itr);

	ret = geometry_serialize(lwcollection_as_lwgeom(coll));
	usd_read_destroy(ctx);
	
	PG_RETURN_POINTER(ret);
#endif
}
