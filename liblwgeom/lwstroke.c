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
 * Copyright (C) 2001-2006 Refractions Research Inc.
 * Copyright (C) 2017      Sandro Santilli <strk@kbt.io>
 * Copyright (C) 2018      Daniel Baston <dbaston@gmail.com>
 *
 **********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "../postgis_config.h"

/*#define POSTGIS_DEBUG_LEVEL 3*/

#include "lwgeom_log.h"

#include "liblwgeom_internal.h"

LWGEOM *pta_unstroke(const POINTARRAY *points, int32_t srid);
LWGEOM* lwline_unstroke(const LWLINE *line);
LWGEOM* lwpolygon_unstroke(const LWPOLY *poly);
LWGEOM* lwmline_unstroke(const LWMLINE *mline);
LWGEOM* lwmpolygon_unstroke(const LWMPOLY *mpoly);
LWGEOM* lwcollection_unstroke(const LWCOLLECTION *c);
LWGEOM* lwgeom_unstroke(const LWGEOM *geom);


/*
 * Determines (recursively in the case of collections) whether the geometry
 * contains at least on arc geometry or segment.
 */
int
lwgeom_has_arc(const LWGEOM *geom)
{
	LWCOLLECTION *col;
	uint32_t i;

	LWDEBUG(2, "lwgeom_has_arc called.");

	switch (geom->type)
	{
	case POINTTYPE:
	case LINETYPE:
	case POLYGONTYPE:
	case TRIANGLETYPE:
	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case POLYHEDRALSURFACETYPE:
	case TINTYPE:
		return LW_FALSE;
	case CIRCSTRINGTYPE:
		return LW_TRUE;
	/* It's a collection that MAY contain an arc */
	default:
		col = (LWCOLLECTION *)geom;
		for (i=0; i<col->ngeoms; i++)
		{
			if (lwgeom_has_arc(col->geoms[i]) == LW_TRUE)
				return LW_TRUE;
		}
		return LW_FALSE;
	}
}

int
lwgeom_type_arc(const LWGEOM *geom)
{
	switch (geom->type)
	{
	case COMPOUNDTYPE:
	case CIRCSTRINGTYPE:
	case CURVEPOLYTYPE:
	case MULTISURFACETYPE:
	case MULTICURVETYPE:
		return LW_TRUE;
	default:
		return LW_FALSE;
	}
}

/*******************************************************************************
 * Begin curve segmentize functions
 ******************************************************************************/

static double interpolate_arc(double angle, double a1, double a2, double a3, double zm1, double zm2, double zm3)
{
	LWDEBUGF(4,"angle %.05g a1 %.05g a2 %.05g a3 %.05g zm1 %.05g zm2 %.05g zm3 %.05g",angle,a1,a2,a3,zm1,zm2,zm3);
	/* Counter-clockwise sweep */
	if ( a1 < a2 )
	{
		if ( angle <= a2 )
			return zm1 + (zm2-zm1) * (angle-a1) / (a2-a1);
		else
			return zm2 + (zm3-zm2) * (angle-a2) / (a3-a2);
	}
	/* Clockwise sweep */
	else
	{
		if ( angle >= a2 )
			return zm1 + (zm2-zm1) * (a1-angle) / (a1-a2);
		else
			return zm2 + (zm3-zm2) * (a2-angle) / (a2-a3);
	}
}

/* Compute the angle covered by a single segment such that
 * a given number of segments per quadrant is achieved. */
static double angle_increment_using_segments_per_quad(double tol)
{
	double increment;
	int perQuad = rint(tol);
	// error out if tol != perQuad ? (not-round)
	if ( perQuad != tol )
	{
		lwerror("lwarc_linearize: segments per quadrant must be an integer value, got %.15g", tol, perQuad);
		return -1;
	}
	if ( perQuad < 1 )
	{
		lwerror("lwarc_linearize: segments per quadrant must be at least 1, got %d", perQuad);
		return -1;
	}
	increment = fabs(M_PI_2 / perQuad);
	LWDEBUGF(2, "lwarc_linearize: perQuad:%d, increment:%g (%g degrees)", perQuad, increment, increment*180/M_PI);

	return increment;
}

/* Compute the angle covered by a single quadrant such that
 * the segment deviates from the arc by no more than a given
 * amount. */
static double angle_increment_using_max_deviation(double max_deviation, double radius)
{
	double increment, halfAngle, maxErr;
	if ( max_deviation <= 0 )
	{
		lwerror("lwarc_linearize: max deviation must be bigger than 0, got %.15g", max_deviation);
		return -1;
	}

	/*
	 * Ref: https://en.wikipedia.org/wiki/Sagitta_(geometry)
	 *
	 * An arc "sagitta" (distance between middle point of arc and
	 * middle point of corresponding chord) is defined as:
	 *
	 *   sagitta = radius * ( 1 - cos( angle ) );
	 *
	 * We want our sagitta to be at most "tolerance" long,
	 * and we want to find out angle, so we use the inverse
	 * formula:
	 *
	 *   tol = radius * ( 1 - cos( angle ) );
	 *   1 - cos( angle ) =  tol/radius
	 *   - cos( angle ) =  tol/radius - 1
	 *   cos( angle ) =  - tol/radius + 1
	 *   angle = acos( 1 - tol/radius )
	 *
	 * Constraints: 1.0 - tol/radius must be between -1 and 1
	 * which means tol must be between 0 and 2 times
	 * the radius, which makes sense as you cannot have a
	 * sagitta bigger than twice the radius!
	 *
	 */
	maxErr = max_deviation;
	if ( maxErr > radius * 2 )
	{
		maxErr = radius * 2;
		LWDEBUGF(2,
			 "lwarc_linearize: tolerance %g is too big, "
			 "using arc-max 2 * radius == %g",
			 max_deviation,
			 maxErr);
	}
	do {
		halfAngle = acos( 1.0 - maxErr / radius );
		/* TODO: avoid a loop here, going rather straight to
		 *       a minimum angle value */
		if ( halfAngle != 0 ) break;
		LWDEBUGF(2, "lwarc_linearize: tolerance %g is too small for this arc"
								" to compute approximation angle, doubling it", maxErr);
		maxErr *= 2;
	} while(1);
	increment = 2 * halfAngle;
	LWDEBUGF(2,
		 "lwarc_linearize: maxDiff:%g, radius:%g, halfAngle:%g, increment:%g (%g degrees)",
		 max_deviation,
		 radius,
		 halfAngle,
		 increment,
		 increment * 180 / M_PI);

	return increment;
}

