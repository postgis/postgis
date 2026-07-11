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
 * Copyright 2001-2003 Refractions Research Inc.
 * Copyright 2026 Darafei Praliaskouski <me@komzpa.net>
 *
 **********************************************************************/

/** @file
*
* SVG output routines.
* Originally written by: Klaus F�rster <klaus@svg.cc>
* Refactored by: Olivier Courtin (Camptocamp)
*
* BNF SVG Path: <http://www.w3.org/TR/SVG/paths.html#PathDataBNF>
**********************************************************************/

#include "liblwgeom_internal.h"
#include "stringbuffer.h"


static void
assvg_geom(stringbuffer_t* sb, const LWGEOM *geom, int relative, int precision);


static void
pointArray_svg_rel(stringbuffer_t* sb, const POINTARRAY *pa, int close_ring, int precision, int start_at_index)
{
	int i, end;
	const POINT2D *pt;

	double f = 1.0;
	double dx, dy, x, y, accum_x, accum_y;

	char sx[OUT_DOUBLE_BUFFER_SIZE];
	char sy[OUT_DOUBLE_BUFFER_SIZE];

	if (precision >= 0)
	{
		f = pow(10, precision);
	}

	end = close_ring ? pa->npoints : pa->npoints - 1;

	/* Starting point */
	pt = getPoint2d_cp(pa, start_at_index);

	x = round(pt->x*f)/f;
	y = round(pt->y*f)/f;

	lwprint_double(x, precision, sx);
	lwprint_double(-y, precision, sy);

	stringbuffer_aprintf(sb, "%s %s l", sx, sy);

	/* accum */
	accum_x = x;
	accum_y = y;

	/* All the following ones */
	for (i = (start_at_index + 1); i < end; i++)
	{
		pt = getPoint2d_cp(pa, i);

		x = round(pt->x*f)/f;
		y = round(pt->y*f)/f;

		dx = x - accum_x;
		dy = y - accum_y;

		accum_x += dx;
		accum_y += dy;

		lwprint_double(dx, precision, sx);
		lwprint_double(-dy, precision, sy);
		stringbuffer_aprintf(sb, " %s %s", sx, sy);
	}
}


/**
 * Returns maximum size of rendered pointarray in bytes.
 */
static void
pointArray_svg_abs(stringbuffer_t* sb, const POINTARRAY *pa, int close_ring, int precision, int start_at_index)
{
	int i, end;
	const POINT2D* pt;
	char sx[OUT_DOUBLE_BUFFER_SIZE];
	char sy[OUT_DOUBLE_BUFFER_SIZE];

	end = close_ring ? pa->npoints : pa->npoints - 1;

	for (i = start_at_index; i < end; i++)
	{
		pt = getPoint2d_cp(pa, i);

		if (i == 1)
		{
			if (start_at_index > 0 ){
					stringbuffer_append(sb, "L ");
			}
			else {
					stringbuffer_append(sb, " L ");
			}
		}
		else if (i) stringbuffer_append(sb, " ");

		lwprint_double(pt->x, precision, sx);
		lwprint_double(-(pt->y), precision, sy);

		stringbuffer_aprintf(sb, "%s %s", sx, sy);
	}
}


static void
assvg_point(stringbuffer_t* sb, const LWPOINT *point, int circle, int precision)
{
	char sx[OUT_DOUBLE_BUFFER_SIZE];
	char sy[OUT_DOUBLE_BUFFER_SIZE];
	if ( !lwgeom_is_empty((LWGEOM*)point) ){
		const POINT2D* pt = getPoint2d_cp(point->point, 0);
		lwprint_double(pt->x, precision, sx);
		lwprint_double(-(pt->y), precision, sy);

		stringbuffer_aprintf(sb,
			circle ? "x=\"%s\" y=\"%s\"" : "cx=\"%s\" cy=\"%s\"",
			sx, sy);
	}
}


static void
assvg_line(stringbuffer_t* sb, const LWLINE *line, int relative, int precision)
{
	/* Start path with SVG MoveTo */
	stringbuffer_append(sb, "M ");
	if (relative)
		pointArray_svg_rel(sb, line->points, 1, precision, 0);
	else
		pointArray_svg_abs(sb, line->points, 1, precision, 0);
}

