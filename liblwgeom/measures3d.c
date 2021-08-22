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
 * Copyright 2011 Nicklas Avén
 * Copyright 2019 Darafei Praliaskouski
 *
 **********************************************************************/

#include <string.h>
#include <stdlib.h>

#include "measures3d.h"
#include "lwrandom.h"
#include "lwgeom_log.h"

static inline int
get_3dvector_from_points(POINT3DZ *p1, POINT3DZ *p2, VECTOR3D *v)
{
	v->x = p2->x - p1->x;
	v->y = p2->y - p1->y;
	v->z = p2->z - p1->z;

	return (!FP_IS_ZERO(v->x) || !FP_IS_ZERO(v->y) || !FP_IS_ZERO(v->z));
}

static inline int
get_3dcross_product(VECTOR3D *v1, VECTOR3D *v2, VECTOR3D *v)
{
	v->x = (v1->y * v2->z) - (v1->z * v2->y);
	v->y = (v1->z * v2->x) - (v1->x * v2->z);
	v->z = (v1->x * v2->y) - (v1->y * v2->x);

	return (!FP_IS_ZERO(v->x) || !FP_IS_ZERO(v->y) || !FP_IS_ZERO(v->z));
}

/**
This function is used to create a vertical line used for cases where one if the
geometries lacks z-values. The vertical line crosses the 2d point that is closest
and the z-range is from maxz to minz in the geometry that has z values.
*/
static LWGEOM *
create_v_line(const LWGEOM *lwgeom, double x, double y, int32_t srid)
{

	LWPOINT *lwpoints[2];
	GBOX gbox;
	int rv = lwgeom_calculate_gbox(lwgeom, &gbox);

	if (rv == LW_FAILURE)
		return NULL;

	lwpoints[0] = lwpoint_make3dz(srid, x, y, gbox.zmin);
	lwpoints[1] = lwpoint_make3dz(srid, x, y, gbox.zmax);

	return (LWGEOM *)lwline_from_ptarray(srid, 2, lwpoints);
}

LWGEOM *
lwgeom_closest_line_3d(const LWGEOM *lw1, const LWGEOM *lw2)
{
	return lw_dist3d_distanceline(lw1, lw2, lw1->srid, DIST_MIN);
}

LWGEOM *
lwgeom_furthest_line_3d(LWGEOM *lw1, LWGEOM *lw2)
{
	return lw_dist3d_distanceline(lw1, lw2, lw1->srid, DIST_MAX);
}

LWGEOM *
lwgeom_closest_point_3d(const LWGEOM *lw1, const LWGEOM *lw2)
{
	return lw_dist3d_distancepoint(lw1, lw2, lw1->srid, DIST_MIN);
}

/**
Function initializing 3dshortestline and 3dlongestline calculations.
*/
LWGEOM *
lw_dist3d_distanceline(const LWGEOM *lw1, const LWGEOM *lw2, int32_t srid, int mode)
{
	LWDEBUG(2, "lw_dist3d_distanceline is called");
	double x1, x2, y1, y2, z1, z2, x, y;
	double initdistance = (mode == DIST_MIN ? DBL_MAX : -1.0);
	DISTPTS3D thedl;
	LWPOINT *lwpoints[2];
	LWGEOM *result;

	thedl.mode = mode;
	thedl.distance = initdistance;
	thedl.tolerance = 0.0;

	/* Check if we really have 3D geometries
	 * If not, send it to 2D-calculations which will give the same result
	 * as an infinite z-value at one or two of the geometries */
	if (!lwgeom_has_z(lw1) || !lwgeom_has_z(lw2))
	{

		lwnotice(
		    "One or both of the geometries is missing z-value. The unknown z-value will be regarded as \"any value\"");

		if (!lwgeom_has_z(lw1) && !lwgeom_has_z(lw2))
			return lw_dist2d_distanceline(lw1, lw2, srid, mode);

		DISTPTS thedl2d;
		thedl2d.mode = mode;
		thedl2d.distance = initdistance;
		thedl2d.tolerance = 0.0;
		if (!lw_dist2d_comp(lw1, lw2, &thedl2d))
		{
			/* should never get here. all cases ought to be error handled earlier */
			lwerror("Some unspecified error.");
			result = (LWGEOM *)lwcollection_construct_empty(COLLECTIONTYPE, srid, 0, 0);
		}
		LWGEOM *vertical_line;
		if (!lwgeom_has_z(lw1))
		{
			x = thedl2d.p1.x;
			y = thedl2d.p1.y;

			vertical_line = create_v_line(lw2, x, y, srid);
			if (!lw_dist3d_recursive(vertical_line, lw2, &thedl))
			{
				/* should never get here. all cases ought to be error handled earlier */
				lwfree(vertical_line);
				lwerror("Some unspecified error.");
				result = (LWGEOM *)lwcollection_construct_empty(COLLECTIONTYPE, srid, 0, 0);
			}
			lwfree(vertical_line);
		}
		if (!lwgeom_has_z(lw2))
		{
			x = thedl2d.p2.x;
			y = thedl2d.p2.y;

			vertical_line = create_v_line(lw1, x, y, srid);
			if (!lw_dist3d_recursive(lw1, vertical_line, &thedl))
			{
				/* should never get here. all cases ought to be error handled earlier */
				lwfree(vertical_line);
				lwerror("Some unspecified error.");
				return (LWGEOM *)lwcollection_construct_empty(COLLECTIONTYPE, srid, 0, 0);
			}
			lwfree(vertical_line);
		}
	}
	else
	{
		if (!lw_dist3d_recursive(lw1, lw2, &thedl))
		{
			/* should never get here. all cases ought to be error handled earlier */
			lwerror("Some unspecified error.");
			result = (LWGEOM *)lwcollection_construct_empty(COLLECTIONTYPE, srid, 0, 0);
		}
	}
	/*if thedl.distance is unchanged there where only empty geometries input*/
	if (thedl.distance == initdistance)
	{
		LWDEBUG(3, "didn't find geometries to measure between, returning null");
		result = (LWGEOM *)lwcollection_construct_empty(COLLECTIONTYPE, srid, 0, 0);
	}
	else
	{
		x1 = thedl.p1.x;
		y1 = thedl.p1.y;
		z1 = thedl.p1.z;
		x2 = thedl.p2.x;
		y2 = thedl.p2.y;
		z2 = thedl.p2.z;

		lwpoints[0] = lwpoint_make3dz(srid, x1, y1, z1);
		lwpoints[1] = lwpoint_make3dz(srid, x2, y2, z2);

		result = (LWGEOM *)lwline_from_ptarray(srid, 2, lwpoints);
	}

	return result;
}

