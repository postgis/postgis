/**********************************************************************
 * $Id: lwout_gml.c 9772 2012-05-21 21:17:59Z colivier $
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 *
 * Copyright 2011 Sandro Santilli <strk@keybit.net>
 * Copyright 2010-2012 Oslandia
 * Copyright 2001-2003 Refractions Research Inc.
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
#include "liblwgeom_internal.h"


static size_t asgml2_point_size(const LWPOINT *point, const char *srs, int precision, const char *prefix);
static char *asgml2_point(const LWPOINT *point, const char *srs, int precision, const char *prefix);
static size_t asgml2_line_size(const LWLINE *line, const char *srs, int precision, const char *prefix);
static char *asgml2_line(const LWLINE *line, const char *srs, int precision, const char *prefix);
static size_t asgml2_poly_size(const LWPOLY *poly, const char *srs, int precision, const char *prefix);
static char *asgml2_poly(const LWPOLY *poly, const char *srs, int precision, const char *prefix);
static size_t asgml2_multi_size(const LWCOLLECTION *col, const char *srs, int precision, const char *prefix);
static char *asgml2_multi(const LWCOLLECTION *col, const char *srs, int precision, const char *prefix);
static size_t asgml2_collection_size(const LWCOLLECTION *col, const char *srs, int precision, const char *prefix);
static char *asgml2_collection(const LWCOLLECTION *col, const char *srs, int precision, const char *prefix);
static size_t pointArray_toGML2(POINTARRAY *pa, char *buf, int precision);

static size_t asgml3_point_size(const LWPOINT *point, const char *srs, int precision, int opts, const char *prefix, const char *id);
static char *asgml3_point(const LWPOINT *point, const char *srs, int precision, int opts, const char *prefix, const char *id);
static size_t asgml3_line_size(const LWLINE *line, const char *srs, int precision, int opts, const char *prefix, const char *id);
static char *asgml3_line(const LWLINE *line, const char *srs, int precision, int opts, const char *prefix, const char *id);
static size_t asgml3_poly_size(const LWPOLY *poly, const char *srs, int precision, int opts, const char *prefix, const char *id);
static char *asgml3_poly(const LWPOLY *poly, const char *srs, int precision, int opts, int is_patch, const char *prefix, const char *id);
static size_t asgml3_triangle_size(const LWTRIANGLE *triangle, const char *srs, int precision, int opts, const char *prefix, const char *id);
static char *asgml3_triangle(const LWTRIANGLE *triangle, const char *srs, int precision, int opts, const char *prefix, const char *id);
static size_t asgml3_multi_size(const LWCOLLECTION *col, const char *srs, int precision, int opts, const char *prefix, const char *id);
static char *asgml3_multi(const LWCOLLECTION *col, const char *srs, int precision, int opts, const char *prefix, const char *id);
static char *asgml3_psurface(const LWPSURFACE *psur, const char *srs, int precision, int opts, const char *prefix, const char *id);
static char *asgml3_tin(const LWTIN *tin, const char *srs, int precision, int opts, const char *prefix, const char *id);
static size_t asgml3_collection_size(const LWCOLLECTION *col, const char *srs, int precision, int opts, const char *prefix, const char *id);
static char *asgml3_collection(const LWCOLLECTION *col, const char *srs, int precision, int opts, const char *prefix, const char *id);
static size_t pointArray_toGML3(POINTARRAY *pa, char *buf, int precision, int opts);

static size_t pointArray_GMLsize(POINTARRAY *pa, int precision);

static char *
gbox_to_gml2(const GBOX *bbox, const char *srs, int precision, const char *prefix)
{
	int size;
        POINT4D pt;
        POINTARRAY *pa;
	char *ptr, *output;
	size_t prefixlen = strlen(prefix);

	if ( ! bbox ) {
		size = ( sizeof("<Box>/") + (prefixlen*2) ) * 2;
		if ( srs ) size += strlen(srs) + sizeof(" srsName=..");

		ptr = output = lwalloc(size);

		ptr += sprintf(ptr, "<%sBox", prefix);

		if ( srs ) ptr += sprintf(ptr, " srsName=\"%s\"", srs);

		ptr += sprintf(ptr, "/>");

		return output;
	}

        pa = ptarray_construct_empty(FLAGS_GET_Z(bbox->flags), 0, 2);

        pt.x = bbox->xmin; 
        pt.y = bbox->ymin; 
        if (FLAGS_GET_Z(bbox->flags)) pt.z = bbox->zmin; 
        ptarray_append_point(pa, &pt, LW_TRUE);
    
        pt.x = bbox->xmax; 
        pt.y = bbox->ymax; 
        if (FLAGS_GET_Z(bbox->flags)) pt.z = bbox->zmax; 
        ptarray_append_point(pa, &pt, LW_TRUE);

	size = pointArray_GMLsize(pa, precision);
	size += ( sizeof("<Box><coordinates>/") + (prefixlen*2) ) * 2;
	if ( srs ) size += strlen(srs) + sizeof(" srsName=..");

	ptr = output = lwalloc(size);

	if ( srs ) ptr += sprintf(ptr, "<%sBox srsName=\"%s\">", prefix, srs);
	else       ptr += sprintf(ptr, "<%sBox>", prefix);

	ptr += sprintf(ptr, "<%scoordinates>", prefix);
	ptr += pointArray_toGML2(pa, ptr, precision);
	ptr += sprintf(ptr, "</%scoordinates></%sBox>", prefix, prefix);

        ptarray_free(pa);

	return output;
}

static char *
gbox_to_gml3(const GBOX *bbox, const char *srs, int precision, int opts, const char *prefix)
{
	int size;
        POINT4D pt;
        POINTARRAY *pa;
	char *ptr, *output;
	size_t prefixlen = strlen(prefix);
	int dimension = 2;

	if ( ! bbox ) {
		size = ( sizeof("<Envelope>/") + (prefixlen*2) ) * 2;
		if ( srs ) size += strlen(srs) + sizeof(" srsName=..");

		ptr = output = lwalloc(size);

		ptr += sprintf(ptr, "<%sEnvelope", prefix);
		if ( srs ) ptr += sprintf(ptr, " srsName=\"%s\"", srs);

		ptr += sprintf(ptr, "/>");

		return output;
	}

        if (FLAGS_GET_Z(bbox->flags)) dimension = 3;

        pa = ptarray_construct_empty(FLAGS_GET_Z(bbox->flags), 0, 1);

        pt.x = bbox->xmin;
        pt.y = bbox->ymin; 
        if (FLAGS_GET_Z(bbox->flags)) pt.z = bbox->zmin; 
        ptarray_append_point(pa, &pt, LW_TRUE);

	size = pointArray_GMLsize(pa, precision) * 2;
	size += ( sizeof("<Envelope><lowerCorner><upperCorner>//") + (prefixlen*3) ) * 2;
	if ( srs ) size += strlen(srs) + sizeof(" srsName=..");
	if ( IS_DIMS(opts) ) size += sizeof(" srsDimension=. .");

	ptr = output = lwalloc(size);

	ptr += sprintf(ptr, "<%sEnvelope", prefix);
	if ( srs ) ptr += sprintf(ptr, " srsName=\"%s\"", srs);
	if ( IS_DIMS(opts) ) ptr += sprintf(ptr, " srsDimension=\"%d\"", dimension);
	ptr += sprintf(ptr, ">");

	ptr += sprintf(ptr, "<%slowerCorner>", prefix);
	ptr += pointArray_toGML3(pa, ptr, precision, opts);
	ptr += sprintf(ptr, "</%slowerCorner>", prefix);

        ptarray_remove_point(pa, 0);
        pt.x = bbox->xmax;
        pt.y = bbox->ymax; 
        if (FLAGS_GET_Z(bbox->flags)) pt.z = bbox->zmax; 
        ptarray_append_point(pa, &pt, LW_TRUE);

	ptr += sprintf(ptr, "<%supperCorner>", prefix);
	ptr += pointArray_toGML3(pa, ptr, precision, opts);
	ptr += sprintf(ptr, "</%supperCorner>", prefix);

	ptr += sprintf(ptr, "</%sEnvelope>", prefix);

        ptarray_free(pa);

	return output;
}


extern char *
lwgeom_extent_to_gml2(const LWGEOM *geom, const char *srs, int precision, const char *prefix)
{
	const GBOX* bbox = lwgeom_get_bbox(geom);
/*
	if ( ! bbox ) {
		lwerror("lwgeom_extent_to_gml2: empty geometry doesn't have a bounding box");
		return NULL;
	}
*/
	char *ret = gbox_to_gml2(bbox, srs, precision, prefix);
	return ret;
}
	