static void
pointArray_svg_arc(stringbuffer_t *sb, const POINTARRAY *pa, int include_start, int relative, int precision)
{
	uint32_t i; //, end;
	char sx[OUT_DOUBLE_BUFFER_SIZE];
	char sy[OUT_DOUBLE_BUFFER_SIZE];
	char sz[OUT_DOUBLE_BUFFER_SIZE];

	LWDEBUG(2, "pointArray_svg_arc called.");

	for (i = 2; i < pa->npoints; i+=2)
	{
		LWDEBUGF(3, "assvg_circstring: arc ending at point %d", i);
		int largeArcFlag, sweepFlag, clockwise;
		int is_circle = LW_FALSE;
		double a1, a3;/**angles**/
		double radius; /* Arc radius */
		double total_angle;
		POINT2D center;
		const POINT2D *t1;
		const POINT2D *t2;
		const POINT2D *t3;
		int p2_side = 0;
		t1 = getPoint2d_cp(pa, i - 2);
		t2 = getPoint2d_cp(pa, i - 1);
		t3 = getPoint2d_cp(pa, i);
		radius = lw_arc_center(t1, t2, t3, &center);
		if (radius < 0.0)
		{
			/* SVG arc radii must be non-negative. A collinear circular arc
			 * is linear, but keep its control point so the degenerate path is
			 * represented exactly even when it lies outside t1-t3. */
			if ((i == 2) && include_start)
			{
				lwprint_double(t1->x, precision, sx);
				lwprint_double(-(t1->y), precision, sy);
				stringbuffer_aprintf(sb, "%s %s", sx, sy);
			}
			if (relative)
			{
				lwprint_double(t2->x - t1->x, precision, sx);
				lwprint_double(-(t2->y - t1->y), precision, sy);
				stringbuffer_aprintf(sb, (i == 2 && !include_start) ? "l %s %s" : " l %s %s", sx, sy);
				lwprint_double(t3->x - t2->x, precision, sx);
				lwprint_double(-(t3->y - t2->y), precision, sy);
			}
			else
			{
				lwprint_double(t2->x, precision, sx);
				lwprint_double(-(t2->y), precision, sy);
				stringbuffer_aprintf(sb, (i == 2 && !include_start) ? "L %s %s" : " L %s %s", sx, sy);
				lwprint_double(t3->x, precision, sx);
				lwprint_double(-(t3->y), precision, sy);
			}
			stringbuffer_aprintf(sb, " %s %s", sx, sy);
			continue;
		}
		if ( t1->x == t3->x && t1->y == t3->y ){
			is_circle = LW_TRUE;
		}
		p2_side = lw_segment_side(t1, t3, t2);
		if ( p2_side == -1 )
			clockwise = LW_TRUE;
		else
			clockwise = LW_FALSE;
		/* Angles of each point that defines the arc section */
		a1 = atan2(t1->y - center.y, t1->x - center.x)*180/M_PI;
		//a2 = atan2(t2->y - center.y, t2->x - center.x)*180/M_PI;
		a3 = atan2(t3->y - center.y, t3->x - center.x)*180/M_PI;

		LWDEBUGF(2, " center is POINT(%.15g %.15g) - radius:%g", center.x, center.y, radius);

		total_angle = clockwise ? a1 - a3 : a3 - a1;
		if (total_angle < 0 ){
			total_angle += 360;
		}

		//stringbuffer_aprintf(sb, "angles (a1 a2 a3): %g %g %g is_circle: %d, total_angle: %g, t1.x: %f, t3.x: %f, t1.y: %f, t3.y: %f ", a1, a2, a3, is_circle, total_angle, t1->x, t3->x, t1->y, t3->y);

		/** endAngle - startAngle <= 180 ? "0" : "1" **/
		largeArcFlag = (total_angle <= 180)? 0 : 1;
		/* The side of the t1/t3 line that t2 falls on dictates the sweep
		direction from t1 to t3. */
		sweepFlag = (p2_side == -1) ? 1 : 0;
		if ((i == 2) && !is_circle && include_start)
		{
			/** add MoveTo first point **/
			lwprint_double(t1->x, precision, sx);
			lwprint_double(-(t1->y), precision, sy);
			stringbuffer_aprintf(sb, "%s %s", sx, sy);
		}
		/** is circle: need to start at center of circle **/
		if ((i == 2) && is_circle && include_start)
		{
			/** add MoveTo center of circle **/
			lwprint_double(center.x, precision, sx);
			lwprint_double(-(center.y), precision, sy);
			stringbuffer_aprintf(sb, "%s %s", sx, sy);
		}
		lwprint_double(radius, precision, sx);
		lwprint_double(0, precision, sy);
		/** is circle need to handle differently **/
		if (is_circle){
			if (include_start)
			{
				// https://stackoverflow.com/questions/5737975/circle-drawing-with-svgs-arc-path
				lwprint_double(radius * 2, precision, sy);
				stringbuffer_aprintf(sb, " m %s 0 a %s %s 0 1 0 -%s 0", sx, sx, sx, sy);
				stringbuffer_aprintf(sb, " a %s %s 0 1 0 %s 0", sx, sx, sy);
			}
			else
			{
				/* A compound component starts at t1 already. Draw the circle
				 * through the opposite point and back without moving the path. */
				lwprint_double(2 * (center.x - t1->x), precision, sy);
				lwprint_double(-2 * (center.y - t1->y), precision, sz);
				stringbuffer_aprintf(sb, "a %s %s 0 1 0 %s %s", sx, sx, sy, sz);
				lwprint_double(-2 * (center.x - t1->x), precision, sy);
				lwprint_double(2 * (center.y - t1->y), precision, sz);
				stringbuffer_aprintf(sb, " a %s %s 0 1 0 %s %s", sx, sx, sy, sz);
			}
		}
		else {
			/* Arc radius radius 0 arcflag swipeflag */
			if (relative){
				stringbuffer_aprintf(sb,
						     (i == 2 && !include_start) ? "a %s %s 0 %d %d "
										: " a %s %s 0 %d %d ",
						     sx,
						     sx,
						     largeArcFlag,
						     sweepFlag);
			}
			else {
				stringbuffer_aprintf(sb,
						     (i == 2 && !include_start) ? "A %s %s 0 %d %d "
										: " A %s %s 0 %d %d ",
						     sx,
						     sx,
						     largeArcFlag,
						     sweepFlag);
			}
			lwprint_double(t3->x, precision, sx);
			lwprint_double(-(t3->y), precision, sy);
			/* end point */
			stringbuffer_aprintf(sb, "%s %s", sx, sy);
		}
	}
}

