/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2003 Refractions Research Inc.
 * Copyright 2010 Oslandia
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

/**
* @file GML output routines.
*
**********************************************************************/


#include <string.h>
#include <math.h> /* fabs */
#include "liblwgeom.h"
#include "liblwgeom.h"

static size_t asgml2_point_size(LWPOINT *point, char *srs, int precision, const char *prefix);
static char *asgml2_point(LWPOINT *point, char *srs, int precision, const char *prefix);
static size_t asgml2_line_size(LWLINE *line, char *srs, int precision, const char *prefix);
static char *asgml2_line(LWLINE *line, char *srs, int precision, const char *prefix);
static size_t asgml2_poly_size(LWPOLY *poly, char *srs, int precision, const char *prefix);
static char *asgml2_poly(LWPOLY *poly, char *srs, int precision, const char *prefix);
static size_t asgml2_multi_size(LWGEOM_INSPECTED *geom, char *srs, int precision, const char *prefix);
static char *asgml2_multi(LWGEOM_INSPECTED *geom, char *srs, int precision, const char *prefix);
static size_t asgml2_collection_size(LWGEOM_INSPECTED *geom, char *srs, int precision, const char *prefix);
static char *asgml2_collection(LWGEOM_INSPECTED *geom, char *srs, int precision, const char *prefix);
static size_t pointArray_toGML2(POINTARRAY *pa, char *buf, int precision);

static size_t asgml3_point_size(LWPOINT *point, char *srs, int precision, const char *prefix);
static char *asgml3_point(LWPOINT *point, char *srs, int precision, int is_deegree, const char *prefix);
static size_t asgml3_line_size(LWLINE *line, char *srs, int precision, const char *prefix);
static char *asgml3_line(LWLINE *line, char *srs, int precision, int is_deegree, const char *prefix);
static size_t asgml3_poly_size(LWPOLY *poly, char *srs, int precision, const char *prefix);
static char *asgml3_poly(LWPOLY *poly, char *srs, int precision, int is_deegree, const char *prefix);
static size_t asgml3_multi_size(LWGEOM_INSPECTED *geom, char *srs, int precision, const char *prefix);
static char *asgml3_multi(LWGEOM_INSPECTED *geom, char *srs, int precision, int is_deegree, const char *prefix);
static size_t asgml3_collection_size(LWGEOM_INSPECTED *geom, char *srs, int precision, const char *prefix);
static char *asgml3_collection(LWGEOM_INSPECTED *insp, char *srs, int precision, int is_deegree, const char *prefix);
static size_t pointArray_toGML3(POINTARRAY *pa, char *buf, int precision, int is_deegree);

static size_t pointArray_GMLsize(POINTARRAY *pa, int precision);



/**
 *  @brief VERSION GML 2
 *  	takes a GEOMETRY and returns a GML@ representation
 */
extern char *
lwgeom_to_gml2(uchar *geom, char *srs, int precision, const char* prefix)
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
		return asgml2_point(point, srs, precision, prefix);

	case LINETYPE:
		line = lwline_deserialize(geom);
		return asgml2_line(line, srs, precision, prefix);

	case POLYGONTYPE:
		poly = lwpoly_deserialize(geom);
		return asgml2_poly(poly, srs, precision, prefix);

	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
		inspected = lwgeom_inspect(geom);
		return asgml2_multi(inspected, srs, precision, prefix);

	case COLLECTIONTYPE:
		inspected = lwgeom_inspect(geom);
		return asgml2_collection(inspected, srs, precision, prefix);

	default:
		lwerror("lwgeom_to_gml2: '%s' geometry type not supported",
		        lwtype_name(type));
		return NULL;
	}
}

static size_t
asgml2_point_size(LWPOINT *point, char *srs, int precision, const char* prefix)
{
	int size;
	size_t prefixlen = strlen(prefix);

	size = pointArray_GMLsize(point->point, precision);
	size += ( sizeof("<point><coordinates>/") + (prefixlen*2) ) * 2;
	if ( srs ) size += strlen(srs) + sizeof(" srsName=..");
	return size;
}

