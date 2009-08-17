/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2003 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of hte GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

/** @file
*
* SVG output routines.
* Originally written by: Klaus Förster <klaus@svg.cc>
* Refactored by: Olivier Courtin (Camptocamp)
*
* BNF SVG Path: <http://www.w3.org/TR/SVG/paths.html#PathDataBNF>
**********************************************************************/


#include "postgres.h"
#include "lwgeom_pg.h"
#include "liblwgeom.h"

Datum assvg_geometry(PG_FUNCTION_ARGS);
char *geometry_to_svg(uchar *srl, bool relative, int precision);
static char * assvg_point(LWPOINT *point, bool relative, int precision);
static char * assvg_line(LWLINE *line, bool relative, int precision);
static char * assvg_polygon(LWPOLY *poly, bool relative, int precision);
static char * assvg_multipoint(LWGEOM_INSPECTED *insp, bool relative, int precision);
static char * assvg_multiline(LWGEOM_INSPECTED *insp, bool relative, int precision);
static char * assvg_multipolygon(LWGEOM_INSPECTED *insp, bool relative, int precision);
static char * assvg_collection(LWGEOM_INSPECTED *insp, bool relative, int precision);

static size_t assvg_inspected_size(LWGEOM_INSPECTED *insp, bool relative, int precision);
static size_t assvg_inspected_buf(LWGEOM_INSPECTED *insp, char *output, bool relative, int precision);
static size_t pointArray_svg_size(POINTARRAY *pa, int precision);
static size_t pointArray_svg_rel(POINTARRAY *pa, char * output, bool close_ring, int precision);
static size_t pointArray_svg_abs(POINTARRAY *pa, char * output, bool close_ring, int precision);

#define SHOW_DIGS_DOUBLE 15
#define MAX_DOUBLE_PRECISION 15
#define MAX_DIGS_DOUBLE (SHOW_DIGS_DOUBLE + 2) /* +2 mean add dot and sign */

/**
 * SVG features
 */
PG_FUNCTION_INFO_V1(assvg_geometry);
Datum assvg_geometry(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	char *svg;
	text *result;
	int len;
	bool relative = false;
	int precision=MAX_DOUBLE_PRECISION;

	if ( PG_ARGISNULL(0) ) PG_RETURN_NULL();

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* check for relative path notation */
	if ( PG_NARGS() > 1 && ! PG_ARGISNULL(1) )
		relative = PG_GETARG_INT32(1) ? true:false;

	if ( PG_NARGS() > 2 && ! PG_ARGISNULL(2) )
	{
		precision = PG_GETARG_INT32(2);
		if ( precision > MAX_DOUBLE_PRECISION )
			precision = MAX_DOUBLE_PRECISION;
		else if ( precision < 0 ) precision = 0;
	}

	svg = geometry_to_svg(SERIALIZED_FORM(geom), relative, precision);
	PG_FREE_IF_COPY(geom, 0);

	len = strlen(svg) + VARHDRSZ;
	result = palloc(len);
	SET_VARSIZE(result, len);
	memcpy(VARDATA(result), svg, len-VARHDRSZ);

	pfree(svg);

	PG_RETURN_POINTER(result);
}


/** takes a GEOMETRY and returns a SVG representation */
char *
geometry_to_svg(uchar *geom, bool relative, int precision)
{
	char *ret = NULL;
	int type;

	type = lwgeom_getType(geom[0]);
	switch (type)
	{
	case POINTTYPE:
		ret = assvg_point(lwpoint_deserialize(geom), relative, precision);
		break;
	case LINETYPE:
		ret = assvg_line(lwline_deserialize(geom), relative, precision);
		break;
	case POLYGONTYPE:
		ret = assvg_polygon(lwpoly_deserialize(geom), relative, precision);
		break;
	case MULTIPOINTTYPE:
		ret = assvg_multipoint(lwgeom_inspect(geom), relative, precision);
		break;
	case MULTILINETYPE:
		ret = assvg_multiline(lwgeom_inspect(geom), relative, precision);
		break;
	case MULTIPOLYGONTYPE:
		ret = assvg_multipolygon(lwgeom_inspect(geom), relative, precision);
		break;
	case COLLECTIONTYPE:
		ret = assvg_collection(lwgeom_inspect(geom), relative, precision);
		break;

	default:
		lwerror("ST_AsSVG: '%s' geometry type not supported.",
		        lwgeom_typename(type));
	}

	return ret;
}


