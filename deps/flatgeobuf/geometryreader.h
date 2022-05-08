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
 * Copyright 2021 Björn Harrtell
 *
 **********************************************************************/

#ifndef FLATGEOBUF_GEOMETRYREADER_H
#define FLATGEOBUF_GEOMETRYREADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lwgeom_log.h"
#include "liblwgeom.h"

#ifdef __cplusplus
}
#endif

#include "feature_generated.h"

namespace FlatGeobuf {

class GeometryReader {
	private:
		const FlatGeobuf::Geometry *m_geometry;
		FlatGeobuf::GeometryType m_geometry_type;
		bool m_has_z;
		bool m_has_m;

		uint32_t m_length = 0;
		uint32_t m_offset = 0;

		LWPOINT *readPoint();
		LWMPOINT *readMultiPoint();
		LWLINE *readLineString();
		LWMLINE *readMultiLineString();
		LWPOLY *readPolygon();
		LWMPOLY *readMultiPolygon();
		LWCOLLECTION *readGeometryCollection();

		POINTARRAY *readPA();

	public:
		GeometryReader(
			const FlatGeobuf::Geometry *geometry,
			FlatGeobuf::GeometryType geometry_type,
			bool has_z,
			bool has_m) :
			m_geometry (geometry),
			m_geometry_type (geometry_type),
			m_has_z (has_z),
			m_has_m (has_m)
			{ }
		LWGEOM *read();
};

}


#endif /* FLATGEOBUF_GEOMETRYREADER_H */