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

#include "reader.h"
#include "tokens.h"

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/fileFormat.h>
#include <pxr/usd/usd/usdaFileFormat.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/basisCurves.h>
#include <pxr/usd/usdGeom/points.h>

#define READ_POSTGIS_ATTRIBUTES \
	int postgis_srid = SRID_UNKNOWN; \
	auto postgis_srid_attr = prim.GetAttribute(TOKEN_POSTGIS_SRID); \
	postgis_srid_attr.Get(&postgis_srid); \
	pxr::TfToken postgis_type_name; \
	auto postgis_type_name_attr = prim.GetAttribute(TOKEN_POSTGIS_TYPE_NAME); \
	postgis_type_name_attr.Get(&postgis_type_name); \
	int postgis_has_z = 0; \
	auto postgis_has_z_attr = prim.GetAttribute(TOKEN_POSTGIS_HAS_Z); \
	postgis_has_z_attr.Get(&postgis_has_z); \
	int postgis_has_m = 0; \
	auto postgis_has_m_attr = prim.GetAttribute(TOKEN_POSTGIS_HAS_M); \
	postgis_has_m_attr.Get(&postgis_has_m); \
	pxr::VtVec4dArray postgis_points; \
	auto postgis_points_attr = prim.GetAttribute(TOKEN_POSTGIS_POINTS); \
	postgis_points_attr.Get(&postgis_points);

using namespace USD;

Reader::Reader() {}

Reader::~Reader()
{
	for (LWGEOM *geom : m_geoms)
	{
		lwgeom_free(geom);
	}
	m_geoms.clear();
}

void
Reader::ScanPrim(pxr::UsdPrim prim)
{
	if (!prim.IsActive() || !prim.IsDefined())
	{
		return;
	}

	pxr::UsdGeomImageable imageable(prim);

	const auto &visibility_attr = imageable.GetVisibilityAttr();
	if (visibility_attr.IsValid())
	{
		pxr::TfToken visibility;
		visibility_attr.Get(&visibility);
		if (visibility == pxr::UsdGeomTokens->invisible)
		{
			return;
		}
	}

	const auto &purpose_attr = imageable.GetPurposeAttr();
	if (purpose_attr.IsValid())
	{
		pxr::TfToken purpose;
		purpose_attr.Get(&purpose);
		if (purpose == pxr::UsdGeomTokens->proxy || purpose == pxr::UsdGeomTokens->guide)
		{
			return;
		}
	}

	if (prim.IsA<pxr::UsdGeomBasisCurves>() || prim.IsA<pxr::UsdGeomMesh>() || prim.IsA<pxr::UsdGeomPoints>())
	{
		m_prims.push_back(prim);
	}

	/* visit children prims */
	for (auto child_prim : prim.GetAllChildren())
	{
		ScanPrim(prim);
	}
}

LWGEOM *
Reader::ReadMesh(pxr::UsdPrim prim)
{
	pxr::UsdGeomMesh mesh(prim);

	pxr::VtIntArray fvc;
	auto fvc_attr = mesh.GetFaceVertexCountsAttr();
	fvc_attr.Get(&fvc);

	pxr::VtIntArray fvi;
	auto fvi_attr = mesh.GetFaceVertexIndicesAttr();
	fvi_attr.Get(&fvi);

	pxr::VtVec3fArray points;
	auto points_attr = mesh.GetPointsAttr();
	points_attr.Get(&points);

	READ_POSTGIS_ATTRIBUTES

	/* read each face as LWPOLY and create LWCOLLECTION */
	std::vector<LWGEOM *> poly_geoms;
	if (postgis_points.size())
	{
		/* use postgis:points */
		for (size_t f = 0, offset = 0; f < fvc.size(); f++)
		{
			int nfv = fvc[f];
			POINT4D pts[nfv + 1];
			memset(pts, 0, sizeof(pts));
			for (int v = 0; v < nfv; v++)
			{
				const auto &point = postgis_points[offset + v];
				pts[v] = POINT4D{point[0], point[1], point[2], point[3]};
			}
			pts[nfv] = pts[0];
			POINTARRAY *pa = ptarray_construct(postgis_has_z, postgis_has_m, nfv + 1);
			ptarray_set_point4d(pa, nfv + 1, pts);

			LWPOLY *poly = lwpoly_construct(postgis_srid, nullptr, 1, &pa);
			poly_geoms.push_back(lwpoly_as_lwgeom(poly));

			offset += nfv + 1;
		}
	}
	else
	{
		/* use USD points, it always has z but no m */
		for (size_t f = 0, offset = 0; f < fvc.size(); f++)
		{
			int nfv = fvc[f];
			POINT4D pts[nfv + 1];
			memset(pts, 0, sizeof(pts));
			for (int v = 0; v < nfv; v++)
			{
				const auto &p = points[offset + v];
				pts[v] = POINT4D{p[0], p[1], p[2]};
			}
			pts[nfv] = pts[0];
			POINTARRAY *pa = ptarray_construct(1, 0, nfv + 1);
			ptarray_set_point4d(pa, nfv + 1, pts);

			LWPOLY *poly = lwpoly_construct(postgis_srid, nullptr, 1, &pa);
			poly_geoms.push_back(lwpoly_as_lwgeom(poly));

			offset += nfv;
		}
	}

	LWGEOM **poly_geoms_data = poly_geoms.data();
	LWCOLLECTION *coll =
	    lwcollection_construct(POLYHEDRALSURFACETYPE, postgis_srid, nullptr, poly_geoms.size(), poly_geoms_data);
	return lwcollection_as_lwgeom(coll);
}

void
Reader::ReadUSDA(const std::string &usda_string)
{
	auto layer = pxr::SdfLayer::CreateAnonymous();
	if (!layer->ImportFromString(usda_string))
	{
		return;
	}
	pxr::UsdStageRefPtr stage = pxr::UsdStage::Open(layer);

	/* iterate the USD stage to gather USD geometry */
	pxr::UsdPrim root_prim = stage->GetPseudoRoot();
	ScanPrim(root_prim);

	/* convert USD geometry to LWGEOM */
	for (auto prim : m_prims)
	{
		LWGEOM *geom = nullptr;
		if (prim.IsA<pxr::UsdGeomMesh>())
		{
			geom = ReadMesh(prim);
		}
		if (geom)
		{
			m_geoms.push_back(geom);
		}
	}
}

void
Reader::ReadUSDC(const char *data, size_t size)
{
	lwerror("usd: not implemented");
}
