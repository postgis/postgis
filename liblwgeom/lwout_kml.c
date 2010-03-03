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

/**
* @file
* KML output routines based on lwgeom_gml.c
* Written by: Eduin Carrillo <yecarrillo@cas.gov.co>
*             © 2006 Corporacion Autonoma Regional de Santander - CAS
*
**********************************************************************/

#include "liblwgeom.h"
#include <math.h>	/* fabs */

char *lwgeom_to_kml2(uchar *srl, int precision);

static size_t askml2_point_size(LWPOINT *point, int precision);
static char *askml2_point(LWPOINT *point, int precision);
static size_t askml2_line_size(LWLINE *line, int precision);
static char *askml2_line(LWLINE *line, int precision);
static size_t askml2_poly_size(LWPOLY *poly, int precision);
static char *askml2_poly(LWPOLY *poly, int precision);
static size_t askml2_inspected_size(LWGEOM_INSPECTED *geom, int precision);
static char *askml2_inspected(LWGEOM_INSPECTED *geom, int precision);
static size_t pointArray_toKML2(POINTARRAY *pa, char *buf, int precision);

static size_t pointArray_KMLsize(POINTARRAY *pa, int precision);


/*
 * KML 2.2.0
 */

/* takes a GEOMETRY and returns a KML representation */
char *
lwgeom_to_kml2(uchar *geom, int precision)
{
	int type;
	LWPOINT *point;
	LWLINE *line;
	LWPOLY *poly;
	LWGEOM_INSPECTED *inspected;

	type = lwgeom_getType(geom[0]);

	switch (type)
	{

	case POINTTYPE:
		point = lwpoint_deserialize(geom);
		return askml2_point(point, precision);

	case LINETYPE:
		line = lwline_deserialize(geom);
		return askml2_line(line, precision);

	case POLYGONTYPE:
		poly = lwpoly_deserialize(geom);
		return askml2_poly(poly, precision);

	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
		inspected = lwgeom_inspect(geom);
		return askml2_inspected(inspected, precision);

	default:
		lwerror("lwgeom_to_kml2: '%s' geometry type not supported",
		        lwgeom_typename(type));
		return NULL;
	}
}

static size_t
askml2_point_size(LWPOINT *point, int precision)
{
	int size;
	size = pointArray_KMLsize(point->point, precision);
	size += sizeof("<point><coordinates>/") * 2;
	return size;
}

static size_t
askml2_point_buf(LWPOINT *point, char *output, int precision)
{
	char *ptr = output;

	ptr += sprintf(ptr, "<Point>");
	ptr += sprintf(ptr, "<coordinates>");
	ptr += pointArray_toKML2(point->point, ptr, precision);
	ptr += sprintf(ptr, "</coordinates></Point>");

	return (ptr-output);
}

static char *
askml2_point(LWPOINT *point, int precision)
{
	char *output;
	int size;

	size = askml2_point_size(point, precision);
	output = lwalloc(size);
	askml2_point_buf(point, output, precision);
	return output;
}

static size_t
askml2_line_size(LWLINE *line, int precision)
{
	int size;
	size = pointArray_KMLsize(line->points, precision);
	size += sizeof("<linestring><coordinates>/") * 2;
	return size;
}

static size_t
askml2_line_buf(LWLINE *line, char *output, int precision)
{
	char *ptr=output;

	ptr += sprintf(ptr, "<LineString>");
	ptr += sprintf(ptr, "<coordinates>");
	ptr += pointArray_toKML2(line->points, ptr, precision);
	ptr += sprintf(ptr, "</coordinates></LineString>");

	return (ptr-output);
}

static char *
askml2_line(LWLINE *line, int precision)
{
	char *output;
	int size;

	size = askml2_line_size(line, precision);
	output = lwalloc(size);
	askml2_line_buf(line, output, precision);
	return output;
}

static size_t
askml2_poly_size(LWPOLY *poly, int precision)
{
	size_t size;
	int i;

	size = sizeof("<polygon></polygon>");
	size += sizeof("<outerboundaryis><linearring><coordinates>/") * 2;
	size += sizeof("<innerboundaryis><linearring><coordinates>/") * 2 *
	        poly->nrings;

	for (i=0; i<poly->nrings; i++)
		size += pointArray_KMLsize(poly->rings[i], precision);

	return size;
}

static size_t
askml2_poly_buf(LWPOLY *poly, char *output, int precision)
{
	int i;
	char *ptr=output;

	ptr += sprintf(ptr, "<Polygon>");
	ptr += sprintf(ptr, "<outerBoundaryIs><LinearRing><coordinates>");
	ptr += pointArray_toKML2(poly->rings[0], ptr, precision);
	ptr += sprintf(ptr, "</coordinates></LinearRing></outerBoundaryIs>");
	for (i=1; i<poly->nrings; i++)
	{
		ptr += sprintf(ptr, "<innerBoundaryIs><LinearRing><coordinates>");
		ptr += pointArray_toKML2(poly->rings[i], ptr, precision);
		ptr += sprintf(ptr, "</coordinates></LinearRing></innerBoundaryIs>");
	}
	ptr += sprintf(ptr, "</Polygon>");

	return (ptr-output);
}

static char *
askml2_poly(LWPOLY *poly, int precision)
{
	char *output;
	int size;

	size = askml2_poly_size(poly, precision);
	output = lwalloc(size);
	askml2_poly_buf(poly, output, precision);
	return output;
}

/*
 * Compute max size required for KML version of this
 * inspected geometry. Will recurse when needed.
 * Don't call this with single-geoms inspected.
 */