/* Check that a given angle is positive and, if so, take
 * it to be the angle covered by a single segment. */
static double angle_increment_using_max_angle(double tol)
{
	if ( tol <= 0 )
	{
		lwerror("lwarc_linearize: max angle must be bigger than 0, got %.15g", tol);
		return -1;
	}

	return tol;
}


/**
 * Segmentize an arc
 *
 * Does not add the final vertex
 *
 * @param to POINTARRAY to append segmentized vertices to
 * @param p1 first point defining the arc
 * @param p2 second point defining the arc
 * @param p3 third point defining the arc
 * @param tol tolerance, semantic driven by tolerance_type
 * @param tolerance_type see LW_LINEARIZE_TOLERANCE_TYPE
 * @param flags LW_LINEARIZE_FLAGS
 *
 * @return number of points appended (0 if collinear),
 *         or -1 on error (lwerror would be called).
 *
 */
static int
lwarc_linearize(POINTARRAY *to,
                 const POINT4D *p1, const POINT4D *p2, const POINT4D *p3,
                 double tol, LW_LINEARIZE_TOLERANCE_TYPE tolerance_type,
                 int flags)
{
	POINT2D center;
	POINT2D *t1 = (POINT2D*)p1;
	POINT2D *t2 = (POINT2D*)p2;
	POINT2D *t3 = (POINT2D*)p3;
	POINT4D pt;
	int p2_side = 0;
	int clockwise = LW_TRUE;
	double radius; /* Arc radius */
	double increment; /* Angle per segment */
	double angle_shift = 0;
	double a1, a2, a3;
	POINTARRAY *pa;
	int is_circle = LW_FALSE;
	int points_added = 0;
	int reverse = 0;
	int segments = 0;

	LWDEBUG(2, "lwarc_linearize called.");

	LWDEBUGF(2, " curve is CIRCULARSTRING(%.15g %.15f, %.15f %.15f, %.15f %15f)",
		t1->x, t1->y, t2->x, t2->y, t3->x, t3->y);

	p2_side = lw_segment_side(t1, t3, t2);

	LWDEBUGF(2, " p2 side is %d", p2_side);

	/* Force counterclockwise scan if SYMMETRIC operation is requested */
	if ( p2_side == -1 && flags & LW_LINEARIZE_FLAG_SYMMETRIC )
	{
		/* swap p1-p3 */
		t1 = (POINT2D*)p3;
		t3 = (POINT2D*)p1;
		p1 = (POINT4D*)t1;
		p3 = (POINT4D*)t3;
		p2_side = 1;
		reverse = 1;
	}

	radius = lw_arc_center(t1, t2, t3, &center);
	LWDEBUGF(2, " center is POINT(%.15g %.15g) - radius:%g", center.x, center.y, radius);

	/* Matched start/end points imply circle */
	if ( p1->x == p3->x && p1->y == p3->y )
		is_circle = LW_TRUE;

	/* Negative radius signals straight line, p1/p2/p3 are collinear */
	if ( (radius < 0.0 || p2_side == 0) && ! is_circle )
	    return 0;

	/* The side of the p1/p3 line that p2 falls on dictates the sweep
	   direction from p1 to p3. */
	if ( p2_side == -1 )
		clockwise = LW_TRUE;
	else
		clockwise = LW_FALSE;

	/* Compute the increment (angle per segment) depending on
	 * our tolerance type. */
	switch(tolerance_type)
	{
		case LW_LINEARIZE_TOLERANCE_TYPE_SEGS_PER_QUAD:
			increment = angle_increment_using_segments_per_quad(tol);
			break;
		case LW_LINEARIZE_TOLERANCE_TYPE_MAX_DEVIATION:
			increment = angle_increment_using_max_deviation(tol, radius);
			break;
		case LW_LINEARIZE_TOLERANCE_TYPE_MAX_ANGLE:
			increment = angle_increment_using_max_angle(tol);
			break;
		default:
			lwerror("lwarc_linearize: unsupported tolerance type %d", tolerance_type);
			return -1;
	}

	if (increment < 0)
	{
		/* Error occurred in increment calculation somewhere
		 * (lwerror already called)
		 */
		return -1;
	}

	/* Angles of each point that defines the arc section */
	a1 = atan2(p1->y - center.y, p1->x - center.x);
	a2 = atan2(p2->y - center.y, p2->x - center.x);
	a3 = atan2(p3->y - center.y, p3->x - center.x);

	LWDEBUGF(2, "lwarc_linearize A1:%g (%g) A2:%g (%g) A3:%g (%g)",
		a1, a1*180/M_PI, a2, a2*180/M_PI, a3, a3*180/M_PI);

	/* Calculate total arc angle, in radians */
	double total_angle = clockwise ? a1 - a3 : a3 - a1;
	if ( total_angle <= 0 ) total_angle += M_PI * 2;

	/* At extreme tolerance values (very low or very high, depending on
	 * the semantic) we may cause our arc to collapse. In this case,
	 * we want shrink the increment enough so that we get two segments
	 * for a standard arc, or three segments for a complete circle. */
	int min_segs = is_circle ? 3 : 2;
	segments = ceil(total_angle / increment);
	if (segments < min_segs)
	{
		segments = min_segs;
		increment = total_angle / min_segs;
	}

	if ( flags & LW_LINEARIZE_FLAG_SYMMETRIC )
	{
		LWDEBUGF(2, "lwarc_linearize SYMMETRIC requested - total angle %g deg", total_angle * 180 / M_PI);

		if ( flags & LW_LINEARIZE_FLAG_RETAIN_ANGLE )
		{
			/* Number of complete steps */
			segments = trunc(total_angle / increment);

			/* Figure out the angle remainder, i.e. the amount of the angle
			 * that is left after we can take no more complete angle
			 * increments. */
			double angle_remainder = total_angle - (increment * segments);

			/* Shift the starting angle by half of the remainder. This
			 * will have the effect of evenly distributing the remainder
			 * among the first and last segments in the arc. */
			angle_shift = angle_remainder / 2.0;

			LWDEBUGF(2,
				 "lwarc_linearize RETAIN_ANGLE operation requested - "
				 "total angle %g, steps %d, increment %g, remainder %g",
				 total_angle * 180 / M_PI,
				 segments,
				 increment * 180 / M_PI,
				 angle_remainder * 180 / M_PI);
		}
		else
		{
			/* Number of segments in output */
			segments = ceil(total_angle / increment);
			/* Tweak increment to be regular for all the arc */
			increment = total_angle / segments;

			LWDEBUGF(2,
				 "lwarc_linearize SYMMETRIC operation requested - "
				 "total angle %g degrees - LINESTRING(%g %g,%g %g,%g %g) - S:%d -   I:%g",
				 total_angle * 180 / M_PI,
				 p1->x,
				 p1->y,
				 center.x,
				 center.y,
				 p3->x,
				 p3->y,
				 segments,
				 increment * 180 / M_PI);
		}
	}

	/* p2 on left side => clockwise sweep */
	if ( clockwise )
	{
		LWDEBUG(2, " Clockwise sweep");
		increment *= -1;
		angle_shift *= -1;
		/* Adjust a3 down so we can decrement from a1 to a3 cleanly */
		if ( a3 > a1 )
			a3 -= 2.0 * M_PI;
		if ( a2 > a1 )
			a2 -= 2.0 * M_PI;
	}
	/* p2 on right side => counter-clockwise sweep */
	else
	{
		LWDEBUG(2, " Counterclockwise sweep");
		/* Adjust a3 up so we can increment from a1 to a3 cleanly */
		if ( a3 < a1 )
			a3 += 2.0 * M_PI;
		if ( a2 < a1 )
			a2 += 2.0 * M_PI;
	}

	/* Override angles for circle case */
	if( is_circle )
	{
		increment = fabs(increment);
		segments = ceil(total_angle / increment);
		if (segments < 3)
		{
			segments = 3;
			increment = total_angle / 3;
		}
		a3 = a1 + 2.0 * M_PI;
		a2 = a1 + M_PI;
		clockwise = LW_FALSE;
		angle_shift = 0.0;
	}

	LWDEBUGF(2, "lwarc_linearize angle_shift:%g, increment:%g",
		angle_shift * 180/M_PI, increment * 180/M_PI);

	if ( reverse )
	{
		/* Append points in order to a temporary POINTARRAY and
		 * reverse them before writing to the output POINTARRAY. */
		const int capacity = 8; /* TODO: compute exactly ? */
		pa = ptarray_construct_empty(ptarray_has_z(to), ptarray_has_m(to), capacity);
	}
	else
	{
		/* Append points directly to the output POINTARRAY,
		 * starting with p1. */
		pa = to;

		ptarray_append_point(pa, p1, LW_FALSE);
		++points_added;
	}

	/* Sweep from a1 to a3 */
	int seg_start = 1; /* First point is added manually */
	int seg_end = segments;
	if (angle_shift != 0.0)
	{
		/* When we have extra angles we need to add the extra segments at the
		 * start and end that cover those parts of the arc */
		seg_start = 0;
		seg_end = segments + 1;
	}
	LWDEBUGF(2, "a1:%g (%g deg), a3:%g (%g deg), inc:%g, shi:%g, cw:%d",
		a1, a1 * 180 / M_PI, a3, a3 * 180 / M_PI, increment, angle_shift, clockwise);
	for (int s = seg_start; s < seg_end; s++)
	{
		double angle = a1 + increment * s + angle_shift;
		LWDEBUGF(2, " SA: %g ( %g deg )", angle, angle*180/M_PI);
		pt.x = center.x + radius * cos(angle);
		pt.y = center.y + radius * sin(angle);
		pt.z = interpolate_arc(angle, a1, a2, a3, p1->z, p2->z, p3->z);
		pt.m = interpolate_arc(angle, a1, a2, a3, p1->m, p2->m, p3->m);
		ptarray_append_point(pa, &pt, LW_FALSE);
		++points_added;
	}

	/* Ensure the final point is EXACTLY the same as the first for the circular case */
	if ( is_circle )
	{
		ptarray_remove_point(pa, pa->npoints - 1);
		ptarray_append_point(pa, p1, LW_FALSE);
	}

	if ( reverse )
	{
		int i;
		ptarray_append_point(to, p3, LW_FALSE);
		for ( i=pa->npoints; i>0; i-- ) {
			getPoint4d_p(pa, i-1, &pt);
			ptarray_append_point(to, &pt, LW_FALSE);
		}
		ptarray_free(pa);
	}

	return points_added;
}