/**
Function initializing 3dclosestpoint calculations.
*/
LWGEOM *
lw_dist3d_distancepoint(const LWGEOM *lw1, const LWGEOM *lw2, int32_t srid, int mode)
{

	double x, y, z;
	DISTPTS3D thedl;
	double initdistance = DBL_MAX;
	LWGEOM *result;

	thedl.mode = mode;
	thedl.distance = initdistance;
	thedl.tolerance = 0;

	LWDEBUG(2, "lw_dist3d_distancepoint is called");

	/* Check if we really have 3D geometries
	 * If not, send it to 2D-calculations which will give the same result
	 * as an infinite z-value at one or two of the geometries
	 */
	if (!lwgeom_has_z(lw1) || !lwgeom_has_z(lw2))
	{
		lwnotice(
		    "One or both of the geometries is missing z-value. The unknown z-value will be regarded as \"any value\"");

		if (!lwgeom_has_z(lw1) && !lwgeom_has_z(lw2))
			return lw_dist2d_distancepoint(lw1, lw2, srid, mode);

		DISTPTS thedl2d;
		thedl2d.mode = mode;
		thedl2d.distance = initdistance;
		thedl2d.tolerance = 0.0;
		if (!lw_dist2d_comp(lw1, lw2, &thedl2d))
		{
			/* should never get here. all cases ought to be error handled earlier */
			lwerror("Some unspecified error.");
			return (LWGEOM *)lwcollection_construct_empty(COLLECTIONTYPE, srid, 0, 0);
		}

		LWGEOM *vertical_line;
		if (!lwgeom_has_z(lw1))
		{
			x = thedl2d.p1.x;
			y = thedl2d.p1.y;

			vertical_line = create_v_line(lw2, x, y, srid);
			if (!lw_dist3d_recursive(vertical_line, lw2, &thedl))
			{
				/* should never get here. all cases ought to be error handled earlier */
				lwfree(vertical_line);
				lwerror("Some unspecified error.");
				return (LWGEOM *)lwcollection_construct_empty(COLLECTIONTYPE, srid, 0, 0);
			}
			lwfree(vertical_line);
		}

		if (!lwgeom_has_z(lw2))
		{
			x = thedl2d.p2.x;
			y = thedl2d.p2.y;

			vertical_line = create_v_line(lw1, x, y, srid);
			if (!lw_dist3d_recursive(lw1, vertical_line, &thedl))
			{
				/* should never get here. all cases ought to be error handled earlier */
				lwfree(vertical_line);
				lwerror("Some unspecified error.");
				result = (LWGEOM *)lwcollection_construct_empty(COLLECTIONTYPE, srid, 0, 0);
			}
			lwfree(vertical_line);
		}
	}
	else
	{
		if (!lw_dist3d_recursive(lw1, lw2, &thedl))
		{
			/* should never get here. all cases ought to be error handled earlier */
			lwerror("Some unspecified error.");
			result = (LWGEOM *)lwcollection_construct_empty(COLLECTIONTYPE, srid, 0, 0);
		}
	}
	if (thedl.distance == initdistance)
	{
		LWDEBUG(3, "didn't find geometries to measure between, returning null");
		result = (LWGEOM *)lwcollection_construct_empty(COLLECTIONTYPE, srid, 0, 0);
	}
	else
	{
		x = thedl.p1.x;
		y = thedl.p1.y;
		z = thedl.p1.z;
		result = (LWGEOM *)lwpoint_make3dz(srid, x, y, z);
	}

	return result;
}

/**
Function initializing 3d max distance calculation
*/
double
lwgeom_maxdistance3d(const LWGEOM *lw1, const LWGEOM *lw2)
{
	return lwgeom_maxdistance3d_tolerance(lw1, lw2, 0.0);
}

/**
Function handling 3d max distance calculations and dfullywithin calculations.
The difference is just the tolerance.
*/
double
lwgeom_maxdistance3d_tolerance(const LWGEOM *lw1, const LWGEOM *lw2, double tolerance)
{
	if (!lwgeom_has_z(lw1) || !lwgeom_has_z(lw2))
	{
		lwnotice(
		    "One or both of the geometries is missing z-value. The unknown z-value will be regarded as \"any value\"");
		return lwgeom_maxdistance2d_tolerance(lw1, lw2, tolerance);
	}
	DISTPTS3D thedl;
	LWDEBUG(2, "lwgeom_maxdistance3d_tolerance is called");
	thedl.mode = DIST_MAX;
	thedl.distance = -1;
	thedl.tolerance = tolerance;
	if (lw_dist3d_recursive(lw1, lw2, &thedl))
		return thedl.distance;

	/* should never get here. all cases ought to be error handled earlier */
	lwerror("Some unspecified error.");
	return -1;
}

/**
	Function initializing 3d min distance calculation
*/
double
lwgeom_mindistance3d(const LWGEOM *lw1, const LWGEOM *lw2)
{
	return lwgeom_mindistance3d_tolerance(lw1, lw2, 0.0);
}

static inline int
gbox_contains_3d(const GBOX *g1, const GBOX *g2)
{
	return (g2->xmin >= g1->xmin) && (g2->xmax <= g1->xmax) && (g2->ymin >= g1->ymin) && (g2->ymax <= g1->ymax) &&
	       (g2->zmin >= g1->zmin) && (g2->zmax <= g1->zmax);
}

static inline int
lwgeom_solid_contains_lwgeom(const LWGEOM *solid, const LWGEOM *g)
{
	const GBOX *b1;
	const GBOX *b2;

	/* If solid isn't solid it can't contain anything */
	if (!FLAGS_GET_SOLID(solid->flags))
		return LW_FALSE;

	b1 = lwgeom_get_bbox(solid);
	b2 = lwgeom_get_bbox(g);

	/* If box won't contain box, shape won't contain shape */
	if (!gbox_contains_3d(b1, b2))
		return LW_FALSE;
	else /* Raycast upwards in Z */
	{
		POINT4D pt;
		/* We'll skew copies if we're not lucky */
		LWGEOM *solid_copy = lwgeom_clone_deep(solid);
		LWGEOM *g_copy = lwgeom_clone_deep(g);

		while (LW_TRUE)
		{
			uint8_t is_boundary = LW_FALSE;
			uint8_t is_inside = LW_FALSE;

			/* take first point */
			if (!lwgeom_startpoint(g_copy, &pt))
				return LW_FALSE;

			/* get part of solid that is upwards */
			LWCOLLECTION *c = lwgeom_clip_to_ordinate_range(solid_copy, 'Z', pt.z, DBL_MAX, 0);

			for (uint32_t i = 0; i < c->ngeoms; i++)
			{
				if (c->geoms[i]->type == POLYGONTYPE)
				{
					/* 3D raycast along Z is 2D point in polygon */
					int t = lwpoly_contains_point((LWPOLY *)c->geoms[i], (POINT2D *)&pt);

					if (t == LW_INSIDE)
						is_inside = !is_inside;
					else if (t == LW_BOUNDARY)
					{
						is_boundary = LW_TRUE;
						break;
					}
				}
				else if (c->geoms[i]->type == TRIANGLETYPE)
				{
					/* 3D raycast along Z is 2D point in polygon */
					LWTRIANGLE *tri = (LWTRIANGLE *)c->geoms[i];
					int t = ptarray_contains_point(tri->points, (POINT2D *)&pt);

					if (t == LW_INSIDE)
						is_inside = !is_inside;
					else if (t == LW_BOUNDARY)
					{
						is_boundary = LW_TRUE;
						break;
					}
				}
			}

			lwcollection_free(c);
			lwgeom_free(solid_copy);
			lwgeom_free(g_copy);

			if (is_boundary)
			{
				/* randomly skew a bit and restart*/
				double cx = lwrandom_uniform() - 0.5;
				double cy = lwrandom_uniform() - 0.5;
				AFFINE aff = {1, 0, cx, 0, 1, cy, 0, 0, 1, 0, 0, 0};

				solid_copy = lwgeom_clone_deep(solid);
				lwgeom_affine(solid_copy, &aff);

				g_copy = lwgeom_clone_deep(g);
				lwgeom_affine(g_copy, &aff);

				continue;
			}
			return is_inside;
		}
	}
}

