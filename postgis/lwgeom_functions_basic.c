/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2006 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "postgres.h"
#include "fmgr.h"
#include "utils/elog.h"
#include "utils/array.h"
#include "utils/geo_decls.h"

#include "liblwgeom.h"
#include "lwalgorithm.h"
#include "lwgeom_pg.h"
#include "profile.h"

#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

Datum LWGEOM_mem_size(PG_FUNCTION_ARGS);
Datum LWGEOM_summary(PG_FUNCTION_ARGS);
Datum LWGEOM_npoints(PG_FUNCTION_ARGS);
Datum LWGEOM_nrings(PG_FUNCTION_ARGS);
Datum LWGEOM_area_polygon(PG_FUNCTION_ARGS);
Datum LWGEOM_dwithin(PG_FUNCTION_ARGS);
Datum postgis_uses_stats(PG_FUNCTION_ARGS);
Datum postgis_autocache_bbox(PG_FUNCTION_ARGS);
Datum postgis_scripts_released(PG_FUNCTION_ARGS);
Datum postgis_version(PG_FUNCTION_ARGS);
Datum postgis_lib_version(PG_FUNCTION_ARGS);
Datum postgis_lib_build_date(PG_FUNCTION_ARGS);
Datum LWGEOM_length2d_linestring(PG_FUNCTION_ARGS);
Datum LWGEOM_length_linestring(PG_FUNCTION_ARGS);
Datum LWGEOM_perimeter2d_poly(PG_FUNCTION_ARGS);
Datum LWGEOM_perimeter_poly(PG_FUNCTION_ARGS);
Datum LWGEOM_mindistance2d(PG_FUNCTION_ARGS);
Datum LWGEOM_maxdistance2d_linestring(PG_FUNCTION_ARGS);
Datum LWGEOM_inside_circle_point(PG_FUNCTION_ARGS);
Datum LWGEOM_collect(PG_FUNCTION_ARGS);
Datum LWGEOM_accum(PG_FUNCTION_ARGS);
Datum LWGEOM_collect_garray(PG_FUNCTION_ARGS);
Datum LWGEOM_expand(PG_FUNCTION_ARGS);
Datum LWGEOM_to_BOX(PG_FUNCTION_ARGS);
Datum LWGEOM_envelope(PG_FUNCTION_ARGS);
Datum LWGEOM_isempty(PG_FUNCTION_ARGS);
Datum LWGEOM_segmentize2d(PG_FUNCTION_ARGS);
Datum LWGEOM_reverse(PG_FUNCTION_ARGS);
Datum LWGEOM_forceRHR_poly(PG_FUNCTION_ARGS);
Datum LWGEOM_noop(PG_FUNCTION_ARGS);
Datum LWGEOM_zmflag(PG_FUNCTION_ARGS);
Datum LWGEOM_hasz(PG_FUNCTION_ARGS);
Datum LWGEOM_hasm(PG_FUNCTION_ARGS);
Datum LWGEOM_ndims(PG_FUNCTION_ARGS);
Datum LWGEOM_makepoint(PG_FUNCTION_ARGS);
Datum LWGEOM_makepoint3dm(PG_FUNCTION_ARGS);
Datum LWGEOM_makeline_garray(PG_FUNCTION_ARGS);
Datum LWGEOM_makeline(PG_FUNCTION_ARGS);
Datum LWGEOM_makepoly(PG_FUNCTION_ARGS);
Datum LWGEOM_line_from_mpoint(PG_FUNCTION_ARGS);
Datum LWGEOM_addpoint(PG_FUNCTION_ARGS);
Datum LWGEOM_removepoint(PG_FUNCTION_ARGS);
Datum LWGEOM_setpoint_linestring(PG_FUNCTION_ARGS);
Datum LWGEOM_asEWKT(PG_FUNCTION_ARGS);
Datum LWGEOM_hasBBOX(PG_FUNCTION_ARGS);
Datum LWGEOM_azimuth(PG_FUNCTION_ARGS);
Datum LWGEOM_affine(PG_FUNCTION_ARGS);
Datum LWGEOM_longitude_shift(PG_FUNCTION_ARGS);
Datum optimistic_overlap(PG_FUNCTION_ARGS);
Datum ST_GeoHash(PG_FUNCTION_ARGS);

void lwgeom_affine_ptarray(POINTARRAY *pa, double afac, double bfac, double cfac,
                           double dfac, double efac, double ffac, double gfac, double hfac, double ifac, double xoff, double yoff, double zoff);

void lwgeom_affine_recursive(uchar *serialized, double afac, double bfac, double cfac,
                             double dfac, double efac, double ffac, double gfac, double hfac, double ifac, double xoff, double yoff, double zoff);

/*------------------------------------------------------------------*/

/** find the size of geometry */
PG_FUNCTION_INFO_V1(LWGEOM_mem_size);
Datum LWGEOM_mem_size(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	size_t size = VARSIZE(geom);
	size_t computed_size = lwgeom_size(SERIALIZED_FORM(geom));
	computed_size += VARHDRSZ; /* varlena size */
	if ( size != computed_size )
	{
		elog(NOTICE, "varlena size (%lu) != computed size+4 (%lu)",
		     (unsigned long)size,
		     (unsigned long)computed_size);
	}

	PG_FREE_IF_COPY(geom,0);
	PG_RETURN_INT32(size);
}

/** get summary info on a GEOMETRY */
PG_FUNCTION_INFO_V1(LWGEOM_summary);
Datum LWGEOM_summary(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	char *result;
	text *mytext;
	LWGEOM *lwgeom;

	lwgeom = lwgeom_deserialize(SERIALIZED_FORM(geom));

	result = lwgeom_summary(lwgeom, 0);

	/* create a text obj to return */
	mytext = (text *) lwalloc(VARHDRSZ  + strlen(result) + 1);
	SET_VARSIZE(mytext, VARHDRSZ + strlen(result) + 1);
	VARDATA(mytext)[0] = '\n';
	memcpy(VARDATA(mytext)+1, result, strlen(result) );

	lwfree(result);
	PG_FREE_IF_COPY(geom,0);

	PG_RETURN_POINTER(mytext);
}