/*
 * @param icurve input curve
 * @param tol tolerance, semantic driven by tolerance_type
 * @param tolerance_type see LW_LINEARIZE_TOLERANCE_TYPE
 * @param flags see flags in lwarc_linearize
 *
 * @return a newly allocated LWLINE
 */
static LWLINE *
lwcircstring_linearize(const LWCIRCSTRING *icurve, double tol,
                        LW_LINEARIZE_TOLERANCE_TYPE tolerance_type,
                        int flags)
{
	LWLINE *oline;
	POINTARRAY *ptarray;
	uint32_t i, j;
	POINT4D p1, p2, p3, p4;
	int ret;

	LWDEBUGF(2, "lwcircstring_linearize called., dim = %d", icurve->points->flags);

	ptarray = ptarray_construct_empty(FLAGS_GET_Z(icurve->points->flags), FLAGS_GET_M(icurve->points->flags), 64);

	for (i = 2; i < icurve->points->npoints; i+=2)
	{
		LWDEBUGF(3, "lwcircstring_linearize: arc ending at point %d", i);

		getPoint4d_p(icurve->points, i - 2, &p1);
		getPoint4d_p(icurve->points, i - 1, &p2);
		getPoint4d_p(icurve->points, i, &p3);

		ret = lwarc_linearize(ptarray, &p1, &p2, &p3, tol, tolerance_type, flags);
		if ( ret > 0 )
		{
			LWDEBUGF(3, "lwcircstring_linearize: generated %d points", ptarray->npoints);
		}
		else if ( ret == 0 )
		{
			LWDEBUG(3, "lwcircstring_linearize: points are colinear, returning curve points as line");

			for (j = i - 2 ; j < i ; j++)
			{
				getPoint4d_p(icurve->points, j, &p4);
				ptarray_append_point(ptarray, &p4, LW_TRUE);
			}
		}
		else
		{
			/* An error occurred, lwerror should have been called by now */
			ptarray_free(ptarray);
			return NULL;
		}
	}
	getPoint4d_p(icurve->points, icurve->points->npoints-1, &p1);
	ptarray_append_point(ptarray, &p1, LW_FALSE);

	oline = lwline_construct(icurve->srid, NULL, ptarray);
	return oline;
}

