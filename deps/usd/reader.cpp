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

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/fileFormat.h>
#include <pxr/usd/usd/usdaFileFormat.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/basisCurves.h>
#include <pxr/usd/usdGeom/points.h>

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

	/* todo: populate the PolyhydralSurface */
	LWCOLLECTION *coll = lwcollection_construct_empty(POLYHEDRALSURFACETYPE, SRID_UNKNOWN, 0, 0);

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
