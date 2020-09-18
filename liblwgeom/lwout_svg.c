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

static lwvarlena_t *assvg_point(const LWPOINT *point, int relative, int precision);
static lwvarlena_t *assvg_line(const LWLINE *line, int relative, int precision);
static lwvarlena_t *assvg_polygon(const LWPOLY *poly, int relative, int precision);
static lwvarlena_t *assvg_multipoint(const LWMPOINT *mpoint, int relative, int precision);
static lwvarlena_t *assvg_multiline(const LWMLINE *mline, int relative, int precision);
static lwvarlena_t *assvg_multipolygon(const LWMPOLY *mpoly, int relative, int precision);
static lwvarlena_t *assvg_collection(const LWCOLLECTION *col, int relative, int precision);

static size_t assvg_geom_size(const LWGEOM *geom, int relative, int precision);
static size_t assvg_geom_buf(const LWGEOM *geom, char *output, int relative, int precision);
static size_t pointArray_svg_size(POINTARRAY *pa, int precision);
static size_t pointArray_svg_rel(POINTARRAY *pa, char * output, int close_ring, int precision);
static size_t pointArray_svg_abs(POINTARRAY *pa, char * output, int close_ring, int precision);


/**
 * Takes a GEOMETRY and returns a SVG representation
 */
lwvarlena_t *
lwgeom_to_svg(const LWGEOM *geom, int precision, int relative)
{
	lwvarlena_t *ret = NULL;
	int type = geom->type;

	/* Empty varlena for empties */
	if( lwgeom_is_empty(geom) )
	{
		lwvarlena_t *v = lwalloc(LWVARHDRSZ);
		LWSIZE_SET(v->size, LWVARHDRSZ);
		return v;
	}

	switch (type)
	{
	case POINTTYPE:
		ret = assvg_point((LWPOINT*)geom, relative, precision);
		break;
	case LINETYPE:
		ret = assvg_line((LWLINE*)geom, relative, precision);
		break;
	case POLYGONTYPE:
		ret = assvg_polygon((LWPOLY*)geom, relative, precision);
		break;
	case MULTIPOINTTYPE:
		ret = assvg_multipoint((LWMPOINT*)geom, relative, precision);
		break;
	case MULTILINETYPE:
		ret = assvg_multiline((LWMLINE*)geom, relative, precision);
		break;
	case MULTIPOLYGONTYPE:
		ret = assvg_multipolygon((LWMPOLY*)geom, relative, precision);
		break;
	case COLLECTIONTYPE:
		ret = assvg_collection((LWCOLLECTION*)geom, relative, precision);
		break;

	default:
		lwerror("lwgeom_to_svg: '%s' geometry type not supported",
		        lwtype_name(type));
	}

	return ret;
}


/**
 * Point Geometry
 */

static size_t
assvg_point_size(__attribute__((__unused__)) const LWPOINT *point, int circle, int precision)
{
	size_t size;

	size = (OUT_MAX_BYTES_DOUBLE + precision) * 2;
	if (circle) size += sizeof("cx='' cy=''");
	else size += sizeof("x='' y=''");

	return size;
}

static size_t
assvg_point_buf(const LWPOINT *point, char * output, int circle, int precision)
{
	char *ptr=output;
	char x[OUT_DOUBLE_BUFFER_SIZE];
	char y[OUT_DOUBLE_BUFFER_SIZE];
	POINT2D pt;

	getPoint2d_p(point->point, 0, &pt);

	lwprint_double(pt.x, precision, x);
	lwprint_double(-pt.y, precision, y);

	if (circle) ptr += sprintf(ptr, "x=\"%s\" y=\"%s\"", x, y);
	else ptr += sprintf(ptr, "cx=\"%s\" cy=\"%s\"", x, y);

	return (ptr-output);
}

static lwvarlena_t *
assvg_point(const LWPOINT *point, int circle, int precision)
{
	size_t size = assvg_point_size(point, circle, precision);
	lwvarlena_t *v = lwalloc(LWVARHDRSZ + size);
	size = assvg_point_buf(point, v->data, circle, precision);
	LWSIZE_SET(v->size, LWVARHDRSZ + size);
	return v;
}


/**
 * Line Geometry
 */

static size_t
assvg_line_size(const LWLINE *line, __attribute__((__unused__)) int relative, int precision)
{
	size_t size;

	size = sizeof("M ");
	size += pointArray_svg_size(line->points, precision);

	return size;
}

