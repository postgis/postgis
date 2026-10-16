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
 * Copyright (C) 2010 - Oslandia
 * Copyright (C) 2026 Darafei Praliaskouski <me@komzpa.net>
 *
 **********************************************************************/

/* basic LWTRIANGLE manipulation */

#include <stdio.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "liblwgeom_internal.h"
#include "lwgeom_log.h"



/* construct a new LWTRIANGLE.
 * use SRID=SRID_UNKNOWN for unknown SRID (will have 8bit type's S = 0)
 */
LWTRIANGLE *
lwtriangle_construct(int32_t srid, GBOX *bbox, POINTARRAY *points)
{
	LWTRIANGLE *result;

	result = (LWTRIANGLE*) lwalloc(sizeof(LWTRIANGLE));
	result->type = TRIANGLETYPE;

	result->flags = points->flags;
	FLAGS_SET_BBOX(result->flags, bbox?1:0);

	result->srid = srid;
	result->points = points;
	result->bbox = bbox;

	return result;
}

LWTRIANGLE *
lwtriangle_construct_empty(int32_t srid, char hasz, char hasm)
{
	LWTRIANGLE *result = lwalloc(sizeof(LWTRIANGLE));
	result->type = TRIANGLETYPE;
	result->flags = lwflags(hasz,hasm,0);
	result->srid = srid;
	result->points = ptarray_construct_empty(hasz, hasm, 1);
	result->bbox = NULL;
	return result;
}

void lwtriangle_free(LWTRIANGLE  *triangle)
{
	if ( ! triangle ) return;

	if (triangle->bbox)
		lwfree(triangle->bbox);

	if (triangle->points)
		ptarray_free(triangle->points);

	lwfree(triangle);
}

void printLWTRIANGLE(LWTRIANGLE *triangle)
{
	if (triangle->type != TRIANGLETYPE)
                lwerror("printLWTRIANGLE called with something else than a Triangle");

	lwnotice("LWTRIANGLE {");
	lwnotice("    ndims = %i", (int)FLAGS_NDIMS(triangle->flags));
	lwnotice("    SRID = %i", (int)triangle->srid);
	printPA(triangle->points);
	lwnotice("}");
}

/* @brief Clone LWTRIANGLE object. Serialized point lists are not copied.
 *
 * @see ptarray_clone
 */
LWTRIANGLE *
lwtriangle_clone(const LWTRIANGLE *g)
{
	LWDEBUGF(2, "lwtriangle_clone called with %p", g);
	return (LWTRIANGLE *)lwline_clone((const LWLINE *)g);
}

int
lwtriangle_has_orientation(const LWTRIANGLE *triangle, int orientation)
{
	return ptarray_has_orientation(triangle->points, orientation);
}

void
lwtriangle_force_orientation(LWTRIANGLE *triangle, int orientation)
{
	if (!lwtriangle_has_orientation(triangle, orientation))
		ptarray_reverse_in_place(triangle->points);
}

void
lwtriangle_release(LWTRIANGLE *lwtriangle)
{
	lwgeom_release(lwtriangle_as_lwgeom(lwtriangle));
}

/* check coordinate equality  */
char
lwtriangle_same(const LWTRIANGLE *t1, const LWTRIANGLE *t2)
{
	char r = ptarray_same(t1->points, t2->points);
	LWDEBUGF(5, "returning %d", r);
	return r;
}

static char
lwtriangle_is_repeated_points(LWTRIANGLE *triangle)
{
	char ret;
	POINTARRAY *pa;

	pa = ptarray_remove_repeated_points(triangle->points, 0.0);
	ret = ptarray_same(pa, triangle->points);
	ptarray_free(pa);

	return ret;
}

/*
 * Construct a triangle from a LWLINE being
 * the shell
 * Pointarray from input geom is cloned.
 * Input line must have 4 points, and be closed.
 */
LWTRIANGLE *
lwtriangle_from_lwline(const LWLINE *shell)
{
	LWTRIANGLE *ret;
	POINTARRAY *pa;

	if ( shell->points->npoints != 4 )
		lwerror("lwtriangle_from_lwline: shell must have exactly 4 points");

	if (   (!FLAGS_GET_Z(shell->flags) && !ptarray_is_closed_2d(shell->points)) ||
	        (FLAGS_GET_Z(shell->flags) && !ptarray_is_closed_3d(shell->points)) )
		lwerror("lwtriangle_from_lwline: shell must be closed");

	pa = ptarray_clone_deep(shell->points);
	ret = lwtriangle_construct(shell->srid, NULL, pa);

	if (lwtriangle_is_repeated_points(ret))
		lwerror("lwtriangle_from_lwline: some points are repeated in triangle");

	return ret;
}

