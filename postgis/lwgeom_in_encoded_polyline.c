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
* Copyright 2014 Kashif Rasul <kashif.rasul@gmail.com> and
 *
 **********************************************************************/


#include <assert.h>

#include "postgres.h"
#include "utils/builtins.h"

#include "../postgis_config.h"
#include "lwgeom_pg.h"
#include "liblwgeom.h"

Datum line_from_encoded_polyline(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(line_from_encoded_polyline);
Datum line_from_encoded_polyline(PG_FUNCTION_ARGS)
{
  GSERIALIZED *geom;
  LWGEOM *lwgeom;
  text *encodedpolyline_input;
  char *encodedpolyline;
  int precision = 5;

  if (PG_ARGISNULL(0)) PG_RETURN_NULL();

  encodedpolyline_input = PG_GETARG_TEXT_P(0);
  encodedpolyline = text_to_cstring(encodedpolyline_input);

  if (PG_NARGS() > 1 && !PG_ARGISNULL(1))
  {
    precision = PG_GETARG_INT32(1);
    if ( precision < 0 ) precision = 5;
  }

  lwgeom = lwgeom_from_encoded_polyline(encodedpolyline, precision);
  if ( ! lwgeom ) {
    /* Shouldn't get here */
    elog(ERROR, "lwgeom_from_encoded_polyline returned NULL");
    PG_RETURN_NULL();
  }
  lwgeom_set_srid(lwgeom, 4326);

  geom = geometry_serialize(lwgeom);
  lwgeom_free(lwgeom);
  PG_RETURN_POINTER(geom);
}