PG_FUNCTION_INFO_V1(postgis_version);
Datum postgis_version(PG_FUNCTION_ARGS)
{
	char *ver = POSTGIS_VERSION;
	text *result;
	result = lwalloc(VARHDRSZ  + strlen(ver));
	SET_VARSIZE(result, VARHDRSZ + strlen(ver));
	memcpy(VARDATA(result), ver, strlen(ver));
	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(postgis_lib_version);
Datum postgis_lib_version(PG_FUNCTION_ARGS)
{
	char *ver = POSTGIS_LIB_VERSION;
	text *result;
	result = lwalloc(VARHDRSZ  + strlen(ver));
	SET_VARSIZE(result, VARHDRSZ + strlen(ver));
	memcpy(VARDATA(result), ver, strlen(ver));
	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(postgis_lib_build_date);
Datum postgis_lib_build_date(PG_FUNCTION_ARGS)
{
	char *ver = POSTGIS_BUILD_DATE;
	text *result;
	result = lwalloc(VARHDRSZ  + strlen(ver));
	SET_VARSIZE(result, VARHDRSZ + strlen(ver));
	memcpy(VARDATA(result), ver, strlen(ver));
	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(postgis_scripts_released);
Datum postgis_scripts_released(PG_FUNCTION_ARGS)
{
	char *ver = POSTGIS_SCRIPTS_VERSION;
	text *result;
	result = lwalloc(VARHDRSZ  + strlen(ver));
	SET_VARSIZE(result, VARHDRSZ + strlen(ver));
	memcpy(VARDATA(result), ver, strlen(ver));
	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(postgis_uses_stats);
Datum postgis_uses_stats(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(TRUE);
}

PG_FUNCTION_INFO_V1(postgis_autocache_bbox);
Datum postgis_autocache_bbox(PG_FUNCTION_ARGS)
{
#ifdef POSTGIS_AUTOCACHE_BBOX
	PG_RETURN_BOOL(TRUE);
#else
	PG_RETURN_BOOL(FALSE);
#endif
}


/**
 * Recursively count points in a SERIALIZED lwgeom
 */
int32
lwgeom_npoints(uchar *serialized)
{
	LWGEOM_INSPECTED *inspected = lwgeom_inspect(serialized);
	int i, j;
	int npoints=0;

	/* now have to do a scan of each object */
	for (i=0; i<inspected->ngeometries; i++)
	{
		LWLINE *line=NULL;
		LWPOINT *point=NULL;
		LWPOLY *poly=NULL;
		LWCIRCSTRING *curve=NULL;
		uchar *subgeom=NULL;

		point = lwgeom_getpoint_inspected(inspected, i);
		if (point !=NULL)
		{
			npoints++;
			continue;
		}

		poly = lwgeom_getpoly_inspected(inspected, i);
		if (poly !=NULL)
		{
			for (j=0; j<poly->nrings; j++)
			{
				npoints += poly->rings[j]->npoints;
			}
			continue;
		}

		line = lwgeom_getline_inspected(inspected, i);
		if (line != NULL)
		{
			npoints += line->points->npoints;
			continue;
		}

		curve = lwgeom_getcircstring_inspected(inspected, i);
		if (curve != NULL)
		{
			npoints += curve->points->npoints;
			continue;
		}

		subgeom = lwgeom_getsubgeometry_inspected(inspected, i);
		if ( subgeom != NULL )
		{
			npoints += lwgeom_npoints(subgeom);
		}
		else
		{
			elog(ERROR, "What ? lwgeom_getsubgeometry_inspected returned NULL??");
		}
	}
	return npoints;
}

/**
 * Recursively count rings in a SERIALIZED lwgeom
 */
int32
lwgeom_nrings_recursive(uchar *serialized)
{
	LWGEOM_INSPECTED *inspected;
	int i;
	int nrings=0;

	inspected = lwgeom_inspect(serialized);

	/* now have to do a scan of each object */
	for (i=0; i<inspected->ngeometries; i++)
	{
		LWPOLY *poly=NULL;
		uchar *subgeom=NULL;

		subgeom = lwgeom_getsubgeometry_inspected(inspected, i);

		if ( lwgeom_getType(subgeom[0]) == POLYGONTYPE )
		{
			poly = lwpoly_deserialize(subgeom);
			nrings += poly->nrings;
			continue;
		}

		if ( lwgeom_getType(subgeom[0]) == COLLECTIONTYPE )
		{
			nrings += lwgeom_nrings_recursive(subgeom);
			continue;
		}
	}

	lwinspected_release(inspected);

	return nrings;
}

/** number of points in an object */
PG_FUNCTION_INFO_V1(LWGEOM_npoints);
Datum LWGEOM_npoints(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int32 npoints = 0;

	npoints = lwgeom_npoints(SERIALIZED_FORM(geom));

	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_INT32(npoints);
}

/** number of rings in an object */
PG_FUNCTION_INFO_V1(LWGEOM_nrings);
Datum LWGEOM_nrings(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int32 nrings = 0;

	nrings = lwgeom_nrings_recursive(SERIALIZED_FORM(geom));

	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_INT32(nrings);
}

/**
 * @brief Calculate the area of all the subobj in a polygon
 * 		area(point) = 0
 * 		area (line) = 0
 * 		area(polygon) = find its 2d area
 */
PG_FUNCTION_INFO_V1(LWGEOM_area_polygon);
Datum LWGEOM_area_polygon(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM_INSPECTED *inspected = lwgeom_inspect(SERIALIZED_FORM(geom));
	LWPOLY *poly;
	LWCURVEPOLY *curvepoly;
	LWGEOM *tmp;
	double area = 0.0;
	int i;

	POSTGIS_DEBUG(2, "in LWGEOM_area_polygon");

	for (i=0; i<inspected->ngeometries; i++)
	{
		tmp = lwgeom_getgeom_inspected(inspected, i);
		if (lwgeom_getType(tmp->type) == POLYGONTYPE)
		{
			poly = (LWPOLY *)tmp;
			area += lwgeom_polygon_area(poly);
		}
		else if (lwgeom_getType(tmp->type) == CURVEPOLYTYPE)
		{
			curvepoly = (LWCURVEPOLY *)tmp;
			area += lwgeom_curvepolygon_area(curvepoly);
		}
		lwgeom_release(tmp);

		POSTGIS_DEBUGF(3, " LWGEOM_area_polygon found a poly (%f)", area);
	}

	lwinspected_release(inspected);

	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_FLOAT8(area);
}

/**
 * @brief find the "length of a geometry"
 *  	length2d(point) = 0
 *  	length2d(line) = length of line
 *  	length2d(polygon) = 0  -- could make sense to return sum(ring perimeter)
 *  	uses euclidian 2d length (even if input is 3d)
 */
PG_FUNCTION_INFO_V1(LWGEOM_length2d_linestring);
Datum LWGEOM_length2d_linestring(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM_INSPECTED *inspected = lwgeom_inspect(SERIALIZED_FORM(geom));
	LWLINE *line;
	double dist = 0.0;
	int i;

	POSTGIS_DEBUG(2, "in LWGEOM_length2d");

	for (i=0; i<inspected->ngeometries; i++)
	{
		line = lwgeom_getline_inspected(inspected, i);
		if ( line == NULL ) continue;
		dist += lwgeom_pointarray_length2d(line->points);

		POSTGIS_DEBUGF(3, " LWGEOM_length2d found a line (%f)", dist);
	}

	lwinspected_release(inspected);

	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_FLOAT8(dist);
}

/**
 * @brief find the "length of a geometry"
 *  	length(point) = 0
 *  	length(line) = length of line
 *  	length(polygon) = 0  -- could make sense to return sum(ring perimeter)
 *  	uses euclidian 3d/2d length depending on input dimensions.
 */
PG_FUNCTION_INFO_V1(LWGEOM_length_linestring);
Datum LWGEOM_length_linestring(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM_INSPECTED *inspected = lwgeom_inspect(SERIALIZED_FORM(geom));
	LWLINE *line;
	double dist = 0.0;
	int i;

	for (i=0; i<inspected->ngeometries; i++)
	{
		line = lwgeom_getline_inspected(inspected, i);
		if ( line == NULL ) continue;
		dist += lwgeom_pointarray_length(line->points);
	}

	lwinspected_release(inspected);

	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_FLOAT8(dist);
}

/**
 *  @brief find the "perimeter of a geometry"
 *  	perimeter(point) = 0
 *  	perimeter(line) = 0
 *  	perimeter(polygon) = sum of ring perimeters
 *  	uses euclidian 3d/2d computation depending on input dimension.
 */
PG_FUNCTION_INFO_V1(LWGEOM_perimeter_poly);
Datum LWGEOM_perimeter_poly(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM_INSPECTED *inspected = lwgeom_inspect(SERIALIZED_FORM(geom));
	double ret = 0.0;
	int i;

	for (i=0; i<inspected->ngeometries; i++)
	{
		LWPOLY *poly;
		poly = lwgeom_getpoly_inspected(inspected, i);
		if ( poly == NULL ) continue;
		ret += lwgeom_polygon_perimeter(poly);
	}

	lwinspected_release(inspected);

	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_FLOAT8(ret);
}

/**
 *  @brief find the "perimeter of a geometry"
 *  	perimeter(point) = 0
 *  	perimeter(line) = 0
 *  	perimeter(polygon) = sum of ring perimeters
 *  	uses euclidian 2d computation even if input is 3d
 */
PG_FUNCTION_INFO_V1(LWGEOM_perimeter2d_poly);
Datum LWGEOM_perimeter2d_poly(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM_INSPECTED *inspected = lwgeom_inspect(SERIALIZED_FORM(geom));
	double ret = 0.0;
	int i;

	for (i=0; i<inspected->ngeometries; i++)
	{
		LWPOLY *poly;
		poly = lwgeom_getpoly_inspected(inspected, i);
		if ( poly == NULL ) continue;
		ret += lwgeom_polygon_perimeter2d(poly);
	}

	lwinspected_release(inspected);
	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_FLOAT8(ret);
}


/**
 *	@brief  Write to already allocated memory 'optr' a 2d version of
 * 		the given serialized form.
 * 		Higher dimensions in input geometry are discarded.
 * 	@return number bytes written in given int pointer.
 */
void
lwgeom_force2d_recursive(uchar *serialized, uchar *optr, size_t *retsize)
{
	LWGEOM_INSPECTED *inspected;
	int i,j,k;
	size_t totsize=0;
	size_t size=0;
	int type;
	uchar newtypefl;
	LWPOINT *point = NULL;
	LWLINE *line = NULL;
	LWCIRCSTRING *curve = NULL;
	LWPOLY *poly = NULL;
	POINTARRAY newpts;
	POINTARRAY **nrings;
	POINT2D p2d;
	uchar *loc;


	LWDEBUG(2, "lwgeom_force2d_recursive: call");

	type = lwgeom_getType(serialized[0]);

	if ( type == POINTTYPE )
	{
		point = lwpoint_deserialize(serialized);
		TYPE_SETZM(newpts.dims, 0, 0);
		newpts.npoints = 1;
		newpts.serialized_pointlist = lwalloc(sizeof(POINT2D));
		loc = newpts.serialized_pointlist;
		getPoint2d_p(point->point, 0, &p2d);
		memcpy(loc, &p2d, sizeof(POINT2D));
		point->point = &newpts;
		TYPE_SETZM(point->type, 0, 0);
		lwpoint_serialize_buf(point, optr, retsize);
		lwfree(newpts.serialized_pointlist);
		lwfree(point);

		LWDEBUG(3, "lwgeom_force2d_recursive returning");

		return;
	}

	if ( type == LINETYPE )
	{
		line = lwline_deserialize(serialized);

		LWDEBUGF(3, "lwgeom_force2d_recursive: it's a line with %d points", line->points->npoints);

		TYPE_SETZM(newpts.dims, 0, 0);
		newpts.npoints = line->points->npoints;
		newpts.serialized_pointlist = lwalloc(sizeof(POINT2D)*line->points->npoints);

		LWDEBUGF(3, "lwgeom_force2d_recursive: %d bytes pointlist allocated", sizeof(POINT2D)*line->points->npoints);

		loc = newpts.serialized_pointlist;
		for (j=0; j<line->points->npoints; j++)
		{
			getPoint2d_p(line->points, j, &p2d);
			memcpy(loc, &p2d, sizeof(POINT2D));
			loc+=sizeof(POINT2D);
		}
		line->points = &newpts;
		TYPE_SETZM(line->type, 0, 0);
		lwline_serialize_buf(line, optr, retsize);
		lwfree(newpts.serialized_pointlist);
		lwfree(line);

		LWDEBUG(3, "lwgeom_force2d_recursive returning");

		return;
	}

	if ( type == CIRCSTRINGTYPE )
	{
		curve = lwcircstring_deserialize(serialized);

		LWDEBUGF(3, "lwgeom_force2d_recursize: it's a circularstring with %d points", curve->points->npoints);

		TYPE_SETZM(newpts.dims, 0, 0);
		newpts.npoints = curve->points->npoints;
		newpts.serialized_pointlist = lwalloc(sizeof(POINT2D)*curve->points->npoints);

		LWDEBUGF(3, "lwgeom_force2d_recursive: %d bytes pointlist allocated", sizeof(POINT2D)*curve->points->npoints);

		loc = newpts.serialized_pointlist;
		for (j=0; j<curve->points->npoints; j++)
		{
			getPoint2d_p(curve->points, j, &p2d);
			memcpy(loc, &p2d, sizeof(POINT2D));
			loc += sizeof(POINT2D);
		}
		curve->points = &newpts;
		TYPE_SETZM(curve->type, 0, 0);
		lwcircstring_serialize_buf(curve, optr, retsize);
		lwfree(newpts.serialized_pointlist);
		lwfree(curve);
		return;
	}

	if ( type == POLYGONTYPE )
	{
		poly = lwpoly_deserialize(serialized);
		TYPE_SETZM(newpts.dims, 0, 0);
		newpts.npoints = 0;
		newpts.serialized_pointlist = lwalloc(1);
		nrings = lwalloc(sizeof(POINTARRAY *)*poly->nrings);
		loc = newpts.serialized_pointlist;
		for (j=0; j<poly->nrings; j++)
		{
			POINTARRAY *ring = poly->rings[j];
			POINTARRAY *nring = lwalloc(sizeof(POINTARRAY));
			TYPE_SETZM(nring->dims, 0, 0);
			nring->npoints = ring->npoints;
			nring->serialized_pointlist =
			    lwalloc(ring->npoints*sizeof(POINT2D));
			loc = nring->serialized_pointlist;
			for (k=0; k<ring->npoints; k++)
			{
				getPoint2d_p(ring, k, &p2d);
				memcpy(loc, &p2d, sizeof(POINT2D));
				loc+=sizeof(POINT2D);
			}
			nrings[j] = nring;
		}
		poly->rings = nrings;
		TYPE_SETZM(poly->type, 0, 0);
		lwpoly_serialize_buf(poly, optr, retsize);
		lwfree(poly);
		/** @todo TODO: free nrigs[*]->serialized_pointlist
			*/

		LWDEBUG(3, "lwgeom_force2d_recursive returning");

		return;
	}

	if ( type != MULTIPOINTTYPE && type != MULTIPOLYGONTYPE &&
	        type != MULTILINETYPE && type != COLLECTIONTYPE &&
	        type != COMPOUNDTYPE && type != CURVEPOLYTYPE &&
	        type != MULTICURVETYPE && type != MULTISURFACETYPE)
	{
		lwerror("lwgeom_force2d_recursive: unknown geometry: %d",
		        type);
	}

	/*
	* OK, this is a collection, so we write down its metadata
	* first and then call us again
	*/

	LWDEBUGF(3, "lwgeom_force2d_recursive: it's a collection (%s)", lwgeom_typename(type));


	/* Add type */
	newtypefl = lwgeom_makeType_full(0, 0, lwgeom_hasSRID(serialized[0]),
	                                 type, lwgeom_hasBBOX(serialized[0]));
	optr[0] = newtypefl;
	optr++;
	totsize++;
	loc=serialized+1;

	LWDEBUGF(3, "lwgeom_force2d_recursive: added collection type (%s[%s]) - size:%d", lwgeom_typename(type), lwgeom_typeflags(newtypefl), totsize);

	if ( lwgeom_hasBBOX(serialized[0]) != lwgeom_hasBBOX(newtypefl) )
		lwerror("typeflag mismatch in BBOX");
	if ( lwgeom_hasSRID(serialized[0]) != lwgeom_hasSRID(newtypefl) )
		lwerror("typeflag mismatch in SRID");

	/* Add BBOX if any */
	if (lwgeom_hasBBOX(serialized[0]))
	{
		memcpy(optr, loc, sizeof(BOX2DFLOAT4));
		optr += sizeof(BOX2DFLOAT4);
		totsize += sizeof(BOX2DFLOAT4);
		loc += sizeof(BOX2DFLOAT4);

		LWDEBUGF(3, "lwgeom_force2d_recursive: added collection bbox - size:%d", totsize);
	}

	/* Add SRID if any */
	if (lwgeom_hasSRID(serialized[0]))
	{
		memcpy(optr, loc, 4);
		optr += 4;
		totsize += 4;
		loc += 4;

		LWDEBUGF(3, "lwgeom_force2d_recursive: added collection SRID - size:%d", totsize);
	}

	/* Add numsubobjects */
	memcpy(optr, loc, sizeof(uint32));
	optr += sizeof(uint32);
	totsize += sizeof(uint32);
	loc += sizeof(uint32);

	LWDEBUGF(3, "lwgeom_force2d_recursive: added collection ngeoms - size:%d", totsize);

	LWDEBUG(3, "lwgeom_force2d_recursive: inspecting subgeoms");

	/* Now recurse for each subobject */
	inspected = lwgeom_inspect(serialized);
	for (i=0; i<inspected->ngeometries; i++)
	{
		uchar *subgeom = lwgeom_getsubgeometry_inspected(inspected, i);
		lwgeom_force2d_recursive(subgeom, optr, &size);
		totsize += size;
		optr += size;

		LWDEBUGF(3, "lwgeom_force2d_recursive: added elem %d size: %d (tot: %d)",
		         i, size, totsize);
	}
	lwinspected_release(inspected);

	LWDEBUG(3, "lwgeom_force2d_recursive returning");

	if ( retsize ) *retsize = totsize;
}

/**
 * @brief Write to already allocated memory 'optr' a 3dz version of
 * 		the given serialized form.
 * 		Higher dimensions in input geometry are discarder.
 * 		If the given version is 2d Z is set to 0.
 * @return number bytes written in given int pointer.
 */
void
lwgeom_force3dz_recursive(uchar *serialized, uchar *optr, size_t *retsize)
{
	LWGEOM_INSPECTED *inspected;
	int i,j,k;
	size_t totsize=0;
	size_t size=0;
	int type;
	LWPOINT *point = NULL;
	LWLINE *line = NULL;
	LWCIRCSTRING *curve = NULL;
	LWPOLY *poly = NULL;
	POINTARRAY newpts;
	POINTARRAY **nrings;
	uchar *loc;
	POINT3DZ point3dz;


	LWDEBUG(2, "lwgeom_force3dz_recursive: call");

	type = lwgeom_getType(serialized[0]);

	if ( type == POINTTYPE )
	{
		point = lwpoint_deserialize(serialized);
		TYPE_SETZM(newpts.dims, 1, 0);
		newpts.npoints = 1;
		newpts.serialized_pointlist = lwalloc(sizeof(POINT3DZ));
		loc = newpts.serialized_pointlist;
		getPoint3dz_p(point->point, 0, &point3dz);
		memcpy(loc, &point3dz, sizeof(POINT3DZ));
		point->point = &newpts;
		TYPE_SETZM(point->type, 1, 0);
		lwpoint_serialize_buf(point, optr, retsize);

		LWDEBUGF(3, "lwgeom_force3dz_recursive: it's a point, size:%d", *retsize);

		return;
	}

	if ( type == LINETYPE )
	{
		line = lwline_deserialize(serialized);

		LWDEBUG(3, "lwgeom_force3dz_recursive: it's a line");

		TYPE_SETZM(newpts.dims, 1, 0);
		newpts.npoints = line->points->npoints;
		newpts.serialized_pointlist = lwalloc(sizeof(POINT3DZ)*line->points->npoints);
		loc = newpts.serialized_pointlist;
		for (j=0; j<line->points->npoints; j++)
		{
			getPoint3dz_p(line->points, j, &point3dz);
			memcpy(loc, &point3dz, sizeof(POINT3DZ));
			loc+=sizeof(POINT3DZ);
		}
		line->points = &newpts;
		TYPE_SETZM(line->type, 1, 0);
		lwline_serialize_buf(line, optr, retsize);

		LWDEBUGF(3, "lwgeom_force3dz_recursive: it's a line, size:%d", *retsize);

		return;
	}

	if ( type == CIRCSTRINGTYPE )
	{
		curve = lwcircstring_deserialize(serialized);

		LWDEBUG(3, "lwgeom_force3dz_recursize: it's a circularstring");

		TYPE_SETZM(newpts.dims, 1, 0);
		newpts.npoints = curve->points->npoints;
		newpts.serialized_pointlist = lwalloc(sizeof(POINT3DZ)*curve->points->npoints);
		loc = newpts.serialized_pointlist;
		for (j=0; j<curve->points->npoints; j++)
		{
			getPoint3dz_p(curve->points, j, &point3dz);
			memcpy(loc, &point3dz, sizeof(POINT3DZ));
			loc+=sizeof(POINT3DZ);
		}
		curve->points = &newpts;
		TYPE_SETZM(curve->type, 1, 0);
		lwcircstring_serialize_buf(curve, optr, retsize);

		LWDEBUGF(3, "lwgeom_force3dz_recursive: it's a circularstring, size:%d", *retsize);

		return;
	}

	if ( type == POLYGONTYPE )
	{
		poly = lwpoly_deserialize(serialized);
		TYPE_SETZM(newpts.dims, 1, 0);
		newpts.npoints = 0;
		newpts.serialized_pointlist = lwalloc(1);
		nrings = lwalloc(sizeof(POINTARRAY *)*poly->nrings);
		loc = newpts.serialized_pointlist;
		for (j=0; j<poly->nrings; j++)
		{
			POINTARRAY *ring = poly->rings[j];
			POINTARRAY *nring = lwalloc(sizeof(POINTARRAY));
			TYPE_SETZM(nring->dims, 1, 0);
			nring->npoints = ring->npoints;
			nring->serialized_pointlist =
			    lwalloc(ring->npoints*sizeof(POINT3DZ));
			loc = nring->serialized_pointlist;
			for (k=0; k<ring->npoints; k++)
			{
				getPoint3dz_p(ring, k, &point3dz);
				memcpy(loc, &point3dz, sizeof(POINT3DZ));
				loc+=sizeof(POINT3DZ);
			}
			nrings[j] = nring;
		}
		poly->rings = nrings;
		TYPE_SETZM(poly->type, 1, 0);
		lwpoly_serialize_buf(poly, optr, retsize);

		LWDEBUGF(3, "lwgeom_force3dz_recursive: it's a poly, size:%d", *retsize);

		return;
	}

	/*
	* OK, this is a collection, so we write down its metadata
	* first and then call us again
	*/

	LWDEBUGF(3, "lwgeom_force3dz_recursive: it's a collection (type:%d)", type);

	/* Add type */
	*optr = lwgeom_makeType_full(1, 0, lwgeom_hasSRID(serialized[0]),
	                             type, lwgeom_hasBBOX(serialized[0]));
	optr++;
	totsize++;
	loc=serialized+1;

	/* Add BBOX if any */
	if (lwgeom_hasBBOX(serialized[0]))
	{
		memcpy(optr, loc, sizeof(BOX2DFLOAT4));
		optr += sizeof(BOX2DFLOAT4);
		totsize += sizeof(BOX2DFLOAT4);
		loc += sizeof(BOX2DFLOAT4);
	}

	/* Add SRID if any */
	if (lwgeom_hasSRID(serialized[0]))
	{
		memcpy(optr, loc, 4);
		optr += 4;
		totsize += 4;
		loc += 4;
	}

	/* Add numsubobjects */
	memcpy(optr, loc, 4);
	optr += 4;
	totsize += 4;

	LWDEBUGF(3, " collection header size:%d", totsize);

	/* Now recurse for each suboject */
	inspected = lwgeom_inspect(serialized);
	for (i=0; i<inspected->ngeometries; i++)
	{
		uchar *subgeom = lwgeom_getsubgeometry_inspected(inspected, i);
		lwgeom_force3dz_recursive(subgeom, optr, &size);
		totsize += size;
		optr += size;

		LWDEBUGF(3, " elem %d size: %d (tot: %d)", i, size, totsize);
	}
	lwinspected_release(inspected);

	*retsize = totsize;
}

/**
 * @brief Write to already allocated memory 'optr' a 3dm version of
 * 		the given serialized form.
 * 		Higher dimensions in input geometry are discarder.
 * 		If the given version is 2d M is set to 0.
 * @return number bytes written in given int pointer.
 */
void
lwgeom_force3dm_recursive(uchar *serialized, uchar *optr, size_t *retsize)
{
	LWGEOM_INSPECTED *inspected;
	int i,j,k;
	size_t totsize=0;
	size_t size=0;
	int type;
	uchar newtypefl;
	LWPOINT *point = NULL;
	LWLINE *line = NULL;
	LWCIRCSTRING *curve = NULL;
	LWPOLY *poly = NULL;
	POINTARRAY newpts;
	POINTARRAY **nrings;
	POINT3DM p3dm;
	uchar *loc;


	LWDEBUG(2, "lwgeom_force3dm_recursive: call");

	type = lwgeom_getType(serialized[0]);

	if ( type == POINTTYPE )
	{
		point = lwpoint_deserialize(serialized);
		TYPE_SETZM(newpts.dims, 0, 1);
		newpts.npoints = 1;
		newpts.serialized_pointlist = lwalloc(sizeof(POINT3DM));
		loc = newpts.serialized_pointlist;
		getPoint3dm_p(point->point, 0, &p3dm);
		memcpy(loc, &p3dm, sizeof(POINT3DM));
		point->point = &newpts;
		TYPE_SETZM(point->type, 0, 1);
		lwpoint_serialize_buf(point, optr, retsize);
		lwfree(newpts.serialized_pointlist);
		lwfree(point);

		LWDEBUG(3, "lwgeom_force3dm_recursive returning");

		return;
	}

	if ( type == LINETYPE )
	{
		line = lwline_deserialize(serialized);

		LWDEBUGF(3, "lwgeom_force3dm_recursive: it's a line with %d points", line->points->npoints);

		TYPE_SETZM(newpts.dims, 0, 1);
		newpts.npoints = line->points->npoints;
		newpts.serialized_pointlist = lwalloc(sizeof(POINT3DM)*line->points->npoints);

		LWDEBUGF(3, "lwgeom_force3dm_recursive: %d bytes pointlist allocated", sizeof(POINT3DM)*line->points->npoints);

		loc = newpts.serialized_pointlist;
		for (j=0; j<line->points->npoints; j++)
		{
			getPoint3dm_p(line->points, j, &p3dm);
			memcpy(loc, &p3dm, sizeof(POINT3DM));
			loc+=sizeof(POINT3DM);
		}
		line->points = &newpts;
		TYPE_SETZM(line->type, 0, 1);
		lwline_serialize_buf(line, optr, retsize);
		lwfree(newpts.serialized_pointlist);
		lwfree(line);

		LWDEBUG(3, "lwgeom_force3dm_recursive returning");

		return;
	}

	if ( type == CIRCSTRINGTYPE )
	{
		curve = lwcircstring_deserialize(serialized);

		LWDEBUGF(3, "lwgeom_force3dm_recursize: it's a circularstring with %d points", curve->points->npoints);

		TYPE_SETZM(newpts.dims, 0, 1);
		newpts.npoints = curve->points->npoints;
		newpts.serialized_pointlist = lwalloc(sizeof(POINT3DM)*curve->points->npoints);

		loc = newpts.serialized_pointlist;
		for (j=0; j<curve->points->npoints; j++)
		{
			getPoint3dm_p(curve->points, j, &p3dm);
			memcpy(loc, &p3dm, sizeof(POINT3DM));
			loc+=sizeof(POINT3DM);
		}
		curve->points = &newpts;
		TYPE_SETZM(curve->type, 0, 1);
		lwcircstring_serialize_buf(curve, optr, retsize);
		lwfree(newpts.serialized_pointlist);
		lwfree(curve);
		return;
	}

	if ( type == POLYGONTYPE )
	{
		poly = lwpoly_deserialize(serialized);
		TYPE_SETZM(newpts.dims, 0, 1);
		newpts.npoints = 0;
		newpts.serialized_pointlist = lwalloc(1);
		nrings = lwalloc(sizeof(POINTARRAY *)*poly->nrings);
		loc = newpts.serialized_pointlist;
		for (j=0; j<poly->nrings; j++)
		{
			POINTARRAY *ring = poly->rings[j];
			POINTARRAY *nring = lwalloc(sizeof(POINTARRAY));
			TYPE_SETZM(nring->dims, 0, 1);
			nring->npoints = ring->npoints;
			nring->serialized_pointlist =
			    lwalloc(ring->npoints*sizeof(POINT3DM));
			loc = nring->serialized_pointlist;
			for (k=0; k<ring->npoints; k++)
			{
				getPoint3dm_p(ring, k, &p3dm);
				memcpy(loc, &p3dm, sizeof(POINT3DM));
				loc+=sizeof(POINT3DM);
			}
			nrings[j] = nring;
		}
		poly->rings = nrings;
		TYPE_SETZM(poly->type, 0, 1);
		lwpoly_serialize_buf(poly, optr, retsize);
		lwfree(poly);
		/** @todo TODO: free nrigs[*]->serialized_pointlist
		*/

		LWDEBUG(3, "lwgeom_force3dm_recursive returning");

		return;
	}

	if ( type != MULTIPOINTTYPE && type != MULTIPOLYGONTYPE &&
	        type != MULTILINETYPE && type != COLLECTIONTYPE &&
	        type != COMPOUNDTYPE && type != CURVEPOLYTYPE &&
	        type != MULTICURVETYPE && type != MULTISURFACETYPE)
	{
		lwerror("lwgeom_force3dm_recursive: unknown geometry: %d",
		        type);
	}

	/*
	* OK, this is a collection, so we write down its metadata
	* first and then call us again
	*/

	LWDEBUGF(3, "lwgeom_force3dm_recursive: it's a collection (%s)", lwgeom_typename(type));


	/* Add type */
	newtypefl = lwgeom_makeType_full(0, 1, lwgeom_hasSRID(serialized[0]),
	                                 type, lwgeom_hasBBOX(serialized[0]));
	optr[0] = newtypefl;
	optr++;
	totsize++;
	loc=serialized+1;

	LWDEBUGF(3, "lwgeom_force3dm_recursive: added collection type (%s[%s]) - size:%d", lwgeom_typename(type), lwgeom_typeflags(newtypefl), totsize);

	if ( lwgeom_hasBBOX(serialized[0]) != lwgeom_hasBBOX(newtypefl) )
		lwerror("typeflag mismatch in BBOX");
	if ( lwgeom_hasSRID(serialized[0]) != lwgeom_hasSRID(newtypefl) )
		lwerror("typeflag mismatch in SRID");

	/* Add BBOX if any */
	if (lwgeom_hasBBOX(serialized[0]))
	{
		memcpy(optr, loc, sizeof(BOX2DFLOAT4));
		optr += sizeof(BOX2DFLOAT4);
		totsize += sizeof(BOX2DFLOAT4);
		loc += sizeof(BOX2DFLOAT4);

		LWDEBUGF(3, "lwgeom_force3dm_recursive: added collection bbox - size:%d", totsize);
	}

	/* Add SRID if any */
	if (lwgeom_hasSRID(serialized[0]))
	{
		memcpy(optr, loc, 4);
		optr += 4;
		totsize += 4;
		loc += 4;

		LWDEBUGF(3, "lwgeom_force3dm_recursive: added collection SRID - size:%d", totsize);
	}

	/* Add numsubobjects */
	memcpy(optr, loc, sizeof(uint32));
	optr += sizeof(uint32);
	totsize += sizeof(uint32);
	loc += sizeof(uint32);

	LWDEBUGF(3, "lwgeom_force3dm_recursive: added collection ngeoms - size:%d", totsize);

	LWDEBUG(3, "lwgeom_force3dm_recursive: inspecting subgeoms");

	/* Now recurse for each subobject */
	inspected = lwgeom_inspect(serialized);
	for (i=0; i<inspected->ngeometries; i++)
	{
		uchar *subgeom = lwgeom_getsubgeometry_inspected(inspected, i);
		lwgeom_force3dm_recursive(subgeom, optr, &size);
		totsize += size;
		optr += size;

		LWDEBUGF(3, "lwgeom_force3dm_recursive: added elem %d size: %d (tot: %d)",
		         i, size, totsize);
	}
	lwinspected_release(inspected);

	LWDEBUG(3, "lwgeom_force3dm_recursive returning");

	if ( retsize ) *retsize = totsize;
}


/*
 * Write to already allocated memory 'optr' a 4d version of
 * the given serialized form.
 * Pad dimensions are set to 0 (this might be z, m or both).
 * Return number bytes written in given int pointer.
 */
void
lwgeom_force4d_recursive(uchar *serialized, uchar *optr, size_t *retsize)
{
	LWGEOM_INSPECTED *inspected;
	int i,j,k;
	size_t totsize=0;
	size_t size=0;
	int type;
	LWPOINT *point = NULL;
	LWLINE *line = NULL;
	LWCIRCSTRING *curve = NULL;
	LWPOLY *poly = NULL;
	POINTARRAY newpts;
	POINTARRAY **nrings;
	POINT4D p4d;
	uchar *loc;


	LWDEBUG(2, "lwgeom_force4d_recursive: call");

	type = lwgeom_getType(serialized[0]);

	if ( type == POINTTYPE )
	{
		point = lwpoint_deserialize(serialized);
		TYPE_SETZM(newpts.dims, 1, 1);
		newpts.npoints = 1;
		newpts.serialized_pointlist = lwalloc(sizeof(POINT4D));
		loc = newpts.serialized_pointlist;
		getPoint4d_p(point->point, 0, &p4d);
		memcpy(loc, &p4d, sizeof(POINT4D));
		point->point = &newpts;
		TYPE_SETZM(point->type, 1, 1);
		lwpoint_serialize_buf(point, optr, retsize);

		LWDEBUGF(3, "lwgeom_force4d_recursive: it's a point, size:%d", *retsize);

		return;
	}

	if ( type == LINETYPE )
	{
		LWDEBUG(3, "lwgeom_force4d_recursive: it's a line");

		line = lwline_deserialize(serialized);
		TYPE_SETZM(newpts.dims, 1, 1);
		newpts.npoints = line->points->npoints;
		newpts.serialized_pointlist = lwalloc(sizeof(POINT4D)*line->points->npoints);
		loc = newpts.serialized_pointlist;
		for (j=0; j<line->points->npoints; j++)
		{
			getPoint4d_p(line->points, j, &p4d);
			memcpy(loc, &p4d, sizeof(POINT4D));
			loc+=sizeof(POINT4D);
		}
		line->points = &newpts;
		TYPE_SETZM(line->type, 1, 1);
		lwline_serialize_buf(line, optr, retsize);

		LWDEBUGF(3, "lwgeom_force4d_recursive: it's a line, size:%d", *retsize);

		return;
	}

	if ( type == CIRCSTRINGTYPE )
	{
		curve = lwcircstring_deserialize(serialized);
		TYPE_SETZM(newpts.dims, 1, 1);
		newpts.npoints = curve->points->npoints;
		newpts.serialized_pointlist = lwalloc(sizeof(POINT4D)*curve->points->npoints);
		loc = newpts.serialized_pointlist;
		for (j=0; j<curve->points->npoints; j++)
		{
			getPoint4d_p(curve->points, j, &p4d);
			memcpy(loc, &p4d, sizeof(POINT4D));
			loc+=sizeof(POINT4D);
		}
		curve->points = &newpts;
		TYPE_SETZM(curve->type, 1, 1);
		lwcircstring_serialize_buf(curve, optr, retsize);

		LWDEBUGF(3, "lwgeom_force4d_recursive: it's a circularstring, size:%d", *retsize);

		return;
	}

	if ( type == POLYGONTYPE )
	{
		poly = lwpoly_deserialize(serialized);
		TYPE_SETZM(newpts.dims, 1, 1);
		newpts.npoints = 0;
		newpts.serialized_pointlist = lwalloc(1);
		nrings = lwalloc(sizeof(POINTARRAY *)*poly->nrings);
		loc = newpts.serialized_pointlist;
		for (j=0; j<poly->nrings; j++)
		{
			POINTARRAY *ring = poly->rings[j];
			POINTARRAY *nring = lwalloc(sizeof(POINTARRAY));
			TYPE_SETZM(nring->dims, 1, 1);
			nring->npoints = ring->npoints;
			nring->serialized_pointlist =
			    lwalloc(ring->npoints*sizeof(POINT4D));
			loc = nring->serialized_pointlist;
			for (k=0; k<ring->npoints; k++)
			{
				getPoint4d_p(ring, k, &p4d);
				memcpy(loc, &p4d, sizeof(POINT4D));
				loc+=sizeof(POINT4D);
			}
			nrings[j] = nring;
		}
		poly->rings = nrings;
		TYPE_SETZM(poly->type, 1, 1);
		lwpoly_serialize_buf(poly, optr, retsize);

		LWDEBUGF(3, "lwgeom_force4d_recursive: it's a poly, size:%d", *retsize);

		return;
	}

	/*
	* OK, this is a collection, so we write down its metadata
	* first and then call us again
	*/

	LWDEBUGF(3, "lwgeom_force4d_recursive: it's a collection (type:%d)", type);

	/* Add type */
	*optr = lwgeom_makeType_full(
	            1, 1,
	            lwgeom_hasSRID(serialized[0]),
	            type, lwgeom_hasBBOX(serialized[0]));
	optr++;
	totsize++;
	loc=serialized+1;

	/* Add BBOX if any */
	if (lwgeom_hasBBOX(serialized[0]))
	{
		memcpy(optr, loc, sizeof(BOX2DFLOAT4));
		optr += sizeof(BOX2DFLOAT4);
		totsize += sizeof(BOX2DFLOAT4);
		loc += sizeof(BOX2DFLOAT4);
	}

	/* Add SRID if any */
	if (lwgeom_hasSRID(serialized[0]))
	{
		memcpy(optr, loc, 4);
		optr += 4;
		totsize += 4;
		loc += 4;
	}

	/* Add numsubobjects */
	memcpy(optr, loc, 4);
	optr += 4;
	totsize += 4;

	LWDEBUGF(3, " collection header size:%d", totsize);

	/* Now recurse for each suboject */
	inspected = lwgeom_inspect(serialized);
	for (i=0; i<inspected->ngeometries; i++)
	{
		uchar *subgeom = lwgeom_getsubgeometry_inspected(inspected, i);
		lwgeom_force4d_recursive(subgeom, optr, &size);
		totsize += size;
		optr += size;

		LWDEBUGF(3, " elem %d size: %d (tot: %d)", i, size, totsize);
	}
	lwinspected_release(inspected);

	*retsize = totsize;
}

/* transform input geometry to 2d if not 2d already */
PG_FUNCTION_INFO_V1(LWGEOM_force_2d);
Datum LWGEOM_force_2d(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	uchar *srl;
	PG_LWGEOM *result;
	size_t size = 0;

	/* already 2d */
	if ( lwgeom_ndims(geom->type) == 2 ) PG_RETURN_POINTER(geom);

	/* allocate a larger for safety and simplicity */
	srl = lwalloc(VARSIZE(geom));

	lwgeom_force2d_recursive(SERIALIZED_FORM(geom),
	                         srl, &size);

	result = PG_LWGEOM_construct(srl, pglwgeom_getSRID(geom),
	                             lwgeom_hasBBOX(geom->type));
	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(result);
}

/* transform input geometry to 3dz if not 3dz already */
PG_FUNCTION_INFO_V1(LWGEOM_force_3dz);
Datum LWGEOM_force_3dz(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	uchar *srl;
	PG_LWGEOM *result;
	int olddims;
	size_t size = 0;

	olddims = lwgeom_ndims(geom->type);

	/* already 3d */
	if ( olddims == 3 && TYPE_HASZ(geom->type) ) PG_RETURN_POINTER(geom);

	if ( olddims > 3 )
	{
		srl = lwalloc(VARSIZE(geom));
	}
	else
	{
		/* allocate double as memory a larger for safety */
		srl = lwalloc(VARSIZE(geom)*1.5);
	}

	lwgeom_force3dz_recursive(SERIALIZED_FORM(geom),
	                          srl, &size);

	result = PG_LWGEOM_construct(srl, pglwgeom_getSRID(geom),
	                             lwgeom_hasBBOX(geom->type));

	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_POINTER(result);
}

/** transform input geometry to 3dm if not 3dm already */
PG_FUNCTION_INFO_V1(LWGEOM_force_3dm);
Datum LWGEOM_force_3dm(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	uchar *srl;
	PG_LWGEOM *result;
	int olddims;
	size_t size = 0;

	olddims = lwgeom_ndims(geom->type);

	/* already 3dm */
	if ( olddims == 3 && TYPE_HASM(geom->type) ) PG_RETURN_POINTER(geom);

	if ( olddims > 3 )
	{
		size = VARSIZE(geom);
	}
	else
	{
		/* allocate double as memory a larger for safety */
		size = VARSIZE(geom) * 2;
	}
	srl = lwalloc(size);

	POSTGIS_DEBUGF(3, "LWGEOM_force_3dm: allocated %d bytes for result", (int)size);

	lwgeom_force3dm_recursive(SERIALIZED_FORM(geom),
	                          srl, &size);

	POSTGIS_DEBUGF(3, "LWGEOM_force_3dm: lwgeom_force3dm_recursive returned a %d sized geom", (int)size);

	result = PG_LWGEOM_construct(srl, pglwgeom_getSRID(geom),
	                             lwgeom_hasBBOX(geom->type));

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(result);
}

/* transform input geometry to 4d if not 4d already */
PG_FUNCTION_INFO_V1(LWGEOM_force_4d);
Datum LWGEOM_force_4d(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	uchar *srl;
	PG_LWGEOM *result;
	int olddims;
	size_t size = 0;

	olddims = lwgeom_ndims(geom->type);

	/* already 4d */
	if ( olddims == 4 ) PG_RETURN_POINTER(geom);

	/* allocate double as memory a larger for safety  */
	srl = lwalloc(VARSIZE(geom)*2);

	lwgeom_force4d_recursive(SERIALIZED_FORM(geom),
	                         srl, &size);

	result = PG_LWGEOM_construct(srl, pglwgeom_getSRID(geom),
	                             lwgeom_hasBBOX(geom->type));

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(result);
}

/** transform input geometry to a collection type */
PG_FUNCTION_INFO_V1(LWGEOM_force_collection);
Datum LWGEOM_force_collection(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *result;
	LWGEOM *lwgeoms[1];
	LWGEOM *lwgeom;
	int SRID;
	BOX2DFLOAT4 *bbox;

	POSTGIS_DEBUG(2, "LWGEOM_force_collection called");

	/*
	 * This funx is a no-op only if a bbox cache is already present
	 * in input. If bbox cache is not there we'll need to handle
	 * automatic bbox addition FOR_COMPLEX_GEOMS.
	 */
	if ( TYPE_GETTYPE(geom->type) == COLLECTIONTYPE &&
	        TYPE_HASBBOX(geom->type) )
	{
		PG_RETURN_POINTER(geom);
	}

	/* deserialize into lwgeoms[0] */
	lwgeom = lwgeom_deserialize(SERIALIZED_FORM(geom));

	/* alread a multi*, just make it a collection */
	if ( lwgeom_is_collection(TYPE_GETTYPE(lwgeom->type)) )
	{
		TYPE_SETTYPE(lwgeom->type, COLLECTIONTYPE);
	}

	/* single geom, make it a collection */
	else
	{
		SRID = lwgeom->SRID;
		/* We transfer bbox ownership from input to output */
		bbox = lwgeom->bbox;
		lwgeom->SRID = -1;
		lwgeom->bbox = NULL;
		lwgeoms[0] = lwgeom;
		lwgeom = (LWGEOM *)lwcollection_construct(COLLECTIONTYPE,
		         SRID, bbox, 1,
		         lwgeoms);
	}

	result = pglwgeom_serialize(lwgeom);
	lwgeom_release(lwgeom);

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(result);
}

/** transform input geometry to a multi* type */
PG_FUNCTION_INFO_V1(LWGEOM_force_multi);
Datum LWGEOM_force_multi(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *result;
	LWGEOM *lwgeom;
	LWGEOM *ogeom;

	POSTGIS_DEBUG(2, "LWGEOM_force_multi called");

	/*
	** This funx is a no-op only if a bbox cache is already present
	** in input. If bbox cache is not there we'll need to handle
	** automatic bbox addition FOR_COMPLEX_GEOMS.
	*/
	if ( lwgeom_is_collection(TYPE_GETTYPE(geom->type)) && TYPE_HASBBOX(geom->type) )
	{
		PG_RETURN_POINTER(geom);
	}


	/* deserialize into lwgeoms[0] */
	lwgeom = lwgeom_deserialize(SERIALIZED_FORM(geom));
	ogeom = lwgeom_as_multi(lwgeom);

	result = pglwgeom_serialize(ogeom);

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(result);
}

/* Minimum 2d distance between objects in geom1 and geom2. */
PG_FUNCTION_INFO_V1(LWGEOM_mindistance2d);
Datum LWGEOM_mindistance2d(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	double mindist;

	PROFSTART(PROF_QRUN);

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	if (pglwgeom_getSRID(geom1) != pglwgeom_getSRID(geom2))
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}

	mindist = lwgeom_mindistance2d_recursive(SERIALIZED_FORM(geom1),
	          SERIALIZED_FORM(geom2));

	PROFSTOP(PROF_QRUN);
	PROFREPORT("dist",geom1, geom2, NULL);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_FLOAT8(mindist);
}

/* Minimum 2d distance between objects in geom1 and geom2. */
PG_FUNCTION_INFO_V1(LWGEOM_dwithin);
Datum LWGEOM_dwithin(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	double mindist, tolerance;

	PROFSTART(PROF_QRUN);

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	tolerance = PG_GETARG_FLOAT8(2);

	if ( tolerance < 0 )
	{
		elog(ERROR,"Tolerance cannot be less than zero\n");
		PG_RETURN_NULL();
	}

	if (pglwgeom_getSRID(geom1) != pglwgeom_getSRID(geom2))
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}

	mindist = lwgeom_mindistance2d_recursive_tolerance(
	              SERIALIZED_FORM(geom1),
	              SERIALIZED_FORM(geom2),
	              tolerance
	          );

	PROFSTOP(PROF_QRUN);
	PROFREPORT("dist",geom1, geom2, NULL);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(tolerance >= mindist);
}

/*
 *  Maximum 2d distance between linestrings.
 *  Returns NULL if given geoms are not linestrings.
 *  This is very bogus (or I'm missing its meaning)
 */
PG_FUNCTION_INFO_V1(LWGEOM_maxdistance2d_linestring);
Datum LWGEOM_maxdistance2d_linestring(PG_FUNCTION_ARGS)
{

	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	LWLINE *line1;
	LWLINE *line2;
	double maxdist = 0;
	int i;

	elog(ERROR, "This function is unimplemented yet");
	PG_RETURN_NULL();

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	line1 = lwline_deserialize(SERIALIZED_FORM(geom1));
	if ( line1 == NULL ) PG_RETURN_NULL(); /* not a linestring */

	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	line2 = lwline_deserialize(SERIALIZED_FORM(geom2));
	if ( line2 == NULL ) PG_RETURN_NULL(); /* not a linestring */

	if (pglwgeom_getSRID(geom1) != pglwgeom_getSRID(geom2))
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}

	for (i=0; i<line1->points->npoints; i++)
	{
		POINT2D p;
		double dist;

		getPoint2d_p(line1->points, i, &p);
		dist = distance2d_pt_ptarray(&p, line2->points);

		if (dist > maxdist) maxdist = dist;
	}

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_FLOAT8(maxdist);
}

/**
 * @brief Longitude shift:
 *  	Y remains the same
 *  	X is converted:
 *	 		from -180..180 to 0..360
 *	 		from 0..360 to -180..180
 */
PG_FUNCTION_INFO_V1(LWGEOM_longitude_shift);
Datum LWGEOM_longitude_shift(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	LWGEOM *lwgeom;
	PG_LWGEOM *ret;

	POSTGIS_DEBUG(2, "LWGEOM_longitude_shift called.");

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));
	lwgeom = pglwgeom_deserialize(geom);

	/* Drop bbox, will be recomputed */
	lwgeom_drop_bbox(lwgeom);

	/* Modify geometry */
	lwgeom_longitude_shift(lwgeom);

	/* Construct PG_LWGEOM */
	ret = pglwgeom_serialize(lwgeom);

	/* Release deserialized geometry */
	lwgeom_release(lwgeom);

	/* Release detoasted geometry */
	pfree(geom);

	PG_RETURN_POINTER(ret);
}

