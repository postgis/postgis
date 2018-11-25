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
 * Copyright (C) 2018 Regina Obe <lr@pcorp.us>
 *
 **********************************************************************/
/******************************************************************************
 * This file is to hold functions we no longer use,
 * but we need to keep because they were used at one time behind SQL API functions.
 * This is to ease pg_upgrade upgrades
 *
 * All functions in this file should throw an error telling the user to upgrade
 * the install
 *
 *****************************************************************************/

#include "postgres.h"
#include "utils/builtins.h"
#include "../postgis_config.h"
#include "lwgeom_pg.h"

/** pgis_abs_in: Removed PostGIS 2.5.0 **/
Datum pgis_abs_in(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(pgis_abs_in);

Datum
pgis_abs_in(PG_FUNCTION_ARGS)
{
	ereport(ERROR,(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
	               errmsg("function %s is out of date. Run: ALTER EXTENSION postgis UPDATE;", __func__)));
	PG_RETURN_POINTER(NULL);
}

/** pgis_abs_out: Removed PostGIS 2.5.0 **/
Datum pgis_abs_out(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(pgis_abs_out);
Datum
pgis_abs_out(PG_FUNCTION_ARGS)
{
	ereport(ERROR,(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
	               errmsg("function %s is out of date. Run: ALTER EXTENSION postgis UPDATE;", __func__)));
	PG_RETURN_POINTER(NULL);
}