/**
 * Find the area of the outer ring
 */
double
lwtriangle_area(const LWTRIANGLE *triangle)
{
	double area=0.0;
	uint32_t i;
	POINT2D p1;
	POINT2D p2;

	if (! triangle->points->npoints) return area; /* empty triangle */

	for (i=0; i < triangle->points->npoints-1; i++)
	{
		getPoint2d_p(triangle->points, i, &p1);
		getPoint2d_p(triangle->points, i+1, &p2);
		area += ( p1.x * p2.y ) - ( p1.y * p2.x );
	}

	area  /= 2.0;

	return fabs(area);
}

typedef enum
{
	LWPLANE_FIND_ORIGIN,
	LWPLANE_FIND_DIRECTION,
	LWPLANE_FIND_NORMAL,
	LWPLANE_VALIDATE
} LWPLANESLOPEPHASE;

typedef struct {
	POINT4D origin;
	POINT4D direction;
	POINT4D normal_point;
	double farthest_distance;
	double nx, ny, nz, nlen, nerror, normal_conditioning;
	LWPLANESLOPEPHASE phase;
	uint8_t has_origin;
	uint8_t has_direction;
	uint8_t has_normal;
} LWPLANESLOPESTATE;

typedef struct {
	double slope;
	uint8_t has_slope;
	uint8_t has_mismatch;
} LWSLOPESTATE;

static double
vec3_length(double x, double y, double z)
{
	return hypot(hypot(x, y), z);
}

static int
lwgeom_point4d_lex_less(const POINT4D *a, const POINT4D *b)
{
	if (a->x != b->x)
		return a->x < b->x;
	if (a->y != b->y)
		return a->y < b->y;
	return a->z < b->z;
}

static void
lwgeom_two_sum(double a, double b, double *sum, double *error)
{
	double bvirt, avirt, bround, around;

	*sum = a + b;
	bvirt = *sum - a;
	avirt = *sum - bvirt;
	bround = b - bvirt;
	around = a - avirt;
	*error = around + bround;
}

static double
lwgeom_product_difference(double a, double b, double c, double d)
{
	double ab, ab_error, cd, cd_error;
	double difference, difference_error;
	double correction, correction_error, correction2, correction2_error;
	double result, result_error;

	ab = a * b;
	ab_error = fma(a, b, -ab);
	cd = c * d;
	cd_error = fma(c, d, -cd);
	lwgeom_two_sum(ab, -cd, &difference, &difference_error);
	lwgeom_two_sum(difference_error, ab_error, &correction, &correction_error);
	lwgeom_two_sum(correction, -cd_error, &correction2, &correction2_error);
	lwgeom_two_sum(difference, correction2, &result, &result_error);
	return result + ((result_error + correction_error) + correction2_error);
}

static int
lwgeom_slope_add_value(LWSLOPESTATE *state, double slope)
{
	if (!state->has_slope)
	{
		state->slope = slope;
		state->has_slope = LW_TRUE;
		return LW_SUCCESS;
	}

	if (fabs(state->slope - slope) > FP_TOLERANCE)
		return LW_FAILURE;

	return LW_SUCCESS;
}

static void
lwgeom_line_grade_add_value(LWSLOPESTATE *state, double slope)
{
	if (lwgeom_slope_add_value(state, slope) == LW_FAILURE)
		state->has_mismatch = LW_TRUE;
}

static int
lwgeom_slope_supported(const LWGEOM *geom)
{
	uint32_t i;
	LWCOLLECTION *col;

	if (!geom)
		return LW_FALSE;

	switch (geom->type)
	{
	case LINETYPE:
	case TRIANGLETYPE:
	case POLYGONTYPE:
		return LW_TRUE;
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case POLYHEDRALSURFACETYPE:
	case TINTYPE:
	case MULTISURFACETYPE:
		col = lwgeom_as_lwcollection(geom);
		for (i = 0; i < col->ngeoms; i++)
		{
			if (!lwgeom_slope_supported(col->geoms[i]))
				return LW_FALSE;
		}
		return LW_TRUE;
	case COLLECTIONTYPE:
		col = lwgeom_as_lwcollection(geom);
		for (i = 0; i < col->ngeoms; i++)
		{
			if (!lwgeom_slope_supported(col->geoms[i]))
				return LW_FALSE;
		}
		return LW_TRUE;
	default:
		return LW_FALSE;
	}
}

