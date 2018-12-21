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
 * Copyright 2018 Raúl Marín
 *
 **********************************************************************/

#include "lwgeom_wagyu.h"
#include "lwgeom_log.h"

#include "mapbox/geometry/wagyu/wagyu.hpp"

#include <iterator>
#include <vector>

using namespace mapbox::geometry;

/* TODO: May be std::int64_t if we do this after MVT coordinate conversion*/
using wagyu_coord_type = std::int64_t;
using wagyu_polygon = mapbox::geometry::polygon<wagyu_coord_type>;
using wagyu_multipolygon = mapbox::geometry::multi_polygon<wagyu_coord_type>;
using wagyu_linearring = mapbox::geometry::line_string<wagyu_coord_type>;
using vwpolygon = std::vector<wagyu_polygon>;

static wagyu_linearring
ptarray_to_wglinearring(const POINTARRAY *pa)
{
	wagyu_linearring lr;
	for (std::uint32_t i = 0; i < pa->npoints; i++)
	{
		const POINT2D *p2d = getPoint2d_cp(pa, i);
		lr.push_back({static_cast<wagyu_coord_type>(p2d->x), static_cast<wagyu_coord_type>(p2d->y)});
	}
	return lr;
}

static vwpolygon
lwpoly_to_vwgpoly(const LWPOLY *geom)
{
	vwpolygon vp;
	for (std::uint32_t i = 0; i < geom->nrings; i++)
	{
		wagyu_polygon pol;
		pol.push_back(ptarray_to_wglinearring(geom->rings[i]));

		/* If it has an inner ring, push it too */
		i++;
		if (i != geom->nrings)
		{
			pol.push_back(ptarray_to_wglinearring(geom->rings[i]));
		}

		vp.push_back(pol);
	}

	return vp;
}

static vwpolygon
lwmpoly_to_vwgpoly(const LWMPOLY *geom)
{
	vwpolygon vp;
	for (std::uint32_t i = 0; i < geom->ngeoms; i++)
	{
		vwpolygon vp_intermediate = lwpoly_to_vwgpoly(geom->geoms[i]);
		vp.insert(vp.end(),
			  std::make_move_iterator(vp_intermediate.begin()),
			  std::make_move_iterator(vp_intermediate.end()));
	}

	return vp;
}

/**
 * Transforms a liblwgeom geometry (types POLYGONTYPE or MULTIPOLYGONTYPE)
 * into an array of mapbox::geometry (polygon)
 * A single lwgeom polygon can lead to N mapbox polygon as odd numbered rings
 * are treated as new polygons (and even numbered treated as holes in the
 * previous rings)
 */
static vwpolygon
lwgeom_to_vwgpoly(const LWGEOM *geom)
{
	switch (geom->type)
	{
	case POLYGONTYPE:
		return lwpoly_to_vwgpoly(reinterpret_cast<const LWPOLY *>(geom));
	case MULTIPOLYGONTYPE:
		return lwmpoly_to_vwgpoly(reinterpret_cast<const LWMPOLY *>(geom));
	default:
		return vwpolygon();
	}
}

static POINTARRAY *
wglinearring_to_ptarray(const wagyu_polygon::linear_ring_type &lr)
{
	const uint32_t npoints = lr.size();
	POINTARRAY *pa = ptarray_construct(0, 0, npoints);

	for (uint32_t i = 0; i < npoints; i++)
	{
		const wagyu_polygon::linear_ring_type::point_type &p = lr[i];
		POINT4D point = {static_cast<double>(p.x), static_cast<double>(p.y), 0.0, 0.0};
		ptarray_set_point4d(pa, i, &point);
	}

	return pa;
}

static LWGEOM *
wgpoly_to_lwgeom(const wagyu_multipolygon::polygon_type &p)
{
	const uint32_t nrings = p.size();
	POINTARRAY **ppa = reinterpret_cast<POINTARRAY **>(lwalloc(sizeof(POINTARRAY *) * nrings));

	for (uint32_t i = 0; i < nrings; i++)
	{
		ppa[i] = wglinearring_to_ptarray(p[i]);
	}

	return reinterpret_cast<LWGEOM *>(lwpoly_construct(0, NULL, nrings, ppa));
}

static LWGEOM *
wgmpoly_to_lwgeom(const wagyu_multipolygon &mp)
{
	const uint32_t ngeoms = mp.size();
	LWGEOM **geoms = reinterpret_cast<LWGEOM **>(lwalloc(sizeof(LWGEOM *) * ngeoms));

	for (uint32_t i = 0; i < ngeoms; i++)
	{
		geoms[i] = wgpoly_to_lwgeom(mp[i]);
	}

	return reinterpret_cast<LWGEOM *>(lwcollection_construct(MULTIPOLYGONTYPE, 0, NULL, ngeoms, geoms));
}

LWGEOM *
lwgeom_wagyu_clip_by_polygon(const LWGEOM *geom, const LWGEOM *clip)
{
	if (!geom || !clip)
		return NULL;

	if (geom->type != POLYGONTYPE && geom->type != MULTIPOLYGONTYPE)
	{
		return NULL;
	}

	if (clip->type != POLYGONTYPE && clip->type != MULTIPOLYGONTYPE)
	{
		return NULL;
	}

	if (lwgeom_is_empty(geom) || (lwgeom_is_empty(clip)))
	{
		LWGEOM *out = lwgeom_construct_empty(MULTIPOLYGONTYPE, geom->srid, 0, 0);
		out->flags = geom->flags;
	}

	wagyu::wagyu<wagyu_coord_type> clipper;

	vwpolygon psubject = lwgeom_to_vwgpoly(geom);
	vwpolygon pclip = lwgeom_to_vwgpoly(clip);

	/* Any polygon from the source geometry is added as `polygon_type_subject` */
	for (auto &p : psubject)
	{
		clipper.add_polygon(p, wagyu::polygon_type::polygon_type_subject);
	}

	/* Any polygon from the clipping geometry is added as `polygon_type_clip` */
	for (auto &p : pclip)
	{
		clipper.add_polygon(p, wagyu::polygon_type::polygon_type_clip);
	}

	/* TODO: Check if we want to reverse the ouput here */
	wagyu_multipolygon solution;
	if (!clipper.execute(
		wagyu::clip_type_intersection, solution, wagyu::fill_type_even_odd, wagyu::fill_type_even_odd))
	{
		return NULL;
	}

	LWGEOM *g = wgmpoly_to_lwgeom(solution);
	g->srid = geom->srid;

	return g;
}

const char *
libwagyu_version()
{
	static char str[50] = {0};
	snprintf(
	    str, sizeof(str), "%d.%d.%d (Internal)", WAGYU_MAJOR_VERSION, WAGYU_MINOR_VERSION, WAGYU_PATCH_VERSION);

	return str;
}