/**
	Function handling 3d min distance calculations and dwithin calculations.
	The difference is just the tolerance.
*/
double
lwgeom_mindistance3d_tolerance(const LWGEOM *lw1, const LWGEOM *lw2, double tolerance)
{
	assert(tolerance >= 0);
	if (!lwgeom_has_z(lw1) || !lwgeom_has_z(lw2))
	{
		lwnotice(
		    "One or both of the geometries is missing z-value. The unknown z-value will be regarded as \"any value\"");

		return lwgeom_mindistance2d_tolerance(lw1, lw2, tolerance);
	}
	DISTPTS3D thedl;
	thedl.mode = DIST_MIN;
	thedl.distance = DBL_MAX;
	thedl.tolerance = tolerance;

	if (lw_dist3d_recursive(lw1, lw2, &thedl))
	{
		if (thedl.distance <= tolerance)
			return thedl.distance;
		if (lwgeom_solid_contains_lwgeom(lw1, lw2) || lwgeom_solid_contains_lwgeom(lw2, lw1))
			return 0;

		return thedl.distance;
	}

	/* should never get here. all cases ought to be error handled earlier */
	lwerror("Some unspecified error.");
	return DBL_MAX;
}

/*------------------------------------------------------------------------------------------------------------
End of Initializing functions
--------------------------------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------------------------------
Preprocessing functions
Functions preparing geometries for distance-calculations
--------------------------------------------------------------------------------------------------------------*/

/**
This is a recursive function delivering every possible combination of subgeometries
*/
int
lw_dist3d_recursive(const LWGEOM *lwg1, const LWGEOM *lwg2, DISTPTS3D *dl)
{
	int i, j;
	int n1 = 1;
	int n2 = 1;
	LWGEOM *g1 = NULL;
	LWGEOM *g2 = NULL;
	LWCOLLECTION *c1 = NULL;
	LWCOLLECTION *c2 = NULL;

	LWDEBUGF(2, "lw_dist3d_recursive is called with type1=%d, type2=%d", lwg1->type, lwg2->type);

	if (lwgeom_is_collection(lwg1))
	{
		LWDEBUG(3, "First geometry is collection");
		c1 = lwgeom_as_lwcollection(lwg1);
		n1 = c1->ngeoms;
	}
	if (lwgeom_is_collection(lwg2))
	{
		LWDEBUG(3, "Second geometry is collection");
		c2 = lwgeom_as_lwcollection(lwg2);
		n2 = c2->ngeoms;
	}

	for (i = 0; i < n1; i++)
	{
		if (lwgeom_is_collection(lwg1))
			g1 = c1->geoms[i];
		else
			g1 = (LWGEOM *)lwg1;

		if (lwgeom_is_empty(g1))
			return LW_TRUE;

		if (lwgeom_is_collection(g1))
		{
			LWDEBUG(3, "Found collection inside first geometry collection, recursing");
			if (!lw_dist3d_recursive(g1, lwg2, dl))
				return LW_FALSE;
			continue;
		}
		for (j = 0; j < n2; j++)
		{
			if (lwgeom_is_collection(lwg2))
				g2 = c2->geoms[j];
			else
				g2 = (LWGEOM *)lwg2;

			if (lwgeom_is_collection(g2))
			{
				LWDEBUG(3, "Found collection inside second geometry collection, recursing");
				if (!lw_dist3d_recursive(g1, g2, dl))
					return LW_FALSE;
				continue;
			}

			/*If one of geometries is empty, return. True here only means continue searching. False would
			 * have stopped the process*/
			if (lwgeom_is_empty(g1) || lwgeom_is_empty(g2))
				return LW_TRUE;

			if (!lw_dist3d_distribute_bruteforce(g1, g2, dl))
				return LW_FALSE;
			if (dl->distance <= dl->tolerance && dl->mode == DIST_MIN)
				return LW_TRUE; /*just a check if the answer is already given*/
		}
	}
	return LW_TRUE;
}

/**

This function distributes the brute-force for 3D so far the only type, tasks depending on type
*/
int
lw_dist3d_distribute_bruteforce(const LWGEOM *lwg1, const LWGEOM *lwg2, DISTPTS3D *dl)
{
	int t1 = lwg1->type;
	int t2 = lwg2->type;

	LWDEBUGF(2, "lw_dist3d_distribute_bruteforce is called with typ1=%d, type2=%d", lwg1->type, lwg2->type);

	if (t1 == POINTTYPE)
	{
		if (t2 == POINTTYPE)
		{
			dl->twisted = 1;
			return lw_dist3d_point_point((LWPOINT *)lwg1, (LWPOINT *)lwg2, dl);
		}
		else if (t2 == LINETYPE)
		{
			dl->twisted = 1;
			return lw_dist3d_point_line((LWPOINT *)lwg1, (LWLINE *)lwg2, dl);
		}
		else if (t2 == POLYGONTYPE)
		{
			dl->twisted = 1;
			return lw_dist3d_point_poly((LWPOINT *)lwg1, (LWPOLY *)lwg2, dl);
		}
		else if (t2 == TRIANGLETYPE)
		{
			dl->twisted = 1;
			return lw_dist3d_point_tri((LWPOINT *)lwg1, (LWTRIANGLE *)lwg2, dl);
		}
		else
		{
			lwerror("%s: Unsupported geometry type: %s", __func__, lwtype_name(t2));
			return LW_FALSE;
		}
	}
	else if (t1 == LINETYPE)
	{
		if (t2 == POINTTYPE)
		{
			dl->twisted = -1;
			return lw_dist3d_point_line((LWPOINT *)lwg2, (LWLINE *)lwg1, dl);
		}
		else if (t2 == LINETYPE)
		{
			dl->twisted = 1;
			return lw_dist3d_line_line((LWLINE *)lwg1, (LWLINE *)lwg2, dl);
		}
		else if (t2 == POLYGONTYPE)
		{
			dl->twisted = 1;
			return lw_dist3d_line_poly((LWLINE *)lwg1, (LWPOLY *)lwg2, dl);
		}
		else if (t2 == TRIANGLETYPE)
		{
			dl->twisted = 1;
			return lw_dist3d_line_tri((LWLINE *)lwg1, (LWTRIANGLE *)lwg2, dl);
		}
		else
		{
			lwerror("%s: Unsupported geometry type: %s", __func__, lwtype_name(t2));
			return LW_FALSE;
		}
	}
	else if (t1 == POLYGONTYPE)
	{
		if (t2 == POLYGONTYPE)
		{
			dl->twisted = 1;
			return lw_dist3d_poly_poly((LWPOLY *)lwg1, (LWPOLY *)lwg2, dl);
		}
		else if (t2 == POINTTYPE)
		{
			dl->twisted = -1;
			return lw_dist3d_point_poly((LWPOINT *)lwg2, (LWPOLY *)lwg1, dl);
		}
		else if (t2 == LINETYPE)
		{
			dl->twisted = -1;
			return lw_dist3d_line_poly((LWLINE *)lwg2, (LWPOLY *)lwg1, dl);
		}
		else if (t2 == TRIANGLETYPE)
		{
			dl->twisted = 1;
			return lw_dist3d_poly_tri((LWPOLY *)lwg1, (LWTRIANGLE *)lwg2, dl);
		}
		else
		{
			lwerror("%s: Unsupported geometry type: %s", __func__, lwtype_name(t2));
			return LW_FALSE;
		}
	}
	else if (t1 == TRIANGLETYPE)
	{
		if (t2 == POLYGONTYPE)
		{
			dl->twisted = -1;
			return lw_dist3d_poly_tri((LWPOLY *)lwg2, (LWTRIANGLE *)lwg1, dl);
		}
		else if (t2 == POINTTYPE)
		{
			dl->twisted = -1;
			return lw_dist3d_point_tri((LWPOINT *)lwg2, (LWTRIANGLE *)lwg1, dl);
		}
		else if (t2 == LINETYPE)
		{
			dl->twisted = -1;
			return lw_dist3d_line_tri((LWLINE *)lwg2, (LWTRIANGLE *)lwg1, dl);
		}
		else if (t2 == TRIANGLETYPE)
		{
			dl->twisted = 1;
			return lw_dist3d_tri_tri((LWTRIANGLE *)lwg1, (LWTRIANGLE *)lwg2, dl);
		}
		else
		{
			lwerror("%s: Unsupported geometry type: %s", __func__, lwtype_name(t2));
			return LW_FALSE;
		}
	}

	else
	{
		lwerror("%s: Unsupported geometry type: %s", __func__, lwtype_name(t1));
		return LW_FALSE;
	}
}

