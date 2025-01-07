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
 * Copyright (C) 2025 Regina Obe <lr@pcorp.us>
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

#define POSTGIS_DEPRECATE(version, funcname) \
	Datum funcname(PG_FUNCTION_ARGS); \
	PG_FUNCTION_INFO_V1(funcname); \
	Datum funcname(PG_FUNCTION_ARGS) \
	{ \
		ereport(ERROR, (\
			errcode(ERRCODE_FEATURE_NOT_SUPPORTED), \
			errmsg("A stored procedure tried to use deprecated C function '%s'", \
			       __func__), \
			errdetail("Library function '%s' was deprecated in PostGIS %s", \
			          __func__, version), \
			errhint("Consider running: SELECT postgis_extensions_upgrade()") \
		)); \
		PG_RETURN_POINTER(NULL); \
	}

POSTGIS_DEPRECATE("3.4.0", ST_ConstrainedDelaunayTriangles)