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

char *geometry_to_kml2(uchar *srl);

static size_t askml2_point_size(LWPOINT *point);
static char *askml2_point(LWPOINT *point);
static size_t askml2_line_size(LWLINE *line);
static char *askml2_line(LWLINE *line);
static size_t askml2_poly_size(LWPOLY *poly);
static char *askml2_poly(LWPOLY *poly);
static size_t askml2_inspected_size(LWGEOM_INSPECTED *geom);
static char *askml2_inspected(LWGEOM_INSPECTED *geom);
static size_t pointArray_toKML2(POINTARRAY *pa, char *buf);

static size_t pointArray_KMLsize(POINTARRAY *pa);

/* Add dot, sign, exponent sign, 'e', exponent digits */
#define SHOW_DIGS (precision + 8)

/* Globals */
int precision;


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

	/* Get precision  */
	precision = PG_GETARG_INT32(2);
	if ( precision < 1 || precision > 15 )
	{
		elog(ERROR, "Precision out of range 1..15");
		PG_RETURN_NULL();
	}
	
	if (version == 2)
	  kml = geometry_to_kml2(SERIALIZED_FORM(geom));
	
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
geometry_to_kml2(uchar *geom)
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
			return askml2_point(point);

		case LINETYPE:
			line = lwline_deserialize(geom);
			return askml2_line(line);

		case POLYGONTYPE:
			poly = lwpoly_deserialize(geom);
			return askml2_poly(poly);

		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
			inspected = lwgeom_inspect(geom);
			return askml2_inspected(inspected);
		
		default:
			lwerror("geometry_to_kml: '%s' geometry type not supported by Google Earth", lwgeom_typename(type));
			return NULL;
	}
}

static size_t
askml2_point_size(LWPOINT *point)
{
	int size;
	size = pointArray_KMLsize(point->point);
	size += sizeof("<point><coordinates>/") * 2;
	return size;
}

static size_t
askml2_point_buf(LWPOINT *point, char *output)
{
	char *ptr = output;

	ptr += sprintf(ptr, "<Point>");
	ptr += sprintf(ptr, "<coordinates>");
	ptr += pointArray_toKML2(point->point, ptr);
	ptr += sprintf(ptr, "</coordinates></Point>");

	return (ptr-output);
}

static char *
askml2_point(LWPOINT *point)
{
	char *output;
	int size;
	
	size = askml2_point_size(point);
	output = palloc(size);
	askml2_point_buf(point, output);
	return output;
}

static size_t
askml2_line_size(LWLINE *line)
{
	int size;
	size = pointArray_KMLsize(line->points);
	size += sizeof("<linestring><coordinates>/") * 2;
	return size;
}

static size_t
askml2_line_buf(LWLINE *line, char *output)
{
	char *ptr=output;

	ptr += sprintf(ptr, "<LineString>");
	ptr += sprintf(ptr, "<coordinates>");
	ptr += pointArray_toKML2(line->points, ptr);
	ptr += sprintf(ptr, "</coordinates></LineString>");

	return (ptr-output);
}

static char *
askml2_line(LWLINE *line)
{
	char *output;
	int size;

	size = askml2_line_size(line);
	output = palloc(size);
	askml2_line_buf(line, output);
	return output;
}

static size_t
askml2_poly_size(LWPOLY *poly)
{
	size_t size;
	int i;

	size = sizeof("<polygon></polygon>");
	size += sizeof("<outerboundaryis><linearring><coordinates>/") * 2;
	size += sizeof("<innerboundaryis><linearring><coordinates>/") * 2 *
		poly->nrings;

	for (i=0; i<poly->nrings; i++)
		size += pointArray_KMLsize(poly->rings[i]);

	return size;
}

