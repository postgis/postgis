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

#include <pxr/base/tf/staticData.h>
#include <pxr/base/tf/token.h>

namespace USD {

class Tokens final {
	public:
		Tokens();

		const pxr::TfToken postgis_points;
		const pxr::TfToken postgis_has_z;
		const pxr::TfToken postgis_has_m;
		const pxr::TfToken postgis_srid;
		const pxr::TfToken postgis_type_name;
};

} // namespace USD

#endif
