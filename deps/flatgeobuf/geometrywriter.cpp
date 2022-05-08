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

#include "geometrywriter.h"

using namespace FlatGeobuf;
using namespace flatbuffers;

const GeometryType GeometryWriter::get_geometrytype(const LWGEOM *lwgeom)
{
	int type = lwgeom->type;
	switch (type)
	{
	case POINTTYPE:
		return GeometryType::Point;
	case LINETYPE:
		return GeometryType::LineString;
	case TRIANGLETYPE:
		return GeometryType::Triangle;
	case POLYGONTYPE:
		return GeometryType::Polygon;
	case MULTIPOINTTYPE:
		return GeometryType::MultiPoint;
	case MULTILINETYPE:
		return GeometryType::MultiLineString;
	case MULTIPOLYGONTYPE:
		return GeometryType::MultiPolygon;
	case TINTYPE:
	case COLLECTIONTYPE:
		return GeometryType::GeometryCollection;
	default:
		lwerror("flatgeobuf: get_geometrytype: '%s' geometry type not supported",
				lwtype_name(type));
		return GeometryType::Unknown;
	}
}

void GeometryWriter::writePoint(const LWPOINT *p)
{
	writePA(p->point);
}

void GeometryWriter::writeMultiPoint(const LWMPOINT *mp)
{
	writePA(lwline_from_lwmpoint(0, mp)->points);
}

void GeometryWriter::writeLineString(const LWLINE *l)
{
	writePA(l->points);
}

void GeometryWriter::writeMultiLineString(const LWMLINE *ml)
{
	uint32_t ngeoms = ml->ngeoms;
	if (ngeoms == 1)
		return writePA(ml->geoms[0]->points);
	POINTARRAY **ppa = (POINTARRAY **) lwalloc(sizeof(POINTARRAY *) * ngeoms);
	for (uint32_t i = 0; i < ngeoms; i++)
		ppa[i] = ml->geoms[i]->points;
	writePPA(ppa, ngeoms);
}

void GeometryWriter::writePolygon(const LWPOLY *p)
{
	writePPA(p->rings, p->nrings);
}

const Offset<Geometry> GeometryWriter::writeMultiPolygon(const LWMPOLY *mp, int depth)
{
	std::vector<Offset<Geometry>> parts;
	for (uint32_t i = 0; i < mp->ngeoms; i++)
	{
		auto part = mp->geoms[i];
		if (part->nrings != 0)
		{
			GeometryWriter writer { m_fbb, (LWGEOM *) part, GeometryType::Polygon, m_has_z, m_has_m };
			parts.push_back(writer.write(depth + 1));
		}
	}
	return CreateGeometryDirect(m_fbb, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, m_geometry_type, &parts);
}

const Offset<Geometry> GeometryWriter::writeGeometryCollection(const LWCOLLECTION *gc, int depth)
{
	std::vector<Offset<Geometry>> parts;
	for (uint32_t i = 0; i < gc->ngeoms; i++)
	{
		auto part = gc->geoms[i];
		auto geometry_type = get_geometrytype(part);
		GeometryWriter writer { m_fbb, part, geometry_type, m_has_z, m_has_m };
		parts.push_back(writer.write(depth + 1));
	}
	return CreateGeometryDirect(m_fbb, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, m_geometry_type, &parts);
}

void GeometryWriter::writePA(POINTARRAY *pa)
{
	POINT4D pt;
	for (uint32_t i = 0; i < pa->npoints; i++) {
		getPoint4d_p(pa, i, &pt);
		m_xy.push_back(pt.x);
		m_xy.push_back(pt.y);
		if (m_has_z)
			m_z.push_back(pt.z);
		if (m_has_m)
			m_m.push_back(pt.m);
	}
}

void GeometryWriter::writePPA(POINTARRAY **ppa, uint32_t len)
{
	if (len == 0)
		return;
	POINTARRAY *pa = ppa[0];
	writePA(pa);
	if (len > 1) {
		uint32_t e = pa->npoints;
		m_ends.push_back(e);
		for (uint32_t i = 1; i < len; i++) {
			pa = ppa[i];
			writePA(pa);
			m_ends.push_back(e += pa->npoints);
		}
	}
}

const Offset<Geometry> GeometryWriter::write(int depth)
{
	bool unknownGeometryType = false;
	if (depth == 0 && m_geometry_type == GeometryType::Unknown) {
		m_geometry_type = get_geometrytype(m_lwgeom);
		unknownGeometryType = true;
	}
	switch (m_geometry_type) {
		case GeometryType::Point:
			writePoint((LWPOINT *) m_lwgeom); break;
		case GeometryType::MultiPoint:
			writeMultiPoint((LWMPOINT *) m_lwgeom); break;
		case GeometryType::LineString:
			writeLineString((LWLINE *) m_lwgeom); break;
		case GeometryType::MultiLineString:
			writeMultiLineString((LWMLINE *) m_lwgeom); break;
		case GeometryType::Polygon:
			writePolygon((LWPOLY *) m_lwgeom); break;
		case GeometryType::MultiPolygon:
			return writeMultiPolygon((LWMPOLY *) m_lwgeom, depth);
		case GeometryType::GeometryCollection:
			return writeGeometryCollection((LWCOLLECTION *) m_lwgeom, depth);
		/*case GeometryType::CircularString:
			writeSimpleCurve(m_ogrGeometry->toCircularString()); break;
		case GeometryType::CompoundCurve:
			return writeCompoundCurve(m_ogrGeometry->toCompoundCurve(), depth);
		case GeometryType::CurvePolygon:
			return writeCurvePolygon(m_ogrGeometry->toCurvePolygon(), depth);
		case GeometryType::MultiCurve:
			return writeGeometryCollection(m_ogrGeometry->toMultiCurve(), depth);
		case GeometryType::MultiSurface:
			return writeGeometryCollection(m_ogrGeometry->toMultiSurface(), depth);
		case GeometryType::PolyhedralSurface:
			return writePolyhedralSurface(m_ogrGeometry->toPolyhedralSurface(), depth);
		case GeometryType::Triangle:
			writePolygon(m_ogrGeometry->toTriangle()); break;
		case GeometryType::TIN:
			writeTIN(m_ogrGeometry->toTriangulatedSurface()); break;*/
		default:
			lwerror("flatgeobuf: GeometryWriter::write: '%s' geometry type not supported",
				lwtype_name(m_lwgeom->type));
			return 0;
	}
	const auto pEnds = m_ends.empty() ? nullptr : &m_ends;
	const auto pXy = m_xy.empty() ? nullptr : &m_xy;
	const auto pZ = m_z.empty() ? nullptr : &m_z;
	const auto pM = m_m.empty() ? nullptr : &m_m;
	const auto geometryType = depth > 0 || unknownGeometryType ? m_geometry_type : GeometryType::Unknown;
	return FlatGeobuf::CreateGeometryDirect(m_fbb, pEnds, pXy, pZ, pM, nullptr, nullptr, geometryType);
}