extern char *
lwgeom_extent_to_gml3(const LWGEOM *geom, const char *srs, int precision, int opts, const char *prefix)
{
	const GBOX* bbox = lwgeom_get_bbox(geom);
/*
	if ( ! bbox ) {
		lwerror("lwgeom_extent_to_gml3: empty geometry doesn't have a bounding box");
		return NULL;
	}
*/
	return gbox_to_gml3(bbox, srs, precision, opts, prefix);
}
	
	
/**
 *  @brief VERSION GML 2
 *  	takes a GEOMETRY and returns a GML2 representation
 */
extern char *
lwgeom_to_gml2(const LWGEOM *geom, const char *srs, int precision, const char* prefix)
{
	int type = geom->type;

	/* Return null for empty (#1377) */
	if ( lwgeom_is_empty(geom) )
		return NULL;

	switch (type)
	{
	case POINTTYPE:
		return asgml2_point((LWPOINT*)geom, srs, precision, prefix);

	case LINETYPE:
		return asgml2_line((LWLINE*)geom, srs, precision, prefix);

	case POLYGONTYPE:
		return asgml2_poly((LWPOLY*)geom, srs, precision, prefix);

	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
		return asgml2_multi((LWCOLLECTION*)geom, srs, precision, prefix);

	case COLLECTIONTYPE:
		return asgml2_collection((LWCOLLECTION*)geom, srs, precision, prefix);

	case TRIANGLETYPE:
	case POLYHEDRALSURFACETYPE:
	case TINTYPE:
		lwerror("Cannot convert %s to GML2. Try ST_AsGML(3, <geometry>) to generate GML3.", lwtype_name(type));
		return NULL;
		
	default:
		lwerror("lwgeom_to_gml2: '%s' geometry type not supported", lwtype_name(type));
		return NULL;
	}
}

static size_t
asgml2_point_size(const LWPOINT *point, const char *srs, int precision, const char* prefix)
{
	int size;
	size_t prefixlen = strlen(prefix);

	size = pointArray_GMLsize(point->point, precision);
	size += ( sizeof("<point><coordinates>/") + (prefixlen*2) ) * 2;
	if ( srs ) size += strlen(srs) + sizeof(" srsName=..");
	return size;
}

static size_t
asgml2_point_buf(const LWPOINT *point, const char *srs, char *output, int precision, const char* prefix)
{
	char *ptr = output;

	ptr += sprintf(ptr, "<%sPoint", prefix);
	if ( srs ) ptr += sprintf(ptr, " srsName=\"%s\"", srs);
	if ( lwpoint_is_empty(point) ) {
		ptr += sprintf(ptr, "/>");
		return (ptr-output);
	}
	ptr += sprintf(ptr, ">");
	ptr += sprintf(ptr, "<%scoordinates>", prefix);
	ptr += pointArray_toGML2(point->point, ptr, precision);
	ptr += sprintf(ptr, "</%scoordinates></%sPoint>", prefix, prefix);

	return (ptr-output);
}

