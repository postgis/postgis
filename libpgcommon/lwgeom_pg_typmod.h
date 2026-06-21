/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * This file is part of PostGIS
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

#ifndef LWGEOM_PG_TYPMOD_H
#define LWGEOM_PG_TYPMOD_H 1

/*
 * Macros for manipulating the PostgreSQL typmod int32_t used by the
 * PostGIS geometry and geography types:
 *
 * Plus/minus = Top bit.
 * Spare bits = Next 2 bits.
 * SRID = Next 21 bits.
 * TYPE = Next 6 bits.
 * ZM Flags = Bottom 2 bits.
 */
#define TYPMOD_GET_SRID(typmod) ((((typmod) & 0x0FFFFF00) - ((typmod) & 0x10000000)) >> 8)
#define TYPMOD_SET_SRID(typmod, srid) ((typmod) = (((typmod) & 0xE00000FF) | ((srid & 0x001FFFFF) << 8)))
#define TYPMOD_GET_TYPE(typmod) (((typmod) & 0x000000FC) >> 2)
#define TYPMOD_SET_TYPE(typmod, type) ((typmod) = ((typmod) & 0xFFFFFF03) | (((type) & 0x0000003F) << 2))
#define TYPMOD_GET_Z(typmod) (((typmod) & 0x00000002) >> 1)
#define TYPMOD_SET_Z(typmod) ((typmod) = (typmod) | 0x00000002)
#define TYPMOD_GET_M(typmod) ((typmod) & 0x00000001)
#define TYPMOD_SET_M(typmod) ((typmod) = (typmod) | 0x00000001)
#define TYPMOD_GET_NDIMS(typmod) (2 + TYPMOD_GET_Z(typmod) + TYPMOD_GET_M(typmod))

#endif /* LWGEOM_PG_TYPMOD_H */
