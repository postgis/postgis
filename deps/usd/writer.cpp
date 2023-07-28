#include "writer.h"

#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usdGeom/basisCurves.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/points.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdGeom/xform.h>

using namespace USD;

static const int PRIM_MAX_SIBLING = 999999;

static const pxr::SdfPath ROOT_PRIM_PATH("/World");
static const pxr::SdfPath GEOMETRY_PRIM_PATH("/World/_geometry");

static const pxr::TfToken POSTGIS_SRID("postgis:token");
static const pxr::TfToken POSTGIS_GEOMETRY_TYPE("postgis:geometry_type");

template <typename AT, typename ET>
static void
PushVtArray(AT &a, const ET &e)
{
	a.push_back(e);
}

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

void
Writer::WritePoint(const LWPOINT *p)
{
	auto prim_path = GenerateNextSdfPath(m_stage, GEOMETRY_PRIM_PATH);
	auto geometry = pxr::UsdGeomPoints::Define(m_stage, prim_path);

	auto points_attr = geometry.CreatePointsAttr();

	pxr::VtVec2dArray points;
	POINTARRAY *pa = p->point;
	for (int i = 0; i < pa->npoints; i++)
	{
		POINT4D pt = {0};
		getPoint4d_p(pa, i, &pt);
		PushVtArray(points, pxr::GfVec2d(pt.x, pt.y));
	}
	points_attr.Set(points);
}

void
Writer::WriteMultiPoint(const LWMPOINT *mp)
{
	auto prim_path = GenerateNextSdfPath(m_stage, GEOMETRY_PRIM_PATH);
	auto geometry = pxr::UsdGeomPoints::Define(m_stage, pxr::SdfPath("_geometry"));

	auto points_attr = geometry.CreatePointsAttr();

	pxr::VtVec2dArray points;
	for (int g = 0; g < mp->ngeoms; g++)
	{
		LWPOINT *p = mp->geoms[g];
		POINTARRAY *pa = p->point;
		for (int i = 0; i < pa->npoints; i++)
		{
			POINT4D pt = {0};
			getPoint4d_p(pa, i, &pt);
			PushVtArray(points, pxr::GfVec2d(pt.x, pt.y));
		}
	}
	points_attr.Set(points);
}

void
Writer::WriteLineString(const LWLINE *l)
{
	auto prim_path = GenerateNextSdfPath(m_stage, GEOMETRY_PRIM_PATH);
	auto geometry = pxr::UsdGeomBasisCurves::Define(m_stage, prim_path);

	/* set type of curve to be linear */
	auto type_attr = geometry.CreateTypeAttr();
	type_attr.Set(pxr::UsdGeomTokens->linear);

	auto points_attr = geometry.CreatePointsAttr();
	pxr::VtVec2dArray points;
	POINTARRAY *pa = l->points;
	for (int i = 0; i < pa->npoints; i++)
	{
		POINT4D pt = {0};
		getPoint4d_p(pa, i, &pt);
		PushVtArray(points, pxr::GfVec2d(pt.x, pt.y));
	}
	points_attr.Set(points);

	auto cvc_attr = geometry.CreateCurveVertexCountsAttr();
	pxr::VtIntArray cvc{static_cast<int>(pa->npoints)};
	cvc_attr.Set(cvc);
}

void
Writer::WriteMultiLineString(const LWMLINE *ml)
{
	auto prim_path = GenerateNextSdfPath(m_stage, GEOMETRY_PRIM_PATH);
	auto geometry = pxr::UsdGeomBasisCurves::Define(m_stage, prim_path);

	auto type_attr = geometry.CreateTypeAttr();
	type_attr.Set(pxr::UsdGeomTokens->linear);

	auto points_attr = geometry.CreatePointsAttr();
	pxr::VtVec2dArray points;

	auto cvc_attr = geometry.CreateCurveVertexCountsAttr();
	pxr::VtIntArray cvc;

	for (int g = 0; g < ml->ngeoms; g++)
	{
		LWLINE *geom = ml->geoms[g];
		POINTARRAY *pa = geom->points;
		for (int i = 0; i < pa->npoints; i++)
		{
			POINT4D pt = {0};
			getPoint4d_p(pa, i, &pt);
			PushVtArray(points, pxr::GfVec2d(pt.x, pt.y));
		}
	}

	points_attr.Set(points);
	cvc_attr.Set(cvc);
}

void
Writer::WritePolygon(const LWPOLY *p)
{
	auto prim_path = GenerateNextSdfPath(m_stage, GEOMETRY_PRIM_PATH);
	auto geometry = pxr::UsdGeomMesh::Define(m_stage, prim_path);

	auto fvi_attr = geometry.GetFaceVertexIndicesAttr();
	pxr::VtIntArray fvi;

	auto fvc_attr = geometry.CreateFaceVertexCountsAttr();
	pxr::VtIntArray fvc;

	auto points_attr = geometry.CreatePointsAttr();
	pxr::VtVec2dArray points;

	POINTARRAY *boundary_ring = p->rings[0];
	fvc.push_back(boundary_ring->npoints);
	for (int i = 0; i < boundary_ring->npoints; i++)
	{
		POINT4D pt = {0};
		getPoint4d_p(boundary_ring, i, &pt);
		PushVtArray(points, pxr::GfVec2d(pt.x, pt.y));
	}

	fvi_attr.Set(fvi);
	fvc_attr.Set(fvc);
	points_attr.Set(points);
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
	auto prim_path = GenerateNextSdfPath(m_stage, GEOMETRY_PRIM_PATH);
	auto geometry = pxr::UsdGeomMesh::Define(m_stage, prim_path);

	auto fvi_attr = geometry.GetFaceVertexIndicesAttr();
	pxr::VtIntArray fvi;

	auto fvc_attr = geometry.CreateFaceVertexCountsAttr();
	pxr::VtIntArray fvc;

	auto points_attr = geometry.CreatePointsAttr();
	pxr::VtVec2dArray points;

	POINTARRAY *pa = t->points;
	for (int i = 0; i < pa->npoints; i += 4)
	{
		POINT4D ptx = {0};
		POINT4D pty = {0};
		POINT4D ptz = {0};
		getPoint4d_p(pa, i, &ptx);
		getPoint4d_p(pa, i + 1, &pty);
		getPoint4d_p(pa, i + 2, &ptz);

		PushVtArray(points, pxr::GfVec2d(ptx.x, ptx.y));
		PushVtArray(points, pxr::GfVec2d(pty.x, pty.y));
		PushVtArray(points, pxr::GfVec2d(ptz.x, ptz.y));
	}

	fvi_attr.Set(fvi);
	fvc_attr.Set(fvc);
	points_attr.Set(points);
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
		auto xform_world = pxr::UsdGeomXform::Define(m_stage, ROOT_PRIM_PATH);
		m_stage->SetDefaultPrim(xform_world.GetPrim());

		/* set postgis SRID as USD metadata */
		m_stage->SetMetadata(POSTGIS_SRID, geom->srid);

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
