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
 **********************************************************************/


#include "../postgis_config.h"

/* PostgreSQL */
#include "access/htup_details.h"
#include "funcapi.h"
#include "postgres.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"
#include "utils/numeric.h"

/* PostGIS */
#include "liblwgeom.h"

Datum geos_intersects2(PG_FUNCTION_ARGS);


PG_FUNCTION_INFO_V1(geos_intersects2);
Datum geos_intersects2(PG_FUNCTION_ARGS)
{
	return DirectFunctionCall2(geos_intersects2,
		PG_GETARG_DATUM(0),
		PG_GETARG_DATUM(1));
}