/*
 * @param icompound input compound curve
 * @param tol tolerance, semantic driven by tolerance_type
 * @param tolerance_type see LW_LINEARIZE_TOLERANCE_TYPE
 * @param flags see flags in lwarc_linearize
 *
 * @return a newly allocated LWLINE
 */
static LWLINE *
lwcompound_linearize(const LWCOMPOUND *icompound, double tol,
                      LW_LINEARIZE_TOLERANCE_TYPE tolerance_type,
                      int flags)
{
	LWGEOM *geom;
	POINTARRAY *ptarray = NULL;
	LWLINE *tmp = NULL;
	uint32_t i, j;
	POINT4D p;

	LWDEBUG(2, "lwcompound_stroke called.");

	ptarray = ptarray_construct_empty(FLAGS_GET_Z(icompound->flags), FLAGS_GET_M(icompound->flags), 64);

	for (i = 0; i < icompound->ngeoms; i++)
	{
		geom = icompound->geoms[i];
		if (geom->type == CIRCSTRINGTYPE)
		{
			tmp = lwcircstring_linearize((LWCIRCSTRING *)geom, tol, tolerance_type, flags);
			for (j = 0; j < tmp->points->npoints; j++)
			{
				getPoint4d_p(tmp->points, j, &p);
				ptarray_append_point(ptarray, &p, LW_TRUE);
			}
			lwline_free(tmp);
		}
		else if (geom->type == LINETYPE)
		{
			tmp = (LWLINE *)geom;
			for (j = 0; j < tmp->points->npoints; j++)
			{
				getPoint4d_p(tmp->points, j, &p);
				ptarray_append_point(ptarray, &p, LW_TRUE);
			}
		}
		else
		{
			lwerror("%s: Unsupported geometry type: %s", __func__, lwtype_name(geom->type));
			return NULL;
		}
	}

	ptarray_remove_repeated_points_in_place(ptarray, 0.0, 2);
	return lwline_construct(icompound->srid, NULL, ptarray);
}


/*
 * @param icompound input curve polygon
 * @param tol tolerance, semantic driven by tolerance_type
 * @param tolerance_type see LW_LINEARIZE_TOLERANCE_TYPE
 * @param flags see flags in lwarc_linearize
 *
 * @return a newly allocated LWPOLY
 */
static LWPOLY *
lwcurvepoly_linearize(const LWCURVEPOLY *curvepoly, double tol,
                       LW_LINEARIZE_TOLERANCE_TYPE tolerance_type,
                       int flags)
{
	LWPOLY *ogeom;
	LWGEOM *tmp;
	LWLINE *line;
	POINTARRAY **ptarray;
	uint32_t i;

	LWDEBUG(2, "lwcurvepoly_linearize called.");

	ptarray = lwalloc(sizeof(POINTARRAY *)*curvepoly->nrings);

	for (i = 0; i < curvepoly->nrings; i++)
	{
		tmp = curvepoly->rings[i];
		if (tmp->type == CIRCSTRINGTYPE)
		{
			line = lwcircstring_linearize((LWCIRCSTRING *)tmp, tol, tolerance_type, flags);
			ptarray[i] = ptarray_clone_deep(line->points);
			lwline_free(line);
		}
		else if (tmp->type == LINETYPE)
		{
			line = (LWLINE *)tmp;
			ptarray[i] = ptarray_clone_deep(line->points);
		}
		else if (tmp->type == COMPOUNDTYPE)
		{
			line = lwcompound_linearize((LWCOMPOUND *)tmp, tol, tolerance_type, flags);
			ptarray[i] = ptarray_clone_deep(line->points);
			lwline_free(line);
		}
		else
		{
			lwerror("Invalid ring type found in CurvePoly.");
			return NULL;
		}
	}

	ogeom = lwpoly_construct(curvepoly->srid, NULL, curvepoly->nrings, ptarray);
	return ogeom;
}