static size_t
asgml2_point_buf(LWPOINT *point, char *srs, char *output, int precision, const char* prefix)
{
	char *ptr = output;

	if ( srs )
	{
		ptr += sprintf(ptr, "<%sPoint srsName=\"%s\">", prefix, srs);
	}
	else
	{
		ptr += sprintf(ptr, "<%sPoint>", prefix);
	}
	ptr += sprintf(ptr, "<%scoordinates>", prefix);
	ptr += pointArray_toGML2(point->point, ptr, precision);
	ptr += sprintf(ptr, "</%scoordinates></%sPoint>", prefix, prefix);

	return (ptr-output);
}

static char *
asgml2_point(LWPOINT *point, char *srs, int precision, const char *prefix)
{
	char *output;
	int size;

	size = asgml2_point_size(point, srs, precision, prefix);
	output = lwalloc(size);
	asgml2_point_buf(point, srs, output, precision, prefix);
	return output;
}

static size_t
asgml2_line_size(LWLINE *line, char *srs, int precision, const char *prefix)
{
	int size;
	size_t prefixlen = strlen(prefix);

	size = pointArray_GMLsize(line->points, precision);
	size += ( sizeof("<linestring><coordinates>/") + (prefixlen*2) ) * 2;
	if ( srs ) size += strlen(srs) + sizeof(" srsName=..");
	return size;
}

static size_t
asgml2_line_buf(LWLINE *line, char *srs, char *output, int precision,
	const char *prefix)
{
	char *ptr=output;

	if ( srs )
	{
		ptr += sprintf(ptr, "<%sLineString srsName=\"%s\">", prefix, srs);
	}
	else
	{
		ptr += sprintf(ptr, "<%sLineString>", prefix);
	}
	ptr += sprintf(ptr, "<%scoordinates>", prefix);
	ptr += pointArray_toGML2(line->points, ptr, precision);
	ptr += sprintf(ptr, "</%scoordinates></%sLineString>", prefix, prefix);

	return (ptr-output);
}

static char *
asgml2_line(LWLINE *line, char *srs, int precision, const char *prefix)
{
	char *output;
	int size;

	size = asgml2_line_size(line, srs, precision, prefix);
	output = lwalloc(size);
	asgml2_line_buf(line, srs, output, precision, prefix);
	return output;
}

static size_t
asgml2_poly_size(LWPOLY *poly, char *srs, int precision, const char *prefix)
{
	size_t size;
	int i;
	size_t prefixlen = strlen(prefix);

	size = sizeof("<polygon></polygon>") + prefixlen*2;
	size += ( sizeof("<outerboundaryis><linearring><coordinates>/") + ( prefixlen*3) ) * 2;
	size += ( sizeof("<innerboundaryis><linearring><coordinates>/") + ( prefixlen*2) ) * 2 * poly->nrings;
	if ( srs ) size += strlen(srs) + sizeof(" srsName=..");

	for (i=0; i<poly->nrings; i++)
		size += pointArray_GMLsize(poly->rings[i], precision);

	return size;
}

static size_t
asgml2_poly_buf(LWPOLY *poly, char *srs, char *output, int precision,
	const char *prefix)
{
	int i;
	char *ptr=output;

	if ( srs )
	{
		ptr += sprintf(ptr, "<%sPolygon srsName=\"%s\">", prefix, srs);
	}
	else
	{
		ptr += sprintf(ptr, "<%sPolygon>", prefix);
	}
	ptr += sprintf(ptr, "<%souterBoundaryIs><%sLinearRing><%scoordinates>",
		prefix, prefix, prefix);
	ptr += pointArray_toGML2(poly->rings[0], ptr, precision);
	ptr += sprintf(ptr, "</%scoordinates></%sLinearRing></%souterBoundaryIs>", prefix, prefix, prefix);
	for (i=1; i<poly->nrings; i++)
	{
		ptr += sprintf(ptr, "<%sinnerBoundaryIs><%sLinearRing><%scoordinates>", prefix, prefix, prefix);
		ptr += pointArray_toGML2(poly->rings[i], ptr, precision);
		ptr += sprintf(ptr, "</%scoordinates></%sLinearRing></%sinnerBoundaryIs>", prefix, prefix, prefix);
	}
	ptr += sprintf(ptr, "</%sPolygon>", prefix);

	return (ptr-output);
}