int
lwgeom_slope_is_supported(const LWGEOM *geom)
{
	return lwgeom_slope_supported(geom);
}

static void
lwgeom_surface_slope_try_normal(LWPLANESLOPESTATE *state, const POINT4D *p)
{
	double ux, uy, uz, ulen, max_u;
	double vx, vy, vz, vlen, max_v;
	double nx, ny, nz, nlen, condition;
	int exponent_u, exponent_v;

	ux = state->direction.x - state->origin.x;
	uy = state->direction.y - state->origin.y;
	uz = state->direction.z - state->origin.z;
	vx = p->x - state->origin.x;
	vy = p->y - state->origin.y;
	vz = p->z - state->origin.z;
	max_u = fmax(fabs(ux), fmax(fabs(uy), fabs(uz)));
	max_v = fmax(fabs(vx), fmax(fabs(vy), fabs(vz)));
	if (max_u == 0.0 || max_v == 0.0 || !isfinite(max_u) || !isfinite(max_v))
		return;

	/* Power-of-two scaling prevents overflow without changing the vector directions. */
	frexp(max_u, &exponent_u);
	frexp(max_v, &exponent_v);
	ux = scalbn(ux, -exponent_u);
	uy = scalbn(uy, -exponent_u);
	uz = scalbn(uz, -exponent_u);
	vx = scalbn(vx, -exponent_v);
	vy = scalbn(vy, -exponent_v);
	vz = scalbn(vz, -exponent_v);
	ulen = vec3_length(ux, uy, uz);
	vlen = vec3_length(vx, vy, vz);

	/* Compensated product differences preserve small normals of thin triangles. */
	nx = lwgeom_product_difference(uy, vz, uz, vy);
	ny = lwgeom_product_difference(uz, vx, ux, vz);
	nz = lwgeom_product_difference(ux, vy, uy, vx);
	nlen = vec3_length(nx, ny, nz);
	if (nlen == 0.0)
		return;

	condition = nlen / (ulen * vlen);
	if (state->has_normal &&
	    (condition < state->normal_conditioning ||
	     (condition == state->normal_conditioning && !lwgeom_point4d_lex_less(p, &state->normal_point))))
		return;

	state->normal_point = *p;
	state->nx = nx;
	state->ny = ny;
	state->nz = nz;
	state->nlen = nlen;
	state->nerror = 16.0 * DBL_EPSILON * nlen;
	state->normal_conditioning = condition;
	state->has_normal = LW_TRUE;
}

static int
lwgeom_surface_slope_add_point(LWPLANESLOPESTATE *state, const POINT4D *p)
{
	double vx, vy, vz, vlen;
	double plane_residual, plane_roundoff;

	switch (state->phase)
	{
	case LWPLANE_FIND_ORIGIN:
		if (!state->has_origin || lwgeom_point4d_lex_less(p, &state->origin))
		{
			state->origin = *p;
			state->has_origin = LW_TRUE;
		}
		return LW_SUCCESS;

	case LWPLANE_FIND_DIRECTION:
		vx = p->x - state->origin.x;
		vy = p->y - state->origin.y;
		vz = p->z - state->origin.z;
		vlen = vec3_length(vx, vy, vz);
		if (!isfinite(vlen))
			return LW_FAILURE;
		if (vlen == 0.0)
			return LW_SUCCESS;
		if (!state->has_direction || vlen > state->farthest_distance ||
		    (vlen == state->farthest_distance && lwgeom_point4d_lex_less(p, &state->direction)))
		{
			state->direction = *p;
			state->farthest_distance = vlen;
			state->has_direction = LW_TRUE;
		}
		return LW_SUCCESS;

	case LWPLANE_FIND_NORMAL:
		lwgeom_surface_slope_try_normal(state, p);
		return LW_SUCCESS;

	case LWPLANE_VALIDATE:
		vx = p->x - state->origin.x;
		vy = p->y - state->origin.y;
		vz = p->z - state->origin.z;
		vlen = vec3_length(vx, vy, vz);
		plane_residual = fma(state->nx, vx, fma(state->ny, vy, state->nz * vz));
		plane_roundoff =
		    state->nerror * vlen +
		    8.0 * DBL_EPSILON * (fabs(state->nx * vx) + fabs(state->ny * vy) + fabs(state->nz * vz));
		if (fabs(plane_residual) > FP_TOLERANCE * state->nlen * vlen + plane_roundoff)
			return LW_FAILURE;
		return LW_SUCCESS;

	default:
		return LW_FAILURE;
	}
}