static size_t
assvg_line_buf(const LWLINE *line, char * output, int relative, int precision)
{
	char *ptr=output;

	/* Start path with SVG MoveTo */
	ptr += sprintf(ptr, "M ");
	if (relative)
		ptr += pointArray_svg_rel(line->points, ptr, 1, precision);
	else
		ptr += pointArray_svg_abs(line->points, ptr, 1, precision);

	return (ptr-output);
}

static lwvarlena_t *
assvg_line(const LWLINE *line, int relative, int precision)
{
	size_t size = assvg_line_size(line, relative, precision);
	lwvarlena_t *v = lwalloc(LWVARHDRSZ + size);
	size = assvg_line_buf(line, v->data, relative, precision);
	LWSIZE_SET(v->size, LWVARHDRSZ + size);
	return v;
}


/**
 * Polygon Geometry
 */

static size_t
assvg_polygon_size(const LWPOLY *poly, __attribute__((__unused__)) int relative, int precision)
{
	uint32_t i;
	size_t size=0;

	for (i=0; i<poly->nrings; i++)
		size += pointArray_svg_size(poly->rings[i], precision) + sizeof(" ");
	size += sizeof("M  Z") * poly->nrings;

	return size;
}

static size_t
assvg_polygon_buf(const LWPOLY *poly, char * output, int relative, int precision)
{
	uint32_t i;
	char *ptr=output;

	for (i=0; i<poly->nrings; i++)
	{
		if (i) ptr += sprintf(ptr, " ");	/* Space beetween each ring */
		ptr += sprintf(ptr, "M ");		/* Start path with SVG MoveTo */

		if (relative)
		{
			ptr += pointArray_svg_rel(poly->rings[i], ptr, 0, precision);
			ptr += sprintf(ptr, " z");	/* SVG closepath */
		}
		else
		{
			ptr += pointArray_svg_abs(poly->rings[i], ptr, 0, precision);
			ptr += sprintf(ptr, " Z");	/* SVG closepath */
		}
	}

	return (ptr-output);
}

static lwvarlena_t *
assvg_polygon(const LWPOLY *poly, int relative, int precision)
{
	size_t size = assvg_polygon_size(poly, relative, precision);
	lwvarlena_t *v = lwalloc(LWVARHDRSZ + size);
	size = assvg_polygon_buf(poly, v->data, relative, precision);
	LWSIZE_SET(v->size, LWVARHDRSZ + size);
	return v;
}


/**
 * Multipoint Geometry
 */

static size_t
assvg_multipoint_size(const LWMPOINT *mpoint, int relative, int precision)
{
	const LWPOINT *point;
	size_t size=0;
	uint32_t i;

	for (i=0 ; i<mpoint->ngeoms ; i++)
	{
		point = mpoint->geoms[i];
		size += assvg_point_size(point, relative, precision);
	}
	size += sizeof(",") * --i;  /* Arbitrary comma separator */

	return size;
}

static size_t
assvg_multipoint_buf(const LWMPOINT *mpoint, char *output, int relative, int precision)
{
	const LWPOINT *point;
	uint32_t i;
	char *ptr=output;

	for (i=0 ; i<mpoint->ngeoms ; i++)
	{
		if (i) ptr += sprintf(ptr, ",");  /* Arbitrary comma separator */
		point = mpoint->geoms[i];
		ptr += assvg_point_buf(point, ptr, relative, precision);
	}

	return (ptr-output);
}

static lwvarlena_t *
assvg_multipoint(const LWMPOINT *mpoint, int relative, int precision)
{
	size_t size = assvg_multipoint_size(mpoint, relative, precision);
	lwvarlena_t *v = lwalloc(LWVARHDRSZ + size);
	size = assvg_multipoint_buf(mpoint, v->data, relative, precision);
	LWSIZE_SET(v->size, LWVARHDRSZ + size);
	return v;
}


/**
 * Multiline Geometry
 */

static size_t
assvg_multiline_size(const LWMLINE *mline, int relative, int precision)
{
	const LWLINE *line;
	size_t size=0;
	uint32_t i;

	for (i=0 ; i<mline->ngeoms ; i++)
	{
		line = mline->geoms[i];
		size += assvg_line_size(line, relative, precision);
	}
	size += sizeof(" ") * --i;   /* SVG whitespace Separator */

	return size;
}

