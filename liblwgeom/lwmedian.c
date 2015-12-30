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
 * Copyright 2015 Daniel Baston <dbaston@gmail.com>
 *
 **********************************************************************/

#include <float.h>

#include "liblwgeom.h"
#include "lwgeom_log.h"

static void
calc_distances_3d(const POINT3D* curr, const POINT3D* points, uint32_t npoints, double* distances)
{
	uint32_t i;
	for (i = 0; i < npoints; i++)
	{
		distances[i] = distance3d_pt_pt(curr, &points[i]);
	}
}

static double
iterate_3d(POINT3D* curr, const POINT3D* points, uint32_t npoints, double* distances)
{
	uint32_t i;
	POINT3D next = { 0, 0, 0 };
	double delta;
	double denom = 0;

	calc_distances_3d(curr, points, npoints, distances);

	for (i = 0; i < npoints; i++)
	{
		denom += 1.0 / distances[i];
	}

	for (i = 0; i < npoints; i++)
	{
		next.x += (points[i].x / distances[i]) / denom;
		next.y += (points[i].y / distances[i]) / denom;
		next.z += (points[i].z / distances[i]) / denom;
	}

	delta = distance3d_pt_pt(curr, &next);

	curr->x = next.x;
	curr->y = next.y;
	curr->z = next.z;

	return delta;
}

static POINT3D
init_guess(const POINT3D* points, uint32_t npoints)
{
	POINT3D guess = { 0, 0, 0 };
	uint32_t i;
	for (i = 0; i < npoints; i++)
	{
		guess.x += points[i].x / npoints;
		guess.y += points[i].y / npoints;
		guess.z += points[i].z / npoints;
	}

	return guess;
}

static POINT3D*
lwmpoint_extract_points_3d(const LWMPOINT* g, uint32_t* ngeoms)
{
	uint32_t i;
	uint32_t n = 0;
	int is_3d = lwgeom_has_z((LWGEOM*) g);

	POINT3D* points = lwalloc(g->ngeoms * sizeof(POINT3D));
	for (i = 0; i < g->ngeoms; i++)
	{
		LWGEOM* subg = lwcollection_getsubgeom((LWCOLLECTION*) g, i);
		if (!lwgeom_is_empty(subg))
		{
			getPoint3dz_p(((LWPOINT*) subg)->point, 0, (POINT3DZ*) &points[n++]);
			if (!is_3d)
				points[n-1].z = 0.0; /* in case the getPoint functions return NaN in the future for 2d */
		}
	}

	if (ngeoms != NULL)
		*ngeoms = n;

	return points;
}

LWPOINT*
lwmpoint_median(const LWMPOINT* g, double tol, uint32_t max_iter)
{
	uint32_t npoints; /* we need to count this ourselves so we can exclude empties */
	uint32_t i;
	double delta = DBL_MAX;
	double* distances;
	POINT3D* points = lwmpoint_extract_points_3d(g, &npoints);
	POINT3D median;

	if (npoints == 0)
		return lwpoint_construct_empty(g->srid, 0, 0);

	median = init_guess(points, npoints);

	distances = lwalloc(npoints * sizeof(double));

	for (i = 0; i < max_iter && delta > tol; i++)
	{
		delta = iterate_3d(&median, points, npoints, distances);
	}

	lwfree(points);
	lwfree(distances);

	if (delta > tol)
	{
		lwerror("Median failed to converge within %g after %d iterations.", tol, max_iter);
		return NULL;
	}

	if (lwgeom_has_z((LWGEOM*) g))
	{
		return lwpoint_make3dz(g->srid, median.x, median.y, median.z);
	}
	else
	{
		return lwpoint_make2d(g->srid, median.x, median.y);
	}
}

LWPOINT*
lwgeom_median(const LWGEOM* g, double tol, uint32_t max_iter)
{
	switch( lwgeom_get_type(g) )
	{
		case POINTTYPE:
			return lwpoint_clone(lwgeom_as_lwpoint(g));
		case MULTIPOINTTYPE:
			return lwmpoint_median(lwgeom_as_lwmpoint(g), tol, max_iter);
		default:
			lwerror("Unsupported geometry type in lwgeom_median");
			return NULL;
	}
}

