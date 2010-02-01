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
* @file GML output routines.
*
**********************************************************************/


#include "postgres.h"

#include "lwgeom_pg.h"
#include "liblwgeom.h"
#include "lwgeom_export.h"


Datum LWGEOM_asGML(PG_FUNCTION_ARGS);

static size_t asgml2_point_size(LWPOINT *point, char *srs, int precision);
static char *asgml2_point(LWPOINT *point, char *srs, int precision);
static size_t asgml2_line_size(LWLINE *line, char *srs, int precision);
static char *asgml2_line(LWLINE *line, char *srs, int precision);
static size_t asgml2_poly_size(LWPOLY *poly, char *srs, int precision);
static char *asgml2_poly(LWPOLY *poly, char *srs, int precision);
static size_t asgml2_multi_size(LWGEOM_INSPECTED *geom, char *srs, int precision);
static char *asgml2_multi(LWGEOM_INSPECTED *geom, char *srs, int precision);
static size_t asgml2_collection_size(LWGEOM_INSPECTED *geom, char *srs, int precision);
static char *asgml2_collection(LWGEOM_INSPECTED *geom, char *srs, int precision);
static size_t pointArray_toGML2(POINTARRAY *pa, char *buf, int precision);

static size_t asgml3_point_size(LWPOINT *point, char *srs, int precision);
static char *asgml3_point(LWPOINT *point, char *srs, int precision, bool is_deegree);
static size_t asgml3_line_size(LWLINE *line, char *srs, int precision);
static char *asgml3_line(LWLINE *line, char *srs, int precision, bool is_deegree);
static size_t asgml3_poly_size(LWPOLY *poly, char *srs, int precision);
static char *asgml3_poly(LWPOLY *poly, char *srs, int precision, bool is_deegree);
static size_t asgml3_multi_size(LWGEOM_INSPECTED *geom, char *srs, int precision);
static char *asgml3_multi(LWGEOM_INSPECTED *geom, char *srs, int precision, bool is_deegree);
static size_t asgml3_collection_size(LWGEOM_INSPECTED *geom, char *srs, int precision);
static char *asgml3_collection(LWGEOM_INSPECTED *insp, char *srs, int precision, bool is_deegree);
static size_t pointArray_toGML3(POINTARRAY *pa, char *buf, int precision, bool is_deegree);

static size_t pointArray_GMLsize(POINTARRAY *pa, int precision);


/**
 * Encode feature in GML
 */
PG_FUNCTION_INFO_V1(LWGEOM_asGML);
Datum LWGEOM_asGML(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	char *gml;
	text *result;
	int len;
	int version;
	char *srs;
	int SRID;
	int precision = MAX_DOUBLE_PRECISION;
	int option=0;
	bool is_deegree = false;

	/* Get the version */
	version = PG_GETARG_INT32(0);
	if ( version != 2 && version != 3 )
	{
		elog(ERROR, "Only GML 2 and GML 3 are supported");
		PG_RETURN_NULL();
	}

	/* Get the geometry */
	if ( PG_ARGISNULL(1) ) PG_RETURN_NULL();
	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	/* Retrieve precision if any (default is max) */
	if (PG_NARGS() >2 && !PG_ARGISNULL(2))
	{
		precision = PG_GETARG_INT32(2);
		if ( precision > MAX_DOUBLE_PRECISION )
			precision = MAX_DOUBLE_PRECISION;
		else if ( precision < 0 ) precision = 0;
	}

	/* retrieve option */
	if (PG_NARGS() >3 && !PG_ARGISNULL(3))
		option = PG_GETARG_INT32(3);

	SRID = lwgeom_getsrid(SERIALIZED_FORM(geom));
	if (SRID == -1) srs = NULL;
	else if (option & 1) srs = getSRSbySRID(SRID, false);
	else srs = getSRSbySRID(SRID, true);

	if (option & 16) is_deegree = true;

	if (version == 2)
		gml = geometry_to_gml2(SERIALIZED_FORM(geom), srs, precision);
	else
		gml = geometry_to_gml3(SERIALIZED_FORM(geom), srs, precision, is_deegree);

	PG_FREE_IF_COPY(geom, 1);

	len = strlen(gml) + VARHDRSZ;

	result = palloc(len);
	SET_VARSIZE(result, len);

	memcpy(VARDATA(result), gml, len-VARHDRSZ);

	pfree(gml);

	PG_RETURN_POINTER(result);
}



/**
 *  @brief VERSION GML 2
 *  	takes a GEOMETRY and returns a GML@ representation
 */
