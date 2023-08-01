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

#ifndef USD_READER_H
#define USD_READER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lwgeom_log.h"
#include "liblwgeom.h"

#ifdef __cplusplus
}
#endif

#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

namespace USD {

class Reader {
	private:
		void ScanPrim(pxr::UsdPrim prim);

		LWGEOM *ReadMesh(pxr::UsdPrim prim);

	public:
		Reader();
		~Reader();

		void ReadUSDA(const std::string &usda_string);
		void ReadUSDC(const char *data, size_t size);

		std::vector<pxr::UsdPrim> m_prims;
        std::vector<LWGEOM *> m_geoms;
};

} // namespace USD

#endif