/**
 * Point Geometry
 */

static size_t
assvg_point_size(LWPOINT *point, bool circle, int precision)
{
	size_t size;

	size = (MAX_DIGS_DOUBLE + precision) * 2;
	if (circle) size += sizeof("cx='' cy=''");
	else size += sizeof("x='' y=''");

	return size;
}

static size_t
assvg_point_buf(LWPOINT *point, char * output, bool circle, int precision)
{
	char *ptr=output;
	char x[MAX_DIGS_DOUBLE+MAX_DOUBLE_PRECISION+1];
	char y[MAX_DIGS_DOUBLE+MAX_DOUBLE_PRECISION+1];
	POINT2D pt;

	getPoint2d_p(point->point, 0, &pt);
	sprintf(x, "%.*f", precision, pt.x);
	trim_trailing_zeros(x);
	/* SVG Y axis is reversed, an no need to transform 0 into -0 */
	sprintf(y, "%.*f", precision, fabs(pt.y) ? pt.y * -1 : pt.y);
	trim_trailing_zeros(y);

	if (circle) ptr += sprintf(ptr, "x=\"%s\" y=\"%s\"", x, y);
	else ptr += sprintf(ptr, "cx=\"%s\" cy=\"%s\"", x, y);

	return (ptr-output);
}

static char *
assvg_point(LWPOINT *point, bool circle, int precision)
{
	char *output;
	int size;

	size = assvg_point_size(point, circle, precision);
	output = palloc(size);
	assvg_point_buf(point, output, circle, precision);

	return output;
}


/**
 * Line Geometry
 */

static size_t
assvg_line_size(LWLINE *line, bool relative, int precision)
{
	size_t size;

	size = sizeof("M ");
	size += pointArray_svg_size(line->points, precision);

	return size;
}

static size_t
assvg_line_buf(LWLINE *line, char * output, bool relative, int precision)
{
	char *ptr=output;

	/* Start path with SVG MoveTo */
	ptr += sprintf(ptr, "M ");
	if (relative)
		ptr += pointArray_svg_rel(line->points, ptr, true, precision);
	else
		ptr += pointArray_svg_abs(line->points, ptr, true, precision);

	return (ptr-output);
}

static char *
assvg_line(LWLINE *line, bool relative, int precision)
{
	char *output;
	int size;

	size = assvg_line_size(line, relative, precision);
	output = palloc(size);
	assvg_line_buf(line, output, relative, precision);

	return output;
}


/**
 * Polygon Geometry
 */

static size_t
assvg_polygon_size(LWPOLY *poly, bool relative, int precision)
{
	int i;
	size_t size=0;

	for (i=0; i<poly->nrings; i++)
		size += pointArray_svg_size(poly->rings[i], precision) + sizeof(" ");
	size += sizeof("M  Z") * poly->nrings;

	return size;
}

static size_t
assvg_polygon_buf(LWPOLY *poly, char * output, bool relative, int precision)
{
	int i;
	char *ptr=output;

	for (i=0; i<poly->nrings; i++)
	{
		if (i) ptr += sprintf(ptr, " ");	/* Space beetween each ring */
		ptr += sprintf(ptr, "M ");		/* Start path with SVG MoveTo */

		if (relative)
		{
			ptr += pointArray_svg_rel(poly->rings[i], ptr, false, precision);
			ptr += sprintf(ptr, " z");	/* SVG closepath */
		}
		else
		{
			ptr += pointArray_svg_abs(poly->rings[i], ptr, false, precision);
			ptr += sprintf(ptr, " Z");	/* SVG closepath */
		}
	}

	return (ptr-output);
}