char *
geometry_to_gml2(uchar *geom, char *srs, int precision)
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
		return asgml2_point(point, srs, precision);

	case LINETYPE:
		line = lwline_deserialize(geom);
		return asgml2_line(line, srs, precision);

	case POLYGONTYPE:
		poly = lwpoly_deserialize(geom);
		return asgml2_poly(poly, srs, precision);

	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
		inspected = lwgeom_inspect(geom);
		return asgml2_multi(inspected, srs, precision);

	case COLLECTIONTYPE:
		inspected = lwgeom_inspect(geom);
		return asgml2_collection(inspected, srs, precision);

	default:
		lwerror("geometry_to_gml2: '%s' geometry type not supported", lwgeom_typename(type));
		return NULL;
	}
}

static size_t
asgml2_point_size(LWPOINT *point, char *srs, int precision)
{
	int size;
	size = pointArray_GMLsize(point->point, precision);
	size += sizeof("<gml:point><gml:coordinates>/") * 2;
	if ( srs ) size += strlen(srs) + sizeof(" srsName=..");
	return size;
}

static size_t
asgml2_point_buf(LWPOINT *point, char *srs, char *output, int precision)
{
	char *ptr = output;

	if ( srs )
	{
		ptr += sprintf(ptr, "<gml:Point srsName=\"%s\">", srs);
	}
	else
	{
		ptr += sprintf(ptr, "<gml:Point>");
	}
	ptr += sprintf(ptr, "<gml:coordinates>");
	ptr += pointArray_toGML2(point->point, ptr, precision);
	ptr += sprintf(ptr, "</gml:coordinates></gml:Point>");

	return (ptr-output);
}

static char *
asgml2_point(LWPOINT *point, char *srs, int precision)
{
	char *output;
	int size;

	size = asgml2_point_size(point, srs, precision);
	output = palloc(size);
	asgml2_point_buf(point, srs, output, precision);
	return output;
}

static size_t
asgml2_line_size(LWLINE *line, char *srs, int precision)
{
	int size;
	size = pointArray_GMLsize(line->points, precision);
	size += sizeof("<gml:linestring><gml:coordinates>/") * 2;
	if ( srs ) size += strlen(srs) + sizeof(" srsName=..");
	return size;
}

static size_t
asgml2_line_buf(LWLINE *line, char *srs, char *output, int precision)
{
	char *ptr=output;

	if ( srs )
	{
		ptr += sprintf(ptr, "<gml:LineString srsName=\"%s\">", srs);
	}
	else
	{
		ptr += sprintf(ptr, "<gml:LineString>");
	}
	ptr += sprintf(ptr, "<gml:coordinates>");
	ptr += pointArray_toGML2(line->points, ptr, precision);
	ptr += sprintf(ptr, "</gml:coordinates></gml:LineString>");

	return (ptr-output);
}

static char *
asgml2_line(LWLINE *line, char *srs, int precision)
{
	char *output;
	int size;

	size = asgml2_line_size(line, srs, precision);
	output = palloc(size);
	asgml2_line_buf(line, srs, output, precision);
	return output;
}

static size_t
asgml2_poly_size(LWPOLY *poly, char *srs, int precision)
{
	size_t size;
	int i;

	size = sizeof("<gml:polygon></gml:polygon>");
	size += sizeof("<gml:outerboundaryis><gml:linearring><gml:coordinates>/") * 2;
	size += sizeof("<gml:innerboundaryis><gml:linearring><gml:coordinates>/") * 2 *
	        poly->nrings;
	if ( srs ) size += strlen(srs) + sizeof(" srsName=..");

	for (i=0; i<poly->nrings; i++)
		size += pointArray_GMLsize(poly->rings[i], precision);

	return size;
}

static size_t
asgml2_poly_buf(LWPOLY *poly, char *srs, char *output, int precision)
{
	int i;
	char *ptr=output;

	if ( srs )
	{
		ptr += sprintf(ptr, "<gml:Polygon srsName=\"%s\">", srs);
	}
	else
	{
		ptr += sprintf(ptr, "<gml:Polygon>");
	}
	ptr += sprintf(ptr, "<gml:outerBoundaryIs><gml:LinearRing><gml:coordinates>");
	ptr += pointArray_toGML2(poly->rings[0], ptr, precision);
	ptr += sprintf(ptr, "</gml:coordinates></gml:LinearRing></gml:outerBoundaryIs>");
	for (i=1; i<poly->nrings; i++)
	{
		ptr += sprintf(ptr, "<gml:innerBoundaryIs><gml:LinearRing><gml:coordinates>");
		ptr += pointArray_toGML2(poly->rings[i], ptr, precision);
		ptr += sprintf(ptr, "</gml:coordinates></gml:LinearRing></gml:innerBoundaryIs>");
	}
	ptr += sprintf(ptr, "</gml:Polygon>");

	return (ptr-output);
}

