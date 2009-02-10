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
 **********************************************************************
 *
 * KML output routines based on lwgeom_gml.c
 * Written by: Eduin Carrillo <yecarrillo@cas.gov.co>
 *             © 2006 Corporacion Autonoma Regional de Santander - CAS
 *
 **********************************************************************/


#include "postgres.h"
#include "executor/spi.h"

#include "lwgeom_pg.h"
#include "liblwgeom.h"

Datum LWGEOM_asKML(PG_FUNCTION_ARGS);

char *geometry_to_kml2(uchar *srl, int precision);

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

/* Add dot, sign, exponent sign, 'e', exponent digits */
#define SHOW_DIGS_DOUBLE 15
#define MAX_DOUBLE_PRECISION 15
#define MAX_DIGS_DOUBLE (SHOW_DIGS_DOUBLE + 2) /* +2 mean add dot and sign */


/**
 * Encode feature in KML 
 */
PG_FUNCTION_INFO_V1(LWGEOM_asKML);
Datum LWGEOM_asKML(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	char *kml;
	text *result;
	int len;
	int version;
	int precision = MAX_DOUBLE_PRECISION;


	/* Get the version */
	version = PG_GETARG_INT32(0);
	if ( version != 2)
	{
		elog(ERROR, "Only KML 2 is supported");
		PG_RETURN_NULL();
	}

	/* Get the geometry */
	if ( PG_ARGISNULL(1) ) PG_RETURN_NULL();
	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	/* Retrieve precision if any (default is max) */
	if (PG_NARGS() >2 && !PG_ARGISNULL(2)) {
		precision = PG_GETARG_INT32(2);
		if ( precision > MAX_DOUBLE_PRECISION )
			precision = MAX_DOUBLE_PRECISION;
		else if ( precision < 0 ) precision = 0;
	}
	
	if (version == 2)
		kml = geometry_to_kml2(SERIALIZED_FORM(geom), precision);
	
	PG_FREE_IF_COPY(geom, 1);

	len = strlen(kml) + VARHDRSZ;

	result = palloc(len);
	SET_VARSIZE(result, len);

	memcpy(VARDATA(result), kml, len-VARHDRSZ);

	pfree(kml);

	PG_RETURN_POINTER(result);
}



/*
 * VERSION KML 2 
 */

/* takes a GEOMETRY and returns a KML representation */
char *
geometry_to_kml2(uchar *geom, int precision)
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
			lwerror("geometry_to_kml: '%s' geometry type not supported by Google Earth", lwgeom_typename(type));
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
	output = palloc(size);
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
	output = palloc(size);
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
	output = palloc(size);
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
	kml = palloc(size);
	askml2_inspected_buf(insp, kml, precision);
	return kml;
}

static size_t
pointArray_toKML2(POINTARRAY *pa, char *output, int precision)
{
	int i;
	char *ptr;
	char x[MAX_DIGS_DOUBLE+3];
	char y[MAX_DIGS_DOUBLE+3];
	char z[MAX_DIGS_DOUBLE+3];

	ptr = output;

	if ( ! TYPE_HASZ(pa->dims) )
	{
		for (i=0; i<pa->npoints; i++)
		{
			POINT2D pt;
			getPoint2d_p(pa, i, &pt);
			sprintf(x, "%.*f", precision, pt.x);
			trim_trailing_zeros(x);
			sprintf(y, "%.*f", precision, pt.y);
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
			sprintf(x, "%.*f", precision, pt.x);
			trim_trailing_zeros(x);
			sprintf(y, "%.*f", precision, pt.y);
			trim_trailing_zeros(y);
			sprintf(z, "%.*f", precision, pt.z);
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
		return (MAX_DIGS_DOUBLE + precision + sizeof(", "))
			* 2 * pa->npoints;

	return (MAX_DIGS_DOUBLE + precision + sizeof(", ")) * 3 * pa->npoints;
}