static size_t
assvg_multiline_buf(const LWMLINE *mline, char *output, int relative, int precision)
{
	const LWLINE *line;
	uint32_t i;
	char *ptr=output;

	for (i=0 ; i<mline->ngeoms ; i++)
	{
		if (i) ptr += sprintf(ptr, " ");  /* SVG whitespace Separator */
		line = mline->geoms[i];
		ptr += assvg_line_buf(line, ptr, relative, precision);
	}

	return (ptr-output);
}

static lwvarlena_t *
assvg_multiline(const LWMLINE *mline, int relative, int precision)
{
	size_t size = assvg_multiline_size(mline, relative, precision);
	lwvarlena_t *v = lwalloc(LWVARHDRSZ + size);
	size = assvg_multiline_buf(mline, v->data, relative, precision);
	LWSIZE_SET(v->size, LWVARHDRSZ + size);
	return v;
}


/*
 * Multipolygon Geometry
 */

static size_t
assvg_multipolygon_size(const LWMPOLY *mpoly, int relative, int precision)
{
	const LWPOLY *poly;
	size_t size=0;
	uint32_t i;

	for (i=0 ; i<mpoly->ngeoms ; i++)
	{
		poly = mpoly->geoms[i];
		size += assvg_polygon_size(poly, relative, precision);
	}
	size += sizeof(" ") * --i;   /* SVG whitespace Separator */

	return size;
}

static size_t
assvg_multipolygon_buf(const LWMPOLY *mpoly, char *output, int relative, int precision)
{
	const LWPOLY *poly;
	uint32_t i;
	char *ptr=output;

	for (i=0 ; i<mpoly->ngeoms ; i++)
	{
		if (i) ptr += sprintf(ptr, " ");  /* SVG whitespace Separator */
		poly = mpoly->geoms[i];
		ptr += assvg_polygon_buf(poly, ptr, relative, precision);
	}

	return (ptr-output);
}

static lwvarlena_t *
assvg_multipolygon(const LWMPOLY *mpoly, int relative, int precision)
{
	size_t size = assvg_multipolygon_size(mpoly, relative, precision);
	lwvarlena_t *v = lwalloc(LWVARHDRSZ + size);
	size = assvg_multipolygon_buf(mpoly, v->data, relative, precision);
	LWSIZE_SET(v->size, LWVARHDRSZ + size);
	return v;
}


/**
* Collection Geometry
*/

static size_t
assvg_collection_size(const LWCOLLECTION *col, int relative, int precision)
{
	uint32_t i = 0;
	size_t size=0;
	const LWGEOM *subgeom;

	for (i=0; i<col->ngeoms; i++)
	{
		subgeom = col->geoms[i];
		size += assvg_geom_size(subgeom, relative, precision);
	}

	if ( i ) /* We have some geometries, so add space for delimiters. */
		size += sizeof(";") * --i;

	if (size == 0) size++; /* GEOMETRYCOLLECTION EMPTY, space for null terminator */

	return size;
}

static size_t
assvg_collection_buf(const LWCOLLECTION *col, char *output, int relative, int precision)
{
	uint32_t i;
	char *ptr=output;
	const LWGEOM *subgeom;

	/* EMPTY GEOMETRYCOLLECTION */
	if (col->ngeoms == 0) *ptr = '\0';

	for (i=0; i<col->ngeoms; i++)
	{
		if (i) ptr += sprintf(ptr, ";");
		subgeom = col->geoms[i];
		ptr += assvg_geom_buf(subgeom, ptr, relative, precision);
	}

	return (ptr - output);
}

static lwvarlena_t *
assvg_collection(const LWCOLLECTION *col, int relative, int precision)
{
	size_t size = assvg_collection_size(col, relative, precision);
	lwvarlena_t *v = lwalloc(LWVARHDRSZ + size);
	size = assvg_collection_buf(col, v->data, relative, precision);
	LWSIZE_SET(v->size, LWVARHDRSZ + size);
	return v;
}