static void
assvg_circstring(stringbuffer_t* sb, const LWCIRCSTRING *icurve, int relative, int precision)
{
	/* Start path with SVG MoveTo */
	stringbuffer_append(sb, "M ");
	pointArray_svg_arc(sb, icurve->points, 1, relative, precision);
}

static uint32_t
nurbscurve_svg_bezier_degree(const LWNURBSCURVE *curve)
{
	uint32_t degree;

	if (!curve || !curve->points || curve->points->npoints == 0)
		return 0;

	degree = curve->degree;
	if (degree < 1 || degree > 3 || curve->points->npoints != degree + 1)
		return 0;

	/* Equal weights cancel, leaving a polynomial B-spline. */
	for (uint32_t i = 1; i < curve->nweights; i++)
	{
		if (!FP_EQUALS(curve->weights[i], curve->weights[0]))
			return 0;
	}

	/* A single clamped span is exactly a Bezier curve. */
	for (uint32_t i = 1; i <= degree && curve->nknots > 0; i++)
	{
		if (!FP_EQUALS(curve->knots[i], curve->knots[0]) ||
		    !FP_EQUALS(curve->knots[degree + 1 + i], curve->knots[degree + 1]))
			return 0;
	}

	return degree;
}

static int
pointArray_svg_nurbs_bezier(
    stringbuffer_t *sb, const LWNURBSCURVE *curve, int relative, int precision, int include_start)
{
	uint32_t degree = nurbscurve_svg_bezier_degree(curve);
	double x[4], y[4], factor = 1.0;
	char sx[OUT_DOUBLE_BUFFER_SIZE], sy[OUT_DOUBLE_BUFFER_SIZE];

	if (!degree)
		return LW_FALSE;

	if (precision >= 0)
		factor = pow(10, precision);
	for (uint32_t i = 0; i <= degree; i++)
	{
		const POINT2D *point = getPoint2d_cp(curve->points, i);
		x[i] = round(point->x * factor) / factor;
		y[i] = round(point->y * factor) / factor;
	}

	if (include_start)
	{
		lwprint_double(x[0], precision, sx);
		lwprint_double(-y[0], precision, sy);
		stringbuffer_aprintf(sb, "%s %s ", sx, sy);
	}
	stringbuffer_append(sb, relative ? (degree == 1 ? "l" : degree == 2 ? "q" : "c") :
	                                   (degree == 1 ? "L" : degree == 2 ? "Q" : "C"));
	for (uint32_t i = 1; i <= degree; i++)
	{
		lwprint_double(relative ? x[i] - x[0] : x[i], precision, sx);
		lwprint_double(relative ? -(y[i] - y[0]) : -y[i], precision, sy);
		stringbuffer_aprintf(sb, " %s %s", sx, sy);
	}

	return LW_TRUE;
}

