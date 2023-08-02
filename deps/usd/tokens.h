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
 * Copyright 2023 J CUBE Inc and Jadason Technology Ltd
 *
 **********************************************************************/

#ifndef USD_TOKENS_H
#define USD_TOKENS_H

#include <pxr/base/tf/token.h>

namespace USD {

static const pxr::TfToken TOKEN_POSTGIS_SRID;
static const pxr::TfToken TOKEN_POSTGIS_TYPE_NAME;
static const pxr::TfToken TOKEN_POSTGIS_HAS_Z;
static const pxr::TfToken TOKEN_POSTGIS_HAS_M;
static const pxr::TfToken TOKEN_POSTGIS_POINTS;

} // namespace USD

#endif
