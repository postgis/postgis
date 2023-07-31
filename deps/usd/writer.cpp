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

#include "writer.h"

#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usdGeom/basisCurves.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/points.h>
#include <pxr/usd/usdGeom/primvarsAPI.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdGeom/xform.h>

using namespace USD;

static const int PRIM_MAX_SIBLING = 999999;

static const pxr::SdfPath USD_ROOT_PRIM_PATH("/World");
static const pxr::SdfPath USD_GEOM_PRIM_PATH("/World/_geometry");

static const pxr::TfToken USD_ATTR_POSTGIS_SRID("postgis:srid");
static const pxr::TfToken USD_ATTR_POSTGIS_TYPE_NAME("postgis:type_name");
static const pxr::TfToken USD_ATTR_POSTGIS_HAS_Z("postgis:has_z");
static const pxr::TfToken USD_ATTR_POSTGIS_HAS_M("postgis:has_m");
static const pxr::TfToken USD_ATTR_POSTGIS_POINTS("postgis:points");

static pxr::SdfPath
GenerateNextSdfPath(pxr::UsdStageRefPtr stage, const pxr::SdfPath &test_path)
{
	auto prim = stage->GetPrimAtPath(test_path);
	if (!prim)
	{
		return test_path;
	}

	auto parent_path = test_path.GetParentPath();
	pxr::SdfPath new_path;
	for (int i = 1; i < PRIM_MAX_SIBLING; i++)
	{
		std::string new_path_string = pxr::TfStringPrintf("%s_%d", test_path.GetText(), i);
		new_path = pxr::SdfPath(new_path_string);
		auto prim = stage->GetPrimAtPath(new_path);
		if (prim)
		{
			continue;
		}
		else
		{
			break;
		}
	}
	return new_path;
}

template <typename T>
static void
SetCustomAttribute(pxr::UsdPrim prim, const pxr::TfToken &name, const pxr::SdfValueTypeName &type, const T &value)
{
	auto attr = prim.CreateAttribute(name, type);
	attr.Set(pxr::VtValue(value));
}

template <class T>
static T
DefineGeom(pxr::UsdStageRefPtr stage, const pxr::SdfPath &path, int srid, const char *type_name, int has_z, int has_m)
{
	auto geometry = T::Define(stage, path);
	auto prim = geometry.GetPrim();

	SetCustomAttribute(prim, USD_ATTR_POSTGIS_SRID, pxr::SdfValueTypeNames->Int, srid);
	SetCustomAttribute(prim, USD_ATTR_POSTGIS_TYPE_NAME, pxr::SdfValueTypeNames->String, std::string(type_name));
	SetCustomAttribute(prim, USD_ATTR_POSTGIS_HAS_Z, pxr::SdfValueTypeNames->Int, has_z);
	SetCustomAttribute(prim, USD_ATTR_POSTGIS_HAS_M, pxr::SdfValueTypeNames->Int, has_m);

	return geometry;
}

static void
ReadPointArray(pxr::VtVec3fArray &points, pxr::VtVec4dArray &xyzm_points, const POINTARRAY *pa)
{
	for (int i = 0; i < pa->npoints; i++)
	{
		POINT4D pt = {0, 0, 0, 0};
		getPoint4d_p(pa, i, &pt);

		pxr::GfVec3f point(pt.x, pt.y, pt.z);
		points.push_back(point);

		pxr::GfVec4d xyzm_point(pt.x, pt.y, pt.z, pt.m);
		xyzm_points.push_back(xyzm_point);
	}
}

void
Writer::WritePoint(const LWPOINT *p)
{
	auto prim_path = GenerateNextSdfPath(m_stage, USD_GEOM_PRIM_PATH);
	auto geometry = DefineGeom<pxr::UsdGeomPoints>(
	    m_stage, prim_path, p->srid, lwtype_name(p->type), FLAGS_GET_Z(p->flags), FLAGS_GET_M(p->flags));
	auto prim = geometry.GetPrim();

	pxr::VtVec3fArray points;
	pxr::VtVec4dArray xyzm_points;
	POINTARRAY *pa = p->point;
	ReadPointArray(points, xyzm_points, pa);

	auto points_attr = geometry.CreatePointsAttr();
	points_attr.Set(points);

	SetCustomAttribute(prim, USD_ATTR_POSTGIS_POINTS, pxr::SdfValueTypeNames->Double4Array, xyzm_points);
}