static void
assvg_compound(stringbuffer_t* sb, const LWCOMPOUND *icompound, int relative, int precision)
{
	uint32_t i;
	LWGEOM *geom;
	LWCIRCSTRING *tmpc = NULL;
	LWLINE *tmpl = NULL;
	LWLINE *nurbs_line = NULL;
	/* Start path with SVG MoveTo */
	stringbuffer_append(sb, "M ");

	for (i = 0; i < icompound->ngeoms; i++)
	{
		if (i) stringbuffer_append(sb, " ");  /* SVG whitespace Separator */
		geom = icompound->geoms[i];

		switch (geom->type)
		{
			case CIRCSTRINGTYPE:
				tmpc = (LWCIRCSTRING *)geom;
				pointArray_svg_arc(sb, tmpc->points, i == 0, relative, precision);
				break;

			case LINETYPE:
				tmpl = (LWLINE *)geom;

				if (i){
					/** if the compound curve does not start with a line,
					 * we need to skip the first point since it is the same as previous
					 * point of previous curve **/
					if (relative)
						pointArray_svg_rel(sb, tmpl->points, 1, precision, 1);
					else
						pointArray_svg_abs(sb, tmpl->points, 1, precision, 1);
				}
				else {
					if (relative)
						pointArray_svg_rel(sb, tmpl->points, 1, precision, 0);
					else
						pointArray_svg_abs(sb, tmpl->points, 1, precision, 0);
				}
				break;

			case NURBSCURVETYPE:
				if (pointArray_svg_nurbs_bezier(
				        sb, (LWNURBSCURVE *)geom, relative, precision, i == 0))
					break;
				nurbs_line = lwnurbscurve_to_linestring((LWNURBSCURVE *)geom, 32);
				if (!nurbs_line)
				{
					lwerror("Unable to linearize NURBS curve for SVG output");
					return;
				}
				if (relative)
					pointArray_svg_rel(sb, nurbs_line->points, 1, precision, i ? 1 : 0);
				else
					pointArray_svg_abs(sb, nurbs_line->points, 1, precision, i ? 1 : 0);
				lwline_free(nurbs_line);
				break;

			default:
				break; /** in theory this should never happen **/
		}
	}
}

static void
assvg_polygon(stringbuffer_t* sb, const LWPOLY *poly, int relative, int precision)
{
	uint32_t i;

	for (i = 0; i<poly->nrings; i++)
	{
		if (i) stringbuffer_append(sb, " ");	/* Space between each ring */
		stringbuffer_append(sb, "M ");		/* Start path with SVG MoveTo */

		if (relative)
		{
			pointArray_svg_rel(sb, poly->rings[i], 0, precision, 0);
			stringbuffer_append(sb, " z");	/* SVG closepath */
		}
		else
		{
			pointArray_svg_abs(sb, poly->rings[i], 0, precision, 0);
			stringbuffer_append(sb, " Z");	/* SVG closepath */
		}
	}
}