static char *
asgml2_poly(LWPOLY *poly, char *srs, int precision, const char *prefix)
{
	char *output;
	int size;

	size = asgml2_poly_size(poly, srs, precision, prefix);
	output = lwalloc(size);
	asgml2_poly_buf(poly, srs, output, precision, prefix);
	return output;
}

/*
 * Compute max size required for GML version of this
 * inspected geometry. Will recurse when needed.
 * Don't call this with single-geoms inspected.
 */
static size_t
asgml2_multi_size(LWGEOM_INSPECTED *insp, char *srs, int precision,
		const char *prefix)
{
	int i;
	size_t size;
	size_t prefixlen = strlen(prefix);

	/* the longest possible multi version */
	size = sizeof("<MultiLineString></MultiLineString>");
	size += 2*prefixlen;

	if ( srs ) size += strlen(srs) + sizeof(" srsName=..");

	for (i=0; i<insp->ngeometries; i++)
	{
		LWPOINT *point;
		LWLINE *line;
		LWPOLY *poly;

		if ((point=lwgeom_getpoint_inspected(insp, i)))
		{
			size += ( sizeof("<pointMember>/") + prefixlen ) * 2;
			size += asgml2_point_size(point, 0, precision, prefix);
			lwpoint_release(point);
		}
		else if ((line=lwgeom_getline_inspected(insp, i)))
		{
			size += ( sizeof("<lineStringMember>/") + prefixlen ) * 2;
			size += asgml2_line_size(line, 0, precision, prefix);
			lwline_release(line);
		}
		else if ((poly=lwgeom_getpoly_inspected(insp, i)))
		{
			size += ( sizeof("<polygonMember>/") + prefixlen ) * 2;
			size += asgml2_poly_size(poly, 0, precision, prefix);
			lwpoly_release(poly);
		}
	}

	return size;
}

/*
 * Don't call this with single-geoms inspected!
 */
static size_t
asgml2_multi_buf(LWGEOM_INSPECTED *insp, char *srs, char *output,
	int precision, const char *prefix)
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
		ptr += sprintf(ptr, "<%s%s srsName=\"%s\">", prefix, gmltype, srs);
	}
	else
	{
		ptr += sprintf(ptr, "<%s%s>", prefix, gmltype);
	}

	for (i=0; i<insp->ngeometries; i++)
	{
		LWPOINT *point;
		LWLINE *line;
		LWPOLY *poly;

		if ((point=lwgeom_getpoint_inspected(insp, i)))
		{
			ptr += sprintf(ptr, "<%spointMember>", prefix);
			ptr += asgml2_point_buf(point, 0, ptr, precision, prefix);
			lwpoint_release(point);
			ptr += sprintf(ptr, "</%spointMember>", prefix);
		}
		else if ((line=lwgeom_getline_inspected(insp, i)))
		{
			ptr += sprintf(ptr, "<%slineStringMember>", prefix);
			ptr += asgml2_line_buf(line, 0, ptr, precision, prefix);
			lwline_release(line);
			ptr += sprintf(ptr, "</%slineStringMember>", prefix);
		}
		else if ((poly=lwgeom_getpoly_inspected(insp, i)))
		{
			ptr += sprintf(ptr, "<%spolygonMember>", prefix);
			ptr += asgml2_poly_buf(poly, 0, ptr, precision, prefix);
			lwpoly_release(poly);
			ptr += sprintf(ptr, "</%spolygonMember>", prefix);
		}
	}

	/* Close outmost tag */
	ptr += sprintf(ptr, "</%s%s>", prefix, gmltype);

	return (ptr-output);
}

/*
 * Don't call this with single-geoms inspected!
 */
