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
 * Copyright (C) 2020 Darafei Praliaskouski <me@komzpa.net>
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

#include <postgres.h>
#include <fmgr.h>

#include "rtpostgis.h"

/* Removed in PostGIS 2.5.0 */
Datum RASTER_to_binary(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(RASTER_to_binary);
Datum RASTER_to_binary(PG_FUNCTION_ARGS)
{
	elog(ERROR, "RASTER_to_binary: This function is out of date. Run: ALTER EXTENSION postgis UPDATE; SELECT PostGIS_Extensions_Upgrade();");
}

/* Removed in PostGIS 3.1.0 */
PG_FUNCTION_INFO_V1(RASTER_histogramCoverage);
Datum RASTER_histogramCoverage(PG_FUNCTION_ARGS)
{
	elog(ERROR, "RASTER_histogramCoverage: This function is out of date. Run: ALTER EXTENSION postgis UPDATE; SELECT PostGIS_Extensions_Upgrade();");
}

PG_FUNCTION_INFO_V1(RASTER_quantileCoverage);
Datum RASTER_quantileCoverage(PG_FUNCTION_ARGS)
{
	elog(ERROR, "RASTER_quantileCoverage: This function is out of date. Run: ALTER EXTENSION postgis UPDATE; SELECT PostGIS_Extensions_Upgrade();");
}