/*------------------------------------------------------------------------------------------------------------
End of Preprocessing functions
--------------------------------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------------------------------
Brute force functions
So far the only way to do 3D-calculations
--------------------------------------------------------------------------------------------------------------*/

/**

point to point calculation
*/
int
lw_dist3d_point_point(LWPOINT *point1, LWPOINT *point2, DISTPTS3D *dl)
{
	POINT3DZ p1;
	POINT3DZ p2;
	LWDEBUG(2, "lw_dist3d_point_point is called");

	getPoint3dz_p(point1->point, 0, &p1);
	getPoint3dz_p(point2->point, 0, &p2);

	return lw_dist3d_pt_pt(&p1, &p2, dl);
}
/**

point to line calculation
*/
int
lw_dist3d_point_line(LWPOINT *point, LWLINE *line, DISTPTS3D *dl)
{
	POINT3DZ p;
	POINTARRAY *pa = line->points;
	LWDEBUG(2, "lw_dist3d_point_line is called");

	getPoint3dz_p(point->point, 0, &p);
	return lw_dist3d_pt_ptarray(&p, pa, dl);
}

/**

Computes point to polygon distance
For mindistance that means:
1) find the plane of the polygon
2) projecting the point to the plane of the polygon
3) finding if that projected point is inside the polygon, if so the distance is measured to that projected point
4) if not in polygon above, check the distance against the boundary of the polygon
for max distance it is always point against boundary

*/

int
lw_dist3d_point_poly(LWPOINT *point, LWPOLY *poly, DISTPTS3D *dl)
{
	POINT3DZ p, projp; /*projp is "point projected on plane"*/
	PLANE3D plane;
	LWDEBUG(2, "lw_dist3d_point_poly is called");
	getPoint3dz_p(point->point, 0, &p);

	/* If we are looking for max distance, longestline or dfullywithin */
	if (dl->mode == DIST_MAX)
		return lw_dist3d_pt_ptarray(&p, poly->rings[0], dl);

	/* Find the plane of the polygon, the "holes" have to be on the same plane. so we only care about the boudary */
	if (!define_plane(poly->rings[0], &plane))
	{
		/* Polygon does not define a plane: Return distance point -> line */
		return lw_dist3d_pt_ptarray(&p, poly->rings[0], dl);
	}

	/* Get our point projected on the plane of the polygon */
	project_point_on_plane(&p, &plane, &projp);

	return lw_dist3d_pt_poly(&p, poly, &plane, &projp, dl);
}

/* point to triangle calculation */
int
lw_dist3d_point_tri(LWPOINT *point, LWTRIANGLE *tri, DISTPTS3D *dl)
{
	POINT3DZ p, projp; /*projp is "point projected on plane"*/
	PLANE3D plane;
	getPoint3dz_p(point->point, 0, &p);

	/* If we are looking for max distance, longestline or dfullywithin */
	if (dl->mode == DIST_MAX)
		return lw_dist3d_pt_ptarray(&p, tri->points, dl);

	/* If triangle does not define a plane, treat it as a line */
	if (!define_plane(tri->points, &plane))
		return lw_dist3d_pt_ptarray(&p, tri->points, dl);

	/* Get our point projected on the plane of triangle */
	project_point_on_plane(&p, &plane, &projp);

	return lw_dist3d_pt_tri(&p, tri, &plane, &projp, dl);
}

/** line to line calculation */
int
lw_dist3d_line_line(LWLINE *line1, LWLINE *line2, DISTPTS3D *dl)
{
	POINTARRAY *pa1 = line1->points;
	POINTARRAY *pa2 = line2->points;
	LWDEBUG(2, "lw_dist3d_line_line is called");

	return lw_dist3d_ptarray_ptarray(pa1, pa2, dl);
}

/** line to polygon calculation */
int
lw_dist3d_line_poly(LWLINE *line, LWPOLY *poly, DISTPTS3D *dl)
{
	PLANE3D plane;
	LWDEBUG(2, "lw_dist3d_line_poly is called");

	if (dl->mode == DIST_MAX)
		return lw_dist3d_ptarray_ptarray(line->points, poly->rings[0], dl);

	/* if polygon does not define a plane: Return distance line to line */
	if (!define_plane(poly->rings[0], &plane))
		return lw_dist3d_ptarray_ptarray(line->points, poly->rings[0], dl);

	return lw_dist3d_ptarray_poly(line->points, poly, &plane, dl);
}

/** line to triangle calculation */
int
lw_dist3d_line_tri(LWLINE *line, LWTRIANGLE *tri, DISTPTS3D *dl)
{
	PLANE3D plane;

	if (dl->mode == DIST_MAX)
		return lw_dist3d_ptarray_ptarray(line->points, tri->points, dl);

	/* if triangle does not define a plane: Return distance line to line */
	if (!define_plane(tri->points, &plane))
		return lw_dist3d_ptarray_ptarray(line->points, tri->points, dl);

	return lw_dist3d_ptarray_tri(line->points, tri, &plane, dl);
}

/** polygon to polygon calculation */
int
lw_dist3d_poly_poly(LWPOLY *poly1, LWPOLY *poly2, DISTPTS3D *dl)
{
	PLANE3D plane1, plane2;
	int planedef1, planedef2;
	LWDEBUG(2, "lw_dist3d_poly_poly is called");
	if (dl->mode == DIST_MAX)
		return lw_dist3d_ptarray_ptarray(poly1->rings[0], poly2->rings[0], dl);

	planedef1 = define_plane(poly1->rings[0], &plane1);
	planedef2 = define_plane(poly2->rings[0], &plane2);

	if (!planedef1 || !planedef2)
	{
		/* Neither polygon define a plane: Return distance line to line */
		if (!planedef1 && !planedef2)
			return lw_dist3d_ptarray_ptarray(poly1->rings[0], poly2->rings[0], dl);

		/* Only poly2 defines a plane: Return distance from line (poly1) to poly2 */
		else if (!planedef1)
			return lw_dist3d_ptarray_poly(poly1->rings[0], poly2, &plane2, dl);

		/* Only poly1 defines a plane: Return distance from line (poly2) to poly1 */
		else
			return lw_dist3d_ptarray_poly(poly2->rings[0], poly1, &plane1, dl);
	}

	/* What we do here is to compare the boundary of one polygon with the other polygon
	and then take the second boundary comparing with the first polygon */
	dl->twisted = 1;
	if (!lw_dist3d_ptarray_poly(poly1->rings[0], poly2, &plane2, dl))
		return LW_FALSE;
	if (dl->distance < dl->tolerance) /* Just check if the answer already is given*/
		return LW_TRUE;

	dl->twisted = -1; /* because we switch the order of geometries we switch "twisted" to -1 which will give the
			     right order of points in shortest line. */
	return lw_dist3d_ptarray_poly(poly2->rings[0], poly1, &plane1, dl);
}