static char *
asgml2_poly(LWPOLY *poly, char *srs, int precision)
{
	char *output;
	int size;

	size = asgml2_poly_size(poly, srs, precision);
	output = palloc(size);
	asgml2_poly_buf(poly, srs, output, precision);
	return output;
}

/*
 * Compute max size required for GML version of this
 * inspected geometry. Will recurse when needed.
 * Don't call this with single-geoms inspected.
 */
static size_t
asgml2_multi_size(LWGEOM_INSPECTED *insp, char *srs, int precision)
{
	int i;
	size_t size;

	/* the longest possible multi version */
	size = sizeof("<gml:MultiLineString></gml:MultiLineString>");

	if ( srs ) size += strlen(srs) + sizeof(" srsName=..");

	for (i=0; i<insp->ngeometries; i++)
	{
		LWPOINT *point;
		LWLINE *line;
		LWPOLY *poly;

		if ((point=lwgeom_getpoint_inspected(insp, i)))
		{
			size += sizeof("<gml:pointMember>/") * 2;
			size += asgml2_point_size(point, 0, precision);
			lwpoint_release(point);
		}
		else if ((line=lwgeom_getline_inspected(insp, i)))
		{
			size += sizeof("<gml:lineStringMember>/") * 2;
			size += asgml2_line_size(line, 0, precision);
			lwline_release(line);
		}
		else if ((poly=lwgeom_getpoly_inspected(insp, i)))
		{
			size += sizeof("<gml:polygonMember>/") * 2;
			size += asgml2_poly_size(poly, 0, precision);
			lwpoly_release(poly);
		}
	}

	return size;
}

/*
 * Don't call this with single-geoms inspected!
 */
static size_t
asgml2_multi_buf(LWGEOM_INSPECTED *insp, char *srs, char *output, int precision)
{
	int type = lwgeom_getType(insp->serialized_form[0]);
	char *ptr, *gmltype;
	int i;

	ptr = output;
	gmltype="";

	if 	(type == MULTIPOINTTYPE)   gmltype = "MultiPoint";
	else if (type == MULTILINETYPE)	   gmltype = "MultiLineString";
	else if (type == MULTIPOLYGONTYPE) gmltype = "MultiPolygon";

	/* Open outmost tag */
	if ( srs )
	{
		ptr += sprintf(ptr, "<gml:%s srsName=\"%s\">", gmltype, srs);
	}
	else
	{
		ptr += sprintf(ptr, "<gml:%s>", gmltype);
	}

	for (i=0; i<insp->ngeometries; i++)
	{
		LWPOINT *point;
		LWLINE *line;
		LWPOLY *poly;

		if ((point=lwgeom_getpoint_inspected(insp, i)))
		{
			ptr += sprintf(ptr, "<gml:pointMember>");
			ptr += asgml2_point_buf(point, 0, ptr, precision);
			lwpoint_release(point);
			ptr += sprintf(ptr, "</gml:pointMember>");
		}
		else if ((line=lwgeom_getline_inspected(insp, i)))
		{
			ptr += sprintf(ptr, "<gml:lineStringMember>");
			ptr += asgml2_line_buf(line, 0, ptr, precision);
			lwline_release(line);
			ptr += sprintf(ptr, "</gml:lineStringMember>");
		}
		else if ((poly=lwgeom_getpoly_inspected(insp, i)))
		{
			ptr += sprintf(ptr, "<gml:polygonMember>");
			ptr += asgml2_poly_buf(poly, 0, ptr, precision);
			lwpoly_release(poly);
			ptr += sprintf(ptr, "</gml:polygonMember>");
		}
	}

	/* Close outmost tag */
	ptr += sprintf(ptr, "</gml:%s>", gmltype);

	return (ptr-output);
}

/*
 * Don't call this with single-geoms inspected!
 */
static char *
asgml2_multi(LWGEOM_INSPECTED *insp, char *srs, int precision)
{
	char *gml;
	size_t size;

	size = asgml2_multi_size(insp, srs, precision);
	gml = palloc(size);
	asgml2_multi_buf(insp, srs, gml, precision);
	return gml;
}