static char *
asgml2_multi(LWGEOM_INSPECTED *insp, char *srs, int precision,
		const char *prefix)
{
	char *gml;
	size_t size;

	size = asgml2_multi_size(insp, srs, precision, prefix);
	gml = lwalloc(size);
	asgml2_multi_buf(insp, srs, gml, precision, prefix);
	return gml;
}


static size_t
asgml2_collection_size(LWGEOM_INSPECTED *insp, char *srs, int precision,
		const char *prefix)
{
	int i;
	size_t size;
	size_t prefixlen = strlen(prefix);

	size = sizeof("<MultiGeometry></MultiGeometry>");
	size += (prefixlen * 2);

	if ( srs ) size += strlen(srs) + sizeof(" srsName=..");

	for (i=0; i<insp->ngeometries; i++)
	{
		LWPOINT *point;
		LWLINE *line;
		LWPOLY *poly;
		LWGEOM_INSPECTED *subinsp;
		uchar *subgeom;

		size += ( sizeof("<geometryMember>/") + prefixlen ) * 2;
		if ((point=lwgeom_getpoint_inspected(insp, i)))
		{
			size += asgml2_point_size(point, 0, precision, prefix);
			lwpoint_release(point);
		}
		else if ((line=lwgeom_getline_inspected(insp, i)))
		{
			size += asgml2_line_size(line, 0, precision, prefix);
			lwline_release(line);
		}
		else if ((poly=lwgeom_getpoly_inspected(insp, i)))
		{
			size += asgml2_poly_size(poly, 0, precision, prefix);
			lwpoly_release(poly);
		}
		else
		{
			subgeom = lwgeom_getsubgeometry_inspected(insp, i);
			subinsp = lwgeom_inspect(subgeom);
			size += asgml2_collection_size(subinsp, 0, precision, prefix);
			lwinspected_release(subinsp);
		}
	}

	return size;
}

/*
 * Don't call this with single-geoms inspected!
 */
static size_t
asgml2_collection_buf(LWGEOM_INSPECTED *insp, char *srs, char *output, int precision, const char *prefix)
{
	char *ptr;
	int i;

	ptr = output;

	/* Open outmost tag */
	if ( srs )
	{
		ptr += sprintf(ptr, "<%sMultiGeometry srsName=\"%s\">", prefix, srs);
	}
	else
	{
		ptr += sprintf(ptr, "<%sMultiGeometry>", prefix);
	}

	for (i=0; i<insp->ngeometries; i++)
	{
		LWPOINT *point;
		LWLINE *line;
		LWPOLY *poly;
		LWGEOM_INSPECTED *subinsp;
		uchar *subgeom;

		ptr += sprintf(ptr, "<%sgeometryMember>", prefix);
		if ((point=lwgeom_getpoint_inspected(insp, i)))
		{
			ptr += asgml2_point_buf(point, 0, ptr, precision, prefix);
			lwpoint_release(point);
		}
		else if ((line=lwgeom_getline_inspected(insp, i)))
		{
			ptr += asgml2_line_buf(line, 0, ptr, precision, prefix);
			lwline_release(line);
		}
		else if ((poly=lwgeom_getpoly_inspected(insp, i)))
		{
			ptr += asgml2_poly_buf(poly, 0, ptr, precision, prefix);
			lwpoly_release(poly);
		}
		else
		{
			subgeom = lwgeom_getsubgeometry_inspected(insp, i);
			subinsp = lwgeom_inspect(subgeom);
			if (lwgeom_getType(subgeom[0]) == COLLECTIONTYPE)
				ptr += asgml2_collection_buf(subinsp, 0, ptr, precision, prefix);
			else
				ptr += asgml2_multi_buf(subinsp, 0, ptr, precision, prefix);
			lwinspected_release(subinsp);
		}
		ptr += sprintf(ptr, "</%sgeometryMember>", prefix);
	}

	/* Close outmost tag */
	ptr += sprintf(ptr, "</%sMultiGeometry>", prefix);

	return (ptr-output);
}