void
Writer::WriteMultiPoint(const LWMPOINT *mp)
{
	auto prim_path = GenerateNextSdfPath(m_stage, USD_GEOM_PRIM_PATH);
	auto geometry = pxr::UsdGeomPoints::Define(m_stage, pxr::SdfPath("_geometry"));
	auto prim = geometry.GetPrim();

	pxr::VtVec3fArray points;
	pxr::VtVec4dArray xyzm_points;
	for (int g = 0; g < mp->ngeoms; g++)
	{
		LWPOINT *p = mp->geoms[g];
		POINTARRAY *pa = p->point;
		ReadPointArray(points, xyzm_points, pa);
	}

	auto points_attr = geometry.CreatePointsAttr();
	points_attr.Set(points);

	SetCustomAttribute(prim, USD_ATTR_POSTGIS_POINTS, pxr::SdfValueTypeNames->Double4Array, xyzm_points);
}

void
Writer::WriteLineString(const LWLINE *l)
{
	auto prim_path = GenerateNextSdfPath(m_stage, USD_GEOM_PRIM_PATH);
	auto geometry = DefineGeom<pxr::UsdGeomBasisCurves>(
	    m_stage, prim_path, l->srid, lwtype_name(l->type), FLAGS_GET_Z(l->flags), FLAGS_GET_M(l->flags));
	auto prim = geometry.GetPrim();

	/* set type of curve to be linear */
	auto type_attr = geometry.CreateTypeAttr();
	type_attr.Set(pxr::UsdGeomTokens->linear);

	pxr::VtVec3fArray points;
	pxr::VtVec4dArray xyzm_points;
	POINTARRAY *pa = l->points;
	ReadPointArray(points, xyzm_points, pa);

	auto points_attr = geometry.CreatePointsAttr();
	points_attr.Set(points);

	SetCustomAttribute(prim, USD_ATTR_POSTGIS_POINTS, pxr::SdfValueTypeNames->Double4Array, xyzm_points);

	auto cvc_attr = geometry.CreateCurveVertexCountsAttr();
	pxr::VtIntArray cvc{static_cast<int>(pa->npoints)};
	cvc_attr.Set(cvc);
}

void
Writer::WriteMultiLineString(const LWMLINE *ml)
{
	auto prim_path = GenerateNextSdfPath(m_stage, USD_GEOM_PRIM_PATH);
	auto geometry = DefineGeom<pxr::UsdGeomBasisCurves>(
	    m_stage, prim_path, ml->srid, lwtype_name(ml->type), FLAGS_GET_Z(ml->flags), FLAGS_GET_M(ml->flags));
	auto prim = geometry.GetPrim();

	auto type_attr = geometry.CreateTypeAttr();
	type_attr.Set(pxr::UsdGeomTokens->linear);

	pxr::VtVec3fArray points;
	pxr::VtVec4dArray xyzm_points;

	pxr::VtIntArray cvc;

	for (int g = 0; g < ml->ngeoms; g++)
	{
		LWLINE *l = ml->geoms[g];
		POINTARRAY *pa = l->points;
		ReadPointArray(points, xyzm_points, pa);
		cvc.push_back(pa->npoints);
	}

	auto points_attr = geometry.CreatePointsAttr();
	points_attr.Set(points);
	SetCustomAttribute(prim, USD_ATTR_POSTGIS_POINTS, pxr::SdfValueTypeNames->Double4Array, xyzm_points);

	auto cvc_attr = geometry.CreateCurveVertexCountsAttr();
	cvc_attr.Set(cvc);
}

