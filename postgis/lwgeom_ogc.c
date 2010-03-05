/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2005 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "postgres.h"

#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "access/gist.h"
#include "access/itup.h"

#include "fmgr.h"
#include "utils/elog.h"

#include "liblwgeom.h"
#include "lwgeom_pg.h"


/* ---- SRID(geometry) */
Datum LWGEOM_getSRID(PG_FUNCTION_ARGS);
/* ---- SetSRID(geometry, integer) */
Datum LWGEOM_setSRID(PG_FUNCTION_ARGS);
/* ---- GeometryType(geometry) */
Datum LWGEOM_getTYPE(PG_FUNCTION_ARGS);
Datum geometry_geometrytype(PG_FUNCTION_ARGS);
/* ---- NumPoints(geometry) */
Datum LWGEOM_numpoints_linestring(PG_FUNCTION_ARGS);
/* ---- NumGeometries(geometry) */
Datum LWGEOM_numgeometries_collection(PG_FUNCTION_ARGS);
/* ---- GeometryN(geometry, integer) */
Datum LWGEOM_geometryn_collection(PG_FUNCTION_ARGS);
/* ---- Dimension(geometry) */
Datum LWGEOM_dimension(PG_FUNCTION_ARGS);
/* ---- ExteriorRing(geometry) */
Datum LWGEOM_exteriorring_polygon(PG_FUNCTION_ARGS);
/* ---- InteriorRingN(geometry, integer) */
Datum LWGEOM_interiorringn_polygon(PG_FUNCTION_ARGS);
/* ---- NumInteriorRings(geometry) */
Datum LWGEOM_numinteriorrings_polygon(PG_FUNCTION_ARGS);
/* ---- PointN(geometry, integer) */
Datum LWGEOM_pointn_linestring(PG_FUNCTION_ARGS);
/* ---- X(geometry) */
Datum LWGEOM_x_point(PG_FUNCTION_ARGS);
/* ---- Y(geometry) */
Datum LWGEOM_y_point(PG_FUNCTION_ARGS);
/* ---- Z(geometry) */
Datum LWGEOM_z_point(PG_FUNCTION_ARGS);
/* ---- M(geometry) */
Datum LWGEOM_m_point(PG_FUNCTION_ARGS);
/* ---- StartPoint(geometry) */
Datum LWGEOM_startpoint_linestring(PG_FUNCTION_ARGS);
/* ---- EndPoint(geometry) */
Datum LWGEOM_endpoint_linestring(PG_FUNCTION_ARGS);
/* ---- AsText(geometry) */
Datum LWGEOM_asText(PG_FUNCTION_ARGS);
/* ---- AsBinary(geometry, [XDR|NDR]) */
Datum LWGEOM_asBinary(PG_FUNCTION_ARGS);
/* ---- GeometryFromText(text, integer) */
Datum LWGEOM_from_text(PG_FUNCTION_ARGS);
/* ---- GeomFromWKB(bytea, integer) */
Datum LWGEOM_from_WKB(PG_FUNCTION_ARGS);
/* ---- IsClosed(geometry) */
Datum LWGEOM_isclosed_linestring(PG_FUNCTION_ARGS);

/* internal */
static int32 lwgeom_numpoints_linestring_recursive(const uchar *serialized);
static int32 lwgeom_dimension_recursive(const uchar *serialized);
char line_is_closed(LWLINE *line);
char circstring_is_closed(LWCIRCSTRING *curve);
char compound_is_closed(LWCOMPOUND *compound);

/*------------------------------------------------------------------*/

