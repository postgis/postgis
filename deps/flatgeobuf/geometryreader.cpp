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

#include "geometryreader.h"

using namespace flatbuffers;
using namespace FlatGeobuf;

LWPOINT *GeometryReader::readPoint()
{
	POINTARRAY *pa;
	POINT4D pt;

	pa = ptarray_construct_empty(m_has_z, m_has_m, 1);

	if (m_geometry->xy() == nullptr || m_geometry->xy()->size() == 0) {
		return lwpoint_construct(0, NULL, pa);
	}

	const auto xy = m_geometry->xy()->data();

	double x = xy[m_offset + 0];
	double y = xy[m_offset + 1];
	double z = 0;
	double m = 0;

	if (m_has_z)
		z = m_geometry->z()->data()[m_offset];
	if (m_has_m)
		m = m_geometry->m()->data()[m_offset];

	pt = (POINT4D) { x, y, z, m };
	ptarray_append_point(pa, &pt, LW_TRUE);
	return lwpoint_construct(0, NULL, pa);
}

POINTARRAY *GeometryReader::readPA()
{
	POINTARRAY *pa;
	POINT4D pt;
	uint32_t npoints;

	const double *xy = m_geometry->xy()->data();
	const double *z = m_has_z ? m_geometry->z()->data() : nullptr;
	const double *m = m_has_m ? m_geometry->m()->data() : nullptr;

	pa = ptarray_construct_empty(m_has_z, m_has_m, m_length);

	for (uint32_t i = m_offset; i < m_offset + m_length; i++) {
		double xv = xy[i * 2 + 0];
		double yv = xy[i * 2 + 1];
		double zv = 0;
		double mv = 0;
		if (m_has_z)
			zv = z[i];
		if (m_has_m)
			mv = m[i];
		pt = (POINT4D) { xv, yv, zv, mv };
		ptarray_append_point(pa, &pt, LW_TRUE);
	}

	return pa;
}

LWMPOINT *GeometryReader::readMultiPoint()
{
	POINTARRAY *pa = readPA();
	return lwmpoint_construct(0, pa);
}

LWLINE *GeometryReader::readLineString()
{
	POINTARRAY *pa = readPA();
	return lwline_construct(0, NULL, pa);
}

LWMLINE *GeometryReader::readMultiLineString()
{
	auto ends = m_geometry->ends();

	uint32_t ngeoms = 1;
	if (ends != nullptr && ends->size() > 1)
		ngeoms = ends->size();

	auto *lwmline = lwmline_construct_empty(0, m_has_z, m_has_m);
	if (ngeoms > 1) {
		for (uint32_t i = 0; i < ngeoms; i++) {
			const auto e = ends->Get(i);
			m_length = e - m_offset;
			POINTARRAY *pa = readPA();
			lwmline_add_lwline(lwmline, lwline_construct(0, NULL, pa));
			m_offset = e;
		}
	} else {
		POINTARRAY *pa = readPA();
		lwmline_add_lwline(lwmline, lwline_construct(0, NULL, pa));
	}

	return lwmline;
}

LWPOLY *GeometryReader::readPolygon()
{
	const auto ends = m_geometry->ends();

	uint32_t nrings = 1;
	if (ends != nullptr && ends->size() > 1)
		nrings = ends->size();

	auto **ppa = (POINTARRAY **) lwalloc(sizeof(POINTARRAY *) * nrings);
	if (nrings > 1) {
		for (uint32_t i = 0; i < nrings; i++) {
			const auto e = ends->Get(i);
			m_length = e - m_offset;
			ppa[i] = readPA();
			m_offset = e;
		}
	} else {
		ppa[0] = readPA();
	}

	return lwpoly_construct(0, NULL, nrings, ppa);
}

LWMPOLY *GeometryReader::readMultiPolygon()
{
	auto parts = m_geometry->parts();
	auto *mp = lwmpoly_construct_empty(0, m_has_z, m_has_m);
	for (uoffset_t i = 0; i < parts->size(); i++) {
		GeometryReader reader { parts->Get(i), GeometryType::Polygon, m_has_z, m_has_m };
		const auto p = (LWPOLY *) reader.read();
		lwmpoly_add_lwpoly(mp, p);
	}
	return mp;
}

LWCOLLECTION *GeometryReader::readGeometryCollection()
{
	auto parts = m_geometry->parts();
	auto *gc = lwcollection_construct_empty(COLLECTIONTYPE, 0, m_has_z, m_has_m);
	for (uoffset_t i = 0; i < parts->size(); i++) {
		auto part = parts->Get(i);
		GeometryReader reader { part, part->type(), m_has_z, m_has_m };
		const auto g = reader.read();
		lwcollection_add_lwgeom(gc, g);
	}
	return gc;
}

LWGEOM *GeometryReader::read()
{
	// nested types
	switch (m_geometry_type) {
		case GeometryType::GeometryCollection: return (LWGEOM *) readGeometryCollection();
		case GeometryType::MultiPolygon: return (LWGEOM *) readMultiPolygon();
		/*case GeometryType::CompoundCurve: return readCompoundCurve();
		case GeometryType::CurvePolygon: return readCurvePolygon();
		case GeometryType::MultiCurve: return readMultiCurve();
		case GeometryType::MultiSurface: return readMultiSurface();
		case GeometryType::PolyhedralSurface: return readPolyhedralSurface();*/
		default: break;
	}

	// if not nested must have geometry data
	const auto pXy = m_geometry->xy();
	const auto xySize = pXy->size();
	m_length = xySize / 2;

	switch (m_geometry_type) {
		case GeometryType::Point: return (LWGEOM *) readPoint();
		case GeometryType::MultiPoint: return (LWGEOM *) readMultiPoint();
		case GeometryType::LineString: return (LWGEOM *) readLineString();
		case GeometryType::MultiLineString: return (LWGEOM *) readMultiLineString();
		case GeometryType::Polygon: return (LWGEOM *) readPolygon();
		/*
		case GeometryType::CircularString: return readSimpleCurve<OGRCircularString>(true);
		case GeometryType::Triangle: return readTriangle();
		case GeometryType::TIN: return readTIN();
		*/
		default:
			lwerror("flatgeobuf: GeometryReader::read: Unknown type %d", (int) m_geometry_type);
	}
	return nullptr;
}