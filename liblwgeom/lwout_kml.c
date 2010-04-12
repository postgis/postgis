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

char *lwgeom_to_kml2(uchar *srl, int precision, const char *prefix);

static size_t askml2_point_size(LWPOINT *point, int precision, const char *prefix);
static char *askml2_point(LWPOINT *point, int precision, const char *prefix);
static size_t askml2_line_size(LWLINE *line, int precision, const char *prefix);
static char *askml2_line(LWLINE *line, int precision, const char *prefix);
static size_t askml2_poly_size(LWPOLY *poly, int precision, const char *prefix);
static char *askml2_poly(LWPOLY *poly, int precision, const char *prefix);
static size_t askml2_inspected_size(LWGEOM_INSPECTED *geom, int precision, const char *prefix);
static char *askml2_inspected(LWGEOM_INSPECTED *geom, int precision, const char *prefix);
static size_t pointArray_toKML2(POINTARRAY *pa, char *buf, int precision);

static size_t pointArray_KMLsize(POINTARRAY *pa, int precision);


/*
 * KML 2.2.0
 */

/* takes a GEOMETRY and returns a KML representation */
char *
lwgeom_to_kml2(uchar *geom, int precision, const char *prefix)
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
		return askml2_point(point, precision, prefix);

	case LINETYPE:
		line = lwline_deserialize(geom);
		return askml2_line(line, precision, prefix);

	case POLYGONTYPE:
		poly = lwpoly_deserialize(geom);
		return askml2_poly(poly, precision, prefix);

	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
		inspected = lwgeom_inspect(geom);
		return askml2_inspected(inspected, precision, prefix);

	default:
		lwerror("lwgeom_to_kml2: '%s' geometry type not supported",
		        lwtype_name(type));
		return NULL;
	}
}

static size_t
askml2_point_size(LWPOINT *point, int precision, const char *prefix)
{
	int size;
	size_t prefixlen = strlen(prefix);

	size = pointArray_KMLsize(point->point, precision);
	size += sizeof("<point><coordinates>/") * 2;
	size += prefixlen * 2 * 2;
	return size;
}

static size_t
askml2_point_buf(LWPOINT *point, char *output, int precision, const char *prefix)
{
	char *ptr = output;

	ptr += sprintf(ptr, "<%sPoint>", prefix);
	ptr += sprintf(ptr, "<%scoordinates>", prefix);
	ptr += pointArray_toKML2(point->point, ptr, precision);
	ptr += sprintf(ptr, "</%scoordinates></%sPoint>", prefix, prefix);

	return (ptr-output);
}

static char *
askml2_point(LWPOINT *point, int precision, const char *prefix)
{
	char *output;
	int size;

	size = askml2_point_size(point, precision, prefix);
	output = lwalloc(size);
	askml2_point_buf(point, output, precision, prefix);
	return output;
}

static size_t
askml2_line_size(LWLINE *line, int precision, const char * prefix)
{
	int size;
	size_t prefixlen = strlen(prefix);

	size = pointArray_KMLsize(line->points, precision);
	size += sizeof("<linestring><coordinates>/") * 2;
	size += prefixlen * 2 * 2;
	return size;
}

static size_t
askml2_line_buf(LWLINE *line, char *output, int precision, const char *prefix)
{
	char *ptr=output;

	ptr += sprintf(ptr, "<%sLineString>", prefix);
	ptr += sprintf(ptr, "<%scoordinates>", prefix);
	ptr += pointArray_toKML2(line->points, ptr, precision);
	ptr += sprintf(ptr, "</%scoordinates></%sLineString>", prefix, prefix);

	return (ptr-output);
}

static char *
askml2_line(LWLINE *line, int precision, const char *prefix)
{
	char *output;
	int size;

	size = askml2_line_size(line, precision, prefix);
	output = lwalloc(size);
	askml2_line_buf(line, output, precision, prefix);
	return output;
}

static size_t
askml2_poly_size(LWPOLY *poly, int precision, const char *prefix)
{
	size_t size;
	int i;
	size_t prefixlen = strlen(prefix);

	size = sizeof("<polygon></polygon>");
	size += sizeof("<outerboundaryis><linearring><coordinates>/") * 2;
	size += sizeof("<innerboundaryis><linearring><coordinates>/") * 2 *
	        poly->nrings;
	
	size += prefixlen * (1 + 3 + (3 * poly->nrings)) * 2;

	for (i=0; i<poly->nrings; i++)
		size += pointArray_KMLsize(poly->rings[i], precision);

	return size;
}

