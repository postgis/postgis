/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright 2015 Daniel Baston <dbaston@gmail.com>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <string.h>
#include "liblwgeom.h"
#include "liblwgeom_internal.h"
#include "lwiterator.h"

typedef struct {
	POINT2D centre;
	double radius;
} CIRCLE;

typedef struct {
	const POINT2D* p1;
	const POINT2D* p2;
	const POINT2D* p3;
} SUPPORTING_POINTS;

static uint32_t num_supporting_points(SUPPORTING_POINTS* support)
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

static void clone_supporting_points(SUPPORTING_POINTS* from, SUPPORTING_POINTS* to)
{
	memcpy(to, from, sizeof(SUPPORTING_POINTS));
}

static int add_supporting_point(SUPPORTING_POINTS* support, const POINT2D* p)
{
	switch(num_supporting_points(support))
	{
		case 0: support->p1 = p;
				break;
		case 1: support->p2 = p;
				break;
		case 2: support->p3 = p;
				break;
        default: printf("CANT DO THAT (%d)\n", num_supporting_points(support));
	}


	// TODO fail when N is too high
	return LW_SUCCESS;
}

static int point_inside_circle(const POINT2D* p, const CIRCLE* c)
{
	if (!c)
		return LW_FALSE;

	if (distance2d_pt_pt(p, &(c->centre)) > c->radius)
		return LW_FALSE;

	return LW_TRUE;
}

/* Copied from JTS, Triangle.java */
static double det(double m00, double m01, double m10, double m11)
{
  return m00 * m11 - m01 * m10;
}

/* Adapted from JTS, Triangle.java */
static void circumcentre(const POINT2D* a, const POINT2D* b, const POINT2D* c, POINT2D* result)
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

static void calculate_mbc_1(const SUPPORTING_POINTS* support, CIRCLE* mbc)
{
	mbc->radius = 0;
	(mbc->centre).x = support->p1->x;
	(mbc->centre).y = support->p1->y;
}

static void calculate_mbc_2(const SUPPORTING_POINTS* support, CIRCLE* mbc)
{
	mbc->centre.x = 0.5*(support->p1->x + support->p2->x);
	mbc->centre.y = 0.5*(support->p1->y + support->p2->y);
	mbc->radius = FP_MAX(distance2d_pt_pt(&(mbc->centre), support->p1),
						 distance2d_pt_pt(&(mbc->centre), support->p2));
}

static void calculate_mbc_3(const SUPPORTING_POINTS* support, CIRCLE* mbc)
{
    circumcentre(support->p1, support->p2, support->p3, &(mbc->centre));
    mbc->radius = FP_MAX(FP_MAX(distance2d_pt_pt(&(mbc->centre), support->p1), distance2d_pt_pt(&(mbc->centre), support->p2)), distance2d_pt_pt(&(mbc->centre), support->p3));

    printf("radius=%f\n", mbc->radius);
}

static CIRCLE calculate_mbc_from_support(SUPPORTING_POINTS* support) {
    printf("calculate_mbc %d\n", num_supporting_points(support));
	CIRCLE mbc;
	memset(&mbc, 0, sizeof(CIRCLE));

	switch(num_supporting_points(support))
	{
        case 0: break;
		case 1: calculate_mbc_1(support, &mbc);
                break;
		case 2: calculate_mbc_2(support, &mbc);
                break;
		case 3: calculate_mbc_3(support, &mbc);
                break;
        default: printf("BAD BAD BAD\n");
	}

	printf("%f %f %f\n", mbc.centre.x, mbc.centre.y, mbc.radius);
	return mbc;
}

static CIRCLE calculate_mbc(const POINT2D** points, uint32_t max_n, SUPPORTING_POINTS* support)
{
	CIRCLE mbc;
	uint32_t i;

    // TODO can I check if support is NULL and create an empty one here, rather than making caller do it?

	mbc = calculate_mbc_from_support(support);

    if (num_supporting_points(support) == 3)
    {
        return mbc;
    }

	for (i = 0; i < max_n; i++)
	{
		if (!point_inside_circle(points[i], &mbc))
		{
			SUPPORTING_POINTS next_support;
			clone_supporting_points(support, &next_support);

			add_supporting_point(&next_support, points[i]);
			mbc = calculate_mbc(points, i, &next_support);
		}
	}

	return mbc;
}

// TODO expose this fn more generally somewhere?
static int extract_points_2d(const LWGEOM* g, POINT2D*** points, uint32_t* num_points)
{
    uint32_t i = 0;
    LWITERATOR it;
    POINT4D p;
   
    *num_points = lwgeom_count_vertices(g);
    *points = malloc(*num_points * sizeof(POINT2D*));

    lwiterator_create(g, &it);
	while (lwiterator_has_next(&it))
	{
        if (!lwiterator_next(&it, &p))
        {
            uint32_t j;
            for (j = 0; j < i; j++)
            {
               lwfree(points[j]); 
            }
            lwfree(points);
            lwiterator_destroy(&it);
            return LW_FAILURE;
        }

        (*points)[i] = malloc(sizeof(POINT2D));
        ((*points)[i])->x = p.x;
        ((*points)[i])->y = p.y;

        i++;
	}

    lwiterator_destroy(&it);
    printf("done N=%d\n", *num_points);
    return LW_SUCCESS;
}

int lwgeom_calculate_mbc(const LWGEOM* g, CIRCLE* result)
{
    POINT2D** points;
    uint32_t num_points;
    uint32_t i;
    if (!extract_points_2d(g, &points, &num_points))
        return LW_FAILURE;

    // TODO shuffle points?

    SUPPORTING_POINTS support;
    support.p1 = NULL;
    support.p2 = NULL;
    support.p3 = NULL;
    *result = calculate_mbc((const POINT2D**) points, num_points, &support);

    for (i = 0; i < num_points; i++)
    {
        lwfree(points[i]);
    }
    lwfree(points);
    
    //TODO return something real
    return LW_SUCCESS;
}
