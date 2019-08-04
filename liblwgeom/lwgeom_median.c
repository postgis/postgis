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
 * Copyright 2017 Darafei Praliaskouski <me@komzpa.net>
 *
 **********************************************************************/

#include <assert.h>
#include <float.h>

#include "liblwgeom_internal.h"
#include "lwgeom_log.h"

static double
calc_weighted_distances_3d(const POINT3D* curr, const POINT4D* points, uint32_t npoints, double* distances)
{
	uint32_t i;
	double weight = 0.0;
	for (i = 0; i < npoints; i++)
	{
		double dist = distance3d_pt_pt(curr, (POINT3D*)&points[i]);
		distances[i] =  dist / points[i].m;
		weight += dist * points[i].m;
	}

	return weight;
}

static uint32_t
iterate_4d(POINT3D* curr, const POINT4D* points, const uint32_t npoints, const uint32_t max_iter, const double tol)
{
	uint32_t i, iter;
	double delta;
	double sum_curr = 0, sum_next = 0;
	int hit = LW_FALSE;
	double *distances = lwalloc(npoints * sizeof(double));

	sum_curr = calc_weighted_distances_3d(curr, points, npoints, distances);

	for (iter = 0; iter < max_iter; iter++)
	{
		POINT3D next = { 0, 0, 0 };
		double denom = 0;

		/** Calculate denom to get the next point */
		for (i = 0; i < npoints; i++)
		{
			/* we need to use lower epsilon than in FP_IS_ZERO in the loop for calculation to converge */
			if (distances[i] > DBL_EPSILON)
			{
				next.x += points[i].x / distances[i];
				next.y += points[i].y / distances[i];
				next.z += points[i].z / distances[i];
				denom += 1.0 / distances[i];
			}
			else
			{
				hit = LW_TRUE;
			}
		}

		if (denom < DBL_EPSILON)
		{
			/* No movement - Final point */
			break;
		}

		/* Calculate the new point */
		next.x /= denom;
		next.y /= denom;
		next.z /= denom;

		/* If any of the intermediate points in the calculation is found in the
	 	* set of input points, the standard Weiszfeld method gets stuck with a
	 	* divide-by-zero.
	 	*
	 	* To get ourselves out of the hole, we follow an alternate procedure to
	 	* get the next iteration, as described in:
	 	*
	 	* Vardi, Y. and Zhang, C. (2011) "A modified Weiszfeld algorithm for the
	 	* Fermat-Weber location problem."  Math. Program., Ser. A 90: 559-566.
	 	* DOI 10.1007/s101070100222
	 	*
	 	* Available online at the time of this writing at
	 	* http://www.stat.rutgers.edu/home/cunhui/papers/43.pdf
	 	*/
		if (hit)
		{
			double dx = 0, dy = 0, dz = 0;
			double d_sqr;
			hit = LW_FALSE;

			for (i = 0; i < npoints; i++)
			{
				if (distances[i] > DBL_EPSILON)
				{
					dx += (points[i].x - curr->x) / distances[i];
					dy += (points[i].y - curr->y) / distances[i];
					dz += (points[i].z - curr->z) / distances[i];
				}
			}

			d_sqr = sqrt(dx*dx + dy*dy + dz*dz);
			if (d_sqr > DBL_EPSILON)
			{
				double r_inv = FP_MAX(0, 1.0 / d_sqr);
				next.x = (1.0 - r_inv)*next.x + r_inv*curr->x;
				next.y = (1.0 - r_inv)*next.y + r_inv*curr->y;
				next.z = (1.0 - r_inv)*next.z + r_inv*curr->z;
			}
		}

		/* Check movement with next point */
		sum_next = calc_weighted_distances_3d(&next, points, npoints, distances);
		delta = sum_curr - sum_next;
		if (delta < tol)
		{
			break;
		}
		else
		{
			curr->x = next.x;
			curr->y = next.y;
			curr->z = next.z;
			sum_curr = sum_next;
		}
	}

	lwfree(distances);
	return iter;
}