/** polygon to triangle calculation */
int
lw_dist3d_poly_tri(LWPOLY *poly, LWTRIANGLE *tri, DISTPTS3D *dl)
{
	PLANE3D plane1, plane2;
	int planedef1, planedef2;

	if (dl->mode == DIST_MAX)
		return lw_dist3d_ptarray_ptarray(poly->rings[0], tri->points, dl);

	planedef1 = define_plane(poly->rings[0], &plane1);
	planedef2 = define_plane(tri->points, &plane2);

	if (!planedef1 || !planedef2)
	{
		/* Neither define a plane: Return distance line to line */
		if (!planedef1 && !planedef2)
			return lw_dist3d_ptarray_ptarray(poly->rings[0], tri->points, dl);

		/* Only tri defines a plane: Return distance from line (poly) to tri */
		else if (!planedef1)
			return lw_dist3d_ptarray_tri(poly->rings[0], tri, &plane2, dl);

		/* Only poly defines a plane: Return distance from line (tri) to poly */
		else
			return lw_dist3d_ptarray_poly(tri->points, poly, &plane1, dl);
	}

	/* What we do here is to compare the boundary of one polygon with the other polygon
	and then take the second boundary comparing with the first polygon */
	dl->twisted = 1;
	if (!lw_dist3d_ptarray_tri(poly->rings[0], tri, &plane2, dl))
		return LW_FALSE;
	if (dl->distance < dl->tolerance) /* Just check if the answer already is given*/
		return LW_TRUE;

	dl->twisted = -1; /* because we switch the order of geometries we switch "twisted" to -1 which will give the
			     right order of points in shortest line. */
	return lw_dist3d_ptarray_poly(tri->points, poly, &plane1, dl);
}

/** triangle to triangle calculation */
int
lw_dist3d_tri_tri(LWTRIANGLE *tri1, LWTRIANGLE *tri2, DISTPTS3D *dl)
{
	PLANE3D plane1, plane2;
	int planedef1, planedef2;

	if (dl->mode == DIST_MAX)
		return lw_dist3d_ptarray_ptarray(tri1->points, tri2->points, dl);

	planedef1 = define_plane(tri1->points, &plane1);
	planedef2 = define_plane(tri2->points, &plane2);

	if (!planedef1 || !planedef2)
	{
		/* Neither define a plane: Return distance line to line */
		if (!planedef1 && !planedef2)
			return lw_dist3d_ptarray_ptarray(tri1->points, tri2->points, dl);

		/* Only tri defines a plane: Return distance from line (tri1) to tri2 */
		else if (!planedef1)
			return lw_dist3d_ptarray_tri(tri1->points, tri2, &plane2, dl);

		/* Only poly defines a plane: Return distance from line (tri2) to tri1 */
		else
			return lw_dist3d_ptarray_tri(tri2->points, tri1, &plane1, dl);
	}

	/* What we do here is to compare the boundary of one polygon with the other polygon
	and then take the second boundary comparing with the first polygon */
	dl->twisted = 1;
	if (!lw_dist3d_ptarray_tri(tri1->points, tri2, &plane2, dl))
		return LW_FALSE;
	if (dl->distance < dl->tolerance) /* Just check if the answer already is given*/
		return LW_TRUE;

	dl->twisted = -1; /* because we switch the order of geometries we switch "twisted" to -1 which will give the
			     right order of points in shortest line. */
	return lw_dist3d_ptarray_tri(tri2->points, tri1, &plane1, dl);
}

/**
 * search all the segments of pointarray to see which one is closest to p
 * Returns distance between point and pointarray
 */
int
lw_dist3d_pt_ptarray(POINT3DZ *p, POINTARRAY *pa, DISTPTS3D *dl)
{
	uint32_t t;
	POINT3DZ start, end;
	int twist = dl->twisted;
	if (!pa)
		return LW_FALSE;

	getPoint3dz_p(pa, 0, &start);

	for (t = 1; t < pa->npoints; t++)
	{
		dl->twisted = twist;
		getPoint3dz_p(pa, t, &end);
		if (!lw_dist3d_pt_seg(p, &start, &end, dl))
			return LW_FALSE;

		if (dl->distance <= dl->tolerance && dl->mode == DIST_MIN)
			return LW_TRUE; /*just a check if  the answer is already given*/
		start = end;
	}

	return LW_TRUE;
}

/**
If searching for min distance, this one finds the closest point on segment A-B from p.
if searching for max distance it just sends p-A and p-B to pt-pt calculation
*/
int
lw_dist3d_pt_seg(POINT3DZ *p, POINT3DZ *A, POINT3DZ *B, DISTPTS3D *dl)
{
	POINT3DZ c;
	double r;
	/*if start==end, then use pt distance */
	if ((A->x == B->x) && (A->y == B->y) && (A->z == B->z))
	{
		return lw_dist3d_pt_pt(p, A, dl);
	}

	r = ((p->x - A->x) * (B->x - A->x) + (p->y - A->y) * (B->y - A->y) + (p->z - A->z) * (B->z - A->z)) /
	    ((B->x - A->x) * (B->x - A->x) + (B->y - A->y) * (B->y - A->y) + (B->z - A->z) * (B->z - A->z));

	/*This is for finding the 3Dmaxdistance.
	the maxdistance have to be between two vertexes,
	compared to mindistance which can be between
	two vertexes vertex.*/
	if (dl->mode == DIST_MAX)
	{
		if (r >= 0.5)
			return lw_dist3d_pt_pt(p, A, dl);
		if (r < 0.5)
			return lw_dist3d_pt_pt(p, B, dl);
	}

	if (r <= 0) /*If the first vertex A is closest to the point p*/
		return lw_dist3d_pt_pt(p, A, dl);
	if (r >= 1) /*If the second vertex B is closest to the point p*/
		return lw_dist3d_pt_pt(p, B, dl);

	/*else if the point p is closer to some point between a and b
	then we find that point and send it to lw_dist3d_pt_pt*/
	c.x = A->x + r * (B->x - A->x);
	c.y = A->y + r * (B->y - A->y);
	c.z = A->z + r * (B->z - A->z);

	return lw_dist3d_pt_pt(p, &c, dl);
}

double
distance3d_pt_pt(const POINT3D *p1, const POINT3D *p2)
{
	double dx = p2->x - p1->x;
	double dy = p2->y - p1->y;
	double dz = p2->z - p1->z;
	return sqrt(dx * dx + dy * dy + dz * dz);
}