static size_t
askml2_poly_buf(LWPOLY *poly, char *output, int precision, const char *prefix)
{
	int i;
	char *ptr=output;

	ptr += sprintf(ptr, "<%sPolygon>", prefix);
	ptr += sprintf(ptr, "<%souterBoundaryIs><%sLinearRing><%scoordinates>",
		prefix, prefix, prefix);
	ptr += pointArray_toKML2(poly->rings[0], ptr, precision);
	ptr += sprintf(ptr, "</%scoordinates></%sLinearRing></%souterBoundaryIs>",
		prefix, prefix, prefix);

	for (i=1; i<poly->nrings; i++)
	{
		ptr += sprintf(ptr,
			"<%sinnerBoundaryIs><%sLinearRing><%scoordinates>",
			prefix, prefix, prefix);

		ptr += pointArray_toKML2(poly->rings[i], ptr, precision);

		ptr += sprintf(ptr,
			"</%scoordinates></%sLinearRing></%sinnerBoundaryIs>",
			prefix, prefix, prefix);
	}
	ptr += sprintf(ptr, "</%sPolygon>", prefix);

	return (ptr-output);
}

static char *
askml2_poly(LWPOLY *poly, int precision, const char *prefix)
{
	char *output;
	int size;

	size = askml2_poly_size(poly, precision, prefix);
	output = lwalloc(size);
	askml2_poly_buf(poly, output, precision, prefix);
	return output;
}

/*
 * Compute max size required for KML version of this
 * inspected geometry. Will recurse when needed.
 * Don't call this with single-geoms inspected.
 */
static size_t
askml2_inspected_size(LWGEOM_INSPECTED *insp, int precision, const char *prefix)
{
	int i;
	size_t size;
	size_t prefixlen = strlen(prefix);

	/* the longest possible multi version */
	size = sizeof("<MultiGeometry></MultiGeometry>") + prefixlen * 2;

	for (i=0; i<insp->ngeometries; i++)
	{
		LWPOINT *point;
		LWLINE *line;
		LWPOLY *poly;
		LWGEOM_INSPECTED *subinsp;
		uchar *subgeom;

		if ((point=lwgeom_getpoint_inspected(insp, i)))
		{
			size += askml2_point_size(point, precision, prefix);
			lwpoint_free(point);
		}
		else if ((line=lwgeom_getline_inspected(insp, i)))
		{
			size += askml2_line_size(line, precision, prefix);
			lwline_free(line);
		}
		else if ((poly=lwgeom_getpoly_inspected(insp, i)))
		{
			size += askml2_poly_size(poly, precision, prefix);
			lwpoly_free(poly);
		}
		else
		{
			subgeom = lwgeom_getsubgeometry_inspected(insp, i);
			subinsp = lwgeom_inspect(subgeom);
			size += askml2_inspected_size(subinsp, precision, prefix);
			lwinspected_release(subinsp);
		}
	}

	return size;
}

/*
 * Don't call this with single-geoms inspected!
 */
static size_t
askml2_inspected_buf(LWGEOM_INSPECTED *insp, char *output, int precision, const char *prefix)
{
	char *ptr, *kmltype;
	int i;

	ptr = output;
	kmltype = "MultiGeometry";

	/* Open outmost tag */
	ptr += sprintf(ptr, "<%s%s>", prefix, kmltype);

	for (i=0; i<insp->ngeometries; i++)
	{
		LWPOINT *point;
		LWLINE *line;
		LWPOLY *poly;
		LWGEOM_INSPECTED *subinsp;
		uchar *subgeom;

		if ((point=lwgeom_getpoint_inspected(insp, i)))
		{
			ptr += askml2_point_buf(point, ptr, precision, prefix);
			lwpoint_free(point);
		}
		else if ((line=lwgeom_getline_inspected(insp, i)))
		{
			ptr += askml2_line_buf(line, ptr, precision, prefix);
			lwline_free(line);
		}
		else if ((poly=lwgeom_getpoly_inspected(insp, i)))
		{
			ptr += askml2_poly_buf(poly, ptr, precision, prefix);
			lwpoly_free(poly);
		}
		else
		{
			subgeom = lwgeom_getsubgeometry_inspected(insp, i);
			subinsp = lwgeom_inspect(subgeom);
			ptr += askml2_inspected_buf(subinsp, ptr, precision, prefix);
			lwinspected_release(subinsp);
		}
	}

	/* Close outmost tag */
	ptr += sprintf(ptr, "</%s%s>",prefix,  kmltype);

	return (ptr-output);
}

/*
 * Don't call this with single-geoms inspected!
 */
static char *
askml2_inspected(LWGEOM_INSPECTED *insp, int precision, const char *prefix)
{
	char *kml;
	size_t size;

	size = askml2_inspected_size(insp, precision, prefix);
	kml = lwalloc(size);
	askml2_inspected_buf(insp, kml, precision, prefix);
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

