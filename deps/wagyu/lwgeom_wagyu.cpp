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

#include <vector>

using namespace mapbox::geometry;

using wagyu_polygon = mapbox::geometry::polygon<double>;
using wagyu_multipolygon = mapbox::geometry::multi_polygon<double>;
using vpolygon = std::vector<wagyu_polygon>;

/**
 * Transforms a liblwgeom geometry (types POLYGONTYPE or MULTIPOLYGONTYPE)
 * into an array of mapbox::geometry (polygon)
 * A single lwgeom polygon can lead to N mapbox polygon as odd numbered rings
 * are treated as new polygons (and even numbered treated as holes in the
 * previous rings)
 */
static vpolygon
lwgeom2wagyu(const LWGEOM *geom)
{
	std::vector<wagyu_polygon> v;
	/* TODO: Implement */
	return v;
}

static LWGEOM *
wagyu2lwgeom(wagyu_multipolygon &mp)
{
	LWGEOM *g = NULL;
	/* TODO: Implement */
	return g;
}

LWGEOM *
lwgeom_wagyu_clip_by_polygon(const LWGEOM *geom, const LWGEOM *clip)
{
	if (!geom || !clip)
		return NULL;

	if (geom->type != POLYGONTYPE && geom->type != MULTIPOLYGONTYPE)
	{
		lwnotice("%s: Input geometry must be of polygon type");
		return NULL;
	}

	if (clip->type != POLYGONTYPE && clip->type != MULTIPOLYGONTYPE)
	{
		lwnotice("%s: Clipping geometry must be of polygon type");
		return NULL;
	}

	if (lwgeom_is_empty(geom) || (lwgeom_is_empty(clip)))
	{
		LWGEOM *out = lwgeom_construct_empty(MULTIPOLYGONTYPE, geom->srid, 0, 0);
		out->flags = geom->flags;
	}

	wagyu::wagyu<double> clipper;

	vpolygon psubject = lwgeom2wagyu(geom);
	vpolygon pclip = lwgeom2wagyu(clip);

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
	if (!clipper.execute(wagyu::clip_type_union, solution, wagyu::fill_type_even_odd, wagyu::fill_type_even_odd))
	{
		lwdebug(1, "%s: Clipping failed");
		return NULL;
	}

	return wagyu2lwgeom(solution);
}