/**

Compares incoming points and
stores the points closest to each other
or most far away from each other
depending on dl->mode (max or min)
*/
int
lw_dist3d_pt_pt(POINT3DZ *thep1, POINT3DZ *thep2, DISTPTS3D *dl)
{
	double dx = thep2->x - thep1->x;
	double dy = thep2->y - thep1->y;
	double dz = thep2->z - thep1->z;
	double dist = sqrt(dx * dx + dy * dy + dz * dz);
	LWDEBUGF(2,
		 "lw_dist3d_pt_pt called (with points: p1.x=%f, p1.y=%f,p1.z=%f,p2.x=%f, p2.y=%f,p2.z=%f)",
		 thep1->x,
		 thep1->y,
		 thep1->z,
		 thep2->x,
		 thep2->y,
		 thep2->z);

	if (((dl->distance - dist) * (dl->mode)) >
	    0) /*multiplication with mode to handle mindistance (mode=1)  and maxdistance (mode = (-1)*/
	{
		dl->distance = dist;

		if (dl->twisted > 0) /*To get the points in right order. twisted is updated between 1 and (-1) every
					time the order is changed earlier in the chain*/
		{
			dl->p1 = *thep1;
			dl->p2 = *thep2;
		}
		else
		{
			dl->p1 = *thep2;
			dl->p2 = *thep1;
		}
	}
	return LW_TRUE;
}

/**

Finds all combinations of segments between two pointarrays
*/
int
lw_dist3d_ptarray_ptarray(POINTARRAY *l1, POINTARRAY *l2, DISTPTS3D *dl)
{
	uint32_t t, u;
	POINT3DZ start, end;
	POINT3DZ start2, end2;
	int twist = dl->twisted;
	LWDEBUGF(2, "lw_dist3d_ptarray_ptarray called (points: %d-%d)", l1->npoints, l2->npoints);

	if (dl->mode == DIST_MAX) /*If we are searching for maxdistance we go straight to point-point calculation since
				     the maxdistance have to be between two vertexes*/
	{
		for (t = 0; t < l1->npoints; t++) /*for each segment in L1 */
		{
			getPoint3dz_p(l1, t, &start);
			for (u = 0; u < l2->npoints; u++) /*for each segment in L2 */
			{
				getPoint3dz_p(l2, u, &start2);
				lw_dist3d_pt_pt(&start, &start2, dl);
			}
		}
	}
	else
	{
		getPoint3dz_p(l1, 0, &start);
		for (t = 1; t < l1->npoints; t++) /*for each segment in L1 */
		{
			getPoint3dz_p(l1, t, &end);
			getPoint3dz_p(l2, 0, &start2);
			for (u = 1; u < l2->npoints; u++) /*for each segment in L2 */
			{
				getPoint3dz_p(l2, u, &end2);
				dl->twisted = twist;
				lw_dist3d_seg_seg(&start, &end, &start2, &end2, dl);
				LWDEBUGF(
				    4, "mindist_ptarray_ptarray; seg %i * seg %i, dist = %g\n", t, u, dl->distance);
				LWDEBUGF(3, " seg%d-seg%d dist: %f, mindist: %f", t, u, dl->distance, dl->tolerance);
				if (dl->distance <= dl->tolerance && dl->mode == DIST_MIN)
					return LW_TRUE; /*just a check if the answer is already given*/
				start2 = end2;
			}
			start = end;
		}
	}
	return LW_TRUE;
}

/**

Finds the two closest points on two linesegments
*/
int
lw_dist3d_seg_seg(POINT3DZ *s1p1, POINT3DZ *s1p2, POINT3DZ *s2p1, POINT3DZ *s2p2, DISTPTS3D *dl)
{
	VECTOR3D v1, v2, vl;
	double s1k, s2k; /*two variables representing where on Line 1 (s1k) and where on Line 2 (s2k) a connecting line
			    between the two lines is perpendicular to both lines*/
	POINT3DZ p1, p2;
	double a, b, c, d, e, D;

	/*s1p1 and s1p2 are the same point */
	if ((s1p1->x == s1p2->x) && (s1p1->y == s1p2->y) && (s1p1->z == s1p2->z))
	{
		return lw_dist3d_pt_seg(s1p1, s2p1, s2p2, dl);
	}
	/*s2p1 and s2p2 are the same point */
	if ((s2p1->x == s2p2->x) && (s2p1->y == s2p2->y) && (s2p1->z == s2p2->z))
	{
		dl->twisted = ((dl->twisted) * (-1));
		return lw_dist3d_pt_seg(s2p1, s1p1, s1p2, dl);
	}

	/*
		Here we use algorithm from softsurfer.com
		that can be found here
		http://softsurfer.com/Archive/algorithm_0106/algorithm_0106.htm
	*/

	if (!get_3dvector_from_points(s1p1, s1p2, &v1))
		return LW_FALSE;

	if (!get_3dvector_from_points(s2p1, s2p2, &v2))
		return LW_FALSE;

	if (!get_3dvector_from_points(s2p1, s1p1, &vl))
		return LW_FALSE;

	a = DOT(v1, v1);
	b = DOT(v1, v2);
	c = DOT(v2, v2);
	d = DOT(v1, vl);
	e = DOT(v2, vl);
	D = a * c - b * b;

	if (D < 0.000000001)
	{ /* the lines are almost parallel*/
		s1k =
		    0.0;   /*If the lines are parallel we try by using the startpoint of first segment. If that gives a
			      projected point on the second line outside segment 2 it wil be found that s2k is >1 or <0.*/
		if (b > c) /* use the largest denominator*/
			s2k = d / b;
		else
			s2k = e / c;
	}
	else
	{
		s1k = (b * e - c * d) / D;
		s2k = (a * e - b * d) / D;
	}

	/* Now we check if the projected closest point on the infinite lines is outside our segments. If so the
	 * combinations with start and end points will be tested*/

	if (s1k <= 0.0 || s1k >= 1.0 || s2k <= 0.0 || s2k >= 1.0)
	{
		if (s1k <= 0.0)
		{
			if (!lw_dist3d_pt_seg(s1p1, s2p1, s2p2, dl))
				return LW_FALSE;
		}
		if (s1k >= 1.0)
		{
			if (!lw_dist3d_pt_seg(s1p2, s2p1, s2p2, dl))
				return LW_FALSE;
		}
		if (s2k <= 0.0)
		{
			dl->twisted = ((dl->twisted) * (-1));
			if (!lw_dist3d_pt_seg(s2p1, s1p1, s1p2, dl))
				return LW_FALSE;
		}
		if (s2k >= 1.0)
		{
			dl->twisted = ((dl->twisted) * (-1));
			if (!lw_dist3d_pt_seg(s2p2, s1p1, s1p2, dl))
				return LW_FALSE;
		}
	}
	else
	{ /*Find the closest point on the edges of both segments*/
		p1.x = s1p1->x + s1k * (s1p2->x - s1p1->x);
		p1.y = s1p1->y + s1k * (s1p2->y - s1p1->y);
		p1.z = s1p1->z + s1k * (s1p2->z - s1p1->z);

		p2.x = s2p1->x + s2k * (s2p2->x - s2p1->x);
		p2.y = s2p1->y + s2k * (s2p2->y - s2p1->y);
		p2.z = s2p1->z + s2k * (s2p2->z - s2p1->z);

		if (!lw_dist3d_pt_pt(&p1, &p2, dl)) /* Send the closest points to point-point calculation*/
		{
			return LW_FALSE;
		}
	}
	return LW_TRUE;
}

