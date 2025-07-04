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

static void pointArray_svg_arc(stringbuffer_t* sb, const POINTARRAY *pa, int close_ring, int relative,  int precision)
{
	uint32_t i; //, end;
	char sx[OUT_DOUBLE_BUFFER_SIZE];
	char sy[OUT_DOUBLE_BUFFER_SIZE];

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
		if ( (i == 2) && !is_circle ){
			/** add MoveTo first point **/
			lwprint_double(t1->x, precision, sx);
			lwprint_double(-(t1->y), precision, sy);
			stringbuffer_aprintf(sb, "%s %s", sx, sy);
		}
		/** is circle: need to start at center of circle **/
		if ( (i == 2) && is_circle){
			/** add MoveTo center of circle **/
			lwprint_double(center.x, precision, sx);
			lwprint_double(-(center.y), precision, sy);
			stringbuffer_aprintf(sb, "%s %s", sx, sy);
		}
		lwprint_double(radius, precision, sx);
		lwprint_double(0, precision, sy);
		/** is circle need to handle differently **/
		if (is_circle){
			//https://stackoverflow.com/questions/5737975/circle-drawing-with-svgs-arc-path
			lwprint_double(radius*2, precision, sy);
			stringbuffer_aprintf(sb, " m %s 0 a %s %s 0 1 0 -%s 0", sx, sx, sx, sy);
			stringbuffer_aprintf(sb, " a %s %s 0 1 0 %s 0", sx, sx, sy);
		}
		else {
			/* Arc radius radius 0 arcflag swipeflag */
			if (relative){
				stringbuffer_aprintf(sb, " a %s %s 0 %d %d ", sx, sx, largeArcFlag, sweepFlag);
			}
			else {
				stringbuffer_aprintf(sb, " A %s %s 0 %d %d ", sx, sx, largeArcFlag, sweepFlag);
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

static void
assvg_compound(stringbuffer_t* sb, const LWCOMPOUND *icompound, int relative, int precision)
{
	uint32_t i;
	LWGEOM *geom;
	LWCIRCSTRING *tmpc = NULL;
	LWLINE *tmpl = NULL;
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
				pointArray_svg_arc(sb, tmpc->points, 1, relative, precision);
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
	case COLLECTIONTYPE:
		assvg_collection(&sb, (LWCOLLECTION*)geom, relative, precision);
		break;

	default:
		lwerror("lwgeom_to_svg: '%s' geometry type not supported", lwtype_name(type));
	}

	return stringbuffer_getvarlena(&sb);
}

