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
 * GML output routines.
 *
 **********************************************************************/


#include "postgres.h"
#include "lwgeom.h"

Datum LWGEOM_asGML(PG_FUNCTION_ARGS);
char *geometry_to_gml(LWGEOM *geometry, int version);
static char *asgml_point(LWPOINT *point, int version);
static char *asgml_line(LWLINE *line, int version);
static char *asgml_poly(LWPOLY *poly, int version);
static size_t pointArray_GMLsize(POINTARRAY *pa, int version);
static size_t pointArray_toGML(POINTARRAY *pa, int version, char *buf);

#define SHOW_DIGS_DOUBLE 15
#define MAX_DIGS_DOUBLE (SHOW_DIGS_DOUBLE + 6 + 1 + 3 +1)

/**
 * Encode feature in GML 
 */
PG_FUNCTION_INFO_V1(LWGEOM_asGML);
Datum LWGEOM_asGML(PG_FUNCTION_ARGS)
{
	LWGEOM *geom;
	char *gml;
	char *result;
	int len;
	int version=0;

	if ( PG_ARGISNULL(0) ) PG_RETURN_NULL();

	geom = (LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	// check for relative path notation
	if ( PG_NARGS() > 1 && ! PG_ARGISNULL(1) )
			version = PG_GETARG_INT32(1);

	gml = geometry_to_gml(geom, version);

	len = strlen(gml) + 5;

	result= palloc(len);
	*((int *) result) = len;

	memcpy(result +4, gml, len-4);

	pfree(gml);
	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_CSTRING(result);
}


// takes a GEOMETRY and returns a GML representation
char *geometry_to_gml(LWGEOM *geometry, int version)
{
	char *result = NULL;
	LWGEOM_INSPECTED *inspected;
	int i;

	if (lwgeom_getType(geometry->type) >= 4 )
	{
		return pstrdup("<!-- MULTI geoms not yet supported -->");
	}

	inspected = lwgeom_inspect(SERIALIZED_FORM(geometry));
	for(i=0; i<inspected->ngeometries; i++)
	{
		char *subgeom = lwgeom_getsubgeometry_inspected(inspected, i);
		LWPOINT *point;
		LWLINE *line;
		LWPOLY *poly;

		if (lwgeom_getType(subgeom[0]) == POINTTYPE)
		{
			point = lwpoint_deserialize(subgeom);
			return asgml_point(point, version);
		}
		if (lwgeom_getType(subgeom[0]) == LINETYPE)
		{
			line = lwline_deserialize(subgeom);
			return asgml_line(line, version);
		}
		if (lwgeom_getType(subgeom[0]) == POLYGONTYPE)
		{
			poly = lwpoly_deserialize(subgeom);
			return asgml_poly(poly, version);
		}
	}
	return(result);
}

static char *
asgml_point(LWPOINT *point, int version)
{
	char *output;
	int size;
	char *ptr;
	
	size = pointArray_GMLsize(point->point, version);
	size += sizeof("<point><coordinates>/") * 2;

	output = palloc(size);
	ptr = output;

	ptr += sprintf(ptr, "<Point><coordinates>");
	ptr += pointArray_toGML(point->point, version, ptr);
	ptr += sprintf(ptr, "</coordinates></Point>");

	return output;
}

static char *
asgml_line(LWLINE *line, int version)
{
	char *output;
	int size;
	char *ptr;
	
	size = pointArray_GMLsize(line->points, version);
	size += sizeof("<linestring><coordinates>/") * 2;

	output = palloc(size);
	ptr = output;

	ptr += sprintf(ptr, "<LineString><coordinates>");
	ptr += pointArray_toGML(line->points, version, ptr);
	ptr += sprintf(ptr, "</coordinates></LineString>");

	return output;
}

static char *
asgml_poly(LWPOLY *poly, int version)
{
	char *output;
	int size;
	int i;
	char *ptr;

	size = sizeof("<polygon></outerboundaryis></linearring>") * 2;
	size += sizeof("<innerboundaryis><linearring>/") * 2 * poly->nrings;

	for (i=0; i<poly->nrings; i++)
		size += pointArray_GMLsize(poly->rings[i], version);
	
	output = palloc(size);
	ptr = output;

 	ptr += sprintf(ptr, "<Polygon>");
 	ptr += sprintf(ptr, "<OuterBoundaryIs>");
	ptr += pointArray_toGML(poly->rings[0], version, ptr);
 	ptr += sprintf(ptr, "</OuterBoundaryIs>");
	for (i=1; i<poly->nrings; i++)
	{
 		ptr += sprintf(ptr, "<InnerBoundaryIs>");
		ptr += pointArray_toGML(poly->rings[i], version, ptr);
 		ptr += sprintf(ptr, "</InnerBoundaryIs>");
	}
	ptr += sprintf(ptr, "</Polygon>");

	return output;
}

static size_t
pointArray_GMLsize(POINTARRAY *pa, int version)
{
	return pa->ndims * (MAX_DIGS_DOUBLE+(pa->ndims-1));
}

static size_t
pointArray_toGML(POINTARRAY *pa, int version, char *output)
{
	int i;
	POINT4D *pt;
	char *ptr;

	ptr = output;

	if ( pa->ndims == 2 )
	{
		for (i=0; i<pa->npoints; i++)
		{
			pt = (POINT4D *)getPoint(pa, i);
			if ( i ) ptr += sprintf(ptr, " ");
			ptr += sprintf(ptr, "%g,%g",
				pt->x, pt->y);
		}
	}
	else if ( pa->ndims == 3 )
	{
		for (i=0; i<pa->npoints; i++)
		{
			pt = (POINT4D *)getPoint(pa, i);
			if ( i ) ptr += sprintf(ptr, " ");
			ptr += sprintf(ptr, "%g,%g,%g",
				pt->x, pt->y, pt->z);
		}
	}
	else if ( pa->ndims == 4 )
	{
		for (i=0; i<pa->npoints; i++)
		{
			pt = (POINT4D *)getPoint(pa, i);
			if ( i ) ptr += sprintf(ptr, " ");
			ptr += sprintf(ptr, "%g,%g,%g,%g", 
				pt->x, pt->y, pt->z, pt->m);
		}
	}

	return ptr-output;
}


/**********************************************************************
 * $Log$
 * Revision 1.1  2004/09/23 11:12:47  strk
 * Initial GML output routines.
 *
 **********************************************************************/

