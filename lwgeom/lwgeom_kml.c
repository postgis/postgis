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
 * KML output routines.
 *
 **********************************************************************/


#include "postgres.h"
#include "executor/spi.h"

#include "lwgeom_pg.h"
#include "liblwgeom.h"

Datum LWGEOM_asKML(PG_FUNCTION_ARGS);
char *geometry_to_kml(uchar *srl, char *srs);

static size_t askml_point_size(LWPOINT *point, char *srs);
static char *askml_point(LWPOINT *point, char *srs);
static size_t askml_line_size(LWLINE *line, char *srs);
static char *askml_line(LWLINE *line, char *srs);
static size_t askml_poly_size(LWPOLY *poly, char *srs);
static char *askml_poly(LWPOLY *poly, char *srs);
static size_t askml_inspected_size(LWGEOM_INSPECTED *geom, char *srs);
static char *askml_inspected(LWGEOM_INSPECTED *geom, char *srs);
static size_t pointArray_KMLsize(POINTARRAY *pa);
static size_t pointArray_toKML(POINTARRAY *pa, char *buf);
static char *getSRSbySRID(int SRID);

#define DEF_PRECISION 15
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
	int version = 2;
	char *srs;
	int SRID;

	precision = DEF_PRECISION;

	if ( PG_ARGISNULL(0) ) PG_RETURN_NULL();

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));

	/* Get precision (if provided)  */
	if ( PG_NARGS() > 1 && ! PG_ARGISNULL(1) )
			precision = PG_GETARG_INT32(1);
	
	if ( precision < 1 || precision > 15 )
	{
		elog(ERROR, "Precision out of range 1..15");
		PG_RETURN_NULL();
	}

	/* Get version (if provided)  */
	if ( PG_NARGS() > 2 && ! PG_ARGISNULL(2) )
			version = PG_GETARG_INT32(2);

	
	if ( version != 2 )
	{
		elog(ERROR, "Only KML 2 is supported");
		PG_RETURN_NULL();
	}

	SRID = lwgeom_getsrid(SERIALIZED_FORM(geom));
	if ( SRID != -1 ) {
	  srs = getSRSbySRID(SRID);
	} else {
		PG_FREE_IF_COPY(geom, 0);
		elog(ERROR,"Input geometry has unknown (-1) SRID");
		PG_RETURN_NULL();
	}

	/*elog(NOTICE, "srs=%s", srs); */

	kml = geometry_to_kml(SERIALIZED_FORM(geom), srs);
	PG_FREE_IF_COPY(geom, 0);

	len = strlen(kml) + VARHDRSZ;

	result = palloc(len);
	VARATT_SIZEP(result) = len;

	memcpy(VARDATA(result), kml, len-VARHDRSZ);

	pfree(kml);

	PG_RETURN_POINTER(result);
}

/* takes a GEOMETRY and returns a KML representation */
char *
geometry_to_kml(uchar *geom, char *srs)
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
			return askml_point(point, srs);

		case LINETYPE:
			line = lwline_deserialize(geom);
			return askml_line(line, srs);

		case POLYGONTYPE:
			poly = lwpoly_deserialize(geom);
			return askml_poly(poly, srs);

		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
			inspected = lwgeom_inspect(geom);
			return askml_inspected(inspected, srs);
		
		default:
			lwerror("geometry_to_kml: '%s' geometry type not supported by Google Earth", lwgeom_typename(type));
			return NULL;
	}
}

static size_t
askml_point_size(LWPOINT *point, char *srs)
{
	int size;
	size = pointArray_KMLsize(point->point);
	size += sizeof("<point><coordinates>/") * 2;
	return size;
}

static size_t
askml_point_buf(LWPOINT *point, char *srs, char *output)
{
	char *ptr = output;

	ptr += sprintf(ptr, "<Point>");
	ptr += sprintf(ptr, "<coordinates>");
	ptr += pointArray_toKML(point->point, ptr);
	ptr += sprintf(ptr, "</coordinates></Point>");

	return (ptr-output);
}

static char *
askml_point(LWPOINT *point, char *srs)
{
	char *output;
	int size;
	
	size = askml_point_size(point, srs);
	output = palloc(size);
	askml_point_buf(point, srs, output);
	return output;
}

static size_t
askml_line_size(LWLINE *line, char *srs)
{
	int size;
	size = pointArray_KMLsize(line->points);
	size += sizeof("<linestring><coordinates>/") * 2;
	return size;
}

static size_t
askml_line_buf(LWLINE *line, char *srs, char *output)
{
	char *ptr=output;

	ptr += sprintf(ptr, "<LineString>");
	ptr += sprintf(ptr, "<coordinates>");
	ptr += pointArray_toKML(line->points, ptr);
	ptr += sprintf(ptr, "</coordinates></LineString>");

	return (ptr-output);
}

static char *
askml_line(LWLINE *line, char *srs)
{
	char *output;
	int size;

	size = askml_line_size(line, srs);
	output = palloc(size);
	askml_line_buf(line, srs, output);
	return output;
}