static size_t
asgml2_collection_size(LWGEOM_INSPECTED *insp, char *srs, int precision)
{
	int i;
	size_t size;

	size = sizeof("<gml:MultiGeometry></gml:MultiGeometry>");

	if ( srs ) size += strlen(srs) + sizeof(" srsName=..");

	for (i=0; i<insp->ngeometries; i++)
	{
		LWPOINT *point;
		LWLINE *line;
		LWPOLY *poly;
		LWGEOM_INSPECTED *subinsp;
		uchar *subgeom;

		size += sizeof("<gml:geometryMember>/") * 2;
		if ((point=lwgeom_getpoint_inspected(insp, i)))
		{
			size += asgml2_point_size(point, 0, precision);
			lwpoint_release(point);
		}
		else if ((line=lwgeom_getline_inspected(insp, i)))
		{
			size += asgml2_line_size(line, 0, precision);
			lwline_release(line);
		}
		else if ((poly=lwgeom_getpoly_inspected(insp, i)))
		{
			size += asgml2_poly_size(poly, 0, precision);
			lwpoly_release(poly);
		}
		else
		{
			subgeom = lwgeom_getsubgeometry_inspected(insp, i);
			subinsp = lwgeom_inspect(subgeom);
			size += asgml2_collection_size(subinsp, 0, precision);
			lwinspected_release(subinsp);
		}
	}

	return size;
}

/*
 * Don't call this with single-geoms inspected!
 */
static size_t
asgml2_collection_buf(LWGEOM_INSPECTED *insp, char *srs, char *output, int precision)
{
	char *ptr;
	int i;

	ptr = output;

	/* Open outmost tag */
	if ( srs )
	{
		ptr += sprintf(ptr, "<gml:MultiGeometry srsName=\"%s\">", srs);
	}
	else
	{
		ptr += sprintf(ptr, "<gml:MultiGeometry>");
	}

	for (i=0; i<insp->ngeometries; i++)
	{
		LWPOINT *point;
		LWLINE *line;
		LWPOLY *poly;
		LWGEOM_INSPECTED *subinsp;
		uchar *subgeom;

		ptr += sprintf(ptr, "<gml:geometryMember>");
		if ((point=lwgeom_getpoint_inspected(insp, i)))
		{
			ptr += asgml2_point_buf(point, 0, ptr, precision);
			lwpoint_release(point);
		}
		else if ((line=lwgeom_getline_inspected(insp, i)))
		{
			ptr += asgml2_line_buf(line, 0, ptr, precision);
			lwline_release(line);
		}
		else if ((poly=lwgeom_getpoly_inspected(insp, i)))
		{
			ptr += asgml2_poly_buf(poly, 0, ptr, precision);
			lwpoly_release(poly);
		}
		else
		{
			subgeom = lwgeom_getsubgeometry_inspected(insp, i);
			subinsp = lwgeom_inspect(subgeom);
			if (lwgeom_getType(subgeom[0]) == COLLECTIONTYPE)
				ptr += asgml2_collection_buf(subinsp, 0, ptr, precision);
			else
				ptr += asgml2_multi_buf(subinsp, 0, ptr, precision);
			lwinspected_release(subinsp);
		}
		ptr += sprintf(ptr, "</gml:geometryMember>");
	}

	/* Close outmost tag */
	ptr += sprintf(ptr, "</gml:MultiGeometry>");

	return (ptr-output);
}

/*
 * Don't call this with single-geoms inspected!
 */
static char *
asgml2_collection(LWGEOM_INSPECTED *insp, char *srs, int precision)
{
	char *gml;
	size_t size;

	size = asgml2_collection_size(insp, srs, precision);
	gml = palloc(size);
	asgml2_collection_buf(insp, srs, gml, precision);
	return gml;
}