static size_t
askml2_poly_buf(LWPOLY *poly, char *output)
{
	int i;
	char *ptr=output;

	ptr += sprintf(ptr, "<Polygon>");
	ptr += sprintf(ptr, "<outerBoundaryIs><LinearRing><coordinates>");
	ptr += pointArray_toKML2(poly->rings[0], ptr);
	ptr += sprintf(ptr, "</coordinates></LinearRing></outerBoundaryIs>");
	for (i=1; i<poly->nrings; i++)
	{
		ptr += sprintf(ptr, "<innerBoundaryIs><LinearRing><coordinates>");
		ptr += pointArray_toKML2(poly->rings[i], ptr);
		ptr += sprintf(ptr, "</coordinates></LinearRing></innerBoundaryIs>");
	}
	ptr += sprintf(ptr, "</Polygon>");

	return (ptr-output);
}

static char *
askml2_poly(LWPOLY *poly)
{
	char *output;
	int size;

	size = askml2_poly_size(poly);
	output = palloc(size);
	askml2_poly_buf(poly, output);
	return output;
}

/*
 * Compute max size required for KML version of this 
 * inspected geometry. Will recurse when needed.
 * Don't call this with single-geoms inspected.
 */
static size_t
askml2_inspected_size(LWGEOM_INSPECTED *insp)
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
			size += askml2_point_size(point);
			lwpoint_free(point);
		}
		else if ((line=lwgeom_getline_inspected(insp, i)))
		{
			size += askml2_line_size(line);
			lwline_free(line);
		}
		else if ((poly=lwgeom_getpoly_inspected(insp, i)))
		{
			size += askml2_poly_size(poly);
			lwpoly_free(poly);
		}
		else
		{
			subgeom = lwgeom_getsubgeometry_inspected(insp, i);
			subinsp = lwgeom_inspect(subgeom);
			size += askml2_inspected_size(subinsp);
			lwinspected_release(subinsp);
		}
	}

	return size;
}

/*
 * Don't call this with single-geoms inspected!
 */
static size_t
askml2_inspected_buf(LWGEOM_INSPECTED *insp, char *output)
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
			ptr += askml2_point_buf(point, ptr);
			lwpoint_free(point);
		}
		else if ((line=lwgeom_getline_inspected(insp, i)))
		{
			ptr += askml2_line_buf(line, ptr);
			lwline_free(line);
		}
		else if ((poly=lwgeom_getpoly_inspected(insp, i)))
		{
			ptr += askml2_poly_buf(poly, ptr);
			lwpoly_free(poly);
		}
		else
		{
			subgeom = lwgeom_getsubgeometry_inspected(insp, i);
			subinsp = lwgeom_inspect(subgeom);
			ptr += askml2_inspected_buf(subinsp, ptr);
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
askml2_inspected(LWGEOM_INSPECTED *insp)
{
	char *kml;
	size_t size;

	size = askml2_inspected_size(insp);
	kml = palloc(size);
	askml2_inspected_buf(insp, kml);
	return kml;
}

static size_t
pointArray_toKML2(POINTARRAY *pa, char *output)
{
	int i;
	char *ptr;

	ptr = output;

	if ( ! TYPE_HASZ(pa->dims) )
	{
		for (i=0; i<pa->npoints; i++)
		{
			POINT2D pt;
			getPoint2d_p(pa, i, &pt);
			if ( i ) ptr += sprintf(ptr, " ");
			ptr += sprintf(ptr, "%.*g,%.*g",
				precision, pt.x,
				precision, pt.y);
		}
	}
	else 
	{
		for (i=0; i<pa->npoints; i++)
		{
			POINT4D pt;
			getPoint4d_p(pa, i, &pt);
			if ( i ) ptr += sprintf(ptr, " ");
			ptr += sprintf(ptr, "%.*g,%.*g,%.*g",
				precision, pt.x,
				precision, pt.y,
				precision, pt.z);
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
pointArray_KMLsize(POINTARRAY *pa)
{
	return TYPE_NDIMS(pa->dims) * pa->npoints * (SHOW_DIGS+(TYPE_NDIMS(pa->dims)-1));
}

/**********************************************************************
 * $Log: $
 **********************************************************************/

