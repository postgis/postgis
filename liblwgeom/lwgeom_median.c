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

static void
calc_weighted_distances_3d(const POINT3D* curr, const POINT4D* points, size_t npoints, double* distances)
{
	size_t i;
	for (i = 0; i < npoints; i++)
	{
		distances[i] = distance3d_pt_pt(curr, (POINT3D*)&points[i]) / points[i].m;
	}
}

static double
iterate_4d(POINT4D* curr, const POINT4D* points, size_t npoints, double* distances)
{
	size_t i;
	POINT4D next = { 0, 0, 0, 1};
	double delta = 0;
	double denom = 0;
	char hit = LW_FALSE;

	calc_weighted_distances_3d((POINT3D*)curr, points, npoints, distances);

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
	/* negative weight shouldn't get here */
	assert(denom >= 0);

	/* denom is zero in case of multipoint of single point when we've converged perfectly */
	if (denom > DBL_EPSILON)
	{
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
			double dx = 0;
			double dy = 0;
			double dz = 0;
			double r_inv;
			for (i = 0; i < npoints; i++)
			{
				if (distances[i] > DBL_EPSILON)
				{
					dx += (points[i].x - curr->x) / distances[i];
					dy += (points[i].y - curr->y) / distances[i];
					dz += (points[i].y - curr->z) / distances[i];
				}
			}

			r_inv = 1.0 / sqrt ( dx*dx + dy*dy + dz*dz );

			next.x = FP_MAX(0, 1.0 - r_inv)*next.x + FP_MIN(1.0, r_inv)*curr->x;
			next.y = FP_MAX(0, 1.0 - r_inv)*next.y + FP_MIN(1.0, r_inv)*curr->y;
			next.z = FP_MAX(0, 1.0 - r_inv)*next.z + FP_MIN(1.0, r_inv)*curr->z;
		}

		delta = distance3d_pt_pt((POINT3D*)curr, (POINT3D*)&next);

		curr->x = next.x;
		curr->y = next.y;
		curr->z = next.z;
	}

	return delta;
}

static POINT4D
init_guess(const POINT4D* points, size_t npoints)
{
	assert(npoints > 0);
	POINT4D guess = { 0, 0, 0, 0 };
	size_t i;
	for (i = 0; i < npoints; i++)
	{
		guess.x += points[i].x * points[i].m;
		guess.y += points[i].y * points[i].m;
		guess.z += points[i].z * points[i].m;
		guess.m += points[i].m;
	}
	guess.x /= guess.m;
	guess.y /= guess.m;
	guess.z /= guess.m;
	return guess;
}

static POINT4D*
lwmpoint_extract_points_4d(const LWMPOINT* g, size_t* npoints, int* input_ok)
{
	assert(npoints != NULL);
	assert(input_ok == LW_TRUE);
	size_t i;
	size_t n = 0;
	int is_3d = lwgeom_has_z((LWGEOM*) g);
	int has_m = lwgeom_has_m((LWGEOM*) g);

	POINT4D* points = lwalloc(g->ngeoms * sizeof(POINT4D));
	for (i = 0; i < g->ngeoms; i++)
	{
		LWGEOM* subg = lwcollection_getsubgeom((LWCOLLECTION*) g, i);
		if (!lwgeom_is_empty(subg))
		{
			getPoint4d_p(((LWPOINT*) subg)->point, 0, (POINT4D*) &points[n]);
			if (!is_3d)
			{
				points[n].z = 0.0; /* in case the getPoint functions return NaN in the future for 2d */
			}
			if (!has_m)
			{
				points[n].m = 1.0;
				n++;
			}
			else
			{
				/* points with zero weight are not going to affect calculation, drop them early */
				if (points[n].m > DBL_EPSILON)
				{
					n++;
				}

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
					lwerror("Geometric median input contains points with negative weights. Implementation can't guarantee global minimum convergence.");
					n = 0;
					input_ok = LW_FALSE;
					break;
				}				
			}
		}
	}

	*npoints = n;

	return points;
}


LWPOINT*
lwmpoint_median(const LWMPOINT* g, double tol, uint32_t max_iter, char fail_if_not_converged)
{
	size_t npoints = 0; /* we need to count this ourselves so we can exclude empties and weightless points */
	size_t i;
	int input_ok = LW_TRUE;
	double delta = DBL_MAX;
	double* distances;
	/* m ordinate is considered weight, if defined */
	POINT4D* points = lwmpoint_extract_points_4d(g, &npoints, &input_ok);
	POINT4D median;
	
	if (!input_ok)
	{
		/* error reported upon input check */
		return NULL;
	}

	if (npoints == 0)
	{
		lwfree(points);
		if (fail_if_not_converged)
		{
			lwerror("Median failed to find suitable input points.");
			
		}
		else
		{
			return lwpoint_construct_empty(g->srid, 0, 0);
		}
	}

	median = init_guess(points, npoints);

	distances = lwalloc(npoints * sizeof(double));

	for (i = 0; i < max_iter && delta > tol; i++)
	{
		delta = iterate_4d(&median, points, npoints, distances);
	}

	lwfree(points);
	lwfree(distances);

	if (fail_if_not_converged && delta > tol)
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
	switch( lwgeom_get_type(g) )
	{
		case POINTTYPE:
			return lwpoint_clone(lwgeom_as_lwpoint(g));
		case MULTIPOINTTYPE:
			return lwmpoint_median(lwgeom_as_lwmpoint(g), tol, max_iter, fail_if_not_converged);
		default:
			lwerror("Unsupported geometry type in lwgeom_median");
			return NULL;
	}
}