static size_t
assvg_geom_buf(const LWGEOM *geom, char *output, int relative, int precision)
{
    int type = geom->type;
	char *ptr=output;

	switch (type)
	{
	case POINTTYPE:
		ptr += assvg_point_buf((LWPOINT*)geom, ptr, relative, precision);
		break;

	case LINETYPE:
		ptr += assvg_line_buf((LWLINE*)geom, ptr, relative, precision);
		break;

	case POLYGONTYPE:
		ptr += assvg_polygon_buf((LWPOLY*)geom, ptr, relative, precision);
		break;

	case MULTIPOINTTYPE:
		ptr += assvg_multipoint_buf((LWMPOINT*)geom, ptr, relative, precision);
		break;

	case MULTILINETYPE:
		ptr += assvg_multiline_buf((LWMLINE*)geom, ptr, relative, precision);
		break;

	case MULTIPOLYGONTYPE:
		ptr += assvg_multipolygon_buf((LWMPOLY*)geom, ptr, relative, precision);
		break;

	default:
		lwerror("assvg_geom_buf: '%s' geometry type not supported.",
		        lwtype_name(type));
	}

	return (ptr-output);
}


static size_t
assvg_geom_size(const LWGEOM *geom, int relative, int precision)
{
    int type = geom->type;
	size_t size = 0;

	switch (type)
	{
	case POINTTYPE:
		size = assvg_point_size((LWPOINT*)geom, relative, precision);
		break;

	case LINETYPE:
		size = assvg_line_size((LWLINE*)geom, relative, precision);
		break;

	case POLYGONTYPE:
		size = assvg_polygon_size((LWPOLY*)geom, relative, precision);
		break;

	case MULTIPOINTTYPE:
		size = assvg_multipoint_size((LWMPOINT*)geom, relative, precision);
		break;

	case MULTILINETYPE:
		size = assvg_multiline_size((LWMLINE*)geom, relative, precision);
		break;

	case MULTIPOLYGONTYPE:
		size = assvg_multipolygon_size((LWMPOLY*)geom, relative, precision);
		break;

	default:
		lwerror("assvg_geom_size: '%s' geometry type not supported.",
		        lwtype_name(type));
	}

	return size;
}


static size_t
pointArray_svg_rel(POINTARRAY *pa, char *output, int close_ring, int precision)
{
	int i, end;
	char *ptr;
	char sx[OUT_DOUBLE_BUFFER_SIZE];
	char sy[OUT_DOUBLE_BUFFER_SIZE];
	const POINT2D *pt;

	double f = 1.0;
	double dx, dy, x, y, accum_x, accum_y;

	ptr = output;

	if (precision >= 0)
	{
		f = pow(10, precision);
	}

	if (close_ring) end = pa->npoints;
	else end = pa->npoints - 1;

	/* Starting point */
	pt = getPoint2d_cp(pa, 0);

	x = round(pt->x*f)/f;
	y = round(pt->y*f)/f;

	lwprint_double(x, precision, sx);
	lwprint_double(-y, precision, sy);
	ptr += sprintf(ptr,"%s %s l", sx, sy);

	/* accum */
	accum_x = x;
	accum_y = y;

	/* All the following ones */
	for (i=1 ; i < end ; i++)
	{
		// lpt = pt;

		pt = getPoint2d_cp(pa, i);

		x = round(pt->x*f)/f;
		y = round(pt->y*f)/f;
		dx = x - accum_x;
		dy = y - accum_y;

		lwprint_double(dx, precision, sx);
		lwprint_double(-dy, precision, sy);

		accum_x += dx;
		accum_y += dy;

		ptr += sprintf(ptr," %s %s", sx, sy);
	}

	return (ptr-output);
}


/**
 * Returns maximum size of rendered pointarray in bytes.
 */
static size_t
pointArray_svg_abs(POINTARRAY *pa, char *output, int close_ring, int precision)
{
	int i, end;
	char *ptr;
	char x[OUT_DOUBLE_BUFFER_SIZE];
	char y[OUT_DOUBLE_BUFFER_SIZE];
	POINT2D pt;

	ptr = output;

	if (close_ring) end = pa->npoints;
	else end = pa->npoints - 1;

	for (i=0 ; i < end ; i++)
	{
		getPoint2d_p(pa, i, &pt);

		lwprint_double(pt.x, precision, x);
		lwprint_double(-pt.y, precision, y);

		if (i == 1) ptr += sprintf(ptr, " L ");
		else if (i) ptr += sprintf(ptr, " ");
		ptr += sprintf(ptr,"%s %s", x, y);
	}

	return (ptr-output);
}


/**
 * Returns maximum size of rendered pointarray in bytes.
 */
static size_t
pointArray_svg_size(POINTARRAY *pa, int precision)
{
	return (OUT_MAX_BYTES_DOUBLE + precision + sizeof(" ")) * 2 * pa->npoints + sizeof(" L ");
}
