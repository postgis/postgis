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
#include "mapbox/geometry/wagyu/quick_clip.hpp"

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
using wagyu_box = mapbox::geometry::box<wagyu_coord_type>;

static wagyu_linearring
ptarray_to_wglinearring(const POINTARRAY *pa, const wagyu_box &tile)
{
	wagyu_linearring lr;
	lr.reserve(pa->npoints);

	/* We calculate the bounding box of the point array */
	wagyu_coord_type min_x = std::numeric_limits<wagyu_coord_type>::max();
	wagyu_coord_type max_x = std::numeric_limits<wagyu_coord_type>::min();
	wagyu_coord_type min_y = min_x;
	wagyu_coord_type max_y = max_x;

	size_t point_size = ptarray_point_size(pa);
	size_t pa_size = pa->npoints;
	uint8_t *buffer = pa->serialized_pointlist;
	for (std::uint32_t i = 0; i < pa_size; i++)
	{
		const wagyu_coord_type x = static_cast<wagyu_coord_type>(*(double*) buffer);
		const wagyu_coord_type y = static_cast<wagyu_coord_type>(((double*) buffer)[1]);
		buffer += point_size;
		min_x = std::min(min_x, x);
		max_x = std::max(max_x, x);
		min_y = std::min(min_y, y);
		max_y = std::max(max_y, y);

		lr.emplace_back(static_cast<wagyu_coord_type>(x), static_cast<wagyu_coord_type>(y));
	}

	/* Check how many sides of the calculated box are inside the tile */
	uint32_t sides_in = 0;
	sides_in += min_x >= tile.min.x && min_x <= tile.max.x;
	sides_in += max_x >= tile.min.x && max_x <= tile.max.x;
	sides_in += min_y >= tile.min.y && min_y <= tile.max.y;
	sides_in += max_y >= tile.min.y && max_y <= tile.max.y;

	/* With 0 sides in, the box it's either outside or covers the tile completely */
	if ((sides_in == 0) && (min_x > tile.max.x || max_x < tile.min.x || min_y > tile.max.y || max_y < tile.min.y))
	{
		/* No overlapping: Return an empty linearring */
		return wagyu_linearring();
	}

	if (sides_in != 4)
	{
		/* Some edges need to be clipped */
		return mapbox::geometry::wagyu::quick_clip::quick_lr_clip(lr, tile);
	}

	/* All points are inside the box */
	return lr;
}

static vwpolygon
lwpoly_to_vwgpoly(const LWPOLY *geom, const wagyu_box &box)
{
	vwpolygon vp;
	for (std::uint32_t i = 0; i < geom->nrings; i++)
	{
		wagyu_polygon pol;
		pol.push_back(ptarray_to_wglinearring(geom->rings[i], box));

		/* If it has an inner ring, push it too */
		i++;
		if (i != geom->nrings)
		{
			pol.push_back(ptarray_to_wglinearring(geom->rings[i], box));
		}

		vp.push_back(pol);
	}

	return vp;
}

static vwpolygon
lwmpoly_to_vwgpoly(const LWMPOLY *geom, const wagyu_box &box)
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
lwgeom_to_vwgpoly(const LWGEOM *geom, const wagyu_box &box)
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

	wagyu_point box_min(std::min(bbox->xmin, bbox->xmax), std::min(bbox->ymin, bbox->ymax));
	wagyu_point box_max(std::max(bbox->xmin, bbox->xmax), std::max(bbox->ymin, bbox->ymax));
	const wagyu_box box(box_min, box_max);

	wagyu_multipolygon solution;
	vwpolygon vpsubject = lwgeom_to_vwgpoly(geom, box);
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