static char *
asgml2_point(const LWPOINT *point, const char *srs, int precision, const char *prefix)
{
	char *output;
	int size;

	size = asgml2_point_size(point, srs, precision, prefix);
	output = lwalloc(size);
	asgml2_point_buf(point, srs, output, precision, prefix);
	return output;
}

static size_t
asgml2_line_size(const LWLINE *line, const char *srs, int precision, const char *prefix)
{
	int size;
	size_t prefixlen = strlen(prefix);

	size = pointArray_GMLsize(line->points, precision);
	size += ( sizeof("<linestring><coordinates>/") + (prefixlen*2) ) * 2;
	if ( srs ) size += strlen(srs) + sizeof(" srsName=..");
	return size;
}

static size_t
asgml2_line_buf(const LWLINE *line, const char *srs, char *output, int precision,
                const char *prefix)
{
	char *ptr=output;

	ptr += sprintf(ptr, "<%sLineString", prefix);
	if ( srs ) ptr += sprintf(ptr, " srsName=\"%s\"", srs);

	if ( lwline_is_empty(line) ) {
		ptr += sprintf(ptr, "/>");
		return (ptr-output);
	}
	ptr += sprintf(ptr, ">");

	ptr += sprintf(ptr, "<%scoordinates>", prefix);
	ptr += pointArray_toGML2(line->points, ptr, precision);
	ptr += sprintf(ptr, "</%scoordinates></%sLineString>", prefix, prefix);

	return (ptr-output);
}

static char *
asgml2_line(const LWLINE *line, const char *srs, int precision, const char *prefix)
{
	char *output;
	int size;

	size = asgml2_line_size(line, srs, precision, prefix);
	output = lwalloc(size);
	asgml2_line_buf(line, srs, output, precision, prefix);
	return output;
}

static size_t
asgml2_poly_size(const LWPOLY *poly, const char *srs, int precision, const char *prefix)
{
	size_t size;
	int i;
	size_t prefixlen = strlen(prefix);

	size = sizeof("<polygon></polygon>") + prefixlen*2;
	if ( srs ) size += strlen(srs) + sizeof(" srsName=..");
	if ( lwpoly_is_empty(poly) ) 
		return size;
	size += ( sizeof("<outerboundaryis><linearring><coordinates>/") + ( prefixlen*3) ) * 2;
	size += ( sizeof("<innerboundaryis><linearring><coordinates>/") + ( prefixlen*2) ) * 2 * poly->nrings;

	for (i=0; i<poly->nrings; i++)
		size += pointArray_GMLsize(poly->rings[i], precision);

	return size;
}