/* getSRID(lwgeom) :: int4 */
PG_FUNCTION_INFO_V1(LWGEOM_getSRID);
Datum LWGEOM_getSRID(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *pglwgeom=(PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int srid = pglwgeom_getSRID (pglwgeom);
	PG_FREE_IF_COPY(pglwgeom,0);
	PG_RETURN_INT32(srid);
}

/* setSRID(lwgeom, int4) :: lwgeom */
PG_FUNCTION_INFO_V1(LWGEOM_setSRID);
Datum LWGEOM_setSRID(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int newSRID = PG_GETARG_INT32(1);
	PG_LWGEOM *result;

	result = PG_LWGEOM_construct(SERIALIZED_FORM(geom), newSRID,
	                             lwgeom_hasBBOX(geom->type));

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(result);
}

/* returns a string representation of this geometry's type */
PG_FUNCTION_INFO_V1(LWGEOM_getTYPE);
Datum LWGEOM_getTYPE(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *lwgeom;
	char *text_ob;
	char *result;
	int32 size;
	uchar type;

	lwgeom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	text_ob = lwalloc(20+VARHDRSZ);
	result = text_ob+VARHDRSZ;

	type = lwgeom_getType(lwgeom->type);

	memset(VARDATA(text_ob), 0, 20);

	if (type == POINTTYPE)
		strcpy(result,"POINT");
	else if (type == MULTIPOINTTYPE)
		strcpy(result,"MULTIPOINT");
	else if (type == LINETYPE)
		strcpy(result,"LINESTRING");
	else if (type == CIRCSTRINGTYPE)
		strcpy(result,"CIRCULARSTRING");
	else if (type == COMPOUNDTYPE)
		strcpy(result, "COMPOUNDCURVE");
	else if (type == MULTILINETYPE)
		strcpy(result,"MULTILINESTRING");
	else if (type == MULTICURVETYPE)
		strcpy(result, "MULTICURVE");
	else if (type == POLYGONTYPE)
		strcpy(result,"POLYGON");
	else if (type == CURVEPOLYTYPE)
		strcpy(result,"CURVEPOLYGON");
	else if (type == MULTIPOLYGONTYPE)
		strcpy(result,"MULTIPOLYGON");
	else if (type == MULTISURFACETYPE)
		strcpy(result, "MULTISURFACE");
	else if (type == COLLECTIONTYPE)
		strcpy(result,"GEOMETRYCOLLECTION");
	else
		strcpy(result,"UNKNOWN");

	if ( TYPE_HASM(lwgeom->type) && ! TYPE_HASZ(lwgeom->type) )
		strcat(result, "M");

	size = strlen(result) + VARHDRSZ ;
	SET_VARSIZE(text_ob, size); /* size of string */

	PG_FREE_IF_COPY(lwgeom, 0);

	PG_RETURN_POINTER(text_ob);
}


/* returns a string representation of this geometry's type */
PG_FUNCTION_INFO_V1(geometry_geometrytype);
Datum geometry_geometrytype(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *lwgeom;
	char *type_text;
	char *type_str = palloc(32);
	size_t size;

	lwgeom = (PG_LWGEOM*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* Make it empty string to start */
	*type_str = 0;

	/* Build up the output string */
	strncat(type_str, "ST_", 32);
	strncat(type_str, lwgeom_typename(lwgeom_getType(lwgeom->type)), 32);
	size = strlen(type_str) + VARHDRSZ;

	/* Build a text type to store things in */
	type_text = lwalloc(size);
	memcpy(VARDATA(type_text),type_str, size - VARHDRSZ);
	pfree(type_str);
	SET_VARSIZE(type_text, size);
	PG_FREE_IF_COPY(lwgeom, 0);
	PG_RETURN_POINTER(type_text);
}


/**
 * Find first linestring in serialized geometry and return
 * the number of points in it. If no linestrings are found
 * return -1.
 */
static int32
lwgeom_numpoints_linestring_recursive(const uchar *serialized)
{
	LWGEOM_INSPECTED *inspected = lwgeom_inspect(serialized);
	int i;

	LWDEBUG(2, "lwgeom_numpoints_linestring_recursive called.");

	/*
	 * CURVEPOLY and COMPOUND have no concept of numpoints but look like
	 * collections once inspected.  Fast-fail on these here.
	 */
	if (lwgeom_getType(inspected->type) == COMPOUNDTYPE ||
	        lwgeom_getType(inspected->type) == CURVEPOLYTYPE)
	{
		return -1;
	}

	for (i=0; i<inspected->ngeometries; i++)
	{
		int32 npoints;
		int type;
		LWGEOM *geom=NULL;
		uchar *subgeom;

		geom = lwgeom_getgeom_inspected(inspected, i);

		LWDEBUGF(3, "numpoints_recursive: type=%d", lwgeom_getType(geom->type));

		if (lwgeom_getType(geom->type) == LINETYPE)
		{
			return ((LWLINE *)geom)->points->npoints;
		}
		else if (lwgeom_getType(geom->type) == CIRCSTRINGTYPE)
		{
			return ((LWCIRCSTRING *)geom)->points->npoints;
		}

		subgeom = lwgeom_getsubgeometry_inspected(inspected, i);
		if ( subgeom == NULL )
		{
			elog(ERROR, "What ? lwgeom_getsubgeometry_inspected returned NULL??");
		}

		type = lwgeom_getType(subgeom[0]);

		/* MULTILINESTRING && GEOMETRYCOLLECTION are worth checking */
		if ( type != MULTILINETYPE && type != COLLECTIONTYPE ) continue;

		npoints = lwgeom_numpoints_linestring_recursive(subgeom);
		if ( npoints == -1 ) continue;

		lwinspected_release(inspected);

		return npoints;
	}

	lwinspected_release(inspected);

	return -1;
}

/**
 * numpoints(GEOMETRY) -- find the first linestring in GEOMETRY, return
 * the number of points in it.  Return NULL if there is no LINESTRING(..)
 * in GEOMETRY
 */
PG_FUNCTION_INFO_V1(LWGEOM_numpoints_linestring);
Datum LWGEOM_numpoints_linestring(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int32 ret;

	POSTGIS_DEBUG(2, "LWGEOM_numpoints_linestring called.");

	ret = lwgeom_numpoints_linestring_recursive(SERIALIZED_FORM(geom));
	if ( ret == -1 )
	{
		PG_FREE_IF_COPY(geom, 0);
		PG_RETURN_NULL();
	}
	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_INT32(ret);
}

PG_FUNCTION_INFO_V1(LWGEOM_numgeometries_collection);
Datum LWGEOM_numgeometries_collection(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int type;
	int32 ret;
	uchar *serialized = SERIALIZED_FORM(geom);

	type = lwgeom_getType(geom->type);
	if (type==MULTIPOINTTYPE || type==MULTILINETYPE ||
	        type==MULTICURVETYPE || type==MULTIPOLYGONTYPE ||
	        type==MULTISURFACETYPE || type==COLLECTIONTYPE)
	{
		ret = lwgeom_getnumgeometries(serialized);
		PG_FREE_IF_COPY(geom, 0);
		PG_RETURN_INT32(ret);
	}
	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_NULL();
}

/** 1-based offset */
PG_FUNCTION_INFO_V1(LWGEOM_geometryn_collection);
Datum LWGEOM_geometryn_collection(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *result;
	int type = lwgeom_getType(geom->type);
	int32 idx;
	LWCOLLECTION *coll;
	LWGEOM *subgeom;

	POSTGIS_DEBUG(2, "LWGEOM_geometryn_collection called.");

	/* elog(NOTICE, "GeometryN called"); */

	/* call is valid on multi* geoms only */
	if (type==POINTTYPE || type==LINETYPE || type==CIRCSTRINGTYPE ||
	        type==COMPOUNDTYPE || type==POLYGONTYPE || type==CURVEPOLYTYPE)
	{
		/* elog(NOTICE, "geometryn: geom is of type %d, requires >=4", type); */
		PG_RETURN_NULL();
	}

	idx = PG_GETARG_INT32(1);
	idx -= 1; /* index is 1-based */

	coll = (LWCOLLECTION *)lwgeom_deserialize(SERIALIZED_FORM(geom));

	if ( idx < 0 ) PG_RETURN_NULL();
	if ( idx >= coll->ngeoms ) PG_RETURN_NULL();

	subgeom = coll->geoms[idx];
	subgeom->SRID = coll->SRID;

	/* COMPUTE_BBOX==TAINTING */
	if ( coll->bbox ) lwgeom_add_bbox(subgeom);

	result = pglwgeom_serialize(subgeom);

	lwgeom_release((LWGEOM *)coll);
	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(result);

}

/**
 * @brief returns 0 for points, 1 for lines, 2 for polygons.
 * 			returns max dimension for a collection.
 * 		returns -1 for the empty geometry (TODO)
 * 		returns -2 on error
 */
static int32
lwgeom_dimension_recursive(const uchar *serialized)
{
	LWGEOM_INSPECTED *inspected;
	int ret = -1;
	int i;

	/** @todo
	 * 		This has the unfortunate habit of traversing the CURVEPOLYTYPe
	 * 		geoms and returning 1, as all contained geoms are linear.
	 * 		Here we preempt this problem.
	 * 		TODO: This should handle things a bit better.  Only
	 * 		GEOMETRYCOLLECTION should ever need to be traversed.
	 */
	if (lwgeom_getType(serialized[0]) == CURVEPOLYTYPE) return 2;

	inspected = lwgeom_inspect(serialized);
	for (i=0; i<inspected->ngeometries; i++)
	{
		uchar *subgeom;
		char typeflags = lwgeom_getsubtype_inspected(inspected, i);
		int type = lwgeom_getType(typeflags);
		int dims=-1;

		LWDEBUGF(3, "lwgeom_dimension_recursive: type %d", type);

		if ( type == POINTTYPE ) dims = 0;
		else if ( type == MULTIPOINTTYPE ) dims=0;
		else if ( type == LINETYPE ) dims=1;
		else if ( type == CIRCSTRINGTYPE ) dims=1;
		else if ( type == COMPOUNDTYPE ) dims=1;
		else if ( type == MULTILINETYPE ) dims=1;
		else if ( type == MULTICURVETYPE ) dims=1;
		else if ( type == POLYGONTYPE ) dims=2;
		else if ( type == CURVEPOLYTYPE ) dims=2;
		else if ( type == MULTIPOLYGONTYPE ) dims=2;
		else if ( type == MULTISURFACETYPE ) dims=2;
		else if ( type == COLLECTIONTYPE )
		{
			subgeom = lwgeom_getsubgeometry_inspected(inspected, i);
			if ( subgeom == NULL )
			{
				lwinspected_release(inspected);
				return -2;
			}

			dims = lwgeom_dimension_recursive(subgeom);
		}

		if ( dims == 2 )
		{
			/* nothing can be higher */
			lwinspected_release(inspected);
			return 2;
		}
		if ( dims > ret ) ret = dims;
	}

	lwinspected_release(inspected);

	return ret;
}

/** @brief
 * 		returns 0 for points, 1 for lines, 2 for polygons.
 * 		returns max dimension for a collection.
 * 	TODO: @todo return -1 for the empty geometry (TODO)
 */
PG_FUNCTION_INFO_V1(LWGEOM_dimension);
Datum LWGEOM_dimension(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int dimension;

	POSTGIS_DEBUG(2, "LWGEOM_dimension called");

	dimension = lwgeom_dimension_recursive(SERIALIZED_FORM(geom));
	if ( dimension == -1 )
	{
		PG_FREE_IF_COPY(geom, 0);
		elog(NOTICE, "Could not compute geometry dimensions");
		PG_RETURN_NULL();
	}

	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_INT32(dimension);
}


/**
 * exteriorRing(GEOMETRY) -- find the first polygon in GEOMETRY
 *	@return its exterior ring (as a linestring).
 * 		Return NULL if there is no POLYGON(..) in (first level of) GEOMETRY.
 */
PG_FUNCTION_INFO_V1(LWGEOM_exteriorring_polygon);
Datum LWGEOM_exteriorring_polygon(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWPOLY *poly = NULL;
	LWCURVEPOLY *curvepoly = NULL;
	POINTARRAY *extring;
	LWGEOM *ring;
	LWLINE *line;
	PG_LWGEOM *result;
	BOX2DFLOAT4 *bbox=NULL;

	POSTGIS_DEBUG(2, "LWGEOM_exteriorring_polygon called.");

	if ( TYPE_GETTYPE(geom->type) != POLYGONTYPE &&
	        TYPE_GETTYPE(geom->type) != CURVEPOLYTYPE)
	{
		elog(ERROR, "ExteriorRing: geom is not a polygon");
		PG_RETURN_NULL();
	}
	if (lwgeom_getType((uchar)SERIALIZED_FORM(geom)[0]) == POLYGONTYPE)
	{
		poly = lwpoly_deserialize(SERIALIZED_FORM(geom));

		/* Ok, now we have a polygon. Here is its exterior ring. */
		extring = poly->rings[0];

		/*
		* This is a LWLINE constructed by exterior ring POINTARRAY
		* If the input geom has a bbox, use it for
		* the output geom, as exterior ring makes it up !
		*/
		if ( poly->bbox ) bbox=box2d_clone(poly->bbox);
		line = lwline_construct(poly->SRID, bbox, extring);

		result = pglwgeom_serialize((LWGEOM *)line);

		lwgeom_release((LWGEOM *)line);
		lwgeom_release((LWGEOM *)poly);
	}
	else
	{
		curvepoly = lwcurvepoly_deserialize(SERIALIZED_FORM(geom));
		ring = curvepoly->rings[0];
		result = pglwgeom_serialize(ring);
		lwgeom_release(ring);
	}

	PG_FREE_IF_COPY(geom, 0);


	PG_RETURN_POINTER(result);
}

/**
 * NumInteriorRings(GEOMETRY) -- find the first polygon in GEOMETRY,
 *	@return the number of its interior rings (holes).
 * 		Return NULL if there is no POLYGON(..) in (first level of) GEOMETRY.
 */
PG_FUNCTION_INFO_V1(LWGEOM_numinteriorrings_polygon);
Datum LWGEOM_numinteriorrings_polygon(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM_INSPECTED *inspected = NULL;
	LWGEOM *tmp = NULL;
	LWPOLY *poly = NULL;
	LWCURVEPOLY *curvepoly = NULL;
	int32 result;
	int i;

	POSTGIS_DEBUG(2, "LWGEOM_numinteriorrings called.");

	if (lwgeom_getType((uchar)SERIALIZED_FORM(geom)[0]) == CURVEPOLYTYPE)
	{
		tmp = (LWGEOM *)lwcurvepoly_deserialize(SERIALIZED_FORM(geom));
	}
	else
	{
		inspected = lwgeom_inspect(SERIALIZED_FORM(geom));
		for (i=0; i<inspected->ngeometries; i++)
		{
			tmp = lwgeom_getgeom_inspected(inspected, i);
			if (lwgeom_getType(tmp->type) == POLYGONTYPE ||
			        lwgeom_getType(tmp->type) == CURVEPOLYTYPE) break;
		}
	}

	if ( tmp == NULL )
	{
		PG_FREE_IF_COPY(geom, 0);
		lwinspected_release(inspected);
		PG_RETURN_NULL();
	}

	POSTGIS_DEBUGF(3, "Geometry of type %d found.", lwgeom_getType(tmp->type));

	if (lwgeom_getType(tmp->type) == POLYGONTYPE)
	{
		poly = (LWPOLY *)tmp;

		/* Ok, now we have a polygon. Here is its number of holes */
		result = poly->nrings-1;
	}
	else if (lwgeom_getType(tmp->type) == CURVEPOLYTYPE)
	{
		POSTGIS_DEBUG(3, "CurvePolygon found.");

		curvepoly = (LWCURVEPOLY *)tmp;
		result = curvepoly->nrings-1;
	}
	else
	{
		PG_FREE_IF_COPY(geom, 0);
		lwinspected_release(inspected);
		PG_RETURN_NULL();
	}
	PG_FREE_IF_COPY(geom, 0);
	if (inspected != NULL) lwinspected_release(inspected);
	lwgeom_release((LWGEOM *)tmp);

	PG_RETURN_INT32(result);
}

/**
 * InteriorRingN(GEOMETRY) -- find the first polygon in GEOMETRY, Index is 1-based.
 * @return its Nth interior ring (as a linestring).
 * 		Return NULL if there is no POLYGON(..) in (first level of) GEOMETRY.
 *
 */
PG_FUNCTION_INFO_V1(LWGEOM_interiorringn_polygon);
Datum LWGEOM_interiorringn_polygon(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	int32 wanted_index;
	LWCURVEPOLY *curvepoly = NULL;
	LWPOLY *poly = NULL;
	POINTARRAY *ring;
	LWLINE *line;
	PG_LWGEOM *result;
	BOX2DFLOAT4 *bbox = NULL;

	POSTGIS_DEBUG(2, "LWGEOM_interierringn_polygon called.");

	wanted_index = PG_GETARG_INT32(1);
	if ( wanted_index < 1 )
	{
		/* elog(ERROR, "InteriorRingN: ring number is 1-based"); */
		PG_RETURN_NULL(); /* index out of range */
	}

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	if ( TYPE_GETTYPE(geom->type) != POLYGONTYPE &&
	        TYPE_GETTYPE(geom->type) != CURVEPOLYTYPE )
	{
		PG_FREE_IF_COPY(geom, 0);
		elog(ERROR, "InteriorRingN: geom is not a polygon");
		PG_RETURN_NULL();
	}
	if ( TYPE_GETTYPE(geom->type) == POLYGONTYPE)
	{
		poly = lwpoly_deserialize(SERIALIZED_FORM(geom));

		/* Ok, now we have a polygon. Let's see if it has enough holes */
		if ( wanted_index >= poly->nrings )
		{
			PG_FREE_IF_COPY(geom, 0);
			lwgeom_release((LWGEOM *)poly);
			PG_RETURN_NULL();
		}

		ring = poly->rings[wanted_index];

		/* COMPUTE_BBOX==TAINTING */
		if ( poly->bbox ) bbox = ptarray_compute_box2d(ring);

		/* This is a LWLINE constructed by interior ring POINTARRAY */
		line = lwline_construct(poly->SRID, bbox, ring);

		/* Copy SRID from polygon */
		line->SRID = poly->SRID;

		result = pglwgeom_serialize((LWGEOM *)line);
		lwgeom_release((LWGEOM *)line);
		lwgeom_release((LWGEOM *)poly);
	}
	else
	{
		curvepoly = lwcurvepoly_deserialize(SERIALIZED_FORM(geom));

		if (wanted_index >= curvepoly->nrings)
		{
			PG_FREE_IF_COPY(geom, 0);
			lwgeom_release((LWGEOM *)curvepoly);
			PG_RETURN_NULL();
		}

		result = pglwgeom_serialize(curvepoly->rings[wanted_index]);
		lwgeom_release((LWGEOM *)curvepoly);
	}


	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(result);
}

/**
 * PointN(GEOMETRY,INTEGER) -- find the first linestring in GEOMETRY,
 * @return the point at index INTEGER (1 is 1st point).  Return NULL if
 * 		there is no LINESTRING(..) in GEOMETRY or INTEGER is out of bounds.
 */
PG_FUNCTION_INFO_V1(LWGEOM_pointn_linestring);
Datum LWGEOM_pointn_linestring(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	int32 wanted_index;
	LWGEOM_INSPECTED *inspected;
	LWLINE *line = NULL;
	LWCIRCSTRING *curve = NULL;
	LWGEOM *tmp = NULL;
	POINTARRAY *pts;
	LWPOINT *point;
	uchar *serializedpoint;
	PG_LWGEOM *result;
	int i, type;

	wanted_index = PG_GETARG_INT32(1);
	if ( wanted_index < 1 )
		PG_RETURN_NULL(); /* index out of range */

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	type = lwgeom_getType((uchar)SERIALIZED_FORM(geom)[0]);
	if (type == COMPOUNDTYPE || type == CURVEPOLYTYPE)
	{
		PG_FREE_IF_COPY(geom, 0);
		PG_RETURN_NULL();
	}
	else
	{
		inspected = lwgeom_inspect(SERIALIZED_FORM(geom));

		for (i=0; i<inspected->ngeometries; i++)
		{
			tmp = lwgeom_getgeom_inspected(inspected, i);
			if (lwgeom_getType(tmp->type) == LINETYPE ||
			        lwgeom_getType(tmp->type) == CIRCSTRINGTYPE)
				break;
		}

		if ( tmp == NULL )
		{
			lwinspected_release(inspected);
			PG_FREE_IF_COPY(geom, 0);
			PG_RETURN_NULL();
		}
		if (lwgeom_getType(tmp->type) == CIRCSTRINGTYPE)
		{
			curve = (LWCIRCSTRING *)tmp;
			if (wanted_index > curve->points->npoints)
			{
				lwinspected_release(inspected);
				PG_FREE_IF_COPY(geom, 0);
				lwgeom_release(tmp);
				PG_RETURN_NULL();
			}
			lwinspected_release(inspected);

			pts = pointArray_construct(getPoint_internal(
			                               curve->points,
			                               wanted_index-1),
			                           TYPE_HASZ(curve->type),
			                           TYPE_HASM(curve->type), 1);
		}
		else if (lwgeom_getType(tmp->type) == LINETYPE)
		{
			line = (LWLINE *)tmp;
			/* Ok, now we have a line. Let's see if it has enough points. */
			if ( wanted_index > line->points->npoints )
			{
				lwinspected_release(inspected);
				PG_FREE_IF_COPY(geom, 0);
				lwgeom_release(tmp);
				PG_RETURN_NULL();
			}
			lwinspected_release(inspected);

			/* Construct a point array */
			pts = pointArray_construct(getPoint_internal(line->points,
			                           wanted_index-1),
			                           TYPE_HASZ(line->type), TYPE_HASM(line->type), 1);
		}
		else
		{
			lwinspected_release(inspected);
			PG_FREE_IF_COPY(geom, 0);
			lwgeom_release(tmp);
			PG_RETURN_NULL();
		}
	}

	/* Construct an LWPOINT */
	point = lwpoint_construct(pglwgeom_getSRID(geom),
	                          NULL, pts);

	/* Serialized the point */
	serializedpoint = lwpoint_serialize(point);

	/* And we construct the line
	 * TODO: use serialize_buf above, instead ..
	 */
	result = PG_LWGEOM_construct(serializedpoint,
	                             pglwgeom_getSRID(geom), 0);

	pfree(point);
	pfree(serializedpoint);
	PG_FREE_IF_COPY(geom, 0);
	lwgeom_release(tmp);
	PG_RETURN_POINTER(result);
}

/**
 * X(GEOMETRY) -- return X value of the point.
 * @return an error if input is not a point.
 */
PG_FUNCTION_INFO_V1(LWGEOM_x_point);
Datum LWGEOM_x_point(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	LWPOINT *point = NULL;
	POINT2D p;

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	if ( TYPE_GETTYPE(geom->type) != POINTTYPE )
		lwerror("Argument to X() must be a point");

	point = lwgeom_getpoint(SERIALIZED_FORM(geom), 0);

	getPoint2d_p(point->point, 0, &p);

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_FLOAT8(p.x);
}

/**
 * Y(GEOMETRY) -- return Y value of the point.
 * 	Raise an error if input is not a point.
 */
PG_FUNCTION_INFO_V1(LWGEOM_y_point);
Datum LWGEOM_y_point(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	LWPOINT *point = NULL;
	POINT2D p;

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	if ( TYPE_GETTYPE(geom->type) != POINTTYPE )
		lwerror("Argument to Y() must be a point");

	point = lwgeom_getpoint(SERIALIZED_FORM(geom), 0);

	getPoint2d_p(point->point, 0, &p);

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_FLOAT8(p.y);
}

/**
 * Z(GEOMETRY) -- return Z value of the point.
 * @return NULL if there is no Z in the point.
 * 		Raise an error if input is not a point.
 */
PG_FUNCTION_INFO_V1(LWGEOM_z_point);
Datum LWGEOM_z_point(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	LWPOINT *point = NULL;
	POINT3DZ p;

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	if ( TYPE_GETTYPE(geom->type) != POINTTYPE )
		lwerror("Argument to Z() must be a point");

	point = lwgeom_getpoint(SERIALIZED_FORM(geom), 0);

	/* no Z in input */
	if ( ! TYPE_HASZ(geom->type) ) PG_RETURN_NULL();

	getPoint3dz_p(point->point, 0, &p);

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_FLOAT8(p.z);
}

/**  M(GEOMETRY) -- find the first POINT(..) in GEOMETRY, returns its M value.
 * @return NULL if there is no POINT(..) in GEOMETRY.
 * 		Return NULL if there is no M in this geometry.
 */
PG_FUNCTION_INFO_V1(LWGEOM_m_point);
Datum LWGEOM_m_point(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	LWPOINT *point = NULL;
	POINT3DM p;

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	if ( TYPE_GETTYPE(geom->type) != POINTTYPE )
		lwerror("Argument to M() must be a point");

	point = lwgeom_getpoint(SERIALIZED_FORM(geom), 0);

	/* no M in input */
	if ( ! TYPE_HASM(point->type) ) PG_RETURN_NULL();

	getPoint3dm_p(point->point, 0, &p);

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_FLOAT8(p.m);
}

/** StartPoint(GEOMETRY) -- find the first linestring in GEOMETRY,
 * @return the first point.
 * 		Return NULL if there is no LINESTRING(..) in GEOMETRY
 */
PG_FUNCTION_INFO_V1(LWGEOM_startpoint_linestring);
Datum LWGEOM_startpoint_linestring(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	LWGEOM_INSPECTED *inspected;
	LWLINE *line = NULL;
	POINTARRAY *pts;
	LWPOINT *point;
	PG_LWGEOM *result;
	int i, type;

	POSTGIS_DEBUG(2, "LWGEOM_startpoint_linestring called.");

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/*
	 * Curved polygons and compound curves are both collections
	 * that should not be traversed looking for linestrings.
	 */
	type = lwgeom_getType((uchar)SERIALIZED_FORM(geom)[0]);
	if (type == CURVEPOLYTYPE || type == COMPOUNDTYPE)
	{
		PG_FREE_IF_COPY(geom, 0);
		PG_RETURN_NULL();
	}

	inspected = lwgeom_inspect(SERIALIZED_FORM(geom));

	for (i=0; i<inspected->ngeometries; i++)
	{
		line = lwgeom_getline_inspected(inspected, i);
		if ( line ) break;
	}

	if ( line == NULL )
	{
		PG_FREE_IF_COPY(geom, 0);
		PG_RETURN_NULL();
	}

	/* Ok, now we have a line.  */

	/* Construct a point array */
	pts = pointArray_construct(getPoint_internal(line->points, 0),
	                           TYPE_HASZ(line->type),
	                           TYPE_HASM(line->type), 1);
	lwgeom_release((LWGEOM *)line);

	/* Construct an LWPOINT */
	point = lwpoint_construct(pglwgeom_getSRID(geom), NULL, pts);

	/* Construct a PG_LWGEOM */
	result = pglwgeom_serialize((LWGEOM *)point);

	lwgeom_release((LWGEOM *)point);
	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(result);
}

/** EndPoint(GEOMETRY) -- find the first linestring in GEOMETRY,
 * @return the last point.
 * 	Return NULL if there is no LINESTRING(..) in GEOMETRY
 */
PG_FUNCTION_INFO_V1(LWGEOM_endpoint_linestring);
Datum LWGEOM_endpoint_linestring(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	LWGEOM_INSPECTED *inspected;
	LWLINE *line = NULL;
	POINTARRAY *pts;
	LWGEOM *point;
	PG_LWGEOM *result;
	int i, type;

	POSTGIS_DEBUG(2, "LWGEOM_endpoint_linestring called.");

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	type = lwgeom_getType((uchar)SERIALIZED_FORM(geom)[0]);
	if (type == CURVEPOLYTYPE || type == COMPOUNDTYPE)
	{
		PG_FREE_IF_COPY(geom, 0);
		PG_RETURN_NULL();
	}

	inspected = lwgeom_inspect(SERIALIZED_FORM(geom));

	for (i=0; i<inspected->ngeometries; i++)
	{
		line = lwgeom_getline_inspected(inspected, i);
		if ( line ) break;
	}
	lwinspected_release(inspected);

	if ( line == NULL )
	{
		PG_FREE_IF_COPY(geom, 0);
		PG_RETURN_NULL();
	}

	/* Ok, now we have a line.  */

	/* Construct a point array */
	pts = pointArray_construct(
	          getPoint_internal(line->points, line->points->npoints-1),
	          TYPE_HASZ(line->type),
	          TYPE_HASM(line->type), 1);
	lwgeom_release((LWGEOM *)line);

	/* Construct an LWPOINT */
	point = (LWGEOM *)lwpoint_construct(pglwgeom_getSRID(geom), NULL, pts);

	/* Serialize an PG_LWGEOM */
	result = pglwgeom_serialize(point);
	lwgeom_release(point);

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(result);
}

/**
 * @brief Returns a geometry Given an OGC WKT (and optionally a SRID)
 * @return a geometry.
 * @note Note that this is a a stricter version
 * 		of geometry_in, where we refuse to
 * 		accept (HEX)WKB or EWKT.
 */
PG_FUNCTION_INFO_V1(LWGEOM_from_text);
Datum LWGEOM_from_text(PG_FUNCTION_ARGS)
{
	text *wkttext = PG_GETARG_TEXT_P(0);
	char *wkt, fc;
	size_t size;
	LWGEOM_PARSER_RESULT lwg_parser_result;
	PG_LWGEOM *geom_result = NULL;
	LWGEOM *lwgeom;
	int result;

	POSTGIS_DEBUG(2, "LWGEOM_from_text");

	size = VARSIZE(wkttext)-VARHDRSZ;

	POSTGIS_DEBUGF(3, "size: %d", (int)size);

	if ( size < 10 )
	{
		lwerror("Invalid OGC WKT (too short)");
		PG_RETURN_NULL();
	}

	fc=*(VARDATA(wkttext));

	wkt = lwalloc(size+1);
	memcpy(wkt, VARDATA(wkttext), size);
	wkt[size]='\0';

	POSTGIS_DEBUGF(3, "wkt: [%s]", wkt);

	result = serialized_lwgeom_from_ewkt(&lwg_parser_result, wkt, PARSER_CHECK_ALL);
	if (result)
		PG_PARSER_ERROR(lwg_parser_result);

	lwgeom = lwgeom_deserialize(lwg_parser_result.serialized_lwgeom);

	if ( lwgeom->SRID != -1 || TYPE_GETZM(lwgeom->type) != 0 )
	{
		elog(WARNING, "OGC WKT expected, EWKT provided - use GeomFromEWKT() for this");
	}

	/* read user-requested SRID if any */
	if ( PG_NARGS() > 1 ) lwgeom->SRID = PG_GETARG_INT32(1);

	geom_result = pglwgeom_serialize(lwgeom);
	lwgeom_release(lwgeom);

	PG_RETURN_POINTER(geom_result);
}

/**
 * Given an OGC WKB (and optionally a SRID)
 * 		return a geometry.
 *
 * @note that this is a wrapper around
 * 		LWGEOMFromWKB, where we refuse to
 * 		accept EWKB.
 */
PG_FUNCTION_INFO_V1(LWGEOM_from_WKB);
Datum LWGEOM_from_WKB(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	int32 SRID;
	PG_LWGEOM *result = NULL;

	geom = (PG_LWGEOM *)DatumGetPointer(DirectFunctionCall1(
	                                        LWGEOMFromWKB, PG_GETARG_DATUM(0)));

	if ( pglwgeom_getSRID(geom) != -1 || TYPE_GETZM(geom->type) != 0 )
	{
		elog(WARNING, "OGC WKB expected, EWKB provided - use GeometryFromEWKB() for this");
	}


	/* read user-requested SRID if any */
	if ( PG_NARGS() > 1 )
	{
		SRID = PG_GETARG_INT32(1);
		if ( SRID != pglwgeom_getSRID(geom) )
		{
			result = pglwgeom_setSRID(geom, SRID);
			pfree(geom);
		}
	}

	if ( ! result )	result = geom;

	PG_RETURN_POINTER(result);
}

/** convert LWGEOM to wkt (in TEXT format) */
PG_FUNCTION_INFO_V1(LWGEOM_asText);
Datum LWGEOM_asText(PG_FUNCTION_ARGS)
{
	LWGEOM_UNPARSER_RESULT lwg_unparser_result;
	PG_LWGEOM *lwgeom;
	PG_LWGEOM *ogclwgeom;
	int len, result;
	char *lwgeom_result,*loc_wkt;
	char *semicolonLoc;

	POSTGIS_DEBUG(2, "LWGEOM_asText called.");

	lwgeom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* Force to 2d */
	ogclwgeom = (PG_LWGEOM *)DatumGetPointer(DirectFunctionCall1(
	                LWGEOM_force_2d, PointerGetDatum(lwgeom)));

	result =  serialized_lwgeom_to_ewkt(&lwg_unparser_result, SERIALIZED_FORM(ogclwgeom), PARSER_CHECK_NONE);
	if (result)
		PG_UNPARSER_ERROR(lwg_unparser_result);

	semicolonLoc = strchr(lwg_unparser_result.wkoutput,';');

	/* loc points to start of wkt */
	if (semicolonLoc == NULL) loc_wkt = lwg_unparser_result.wkoutput;
	else loc_wkt = semicolonLoc +1;

	len = strlen(loc_wkt)+VARHDRSZ;
	lwgeom_result = palloc(len);
	SET_VARSIZE(lwgeom_result, len);

	memcpy(VARDATA(lwgeom_result), loc_wkt, len-VARHDRSZ);

	pfree(lwg_unparser_result.wkoutput);
	PG_FREE_IF_COPY(lwgeom, 0);
	if ( ogclwgeom != lwgeom ) pfree(ogclwgeom);

	PG_RETURN_POINTER(lwgeom_result);
}

/** convert LWGEOM to wkb (in BINARY format) */
PG_FUNCTION_INFO_V1(LWGEOM_asBinary);
Datum LWGEOM_asBinary(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *ogclwgeom;
	char *result;

	/* Force to 2d */
	ogclwgeom = (PG_LWGEOM *)DatumGetPointer(DirectFunctionCall1(
	                LWGEOM_force_2d, PG_GETARG_DATUM(0)));

	/* Drop SRID */
	ogclwgeom = (PG_LWGEOM *)DatumGetPointer(DirectFunctionCall2(
	                LWGEOM_setSRID, PointerGetDatum(ogclwgeom), -1));

	/* Call WKBFromLWGEOM */
	if ( (PG_NARGS()>1) && (!PG_ARGISNULL(1)) )
	{
		result = (char *)DatumGetPointer(DirectFunctionCall2(
		                                     WKBFromLWGEOM, PointerGetDatum(ogclwgeom),
		                                     PG_GETARG_DATUM(1)));
	}
	else
	{
		result = (char *)DatumGetPointer(DirectFunctionCall1(
		                                     WKBFromLWGEOM, PointerGetDatum(ogclwgeom)));
	}

	PG_RETURN_POINTER(result);
}

char line_is_closed(LWLINE *line)
{
	POINT3DZ sp, ep;

	LWDEBUG(2, "line_is_closed called.");

	getPoint3dz_p(line->points, 0, &sp);
	getPoint3dz_p(line->points, line->points->npoints-1, &ep);

	if ( sp.x != ep.x ) return 0;
	if ( sp.y != ep.y ) return 0;
	if ( TYPE_HASZ(line->type) )
	{
		if ( sp.z != ep.z ) return 0;
	}

	return 1;
}

char circstring_is_closed(LWCIRCSTRING *curve)
{
	POINT3DZ sp, ep;

	LWDEBUG(2, "circstring_is_closed called.");

	getPoint3dz_p(curve->points, 0, &sp);
	getPoint3dz_p(curve->points, curve->points->npoints-1, &ep);

	if (sp.x != ep.x) return 0;
	if (sp.y != ep.y) return 0;
	if (TYPE_HASZ(curve->type))
	{
		if (sp.z != ep.z) return 0;
	}
	return 1;
}

char compound_is_closed(LWCOMPOUND *compound)
{
	POINT3DZ sp, ep;
	LWGEOM *tmp;

	LWDEBUG(2, "compound_is_closed called.");

	tmp = compound->geoms[0];
	if (lwgeom_getType(tmp->type) == LINETYPE)
	{
		getPoint3dz_p(((LWLINE *)tmp)->points, 0, &sp);
	}
	else
	{
		getPoint3dz_p(((LWCIRCSTRING *)tmp)->points, 0, &sp);
	}

	tmp = compound->geoms[compound->ngeoms - 1];
	if (lwgeom_getType(tmp->type) == LINETYPE)
	{
		getPoint3dz_p(((LWLINE *)tmp)->points, ((LWLINE *)tmp)->points->npoints - 1, &ep);
	}
	else
	{
		getPoint3dz_p(((LWCIRCSTRING *)tmp)->points, ((LWCIRCSTRING *)tmp)->points->npoints - 1, &ep);
	}

	if (sp.x != ep.x) return 0;
	if (sp.y != ep.y) return 0;
	if (TYPE_HASZ(compound->type))
	{
		if (sp.z != ep.z) return 0;
	}
	return 1;
}

/**
 * @brief IsClosed(GEOMETRY) if geometry is a linestring then returns
 * 		startpoint == endpoint.  If its not a linestring then return NULL.
 * 		If it's a collection containing multiple linestrings,
 * @return true only if all the linestrings have startpoint=endpoint.
 */
PG_FUNCTION_INFO_V1(LWGEOM_isclosed_linestring);
Datum LWGEOM_isclosed_linestring(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	LWGEOM_INSPECTED *inspected;
	LWGEOM *sub = NULL;
	LWCOMPOUND *compound = NULL;
	int linesfound=0;
	int i;

	POSTGIS_DEBUG(2, "LWGEOM_isclosed_linestring called.");

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	if (lwgeom_getType((uchar)SERIALIZED_FORM(geom)[0]) == COMPOUNDTYPE)
	{
		compound = lwcompound_deserialize(SERIALIZED_FORM(geom));
		if (compound_is_closed(compound))
		{
			lwgeom_release((LWGEOM *)compound);
			PG_FREE_IF_COPY(geom, 0);
			PG_RETURN_BOOL(TRUE);
		}
		else
		{
			lwgeom_release((LWGEOM *)compound);
			PG_FREE_IF_COPY(geom, 0);
			PG_RETURN_BOOL(FALSE);
		}
	}

	inspected = lwgeom_inspect(SERIALIZED_FORM(geom));

	for (i=0; i<inspected->ngeometries; i++)
	{
		sub = lwgeom_getgeom_inspected(inspected, i);
		if ( sub == NULL ) continue;
		else if (lwgeom_getType(sub->type) == LINETYPE &&
		         !line_is_closed((LWLINE *)sub))
		{
			lwgeom_release(sub);
			lwinspected_release(inspected);
			PG_FREE_IF_COPY(geom, 0);
			PG_RETURN_BOOL(FALSE);
		}
		else if (lwgeom_getType(sub->type) == CIRCSTRINGTYPE &&
		         !circstring_is_closed((LWCIRCSTRING *)sub))
		{
			lwgeom_release(sub);
			lwinspected_release(inspected);
			PG_FREE_IF_COPY(geom, 0);
			PG_RETURN_BOOL(FALSE);
		}
		else if (lwgeom_getType(sub->type) == COMPOUNDTYPE &&
		         !compound_is_closed((LWCOMPOUND *)sub))
		{
			lwgeom_release(sub);
			lwinspected_release(inspected);
			PG_FREE_IF_COPY(geom, 0);
			PG_RETURN_BOOL(FALSE);
		}
		lwgeom_release(sub);
		linesfound++;
	}
	lwinspected_release(inspected);

	if ( ! linesfound )
	{
		PG_FREE_IF_COPY(geom, 0);
		PG_RETURN_NULL();
	}
	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_BOOL(TRUE);

}
