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

#ifndef FLATGEOBUF_GEOMETRYWRITER_H
#define FLATGEOBUF_GEOMETRYWRITER_H

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

class GeometryWriter {
	private:
		flatbuffers::FlatBufferBuilder &m_fbb;
		const LWGEOM *m_lwgeom;
		FlatGeobuf::GeometryType m_geometry_type;
		const bool m_has_z;
		const bool m_has_m;
		std::vector<double> m_xy;
		std::vector<double> m_z;
		std::vector<double> m_m;

		void writePoint(const LWPOINT *p);
		void writeMultiPoint(const LWMPOINT *mp);
		void writeLineString(const LWLINE *l);
		void writeMultiLineString(const LWMLINE *ml);
		void writePolygon(const LWPOLY *p);
		const flatbuffers::Offset<FlatGeobuf::Geometry> writeMultiPolygon(const LWMPOLY *mp, int depth);
		const flatbuffers::Offset<FlatGeobuf::Geometry> writeGeometryCollection(const LWCOLLECTION *gc, int depth);

		void writePA(POINTARRAY *pa);
		void writePPA(POINTARRAY **ppa, uint32_t len);
		/*
		const flatbuffers::Offset<FlatGeobuf::Geometry> writeCompoundCurve(const OGRCompoundCurve *cc, int depth);
		const flatbuffers::Offset<FlatGeobuf::Geometry> writeCurvePolygon(const OGRCurvePolygon *cp, int depth);
		const flatbuffers::Offset<FlatGeobuf::Geometry> writePolyhedralSurface(const OGRPolyhedralSurface *p, int depth);
		void writeTIN(const OGRTriangulatedSurface *p);
		*/

	public:
		GeometryWriter(
			flatbuffers::FlatBufferBuilder &fbb,
			const LWGEOM *lwgeom,
			const FlatGeobuf::GeometryType geometry_type,
			const bool has_z,
			const bool has_m) :
			m_fbb (fbb),
			m_lwgeom (lwgeom),
			m_geometry_type (geometry_type),
			m_has_z (has_z),
			m_has_m (has_m)
			{ }
		std::vector<uint32_t> m_ends;
		const flatbuffers::Offset<FlatGeobuf::Geometry> write(int depth);
		static const FlatGeobuf::GeometryType get_geometrytype(const LWGEOM *lwgeom);
};

}


#endif /* FLATGEOBUF_GEOMETRYWRITER_H */