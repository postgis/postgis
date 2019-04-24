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
 * Copyright 2012-2013 Oslandia <infos@oslandia.com>
 *
 **********************************************************************/

#include "liblwgeom_internal.h"
#include <SFCGAL/capi/sfcgal_c.h>

/* return SFCGAL version string */
const char *lwgeom_sfcgal_version(void);

/* Convert SFCGAL structure to lwgeom PostGIS */
LWGEOM *SFCGAL2LWGEOM(const sfcgal_geometry_t *geom, int force3D, int32_t SRID);

/* Convert lwgeom PostGIS to SFCGAL structure */
sfcgal_geometry_t *LWGEOM2SFCGAL(const LWGEOM *geom);

/* No Operation SFCGAL function, used (only) for cunit tests
 * Take a PostGIS geometry, send it to SFCGAL and return it unchanged
 */
LWGEOM *lwgeom_sfcgal_noop(const LWGEOM *geom_in);