static char *
assvg_polygon(LWPOLY *poly, bool relative, int precision)
{
	char *output;
	int size;

	size = assvg_polygon_size(poly, relative, precision);
	output = palloc(size);
	assvg_polygon_buf(poly, output, relative, precision);

	return output;
}


/**
 * Multipoint Geometry
 */

static size_t
assvg_multipoint_size(LWGEOM_INSPECTED *insp, bool relative, int precision)
{
	LWPOINT *point;
	size_t size=0;
	int i;

	for (i=0 ; i<insp->ngeometries ; i++)
	{
		point = lwgeom_getpoint_inspected(insp, i);
		size += assvg_point_size(point, relative, precision);
		if (point) lwpoint_release(point);
	}
	size += sizeof(",") * --i;  /* Arbitrary comma separator */

	return size;
}

static size_t
assvg_multipoint_buf(LWGEOM_INSPECTED *insp, char *output, bool relative, int precision)
{
	LWPOINT *point;
	int i;
	char *ptr=output;

	for (i=0 ; i<insp->ngeometries ; i++)
	{
		if (i) ptr += sprintf(ptr, ",");  /* Arbitrary comma separator */
		point = lwgeom_getpoint_inspected(insp, i);
		ptr += assvg_point_buf(point, ptr, relative, precision);
		if (point) lwpoint_release(point);
	}

	return (ptr-output);
}

static char *
assvg_multipoint(LWGEOM_INSPECTED *point, bool relative, int precision)
{
	char *output;
	int size;

	size = assvg_multipoint_size(point, relative, precision);
	output = palloc(size);
	assvg_multipoint_buf(point, output, relative, precision);

	return output;
}


/**
 * Multiline Geometry
 */

static size_t
assvg_multiline_size(LWGEOM_INSPECTED *insp, bool relative, int precision)
{
	LWLINE *line;
	size_t size=0;
	int i;

	for (i=0 ; i<insp->ngeometries ; i++)
	{
		line = lwgeom_getline_inspected(insp, i);
		size += assvg_line_size(line, relative, precision);
		if (line) lwline_release(line);
	}
	size += sizeof(" ") * --i;   /* SVG whitespace Separator */

	return size;
}

static size_t
assvg_multiline_buf(LWGEOM_INSPECTED *insp, char *output, bool relative, int precision)
{
	LWLINE *line;
	int i;
	char *ptr=output;

	for (i=0 ; i<insp->ngeometries ; i++)
	{
		if (i) ptr += sprintf(ptr, " ");  /* SVG whitespace Separator */
		line = lwgeom_getline_inspected(insp, i);
		ptr += assvg_line_buf(line, ptr, relative, precision);
		if (line) lwline_release(line);
	}

	return (ptr-output);
}

static char *
assvg_multiline(LWGEOM_INSPECTED *line, bool relative, int precision)
{
	char *output;
	int size;

	size = assvg_multiline_size(line, relative, precision);
	output = palloc(size);
	assvg_multiline_buf(line, output, relative, precision);

	return output;
}


/*
 * Multipolygon Geometry
 */

static size_t
assvg_multipolygon_size(LWGEOM_INSPECTED *insp, bool relative, int precision)
{
	LWPOLY *poly;
	size_t size=0;
	int i;

	for (i=0 ; i<insp->ngeometries ; i++)
	{
		poly = lwgeom_getpoly_inspected(insp, i);
		size += assvg_polygon_size(poly, relative, precision);
		if (poly) lwpoly_release(poly);
	}
	size += sizeof(" ") * --i;   /* SVG whitespace Separator */

	return size;
}

