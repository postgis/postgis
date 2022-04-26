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
 * Copyright 2019 Raúl Marín
 *
 **********************************************************************/

#include "lwgeom_wagyu.h"

#define USE_WAGYU_INTERRUPT
#include "mapbox/geometry/wagyu/wagyu.hpp"

#include <iterator>
#include <limits>
#include <vector>

using namespace mapbox::geometry;

using wagyu_coord_type = std::int32_t;
using wagyu_polygon = mapbox::geometry::polygon<wagyu_coord_type>;
using wagyu_multipolygon = mapbox::geometry::multi_polygon<wagyu_coord_type>;
using wagyu_linearring = mapbox::geometry::linear_ring<wagyu_coord_type>;
using vwpolygon = std::vector<wagyu_polygon>;
using wagyu_point = mapbox::geometry::point<wagyu_coord_type>;

static wagyu_linearring
ptarray_to_wglinearring(const POINTARRAY *pa)
{
	wagyu_linearring lr;
	lr.reserve(pa->npoints);

	size_t point_size = ptarray_point_size(pa);
	size_t pa_size = pa->npoints;
	uint8_t *buffer = pa->serialized_pointlist;
	for (std::uint32_t i = 0; i < pa_size; i++)
	{
		const wagyu_coord_type x = static_cast<wagyu_coord_type>(*(double*) buffer);
		const wagyu_coord_type y = static_cast<wagyu_coord_type>(((double*) buffer)[1]);
		buffer += point_size;

		lr.emplace_back(static_cast<wagyu_coord_type>(x), static_cast<wagyu_coord_type>(y));
	}

	return lr;
}

static vwpolygon
lwpoly_to_vwgpoly(const LWPOLY *geom, const GBOX *box)
{
	LWCOLLECTION *geom_clipped_x = lwgeom_clip_to_ordinate_range((LWGEOM *)geom, 'X', box->xmin, box->xmax, 0);
	LWCOLLECTION *geom_clipped = lwgeom_clip_to_ordinate_range((LWGEOM *)geom_clipped_x, 'Y', box->ymin, box->ymax, 0);

	vwpolygon vp;
	for (std::uint32_t i = 0; i < geom_clipped->ngeoms; i++)
	{
		assert(geom_clipped->geoms[i]->type == POLYGONTYPE);
		LWPOLY *poly = (LWPOLY *)geom_clipped->geoms[i];
		for (std::uint32_t ring = 0; ring < poly->nrings; ring++)
		{
			wagyu_polygon pol;
			pol.push_back(ptarray_to_wglinearring(poly->rings[ring]));

			/* If it has an inner ring, push it too */
			ring++;
			if (ring != poly->nrings)
			{
				pol.push_back(ptarray_to_wglinearring(poly->rings[ring]));
			}

			vp.push_back(pol);
		}
	}

	lwgeom_free((LWGEOM *)geom_clipped_x);
	lwgeom_free((LWGEOM *)geom_clipped);

	return vp;
}

static vwpolygon
lwmpoly_to_vwgpoly(const LWMPOLY *geom, const GBOX *box)
{
	vwpolygon vp;
	for (std::uint32_t i = 0; i < geom->ngeoms; i++)
	{
		vwpolygon vp_intermediate = lwpoly_to_vwgpoly(geom->geoms[i], box);
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
lwgeom_to_vwgpoly(const LWGEOM *geom, const GBOX *box)
{
	switch (geom->type)
	{
	case POLYGONTYPE:
		return lwpoly_to_vwgpoly(reinterpret_cast<const LWPOLY *>(geom), box);
	case MULTIPOLYGONTYPE:
		return lwmpoly_to_vwgpoly(reinterpret_cast<const LWMPOLY *>(geom), box);
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
	if (ngeoms == 0)
	{
		return NULL;
	}
	else if (ngeoms == 1)
	{
		return wgpoly_to_lwgeom(mp[0]);
	}
	else
	{
		LWGEOM **geoms = reinterpret_cast<LWGEOM **>(lwalloc(sizeof(LWGEOM *) * ngeoms));

		for (uint32_t i = 0; i < ngeoms; i++)
		{
			geoms[i] = wgpoly_to_lwgeom(mp[i]);
		}

		return reinterpret_cast<LWGEOM *>(lwcollection_construct(MULTIPOLYGONTYPE, 0, NULL, ngeoms, geoms));
	}
}

static LWGEOM *
__lwgeom_wagyu_clip_by_box(const LWGEOM *geom, const GBOX *bbox)
{
	if (!geom || !bbox)
	{
		return NULL;
	}

	if (geom->type != POLYGONTYPE && geom->type != MULTIPOLYGONTYPE)
	{
		return NULL;
	}

	if (lwgeom_is_empty(geom))
	{
		LWGEOM *out = lwgeom_construct_empty(MULTIPOLYGONTYPE, geom->srid, 0, 0);
		out->flags = geom->flags;
		return out;
	}

	wagyu_multipolygon solution;
	vwpolygon vpsubject = lwgeom_to_vwgpoly(geom, bbox);
	if (vpsubject.size() == 0)
	{
		LWGEOM *out = lwgeom_construct_empty(MULTIPOLYGONTYPE, geom->srid, 0, 0);
		out->flags = geom->flags;
		return out;
	}

	wagyu::wagyu<wagyu_coord_type> clipper;
	for (auto &poly : vpsubject)
	{
		for (auto &lr : poly)
		{
			if (!lr.empty())
			{
				clipper.add_ring(lr, wagyu::polygon_type_subject);
			}
		}
	}

	clipper.execute(wagyu::clip_type_union, solution, wagyu::fill_type_even_odd, wagyu::fill_type_even_odd);

	LWGEOM *g = wgmpoly_to_lwgeom(solution);
	if (g)
		g->srid = geom->srid;

	return g;
}

LWGEOM *
lwgeom_wagyu_clip_by_box(const LWGEOM *geom, const GBOX *bbox)
{
	mapbox::geometry::wagyu::interrupt_reset();
	try
	{
		return __lwgeom_wagyu_clip_by_box(geom, bbox);
	}
	catch (...)
	{
		return NULL;
	}
}

const char *
libwagyu_version()
{
	static char str[50] = {0};
	snprintf(
		str, sizeof(str), "%d.%d.%d (Internal)", WAGYU_MAJOR_VERSION, WAGYU_MINOR_VERSION, WAGYU_PATCH_VERSION);

	return str;
}

void
lwgeom_wagyu_interruptRequest()
{
	mapbox::geometry::wagyu::interrupt_request();
}

void
lwgeom_wagyu_interruptReset()
{
    mapbox::geometry::wagyu::interrupt_reset();
}
