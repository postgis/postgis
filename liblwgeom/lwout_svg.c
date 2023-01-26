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
pointArray_svg_rel(stringbuffer_t* sb, const POINTARRAY *pa, int close_ring, int precision)
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
	pt = getPoint2d_cp(pa, 0);

	x = round(pt->x*f)/f;
	y = round(pt->y*f)/f;

	lwprint_double(x, precision, sx);
	lwprint_double(-y, precision, sy);

	stringbuffer_aprintf(sb, "%s %s l", sx, sy);

	/* accum */
	accum_x = x;
	accum_y = y;

	/* All the following ones */
	for (i = 1; i < end; i++)
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
pointArray_svg_abs(stringbuffer_t* sb, const POINTARRAY *pa, int close_ring, int precision)
{
	int i, end;
	const POINT2D* pt;
	char sx[OUT_DOUBLE_BUFFER_SIZE];
	char sy[OUT_DOUBLE_BUFFER_SIZE];

	end = close_ring ? pa->npoints : pa->npoints - 1;

	for (i = 0; i < end; i++)
	{
		pt = getPoint2d_cp(pa, i);

		if (i == 1) stringbuffer_append(sb, " L ");
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

	const POINT2D* pt = getPoint2d_cp(point->point, 0);
	lwprint_double(pt->x, precision, sx);
	lwprint_double(-(pt->y), precision, sy);

	stringbuffer_aprintf(sb,
		circle ? "x=\"%s\" y=\"%s\"" : "cx=\"%s\" cy=\"%s\"",
		sx, sy);
}


static void
assvg_line(stringbuffer_t* sb, const LWLINE *line, int relative, int precision)
{
	/* Start path with SVG MoveTo */
	stringbuffer_append(sb, "M ");
	if (relative)
		pointArray_svg_rel(sb, line->points, 1, precision);
	else
		pointArray_svg_abs(sb, line->points, 1, precision);
}


static void
assvg_polygon(stringbuffer_t* sb, const LWPOLY *poly, int relative, int precision)
{
	uint32_t i;

	for (i = 0; i<poly->nrings; i++)
	{
		if (i) stringbuffer_append(sb, " ");	/* Space beetween each ring */
		stringbuffer_append(sb, "M ");		/* Start path with SVG MoveTo */

		if (relative)
		{
			pointArray_svg_rel(sb, poly->rings[i], 0, precision);
			stringbuffer_append(sb, " z");	/* SVG closepath */
		}
		else
		{
			pointArray_svg_abs(sb, poly->rings[i], 0, precision);
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
assvg_collection(stringbuffer_t* sb, const LWCOLLECTION *col, int relative, int precision)
{
	uint32_t i;
	const LWGEOM *subgeom;

	/* EMPTY GEOMETRYCOLLECTION */
	if (col->ngeoms == 0) return;

	for (i = 0; i<col->ngeoms; i++)
	{
		if (i) stringbuffer_append(sb, ";");
		subgeom = col->geoms[i];
		assvg_geom(sb, subgeom, relative, precision);
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
	case MULTIPOINTTYPE:
		assvg_multipoint(&sb, (LWMPOINT*)geom, relative, precision);
		break;
	case MULTILINETYPE:
		assvg_multiline(&sb, (LWMLINE*)geom, relative, precision);
		break;
	case MULTIPOLYGONTYPE:
		assvg_multipolygon(&sb, (LWMPOLY*)geom, relative, precision);
		break;
	case COLLECTIONTYPE:
		assvg_collection(&sb, (LWCOLLECTION*)geom, relative, precision);
		break;

	default:
		lwerror("lwgeom_to_svg: '%s' geometry type not supported", lwtype_name(type));
	}

	return stringbuffer_getvarlena(&sb);
}