/*
 * Don't call this with single-geoms inspected!
 */
static char *
asgml2_collection(LWGEOM_INSPECTED *insp, char *srs, int precision,
		const char *prefix)
{
	char *gml;
	size_t size;

	size = asgml2_collection_size(insp, srs, precision, prefix);
	gml = lwalloc(size);
	asgml2_collection_buf(insp, srs, gml, precision, prefix);
	return gml;
}


static size_t
pointArray_toGML2(POINTARRAY *pa, char *output, int precision)
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
 * VERSION GML 3.1.1
 */


/* takes a GEOMETRY and returns a GML representation */
extern char *
lwgeom_to_gml3(uchar *geom, char *srs, int precision, int is_deegree,
		const char *prefix)
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
		return asgml3_point(point, srs, precision, is_deegree, prefix);

	case LINETYPE:
		line = lwline_deserialize(geom);
		return asgml3_line(line, srs, precision, is_deegree, prefix);

	case POLYGONTYPE:
		poly = lwpoly_deserialize(geom);
		return asgml3_poly(poly, srs, precision, is_deegree, prefix);

	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
		inspected = lwgeom_inspect(geom);
		return asgml3_multi(inspected, srs, precision, is_deegree, prefix);

	case COLLECTIONTYPE:
		inspected = lwgeom_inspect(geom);
		return asgml3_collection(inspected, srs, precision, is_deegree, prefix);

	default:
		lwerror("lwgeom_to_gml3: '%s' geometry type not supported", lwtype_name(type));
		return NULL;
	}
}

static size_t
asgml3_point_size(LWPOINT *point, char *srs, int precision,
		const char *prefix)
{
	int size;
	size_t prefixlen = strlen(prefix);

	size = pointArray_GMLsize(point->point, precision);
	size += ( sizeof("<point><pos srsDimension='x'>/") + (prefixlen*2) ) * 2;
	if ( srs ) size += strlen(srs) + sizeof(" srsName=..");
	return size;
}

static size_t
asgml3_point_buf(LWPOINT *point, char *srs, char *output, int precision, int is_deegree, const char *prefix)
{
	char *ptr = output;
	int dimension=2;

	if (TYPE_HASZ(point->type)) dimension = 3;
	if ( srs )
	{
		ptr += sprintf(ptr, "<%sPoint srsName=\"%s\">", prefix, srs);
	}
	else
	{
		ptr += sprintf(ptr, "<%sPoint>", prefix);
	}
	ptr += sprintf(ptr, "<%spos srsDimension=\"%d\">", prefix, dimension);
	ptr += pointArray_toGML3(point->point, ptr, precision, is_deegree);
	ptr += sprintf(ptr, "</%spos></%sPoint>", prefix, prefix);

	return (ptr-output);
}

static char *
asgml3_point(LWPOINT *point, char *srs, int precision, int is_deegree, const char *prefix)
{
	char *output;
	int size;

	size = asgml3_point_size(point, srs, precision, prefix);
	output = lwalloc(size);
	asgml3_point_buf(point, srs, output, precision, is_deegree, prefix);
	return output;
}


static size_t
asgml3_line_size(LWLINE *line, char *srs, int precision, const char *prefix)
{
	int size;
	size_t prefixlen = strlen(prefix);

	size = pointArray_GMLsize(line->points, precision);
	size += ( sizeof("<Curve><segments><LineStringSegment><posList>/") + ( prefixlen * 4 ) ) * 2;
	size += sizeof(" srsDimension='x'");
	if ( srs ) size += strlen(srs) + sizeof(" srsName=..");
	return size;
}