static size_t
pointArray_toGML2(POINTARRAY *pa, char *output, int precision)
{
	int i;
	char *ptr;
	char x[MAX_DIGS_DOUBLE+MAX_DOUBLE_PRECISION+1];
	char y[MAX_DIGS_DOUBLE+MAX_DOUBLE_PRECISION+1];
	char z[MAX_DIGS_DOUBLE+MAX_DOUBLE_PRECISION+1];

	ptr = output;

	if ( ! TYPE_HASZ(pa->dims) )
	{
		for (i=0; i<pa->npoints; i++)
		{
			POINT2D pt;
			getPoint2d_p(pa, i, &pt);

			if (fabs(pt.x) < MAX_DOUBLE)
				sprintf(x, "%.*f", precision, pt.x);
			else
				sprintf(x, "%g", pt.x);
			trim_trailing_zeros(x);

			if (fabs(pt.y) < MAX_DOUBLE)
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

			if (fabs(pt.x) < MAX_DOUBLE)
				sprintf(x, "%.*f", precision, pt.x);
			else
				sprintf(x, "%g", pt.x);
			trim_trailing_zeros(x);

			if (fabs(pt.y) < MAX_DOUBLE)
				sprintf(y, "%.*f", precision, pt.y);
			else
				sprintf(y, "%g", pt.y);
			trim_trailing_zeros(y);

			if (fabs(pt.z) < MAX_DOUBLE)
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
 * VERSION GML 3.1.1
 */


/* takes a GEOMETRY and returns a GML representation */
char *
geometry_to_gml3(uchar *geom, char *srs, int precision, bool is_deegree)
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
		return asgml3_point(point, srs, precision, is_deegree);

	case LINETYPE:
		line = lwline_deserialize(geom);
		return asgml3_line(line, srs, precision, is_deegree);

	case POLYGONTYPE:
		poly = lwpoly_deserialize(geom);
		return asgml3_poly(poly, srs, precision, is_deegree);

	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
		inspected = lwgeom_inspect(geom);
		return asgml3_multi(inspected, srs, precision, is_deegree);

	case COLLECTIONTYPE:
		inspected = lwgeom_inspect(geom);
		return asgml3_collection(inspected, srs, precision, is_deegree);

	default:
		lwerror("geometry_to_gml3: '%s' geometry type not supported", lwgeom_typename(type));
		return NULL;
	}
}

static size_t
asgml3_point_size(LWPOINT *point, char *srs, int precision)
{
	int size;
	size = pointArray_GMLsize(point->point, precision);
	size += sizeof("<gml:point><gml:pos srsDimension='x'>/") * 2;
	if ( srs ) size += strlen(srs) + sizeof(" srsName=..");
	return size;
}

static size_t
asgml3_point_buf(LWPOINT *point, char *srs, char *output, int precision, bool is_deegree)
{
	char *ptr = output;
	int dimension=2;

	if (TYPE_HASZ(point->type)) dimension = 3;
	if ( srs )
	{
		ptr += sprintf(ptr, "<gml:Point srsName=\"%s\">", srs);
	}
	else
	{
		ptr += sprintf(ptr, "<gml:Point>");
	}
	ptr += sprintf(ptr, "<gml:pos srsDimension=\"%d\">", dimension);
	ptr += pointArray_toGML3(point->point, ptr, precision, is_deegree);
	ptr += sprintf(ptr, "</gml:pos></gml:Point>");

	return (ptr-output);
}

static char *
asgml3_point(LWPOINT *point, char *srs, int precision, bool is_deegree)
{
	char *output;
	int size;

	size = asgml3_point_size(point, srs, precision);
	output = palloc(size);
	asgml3_point_buf(point, srs, output, precision, is_deegree);
	return output;
}


static size_t
asgml3_line_size(LWLINE *line, char *srs, int precision)
{
	int size;
	size = pointArray_GMLsize(line->points, precision);
	size += sizeof("<gml:Curve><gml:segments><gml:LineStringSegment><gml:posList>/") * 2;
	size += sizeof(" srsDimension='x'");
	if ( srs ) size += strlen(srs) + sizeof(" srsName=..");
	return size;
}

static size_t
asgml3_line_buf(LWLINE *line, char *srs, char *output, int precision, bool is_deegree)
{
	char *ptr=output;
	int dimension=2;

	if (TYPE_HASZ(line->type)) dimension = 3;
	if ( srs )
	{
		ptr += sprintf(ptr, "<gml:Curve srsName=\"%s\">", srs);
	}
	else
	{
		ptr += sprintf(ptr, "<gml:Curve>");
	}
	ptr += sprintf(ptr, "<gml:segments>");
	ptr += sprintf(ptr, "<gml:LineStringSegment>");
	ptr += sprintf(ptr, "<gml:posList srsDimension=\"%d\">", dimension);
	ptr += pointArray_toGML3(line->points, ptr, precision, is_deegree);
	ptr += sprintf(ptr, "</gml:posList></gml:LineStringSegment>");
	ptr += sprintf(ptr, "</gml:segments>");
	ptr += sprintf(ptr, "</gml:Curve>");

	return (ptr-output);
}

static char *
asgml3_line(LWLINE *line, char *srs, int precision, bool is_deegree)
{
	char *output;
	int size;

	size = asgml3_line_size(line, srs, precision);
	output = palloc(size);
	asgml3_line_buf(line, srs, output, precision, is_deegree);
	return output;
}


static size_t
asgml3_poly_size(LWPOLY *poly, char *srs, int precision)
{
	size_t size;
	int i;

	size = sizeof("<gml:Polygon>");

	size += sizeof("<gml:exterior><gml:LinearRing><gml:posList srsDimension='x'>");
	size += sizeof("</gml:posList></gml:LinearRing></gml:exterior>");

	size += sizeof("<gml:interior><gml:LinearRing><gml:posList>") * (poly->nrings - 1);
	size += sizeof("</gml:posList></gml:LinearRing></gml:interior>") * (poly->nrings - 1);

	size += sizeof("</gml:Polygon>");

	if ( srs ) size += strlen(srs) + sizeof(" srsName=..");

	for (i=0; i<poly->nrings; i++)
		size += pointArray_GMLsize(poly->rings[i], precision);

	return size;
}

static size_t
asgml3_poly_buf(LWPOLY *poly, char *srs, char *output, int precision, bool is_deegree)
{
	int i;
	char *ptr=output;
	int dimension=2;

	if (TYPE_HASZ(poly->type)) dimension = 3;
	if ( srs )
	{
		ptr += sprintf(ptr, "<gml:Polygon srsName=\"%s\">", srs);
	}
	else
	{
		ptr += sprintf(ptr, "<gml:Polygon>");
	}
	ptr += sprintf(ptr, "<gml:exterior><gml:LinearRing>");
	ptr += sprintf(ptr, "<gml:posList srsDimension=\"%d\">", dimension);
	ptr += pointArray_toGML3(poly->rings[0], ptr, precision, is_deegree);
	ptr += sprintf(ptr, "</gml:posList></gml:LinearRing></gml:exterior>");
	for (i=1; i<poly->nrings; i++)
	{
		ptr += sprintf(ptr, "<gml:interior><gml:LinearRing>");
		ptr += sprintf(ptr, "<gml:posList srsDimension=\"%d\">", dimension);
		ptr += pointArray_toGML3(poly->rings[i], ptr, precision, is_deegree);
		ptr += sprintf(ptr, "</gml:posList></gml:LinearRing></gml:interior>");
	}
	ptr += sprintf(ptr, "</gml:Polygon>");

	return (ptr-output);
}

static char *
asgml3_poly(LWPOLY *poly, char *srs, int precision, bool is_deegree)
{
	char *output;
	int size;

	size = asgml3_poly_size(poly, srs, precision);
	output = palloc(size);
	asgml3_poly_buf(poly, srs, output, precision, is_deegree);
	return output;
}


/*
 * Compute max size required for GML version of this
 * inspected geometry. Will recurse when needed.
 * Don't call this with single-geoms inspected.
 */
static size_t
asgml3_multi_size(LWGEOM_INSPECTED *insp, char *srs, int precision)
{
	int i;
	size_t size;

	/* the longest possible multi version */
	size = sizeof("<gml:MultiLineString></gml:MultiLineString>");

	if ( srs ) size += strlen(srs) + sizeof(" srsName=..");

	for (i=0; i<insp->ngeometries; i++)
	{
		LWPOINT *point;
		LWLINE *line;
		LWPOLY *poly;

		if ((point=lwgeom_getpoint_inspected(insp, i)))
		{
			size += sizeof("<gml:pointMember>/") * 2;
			size += asgml3_point_size(point, 0, precision);
			lwpoint_release(point);
		}
		else if ((line=lwgeom_getline_inspected(insp, i)))
		{
			size += sizeof("<gml:curveMember>/") * 2;
			size += asgml3_line_size(line, 0, precision);
			lwline_release(line);
		}
		else if ((poly=lwgeom_getpoly_inspected(insp, i)))
		{
			size += sizeof("<gml:surfaceMember>/") * 2;
			size += asgml3_poly_size(poly, 0, precision);
			lwpoly_release(poly);
		}
	}

	return size;
}

/*
 * Don't call this with single-geoms inspected!
 */
static size_t
asgml3_multi_buf(LWGEOM_INSPECTED *insp, char *srs, char *output, int precision, bool is_deegree)
{
	int type = lwgeom_getType(insp->serialized_form[0]);
	char *ptr, *gmltype;
	int i;

	ptr = output;
	gmltype="";

	if 	(type == MULTIPOINTTYPE)   gmltype = "MultiPoint";
	else if (type == MULTILINETYPE)    gmltype = "MultiCurve";
	else if (type == MULTIPOLYGONTYPE) gmltype = "MultiSurface";

	/* Open outmost tag */
	if ( srs )
	{
		ptr += sprintf(ptr, "<gml:%s srsName=\"%s\">", gmltype, srs);
	}
	else
	{
		ptr += sprintf(ptr, "<gml:%s>", gmltype);
	}

	for (i=0; i<insp->ngeometries; i++)
	{
		LWPOINT *point;
		LWLINE *line;
		LWPOLY *poly;

		if ((point=lwgeom_getpoint_inspected(insp, i)))
		{
			ptr += sprintf(ptr, "<gml:pointMember>");
			ptr += asgml3_point_buf(point, 0, ptr, precision, is_deegree);
			lwpoint_release(point);
			ptr += sprintf(ptr, "</gml:pointMember>");
		}
		else if ((line=lwgeom_getline_inspected(insp, i)))
		{
			ptr += sprintf(ptr, "<gml:curveMember>");
			ptr += asgml3_line_buf(line, 0, ptr, precision, is_deegree);
			lwline_release(line);
			ptr += sprintf(ptr, "</gml:curveMember>");
		}
		else if ((poly=lwgeom_getpoly_inspected(insp, i)))
		{
			ptr += sprintf(ptr, "<gml:surfaceMember>");
			ptr += asgml3_poly_buf(poly, 0, ptr, precision, is_deegree);
			lwpoly_release(poly);
			ptr += sprintf(ptr, "</gml:surfaceMember>");
		}
	}

	/* Close outmost tag */
	ptr += sprintf(ptr, "</gml:%s>", gmltype);

	return (ptr-output);
}

/*
 * Don't call this with single-geoms inspected!
 */
static char *
asgml3_multi(LWGEOM_INSPECTED *insp, char *srs, int precision, bool is_deegree)
{
	char *gml;
	size_t size;

	size = asgml3_multi_size(insp, srs, precision);
	gml = palloc(size);
	asgml3_multi_buf(insp, srs, gml, precision, is_deegree);
	return gml;
}


static size_t
asgml3_collection_size(LWGEOM_INSPECTED *insp, char *srs, int precision)
{
	int i;
	size_t size;

	size = sizeof("<gml:MultiGeometry></gml:MultiGeometry>");

	if ( srs ) size += strlen(srs) + sizeof(" srsName=..");

	for (i=0; i<insp->ngeometries; i++)
	{
		LWPOINT *point;
		LWLINE *line;
		LWPOLY *poly;
		LWGEOM_INSPECTED *subinsp;
		uchar *subgeom;

		size += sizeof("<gml:geometryMember>/") * 2;
		if ((point=lwgeom_getpoint_inspected(insp, i)))
		{
			size += asgml3_point_size(point, 0, precision);
			lwpoint_release(point);
		}
		else if ((line=lwgeom_getline_inspected(insp, i)))
		{
			size += asgml3_line_size(line, 0, precision);
			lwline_release(line);
		}
		else if ((poly=lwgeom_getpoly_inspected(insp, i)))
		{
			size += asgml3_poly_size(poly, 0, precision);
			lwpoly_release(poly);
		}
		else
		{
			subgeom = lwgeom_getsubgeometry_inspected(insp, i);
			subinsp = lwgeom_inspect(subgeom);
			size += asgml3_multi_size(subinsp, 0, precision);
			lwinspected_release(subinsp);
		}
	}

	return size;
}

static size_t
asgml3_collection_buf(LWGEOM_INSPECTED *insp, char *srs, char *output, int precision, bool is_deegree)
{
	char *ptr;
	int i;

	ptr = output;

	/* Open outmost tag */
	if ( srs )
	{
		ptr += sprintf(ptr, "<gml:MultiGeometry srsName=\"%s\">", srs);
	}
	else
	{
		ptr += sprintf(ptr, "<gml:MultiGeometry>");
	}

	for (i=0; i<insp->ngeometries; i++)
	{
		LWPOINT *point;
		LWLINE *line;
		LWPOLY *poly;
		LWGEOM_INSPECTED *subinsp;
		uchar *subgeom;

		ptr += sprintf(ptr, "<gml:geometryMember>");
		if ((point=lwgeom_getpoint_inspected(insp, i)))
		{
			ptr += asgml3_point_buf(point, 0, ptr, precision, is_deegree);
			lwpoint_release(point);
		}
		else if ((line=lwgeom_getline_inspected(insp, i)))
		{
			ptr += asgml3_line_buf(line, 0, ptr, precision, is_deegree);
			lwline_release(line);
		}
		else if ((poly=lwgeom_getpoly_inspected(insp, i)))
		{
			ptr += asgml3_poly_buf(poly, 0, ptr, precision, is_deegree);
			lwpoly_release(poly);
		}
		else
		{
			subgeom = lwgeom_getsubgeometry_inspected(insp, i);
			subinsp = lwgeom_inspect(subgeom);
			if (lwgeom_getType(subgeom[0]) == COLLECTIONTYPE)
				ptr += asgml3_collection_buf(subinsp, 0, ptr, precision, is_deegree);
			else
				ptr += asgml3_multi_buf(subinsp, 0, ptr, precision, is_deegree);
			lwinspected_release(subinsp);
		}
		ptr += sprintf(ptr, "</gml:geometryMember>");
	}

	/* Close outmost tag */
	ptr += sprintf(ptr, "</gml:MultiGeometry>");

	return (ptr-output);
}

/*
 * Don't call this with single-geoms inspected!
 */
static char *
asgml3_collection(LWGEOM_INSPECTED *insp, char *srs, int precision, bool is_deegree)
{
	char *gml;
	size_t size;

	size = asgml3_collection_size(insp, srs, precision);
	gml = palloc(size);
	asgml3_collection_buf(insp, srs, gml, precision, is_deegree);
	return gml;
}


/* In GML3, inside <posList> or <pos>, coordinates are separated by a space separator
 * In GML3 also, lat/lon are reversed for geocentric data
 */
static size_t
pointArray_toGML3(POINTARRAY *pa, char *output, int precision, bool is_deegree)
{
	int i;
	char *ptr;
	char x[MAX_DIGS_DOUBLE+MAX_DOUBLE_PRECISION+1];
	char y[MAX_DIGS_DOUBLE+MAX_DOUBLE_PRECISION+1];
	char z[MAX_DIGS_DOUBLE+MAX_DOUBLE_PRECISION+1];

	ptr = output;

	if ( ! TYPE_HASZ(pa->dims) )
	{
		for (i=0; i<pa->npoints; i++)
		{
			POINT2D pt;
			getPoint2d_p(pa, i, &pt);

			if (fabs(pt.x) < MAX_DOUBLE)
				sprintf(x, "%.*f", precision, pt.x);
			else
				sprintf(x, "%g", pt.x);
			trim_trailing_zeros(x);

			if (fabs(pt.y) < MAX_DOUBLE)
				sprintf(y, "%.*f", precision, pt.y);
			else
				sprintf(y, "%g", pt.y);
			trim_trailing_zeros(y);

			if ( i ) ptr += sprintf(ptr, " ");
			if (is_deegree)
				ptr += sprintf(ptr, "%s %s", y, x);
			else
				ptr += sprintf(ptr, "%s %s", x, y);
		}
	}
	else
	{
		for (i=0; i<pa->npoints; i++)
		{
			POINT4D pt;
			getPoint4d_p(pa, i, &pt);

			if (fabs(pt.x) < MAX_DOUBLE)
				sprintf(x, "%.*f", precision, pt.x);
			else
				sprintf(x, "%g", pt.x);
			trim_trailing_zeros(x);

			if (fabs(pt.y) < MAX_DOUBLE)
				sprintf(y, "%.*f", precision, pt.y);
			else
				sprintf(y, "%g", pt.y);
			trim_trailing_zeros(y);

			if (fabs(pt.z) < MAX_DOUBLE)
				sprintf(z, "%.*f", precision, pt.z);
			else
				sprintf(z, "%g", pt.z);
			trim_trailing_zeros(z);

			if ( i ) ptr += sprintf(ptr, " ");
			if (is_deegree)
				ptr += sprintf(ptr, "%s %s %s", y, x, z);
			else
				ptr += sprintf(ptr, "%s %s %s", x, y, z);
		}
	}

	return ptr-output;
}



/*
 * Returns maximum size of rendered pointarray in bytes.
 */
static size_t
pointArray_GMLsize(POINTARRAY *pa, int precision)
{
	if (TYPE_NDIMS(pa->dims) == 2)
		return (MAX_DIGS_DOUBLE + precision + sizeof(", "))
		       * 2 * pa->npoints;

	return (MAX_DIGS_DOUBLE + precision + sizeof(", ")) * 3 * pa->npoints;
}