static void
assvg_triangle(stringbuffer_t *sb, const LWTRIANGLE *triangle, int relative, int precision)
{
	stringbuffer_append(sb, "M ");
	if (relative)
	{
		pointArray_svg_rel(sb, triangle->points, 0, precision, 0);
		stringbuffer_append(sb, " z");
	}
	else
	{
		pointArray_svg_abs(sb, triangle->points, 0, precision, 0);
		stringbuffer_append(sb, " Z");
	}
}

static void
assvg_nurbscurve(stringbuffer_t *sb, const LWNURBSCURVE *curve, int relative, int precision)
{
	LWLINE *line;

	if (!curve || !curve->points || curve->points->npoints == 0)
		return;

	if (nurbscurve_svg_bezier_degree(curve))
	{
		stringbuffer_append(sb, "M ");
		pointArray_svg_nurbs_bezier(sb, curve, relative, precision, LW_TRUE);
		return;
	}

	/* SVG has no rational B-spline primitive. Preserve general NURBS by
	 * falling back to the library's sampled approximation. */
	line = lwnurbscurve_to_linestring(curve, 32);
	if (!line)
	{
		lwerror("Unable to linearize NURBS curve for SVG output");
		return;
	}
	assvg_line(sb, line, relative, precision);
	lwline_free(line);
}

static void
assvg_collection_members(stringbuffer_t *sb, const LWCOLLECTION *collection, int relative, int precision)
{
	uint32_t emitted = 0;
	for (uint32_t i = 0; i < collection->ngeoms; i++)
	{
		const LWGEOM *member = collection->geoms[i];
		if (lwgeom_is_empty(member))
			continue;
		if (emitted++)
			stringbuffer_append(sb, " ");
		assvg_geom(sb, member, relative, precision);
	}
}

static void
assvg_multipoint(stringbuffer_t* sb, const LWMPOINT *mpoint, int relative, int precision)
{
	const LWPOINT *point;
	uint32_t i;

	for (i = 0; i<mpoint->ngeoms; i++)
	{
		if (i) stringbuffer_append(sb, ",");  /* Arbitrary comma separator */
		point = mpoint->geoms[i];
		assvg_point(sb, point, relative, precision);
	}
}


static void
assvg_multiline(stringbuffer_t* sb, const LWMLINE *mline, int relative, int precision)
{
	const LWLINE *line;
	uint32_t i;

	for (i = 0; i<mline->ngeoms; i++)
	{
		if (i) stringbuffer_append(sb, " ");  /* SVG whitespace Separator */
		line = mline->geoms[i];
		assvg_line(sb, line, relative, precision);
	}
}

static void
assvg_multipolygon(stringbuffer_t* sb, const LWMPOLY *mpoly, int relative, int precision)
{
	const LWPOLY *poly;
	uint32_t i;

	for (i = 0; i<mpoly->ngeoms; i++)
	{
		if (i) stringbuffer_append(sb, " ");  /* SVG whitespace Separator */
		poly = mpoly->geoms[i];
		assvg_polygon(sb, poly, relative, precision);
	}
}

static void
assvg_multicurve(stringbuffer_t* sb, const LWMCURVE *mcurve, int relative, int precision)
{
	uint32_t i;
	LWGEOM *geom;
	const LWCIRCSTRING *tmpc = NULL;
	const LWLINE *tmpl = NULL;

	for (i = 0; i < mcurve->ngeoms; i++)
	{
		if (i) stringbuffer_append(sb, " ");  /* SVG whitespace Separator */
		geom = mcurve->geoms[i];

		switch (geom->type)
		{
			case CIRCSTRINGTYPE:
				tmpc = (LWCIRCSTRING *)geom;
				assvg_circstring(sb, tmpc, relative, precision);
				break;

			case LINETYPE:
				tmpl = (LWLINE *)geom;
				assvg_line(sb, tmpl, relative, precision);
				break;

			case NURBSCURVETYPE:
				assvg_nurbscurve(sb, (LWNURBSCURVE *)geom, relative, precision);
				break;

			case COMPOUNDTYPE:
				assvg_compound(sb, (LWCOMPOUND *)geom, relative, precision);
				break;

			default:
				break; /** in theory this should never happen **/
		}
	}
}

