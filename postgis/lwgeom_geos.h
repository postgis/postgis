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
 * Copyright 2008 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 **********************************************************************/


#ifndef LWGEOM_GEOS_H_
#define LWGEOM_GEOS_H_ 1

#include "../liblwgeom/lwgeom_geos.h" /* for GEOSGeom */
#include "liblwgeom.h" /* for GSERIALIZED */
#include "utils/array.h" /* for ArrayType */

/*
** Public prototypes for GEOS utility functions.
*/

GSERIALIZED *GEOS2POSTGIS(GEOSGeom geom, char want3d);
GEOSGeometry *POSTGIS2GEOS(const GSERIALIZED *g);
GEOSGeometry** ARRAY2GEOS(ArrayType* array, uint32_t nelems, int* is3d, int* srid);
LWGEOM** ARRAY2LWGEOM(ArrayType* array, uint32_t nelems, int* is3d, int* srid);

Datum geos_intersects(PG_FUNCTION_ARGS);
Datum geos_intersection(PG_FUNCTION_ARGS);
Datum geos_difference(PG_FUNCTION_ARGS);
Datum geos_geomunion(PG_FUNCTION_ARGS);
Datum LWGEOM_area_polygon(PG_FUNCTION_ARGS);
Datum LWGEOM_mindistance2d(PG_FUNCTION_ARGS);
Datum ST_3DDistance(PG_FUNCTION_ARGS);

uint32_t array_nelems_not_null(ArrayType* array);

#endif /* LWGEOM_GEOS_H_ */