PG_FUNCTION_INFO_V1(LWGEOM_inside_circle_point);
Datum LWGEOM_inside_circle_point(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	double cx = PG_GETARG_FLOAT8(1);
	double cy = PG_GETARG_FLOAT8(2);
	double rr = PG_GETARG_FLOAT8(3);
	LWPOINT *point;
	POINT2D pt;

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	point = lwpoint_deserialize(SERIALIZED_FORM(geom));
	if ( point == NULL )
	{
		PG_FREE_IF_COPY(geom, 0);
		PG_RETURN_NULL(); /* not a point */
	}

	getPoint2d_p(point->point, 0, &pt);

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_BOOL(lwgeom_pt_inside_circle(&pt, cx, cy, rr));
}

/**
 *  @brief collect( geom, geom ) returns a geometry which contains
 *  		all the sub_objects from both of the argument geometries
 *  @return geometry is the simplest possible, based on the types
 *  	of the collected objects
 *  	ie. if all are of either X or multiX, then a multiX is returned.
 */
PG_FUNCTION_INFO_V1(LWGEOM_collect);
Datum LWGEOM_collect(PG_FUNCTION_ARGS)
{
	Pointer geom1_ptr = PG_GETARG_POINTER(0);
	Pointer geom2_ptr =  PG_GETARG_POINTER(1);
	PG_LWGEOM *pglwgeom1, *pglwgeom2, *result;
	LWGEOM *lwgeoms[2], *outlwg;
	unsigned int type1, type2, outtype;
	BOX2DFLOAT4 *box=NULL;
	int SRID;

	POSTGIS_DEBUG(2, "LWGEOM_collect called.");

	/* return null if both geoms are null */
	if ( (geom1_ptr == NULL) && (geom2_ptr == NULL) )
	{
		PG_RETURN_NULL();
	}

	/* return a copy of the second geom if only first geom is null */
	if (geom1_ptr == NULL)
	{
		result = (PG_LWGEOM *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(1));
		PG_RETURN_POINTER(result);
	}

	/* return a copy of the first geom if only second geom is null */
	if (geom2_ptr == NULL)
	{
		result = (PG_LWGEOM *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));
		PG_RETURN_POINTER(result);
	}


	pglwgeom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	pglwgeom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	POSTGIS_DEBUGF(3, "LWGEOM_collect(%s, %s): call", lwgeom_typename(TYPE_GETTYPE(pglwgeom1->type)), lwgeom_typename(TYPE_GETTYPE(pglwgeom2->type)));