/* Kept for backward compatibility - TODO: drop */
LWPOLY *
lwcurvepoly_stroke(const LWCURVEPOLY *curvepoly, uint32_t perQuad)
{
		return lwcurvepoly_linearize(curvepoly, perQuad, LW_LINEARIZE_TOLERANCE_TYPE_SEGS_PER_QUAD, 0);
}


/**
 * @param mcurve input compound curve
 * @param tol tolerance, semantic driven by tolerance_type
 * @param tolerance_type see LW_LINEARIZE_TOLERANCE_TYPE
 * @param flags see flags in lwarc_linearize
 *
 * @return a newly allocated LWMLINE
 */
static LWMLINE *
lwmcurve_linearize(const LWMCURVE *mcurve, double tol,
                    LW_LINEARIZE_TOLERANCE_TYPE type,
                    int flags)
{
	LWMLINE *ogeom;
	LWGEOM **lines;
	uint32_t i;

	LWDEBUGF(2, "lwmcurve_linearize called, geoms=%d, dim=%d.", mcurve->ngeoms, FLAGS_NDIMS(mcurve->flags));

	lines = lwalloc(sizeof(LWGEOM *)*mcurve->ngeoms);

	for (i = 0; i < mcurve->ngeoms; i++)
	{
		const LWGEOM *tmp = mcurve->geoms[i];
		if (tmp->type == CIRCSTRINGTYPE)
		{
			lines[i] = (LWGEOM *)lwcircstring_linearize((LWCIRCSTRING *)tmp, tol, type, flags);
		}
		else if (tmp->type == LINETYPE)
		{
			lines[i] = (LWGEOM *)lwline_construct(mcurve->srid, NULL, ptarray_clone_deep(((LWLINE *)tmp)->points));
		}
		else if (tmp->type == COMPOUNDTYPE)
		{
			lines[i] = (LWGEOM *)lwcompound_linearize((LWCOMPOUND *)tmp, tol, type, flags);
		}
		else
		{
			lwerror("Unsupported geometry found in MultiCurve.");
			return NULL;
		}
	}

	ogeom = (LWMLINE *)lwcollection_construct(MULTILINETYPE, mcurve->srid, NULL, mcurve->ngeoms, lines);
	return ogeom;
}

/**
 * @param msurface input multi surface
 * @param tol tolerance, semantic driven by tolerance_type
 * @param tolerance_type see LW_LINEARIZE_TOLERANCE_TYPE
 * @param flags see flags in lwarc_linearize
 *
 * @return a newly allocated LWMPOLY
 */
static LWMPOLY *
lwmsurface_linearize(const LWMSURFACE *msurface, double tol,
                      LW_LINEARIZE_TOLERANCE_TYPE type,
                      int flags)
{
	LWMPOLY *ogeom;
	LWGEOM *tmp;
	LWPOLY *poly;
	LWGEOM **polys;
	POINTARRAY **ptarray;
	uint32_t i, j;

	LWDEBUG(2, "lwmsurface_linearize called.");

	polys = lwalloc(sizeof(LWGEOM *)*msurface->ngeoms);

	for (i = 0; i < msurface->ngeoms; i++)
	{
		tmp = msurface->geoms[i];
		if (tmp->type == CURVEPOLYTYPE)
		{
			polys[i] = (LWGEOM *)lwcurvepoly_linearize((LWCURVEPOLY *)tmp, tol, type, flags);
		}
		else if (tmp->type == POLYGONTYPE)
		{
			poly = (LWPOLY *)tmp;
			ptarray = lwalloc(sizeof(POINTARRAY *)*poly->nrings);
			for (j = 0; j < poly->nrings; j++)
			{
				ptarray[j] = ptarray_clone_deep(poly->rings[j]);
			}
			polys[i] = (LWGEOM *)lwpoly_construct(msurface->srid, NULL, poly->nrings, ptarray);
		}
	}
	ogeom = (LWMPOLY *)lwcollection_construct(MULTIPOLYGONTYPE, msurface->srid, NULL, msurface->ngeoms, polys);
	return ogeom;
}

/**
 * @param collection input geometry collection
 * @param tol tolerance, semantic driven by tolerance_type
 * @param tolerance_type see LW_LINEARIZE_TOLERANCE_TYPE
 * @param flags see flags in lwarc_linearize
 *
 * @return a newly allocated LWCOLLECTION
 */
static LWCOLLECTION *
lwcollection_linearize(const LWCOLLECTION *collection, double tol,
                    LW_LINEARIZE_TOLERANCE_TYPE type,
                    int flags)
{
	LWCOLLECTION *ocol;
	LWGEOM *tmp;
	LWGEOM **geoms;
	uint32_t i;

	LWDEBUG(2, "lwcollection_linearize called.");

	geoms = lwalloc(sizeof(LWGEOM *)*collection->ngeoms);

	for (i=0; i<collection->ngeoms; i++)
	{
		tmp = collection->geoms[i];
		switch (tmp->type)
		{
		case CIRCSTRINGTYPE:
			geoms[i] = (LWGEOM *)lwcircstring_linearize((LWCIRCSTRING *)tmp, tol, type, flags);
			break;
		case COMPOUNDTYPE:
			geoms[i] = (LWGEOM *)lwcompound_linearize((LWCOMPOUND *)tmp, tol, type, flags);
			break;
		case CURVEPOLYTYPE:
			geoms[i] = (LWGEOM *)lwcurvepoly_linearize((LWCURVEPOLY *)tmp, tol, type, flags);
			break;
		case MULTICURVETYPE:
		case MULTISURFACETYPE:
		case COLLECTIONTYPE:
			geoms[i] = (LWGEOM *)lwcollection_linearize((LWCOLLECTION *)tmp, tol, type, flags);
			break;
		default:
			geoms[i] = lwgeom_clone_deep(tmp);
			break;
		}
	}
	ocol = lwcollection_construct(COLLECTIONTYPE, collection->srid, NULL, collection->ngeoms, geoms);
	return ocol;
}

