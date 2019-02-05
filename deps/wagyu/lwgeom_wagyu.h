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
 * Copyright 2019 Raúl Marín
 *
 **********************************************************************/

#ifndef LWGEOM_WAGYU_H
#define LWGEOM_WAGYU_H

#ifdef __cplusplus
extern "C" {
#endif

#include <liblwgeom.h>

/**
 * Returns a geometry that represents the point set intersection of 2 polygons.
 * The result is guaranteed to be valid according to OGC standard.
 * This function only supports 2D and will DROP any extra dimensions.
 *
 * The return value will be either NULL (on error) or a new geometry with the
 * same SRID as the first input.
 * The ownership of it is given to the caller.
 *
 * @param geom - A geometry of either POLYGONTYPE or MULTIPOLYGONTYPE type.
 * @param bbox - The GBOX that will be used to clip.
 * @return - NULL on invalid input (NULL or wrong type), or when interrupted.
 *         - An empty MULTIPOLYGONTYPE the geometry is empty.
 *         - A pointer to a LWMPOLY otherwise.
 */
LWGEOM *lwgeom_wagyu_clip_by_box(const LWGEOM *geom, const GBOX *bbox);

/**
 * Returns a pointer to the static string representing the Wagyu release
 * being used (Should not be freed)
 */
const char *libwagyu_version();

/**
 * Requests wagyu to stop processing. In this case it will return NULL
 */
void lwgeom_wagyu_interruptRequest();

#ifdef __cplusplus
}
#endif

#endif /* LWGEOM_WAGYU_H */