static int
lwgeom_surface_slope_add_ptarray(LWPLANESLOPESTATE *state, const POINTARRAY *pa)
{
	uint32_t i;
	const int hasz = FLAGS_GET_Z(pa->flags);

	for (i = 0; i < pa->npoints; i++)
	{
		POINT4D p;
		getPoint4d_p(pa, i, &p);
		if (!hasz)
			p.z = 0.0;

		if (lwgeom_surface_slope_add_point(state, &p) == LW_FAILURE)
			return LW_FAILURE;
	}

	return LW_SUCCESS;
}

static int
lwgeom_surface_slope_add_geom(LWPLANESLOPESTATE *state, const LWGEOM *geom)
{
	uint32_t i;

	switch (geom->type)
	{
	case TRIANGLETYPE:
		return lwgeom_surface_slope_add_ptarray(state, ((const LWTRIANGLE *)geom)->points);

	case POLYGONTYPE: {
		const LWPOLY *poly = (const LWPOLY *)geom;
		for (i = 0; i < poly->nrings; i++)
		{
			if (lwgeom_surface_slope_add_ptarray(state, poly->rings[i]) == LW_FAILURE)
				return LW_FAILURE;
		}
		return LW_SUCCESS;
	}

	case MULTIPOLYGONTYPE:
	case POLYHEDRALSURFACETYPE:
	case TINTYPE:
	case MULTISURFACETYPE:
	case COLLECTIONTYPE: {
		const LWCOLLECTION *col = (const LWCOLLECTION *)geom;
		for (i = 0; i < col->ngeoms; i++)
		{
			if (lwgeom_surface_slope_add_geom(state, col->geoms[i]) == LW_FAILURE)
				return LW_FAILURE;
		}
		return LW_SUCCESS;
	}

	default:
		return LW_FAILURE;
	}
}

static int lwgeom_plane_slope_from_state(const LWPLANESLOPESTATE *state, double *slope);

static int
lwgeom_plane_slope_from_state(const LWPLANESLOPESTATE *state, double *slope)
{
	double normal_horizontal;

	if (!state || !slope || !state->has_normal)
		return LW_FAILURE;

	/*
	 * Use the plane normal so vertex ordering does not matter. A horizontal
	 * plane has a vertical normal and slope 0; a vertical plane has no vertical
	 * normal component and slope pi/2.
	 */
	normal_horizontal = hypot(state->nx, state->ny);
	*slope = atan2(normal_horizontal, fabs(state->nz));
	return LW_SUCCESS;
}

static int
lwgeom_line_grade_from_ptarray(const POINTARRAY *pa, double *slope)
{
	POINT4D first, last;
	double dx, dy, dz, horizontal_distance;

	if (!pa || !slope || pa->npoints < 2)
		return LW_FAILURE;

	getPoint4d_p(pa, 0, &first);
	getPoint4d_p(pa, pa->npoints - 1, &last);

	if (!FLAGS_GET_Z(pa->flags))
	{
		first.z = 0.0;
		last.z = 0.0;
	}

	dx = last.x - first.x;
	dy = last.y - first.y;
	dz = fabs(last.z - first.z);
	horizontal_distance = hypot(dx, dy);

	/* A short nonzero displacement still has a well-defined grade. */
	if (horizontal_distance == 0.0 && dz == 0.0)
		return LW_FAILURE;

	*slope = atan2(dz, horizontal_distance);
	return LW_SUCCESS;
}