static size_t
asgml3_line_buf(LWLINE *line, char *srs, char *output, int precision, int is_deegree, const char *prefix)
{
	char *ptr=output;
	int dimension=2;

	if (TYPE_HASZ(line->type)) dimension = 3;
	if ( srs )
	{
		ptr += sprintf(ptr, "<%sCurve srsName=\"%s\">", prefix, srs);
	}
	else
	{
		ptr += sprintf(ptr, "<%sCurve>", prefix);
	}
	ptr += sprintf(ptr, "<%ssegments>", prefix);
	ptr += sprintf(ptr, "<%sLineStringSegment>", prefix);
	ptr += sprintf(ptr, "<%sposList srsDimension=\"%d\">", prefix, dimension);
	ptr += pointArray_toGML3(line->points, ptr, precision, is_deegree);
	ptr += sprintf(ptr, "</%sposList></%sLineStringSegment>", prefix, prefix);
	ptr += sprintf(ptr, "</%ssegments>", prefix);
	ptr += sprintf(ptr, "</%sCurve>", prefix);

	return (ptr-output);
}

static char *
asgml3_line(LWLINE *line, char *srs, int precision, int is_deegree, const char *prefix)
{
	char *output;
	int size;

	size = asgml3_line_size(line, srs, precision, prefix);
	output = lwalloc(size);
	asgml3_line_buf(line, srs, output, precision, is_deegree, prefix);
	return output;
}


static size_t
asgml3_poly_size(LWPOLY *poly, char *srs, int precision, const char *prefix)
{
	size_t size;
	size_t prefixlen = strlen(prefix);
	int i;

	size = ( sizeof("<Polygon><exterior><LinearRing>///") + (prefixlen*3) ) * 2;
	size += ( sizeof("<interior><LinearRing>//") + (prefixlen*2) ) * 2 * (poly->nrings - 1);
	size += ( sizeof("<posList srsDimension='x'></posList>") + (prefixlen*2) ) * poly->nrings;
	if ( srs ) size += strlen(srs) + sizeof(" srsName=..");

	for (i=0; i<poly->nrings; i++)
		size += pointArray_GMLsize(poly->rings[i], precision);

	return size;
}

static size_t
asgml3_poly_buf(LWPOLY *poly, char *srs, char *output, int precision, int is_deegree, const char *prefix)
{
	int i;
	char *ptr=output;
	int dimension=2;

	if (TYPE_HASZ(poly->type)) dimension = 3;
	if ( srs )
	{
		ptr += sprintf(ptr, "<%sPolygon srsName=\"%s\">", prefix, srs);
	}
	else
	{
		ptr += sprintf(ptr, "<%sPolygon>", prefix);
	}
	ptr += sprintf(ptr, "<%sexterior><%sLinearRing>", prefix, prefix);
	ptr += sprintf(ptr, "<%sposList srsDimension=\"%d\">", prefix, dimension);
	ptr += pointArray_toGML3(poly->rings[0], ptr, precision, is_deegree);
	ptr += sprintf(ptr, "</%sposList></%sLinearRing></%sexterior>",
		prefix, prefix, prefix);
	for (i=1; i<poly->nrings; i++)
	{
		ptr += sprintf(ptr, "<%sinterior><%sLinearRing>", prefix, prefix);
		ptr += sprintf(ptr, "<%sposList srsDimension=\"%d\">", prefix, dimension);
		ptr += pointArray_toGML3(poly->rings[i], ptr, precision, is_deegree);
		ptr += sprintf(ptr, "</%sposList></%sLinearRing></%sinterior>",
			prefix, prefix, prefix);
	}
	ptr += sprintf(ptr, "</%sPolygon>", prefix);

	return (ptr-output);
}

static char *
asgml3_poly(LWPOLY *poly, char *srs, int precision, int is_deegree, const char *prefix)
{
	char *output;
	int size;

	size = asgml3_poly_size(poly, srs, precision, prefix);
	output = lwalloc(size);
	asgml3_poly_buf(poly, srs, output, precision, is_deegree, prefix);
	return output;
}


/*
 * Compute max size required for GML version of this
 * inspected geometry. Will recurse when needed.
 * Don't call this with single-geoms inspected.
 */