#if 0
	if ( pglwgeom_getSRID(pglwgeom1) != pglwgeom_getSRID(pglwgeom2) )
	{
		elog(ERROR, "Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}
#endif

	SRID = pglwgeom_getSRID(pglwgeom1);
	errorIfSRIDMismatch(SRID, pglwgeom_getSRID(pglwgeom2));

	lwgeoms[0] = lwgeom_deserialize(SERIALIZED_FORM(pglwgeom1));
	lwgeoms[1] = lwgeom_deserialize(SERIALIZED_FORM(pglwgeom2));

	type1 = TYPE_GETTYPE(lwgeoms[0]->type);
	type2 = TYPE_GETTYPE(lwgeoms[1]->type);
	if ( type1 == type2 && type1 < 4 ) outtype = type1+3;
	else outtype = COLLECTIONTYPE;

	POSTGIS_DEBUGF(3, " outtype = %d", outtype);

	/* COMPUTE_BBOX WHEN_SIMPLE */
	if ( lwgeoms[0]->bbox && lwgeoms[1]->bbox )
	{
		box = palloc(sizeof(BOX2DFLOAT4));
		box->xmin = LW_MIN(lwgeoms[0]->bbox->xmin, lwgeoms[1]->bbox->xmin);
		box->ymin = LW_MIN(lwgeoms[0]->bbox->ymin, lwgeoms[1]->bbox->ymin);
		box->xmax = LW_MAX(lwgeoms[0]->bbox->xmax, lwgeoms[1]->bbox->xmax);
		box->ymax = LW_MAX(lwgeoms[0]->bbox->ymax, lwgeoms[1]->bbox->ymax);
	}

	/* Drop input geometries bbox and SRID */
	lwgeom_drop_bbox(lwgeoms[0]);
	lwgeom_dropSRID(lwgeoms[0]);
	lwgeom_drop_bbox(lwgeoms[1]);
	lwgeom_dropSRID(lwgeoms[1]);

	outlwg = (LWGEOM *)lwcollection_construct(
	             outtype, SRID,
	             box, 2, lwgeoms);

	result = pglwgeom_serialize(outlwg);

	PG_FREE_IF_COPY(pglwgeom1, 0);
	PG_FREE_IF_COPY(pglwgeom2, 1);
	lwgeom_release(lwgeoms[0]);
	lwgeom_release(lwgeoms[1]);

	PG_RETURN_POINTER(result);
}

/**
 * @brief This is a geometry array constructor
 * 		for use as aggregates sfunc.
 * 		Will have as input an array of Geometry pointers and a Geometry.
 * 		Will DETOAST given geometry and put a pointer to it
 * 			in the given array. DETOASTED value is first copied
 * 		to a safe memory context to avoid premature deletion.
 */
PG_FUNCTION_INFO_V1(LWGEOM_accum);
Datum LWGEOM_accum(PG_FUNCTION_ARGS)
{
	ArrayType *array = NULL;
	int nelems;
	int lbs=1;
	size_t nbytes, oldsize;
	Datum datum;
	PG_LWGEOM *geom;
	ArrayType *result;
	Oid oid = get_fn_expr_argtype(fcinfo->flinfo, 1);

	POSTGIS_DEBUG(2, "LWGEOM_accum called");

	datum = PG_GETARG_DATUM(0);
	if ( (Pointer *)datum == NULL )
	{
		array = NULL;
		nelems = 0;

		POSTGIS_DEBUG(3, "geom_accum: NULL array");
	}
	else
	{
		array = DatumGetArrayTypePCopy(datum);
		/*array = PG_GETARG_ARRAYTYPE_P(0); */
		nelems = ArrayGetNItems(ARR_NDIM(array), ARR_DIMS(array));

		POSTGIS_DEBUGF(3, "geom_accum: array of nelems=%d", nelems);
	}

	datum = PG_GETARG_DATUM(1);
	/* Do nothing, return state array */
	if ( (Pointer *)datum == NULL )
	{
		POSTGIS_DEBUGF(3, "geom_accum: NULL geom, nelems=%d", nelems);

		if ( array == NULL ) PG_RETURN_NULL();
		PG_RETURN_ARRAYTYPE_P(array);
	}

	/* Make a DETOASTED copy of input geometry */
	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(datum);

	POSTGIS_DEBUGF(3, "geom_accum: detoasted geom: %p", (void*)geom);

	/*
	 * Might use a more optimized version instead of lwrealloc'ing
	 * at every iteration. This is not the bottleneck anyway.
	 * 		--strk(TODO);
	 */
	++nelems;
	if ( nelems == 1 || ! array )
	{
		nbytes = ARR_OVERHEAD_NONULLS(1)+INTALIGN(VARSIZE(geom));

		POSTGIS_DEBUGF(3, "geom_accum: adding %p (nelems=%d; nbytes=%d)",
		               (void*)geom, nelems, (int)nbytes);

		result = lwalloc(nbytes);
		if ( ! result )
		{
			elog(ERROR, "Out of virtual memory");
			PG_RETURN_NULL();
		}

		SET_VARSIZE(result, nbytes);
		result->ndim = 1;
		result->elemtype = oid;

#if POSTGIS_PGSQL_VERSION > 81
		result->dataoffset = 0;
#endif
		memcpy(ARR_DIMS(result), &nelems, sizeof(int));
		memcpy(ARR_LBOUND(result), &lbs, sizeof(int));
		memcpy(ARR_DATA_PTR(result), geom, VARSIZE(geom));

		POSTGIS_DEBUGF(3, " %d bytes memcopied", VARSIZE(geom));

	}
	else
	{
		oldsize = VARSIZE(array);
		nbytes = oldsize + INTALIGN(VARSIZE(geom));

		POSTGIS_DEBUGF(3, "geom_accum: old array size: %d, adding %ld bytes (nelems=%d; nbytes=%u)", ARR_SIZE(array), INTALIGN(VARSIZE(geom)), nelems, (int)nbytes);

		result = (ArrayType *) lwrealloc(array, nbytes);
		if ( ! result )
		{
			elog(ERROR, "Out of virtual memory");
			PG_RETURN_NULL();
		}

		POSTGIS_DEBUGF(3, " %d bytes allocated for array", (int)nbytes);

		POSTGIS_DEBUGF(3, " array start  @ %p", (void*)result);
		POSTGIS_DEBUGF(3, " ARR_DATA_PTR @ %p (%ld)",
		               ARR_DATA_PTR(result), (uchar *)ARR_DATA_PTR(result)-(uchar *)result);
		POSTGIS_DEBUGF(3, " next element @ %p", (uchar *)result+oldsize);

		SET_VARSIZE(result, nbytes);
		memcpy(ARR_DIMS(result), &nelems, sizeof(int));

		POSTGIS_DEBUGF(3, " writing next element starting @ %p",
		               (void*)(result+oldsize));

		memcpy((uchar *)result+oldsize, geom, VARSIZE(geom));
	}

	POSTGIS_DEBUG(3, " returning");

	PG_RETURN_ARRAYTYPE_P(result);

}

/**
 * @brief collect_garray ( GEOMETRY[] ) returns a geometry which contains
 * 		all the sub_objects from all of the geometries in given array.
 *
 * @return geometry is the simplest possible, based on the types
 * 		of the collected objects
 * 		ie. if all are of either X or multiX, then a multiX is returned
 * 		bboxonly types are treated as null geometries (no sub_objects)
 */
PG_FUNCTION_INFO_V1(LWGEOM_collect_garray);
Datum LWGEOM_collect_garray(PG_FUNCTION_ARGS)
{
	Datum datum;
	ArrayType *array;
	int nelems;
	/*PG_LWGEOM **geoms; */
	PG_LWGEOM *result=NULL;
	LWGEOM **lwgeoms, *outlwg;
	unsigned int outtype;
	int i, count;
	int SRID=-1;
	size_t offset;
	BOX2DFLOAT4 *box=NULL;
	bits8 *bitmap; 
	int bitmask;

	POSTGIS_DEBUG(2, "LWGEOM_collect_garray called.");

	/* Get input datum */
	datum = PG_GETARG_DATUM(0);

	/* Return null on null input */
	if ( (Pointer *)datum == NULL )
	{
		elog(NOTICE, "NULL input");
		PG_RETURN_NULL();
	}

	/* Get actual ArrayType */
	array = DatumGetArrayTypeP(datum);

	POSTGIS_DEBUGF(3, " array is %d-bytes in size, %ld w/out header",
	               ARR_SIZE(array), ARR_SIZE(array)-ARR_OVERHEAD_NONULLS(ARR_NDIM(array)));


	/* Get number of geometries in array */
	nelems = ArrayGetNItems(ARR_NDIM(array), ARR_DIMS(array));

	POSTGIS_DEBUGF(3, "LWGEOM_collect_garray: array has %d elements", nelems);

	/* Return null on 0-elements input array */
	if ( nelems == 0 )
	{
		elog(NOTICE, "0 elements input array");
		PG_RETURN_NULL();
	}

	/*
	 * Deserialize all geometries in array into the lwgeoms pointers
	 * array. Check input types to form output type.
	 */
	lwgeoms = palloc(sizeof(LWGEOM *)*nelems);
	count = 0;
	outtype = 0;
	offset = 0;
	bitmap = ARR_NULLBITMAP(array);
	bitmask = 1;
	for (i=0; i<nelems; i++)
	{
		/* Don't do anything for NULL values */ 
 		if ((bitmap && (*bitmap & bitmask) != 0) || !bitmap)
		{
			PG_LWGEOM *geom = (PG_LWGEOM *)(ARR_DATA_PTR(array)+offset);
			unsigned int intype = TYPE_GETTYPE(geom->type);
	
			offset += INTALIGN(VARSIZE(geom));
	
			lwgeoms[count] = lwgeom_deserialize(SERIALIZED_FORM(geom));
	
			POSTGIS_DEBUGF(3, "LWGEOM_collect_garray: geom %d deserialized", i);
	
			if ( ! count )
			{
				/* Get first geometry SRID */
				SRID = lwgeoms[count]->SRID;
	
				/* COMPUTE_BBOX WHEN_SIMPLE */
				if ( lwgeoms[count]->bbox )
				{
					box = box2d_clone(lwgeoms[count]->bbox);
				}
			}
			else
			{
				/* Check SRID homogeneity */
				if ( lwgeoms[count]->SRID != SRID )
				{
					elog(ERROR,
					"Operation on mixed SRID geometries");
					PG_RETURN_NULL();
				}
	
				/* COMPUTE_BBOX WHEN_SIMPLE */
				if ( box )
				{
					if ( lwgeoms[count]->bbox )
					{
						box->xmin = LW_MIN(box->xmin, lwgeoms[count]->bbox->xmin);
						box->ymin = LW_MIN(box->ymin, lwgeoms[count]->bbox->ymin);
						box->xmax = LW_MAX(box->xmax, lwgeoms[count]->bbox->xmax);
						box->ymax = LW_MAX(box->ymax, lwgeoms[count]->bbox->ymax);
					}
					else
					{
						pfree(box);
						box = NULL;
					}
				}
			}

			lwgeom_dropSRID(lwgeoms[count]);
			lwgeom_drop_bbox(lwgeoms[count]);
	
			/* Output type not initialized */
			if ( ! outtype )
			{
				/* Input is single, make multi */
				if ( intype < 4 ) outtype = intype+3;
				/* Input is multi, make collection */
				else outtype = COLLECTIONTYPE;
			}
	
			/* Input type not compatible with output */
			/* make output type a collection */
			else if ( outtype != COLLECTIONTYPE && intype != outtype-3 )
			{
				outtype = COLLECTIONTYPE;
			}

			/* Advance NULL bitmap */
			if (bitmap)
			{
				bitmask <<= 1;
				if (bitmask == 0x100)
				{
					bitmap++;
					bitmask = 1;
				}
			}

			count++;
		}
	}

	POSTGIS_DEBUGF(3, "LWGEOM_collect_garray: outtype = %d", outtype);

	/* If we have been passed a complete set of NULLs then return NULL */
	if (!outtype)
	{
		PG_RETURN_NULL();
	}
	else
	{
		outlwg = (LWGEOM *)lwcollection_construct(
			outtype, SRID,
			box, count, lwgeoms);
	
		result = pglwgeom_serialize(outlwg);
	
		PG_RETURN_POINTER(result);
	}
}

/**
 * LineFromMultiPoint ( GEOMETRY ) returns a LINE formed by
 * 		all the points in the in given multipoint.
 */
PG_FUNCTION_INFO_V1(LWGEOM_line_from_mpoint);
Datum LWGEOM_line_from_mpoint(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *ingeom, *result;
	LWLINE *lwline;
	LWMPOINT *mpoint;

	POSTGIS_DEBUG(2, "LWGEOM_line_from_mpoint called");

	/* Get input PG_LWGEOM and deserialize it */
	ingeom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	if ( TYPE_GETTYPE(ingeom->type) != MULTIPOINTTYPE )
	{
		elog(ERROR, "makeline: input must be a multipoint");
		PG_RETURN_NULL(); /* input is not a multipoint */
	}

	mpoint = lwmpoint_deserialize(SERIALIZED_FORM(ingeom));
	lwline = lwline_from_lwmpoint(mpoint->SRID, mpoint);
	if ( ! lwline )
	{
		PG_FREE_IF_COPY(ingeom, 0);
		elog(ERROR, "makeline: lwline_from_lwmpoint returned NULL");
		PG_RETURN_NULL();
	}

	result = pglwgeom_serialize((LWGEOM *)lwline);

	PG_FREE_IF_COPY(ingeom, 0);
	lwgeom_release((LWGEOM *)lwline);

	PG_RETURN_POINTER(result);
}

/**
 * @brief makeline_garray ( GEOMETRY[] ) returns a LINE formed by
 * 		all the point geometries in given array.
 * 		array elements that are NOT points are discarded..
 */
PG_FUNCTION_INFO_V1(LWGEOM_makeline_garray);
Datum LWGEOM_makeline_garray(PG_FUNCTION_ARGS)
{
	Datum datum;
	ArrayType *array;
	int nelems;
	PG_LWGEOM *result=NULL;
	LWPOINT **lwpoints;
	LWGEOM *outlwg;
	unsigned int npoints;
	int i;
	size_t offset;
	int SRID=-1;

	bits8 *bitmap;
	int bitmask;

	POSTGIS_DEBUG(2, "LWGEOM_makeline_garray called.");

	/* Get input datum */
	datum = PG_GETARG_DATUM(0);

	/* Return null on null input */
	if ( (Pointer *)datum == NULL )
	{
		elog(NOTICE, "NULL input");
		PG_RETURN_NULL();
	}

	/* Get actual ArrayType */
	array = DatumGetArrayTypeP(datum);

	POSTGIS_DEBUG(3, "LWGEOM_makeline_garray: array detoasted");

	/* Get number of geometries in array */
	nelems = ArrayGetNItems(ARR_NDIM(array), ARR_DIMS(array));

	POSTGIS_DEBUGF(3, "LWGEOM_makeline_garray: array has %d elements", nelems);

	/* Return null on 0-elements input array */
	if ( nelems == 0 )
	{
		elog(NOTICE, "0 elements input array");
		PG_RETURN_NULL();
	}

	/*
	 * Deserialize all point geometries in array into the
	 * lwpoints pointers array.
	 * Count actual number of points.
	 */

	/* possibly more then required */
	lwpoints = palloc(sizeof(LWGEOM *)*nelems);
	npoints = 0;
	offset = 0;
	bitmap = ARR_NULLBITMAP(array);
	bitmask = 1;
	for (i=0; i<nelems; i++)
	{
		/* Don't do anything for NULL values */  
		if ((bitmap && (*bitmap & bitmask) != 0) || !bitmap) 
 		{ 
			PG_LWGEOM *geom = (PG_LWGEOM *)(ARR_DATA_PTR(array)+offset);
			offset += INTALIGN(VARSIZE(geom));
	
			if ( TYPE_GETTYPE(geom->type) != POINTTYPE ) continue;
	
			lwpoints[npoints++] =
			lwpoint_deserialize(SERIALIZED_FORM(geom));
	
			/* Check SRID homogeneity */
			if ( npoints == 1 )
			{
				/* Get first geometry SRID */
				SRID = lwpoints[npoints-1]->SRID;
			}
			else
			{
				if ( lwpoints[npoints-1]->SRID != SRID )
				{
					elog(ERROR,
					"Operation on mixed SRID geometries");
					PG_RETURN_NULL();
				}
			}
	
			POSTGIS_DEBUGF(3, "LWGEOM_makeline_garray: element %d deserialized",
				i);
		}

		/* Advance NULL bitmap */
		if (bitmap)
		{
			bitmask <<= 1;
			if (bitmask == 0x100)
			{
				bitmap++;
				bitmask = 1;
			}
		}
	}

	/* Return null on 0-points input array */
	if ( npoints == 0 )
	{
		elog(NOTICE, "No points in input array");
		PG_RETURN_NULL();
	}

	POSTGIS_DEBUGF(3, "LWGEOM_makeline_garray: point elements: %d", npoints);

	outlwg = (LWGEOM *)lwline_from_lwpointarray(SRID, npoints, lwpoints);

	result = pglwgeom_serialize(outlwg);

	PG_RETURN_POINTER(result);
}

/**
 * makeline ( GEOMETRY, GEOMETRY ) returns a LINESTRIN segment
 * formed by the given point geometries.
 */
PG_FUNCTION_INFO_V1(LWGEOM_makeline);
Datum LWGEOM_makeline(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *pglwg1, *pglwg2;
	PG_LWGEOM *result=NULL;
	LWPOINT *lwpoints[2];
	LWLINE *outline;

	POSTGIS_DEBUG(2, "LWGEOM_makeline called.");

	/* Get input datum */
	pglwg1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	pglwg2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	if ( ! TYPE_GETTYPE(pglwg1->type) == POINTTYPE ||
	        ! TYPE_GETTYPE(pglwg2->type) == POINTTYPE )
	{
		elog(ERROR, "Input geometries must be points");
		PG_RETURN_NULL();
	}

	errorIfSRIDMismatch(pglwgeom_getSRID(pglwg1), pglwgeom_getSRID(pglwg2));

	lwpoints[0] = lwpoint_deserialize(SERIALIZED_FORM(pglwg1));
	lwpoints[1] = lwpoint_deserialize(SERIALIZED_FORM(pglwg2));

	outline = lwline_from_lwpointarray(lwpoints[0]->SRID, 2, lwpoints);

	result = pglwgeom_serialize((LWGEOM *)outline);

	PG_FREE_IF_COPY(pglwg1, 0);
	PG_FREE_IF_COPY(pglwg2, 1);
	lwgeom_release((LWGEOM *)lwpoints[0]);
	lwgeom_release((LWGEOM *)lwpoints[1]);

	PG_RETURN_POINTER(result);
}

/**
 * makepoly( GEOMETRY, GEOMETRY[] ) returns a POLYGON
 * 		formed by the given shell and holes geometries.
 */
PG_FUNCTION_INFO_V1(LWGEOM_makepoly);
Datum LWGEOM_makepoly(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *pglwg1;
	ArrayType *array=NULL;
	PG_LWGEOM *result=NULL;
	const LWLINE *shell=NULL;
	const LWLINE **holes=NULL;
	LWPOLY *outpoly;
	unsigned int nholes=0;
	unsigned int i;
	size_t offset=0;

	POSTGIS_DEBUG(2, "LWGEOM_makepoly called.");

	/* Get input shell */
	pglwg1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	if ( ! TYPE_GETTYPE(pglwg1->type) == LINETYPE )
	{
		lwerror("Shell is not a line");
	}
	shell = lwline_deserialize(SERIALIZED_FORM(pglwg1));

	/* Get input holes if any */
	if ( PG_NARGS() > 1 )
	{
		array = PG_GETARG_ARRAYTYPE_P(1);
		nholes = ArrayGetNItems(ARR_NDIM(array), ARR_DIMS(array));
		holes = lwalloc(sizeof(LWLINE *)*nholes);
		for (i=0; i<nholes; i++)
		{
			PG_LWGEOM *g = (PG_LWGEOM *)(ARR_DATA_PTR(array)+offset);
			LWLINE *hole;
			offset += INTALIGN(VARSIZE(g));
			if ( TYPE_GETTYPE(g->type) != LINETYPE )
			{
				lwerror("Hole %d is not a line", i);
			}
			hole = lwline_deserialize(SERIALIZED_FORM(g));
			holes[i] = hole;
		}
	}

	outpoly = lwpoly_from_lwlines(shell, nholes, holes);

	POSTGIS_DEBUGF(3, "%s", lwgeom_summary((LWGEOM*)outpoly, 0));

	result = pglwgeom_serialize((LWGEOM *)outpoly);

	PG_FREE_IF_COPY(pglwg1, 0);
	lwgeom_release((LWGEOM *)shell);
	for (i=0; i<nholes; i++) lwgeom_release((LWGEOM *)holes[i]);

	PG_RETURN_POINTER(result);
}

/**
 *  makes a polygon of the expanded features bvol - 1st point = LL 3rd=UR
 *  2d only. (3d might be worth adding).
 *  create new geometry of type polygon, 1 ring, 5 points
 */
PG_FUNCTION_INFO_V1(LWGEOM_expand);
Datum LWGEOM_expand(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	double d = PG_GETARG_FLOAT8(1);
	BOX2DFLOAT4 box;
	POINT2D *pts = lwalloc(sizeof(POINT2D)*5);
	POINTARRAY *pa[1];
	LWPOLY *poly;
	int SRID;
	PG_LWGEOM *result;

	POSTGIS_DEBUG(2, "LWGEOM_expand called.");

	/* get geometry box  */
	if ( ! getbox2d_p(SERIALIZED_FORM(geom), &box) )
	{
		/* must be an EMPTY geometry */
		PG_RETURN_POINTER(geom);
	}

	/* get geometry SRID */
	SRID = lwgeom_getsrid(SERIALIZED_FORM(geom));

	/* expand it */
	expand_box2d(&box, d);

	/* Assign coordinates to POINT2D array */
	pts[0].x = box.xmin;
	pts[0].y = box.ymin;
	pts[1].x = box.xmin;
	pts[1].y = box.ymax;
	pts[2].x = box.xmax;
	pts[2].y = box.ymax;
	pts[3].x = box.xmax;
	pts[3].y = box.ymin;
	pts[4].x = box.xmin;
	pts[4].y = box.ymin;

	/* Construct point array */
	pa[0] = lwalloc(sizeof(POINTARRAY));
	pa[0]->serialized_pointlist = (uchar *)pts;
	TYPE_SETZM(pa[0]->dims, 0, 0);
	pa[0]->npoints = 5;

	/* Construct polygon  */
	poly = lwpoly_construct(SRID, box2d_clone(&box), 1, pa);

	/* Construct PG_LWGEOM  */
	result = pglwgeom_serialize((LWGEOM *)poly);

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(result);
}

/** Convert geometry to BOX (internal postgres type) */
PG_FUNCTION_INFO_V1(LWGEOM_to_BOX);
Datum LWGEOM_to_BOX(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *pg_lwgeom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	BOX3D *box3d;
	BOX *result = (BOX *)lwalloc(sizeof(BOX));
	LWGEOM *lwgeom = lwgeom_deserialize(SERIALIZED_FORM(pg_lwgeom));

	/* Calculate the BOX3D of the geometry */
	box3d = lwgeom_compute_box3d(lwgeom);
	box3d_to_box_p(box3d, result);
	lwfree(box3d);
	lwfree(lwgeom);

	PG_FREE_IF_COPY(pg_lwgeom, 0);

	PG_RETURN_POINTER(result);
}

/**
 *  makes a polygon of the features bvol - 1st point = LL 3rd=UR
 *  2d only. (3d might be worth adding).
 *  create new geometry of type polygon, 1 ring, 5 points
 */
PG_FUNCTION_INFO_V1(LWGEOM_envelope);
Datum LWGEOM_envelope(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	BOX3D box;
	int SRID;
	POINTARRAY *pa;
	PG_LWGEOM *result;
	uchar *ser = NULL;


	/* get bounding box  */
	if ( ! compute_serialized_box3d_p(SERIALIZED_FORM(geom), &box) )
	{
		/* must be the EMPTY geometry */
		PG_RETURN_POINTER(geom);
	}

	/* get geometry SRID */
	SRID = lwgeom_getsrid(SERIALIZED_FORM(geom));


	/*
	 * Alter envelope type so that a valid geometry is always
	 * returned depending upon the size of the geometry. The
	 * code makes the following assumptions:
	 *     - If the bounding box is a single point then return a
	 *     POINT geometry
	 *     - If the bounding box represents either a horizontal or
	 *     vertical line, return a LINESTRING geometry
	 *     - Otherwise return a POLYGON
	 */


	if (box.xmin == box.xmax &&
	        box.ymin == box.ymax)
	{
		/* Construct and serialize point */
		LWPOINT *point = make_lwpoint2d(SRID, box.xmin, box.ymin);
		ser = lwpoint_serialize(point);
	}
	else if (box.xmin == box.xmax ||
	         box.ymin == box.ymax)
	{
		LWLINE *line;
		POINT2D *pts = palloc(sizeof(POINT2D)*2);

		/* Assign coordinates to POINT2D array */
		pts[0].x = box.xmin;
		pts[0].y = box.ymin;
		pts[1].x = box.xmax;
		pts[1].y = box.ymax;

		/* Construct point array */
		pa = pointArray_construct((uchar *)pts, 0, 0, 2);

		/* Construct and serialize linestring */
		line = lwline_construct(SRID, NULL, pa);
		ser = lwline_serialize(line);
	}
	else
	{
		LWPOLY *poly;
		POINT2D *pts = lwalloc(sizeof(POINT2D)*5);
		BOX2DFLOAT4 box2d;
		getbox2d_p(SERIALIZED_FORM(geom), &box2d);

		/* Assign coordinates to POINT2D array */
		pts[0].x = box2d.xmin;
		pts[0].y = box2d.ymin;
		pts[1].x = box2d.xmin;
		pts[1].y = box2d.ymax;
		pts[2].x = box2d.xmax;
		pts[2].y = box2d.ymax;
		pts[3].x = box2d.xmax;
		pts[3].y = box2d.ymin;
		pts[4].x = box2d.xmin;
		pts[4].y = box2d.ymin;

		/* Construct point array */
		pa = pointArray_construct((uchar *)pts, 0, 0, 5);

		/* Construct polygon  */
		poly = lwpoly_construct(SRID, box2d_clone(&box2d), 1, &pa);

		/* Serialize polygon */
		ser = lwpoly_serialize(poly);
	}

	PG_FREE_IF_COPY(geom, 0);

	/* Construct PG_LWGEOM  */
	result = PG_LWGEOM_construct(ser, SRID, 1);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(LWGEOM_isempty);
Datum LWGEOM_isempty(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	if ( lwgeom_getnumgeometries(SERIALIZED_FORM(geom)) == 0 )
	{
		PG_FREE_IF_COPY(geom, 0);
		PG_RETURN_BOOL(TRUE);
	}
	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_BOOL(FALSE);
}


/**
 *  @brief Returns a modified geometry so that no segment is
 *  	longer then the given distance (computed using 2d).
 *  	Every input point is kept.
 *  	Z and M values for added points (if needed) are set to 0.
 */
PG_FUNCTION_INFO_V1(LWGEOM_segmentize2d);
Datum LWGEOM_segmentize2d(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *outgeom, *ingeom;
	double dist;
	LWGEOM *inlwgeom, *outlwgeom;

	POSTGIS_DEBUG(2, "LWGEOM_segmentize2d called");

	ingeom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	dist = PG_GETARG_FLOAT8(1);

	/* Avoid deserialize/serialize steps */
	if ( (TYPE_GETTYPE(ingeom->type) == POINTTYPE) ||
	        (TYPE_GETTYPE(ingeom->type) == MULTIPOINTTYPE) )
		PG_RETURN_POINTER(ingeom);

	inlwgeom = lwgeom_deserialize(SERIALIZED_FORM(ingeom));
	outlwgeom = lwgeom_segmentize2d(inlwgeom, dist);

	/* Copy input bounding box if any */
	if ( inlwgeom->bbox )
		outlwgeom->bbox = box2d_clone(inlwgeom->bbox);

	outgeom = pglwgeom_serialize(outlwgeom);

	PG_FREE_IF_COPY(ingeom, 0);
	lwgeom_release(outlwgeom);
	lwgeom_release(inlwgeom);

	PG_RETURN_POINTER(outgeom);
}

/** Reverse vertex order of geometry */
PG_FUNCTION_INFO_V1(LWGEOM_reverse);
Datum LWGEOM_reverse(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	LWGEOM *lwgeom;

	POSTGIS_DEBUG(2, "LWGEOM_reverse called");

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));

	lwgeom = lwgeom_deserialize(SERIALIZED_FORM(geom));
	lwgeom_reverse(lwgeom);

	geom = pglwgeom_serialize(lwgeom);

	PG_RETURN_POINTER(geom);
}

