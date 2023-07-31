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

#ifndef USD_WRITER_H
#define USD_WRITER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lwgeom_log.h"
#include "liblwgeom.h"

#ifdef __cplusplus
}
#endif

#include <pxr/usd/usd/stage.h>

namespace USD {

class Writer {
	private:
		void WritePoint(const LWPOINT *p);
		void WriteMultiPoint(const LWMPOINT *mp);
		void WriteLineString(const LWLINE* l);
		void WriteMultiLineString(const LWMLINE* ml);
		void WritePolygon(const LWPOLY *p);
		void WriteMultiPolygon(const LWMPOLY *mp);
		void WriteTriangle(const LWTRIANGLE *t);
		void WriteCollection(const LWCOLLECTION *coll);

	public:
		Writer();
		~Writer();

		void Write(LWGEOM *geom);

		pxr::UsdStageRefPtr m_stage;
};

}; // namespace USD

#endif