static size_t
asgml3_multi_size(LWGEOM_INSPECTED *insp, char *srs, int precision, const char *prefix)
{
	int i;
	size_t size;
	size_t prefixlen = strlen(prefix);

	/* the longest possible multi version */
	size = sizeof("<MultiLineString></MultiLineString>") + prefixlen*2;

	if ( srs ) size += strlen(srs) + sizeof(" srsName=..");

	for (i=0; i<insp->ngeometries; i++)
	{
		LWPOINT *point;
		LWLINE *line;
		LWPOLY *poly;

		if ((point=lwgeom_getpoint_inspected(insp, i)))
		{
			size += ( sizeof("<pointMember>/") + prefixlen ) * 2;
			size += asgml3_point_size(point, 0, precision, prefix);
			lwpoint_release(point);
		}
		else if ((line=lwgeom_getline_inspected(insp, i)))
		{
			size += ( sizeof("<curveMember>/") + prefixlen ) * 2;
			size += asgml3_line_size(line, 0, precision, prefix);
			lwline_release(line);
		}
		else if ((poly=lwgeom_getpoly_inspected(insp, i)))
		{
			size += ( sizeof("<surfaceMember>/") + prefixlen ) * 2;
			size += asgml3_poly_size(poly, 0, precision, prefix);
			lwpoly_release(poly);
		}
	}

	return size;
}

/*
 * Don't call this with single-geoms inspected!
 */
static size_t
asgml3_multi_buf(LWGEOM_INSPECTED *insp, char *srs, char *output, int precision, int is_deegree, const char *prefix)
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
		ptr += sprintf(ptr, "<%s%s srsName=\"%s\">", prefix, gmltype, srs);
	}
	else
	{
		ptr += sprintf(ptr, "<%s%s>", prefix, gmltype);
	}

	for (i=0; i<insp->ngeometries; i++)
	{
		LWPOINT *point;
		LWLINE *line;
		LWPOLY *poly;

		if ((point=lwgeom_getpoint_inspected(insp, i)))
		{
			ptr += sprintf(ptr, "<%spointMember>", prefix);
			ptr += asgml3_point_buf(point, 0, ptr, precision, is_deegree, prefix);
			lwpoint_release(point);
			ptr += sprintf(ptr, "</%spointMember>", prefix);
		}
		else if ((line=lwgeom_getline_inspected(insp, i)))
		{
			ptr += sprintf(ptr, "<%scurveMember>", prefix);
			ptr += asgml3_line_buf(line, 0, ptr, precision, is_deegree, prefix);
			lwline_release(line);
			ptr += sprintf(ptr, "</%scurveMember>", prefix);
		}
		else if ((poly=lwgeom_getpoly_inspected(insp, i)))
		{
			ptr += sprintf(ptr, "<%ssurfaceMember>", prefix);
			ptr += asgml3_poly_buf(poly, 0, ptr, precision, is_deegree, prefix);
			lwpoly_release(poly);
			ptr += sprintf(ptr, "</%ssurfaceMember>", prefix);
		}
	}

	/* Close outmost tag */
	ptr += sprintf(ptr, "</%s%s>", prefix, gmltype);

	return (ptr-output);
}

/*
 * Don't call this with single-geoms inspected!
 */
static char *
asgml3_multi(LWGEOM_INSPECTED *insp, char *srs, int precision, int is_deegree, const char *prefix)
{
	char *gml;
	size_t size;

	size = asgml3_multi_size(insp, srs, precision, prefix);
	gml = lwalloc(size);
	asgml3_multi_buf(insp, srs, gml, precision, is_deegree, prefix);
	return gml;
}