/** Force polygons of the collection to obey Right-Hand-Rule */
PG_FUNCTION_INFO_V1(LWGEOM_forceRHR_poly);
Datum LWGEOM_forceRHR_poly(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *ingeom, *outgeom;
	LWGEOM *lwgeom;

	POSTGIS_DEBUG(2, "LWGEOM_forceRHR_poly called");

	ingeom = (PG_LWGEOM *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));

	lwgeom = lwgeom_deserialize(SERIALIZED_FORM(ingeom));
	lwgeom_force_rhr(lwgeom);

	outgeom = pglwgeom_serialize(lwgeom);

	PG_FREE_IF_COPY(ingeom, 0);
	lwgeom_release(lwgeom);

	PG_RETURN_POINTER(outgeom);
}

/** Test deserialize/serialize operations */
PG_FUNCTION_INFO_V1(LWGEOM_noop);
Datum LWGEOM_noop(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *in, *out;
	LWGEOM *lwgeom;

	POSTGIS_DEBUG(2, "LWGEOM_noop called");

	in = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	lwgeom = lwgeom_deserialize(SERIALIZED_FORM(in));

	POSTGIS_DEBUGF(3, "Deserialized: %s", lwgeom_summary(lwgeom, 0));

	out = pglwgeom_serialize(lwgeom);

	PG_FREE_IF_COPY(in, 0);
	lwgeom_release(lwgeom);

	PG_RETURN_POINTER(out);
}

