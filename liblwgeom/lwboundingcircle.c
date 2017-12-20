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


#include <string.h>
#include "liblwgeom_internal.h"

typedef struct {
	const POINT2D* p1;
	const POINT2D* p2;
	const POINT2D* p3;
} SUPPORTING_POINTS;

static SUPPORTING_POINTS*
supporting_points_create()
{
	SUPPORTING_POINTS* s = lwalloc(sizeof(SUPPORTING_POINTS));
	s->p1 = NULL;
	s->p2 = NULL;
	s->p3 = NULL;

	return s;
}

static void
supporting_points_destroy(SUPPORTING_POINTS* s)
{
	lwfree(s);
}

static uint32_t
num_supporting_points(SUPPORTING_POINTS* support)
{
	uint32_t N = 0;

	if (support->p1 != NULL)
		N++;
	if (support->p2 != NULL)
		N++;
	if (support->p3 != NULL)
		N++;

	return N;
}

static int
add_supporting_point(SUPPORTING_POINTS* support, const POINT2D* p)
{
	switch(num_supporting_points(support))
	{
		case 0: support->p1 = p;
				break;
		case 1: support->p2 = p;
				break;
		case 2: support->p3 = p;
				break;
		default: return LW_FAILURE;
	}

	return LW_SUCCESS;
}

static int
point_inside_circle(const POINT2D* p, const LWBOUNDINGCIRCLE* c)
{
	if (!c)
		return LW_FALSE;

	if (distance2d_pt_pt(p, c->center) - c->radius > DBL_EPSILON)
		return LW_FALSE;

	return LW_TRUE;
}

static double
det(double m00, double m01, double m10, double m11)
{
	return m00 * m11 - m01 * m10;
}

static void
circumcenter(const POINT2D* a, const POINT2D* b, const POINT2D* c, POINT2D* result)
{
	double cx = c->x;
	double cy = c->y;
	double ax = a->x - cx;
	double ay = a->y - cy;
	double bx = b->x - cx;
	double by = b->y - cy;

	double denom = 2 * det(ax, ay, bx, by);
	double numx = det(ay, ax * ax + ay * ay, by, bx * bx + by * by);
	double numy = det(ax, ax * ax + ay * ay, bx, bx * bx + by * by);

	result->x = cx - numx / denom;
	result->y = cy + numy / denom;
}

static void
calculate_mbc_1(const SUPPORTING_POINTS* support, LWBOUNDINGCIRCLE* mbc)
{
	mbc->radius = 0;
	mbc->center->x = support->p1->x;
	mbc->center->y = support->p1->y;
}

static void
calculate_mbc_2(const SUPPORTING_POINTS* support, LWBOUNDINGCIRCLE* mbc)
{
	double d1, d2;

	mbc->center->x = 0.5*(support->p1->x + support->p2->x);
	mbc->center->y = 0.5*(support->p1->y + support->p2->y);

	d1 = distance2d_pt_pt(mbc->center, support->p1);
	d2 = distance2d_pt_pt(mbc->center, support->p2);

	mbc->radius = FP_MAX(d1, d2);
}

static void
calculate_mbc_3(const SUPPORTING_POINTS* support, LWBOUNDINGCIRCLE* mbc)
{
	double d1, d2, d3;
	circumcenter(support->p1, support->p2, support->p3, mbc->center);

	d1 = distance2d_pt_pt(mbc->center, support->p1);
	d2 = distance2d_pt_pt(mbc->center, support->p2);
	d3 = distance2d_pt_pt(mbc->center, support->p3);

	mbc->radius = FP_MAX(FP_MAX(d1, d2), d3);
}

static int
calculate_mbc_from_support(SUPPORTING_POINTS* support, LWBOUNDINGCIRCLE* mbc)
{
	switch(num_supporting_points(support))
	{
		case 0: break;
		case 1: calculate_mbc_1(support, mbc);
				break;
		case 2: calculate_mbc_2(support, mbc);
				break;
		case 3: calculate_mbc_3(support, mbc);
				break;
		default: return LW_FAILURE;
	}

	return LW_SUCCESS;
}

static int
calculate_mbc(const POINT2D** points, uint32_t max_n, SUPPORTING_POINTS* support, LWBOUNDINGCIRCLE* mbc)
{
	uint32_t i;

	if(!calculate_mbc_from_support(support, mbc))
	{
		return LW_FAILURE;
	}

	if (num_supporting_points(support) == 3)
	{
		/* If we're entering the function with three supporting points already, our circle
		 * is already fully constrained - we couldn't add another supporting point if we
		 * needed to. So, there's no point in proceeding further.  Welzl (1991) provides
		 * a much better explanation of why this works.
		 * */
		return LW_SUCCESS;
	}

	for (i = 0; i < max_n; i++)
	{
		if (!point_inside_circle(points[i], mbc))
		{
			/* We've run into a point that isn't inside our circle.  To fix this, we'll
			 * go back in time, and re-run the algorithm for each point we've seen so
			 * far, with the constraint that the current point must be on the boundary
			 * of the circle.  Then, we'll continue on in this loop with the modified
			 * circle that by definition includes the current point. */
			SUPPORTING_POINTS next_support;
			memcpy(&next_support, support, sizeof(SUPPORTING_POINTS));

			add_supporting_point(&next_support, points[i]);
			if (!calculate_mbc(points, i, &next_support, mbc))
			{
				return LW_FAILURE;
			}
		}
	}

	return LW_SUCCESS;
}

static LWBOUNDINGCIRCLE*
lwboundingcircle_create()
{
	LWBOUNDINGCIRCLE* c = lwalloc(sizeof(LWBOUNDINGCIRCLE));
	c->center = lwalloc(sizeof(POINT2D));

	c->radius = 0.0;
	c->center->x = 0.0;
	c->center->y = 0.0;

	return c;
}

void
lwboundingcircle_destroy(LWBOUNDINGCIRCLE* c)
{
	lwfree(c->center);
	lwfree(c);
}

LWBOUNDINGCIRCLE*
lwgeom_calculate_mbc(const LWGEOM* g)
{
	SUPPORTING_POINTS* support;
	LWBOUNDINGCIRCLE* result;
	LWPOINTITERATOR* it;
	uint32_t num_points;
	POINT2D** points;
	POINT4D p;
	uint32_t i;
	int success;

	if(g == NULL || lwgeom_is_empty(g))
		return LW_FAILURE;

	num_points = lwgeom_count_vertices(g);
	it = lwpointiterator_create(g);
	points = lwalloc(num_points * sizeof(POINT2D*));
	for (i = 0; i < num_points; i++)
	{
		if(!lwpointiterator_next(it, &p))
		{
			uint32_t j;
			for (j = 0; j < i; j++)
			{
				lwfree(points[j]);
			}
			lwpointiterator_destroy(it);
			lwfree(points);
			return LW_FAILURE;
		}

		points[i] = lwalloc(sizeof(POINT2D));
		points[i]->x = p.x;
		points[i]->y = p.y;
	}
	lwpointiterator_destroy(it);

	support = supporting_points_create();
	result = lwboundingcircle_create();
	/* Technically, a randomized algorithm would demand that we shuffle the input points
	 * before we call calculate_mbc().  However, we make the (perhaps poor) assumption that
	 * the order we happen to find the points is as good as random, or close enough.
	 * */
	success = calculate_mbc((const POINT2D**) points, num_points, support, result);

	for (i = 0; i < num_points; i++)
	{
		lwfree(points[i]);
	}
	lwfree(points);
	supporting_points_destroy(support);

	if (!success)
		return NULL;

	return result;
}