/**

Checking if the point projected on the plane of the polygon actually is inside that polygon.
If so the mindistance is between that projected point and our original point.
If not we check from original point to the boundary.
If the projected point is inside a hole of the polygon we check the distance to the boundary of that hole.
*/
int
lw_dist3d_pt_poly(POINT3DZ *p, LWPOLY *poly, PLANE3D *plane, POINT3DZ *projp, DISTPTS3D *dl)
{
	uint32_t i;

	if (pt_in_ring_3d(projp, poly->rings[0], plane))
	{
		for (i = 1; i < poly->nrings; i++)
		{
			/* Inside a hole. Distance = pt -> ring */
			if (pt_in_ring_3d(projp, poly->rings[i], plane))
				return lw_dist3d_pt_ptarray(p, poly->rings[i], dl);
		}

		/* if the projected point is inside the polygon the shortest distance is between that point and the
		 * input point */
		return lw_dist3d_pt_pt(p, projp, dl);
	}
	else
		/* if the projected point is outside the polygon we search for the closest distance against the boundary
		 * instead */
		return lw_dist3d_pt_ptarray(p, poly->rings[0], dl);

	return LW_TRUE;
}

int
lw_dist3d_pt_tri(POINT3DZ *p, LWTRIANGLE *tri, PLANE3D *plane, POINT3DZ *projp, DISTPTS3D *dl)
{
	if (pt_in_ring_3d(projp, tri->points, plane))
		/* if the projected point is inside the polygon the shortest distance is between that point and the
		 * input point */
		return lw_dist3d_pt_pt(p, projp, dl);
	else
		/* if the projected point is outside the polygon we search for the closest distance against the boundary
		 * instead */
		return lw_dist3d_pt_ptarray(p, tri->points, dl);

	return LW_TRUE;
}

/** Computes pointarray to polygon distance */
int
lw_dist3d_ptarray_poly(POINTARRAY *pa, LWPOLY *poly, PLANE3D *plane, DISTPTS3D *dl)
{
	uint32_t i, j, k;
	double f, s1, s2;
	VECTOR3D projp1_projp2;
	POINT3DZ p1, p2, projp1, projp2, intersectionp;

	getPoint3dz_p(pa, 0, &p1);

	/* the sign of s1 tells us on which side of the plane the point is. */
	s1 = project_point_on_plane(&p1, plane, &projp1);
	lw_dist3d_pt_poly(&p1, poly, plane, &projp1, dl);
	if ((s1 == 0.0) && (dl->distance < dl->tolerance))
		return LW_TRUE;

	for (i = 1; i < pa->npoints; i++)
	{
		int intersects;
		getPoint3dz_p(pa, i, &p2);
		s2 = project_point_on_plane(&p2, plane, &projp2);
		lw_dist3d_pt_poly(&p2, poly, plane, &projp2, dl);
		if ((s2 == 0.0) && (dl->distance < dl->tolerance))
			return LW_TRUE;

		/* If s1 and s2 has different signs that means they are on different sides of the plane of the polygon.
		 * That means that the edge between the points crosses the plane and might intersect with the polygon */
		if ((s1 * s2) < 0)
		{
			/* The size of s1 and s2 is the distance from the point to the plane. */
			f = fabs(s1) / (fabs(s1) + fabs(s2));
			get_3dvector_from_points(&projp1, &projp2, &projp1_projp2);

			/* Get the point where the line segment crosses the plane */
			intersectionp.x = projp1.x + f * projp1_projp2.x;
			intersectionp.y = projp1.y + f * projp1_projp2.y;
			intersectionp.z = projp1.z + f * projp1_projp2.z;

			/* We set intersects to true until the opposite is proved */
			intersects = LW_TRUE;

			if (pt_in_ring_3d(&intersectionp, poly->rings[0], plane)) /*Inside outer ring*/
			{
				for (k = 1; k < poly->nrings; k++)
				{
					/* Inside a hole, so no intersection with the polygon*/
					if (pt_in_ring_3d(&intersectionp, poly->rings[k], plane))
					{
						intersects = LW_FALSE;
						break;
					}
				}
				if (intersects)
				{
					dl->distance = 0.0;
					dl->p1.x = intersectionp.x;
					dl->p1.y = intersectionp.y;
					dl->p1.z = intersectionp.z;

					dl->p2.x = intersectionp.x;
					dl->p2.y = intersectionp.y;
					dl->p2.z = intersectionp.z;
					return LW_TRUE;
				}
			}
		}

		projp1 = projp2;
		s1 = s2;
		p1 = p2;
	}

	/* check our pointarray against boundary and inner boundaries of the polygon */
	for (j = 0; j < poly->nrings; j++)
		lw_dist3d_ptarray_ptarray(pa, poly->rings[j], dl);

	return LW_TRUE;
}

/** Computes pointarray to triangle distance */
int
lw_dist3d_ptarray_tri(POINTARRAY *pa, LWTRIANGLE *tri, PLANE3D *plane, DISTPTS3D *dl)
{
	uint32_t i;
	double f, s1, s2;
	VECTOR3D projp1_projp2;
	POINT3DZ p1, p2, projp1, projp2, intersectionp;

	getPoint3dz_p(pa, 0, &p1);

	/* the sign of s1 tells us on which side of the plane the point is. */
	s1 = project_point_on_plane(&p1, plane, &projp1);
	lw_dist3d_pt_tri(&p1, tri, plane, &projp1, dl);
	if ((s1 == 0.0) && (dl->distance < dl->tolerance))
		return LW_TRUE;

	for (i = 1; i < pa->npoints; i++)
	{
		int intersects;
		getPoint3dz_p(pa, i, &p2);
		s2 = project_point_on_plane(&p2, plane, &projp2);
		lw_dist3d_pt_tri(&p2, tri, plane, &projp2, dl);
		if ((s2 == 0.0) && (dl->distance < dl->tolerance))
			return LW_TRUE;

		/* If s1 and s2 has different signs that means they are on different sides of the plane of the triangle.
		 * That means that the edge between the points crosses the plane and might intersect with the triangle
		 */
		if ((s1 * s2) < 0)
		{
			/* The size of s1 and s2 is the distance from the point to the plane. */
			f = fabs(s1) / (fabs(s1) + fabs(s2));
			get_3dvector_from_points(&projp1, &projp2, &projp1_projp2);

			/* Get the point where the line segment crosses the plane */
			intersectionp.x = projp1.x + f * projp1_projp2.x;
			intersectionp.y = projp1.y + f * projp1_projp2.y;
			intersectionp.z = projp1.z + f * projp1_projp2.z;

			/* We set intersects to true until the opposite is proved */
			intersects = LW_TRUE;

			if (pt_in_ring_3d(&intersectionp, tri->points, plane)) /*Inside outer ring*/
			{
				if (intersects)
				{
					dl->distance = 0.0;
					dl->p1.x = intersectionp.x;
					dl->p1.y = intersectionp.y;
					dl->p1.z = intersectionp.z;

					dl->p2.x = intersectionp.x;
					dl->p2.y = intersectionp.y;
					dl->p2.z = intersectionp.z;
					return LW_TRUE;
				}
			}
		}

		projp1 = projp2;
		s1 = s2;
		p1 = p2;
	}

	/* check our pointarray against triangle contour */
	lw_dist3d_ptarray_ptarray(pa, tri->points, dl);
	return LW_TRUE;
}

/* Here we define the plane of a polygon (boundary pointarray of a polygon)
 * the plane is stored as a point in plane (plane.pop) and a normal vector (plane.pv)
 *
 * We are calculating the average point and using it to break the polygon into
 * POL_BREAKS (3) parts. Then we calculate the normal of those 3 vectors and
 * use its average to define the normal of the plane.This is done to minimize
 * the error introduced by FP precision
 * We use [3] breaks to contemplate the special case of 3d triangles
 */