static size_t
assvg_multipolygon_buf(LWGEOM_INSPECTED *insp, char *output, bool relative, int precision)
{
	LWPOLY *poly;
	int i;
	char *ptr=output;

	for (i=0 ; i<insp->ngeometries ; i++)
	{
		if (i) ptr += sprintf(ptr, " ");  /* SVG whitespace Separator */
		poly = lwgeom_getpoly_inspected(insp, i);
		ptr += assvg_polygon_buf(poly, ptr, relative, precision);
		if (poly) lwpoly_release(poly);
	}

	return (ptr-output);
}

static char *
assvg_multipolygon(LWGEOM_INSPECTED *poly, bool relative, int precision)
{
	char *output;
	int size;

	size = assvg_multipolygon_size(poly, relative, precision);
	output = palloc(size);
	assvg_multipolygon_buf(poly, output, relative, precision);

	return output;
}


/**
* Collection Geometry
*/

static size_t
assvg_collection_size(LWGEOM_INSPECTED *insp, bool relative, int precision)
{
	int i;
	size_t size=0;
	LWGEOM_INSPECTED *subinsp;
	uchar *subgeom;

	for (i=0; i<insp->ngeometries; i++)
	{
		subgeom = lwgeom_getsubgeometry_inspected(insp, i);
		subinsp = lwgeom_inspect(subgeom);
		size += assvg_inspected_size(subinsp, relative, precision);
		lwinspected_release(subinsp);
	}
	
	if ( i ) /* We have some geometries, so we need extra space for delimiters. */
		size += sizeof(";") * --i;

	return size;
}

static size_t
assvg_collection_buf(LWGEOM_INSPECTED *insp, char *output, bool relative, int precision)
{
	int i;
	char *ptr=output;
	LWGEOM_INSPECTED *subinsp;
	uchar *subgeom;

	for (i=0; i<insp->ngeometries; i++)
	{
		if (i) ptr += sprintf(ptr, ";");
		subgeom = lwgeom_getsubgeometry_inspected(insp, i);
		subinsp = lwgeom_inspect(subgeom);
		ptr += assvg_inspected_buf(subinsp, ptr, relative, precision);
		lwinspected_release(subinsp);
	}

	return (ptr - output);
}

static char *
assvg_collection(LWGEOM_INSPECTED *insp, bool relative, int precision)
{
	char *output;
	int size;

	size = assvg_collection_size(insp, relative, precision);
	output = palloc(size);
	assvg_collection_buf(insp, output, relative, precision);

	return output;
}



static size_t
assvg_inspected_buf(LWGEOM_INSPECTED *insp, char *output, bool relative, int precision)
{
	LWPOINT *point;
	LWLINE *line;
	LWPOLY *poly;
	int type = lwgeom_getType(insp->serialized_form[0]);
	char *ptr=output;

	switch (type)
	{
	case POINTTYPE:
		point=lwgeom_getpoint_inspected(insp, 0);
		ptr += assvg_point_buf(point, ptr, relative, precision);
		lwpoint_release(point);
		break;

	case LINETYPE:
		line=lwgeom_getline_inspected(insp, 0);
		ptr += assvg_line_buf(line, ptr, relative, precision);
		lwline_release(line);
		break;

	case POLYGONTYPE:
		poly=lwgeom_getpoly_inspected(insp, 0);
		ptr += assvg_polygon_buf(poly, ptr, relative, precision);
		lwpoly_release(poly);
		break;

	case MULTIPOINTTYPE:
		ptr += assvg_multipoint_buf(insp, ptr, relative, precision);
		break;

	case MULTILINETYPE:
		ptr += assvg_multiline_buf(insp, ptr, relative, precision);
		break;

	case MULTIPOLYGONTYPE:
		ptr += assvg_multipolygon_buf(insp, ptr, relative, precision);
		break;

	default:
		lwerror("ST_AsSVG: '%s' geometry type not supported.",
		        lwgeom_typename(type));
	}

	return (ptr-output);
}