void
Writer::WritePolygon(const LWPOLY *p)
{
	auto prim_path = GenerateNextSdfPath(m_stage, USD_GEOM_PRIM_PATH);
	auto geometry = DefineGeom<pxr::UsdGeomMesh>(
	    m_stage, prim_path, p->srid, lwtype_name(p->type), FLAGS_GET_Z(p->flags), FLAGS_GET_M(p->flags));
	auto prim = geometry.GetPrim();

	pxr::VtIntArray fvi;
	pxr::VtIntArray fvc;
	pxr::VtVec3fArray points;
	pxr::VtVec4dArray xyzm_points;

	POINTARRAY *boundary_pa = p->rings[0];
	ReadPointArray(points, xyzm_points, boundary_pa);

	/* remove duplicated last point of polygon */
	points.pop_back();
	xyzm_points.pop_back();
	fvc.push_back(boundary_pa->npoints - 1);

	auto fvi_attr = geometry.GetFaceVertexIndicesAttr();
	fvi.resize(fvc.front());
	std::iota(std::begin(fvi), std::end(fvi), 0);
	fvi_attr.Set(fvi);

	auto fvc_attr = geometry.CreateFaceVertexCountsAttr();
	fvc_attr.Set(fvc);

	auto points_attr = geometry.CreatePointsAttr();
	points_attr.Set(points);

	SetCustomAttribute(prim, USD_ATTR_POSTGIS_POINTS, pxr::SdfValueTypeNames->Double4Array, xyzm_points);
}

void
Writer::WriteMultiPolygon(const LWMPOLY *mp)
{
	for (int i = 0; i < mp->ngeoms; ++i)
	{
		LWPOLY *geom = mp->geoms[i];
		WritePolygon(geom);
	}
}

void
Writer::WriteTriangle(const LWTRIANGLE *t)
{
	auto prim_path = GenerateNextSdfPath(m_stage, USD_GEOM_PRIM_PATH);
	auto geometry = DefineGeom<pxr::UsdGeomMesh>(
	    m_stage, prim_path, t->srid, lwtype_name(t->type), FLAGS_GET_Z(t->flags), FLAGS_GET_M(t->flags));
	auto prim = geometry.GetPrim();

	pxr::VtIntArray fvi;
	pxr::VtIntArray fvc;
	pxr::VtVec3fArray points;
	pxr::VtVec4dArray xyzm_points;

	POINTARRAY *pa = t->points;
	ReadPointArray(points, xyzm_points, pa);

	auto fvi_attr = geometry.GetFaceVertexIndicesAttr();
	fvi_attr.Set(fvi);

	auto fvc_attr = geometry.CreateFaceVertexCountsAttr();
	fvc_attr.Set(fvc);

	auto points_attr = geometry.CreatePointsAttr();
	points_attr.Set(points);

	SetCustomAttribute(prim, USD_ATTR_POSTGIS_POINTS, pxr::SdfValueTypeNames->Double4Array, xyzm_points);
}

void
Writer::WriteCollection(const LWCOLLECTION *coll)
{
	for (int i = 0; i < coll->ngeoms; i++)
	{
		auto part = coll->geoms[i];
		Write(part);
	}
}

Writer::Writer()
{
	m_stage = pxr::UsdStage::CreateInMemory();
}

Writer::~Writer() {}

void
Writer::Write(LWGEOM *geom)
{
	try
	{
		/* set USD default prim */
		auto xform_world = pxr::UsdGeomXform::Define(m_stage, USD_ROOT_PRIM_PATH);
		m_stage->SetDefaultPrim(xform_world.GetPrim());

		int type = geom->type;
		switch (type)
		{
		case POINTTYPE:
			WritePoint((LWPOINT *)geom);
			break;
		case MULTIPOINTTYPE:
			WriteMultiPoint((LWMPOINT *)geom);
			break;
		case LINETYPE:
			WriteLineString((LWLINE *)geom);
			break;
		case MULTILINETYPE:
			WriteMultiLineString((LWMLINE *)geom);
			break;
		case POLYGONTYPE:
			WritePolygon((LWPOLY *)geom);
			break;
		case MULTIPOLYGONTYPE:
			WriteMultiPolygon((LWMPOLY *)geom);
			break;
		case TRIANGLETYPE:
			WriteTriangle((LWTRIANGLE *)geom);
			break;
		case COLLECTIONTYPE:
			WriteCollection((LWCOLLECTION *)geom);
			break;
		default:
			lwerror("usd: geometry type '%s' not supported", lwtype_name(type));
			break;
		}
	}
	catch (const std::exception &e)
	{
		lwerror("usd: caught standard c++ exception '%s'", e.what());
	}
	catch (...)
	{
		lwerror("usd: caught unknown c++ exception");
	}
}