int
define_plane(POINTARRAY *pa, PLANE3D *pl)
{
	const uint32_t POL_BREAKS = 3;

	assert(pa);
	assert(pa->npoints > 3);
	if (!pa)
		return LW_FALSE;

	uint32_t unique_points = pa->npoints - 1;
	uint32_t i;

	if (pa->npoints < 3)
		return LW_FALSE;

	/* Calculate the average point */
	pl->pop.x = 0.0;
	pl->pop.y = 0.0;
	pl->pop.z = 0.0;
	for (i = 0; i < unique_points; i++)
	{
		POINT3DZ p;
		getPoint3dz_p(pa, i, &p);
		pl->pop.x += p.x;
		pl->pop.y += p.y;
		pl->pop.z += p.z;
	}

	pl->pop.x /= unique_points;
	pl->pop.y /= unique_points;
	pl->pop.z /= unique_points;

	pl->pv.x = 0.0;
	pl->pv.y = 0.0;
	pl->pv.z = 0.0;
	for (i = 0; i < POL_BREAKS; i++)
	{
		POINT3DZ point1, point2;
		VECTOR3D v1, v2, vp;
		uint32_t n1, n2;
		n1 = i * unique_points / POL_BREAKS;
		n2 = n1 + unique_points / POL_BREAKS; /* May overflow back to the first / last point */

		if (n1 == n2)
			continue;

		getPoint3dz_p(pa, n1, &point1);
		if (!get_3dvector_from_points(&pl->pop, &point1, &v1))
			continue;

		getPoint3dz_p(pa, n2, &point2);
		if (!get_3dvector_from_points(&pl->pop, &point2, &v2))
			continue;

		if (get_3dcross_product(&v1, &v2, &vp))
		{
			/* If the cross product isn't zero these 3 points aren't collinear
			 * We divide by its lengthsq to normalize the additions */
			double vl = vp.x * vp.x + vp.y * vp.y + vp.z * vp.z;
			pl->pv.x += vp.x / vl;
			pl->pv.y += vp.y / vl;
			pl->pv.z += vp.z / vl;
		}
	}

	return (!FP_IS_ZERO(pl->pv.x) || !FP_IS_ZERO(pl->pv.y) || !FP_IS_ZERO(pl->pv.z));
}

/**

Finds a point on a plane from where the original point is perpendicular to the plane
*/
double
project_point_on_plane(POINT3DZ *p, PLANE3D *pl, POINT3DZ *p0)
{
	/*In our plane definition we have a point on the plane and a normal vector (pl.pv), perpendicular to the plane
	this vector will be parallel to the line between our inputted point above the plane and the point we are
	searching for on the plane. So, we already have a direction from p to find p0, but we don't know the distance.
	*/

	VECTOR3D v1;
	double f;

	if (!get_3dvector_from_points(&(pl->pop), p, &v1))
		return 0.0;

	f = DOT(pl->pv, v1);
	if (FP_IS_ZERO(f))
	{
		/* Point is in the plane */
		*p0 = *p;
		return 0;
	}

	f = -f / DOT(pl->pv, pl->pv);

	p0->x = p->x + pl->pv.x * f;
	p0->y = p->y + pl->pv.y * f;
	p0->z = p->z + pl->pv.z * f;

	return f;
}

/**
 * pt_in_ring_3d(): crossing number test for a point in a polygon
 *      input:   p = a point,
 *               pa = vertex points of a ring V[n+1] with V[n]=V[0]
 *		plane=the plane that the vertex points are lying on
 *      returns: 0 = outside, 1 = inside
 *
 *	Our polygons have first and last point the same,
 *
 *	The difference in 3D variant is that we exclude the dimension that faces the plane least.
 *	That is the dimension with the highest number in pv
 */
int
pt_in_ring_3d(const POINT3DZ *p, const POINTARRAY *ring, PLANE3D *plane)
{

	uint32_t cn = 0; /* the crossing number counter */
	uint32_t i;
	POINT3DZ v1, v2;

	POINT3DZ first, last;

	getPoint3dz_p(ring, 0, &first);
	getPoint3dz_p(ring, ring->npoints - 1, &last);
	if (memcmp(&first, &last, sizeof(POINT3DZ)))
	{
		lwerror("pt_in_ring_3d: V[n] != V[0] (%g %g %g!= %g %g %g)",
			first.x,
			first.y,
			first.z,
			last.x,
			last.y,
			last.z);
		return LW_FALSE;
	}

	LWDEBUGF(2, "pt_in_ring_3d called with point: %g %g %g", p->x, p->y, p->z);
	/* printPA(ring); */

	/* loop through all edges of the polygon */
	getPoint3dz_p(ring, 0, &v1);

	if (fabs(plane->pv.z) >= fabs(plane->pv.x) &&
	    fabs(plane->pv.z) >= fabs(plane->pv.y)) /*If the z vector of the normal vector to the plane is larger than x
						       and y vector we project the ring to the xy-plane*/
	{
		for (i = 0; i < ring->npoints - 1; i++)
		{
			double vt;
			getPoint3dz_p(ring, i + 1, &v2);

			/* edge from vertex i to vertex i+1 */
			if (
			    /* an upward crossing */
			    ((v1.y <= p->y) && (v2.y > p->y))
			    /* a downward crossing */
			    || ((v1.y > p->y) && (v2.y <= p->y)))
			{

				vt = (double)(p->y - v1.y) / (v2.y - v1.y);

				/* P.x <intersect */
				if (p->x < v1.x + vt * (v2.x - v1.x))
				{
					/* a valid crossing of y=p.y right of p.x */
					++cn;
				}
			}
			v1 = v2;
		}
	}
	else if (fabs(plane->pv.y) >= fabs(plane->pv.x) &&
		 fabs(plane->pv.y) >= fabs(plane->pv.z)) /*If the y vector of the normal vector to the plane is larger
							    than x and z vector we project the ring to the xz-plane*/
	{
		for (i = 0; i < ring->npoints - 1; i++)
		{
			double vt;
			getPoint3dz_p(ring, i + 1, &v2);

			/* edge from vertex i to vertex i+1 */
			if (
			    /* an upward crossing */
			    ((v1.z <= p->z) && (v2.z > p->z))
			    /* a downward crossing */
			    || ((v1.z > p->z) && (v2.z <= p->z)))
			{

				vt = (double)(p->z - v1.z) / (v2.z - v1.z);

				/* P.x <intersect */
				if (p->x < v1.x + vt * (v2.x - v1.x))
				{
					/* a valid crossing of y=p.y right of p.x */
					++cn;
				}
			}
			v1 = v2;
		}
	}
	else /*Hopefully we only have the cases where x part of the normal vector is largest left*/
	{
		for (i = 0; i < ring->npoints - 1; i++)
		{
			double vt;
			getPoint3dz_p(ring, i + 1, &v2);

			/* edge from vertex i to vertex i+1 */
			if (
			    /* an upward crossing */
			    ((v1.z <= p->z) && (v2.z > p->z))
			    /* a downward crossing */
			    || ((v1.z > p->z) && (v2.z <= p->z)))
			{

				vt = (double)(p->z - v1.z) / (v2.z - v1.z);

				/* P.x <intersect */
				if (p->y < v1.y + vt * (v2.y - v1.y))
				{
					/* a valid crossing of y=p.y right of p.x */
					++cn;
				}
			}
			v1 = v2;
		}
	}
	LWDEBUGF(3, "pt_in_ring_3d returning %d", cn & 1);

	return (cn & 1); /* 0 if even (out), and 1 if odd (in) */
}

/*------------------------------------------------------------------------------------------------------------
End of Brute force functions
--------------------------------------------------------------------------------------------------------------*/