LWGEOM *
lwcurve_linearize(const LWGEOM *geom, double tol,
                  LW_LINEARIZE_TOLERANCE_TYPE type,
                  int flags)
{
	LWGEOM * ogeom = NULL;
	switch (geom->type)
	{
	case CIRCSTRINGTYPE:
		ogeom = (LWGEOM *)lwcircstring_linearize((LWCIRCSTRING *)geom, tol, type, flags);
		break;
	case COMPOUNDTYPE:
		ogeom = (LWGEOM *)lwcompound_linearize((LWCOMPOUND *)geom, tol, type, flags);
		break;
	case CURVEPOLYTYPE:
		ogeom = (LWGEOM *)lwcurvepoly_linearize((LWCURVEPOLY *)geom, tol, type, flags);
		break;
	case MULTICURVETYPE:
		ogeom = (LWGEOM *)lwmcurve_linearize((LWMCURVE *)geom, tol, type, flags);
		break;
	case MULTISURFACETYPE:
		ogeom = (LWGEOM *)lwmsurface_linearize((LWMSURFACE *)geom, tol, type, flags);
		break;
	case COLLECTIONTYPE:
		ogeom = (LWGEOM *)lwcollection_linearize((LWCOLLECTION *)geom, tol, type, flags);
		break;
	default:
		ogeom = lwgeom_clone_deep(geom);
	}
	return ogeom;
}

/* Kept for backward compatibility - TODO: drop */
LWGEOM *
lwgeom_stroke(const LWGEOM *geom, uint32_t perQuad)
{
	return lwcurve_linearize(geom, perQuad, LW_LINEARIZE_TOLERANCE_TYPE_SEGS_PER_QUAD, 0);
}

/**
 * Return ABC angle in radians
 * TODO: move to lwalgorithm
 */
static double
lw_arc_angle(const POINT2D *a, const POINT2D *b, const POINT2D *c)
{
  POINT2D ab, cb;

  ab.x = b->x - a->x;
  ab.y = b->y - a->y;

  cb.x = b->x - c->x;
  cb.y = b->y - c->y;

  double dot = (ab.x * cb.x + ab.y * cb.y); /* dot product */
  double cross = (ab.x * cb.y - ab.y * cb.x); /* cross product */

  double alpha = atan2(cross, dot);

  return alpha;
}

/**
* Returns LW_TRUE if b is on the arc formed by a1/a2/a3, but not within
* that portion already described by a1/a2/a3
*/
static int pt_continues_arc(const POINT4D *a1, const POINT4D *a2, const POINT4D *a3, const POINT4D *b)
{
	POINT2D center;
	POINT2D *t1 = (POINT2D*)a1;
	POINT2D *t2 = (POINT2D*)a2;
	POINT2D *t3 = (POINT2D*)a3;
	POINT2D *tb = (POINT2D*)b;
	double radius = lw_arc_center(t1, t2, t3, &center);
	double b_distance, diff;

	/* Co-linear a1/a2/a3 */
	if ( radius < 0.0 )
		return LW_FALSE;

	b_distance = distance2d_pt_pt(tb, &center);
	diff = fabs(radius - b_distance);
	LWDEBUGF(4, "circle_radius=%g, b_distance=%g, diff=%g, percentage=%g", radius, b_distance, diff, diff/radius);

	/* Is the point b on the circle? */
	if ( diff < EPSILON_SQLMM )
	{
		int a2_side = lw_segment_side(t1, t3, t2);
		int b_side  = lw_segment_side(t1, t3, tb);
		double angle1 = lw_arc_angle(t1, t2, t3);
		double angle2 = lw_arc_angle(t2, t3, tb);

		/* Is the angle similar to the previous one ? */
		diff = fabs(angle1 - angle2);
		LWDEBUGF(4, " angle1: %g, angle2: %g, diff:%g", angle1, angle2, diff);
		if ( diff > EPSILON_SQLMM )
		{
			return LW_FALSE;
		}

		/* Is the point b on the same side of a1/a3 as the mid-point a2 is? */
		/* If not, it's in the unbounded part of the circle, so it continues the arc, return true. */
		if ( b_side != a2_side )
			return LW_TRUE;
	}
	return LW_FALSE;
}

static LWGEOM *
linestring_from_pa(const POINTARRAY *pa, int32_t srid, int start, int end)
{
	int i = 0, j = 0;
	POINT4D p;
	POINTARRAY *pao = ptarray_construct(ptarray_has_z(pa), ptarray_has_m(pa), end-start+2);
	LWDEBUGF(4, "srid=%d, start=%d, end=%d", srid, start, end);
	for( i = start; i < end + 2; i++ )
	{
		getPoint4d_p(pa, i, &p);
		ptarray_set_point4d(pao, j++, &p);
	}
	return lwline_as_lwgeom(lwline_construct(srid, NULL, pao));
}

static LWGEOM *
circstring_from_pa(const POINTARRAY *pa, int32_t srid, int start, int end)
{

	POINT4D p0, p1, p2;
	POINTARRAY *pao = ptarray_construct(ptarray_has_z(pa), ptarray_has_m(pa), 3);
	LWDEBUGF(4, "srid=%d, start=%d, end=%d", srid, start, end);
	getPoint4d_p(pa, start, &p0);
	ptarray_set_point4d(pao, 0, &p0);
	getPoint4d_p(pa, (start+end+1)/2, &p1);
	ptarray_set_point4d(pao, 1, &p1);
	getPoint4d_p(pa, end+1, &p2);
	ptarray_set_point4d(pao, 2, &p2);
	return lwcircstring_as_lwgeom(lwcircstring_construct(srid, NULL, pao));
}