static size_t
asgml3_collection_size(LWGEOM_INSPECTED *insp, char *srs, int precision, const char *prefix)
{
	int i;
	size_t size;
	size_t prefixlen = strlen(prefix);

	size = sizeof("<MultiGeometry></MultiGeometry>") + prefixlen*2;

	if ( srs ) size += strlen(srs) + sizeof(" srsName=..");

	for (i=0; i<insp->ngeometries; i++)
	{
		LWPOINT *point;
		LWLINE *line;
		LWPOLY *poly;
		LWGEOM_INSPECTED *subinsp;
		uchar *subgeom;

		size += ( sizeof("<geometryMember>/") + prefixlen ) * 2;
		if ((point=lwgeom_getpoint_inspected(insp, i)))
		{
			size += asgml3_point_size(point, 0, precision, prefix);
			lwpoint_release(point);
		}
		else if ((line=lwgeom_getline_inspected(insp, i)))
		{
			size += asgml3_line_size(line, 0, precision, prefix);
			lwline_release(line);
		}
		else if ((poly=lwgeom_getpoly_inspected(insp, i)))
		{
			size += asgml3_poly_size(poly, 0, precision, prefix);
			lwpoly_release(poly);
		}
		else
		{
			subgeom = lwgeom_getsubgeometry_inspected(insp, i);
			subinsp = lwgeom_inspect(subgeom);
			size += asgml3_multi_size(subinsp, 0, precision, prefix);
			lwinspected_release(subinsp);
		}
	}

	return size;
}

static size_t
asgml3_collection_buf(LWGEOM_INSPECTED *insp, char *srs, char *output, int precision, int is_deegree, const char *prefix)
{
	char *ptr;
	int i;

	ptr = output;

	/* Open outmost tag */
	if ( srs )
	{
		ptr += sprintf(ptr, "<%sMultiGeometry srsName=\"%s\">", prefix, srs);
	}
	else
	{
		ptr += sprintf(ptr, "<%sMultiGeometry>", prefix);
	}

	for (i=0; i<insp->ngeometries; i++)
	{
		LWPOINT *point;
		LWLINE *line;
		LWPOLY *poly;
		LWGEOM_INSPECTED *subinsp;
		uchar *subgeom;

		ptr += sprintf(ptr, "<%sgeometryMember>", prefix);
		if ((point=lwgeom_getpoint_inspected(insp, i)))
		{
			ptr += asgml3_point_buf(point, 0, ptr, precision, is_deegree, prefix);
			lwpoint_release(point);
		}
		else if ((line=lwgeom_getline_inspected(insp, i)))
		{
			ptr += asgml3_line_buf(line, 0, ptr, precision, is_deegree, prefix);
			lwline_release(line);
		}
		else if ((poly=lwgeom_getpoly_inspected(insp, i)))
		{
			ptr += asgml3_poly_buf(poly, 0, ptr, precision, is_deegree, prefix);
			lwpoly_release(poly);
		}
		else
		{
			subgeom = lwgeom_getsubgeometry_inspected(insp, i);
			subinsp = lwgeom_inspect(subgeom);
			if (lwgeom_getType(subgeom[0]) == COLLECTIONTYPE)
				ptr += asgml3_collection_buf(subinsp, 0, ptr, precision, is_deegree, prefix);
			else
				ptr += asgml3_multi_buf(subinsp, 0, ptr, precision, is_deegree, prefix);
			lwinspected_release(subinsp);
		}
		ptr += sprintf(ptr, "</%sgeometryMember>", prefix);
	}

	/* Close outmost tag */
	ptr += sprintf(ptr, "</%sMultiGeometry>", prefix);

	return (ptr-output);
}

/*
 * Don't call this with single-geoms inspected!
 */
static char *
asgml3_collection(LWGEOM_INSPECTED *insp, char *srs, int precision, int is_deegree, const char *prefix)
{
	char *gml;
	size_t size;

	size = asgml3_collection_size(insp, srs, precision, prefix);
	gml = lwalloc(size);
	asgml3_collection_buf(insp, srs, gml, precision, is_deegree, prefix);
	return gml;
}


/* In GML3, inside <posList> or <pos>, coordinates are separated by a space separator
 * In GML3 also, lat/lon are reversed for geocentric data
 */
static size_t
pointArray_toGML3(POINTARRAY *pa, char *output, int precision, int is_deegree)
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
		return (OUT_MAX_DIGS_DOUBLE + precision + sizeof(", "))
		       * 2 * pa->npoints;

	return (OUT_MAX_DIGS_DOUBLE + precision + sizeof(", ")) * 3 * pa->npoints;
}