static int
lwgeom_line_slope_add_ptarray(LWPLANESLOPESTATE *plane_state, LWSLOPESTATE *grade_state, const POINTARRAY *pa)
{
	double slope;

	if (lwgeom_surface_slope_add_ptarray(plane_state, pa) == LW_FAILURE)
		return LW_FAILURE;

	if (grade_state && lwgeom_line_grade_from_ptarray(pa, &slope) == LW_SUCCESS)
		lwgeom_line_grade_add_value(grade_state, slope);

	return LW_SUCCESS;
}

static int
lwgeom_line_slope_add_geom(LWPLANESLOPESTATE *plane_state, LWSLOPESTATE *grade_state, const LWGEOM *geom)
{
	uint32_t i;

	switch (geom->type)
	{
	case LINETYPE:
		return lwgeom_line_slope_add_ptarray(plane_state, grade_state, ((const LWLINE *)geom)->points);

	case MULTILINETYPE: {
		const LWCOLLECTION *col = (const LWCOLLECTION *)geom;
		for (i = 0; i < col->ngeoms; i++)
		{
			if (lwgeom_line_slope_add_geom(plane_state, grade_state, col->geoms[i]) == LW_FAILURE)
				return LW_FAILURE;
		}
		return LW_SUCCESS;
	}

	default:
		return LW_FAILURE;
	}
}

static int
lwgeom_slope_collect_geom(LWPLANESLOPESTATE *plane_state, LWSLOPESTATE *grade_state, const LWGEOM *geom)
{
	uint32_t i;

	switch (geom->type)
	{
	case LINETYPE:
	case MULTILINETYPE:
		return lwgeom_line_slope_add_geom(plane_state, grade_state, geom);

	case TRIANGLETYPE:
	case POLYGONTYPE:
	case MULTIPOLYGONTYPE:
	case POLYHEDRALSURFACETYPE:
	case TINTYPE:
	case MULTISURFACETYPE:
		return lwgeom_surface_slope_add_geom(plane_state, geom);

	case COLLECTIONTYPE: {
		const LWCOLLECTION *col = (const LWCOLLECTION *)geom;
		for (i = 0; i < col->ngeoms; i++)
		{
			if (lwgeom_slope_collect_geom(plane_state, grade_state, col->geoms[i]) == LW_FAILURE)
				return LW_FAILURE;
		}
		return LW_SUCCESS;
	}

	default:
		return LW_FAILURE;
	}
}

int
lwgeom_slope(const LWGEOM *geom, double *slope)
{
	LWPLANESLOPESTATE plane_state = {0};
	LWSLOPESTATE grade_state = {0};

	if (!geom || !slope || !lwgeom_slope_supported(geom))
		return LW_FAILURE;

	/* Choose canonical vertices before computing a normal so input order cannot choose the plane. */
	plane_state.phase = LWPLANE_FIND_ORIGIN;
	if (lwgeom_slope_collect_geom(&plane_state, &grade_state, geom) == LW_FAILURE)
		return LW_FAILURE;

	if (plane_state.has_origin)
	{
		plane_state.phase = LWPLANE_FIND_DIRECTION;
		if (lwgeom_slope_collect_geom(&plane_state, NULL, geom) == LW_FAILURE)
			return LW_FAILURE;
	}

	if (plane_state.has_direction)
	{
		plane_state.phase = LWPLANE_FIND_NORMAL;
		if (lwgeom_slope_collect_geom(&plane_state, NULL, geom) == LW_FAILURE)
			return LW_FAILURE;
	}

	if (plane_state.has_normal)
	{
		plane_state.phase = LWPLANE_VALIDATE;
		if (lwgeom_slope_collect_geom(&plane_state, NULL, geom) == LW_FAILURE)
			return LW_FAILURE;
		return lwgeom_plane_slope_from_state(&plane_state, slope);
	}

	/* Endpoint grade is only a fallback for collinear linework. */
	if (grade_state.has_slope && !grade_state.has_mismatch)
	{
		*slope = grade_state.slope;
		return LW_SUCCESS;
	}

	return LW_FAILURE;
}

double
lwtriangle_perimeter(const LWTRIANGLE *triangle)
{
	if( triangle->points )
		return ptarray_length(triangle->points);
	else
		return 0.0;
}

double
lwtriangle_perimeter_2d(const LWTRIANGLE *triangle)
{
	if( triangle->points )
		return ptarray_length_2d(triangle->points);
	else
		return 0.0;
}