static LWGEOM *
geom_from_pa(const POINTARRAY *pa, int32_t srid, int is_arc, int start, int end)
{
	LWDEBUGF(4, "srid=%d, is_arc=%d, start=%d, end=%d", srid, is_arc, start, end);
	if ( is_arc )
		return circstring_from_pa(pa, srid, start, end);
	else
		return linestring_from_pa(pa, srid, start, end);
}

LWGEOM *
pta_unstroke(const POINTARRAY *points, int32_t srid)
{
	int i = 0, j, k;
	POINT4D a1, a2, a3, b;
	POINT4D first, center;
	char *edges_in_arcs;
	int found_arc = LW_FALSE;
	int current_arc = 1;
	int num_edges;
	int edge_type; /* non-zero if edge is part of an arc */
	int start, end;
	LWCOLLECTION *outcol;
	/* Minimum number of edges, per quadrant, required to define an arc */
	const unsigned int min_quad_edges = 2;

	/* Die on null input */
	if ( ! points )
		lwerror("pta_unstroke called with null pointarray");

	/* Null on empty input? */
	if ( points->npoints == 0 )
		return NULL;

	/* We can't desegmentize anything shorter than four points */
	if ( points->npoints < 4 )
	{
		/* Return a linestring here*/
		lwerror("pta_unstroke needs implementation for npoints < 4");
	}

	/* Allocate our result array of vertices that are part of arcs */
	num_edges = points->npoints - 1;
	edges_in_arcs = lwalloc(num_edges + 1);
	memset(edges_in_arcs, 0, num_edges + 1);

	/* We make a candidate arc of the first two edges, */
	/* And then see if the next edge follows it */
	while( i < num_edges-2 )
	{
		unsigned int arc_edges;
		double num_quadrants;
		double angle;

		found_arc = LW_FALSE;
		/* Make candidate arc */
		getPoint4d_p(points, i  , &a1);
		getPoint4d_p(points, i+1, &a2);
		getPoint4d_p(points, i+2, &a3);
		memcpy(&first, &a1, sizeof(POINT4D));

		for( j = i+3; j < num_edges+1; j++ )
		{
			LWDEBUGF(4, "i=%d, j=%d", i, j);
			getPoint4d_p(points, j, &b);
			/* Does this point fall on our candidate arc? */
			if ( pt_continues_arc(&a1, &a2, &a3, &b) )
			{
				/* Yes. Mark this edge and the two preceding it as arc components */
				LWDEBUGF(4, "pt_continues_arc #%d", current_arc);
				found_arc = LW_TRUE;
				for ( k = j-1; k > j-4; k-- )
					edges_in_arcs[k] = current_arc;
			}
			else
			{
				/* No. So we're done with this candidate arc */
				LWDEBUG(4, "pt_continues_arc = false");
				current_arc++;
				break;
			}

			memcpy(&a1, &a2, sizeof(POINT4D));
			memcpy(&a2, &a3, sizeof(POINT4D));
			memcpy(&a3,  &b, sizeof(POINT4D));
		}
		/* Jump past all the edges that were added to the arc */
		if ( found_arc )
		{
			/* Check if an arc was composed by enough edges to be
			 * really considered an arc
			 * See http://trac.osgeo.org/postgis/ticket/2420
			 */
			arc_edges = j - 1 - i;
			LWDEBUGF(4, "arc defined by %d edges found", arc_edges);
			if ( first.x == b.x && first.y == b.y ) {
				LWDEBUG(4, "arc is a circle");
				num_quadrants = 4;
			}
			else {
				lw_arc_center((POINT2D*)&first, (POINT2D*)&b, (POINT2D*)&a1, (POINT2D*)&center);
				angle = lw_arc_angle((POINT2D*)&first, (POINT2D*)&center, (POINT2D*)&b);
				int p2_side = lw_segment_side((POINT2D*)&first, (POINT2D*)&a1, (POINT2D*)&b);
				if ( p2_side >= 0 ) angle = -angle;

				if ( angle < 0 ) angle = 2 * M_PI + angle;
				num_quadrants = ( 4 * angle ) / ( 2 * M_PI );
				LWDEBUGF(4, "arc angle (%g %g, %g %g, %g %g) is %g (side is %d), quadrants:%g", first.x, first.y, center.x, center.y, b.x, b.y, angle, p2_side, num_quadrants);
			}
			/* a1 is first point, b is last point */
			if ( arc_edges < min_quad_edges * num_quadrants ) {
				LWDEBUGF(4, "Not enough edges for a %g quadrants arc, %g needed", num_quadrants, min_quad_edges * num_quadrants);
				for ( k = j-1; k >= i; k-- )
					edges_in_arcs[k] = 0;
			}

			i = j-1;
		}
		else
		{
			/* Mark this edge as a linear edge */
			edges_in_arcs[i] = 0;
			i = i+1;
		}
	}

#if POSTGIS_DEBUG_LEVEL > 3
	{
		char *edgestr = lwalloc(num_edges+1);
		for ( i = 0; i < num_edges; i++ )
		{
			if ( edges_in_arcs[i] )
				edgestr[i] = 48 + edges_in_arcs[i];
			else
				edgestr[i] = '.';
		}
		edgestr[num_edges] = 0;
		LWDEBUGF(3, "edge pattern %s", edgestr);
		lwfree(edgestr);
	}
#endif

	start = 0;
	edge_type = edges_in_arcs[0];
	outcol = lwcollection_construct_empty(COMPOUNDTYPE, srid, ptarray_has_z(points), ptarray_has_m(points));
	for( i = 1; i < num_edges; i++ )
	{
		if( edge_type != edges_in_arcs[i] )
		{
			end = i - 1;
			lwcollection_add_lwgeom(outcol, geom_from_pa(points, srid, edge_type, start, end));
			start = i;
			edge_type = edges_in_arcs[i];
		}
	}
	lwfree(edges_in_arcs); /* not needed anymore */

	/* Roll out last item */
	end = num_edges - 1;
	lwcollection_add_lwgeom(outcol, geom_from_pa(points, srid, edge_type, start, end));

	/* Strip down to singleton if only one entry */
	if ( outcol->ngeoms == 1 )
	{
		LWGEOM *outgeom = outcol->geoms[0];
		outcol->ngeoms = 0; lwcollection_free(outcol);
		return outgeom;
	}
	return lwcollection_as_lwgeom(outcol);
}


