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
 * Copyright 2026 Darafei Praliaskouski <me@komzpa.net>
 *
 **********************************************************************/

#include "liblwgeom_internal.h"

/*
 * Keep public helper wrappers separate from their implementation objects. The
 * PostGIS SQL module links liblwgeom as a hidden static archive and provides
 * its own exported shims for these names, while ordinary liblwgeom users still
 * get the public API from this object.
 */

#if defined(HAVE_LIBJSON)
PGDLLEXPORT LWGEOM *
parse_geojson(struct json_object *geojson, int *hasz)
{
	return lwgeom_parse_geojson(geojson, hasz);
}
#endif

PGDLLEXPORT size_t
lwgeom_to_wkb_size(const LWGEOM *geom, uint8_t variant)
{
	return lwgeom_to_wkb_size_internal(geom, variant);
}

PGDLLEXPORT uint8_t *
lwgeom_to_wkb_buf(const LWGEOM *geom, uint8_t *buf, uint8_t variant)
{
	return lwgeom_to_wkb_buf_internal(geom, buf, variant);
}