static size_t
askml2_inspected_size(LWGEOM_INSPECTED *insp, int precision)
{
	int i;
	size_t size;

	/* the longest possible multi version */
	size = sizeof("<MultiGeometry></MultiGeometry>");

	for (i=0; i<insp->ngeometries; i++)
	{
		LWPOINT *point;
		LWLINE *line;
		LWPOLY *poly;
		LWGEOM_INSPECTED *subinsp;
		uchar *subgeom;

		if ((point=lwgeom_getpoint_inspected(insp, i)))
		{
			size += askml2_point_size(point, precision);
			lwpoint_free(point);
		}
		else if ((line=lwgeom_getline_inspected(insp, i)))
		{
			size += askml2_line_size(line, precision);
			lwline_free(line);
		}
		else if ((poly=lwgeom_getpoly_inspected(insp, i)))
		{
			size += askml2_poly_size(poly, precision);
			lwpoly_free(poly);
		}
		else
		{
			subgeom = lwgeom_getsubgeometry_inspected(insp, i);
			subinsp = lwgeom_inspect(subgeom);
			size += askml2_inspected_size(subinsp, precision);
			lwinspected_release(subinsp);
		}
	}

	return size;
}

/*
 * Don't call this with single-geoms inspected!
 */
static size_t
askml2_inspected_buf(LWGEOM_INSPECTED *insp, char *output, int precision)
{
	char *ptr, *kmltype;
	int i;

	ptr = output;
	kmltype = "MultiGeometry";

	/* Open outmost tag */
	ptr += sprintf(ptr, "<%s>", kmltype);

	for (i=0; i<insp->ngeometries; i++)
	{
		LWPOINT *point;
		LWLINE *line;
		LWPOLY *poly;
		LWGEOM_INSPECTED *subinsp;
		uchar *subgeom;

		if ((point=lwgeom_getpoint_inspected(insp, i)))
		{
			ptr += askml2_point_buf(point, ptr, precision);
			lwpoint_free(point);
		}
		else if ((line=lwgeom_getline_inspected(insp, i)))
		{
			ptr += askml2_line_buf(line, ptr, precision);
			lwline_free(line);
		}
		else if ((poly=lwgeom_getpoly_inspected(insp, i)))
		{
			ptr += askml2_poly_buf(poly, ptr, precision);
			lwpoly_free(poly);
		}
		else
		{
			subgeom = lwgeom_getsubgeometry_inspected(insp, i);
			subinsp = lwgeom_inspect(subgeom);
			ptr += askml2_inspected_buf(subinsp, ptr, precision);
			lwinspected_release(subinsp);
		}
	}

	/* Close outmost tag */
	ptr += sprintf(ptr, "</%s>", kmltype);

	return (ptr-output);
}

/*
 * Don't call this with single-geoms inspected!
 */
static char *
askml2_inspected(LWGEOM_INSPECTED *insp, int precision)
{
	char *kml;
	size_t size;

	size = askml2_inspected_size(insp, precision);
	kml = lwalloc(size);
	askml2_inspected_buf(insp, kml, precision);
	return kml;
}

static size_t
pointArray_toKML2(POINTARRAY *pa, char *output, int precision)
{
	int i;
	char *ptr;
	char x[OUT_MAX_DIGS_DOUBLE+OUT_MAX_DOUBLE_PRECISION+1];
	char y[OUT_MAX_DIGS_DOUBLE+OUT_MAX_DOUBLE_PRECISION+1];
	char z[OUT_MAX_DIGS_DOUBLE+OUT_MAX_DOUBLE_PRECISION+1];

	ptr = output;

	if ( ! TYPE_HASZ(pa->dims) )
	{
		for (i=0; i<pa->npoints; i++)
		{
			POINT2D pt;
			getPoint2d_p(pa, i, &pt);

			if (fabs(pt.x) < OUT_MAX_DOUBLE)
				sprintf(x, "%.*f", precision, pt.x);
			else
				sprintf(x, "%g", pt.x);
			trim_trailing_zeros(x);

			if (fabs(pt.y) < OUT_MAX_DOUBLE)
				sprintf(y, "%.*f", precision, pt.y);
			else
				sprintf(y, "%g", pt.y);
			trim_trailing_zeros(y);

			if ( i ) ptr += sprintf(ptr, " ");
			ptr += sprintf(ptr, "%s,%s", x, y);
		}
	}
	else
	{
		for (i=0; i<pa->npoints; i++)
		{
			POINT4D pt;
			getPoint4d_p(pa, i, &pt);

			if (fabs(pt.x) < OUT_MAX_DOUBLE)
				sprintf(x, "%.*f", precision, pt.x);
			else
				sprintf(x, "%g", pt.x);
			trim_trailing_zeros(x);

			if (fabs(pt.y) < OUT_MAX_DOUBLE)
				sprintf(y, "%.*f", precision, pt.y);
			else
				sprintf(y, "%g", pt.y);
			trim_trailing_zeros(y);

			if (fabs(pt.z) < OUT_MAX_DOUBLE)
				sprintf(z, "%.*f", precision, pt.z);
			else
				sprintf(z, "%g", pt.z);
			trim_trailing_zeros(z);

			if ( i ) ptr += sprintf(ptr, " ");
			ptr += sprintf(ptr, "%s,%s,%s", x, y, z);
		}
	}

	return ptr-output;
}



/*
 * Common KML routines
 */

/*
 * Returns maximum size of rendered pointarray in bytes.
 */
static size_t
pointArray_KMLsize(POINTARRAY *pa, int precision)
{
	if (TYPE_NDIMS(pa->dims) == 2)
		return (OUT_MAX_DIGS_DOUBLE + precision + sizeof(", "))
		       * 2 * pa->npoints;

	return (OUT_MAX_DIGS_DOUBLE + precision + sizeof(", ")) * 3 * pa->npoints;
}