static size_t
askml_poly_size(LWPOLY *poly, char *srs)
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
askml_poly_buf(LWPOLY *poly, char *srs, char *output)
{
	int i;
	char *ptr=output;

	ptr += sprintf(ptr, "<Polygon>");
	ptr += sprintf(ptr, "<outerBoundaryIs><LinearRing><coordinates>");
	ptr += pointArray_toKML(poly->rings[0], ptr);
	ptr += sprintf(ptr, "</coordinates></LinearRing></outerBoundaryIs>");
	for (i=1; i<poly->nrings; i++)
	{
		ptr += sprintf(ptr, "<innerBoundaryIs><LinearRing><coordinates>");
		ptr += pointArray_toKML(poly->rings[i], ptr);
		ptr += sprintf(ptr, "</coordinates></LinearRing></innerBoundaryIs>");
	}
	ptr += sprintf(ptr, "</Polygon>");

	return (ptr-output);
}

static char *
askml_poly(LWPOLY *poly, char *srs)
{
	char *output;
	int size;

	size = askml_poly_size(poly, srs);
	output = palloc(size);
	askml_poly_buf(poly, srs, output);
	return output;
}

/*
 * Compute max size required for KML version of this 
 * inspected geometry. Will recurse when needed.
 * Don't call this with single-geoms inspected.
 */
static size_t
askml_inspected_size(LWGEOM_INSPECTED *insp, char *srs)
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
			size += askml_point_size(point, 0);
			pfree_point(point);
		}
		else if ((line=lwgeom_getline_inspected(insp, i)))
		{
			size += askml_line_size(line, 0);
			pfree_line(line);
		}
		else if ((poly=lwgeom_getpoly_inspected(insp, i)))
		{
			size += askml_poly_size(poly, 0);
			pfree_polygon(poly);
		}
		else
		{
			subgeom = lwgeom_getsubgeometry_inspected(insp, i);
			subinsp = lwgeom_inspect(subgeom);
			size += askml_inspected_size(subinsp, 0);
			pfree_inspected(subinsp);
		}
	}

	return size;
}

/*
 * Don't call this with single-geoms inspected!
 */
static size_t
askml_inspected_buf(LWGEOM_INSPECTED *insp, char *srs, char *output)
{
	char *ptr, *kmltype;
	int i;

	ptr = output;
	kmltype = "MultiGeometry";

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
			ptr += askml_point_buf(point, 0, ptr);
			pfree_point(point);
		}
		else if ((line=lwgeom_getline_inspected(insp, i)))
		{
			ptr += askml_line_buf(line, 0, ptr);
			pfree_line(line);
		}
		else if ((poly=lwgeom_getpoly_inspected(insp, i)))
		{
			ptr += askml_poly_buf(poly, 0, ptr);
			pfree_polygon(poly);
		}
		else
		{
			subgeom = lwgeom_getsubgeometry_inspected(insp, i);
			subinsp = lwgeom_inspect(subgeom);
			ptr += askml_inspected_buf(subinsp, 0, ptr);
			pfree_inspected(subinsp);
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
askml_inspected(LWGEOM_INSPECTED *insp, char *srs)
{
	char *kml;
	size_t size;

	size = askml_inspected_size(insp, srs);
	kml = palloc(size);
	askml_inspected_buf(insp, srs, kml);
	return kml;
}

/*
 * Returns maximum size of rendered pointarray in bytes.
 */
static size_t
pointArray_KMLsize(POINTARRAY *pa)
{
	return TYPE_NDIMS(pa->dims) * pa->npoints * (SHOW_DIGS+(TYPE_NDIMS(pa->dims)-1));
}

static size_t
pointArray_toKML(POINTARRAY *pa, char *output)
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
			ptr += sprintf(ptr, "%.*g,%.*g,0",
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

static char *
getSRSbySRID(int SRID)
{
	char query[128];
	char *srs, *srscopy;
	int size, err;

	/* connect to SPI */
	if (SPI_OK_CONNECT != SPI_connect ()) {
		elog(NOTICE, "getSRSbySRID: could not connect to SPI manager");
		SPI_finish();
		return NULL;
	}

	/* write query */
	sprintf(query, "SELECT textcat(auth_name, textcat(':', auth_srid)) \
		FROM spatial_ref_sys WHERE srid = '%d'", SRID);
#ifdef PGIS_DEBUG
	elog(NOTICE, "Query: %s", query);
#endif

	/* execute query */
	err = SPI_exec(query, 1);
	if ( err < 0 ) {
		elog(NOTICE, "getSRSbySRID: error executing query %d", err);
		SPI_finish();
		return NULL;
	}

	/* no entry in spatial_ref_sys */
	if (SPI_processed <= 0) {
		/*elog(NOTICE, "getSRSbySRID: no record for SRID %d", SRID); */
		SPI_finish();
		return NULL;
	}

	/* get result  */
	srs = SPI_getvalue(SPI_tuptable->vals[0],
		SPI_tuptable->tupdesc, 1);
	
	/* NULL result */
	if ( ! srs ) {
		/*elog(NOTICE, "getSRSbySRID: null result"); */
		SPI_finish();
		return NULL;
	}

	/* copy result to upper executor context */
	size = strlen(srs)+1;
	srscopy = SPI_palloc(size);
	memcpy(srscopy, srs, size);

	/* disconnect from SPI */
	SPI_finish();

	return srscopy;
}

/**********************************************************************
 * $Log$
 **********************************************************************/