/**
 *  @return:
 *   0==2d
 *   1==3dm
 *   2==3dz
 *   3==4d
 */
PG_FUNCTION_INFO_V1(LWGEOM_zmflag);
Datum LWGEOM_zmflag(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *in;
	uchar type;
	int ret = 0;

	in = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	type = in->type;
	if ( TYPE_HASZ(type) ) ret += 2;
	if ( TYPE_HASM(type) ) ret += 1;
	PG_FREE_IF_COPY(in, 0);
	PG_RETURN_INT16(ret);
}

PG_FUNCTION_INFO_V1(LWGEOM_hasz);
Datum LWGEOM_hasz(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *in = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_RETURN_BOOL(TYPE_HASZ(in->type));
}

PG_FUNCTION_INFO_V1(LWGEOM_hasm);
Datum LWGEOM_hasm(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *in = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_RETURN_BOOL(TYPE_HASM(in->type));
}


PG_FUNCTION_INFO_V1(LWGEOM_hasBBOX);
Datum LWGEOM_hasBBOX(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *in;
	char res;

	in = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	res=lwgeom_hasBBOX(in->type);
	PG_FREE_IF_COPY(in, 0);

	PG_RETURN_BOOL(res);
}

/** Return: 2,3 or 4 */
PG_FUNCTION_INFO_V1(LWGEOM_ndims);
Datum LWGEOM_ndims(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *in;
	int ret;

	in = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	ret = (TYPE_NDIMS(in->type));
	PG_FREE_IF_COPY(in, 0);
	PG_RETURN_INT16(ret);
}