static size_t
assvg_inspected_size(LWGEOM_INSPECTED *insp, bool relative, int precision)
{
	int type = lwgeom_getType(insp->serialized_form[0]);
	size_t size = 0;
	LWPOINT *point;
	LWLINE *line;
	LWPOLY *poly;

	switch (type)
	{
	case POINTTYPE:
		point=lwgeom_getpoint_inspected(insp, 0);
		size = assvg_point_size(point, relative, precision);
		lwpoint_release(point);
		break;

	case LINETYPE:
		line=lwgeom_getline_inspected(insp, 0);
		size = assvg_line_size(line, relative, precision);
		lwline_release(line);
		break;

	case POLYGONTYPE:
		poly=lwgeom_getpoly_inspected(insp, 0);
		size = assvg_polygon_size(poly, relative, precision);
		lwpoly_release(poly);

	case MULTIPOINTTYPE:
		size = assvg_multipoint_size(insp, relative, precision);
		break;

	case MULTILINETYPE:
		size = assvg_multiline_size(insp, relative, precision);
		break;

	case MULTIPOLYGONTYPE:
		size = assvg_multipolygon_size(insp, relative, precision);
		break;

	default:
		lwerror("ST_AsSVG: geometry not supported.");
	}

	return size;
}


static size_t
pointArray_svg_rel(POINTARRAY *pa, char *output, bool close_ring, int precision)
{
	int i, end;
	char *ptr;
	char x[MAX_DIGS_DOUBLE+MAX_DOUBLE_PRECISION+1];
	char y[MAX_DIGS_DOUBLE+MAX_DOUBLE_PRECISION+1];
	POINT2D pt, lpt;

	ptr = output;

	if (close_ring) end = pa->npoints;
	else end = pa->npoints - 1;

	/* Starting point */
	getPoint2d_p(pa, 0, &pt);
	sprintf(x, "%.*f", precision, pt.x);
	trim_trailing_zeros(x);
	sprintf(y, "%.*f", precision, fabs(pt.y) ? pt.y * -1 : pt.y);
	trim_trailing_zeros(y);
	ptr += sprintf(ptr,"%s %s l", x, y);

	/* All the following ones */
	for (i=1 ; i < end ; i++)
	{
		lpt = pt;
		getPoint2d_p(pa, i, &pt);
		sprintf(x, "%.*f", precision, pt.x -lpt.x);
		trim_trailing_zeros(x);
		/* SVG Y axis is reversed, an no need to transform 0 into -0 */
		sprintf(y, "%.*f", precision,
		        fabs(pt.y -lpt.y) ? (pt.y - lpt.y) * -1: (pt.y - lpt.y));
		trim_trailing_zeros(y);
		ptr += sprintf(ptr," %s %s", x, y);
	}

	return (ptr-output);
}


/**
 * Returns maximum size of rendered pointarray in bytes.
 */
static size_t
pointArray_svg_abs(POINTARRAY *pa, char *output, bool close_ring, int precision)
{
	int i, end;
	char *ptr;
	char x[MAX_DIGS_DOUBLE+MAX_DOUBLE_PRECISION+1];
	char y[MAX_DIGS_DOUBLE+MAX_DOUBLE_PRECISION+1];
	POINT2D pt;

	ptr = output;

	if (close_ring) end = pa->npoints;
	else end = pa->npoints - 1;

	for (i=0 ; i < end ; i++)
	{
		getPoint2d_p(pa, i, &pt);
		sprintf(x, "%.*f", precision, pt.x);
		trim_trailing_zeros(x);
		/* SVG Y axis is reversed, an no need to transform 0 into -0 */
		sprintf(y, "%.*f", precision, fabs(pt.y) ? pt.y * -1:pt.y);
		trim_trailing_zeros(y);
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
	return (MAX_DIGS_DOUBLE + precision + sizeof(" "))
	       * 2 * pa->npoints + sizeof(" L ");
}