static void
assvg_curvepoly(stringbuffer_t* sb, const LWCURVEPOLY *curvepoly, int relative, int precision)
{
	uint32_t i;
	LWGEOM *tmp;

	for (i = 0; i < curvepoly->nrings; i++)
	{
		if (i) stringbuffer_append(sb, " ");	/* Space between each ring */
		tmp = curvepoly->rings[i];
		switch (tmp->type)
		{
			case CIRCSTRINGTYPE:
				assvg_circstring(sb, (LWCIRCSTRING *)tmp, relative, precision);
				break;

			case LINETYPE:
				assvg_line(sb, (LWLINE *)tmp, relative, precision);
				break;

			case COMPOUNDTYPE:
				assvg_compound(sb, (LWCOMPOUND *)tmp, relative, precision);
				break;

			case NURBSCURVETYPE:
				assvg_nurbscurve(sb, (LWNURBSCURVE *)tmp, relative, precision);
				break;

			default:
				break; /** in theory this should never happen **/
		}
		if (relative)
		{
			stringbuffer_append(sb, " z");	/* SVG closepath */
		}
		else
		{
			stringbuffer_append(sb, " Z");	/* SVG closepath */
		}
	}
}

static void
assvg_multisurface(stringbuffer_t* sb, const LWMSURFACE *msurface, int relative, int precision)
{
	LWGEOM *geom;
	uint32_t i;
	const LWPOLY *poly;
	const LWCURVEPOLY *curvepoly;

	for (i = 0; i < msurface->ngeoms; i++)
	{
		if (i) stringbuffer_append(sb, " ");  /* SVG whitespace Separator */
		geom = msurface->geoms[i];
		switch (geom->type)
		{
			case CURVEPOLYTYPE:
				curvepoly = (LWCURVEPOLY *) geom;
				assvg_curvepoly(sb, curvepoly, relative, precision);
				break;
			case POLYGONTYPE:
				poly = (LWPOLY *) geom;
				assvg_polygon(sb, poly, relative, precision);
				break;
			default:
				break; /** in theory this should never happen **/
		}
	}
}

static void
assvg_collection(stringbuffer_t* sb, const LWCOLLECTION *col, int relative, int precision)
{
	uint32_t i; uint32_t j = 0;
	const LWGEOM *subgeom;

	/* EMPTY GEOMETRYCOLLECTION */
	if (col->ngeoms == 0) return;

	for (i = 0; i<col->ngeoms; i++)
	{
		subgeom = col->geoms[i];
		if (!lwgeom_is_empty(subgeom) ){
			/** Note the j is to prevent adding a ;
			 * if the first geometry is empty, but subsequent aren't
			 **/
			if (j) stringbuffer_append(sb, ";");
			j++;
			assvg_geom(sb, subgeom, relative, precision);
		}
	}

}