/** lwgeom_same(lwgeom1, lwgeom2) */
PG_FUNCTION_INFO_V1(LWGEOM_same);
Datum LWGEOM_same(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *g1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *g2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	LWGEOM *lwg1, *lwg2;
	bool result;

	if ( TYPE_GETTYPE(g1->type) != TYPE_GETTYPE(g2->type) )
	{
		PG_FREE_IF_COPY(g1, 0);
		PG_FREE_IF_COPY(g2, 1);
		PG_RETURN_BOOL(FALSE); /* different types */
	}

	if ( TYPE_GETZM(g1->type) != TYPE_GETZM(g2->type) )
	{
		PG_FREE_IF_COPY(g1, 0);
		PG_FREE_IF_COPY(g2, 1);
		PG_RETURN_BOOL(FALSE); /* different dimensions */
	}

	/* ok, deserialize. */
	lwg1 = lwgeom_deserialize(SERIALIZED_FORM(g1));
	lwg2 = lwgeom_deserialize(SERIALIZED_FORM(g2));

	/* invoke appropriate function */
	result = lwgeom_same(lwg1, lwg2);

	/* Relase memory */
	lwgeom_release(lwg1);
	lwgeom_release(lwg2);
	PG_FREE_IF_COPY(g1, 0);
	PG_FREE_IF_COPY(g2, 1);

	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(LWGEOM_makepoint);
Datum LWGEOM_makepoint(PG_FUNCTION_ARGS)
{
	double x,y,z,m;
	LWPOINT *point;
	PG_LWGEOM *result;

	POSTGIS_DEBUG(2, "LWGEOM_makepoint called");

	x = PG_GETARG_FLOAT8(0);
	y = PG_GETARG_FLOAT8(1);

	if ( PG_NARGS() == 2 ) point = make_lwpoint2d(-1, x, y);
	else if ( PG_NARGS() == 3 )
	{
		z = PG_GETARG_FLOAT8(2);
		point = make_lwpoint3dz(-1, x, y, z);
	}
	else if ( PG_NARGS() == 4 )
	{
		z = PG_GETARG_FLOAT8(2);
		m = PG_GETARG_FLOAT8(3);
		point = make_lwpoint4d(-1, x, y, z, m);
	}
	else
	{
		elog(ERROR, "LWGEOM_makepoint: unsupported number of args: %d",
		     PG_NARGS());
		PG_RETURN_NULL();
	}

	result = pglwgeom_serialize((LWGEOM *)point);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(LWGEOM_makepoint3dm);
Datum LWGEOM_makepoint3dm(PG_FUNCTION_ARGS)
{
	double x,y,m;
	LWPOINT *point;
	PG_LWGEOM *result;

	POSTGIS_DEBUG(2, "LWGEOM_makepoint3dm called.");

	x = PG_GETARG_FLOAT8(0);
	y = PG_GETARG_FLOAT8(1);
	m = PG_GETARG_FLOAT8(2);

	point = make_lwpoint3dm(-1, x, y, m);
	result = pglwgeom_serialize((LWGEOM *)point);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(LWGEOM_addpoint);
Datum LWGEOM_addpoint(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *pglwg1, *pglwg2, *result;
	LWPOINT *point;
	LWLINE *line, *outline;
	int where = -1;

	POSTGIS_DEBUG(2, "LWGEOM_addpoint called.");

	pglwg1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	pglwg2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	if ( PG_NARGS() > 2 )
	{
		where = PG_GETARG_INT32(2);
	}

	if ( ! TYPE_GETTYPE(pglwg1->type) == LINETYPE )
	{
		elog(ERROR, "First argument must be a LINESTRING");
		PG_RETURN_NULL();
	}

	if ( ! TYPE_GETTYPE(pglwg2->type) == POINTTYPE )
	{
		elog(ERROR, "Second argument must be a POINT");
		PG_RETURN_NULL();
	}

	line = lwline_deserialize(SERIALIZED_FORM(pglwg1));

	if ( where == -1 ) where = line->points->npoints;
	else if ( (unsigned int)where > line->points->npoints )
	{
		elog(ERROR, "Invalid offset");
		PG_RETURN_NULL();
	}

	point = lwpoint_deserialize(SERIALIZED_FORM(pglwg2));

	outline = lwline_addpoint(line, point, where);

	result = pglwgeom_serialize((LWGEOM *)outline);

	/* Release memory */
	PG_FREE_IF_COPY(pglwg1, 0);
	PG_FREE_IF_COPY(pglwg2, 1);
	lwgeom_release((LWGEOM *)point);
	lwgeom_release((LWGEOM *)line);
	lwgeom_release((LWGEOM *)outline);

	PG_RETURN_POINTER(result);

}

PG_FUNCTION_INFO_V1(LWGEOM_removepoint);
Datum LWGEOM_removepoint(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *pglwg1, *result;
	LWLINE *line, *outline;
	unsigned int which;

	POSTGIS_DEBUG(2, "LWGEOM_removepoint called.");

	pglwg1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	which = PG_GETARG_INT32(1);

	if ( ! TYPE_GETTYPE(pglwg1->type) == LINETYPE )
	{
		elog(ERROR, "First argument must be a LINESTRING");
		PG_RETURN_NULL();
	}

	line = lwline_deserialize(SERIALIZED_FORM(pglwg1));

	if ( which > line->points->npoints-1 )
	{
		elog(ERROR, "Point index out of range (%d..%d)", 0, line->points->npoints-1);
		PG_RETURN_NULL();
	}

	if ( line->points->npoints < 3 )
	{
		elog(ERROR, "Can't remove points from a single segment line");
		PG_RETURN_NULL();
	}

	outline = lwline_removepoint(line, which);

	result = pglwgeom_serialize((LWGEOM *)outline);

	/* Release memory */
	PG_FREE_IF_COPY(pglwg1, 0);
	lwgeom_release((LWGEOM *)line);
	lwgeom_release((LWGEOM *)outline);

	PG_RETURN_POINTER(result);

}

PG_FUNCTION_INFO_V1(LWGEOM_setpoint_linestring);
Datum LWGEOM_setpoint_linestring(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *pglwg1, *pglwg2, *result;
	LWGEOM *lwg;
	LWLINE *line;
	LWPOINT *lwpoint;
	POINT4D newpoint;
	unsigned int which;

	POSTGIS_DEBUG(2, "LWGEOM_setpoint_linestring called.");

	/* we copy input as we're going to modify it */
	pglwg1 = (PG_LWGEOM *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));

	which = PG_GETARG_INT32(1);
	pglwg2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(2));


	/* Extract a POINT4D from the point */
	lwg = pglwgeom_deserialize(pglwg2);
	lwpoint = lwgeom_as_lwpoint(lwg);
	if ( ! lwpoint )
	{
		elog(ERROR, "Third argument must be a POINT");
		PG_RETURN_NULL();
	}
	getPoint4d_p(lwpoint->point, 0, &newpoint);
	lwgeom_release((LWGEOM *)lwpoint);
	PG_FREE_IF_COPY(pglwg2, 2);

	lwg = pglwgeom_deserialize(pglwg1);
	line = lwgeom_as_lwline(lwg);
	if ( ! line )
	{
		elog(ERROR, "First argument must be a LINESTRING");
		PG_RETURN_NULL();
	}
	if ( which > line->points->npoints-1 )
	{
		elog(ERROR, "Point index out of range (%d..%d)", 0, line->points->npoints-1);
		PG_RETURN_NULL();
	}

	/*
	 * This will change pointarray of the serialized pglwg1,
	 */
	lwline_setPoint4d(line, which, &newpoint);
	result = pglwgeom_serialize((LWGEOM *)line);

	/* Release memory */
	pfree(pglwg1); /* we forced copy, POINARRAY is released now */
	lwgeom_release((LWGEOM *)line);

	PG_RETURN_POINTER(result);

}

/* convert LWGEOM to ewkt (in TEXT format) */
PG_FUNCTION_INFO_V1(LWGEOM_asEWKT);
Datum LWGEOM_asEWKT(PG_FUNCTION_ARGS)
{
	LWGEOM_UNPARSER_RESULT lwg_unparser_result;
	PG_LWGEOM *lwgeom;
	int len, result;
	char *lwgeom_result,*loc_wkt;
	/*char *semicolonLoc; */

	lwgeom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	result = serialized_lwgeom_to_ewkt(&lwg_unparser_result, SERIALIZED_FORM(lwgeom), PARSER_CHECK_ALL);
	if (result)
		PG_UNPARSER_ERROR(lwg_unparser_result);

#if 0
	semicolonLoc = strchr(lwg_unparser_result.wkb,';');

	/*loc points to start of wkt */
	if (semicolonLoc == NULL) loc_wkt = lwg_unparser_result.wkoutput;
	else loc_wkt = semicolonLoc +1;
#endif
	loc_wkt = lwg_unparser_result.wkoutput;

	len = strlen(loc_wkt);
	lwgeom_result = palloc(len + VARHDRSZ);
	SET_VARSIZE(lwgeom_result, len + VARHDRSZ);

	memcpy(VARDATA(lwgeom_result), loc_wkt, len);

	pfree(lwg_unparser_result.wkoutput);
	PG_FREE_IF_COPY(lwgeom, 0);

	PG_RETURN_POINTER(lwgeom_result);
}

/**
 * Compute the azimuth of segment defined by the two
 * given Point geometries.
 * @return NULL on exception (same point).
 * 		Return radians otherwise.
 */
PG_FUNCTION_INFO_V1(LWGEOM_azimuth);
Datum LWGEOM_azimuth(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	LWPOINT *lwpoint;
	POINT2D p1, p2;
	double result;
	int SRID;

	/* Extract first point */
	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	lwpoint = lwpoint_deserialize(SERIALIZED_FORM(geom));
	if ( ! lwpoint )
	{
		PG_FREE_IF_COPY(geom, 0);
		lwerror("Argument must be POINT geometries");
		PG_RETURN_NULL();
	}
	SRID = lwpoint->SRID;
	if ( ! getPoint2d_p(lwpoint->point, 0, &p1) )
	{
		PG_FREE_IF_COPY(geom, 0);
		lwerror("Error extracting point");
		PG_RETURN_NULL();
	}
	lwgeom_release((LWGEOM *)lwpoint);
	PG_FREE_IF_COPY(geom, 0);

	/* Extract second point */
	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	lwpoint = lwpoint_deserialize(SERIALIZED_FORM(geom));
	if ( ! lwpoint )
	{
		PG_FREE_IF_COPY(geom, 1);
		lwerror("Argument must be POINT geometries");
		PG_RETURN_NULL();
	}
	if ( lwpoint->SRID != SRID )
	{
		PG_FREE_IF_COPY(geom, 1);
		lwerror("Operation on mixed SRID geometries");
		PG_RETURN_NULL();
	}
	if ( ! getPoint2d_p(lwpoint->point, 0, &p2) )
	{
		PG_FREE_IF_COPY(geom, 1);
		lwerror("Error extracting point");
		PG_RETURN_NULL();
	}
	lwgeom_release((LWGEOM *)lwpoint);
	PG_FREE_IF_COPY(geom, 1);

	/* Compute azimuth */
	if ( ! azimuth_pt_pt(&p1, &p2, &result) )
	{
		PG_RETURN_NULL();
	}

	PG_RETURN_FLOAT8(result);
}





/*
 * optimistic_overlap(Polygon P1, Multipolygon MP2, double dist)
 * returns true if P1 overlaps MP2
 *   method: bbox check -
 *   is separation < dist?  no - return false (quick)
 *                          yes  - return distance(P1,MP2) < dist
 */
PG_FUNCTION_INFO_V1(optimistic_overlap);
Datum optimistic_overlap(PG_FUNCTION_ARGS)
{

	PG_LWGEOM                  *pg_geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM                  *pg_geom2 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	double                  dist = PG_GETARG_FLOAT8(2);
	BOX2DFLOAT4             g1_bvol;
	double                  calc_dist;

	LWGEOM                     *geom1;
	LWGEOM                     *geom2;


	/* deserialized PG_LEGEOM into their respective LWGEOM */
	geom1 = lwgeom_deserialize(SERIALIZED_FORM(pg_geom1));
	geom2 = lwgeom_deserialize(SERIALIZED_FORM(pg_geom2));

	if (geom1->SRID != geom2->SRID)
	{

		elog(ERROR,"optimistic_overlap:Operation on two GEOMETRIES with different SRIDs\\n");
		PG_RETURN_NULL();
	}

	if (TYPE_GETTYPE(geom1->type) != POLYGONTYPE)
	{
		elog(ERROR,"optimistic_overlap: first arg isnt a polygon\n");
		PG_RETURN_NULL();
	}

	if ( (TYPE_GETTYPE(geom2->type) != POLYGONTYPE) &&  (geom2->type != MULTIPOLYGONTYPE) )
	{
		elog(ERROR,"optimistic_overlap: 2nd arg isnt a [multi-]polygon\n");
		PG_RETURN_NULL();
	}

	/*bbox check */

	getbox2d_p( SERIALIZED_FORM(pg_geom1), &g1_bvol );


	g1_bvol.xmin = g1_bvol.xmin - dist;
	g1_bvol.ymin = g1_bvol.ymin - dist;
	g1_bvol.xmax = g1_bvol.xmax + dist;
	g1_bvol.ymax = g1_bvol.ymax + dist;

	if (  (g1_bvol.xmin > geom2->bbox->xmax) ||
	        (g1_bvol.xmax < geom2->bbox->xmin) ||
	        (g1_bvol.ymin > geom2->bbox->ymax) ||
	        (g1_bvol.ymax < geom2->bbox->ymin)
	   )
	{
		PG_RETURN_BOOL(FALSE);  /*bbox not overlap */
	}

	/*
	 * compute distances
	 * should be a fast calc if they actually do intersect
	 */
	calc_dist =     DatumGetFloat8 ( DirectFunctionCall2(LWGEOM_mindistance2d,   PointerGetDatum( pg_geom1 ),       PointerGetDatum( pg_geom2 )));

	PG_RETURN_BOOL(calc_dist < dist);
}

/**
 * Affine transform a pointarray.
 */
void
lwgeom_affine_ptarray(POINTARRAY *pa,
                      double afac, double bfac, double cfac,
                      double dfac, double efac, double ffac,
                      double gfac, double hfac, double ifac,
                      double xoff, double yoff, double zoff)
{
	int i;
	double x,y,z;
	POINT4D p4d;

	LWDEBUG(2, "lwgeom_affine_ptarray start");

	if ( TYPE_HASZ(pa->dims) )
	{
		LWDEBUG(3, " has z");

		for (i=0; i<pa->npoints; i++)
		{
			getPoint4d_p(pa, i, &p4d);
			x = p4d.x;
			y = p4d.y;
			z = p4d.z;
			p4d.x = afac * x + bfac * y + cfac * z + xoff;
			p4d.y = dfac * x + efac * y + ffac * z + yoff;
			p4d.z = gfac * x + hfac * y + ifac * z + zoff;
			setPoint4d(pa, i, &p4d);

			LWDEBUGF(3, " POINT %g %g %g => %g %g %g", x, y, x, p4d.x, p4d.y, p4d.z);
		}
	}
	else
	{
		LWDEBUG(3, " doesn't have z");

		for (i=0; i<pa->npoints; i++)
		{
			getPoint4d_p(pa, i, &p4d);
			x = p4d.x;
			y = p4d.y;
			p4d.x = afac * x + bfac * y + xoff;
			p4d.y = dfac * x + efac * y + yoff;
			setPoint4d(pa, i, &p4d);

			LWDEBUGF(3, " POINT %g %g %g => %g %g %g", x, y, x, p4d.x, p4d.y, p4d.z);
		}
	}

	LWDEBUG(3, "lwgeom_affine_ptarray end");

}

void
lwgeom_affine_recursive(uchar *serialized,
                        double afac, double bfac, double cfac,
                        double dfac, double efac, double ffac,
                        double gfac, double hfac, double ifac,
                        double xoff, double yoff, double zoff)
{
	LWGEOM_INSPECTED *inspected;
	int i, j;

	inspected = lwgeom_inspect(serialized);

	/* scan each object translating it */
	for (i=0; i<inspected->ngeometries; i++)
	{
		LWLINE *line=NULL;
		LWPOINT *point=NULL;
		LWPOLY *poly=NULL;
		LWCIRCSTRING *curve=NULL;
		uchar *subgeom=NULL;

		point = lwgeom_getpoint_inspected(inspected, i);
		if (point !=NULL)
		{
			lwgeom_affine_ptarray(point->point,
			                      afac, bfac, cfac,
			                      dfac, efac, ffac,
			                      gfac, hfac, ifac,
			                      xoff, yoff, zoff);
			lwgeom_release((LWGEOM *)point);
			continue;
		}

		poly = lwgeom_getpoly_inspected(inspected, i);
		if (poly !=NULL)
		{
			for (j=0; j<poly->nrings; j++)
			{
				lwgeom_affine_ptarray(poly->rings[j],
				                      afac, bfac, cfac,
				                      dfac, efac, ffac,
				                      gfac, hfac, ifac,
				                      xoff, yoff, zoff);
			}
			lwgeom_release((LWGEOM *)poly);
			continue;
		}

		line = lwgeom_getline_inspected(inspected, i);
		if (line != NULL)
		{
			lwgeom_affine_ptarray(line->points,
			                      afac, bfac, cfac,
			                      dfac, efac, ffac,
			                      gfac, hfac, ifac,
			                      xoff, yoff, zoff);
			lwgeom_release((LWGEOM *)line);
			continue;
		}

		curve = lwgeom_getcircstring_inspected(inspected, i);
		if (curve != NULL)
		{
			lwgeom_affine_ptarray(curve->points,
			                      afac, bfac, cfac,
			                      dfac, efac, ffac,
			                      gfac, hfac, ifac,
			                      xoff, yoff, zoff);
			lwgeom_release((LWGEOM *)curve);
			continue;
		}

		subgeom = lwgeom_getsubgeometry_inspected(inspected, i);
		if ( subgeom == NULL )
		{
			elog(ERROR, "lwgeom_getsubgeometry_inspected returned NULL??");
		}

		lwgeom_affine_recursive(subgeom,
		                        afac, bfac, cfac,
		                        dfac, efac, ffac,
		                        gfac, hfac, ifac,
		                        xoff, yoff, zoff);
	}

	lwinspected_release(inspected);
}

/*affine transform geometry */
PG_FUNCTION_INFO_V1(LWGEOM_affine);
Datum LWGEOM_affine(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));
	PG_LWGEOM *ret;
	LWGEOM *tmp;
	uchar *srl = SERIALIZED_FORM(geom);

	double afac =  PG_GETARG_FLOAT8(1);
	double bfac =  PG_GETARG_FLOAT8(2);
	double cfac =  PG_GETARG_FLOAT8(3);
	double dfac =  PG_GETARG_FLOAT8(4);
	double efac =  PG_GETARG_FLOAT8(5);
	double ffac =  PG_GETARG_FLOAT8(6);
	double gfac =  PG_GETARG_FLOAT8(7);
	double hfac =  PG_GETARG_FLOAT8(8);
	double ifac =  PG_GETARG_FLOAT8(9);
	double xoff =  PG_GETARG_FLOAT8(10);
	double yoff =  PG_GETARG_FLOAT8(11);
	double zoff =  PG_GETARG_FLOAT8(12);

	POSTGIS_DEBUG(2, "LWGEOM_affine called.");

	lwgeom_affine_recursive(srl,
	                        afac, bfac, cfac,
	                        dfac, efac, ffac,
	                        gfac, hfac, ifac,
	                        xoff, yoff, zoff);

	/* COMPUTE_BBOX TAINTING */
	tmp = pglwgeom_deserialize(geom);
	lwgeom_drop_bbox(tmp);
	tmp->bbox = lwgeom_compute_box2d(tmp);
	ret = pglwgeom_serialize(tmp);

	/* Release memory */
	pfree(geom);
	lwgeom_release(tmp);

	PG_RETURN_POINTER(ret);
}

PG_FUNCTION_INFO_V1(ST_GeoHash);
Datum ST_GeoHash(PG_FUNCTION_ARGS)
{

	PG_LWGEOM *geom = NULL;
	int precision = 0;
	int len = 0;
	char *geohash = NULL;
	char *result = NULL;

	if ( PG_ARGISNULL(0) )
	{
		PG_RETURN_NULL();
	}

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	if ( ! PG_ARGISNULL(1) )
	{
		precision = PG_GETARG_INT32(1);
	}

	geohash = lwgeom_geohash((LWGEOM*)(pglwgeom_deserialize(geom)), precision);

	if ( ! geohash )
	{
		elog(ERROR,"ST_GeoHash: lwgeom_geohash returned NULL.\n");
		PG_RETURN_NULL();
	}

	len = strlen(geohash) + VARHDRSZ;
	result = palloc(len);
	SET_VARSIZE(result, len);
	memcpy(VARDATA(result), geohash, len-VARHDRSZ);
	pfree(geohash);
	PG_RETURN_POINTER(result);

}