static POINT3D
init_guess(const POINT4D* points, uint32_t npoints)
{
	assert(npoints > 0);
	POINT3D guess = { 0, 0, 0 };
	double mass = 0;
	uint32_t i;
	for (i = 0; i < npoints; i++)
	{
		guess.x += points[i].x * points[i].m;
		guess.y += points[i].y * points[i].m;
		guess.z += points[i].z * points[i].m;
		mass += points[i].m;
	}
	guess.x /= mass;
	guess.y /= mass;
	guess.z /= mass;
	return guess;
}

POINT4D*
lwmpoint_extract_points_4d(const LWMPOINT* g, uint32_t* npoints, int* input_empty)
{
	uint32_t i;
	uint32_t n = 0;
	POINT4D* points = lwalloc(g->ngeoms * sizeof(POINT4D));
	int has_m = lwgeom_has_m((LWGEOM*) g);

	for (i = 0; i < g->ngeoms; i++)
	{
		LWGEOM* subg = lwcollection_getsubgeom((LWCOLLECTION*) g, i);
		if (!lwgeom_is_empty(subg))
		{
			*input_empty = LW_FALSE;
			if (!getPoint4d_p(((LWPOINT*) subg)->point, 0, &points[n]))
			{
				lwerror("Geometric median: getPoint4d_p reported failure on point (POINT(%g %g %g %g), number %d of %d in input).", points[n].x, points[n].y, points[n].z, points[n].m, i, g->ngeoms);
				lwfree(points);
				return NULL;
			}
			if (has_m)
			{
				/* This limitation on non-negativity can be lifted
				 * by replacing Weiszfeld algorithm with different one.
				 * Possible option described in:
				 *
				 * Drezner, Zvi & O. Wesolowsky, George. (1991).
				 * The Weber Problem On The Plane With Some Negative Weights.
				 * INFOR. Information Systems and Operational Research.
				 * 29. 10.1080/03155986.1991.11732158.
				 */
				if (points[n].m < 0)
				{
					lwerror("Geometric median input contains points with negative weights (POINT(%g %g %g %g), number %d of %d in input). Implementation can't guarantee global minimum convergence.", points[n].x, points[n].y, points[n].z, points[n].m, i, g->ngeoms);
					lwfree(points);
					return NULL;
				}

				/* points with zero weight are not going to affect calculation, drop them early */
				if (points[n].m > DBL_EPSILON) n++;
			}
			else
			{
				points[n].m = 1.0;
				n++;
			}
		}
	}

#if PARANOIA_LEVEL > 0
	/* check Z=0 for 2D inputs*/
	if (!lwgeom_has_z((LWGEOM*) g)) for (i = 0; i < n; i++) assert(points[i].z == 0);
#endif

	*npoints = n;
	return points;
}


LWPOINT*
lwmpoint_median(const LWMPOINT* g, double tol, uint32_t max_iter, char fail_if_not_converged)
{
	/* m ordinate is considered weight, if defined */
	uint32_t npoints = 0; /* we need to count this ourselves so we can exclude empties and weightless points */
	uint32_t i;
	int input_empty = LW_TRUE;
	POINT3D median;
	POINT4D* points = lwmpoint_extract_points_4d(g, &npoints, &input_empty);

	/* input validation failed, error reported already */
	if (points == NULL) return NULL;

	if (npoints == 0)
	{
		lwfree(points);
		if (input_empty)
		{
			return lwpoint_construct_empty(g->srid, 0, 0);
		}
		else
		{
			lwerror("Median failed to find non-empty input points with positive weight.");
			return NULL;
		}
	}

	median = init_guess(points, npoints);

	i = iterate_4d(&median, points, npoints, max_iter, tol);

	lwfree(points);

	if (fail_if_not_converged && i >= max_iter)
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
lwgeom_median(const LWGEOM* g, double tol, uint32_t max_iter, char fail_if_not_converged)
{
	switch (g->type)
	{
		case POINTTYPE:
			return lwpoint_clone(lwgeom_as_lwpoint(g));
		case MULTIPOINTTYPE:
			return lwmpoint_median(lwgeom_as_lwmpoint(g), tol, max_iter, fail_if_not_converged);
		default:
			lwerror("%s: Unsupported geometry type: %s", __func__, lwtype_name(g->type));
			return NULL;
	}
}