static void
assvg_geom(stringbuffer_t* sb, const LWGEOM *geom, int relative, int precision)
{
    int type = geom->type;

	switch (type)
	{
	case POINTTYPE:
		assvg_point(sb, (LWPOINT*)geom, relative, precision);
		break;

	case LINETYPE:
		assvg_line(sb, (LWLINE*)geom, relative, precision);
		break;

	case POLYGONTYPE:
		assvg_polygon(sb, (LWPOLY*)geom, relative, precision);
		break;

	case TRIANGLETYPE:
		assvg_triangle(sb, (LWTRIANGLE *)geom, relative, precision);
		break;

	case MULTIPOINTTYPE:
		assvg_multipoint(sb, (LWMPOINT*)geom, relative, precision);
		break;

	case MULTILINETYPE:
		assvg_multiline(sb, (LWMLINE*)geom, relative, precision);
		break;

	case MULTIPOLYGONTYPE:
		assvg_multipolygon(sb, (LWMPOLY*)geom, relative, precision);
		break;

	case CIRCSTRINGTYPE:
		assvg_circstring(sb, (LWCIRCSTRING*)geom, relative, precision);
		break;

	case COMPOUNDTYPE:
		assvg_compound(sb, (LWCOMPOUND*)geom, relative, precision);
		break;

	case CURVEPOLYTYPE:
		assvg_curvepoly(sb, (LWCURVEPOLY*)geom, relative, precision);
		break;

	case MULTICURVETYPE:
		assvg_multicurve(sb, (LWMCURVE*)geom, relative, precision);
		break;

	case MULTISURFACETYPE:
		assvg_multisurface(sb, (LWMSURFACE*)geom, relative, precision);
		break;

	case NURBSCURVETYPE:
		assvg_nurbscurve(sb, (LWNURBSCURVE *)geom, relative, precision);
		break;

	case TINTYPE:
	case POLYHEDRALSURFACETYPE:
		assvg_collection_members(sb, (LWCOLLECTION *)geom, relative, precision);
		break;

	case COLLECTIONTYPE:
		assvg_collection(sb, (LWCOLLECTION *)geom, relative, precision);
		break;

	default:
		lwerror("assvg_geom_buf: '%s' geometry type not supported.",
		        lwtype_name(type));
	}

}

/**
 * Takes a GEOMETRY and returns a SVG representation
 */
lwvarlena_t *
lwgeom_to_svg(const LWGEOM *geom, int precision, int relative)
{
	stringbuffer_t sb;
	int type = geom->type;

	/* Empty varlena for empties */
	if(lwgeom_is_empty(geom))
	{
		lwvarlena_t *v = lwalloc(LWVARHDRSZ);
		LWSIZE_SET(v->size, LWVARHDRSZ);
		return v;
	}

	stringbuffer_init_varlena(&sb);

	switch (type)
	{
	case POINTTYPE:
		assvg_point(&sb, (LWPOINT*)geom, relative, precision);
		break;
	case LINETYPE:
		assvg_line(&sb, (LWLINE*)geom, relative, precision);
		break;
	case POLYGONTYPE:
		assvg_polygon(&sb, (LWPOLY*)geom, relative, precision);
		break;
	case TRIANGLETYPE:
		assvg_triangle(&sb, (LWTRIANGLE *)geom, relative, precision);
		break;
	case CIRCSTRINGTYPE:
		assvg_circstring(&sb, (LWCIRCSTRING*)geom, relative, precision);
		break;
	case COMPOUNDTYPE:
		assvg_compound(&sb, (LWCOMPOUND*)geom, relative, precision);
		break;
	case CURVEPOLYTYPE:
		assvg_curvepoly(&sb, (LWCURVEPOLY*)geom, relative, precision);
		break;
	case MULTIPOINTTYPE:
		assvg_multipoint(&sb, (LWMPOINT*)geom, relative, precision);
		break;
	case MULTILINETYPE:
		assvg_multiline(&sb, (LWMLINE*)geom, relative, precision);
		break;
	case MULTICURVETYPE:
		assvg_multicurve(&sb, (LWMCURVE*)geom, relative, precision);
		break;
	case MULTIPOLYGONTYPE:
		assvg_multipolygon(&sb, (LWMPOLY*)geom, relative, precision);
		break;
	case MULTISURFACETYPE:
		assvg_multisurface(&sb, (LWMSURFACE*)geom, relative, precision);
		break;
	case NURBSCURVETYPE:
		assvg_nurbscurve(&sb, (LWNURBSCURVE *)geom, relative, precision);
		break;
	case TINTYPE:
	case POLYHEDRALSURFACETYPE:
		assvg_collection_members(&sb, (LWCOLLECTION *)geom, relative, precision);
		break;
	case COLLECTIONTYPE:
		assvg_collection(&sb, (LWCOLLECTION*)geom, relative, precision);
		break;

	default:
		lwerror("lwgeom_to_svg: '%s' geometry type not supported", lwtype_name(type));
	}

	return stringbuffer_getvarlena(&sb);
}