static size_t
asgml2_poly_buf(const LWPOLY *poly, const char *srs, char *output, int precision,
                const char *prefix)
{
	int i;
	char *ptr=output;

	ptr += sprintf(ptr, "<%sPolygon", prefix);
	if ( srs ) ptr += sprintf(ptr, " srsName=\"%s\"", srs);
	if ( lwpoly_is_empty(poly) ) {
		ptr += sprintf(ptr, "/>");
		return (ptr-output);
	}
	ptr += sprintf(ptr, ">");
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
asgml2_poly(const LWPOLY *poly, const char *srs, int precision, const char *prefix)
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
asgml2_multi_size(const LWCOLLECTION *col, const char *srs, int precision,
                  const char *prefix)
{
	int i;
	size_t size;
	size_t prefixlen = strlen(prefix);
	LWGEOM *subgeom;

	/* the longest possible multi version */
	size = sizeof("<MultiLineString></MultiLineString>");
	size += 2*prefixlen;

	if ( srs ) size += strlen(srs) + sizeof(" srsName=..");

	for (i=0; i<col->ngeoms; i++)
	{
		subgeom = col->geoms[i];
		if (subgeom->type == POINTTYPE)
		{
			size += ( sizeof("<pointMember>/") + prefixlen ) * 2;
			size += asgml2_point_size((LWPOINT*)subgeom, 0, precision, prefix);
		}
		else if (subgeom->type == LINETYPE)
		{
			size += ( sizeof("<lineStringMember>/") + prefixlen ) * 2;
			size += asgml2_line_size((LWLINE*)subgeom, 0, precision, prefix);
		}
		else if (subgeom->type == POLYGONTYPE)
		{
			size += ( sizeof("<polygonMember>/") + prefixlen ) * 2;
			size += asgml2_poly_size((LWPOLY*)subgeom, 0, precision, prefix);
		}
	}

	return size;
}

/*
 * Don't call this with single-geoms inspected!
 */
static size_t
asgml2_multi_buf(const LWCOLLECTION *col, const char *srs, char *output,
                 int precision, const char *prefix)
{
	int type = col->type;
	char *ptr, *gmltype;
	int i;
	LWGEOM *subgeom;

	ptr = output;
	gmltype="";

	if 	(type == MULTIPOINTTYPE)   gmltype = "MultiPoint";
	else if (type == MULTILINETYPE)	   gmltype = "MultiLineString";
	else if (type == MULTIPOLYGONTYPE) gmltype = "MultiPolygon";

	/* Open outmost tag */
	ptr += sprintf(ptr, "<%s%s", prefix, gmltype);
	if ( srs ) ptr += sprintf(ptr, " srsName=\"%s\"", srs);

	if (!col->ngeoms) {
		ptr += sprintf(ptr, "/>");
		return (ptr-output);
	}
	ptr += sprintf(ptr, ">");

	for (i=0; i<col->ngeoms; i++)
	{
		subgeom = col->geoms[i];
		if (subgeom->type == POINTTYPE)
		{
			ptr += sprintf(ptr, "<%spointMember>", prefix);
			ptr += asgml2_point_buf((LWPOINT*)subgeom, 0, ptr, precision, prefix);
			ptr += sprintf(ptr, "</%spointMember>", prefix);
		}
		else if (subgeom->type == LINETYPE)
		{
			ptr += sprintf(ptr, "<%slineStringMember>", prefix);
			ptr += asgml2_line_buf((LWLINE*)subgeom, 0, ptr, precision, prefix);
			ptr += sprintf(ptr, "</%slineStringMember>", prefix);
		}
		else if (subgeom->type == POLYGONTYPE)
		{
			ptr += sprintf(ptr, "<%spolygonMember>", prefix);
			ptr += asgml2_poly_buf((LWPOLY*)subgeom, 0, ptr, precision, prefix);
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
asgml2_multi(const LWCOLLECTION *col, const char *srs, int precision,
             const char *prefix)
{
	char *gml;
	size_t size;

	size = asgml2_multi_size(col, srs, precision, prefix);
	gml = lwalloc(size);
	asgml2_multi_buf(col, srs, gml, precision, prefix);
	return gml;
}


/*
 * Don't call this with single-geoms!
 */
static size_t
asgml2_collection_size(const LWCOLLECTION *col, const char *srs, int precision,
                       const char *prefix)
{
	int i;
	size_t size;
	size_t prefixlen = strlen(prefix);
	LWGEOM *subgeom;

	size = sizeof("<MultiGeometry></MultiGeometry>");
	size += (prefixlen * 2);

	if ( srs ) size += strlen(srs) + sizeof(" srsName=..");

	for (i=0; i<col->ngeoms; i++)
	{
		subgeom = col->geoms[i];
		
		size += ( sizeof("<geometryMember>/") + prefixlen ) * 2;
		if ( subgeom->type == POINTTYPE)
		{
			size += asgml2_point_size((LWPOINT*)subgeom, 0, precision, prefix);
		}
		else if ( subgeom->type == LINETYPE)
		{
			size += asgml2_line_size((LWLINE*)subgeom, 0, precision, prefix);
		}
		else if ( subgeom->type == POLYGONTYPE)
		{
			size += asgml2_poly_size((LWPOLY*)subgeom, 0, precision, prefix);
		}
		else if ( lwgeom_is_collection(subgeom) )
		{
			size += asgml2_collection_size((LWCOLLECTION*)subgeom, 0, precision, prefix);
		}
		else
			lwerror("asgml2_collection_size: Unable to process geometry type!");
	}


	return size;
}

/*
 * Don't call this with single-geoms inspected!
 */
static size_t
asgml2_collection_buf(const LWCOLLECTION *col, const char *srs, char *output, int precision, const char *prefix)
{
	char *ptr;
	int i;
	LWGEOM *subgeom;
	
	ptr = output;

	/* Open outmost tag */
	ptr += sprintf(ptr, "<%sMultiGeometry", prefix);
	if ( srs ) ptr += sprintf(ptr, " srsName=\"%s\"", srs);

	if (!col->ngeoms) {
		ptr += sprintf(ptr, "/>");
		return (ptr-output);
	}
	ptr += sprintf(ptr, ">");

	for (i=0; i<col->ngeoms; i++)
	{
 		subgeom = col->geoms[i];

		ptr += sprintf(ptr, "<%sgeometryMember>", prefix);
		if (subgeom->type == POINTTYPE)
		{
			ptr += asgml2_point_buf((LWPOINT*)subgeom, 0, ptr, precision, prefix);
		}
		else if (subgeom->type == LINETYPE)
		{
			ptr += asgml2_line_buf((LWLINE*)subgeom, 0, ptr, precision, prefix);
		}
		else if (subgeom->type == POLYGONTYPE)
		{
			ptr += asgml2_poly_buf((LWPOLY*)subgeom, 0, ptr, precision, prefix);
		}
		else if (lwgeom_is_collection(subgeom))
		{
			if (subgeom->type == COLLECTIONTYPE)
				ptr += asgml2_collection_buf((LWCOLLECTION*)subgeom, 0, ptr, precision, prefix);
			else
				ptr += asgml2_multi_buf((LWCOLLECTION*)subgeom, 0, ptr, precision, prefix);
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
asgml2_collection(const LWCOLLECTION *col, const char *srs, int precision,
                  const char *prefix)
{
	char *gml;
	size_t size;

	size = asgml2_collection_size(col, srs, precision, prefix);
	gml = lwalloc(size);
	asgml2_collection_buf(col, srs, gml, precision, prefix);
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

	if ( ! FLAGS_GET_Z(pa->flags) )
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
lwgeom_to_gml3(const LWGEOM *geom, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
	int type = geom->type;

	/* Return null for empty (#1377) */
	if ( lwgeom_is_empty(geom) )
		return NULL;

	switch (type)
	{
	case POINTTYPE:
		return asgml3_point((LWPOINT*)geom, srs, precision, opts, prefix, id);

	case LINETYPE:
		return asgml3_line((LWLINE*)geom, srs, precision, opts, prefix, id);

	case POLYGONTYPE:
		return asgml3_poly((LWPOLY*)geom, srs, precision, opts, 0, prefix, id);

	case TRIANGLETYPE:
		return asgml3_triangle((LWTRIANGLE*)geom, srs, precision, opts, prefix, id);

	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
		return asgml3_multi((LWCOLLECTION*)geom, srs, precision, opts, prefix, id);

	case POLYHEDRALSURFACETYPE:
		return asgml3_psurface((LWPSURFACE*)geom, srs, precision, opts, prefix, id);

	case TINTYPE:
		return asgml3_tin((LWTIN*)geom, srs, precision, opts, prefix, id);

	case COLLECTIONTYPE:
		return asgml3_collection((LWCOLLECTION*)geom, srs, precision, opts, prefix, id);

	default:
		lwerror("lwgeom_to_gml3: '%s' geometry type not supported", lwtype_name(type));
		return NULL;
	}
}

static size_t
asgml3_point_size(const LWPOINT *point, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
	int size;
	size_t prefixlen = strlen(prefix);

	size = pointArray_GMLsize(point->point, precision);
	size += ( sizeof("<point><pos>/") + (prefixlen*2) ) * 2;
	if (srs) size += strlen(srs) + sizeof(" srsName=..");
	if (id)  size += strlen(id) + strlen(prefix) + sizeof(" id=..");
	if (IS_DIMS(opts)) size += sizeof(" srsDimension='x'");
	return size;
}

static size_t
asgml3_point_buf(const LWPOINT *point, const char *srs, char *output, int precision, int opts, const char *prefix, const char *id)
{
	char *ptr = output;
	int dimension=2;

	if (FLAGS_GET_Z(point->flags)) dimension = 3;

	ptr += sprintf(ptr, "<%sPoint", prefix);
	if ( srs ) ptr += sprintf(ptr, " srsName=\"%s\"", srs);
	if ( id )  ptr += sprintf(ptr, " %sid=\"%s\"", prefix, id);
	if ( lwpoint_is_empty(point) ) {
		ptr += sprintf(ptr, "/>");
		return (ptr-output);
	}

	ptr += sprintf(ptr, ">");
	if (IS_DIMS(opts)) ptr += sprintf(ptr, "<%spos srsDimension=\"%d\">", prefix, dimension);
	else         ptr += sprintf(ptr, "<%spos>", prefix);
	ptr += pointArray_toGML3(point->point, ptr, precision, opts);
	ptr += sprintf(ptr, "</%spos></%sPoint>", prefix, prefix);

	return (ptr-output);
}

static char *
asgml3_point(const LWPOINT *point, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
	char *output;
	int size;

	size = asgml3_point_size(point, srs, precision, opts, prefix, id);
	output = lwalloc(size);
	asgml3_point_buf(point, srs, output, precision, opts, prefix, id);
	return output;
}


static size_t
asgml3_line_size(const LWLINE *line, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
	int size;
	size_t prefixlen = strlen(prefix);

	size = pointArray_GMLsize(line->points, precision);
	if ( opts & LW_GML_SHORTLINE )
	{
		size += (
		  sizeof("<LineString><posList>/") +
		  ( prefixlen * 2 )
		) * 2;
	}
	else
	{
		size += (
		  sizeof("<Curve><segments><LineStringSegment><posList>/") +
		  ( prefixlen * 4 )
		) * 2;
	}
	if (srs) size += strlen(srs) + sizeof(" srsName=..");
	if (id)  size += strlen(id) + strlen(prefix) + sizeof(" id=..");
	if (IS_DIMS(opts)) size += sizeof(" srsDimension='x'");
	return size;
}

static size_t
asgml3_line_buf(const LWLINE *line, const char *srs, char *output, int precision, int opts, const char *prefix, const char *id)
{
	char *ptr=output;
	int dimension=2;
	int shortline = ( opts & LW_GML_SHORTLINE );

	if (FLAGS_GET_Z(line->flags)) dimension = 3;

	if ( shortline ) {
		ptr += sprintf(ptr, "<%sLineString", prefix);
	} else {
		ptr += sprintf(ptr, "<%sCurve", prefix);
	}

	if (srs) ptr += sprintf(ptr, " srsName=\"%s\"", srs);
	if (id)  ptr += sprintf(ptr, " %sid=\"%s\"", prefix, id);

	if ( lwline_is_empty(line) ) {
		ptr += sprintf(ptr, "/>");
		return (ptr-output);
	}
	ptr += sprintf(ptr, ">");

	if ( ! shortline ) {
		ptr += sprintf(ptr, "<%ssegments>", prefix);
		ptr += sprintf(ptr, "<%sLineStringSegment>", prefix);
	}

	if (IS_DIMS(opts)) {
		ptr += sprintf(ptr, "<%sposList srsDimension=\"%d\">",
			prefix, dimension);
	} else {
		ptr += sprintf(ptr, "<%sposList>", prefix);
	}

	ptr += pointArray_toGML3(line->points, ptr, precision, opts);

	ptr += sprintf(ptr, "</%sposList>", prefix);

	if ( shortline ) {
		ptr += sprintf(ptr, "</%sLineString>", prefix);
	} else {
		ptr += sprintf(ptr, "</%sLineStringSegment>", prefix);
		ptr += sprintf(ptr, "</%ssegments>", prefix);
		ptr += sprintf(ptr, "</%sCurve>", prefix);
	}

	return (ptr-output);
}

static char *
asgml3_line(const LWLINE *line, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
	char *output;
	int size;

	size = asgml3_line_size(line, srs, precision, opts, prefix, id);
	output = lwalloc(size);
	asgml3_line_buf(line, srs, output, precision, opts, prefix, id);
	return output;
}


static size_t
asgml3_poly_size(const LWPOLY *poly, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
	size_t size;
	size_t prefixlen = strlen(prefix);
	int i;

	size = ( sizeof("<PolygonPatch><exterior><LinearRing>///") + (prefixlen*3) ) * 2;
	size += ( sizeof("<interior><LinearRing>//") + (prefixlen*2) ) * 2 * (poly->nrings - 1);
	size += ( sizeof("<posList></posList>") + (prefixlen*2) ) * poly->nrings;
	if (srs) size += strlen(srs) + sizeof(" srsName=..");
	if (id)  size += strlen(id) + strlen(prefix) + sizeof(" id=..");
	if (IS_DIMS(opts)) size += sizeof(" srsDimension='x'") * poly->nrings;

	for (i=0; i<poly->nrings; i++)
		size += pointArray_GMLsize(poly->rings[i], precision);

	return size;
}

static size_t
asgml3_poly_buf(const LWPOLY *poly, const char *srs, char *output, int precision, int opts, int is_patch, const char *prefix, const char *id)
{
	int i;
	char *ptr=output;
	int dimension=2;

	if (FLAGS_GET_Z(poly->flags)) dimension = 3;
	if (is_patch)
	{
		ptr += sprintf(ptr, "<%sPolygonPatch", prefix);

	}
	else
	{
		ptr += sprintf(ptr, "<%sPolygon", prefix);
	}

	if (srs) ptr += sprintf(ptr, " srsName=\"%s\"", srs);
	if (id)  ptr += sprintf(ptr, " %sid=\"%s\"", prefix, id);

	if ( lwpoly_is_empty(poly) ) {
		ptr += sprintf(ptr, "/>");
		return (ptr-output);
	}
	ptr += sprintf(ptr, ">");

	ptr += sprintf(ptr, "<%sexterior><%sLinearRing>", prefix, prefix);
	if (IS_DIMS(opts)) ptr += sprintf(ptr, "<%sposList srsDimension=\"%d\">", prefix, dimension);
	else         ptr += sprintf(ptr, "<%sposList>", prefix);

	ptr += pointArray_toGML3(poly->rings[0], ptr, precision, opts);
	ptr += sprintf(ptr, "</%sposList></%sLinearRing></%sexterior>",
	               prefix, prefix, prefix);
	for (i=1; i<poly->nrings; i++)
	{
		ptr += sprintf(ptr, "<%sinterior><%sLinearRing>", prefix, prefix);
		if (IS_DIMS(opts)) ptr += sprintf(ptr, "<%sposList srsDimension=\"%d\">", prefix, dimension);
		else         ptr += sprintf(ptr, "<%sposList>", prefix);
		ptr += pointArray_toGML3(poly->rings[i], ptr, precision, opts);
		ptr += sprintf(ptr, "</%sposList></%sLinearRing></%sinterior>",
		               prefix, prefix, prefix);
	}
	if (is_patch) ptr += sprintf(ptr, "</%sPolygonPatch>", prefix);
	else ptr += sprintf(ptr, "</%sPolygon>", prefix);

	return (ptr-output);
}

static char *
asgml3_poly(const LWPOLY *poly, const char *srs, int precision, int opts, int is_patch, const char *prefix, const char *id)
{
	char *output;
	int size;

	size = asgml3_poly_size(poly, srs, precision, opts, prefix, id);
	output = lwalloc(size);
	asgml3_poly_buf(poly, srs, output, precision, opts, is_patch, prefix, id);
	return output;
}


static size_t
asgml3_triangle_size(const LWTRIANGLE *triangle, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
	size_t size;
	size_t prefixlen = strlen(prefix);

	size =  ( sizeof("<Triangle><exterior><LinearRing>///") + (prefixlen*3) ) * 2;
	size +=   sizeof("<posList></posList>") + (prefixlen*2);
	if (srs) size += strlen(srs) + sizeof(" srsName=..");
	if (id)  size += strlen(prefix) + strlen(id) + sizeof(" id=..");
	if (IS_DIMS(opts)) size += sizeof(" srsDimension='x'");

	size += pointArray_GMLsize(triangle->points, precision);

	return size;
}

static size_t
asgml3_triangle_buf(const LWTRIANGLE *triangle, const char *srs, char *output, int precision, int opts, const char *prefix, const char *id)
{
	char *ptr=output;
	int dimension=2;

	if (FLAGS_GET_Z(triangle->flags)) dimension = 3;
	ptr += sprintf(ptr, "<%sTriangle", prefix);
	if (srs) ptr += sprintf(ptr, " srsName=\"%s\"", srs);
	if (id)  ptr += sprintf(ptr, " %sid=\"%s\"", prefix, id);
	ptr += sprintf(ptr, ">");

	ptr += sprintf(ptr, "<%sexterior><%sLinearRing>", prefix, prefix);
	if (IS_DIMS(opts)) ptr += sprintf(ptr, "<%sposList srsDimension=\"%d\">", prefix, dimension);
	else         ptr += sprintf(ptr, "<%sposList>", prefix);

	ptr += pointArray_toGML3(triangle->points, ptr, precision, opts);
	ptr += sprintf(ptr, "</%sposList></%sLinearRing></%sexterior>",
	               prefix, prefix, prefix);

	ptr += sprintf(ptr, "</%sTriangle>", prefix);

	return (ptr-output);
}

static char *
asgml3_triangle(const LWTRIANGLE *triangle, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
	char *output;
	int size;

	size = asgml3_triangle_size(triangle, srs, precision, opts, prefix, id);
	output = lwalloc(size);
	asgml3_triangle_buf(triangle, srs, output, precision, opts, prefix, id);
	return output;
}


/*
 * Compute max size required for GML version of this
 * inspected geometry. Will recurse when needed.
 * Don't call this with single-geoms inspected.
 */
static size_t
asgml3_multi_size(const LWCOLLECTION *col, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
	int i;
	size_t size;
	size_t prefixlen = strlen(prefix);
	LWGEOM *subgeom;

	/* the longest possible multi version */
	size = sizeof("<MultiLineString></MultiLineString>") + prefixlen*2;

	if (srs) size += strlen(srs) + sizeof(" srsName=..");
	if (id)  size += strlen(id) + strlen(prefix) + sizeof(" id=..");

	for (i=0; i<col->ngeoms; i++)
	{
		subgeom = col->geoms[i];
		if (subgeom->type == POINTTYPE)
		{
			size += ( sizeof("<pointMember>/") + prefixlen ) * 2;
			size += asgml3_point_size((LWPOINT*)subgeom, 0, precision, opts, prefix, id);
		}
		else if (subgeom->type == LINETYPE)
		{
			size += ( sizeof("<curveMember>/") + prefixlen ) * 2;
			size += asgml3_line_size((LWLINE*)subgeom, 0, precision, opts, prefix, id);
		}
		else if (subgeom->type == POLYGONTYPE)
		{
			size += ( sizeof("<surfaceMember>/") + prefixlen ) * 2;
			size += asgml3_poly_size((LWPOLY*)subgeom, 0, precision, opts, prefix, id);
		}
	}

	return size;
}

/*
 * Don't call this with single-geoms inspected!
 */
static size_t
asgml3_multi_buf(const LWCOLLECTION *col, const char *srs, char *output, int precision, int opts, const char *prefix, const char *id)
{
	int type = col->type;
	char *ptr, *gmltype;
	int i;
	LWGEOM *subgeom;

	ptr = output;
	gmltype="";

	if 	(type == MULTIPOINTTYPE)   gmltype = "MultiPoint";
	else if (type == MULTILINETYPE)    gmltype = "MultiCurve";
	else if (type == MULTIPOLYGONTYPE) gmltype = "MultiSurface";

	/* Open outmost tag */
	ptr += sprintf(ptr, "<%s%s", prefix, gmltype);
	if (srs) ptr += sprintf(ptr, " srsName=\"%s\"", srs);
	if (id)  ptr += sprintf(ptr, " %sid=\"%s\"", prefix, id);

	if (!col->ngeoms) {
		ptr += sprintf(ptr, "/>");
		return (ptr-output);
	}
	ptr += sprintf(ptr, ">");

	for (i=0; i<col->ngeoms; i++)
	{
		subgeom = col->geoms[i];
		if (subgeom->type == POINTTYPE)
		{
			ptr += sprintf(ptr, "<%spointMember>", prefix);
			ptr += asgml3_point_buf((LWPOINT*)subgeom, 0, ptr, precision, opts, prefix, id);
			ptr += sprintf(ptr, "</%spointMember>", prefix);
		}
		else if (subgeom->type == LINETYPE)
		{
			ptr += sprintf(ptr, "<%scurveMember>", prefix);
			ptr += asgml3_line_buf((LWLINE*)subgeom, 0, ptr, precision, opts, prefix, id);
			ptr += sprintf(ptr, "</%scurveMember>", prefix);
		}
		else if (subgeom->type == POLYGONTYPE)
		{
			ptr += sprintf(ptr, "<%ssurfaceMember>", prefix);
			ptr += asgml3_poly_buf((LWPOLY*)subgeom, 0, ptr, precision, opts, 0, prefix, id);
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
asgml3_multi(const LWCOLLECTION *col, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
	char *gml;
	size_t size;

	size = asgml3_multi_size(col, srs, precision, opts, prefix, id);
	gml = lwalloc(size);
	asgml3_multi_buf(col, srs, gml, precision, opts, prefix, id);
	return gml;
}


static size_t
asgml3_psurface_size(const LWPSURFACE *psur, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
	int i;
	size_t size;
	size_t prefixlen = strlen(prefix);

	size = (sizeof("<PolyhedralSurface><polygonPatches>/") + prefixlen*2) * 2;
	if (srs) size += strlen(srs) + sizeof(" srsName=..");
	if (id)  size += strlen(id) + strlen(prefix) + sizeof(" id=..");

	for (i=0; i<psur->ngeoms; i++)
	{
		size += asgml3_poly_size(psur->geoms[i], 0, precision, opts, prefix, id);
	}

	return size;
}


/*
 * Don't call this with single-geoms inspected!
 */
static size_t
asgml3_psurface_buf(const LWPSURFACE *psur, const char *srs, char *output, int precision, int opts, const char *prefix, const char *id)
{
	char *ptr;
	int i;

	ptr = output;

	/* Open outmost tag */
	ptr += sprintf(ptr, "<%sPolyhedralSurface", prefix);
	if (srs) ptr += sprintf(ptr, " srsName=\"%s\"", srs);
	if (id)  ptr += sprintf(ptr, " %sid=\"%s\"", prefix, id);
	ptr += sprintf(ptr, "><%spolygonPatches>", prefix);

	for (i=0; i<psur->ngeoms; i++)
	{
		ptr += asgml3_poly_buf(psur->geoms[i], 0, ptr, precision, opts, 1, prefix, id);
	}

	/* Close outmost tag */
	ptr += sprintf(ptr, "</%spolygonPatches></%sPolyhedralSurface>",
	               prefix, prefix);

	return (ptr-output);
}

/*
 * Don't call this with single-geoms inspected!
 */
static char *
asgml3_psurface(const LWPSURFACE *psur, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
	char *gml;
	size_t size;

	size = asgml3_psurface_size(psur, srs, precision, opts, prefix, id);
	gml = lwalloc(size);
	asgml3_psurface_buf(psur, srs, gml, precision, opts, prefix, id);
	return gml;
}


static size_t
asgml3_tin_size(const LWTIN *tin, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
	int i;
	size_t size;
	size_t prefixlen = strlen(prefix);

	size = (sizeof("<Tin><trianglePatches>/") + prefixlen*2) * 2;
	if (srs) size += strlen(srs) + sizeof(" srsName=..");
	if (id)  size += strlen(id) + strlen(prefix) + sizeof(" id=..");

	for (i=0; i<tin->ngeoms; i++)
	{
		size += asgml3_triangle_size(tin->geoms[i], 0, precision, opts, prefix, id);
	}

	return size;
}


/*
 * Don't call this with single-geoms inspected!
 */
static size_t
asgml3_tin_buf(const LWTIN *tin, const char *srs, char *output, int precision, int opts, const char *prefix, const char *id)
{
	char *ptr;
	int i;

	ptr = output;

	/* Open outmost tag */
	ptr += sprintf(ptr, "<%sTin", prefix);
	if (srs) ptr += sprintf(ptr, " srsName=\"%s\"", srs);
	if (id)  ptr += sprintf(ptr, " %sid=\"%s\"", prefix, id);
	else	 ptr += sprintf(ptr, "><%strianglePatches>", prefix);

	for (i=0; i<tin->ngeoms; i++)
	{
		ptr += asgml3_triangle_buf(tin->geoms[i], 0, ptr, precision,
		                           opts, prefix, id);
	}

	/* Close outmost tag */
	ptr += sprintf(ptr, "</%strianglePatches></%sTin>", prefix, prefix);

	return (ptr-output);
}

/*
 * Don't call this with single-geoms inspected!
 */
static char *
asgml3_tin(const LWTIN *tin, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
	char *gml;
	size_t size;

	size = asgml3_tin_size(tin, srs, precision, opts, prefix, id);
	gml = lwalloc(size);
	asgml3_tin_buf(tin, srs, gml, precision, opts, prefix, id);
	return gml;
}

static size_t
asgml3_collection_size(const LWCOLLECTION *col, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
	int i;
	size_t size;
	size_t prefixlen = strlen(prefix);
	LWGEOM *subgeom;

	size = sizeof("<MultiGeometry></MultiGeometry>") + prefixlen*2;

	if (srs) size += strlen(srs) + sizeof(" srsName=..");
	if (id)  size += strlen(id) + strlen(prefix) + sizeof(" id=..");

	for (i=0; i<col->ngeoms; i++)
	{
		subgeom = col->geoms[i];
		size += ( sizeof("<geometryMember>/") + prefixlen ) * 2;
		if ( subgeom->type == POINTTYPE )
		{
			size += asgml3_point_size((LWPOINT*)subgeom, 0, precision, opts, prefix, id);
		}
		else if ( subgeom->type == LINETYPE )
		{
			size += asgml3_line_size((LWLINE*)subgeom, 0, precision, opts, prefix, id);
		}
		else if ( subgeom->type == POLYGONTYPE )
		{
			size += asgml3_poly_size((LWPOLY*)subgeom, 0, precision, opts, prefix, id);
		}
		else if ( lwgeom_is_collection(subgeom) )
		{
			size += asgml3_multi_size((LWCOLLECTION*)subgeom, 0, precision, opts, prefix, id);
		}
		else
			lwerror("asgml3_collection_size: unknown geometry type");
	}

	return size;
}

static size_t
asgml3_collection_buf(const LWCOLLECTION *col, const char *srs, char *output, int precision, int opts, const char *prefix, const char *id)
{
	char *ptr;
	int i;
	LWGEOM *subgeom;

	ptr = output;

	/* Open outmost tag */
	ptr += sprintf(ptr, "<%sMultiGeometry", prefix);
	if (srs) ptr += sprintf(ptr, " srsName=\"%s\"", srs);
	if (id)  ptr += sprintf(ptr, " %sid=\"%s\"", prefix, id);

	if (!col->ngeoms) {
		ptr += sprintf(ptr, "/>");
		return (ptr-output);
	}
	ptr += sprintf(ptr, ">");

	for (i=0; i<col->ngeoms; i++)
	{
		subgeom = col->geoms[i];
		ptr += sprintf(ptr, "<%sgeometryMember>", prefix);
		if ( subgeom->type == POINTTYPE )
		{
			ptr += asgml3_point_buf((LWPOINT*)subgeom, 0, ptr, precision, opts, prefix, id);
		}
		else if ( subgeom->type == LINETYPE )
		{
			ptr += asgml3_line_buf((LWLINE*)subgeom, 0, ptr, precision, opts, prefix, id);
		}
		else if ( subgeom->type == POLYGONTYPE )
		{
			ptr += asgml3_poly_buf((LWPOLY*)subgeom, 0, ptr, precision, opts, 0, prefix, id);
		}
		else if ( lwgeom_is_collection(subgeom) )
		{
			if ( subgeom->type == COLLECTIONTYPE )
				ptr += asgml3_collection_buf((LWCOLLECTION*)subgeom, 0, ptr, precision, opts, prefix, id);
			else
				ptr += asgml3_multi_buf((LWCOLLECTION*)subgeom, 0, ptr, precision, opts, prefix, id);
		}
		else 
			lwerror("asgml3_collection_buf: unknown geometry type");
			
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
asgml3_collection(const LWCOLLECTION *col, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
	char *gml;
	size_t size;

	size = asgml3_collection_size(col, srs, precision, opts, prefix, id);
	gml = lwalloc(size);
	asgml3_collection_buf(col, srs, gml, precision, opts, prefix, id);
	return gml;
}


/* In GML3, inside <posList> or <pos>, coordinates are separated by a space separator
 * In GML3 also, lat/lon are reversed for geocentric data
 */
static size_t
pointArray_toGML3(POINTARRAY *pa, char *output, int precision, int opts)
{
	int i;
	char *ptr;
	char x[OUT_MAX_DIGS_DOUBLE+OUT_MAX_DOUBLE_PRECISION+1];
	char y[OUT_MAX_DIGS_DOUBLE+OUT_MAX_DOUBLE_PRECISION+1];
	char z[OUT_MAX_DIGS_DOUBLE+OUT_MAX_DOUBLE_PRECISION+1];

	ptr = output;

	if ( ! FLAGS_GET_Z(pa->flags) )
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
			if (IS_DEGREE(opts))
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
			if (IS_DEGREE(opts))
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
	if (FLAGS_NDIMS(pa->flags) == 2)
		return (OUT_MAX_DIGS_DOUBLE + precision + sizeof(", ")) * 2 * pa->npoints;

	return (OUT_MAX_DIGS_DOUBLE + precision + sizeof(", ")) * 3 * pa->npoints;
}