LWGEOM *
lwline_unstroke(const LWLINE *line)
{
	LWDEBUG(2, "lwline_unstroke called.");

	if ( line->points->npoints < 4 ) return lwline_as_lwgeom(lwline_clone_deep(line));
	else return pta_unstroke(line->points, line->srid);
}

LWGEOM *
lwpolygon_unstroke(const LWPOLY *poly)
{
	LWGEOM **geoms;
	uint32_t i, hascurve = 0;

	LWDEBUG(2, "lwpolygon_unstroke called.");

	geoms = lwalloc(sizeof(LWGEOM *)*poly->nrings);
	for (i=0; i<poly->nrings; i++)
	{
		geoms[i] = pta_unstroke(poly->rings[i], poly->srid);
		if (geoms[i]->type == CIRCSTRINGTYPE || geoms[i]->type == COMPOUNDTYPE)
		{
			hascurve = 1;
		}
	}
	if (hascurve == 0)
	{
		for (i=0; i<poly->nrings; i++)
		{
			lwfree(geoms[i]); /* TODO: should this be lwgeom_free instead ? */
		}
		return lwgeom_clone_deep((LWGEOM *)poly);
	}

	return (LWGEOM *)lwcollection_construct(CURVEPOLYTYPE, poly->srid, NULL, poly->nrings, geoms);
}

LWGEOM *
lwmline_unstroke(const LWMLINE *mline)
{
	LWGEOM **geoms;
	uint32_t i, hascurve = 0;

	LWDEBUG(2, "lwmline_unstroke called.");

	geoms = lwalloc(sizeof(LWGEOM *)*mline->ngeoms);
	for (i=0; i<mline->ngeoms; i++)
	{
		geoms[i] = lwline_unstroke((LWLINE *)mline->geoms[i]);
		if (geoms[i]->type == CIRCSTRINGTYPE || geoms[i]->type == COMPOUNDTYPE)
		{
			hascurve = 1;
		}
	}
	if (hascurve == 0)
	{
		for (i=0; i<mline->ngeoms; i++)
		{
			lwfree(geoms[i]); /* TODO: should this be lwgeom_free instead ? */
		}
		return lwgeom_clone_deep((LWGEOM *)mline);
	}
	return (LWGEOM *)lwcollection_construct(MULTICURVETYPE, mline->srid, NULL, mline->ngeoms, geoms);
}

LWGEOM *
lwmpolygon_unstroke(const LWMPOLY *mpoly)
{
	LWGEOM **geoms;
	uint32_t i, hascurve = 0;

	LWDEBUG(2, "lwmpoly_unstroke called.");

	geoms = lwalloc(sizeof(LWGEOM *)*mpoly->ngeoms);
	for (i=0; i<mpoly->ngeoms; i++)
	{
		geoms[i] = lwpolygon_unstroke((LWPOLY *)mpoly->geoms[i]);
		if (geoms[i]->type == CURVEPOLYTYPE)
		{
			hascurve = 1;
		}
	}
	if (hascurve == 0)
	{
		for (i=0; i<mpoly->ngeoms; i++)
		{
			lwfree(geoms[i]); /* TODO: should this be lwgeom_free instead ? */
		}
		return lwgeom_clone_deep((LWGEOM *)mpoly);
	}
	return (LWGEOM *)lwcollection_construct(MULTISURFACETYPE, mpoly->srid, NULL, mpoly->ngeoms, geoms);
}

LWGEOM *
lwcollection_unstroke(const LWCOLLECTION *c)
{
	LWCOLLECTION *ret = lwalloc(sizeof(LWCOLLECTION));
	memcpy(ret, c, sizeof(LWCOLLECTION));

	if (c->ngeoms > 0)
	{
		uint32_t i;
		ret->geoms = lwalloc(sizeof(LWGEOM *)*c->ngeoms);
		for (i=0; i < c->ngeoms; i++)
		{
			ret->geoms[i] = lwgeom_unstroke(c->geoms[i]);
		}
		if (c->bbox)
		{
			ret->bbox = gbox_copy(c->bbox);
		}
	}
	else
	{
		ret->bbox = NULL;
		ret->geoms = NULL;
	}
	return (LWGEOM *)ret;
}


LWGEOM *
lwgeom_unstroke(const LWGEOM *geom)
{
	LWDEBUG(2, "lwgeom_unstroke called.");

	switch (geom->type)
	{
	case LINETYPE:
		return lwline_unstroke((LWLINE *)geom);
	case POLYGONTYPE:
		return lwpolygon_unstroke((LWPOLY *)geom);
	case MULTILINETYPE:
		return lwmline_unstroke((LWMLINE *)geom);
	case MULTIPOLYGONTYPE:
		return lwmpolygon_unstroke((LWMPOLY *)geom);
	case COLLECTIONTYPE:
		return lwcollection_unstroke((LWCOLLECTION *)geom);
	default:
		return lwgeom_clone_deep(geom);
	}
}

