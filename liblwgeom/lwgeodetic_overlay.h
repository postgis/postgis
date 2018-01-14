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
 * Copyright (C) 2017 Danny Götte <danny.goette@fem.tu-ilmenau.de>
 *
 **********************************************************************/


#ifndef _LWGEODETIC_OVERLAY_H
#define _LWGEODETIC_OVERLAY_H 1

#include "lwgeodetic.h"

LWGEOM* lwgeodetic_difference(const LWGEOM* geom1, const LWGEOM* geom2);
LWGEOM* lwgeodetic_symdifference(const LWGEOM* geom1, const LWGEOM* geom2);
LWGEOM* lwgeodetic_intersection(const LWGEOM* geom1, const LWGEOM* geom2);
LWGEOM* lwgeodetic_union(const LWGEOM* geom1, const LWGEOM* geom2);

#endif /* _LWGEODETIC_OVERLAY_H */
