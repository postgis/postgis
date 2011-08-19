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
Datum LWGEOM_get_srid(PG_FUNCTION_ARGS);
/* ---- SetSRID(geometry, integer) */
Datum LWGEOM_set_srid(PG_FUNCTION_ARGS);
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
Datum LWGEOM_isclosed(PG_FUNCTION_ARGS);

/*------------------------------------------------------------------*/

/* getSRID(lwgeom) :: int4 */
PG_FUNCTION_INFO_V1(LWGEOM_get_srid);
Datum LWGEOM_get_srid(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom=(PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int srid = pglwgeom_get_srid (geom);
	PG_FREE_IF_COPY(geom,0);
	PG_RETURN_INT32(srid);
}

/* setSRID(lwgeom, int4) :: lwgeom */
PG_FUNCTION_INFO_V1(LWGEOM_set_srid);
Datum LWGEOM_set_srid(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *result;
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int srid = PG_GETARG_INT32(1);
	LWGEOM *lwgeom = pglwgeom_deserialize(geom);
	lwgeom_set_srid(lwgeom, srid);
	result = pglwgeom_serialize(lwgeom);
	lwgeom_free(lwgeom);
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
	uint8_t type;

	lwgeom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	text_ob = lwalloc(20+VARHDRSZ);
	result = text_ob+VARHDRSZ;

	type = pglwgeom_get_type(lwgeom);

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
	else if (type == TRIANGLETYPE)
		strcpy(result,"TRIANGLE");
	else if (type == CURVEPOLYTYPE)
		strcpy(result,"CURVEPOLYGON");
	else if (type == MULTIPOLYGONTYPE)
		strcpy(result,"MULTIPOLYGON");
	else if (type == MULTISURFACETYPE)
		strcpy(result, "MULTISURFACE");
	else if (type == COLLECTIONTYPE)
		strcpy(result,"GEOMETRYCOLLECTION");
	else if (type == POLYHEDRALSURFACETYPE)
		strcpy(result,"POLYHEDRALSURFACE");
	else if (type == TINTYPE)
		strcpy(result,"TIN");
	else
		strcpy(result,"UNKNOWN");

	if ( pglwgeom_has_m(lwgeom) && ! pglwgeom_has_z(lwgeom) )
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
	text *type_text;
	char *type_str = palloc(32);

	lwgeom = (PG_LWGEOM*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* Make it empty string to start */
	*type_str = 0;

	/* Build up the output string */
	strncat(type_str, "ST_", 32);
	strncat(type_str, lwtype_name(pglwgeom_get_type(lwgeom)), 32);
	
	/* Build a text type to store things in */
	type_text = cstring2text(type_str);
	pfree(type_str);

	PG_FREE_IF_COPY(lwgeom, 0);
	PG_RETURN_TEXT_P(type_text);
}



/**
* numpoints(LINESTRING) -- return the number of points in the 
* linestring, or NULL if it is not a linestring
*/
PG_FUNCTION_INFO_V1(LWGEOM_numpoints_linestring);
Datum LWGEOM_numpoints_linestring(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *lwgeom = pglwgeom_deserialize(geom);
	int count = -1;
	
	if ( lwgeom->type == LINETYPE )
		count = lwgeom_count_vertices(lwgeom);

	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(geom, 0);

	/* OGC says this functions is only valid on LINESTRING */
	if ( count < 0 )
		PG_RETURN_NULL();

	PG_RETURN_INT32(count);
}

PG_FUNCTION_INFO_V1(LWGEOM_numgeometries_collection);
Datum LWGEOM_numgeometries_collection(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *lwgeom;
	int32 ret = 1;

	lwgeom = pglwgeom_deserialize(geom);
	if ( lwgeom_is_collection(lwgeom) )
	{
		LWCOLLECTION *col = lwgeom_as_lwcollection(lwgeom);
		ret = col->ngeoms;
	}
	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_INT32(ret);
}

/** 1-based offset */
PG_FUNCTION_INFO_V1(LWGEOM_geometryn_collection);
Datum LWGEOM_geometryn_collection(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *result;
	int type = pglwgeom_get_type(geom);
	int32 idx;
	LWCOLLECTION *coll;
	LWGEOM *subgeom;

	POSTGIS_DEBUG(2, "LWGEOM_geometryn_collection called.");

	/* elog(NOTICE, "GeometryN called"); */

	idx = PG_GETARG_INT32(1);
	idx -= 1; /* index is 1-based */

	/* call is valid on multi* geoms only */
	if (type==POINTTYPE || type==LINETYPE || type==CIRCSTRINGTYPE ||
	        type==COMPOUNDTYPE || type==POLYGONTYPE ||
		type==CURVEPOLYTYPE || type==TRIANGLETYPE)
	{
		if ( idx == 0 ) PG_RETURN_POINTER(geom);
		PG_RETURN_NULL();
	}

	coll = lwgeom_as_lwcollection(pglwgeom_deserialize(geom));

	if ( idx < 0 ) PG_RETURN_NULL();
	if ( idx >= coll->ngeoms ) PG_RETURN_NULL();

	subgeom = coll->geoms[idx];
	subgeom->srid = coll->srid;

	/* COMPUTE_BBOX==TAINTING */
	if ( coll->bbox ) lwgeom_add_bbox(subgeom);

	result = pglwgeom_serialize(subgeom);

	lwgeom_release((LWGEOM *)coll);
	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(result);

}


/** @brief
* 		returns 0 for points, 1 for lines, 2 for polygons, 3 for volume.
* 		returns max dimension for a collection.
*/
PG_FUNCTION_INFO_V1(LWGEOM_dimension);
Datum LWGEOM_dimension(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *lwgeom = pglwgeom_deserialize(geom);
	int dimension = -1;

	dimension = lwgeom_dimension(lwgeom);
	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(geom, 0);
	
	if ( dimension < 0 )
	{
		elog(NOTICE, "Could not compute geometry dimensions");
		PG_RETURN_NULL();
	}

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
	PG_LWGEOM *result;
	POINTARRAY *extring;
	LWGEOM *lwgeom;
	LWLINE *line;
	GBOX *bbox=NULL;
	int type = pglwgeom_get_type(geom);

	POSTGIS_DEBUG(2, "LWGEOM_exteriorring_polygon called.");

	if ( (type != POLYGONTYPE) &&
	     (type != CURVEPOLYTYPE) &&
	     (type != TRIANGLETYPE))
	{
		elog(ERROR, "ExteriorRing: geom is not a polygon");
		PG_RETURN_NULL();
	}
	
	lwgeom = pglwgeom_deserialize(geom);
	
	if( lwgeom_is_empty(lwgeom) )
	{
		PG_RETURN_NULL();
		lwgeom_free(lwgeom);
		PG_FREE_IF_COPY(geom, 0);
	}
	
	if ( lwgeom->type == POLYGONTYPE )
	{
		LWPOLY *poly = lwgeom_as_lwpoly(lwgeom);

		/* Ok, now we have a polygon. Here is its exterior ring. */
		extring = poly->rings[0];

		/*
		* This is a LWLINE constructed by exterior ring POINTARRAY
		* If the input geom has a bbox, use it for
		* the output geom, as exterior ring makes it up !
		*/
		if ( poly->bbox ) 
			bbox = gbox_copy(poly->bbox);

		line = lwline_construct(poly->srid, bbox, extring);
		result = pglwgeom_serialize((LWGEOM *)line);

		lwgeom_release((LWGEOM *)line);
	}
	else if ( lwgeom->type == TRIANGLETYPE )
	{
		LWTRIANGLE *triangle = lwgeom_as_lwtriangle(lwgeom);

		/*
		* This is a LWLINE constructed by exterior ring POINTARRAY
		* If the input geom has a bbox, use it for
		* the output geom, as exterior ring makes it up !
		*/
		if ( triangle->bbox ) 
			bbox = gbox_copy(triangle->bbox);
		line = lwline_construct(triangle->srid, bbox, triangle->points);

		result = pglwgeom_serialize((LWGEOM *)line);

		lwgeom_release((LWGEOM *)line);
	}
	else
	{
		LWCURVEPOLY *curvepoly = lwgeom_as_lwcurvepoly(lwgeom);
		result = pglwgeom_serialize(curvepoly->rings[0]);
	}

	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_POINTER(result);
}

/**
* NumInteriorRings(GEOMETRY) defined for Polygon and
* and CurvePolygon.
*
* @return the number of its interior rings (holes). NULL if not a polygon.
*/
PG_FUNCTION_INFO_V1(LWGEOM_numinteriorrings_polygon);
Datum LWGEOM_numinteriorrings_polygon(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *lwgeom = pglwgeom_deserialize(geom);
	LWPOLY *poly = NULL;
	LWCURVEPOLY *curvepoly = NULL;
	int result = -1;

	if ( lwgeom->type == POLYGONTYPE )
	{
		poly = lwgeom_as_lwpoly(lwgeom);
		result = poly->nrings - 1;
	}
	else if ( lwgeom->type == CURVEPOLYTYPE )
	{
		curvepoly = lwgeom_as_lwcurvepoly(lwgeom);
		result = curvepoly->nrings - 1;
	}
	
	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(geom, 0);
	
	if ( result < 0 )
		PG_RETURN_NULL();

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
	LWGEOM *lwgeom;
	PG_LWGEOM *result;
	GBOX *bbox = NULL;
	int type;

	POSTGIS_DEBUG(2, "LWGEOM_interierringn_polygon called.");

	wanted_index = PG_GETARG_INT32(1);
	if ( wanted_index < 1 )
	{
		/* elog(ERROR, "InteriorRingN: ring number is 1-based"); */
		PG_RETURN_NULL(); /* index out of range */
	}

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	type = pglwgeom_get_type(geom);

	if ( (type != POLYGONTYPE) && (type != CURVEPOLYTYPE) )
	{
		elog(ERROR, "InteriorRingN: geom is not a polygon");
		PG_FREE_IF_COPY(geom, 0);
		PG_RETURN_NULL();
	}
	
	lwgeom = pglwgeom_deserialize(geom);
	if( lwgeom_is_empty(lwgeom) )
	{
		lwgeom_free((LWGEOM *)poly);
		PG_FREE_IF_COPY(geom, 0);
		PG_RETURN_NULL();
	}
	
	if ( type == POLYGONTYPE)
	{
		poly = lwgeom_as_lwpoly(pglwgeom_deserialize(geom));

		/* Ok, now we have a polygon. Let's see if it has enough holes */
		if ( wanted_index >= poly->nrings )
		{
			lwgeom_free((LWGEOM *)poly);
			PG_FREE_IF_COPY(geom, 0);
			PG_RETURN_NULL();
		}

		ring = poly->rings[wanted_index];

		/* COMPUTE_BBOX==TAINTING */
		if ( poly->bbox ) 
		{
			bbox = lwalloc(sizeof(GBOX));
			ptarray_calculate_gbox_cartesian(ring, bbox);
		}

		/* This is a LWLINE constructed by interior ring POINTARRAY */
		line = lwline_construct(poly->srid, bbox, ring);


		result = pglwgeom_serialize((LWGEOM *)line);
		lwpoly_free(poly);
		lwline_release(line);
	}
	else
	{
		curvepoly = lwgeom_as_lwcurvepoly(pglwgeom_deserialize(geom));

		if (wanted_index >= curvepoly->nrings)
		{
			PG_FREE_IF_COPY(geom, 0);
			lwgeom_release((LWGEOM *)curvepoly);
			PG_RETURN_NULL();
		}

		result = pglwgeom_serialize(curvepoly->rings[wanted_index]);
		lwgeom_free((LWGEOM *)curvepoly);
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
	PG_LWGEOM *geom = (PG_LWGEOM*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int where = PG_GETARG_INT32(1);
	LWGEOM *lwgeom = pglwgeom_deserialize(geom);
	LWPOINT *lwpoint = NULL;
	int type = lwgeom->type;
	
	/* Can't handle crazy index! */
	if ( where < 1 )
		PG_RETURN_NULL();

	if ( type == LINETYPE || type == CIRCSTRINGTYPE )
	{
		/* OGC index starts at one, so we substract first. */
		lwpoint = lwline_get_lwpoint((LWLINE*)lwgeom, where - 1);
	}

	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(geom, 0);

	if ( ! lwpoint )
		PG_RETURN_NULL();

	PG_RETURN_POINTER(pglwgeom_serialize(lwpoint_as_lwgeom(lwpoint)));
}

/**
 * X(GEOMETRY) -- return X value of the point.
 * @return an error if input is not a point.
 */
PG_FUNCTION_INFO_V1(LWGEOM_x_point);
Datum LWGEOM_x_point(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	LWGEOM *lwgeom;
	LWPOINT *point = NULL;
	POINT2D p;

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	if ( pglwgeom_get_type(geom) != POINTTYPE )
		lwerror("Argument to X() must be a point");

	lwgeom = pglwgeom_deserialize(geom);
	point = lwgeom_as_lwpoint(lwgeom);
	
	if ( lwgeom_is_empty(lwgeom) )
		PG_RETURN_NULL();

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
	LWGEOM *lwgeom;
	POINT2D p;

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	if ( pglwgeom_get_type(geom) != POINTTYPE )
		lwerror("Argument to Y() must be a point");

	lwgeom = pglwgeom_deserialize(geom);
	point = lwgeom_as_lwpoint(lwgeom);
	
	if ( lwgeom_is_empty(lwgeom) )
		PG_RETURN_NULL();

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
	LWGEOM *lwgeom;
	POINT3DZ p;

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	if ( pglwgeom_get_type(geom) != POINTTYPE )
		lwerror("Argument to Z() must be a point");

	lwgeom = pglwgeom_deserialize(geom);
	point = lwgeom_as_lwpoint(lwgeom);
	
	if ( lwgeom_is_empty(lwgeom) )
		PG_RETURN_NULL();

	/* no Z in input */
	if ( ! pglwgeom_has_z(geom) ) PG_RETURN_NULL();

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
	LWGEOM *lwgeom;
	POINT3DM p;

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	if ( pglwgeom_get_type(geom) != POINTTYPE )
		lwerror("Argument to M() must be a point");

	lwgeom = pglwgeom_deserialize(geom);
	point = lwgeom_as_lwpoint(lwgeom);
	
	if ( lwgeom_is_empty(lwgeom) )
		PG_RETURN_NULL();

	/* no M in input */
	if ( ! FLAGS_GET_M(point->flags) ) PG_RETURN_NULL();

	getPoint3dm_p(point->point, 0, &p);

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_FLOAT8(p.m);
}

/** 
* ST_StartPoint(GEOMETRY) 
* @return the first point of a linestring.
* 		Return NULL if there is no LINESTRING
*/
PG_FUNCTION_INFO_V1(LWGEOM_startpoint_linestring);
Datum LWGEOM_startpoint_linestring(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *lwgeom = pglwgeom_deserialize(geom);
	LWPOINT *lwpoint = NULL;
	int type = lwgeom->type;

	if ( type == LINETYPE || type == CIRCSTRINGTYPE )
	{
		lwpoint = lwline_get_lwpoint((LWLINE*)lwgeom, 0);
	}

	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(geom, 0);

	if ( ! lwpoint )
		PG_RETURN_NULL();

	PG_RETURN_POINTER(pglwgeom_serialize(lwpoint_as_lwgeom(lwpoint)));
}

/** EndPoint(GEOMETRY) -- find the first linestring in GEOMETRY,
 * @return the last point.
 * 	Return NULL if there is no LINESTRING(..) in GEOMETRY
 */
PG_FUNCTION_INFO_V1(LWGEOM_endpoint_linestring);
Datum LWGEOM_endpoint_linestring(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *lwgeom = pglwgeom_deserialize(geom);
	LWPOINT *lwpoint = NULL;
	int type = lwgeom->type;

	if ( type == LINETYPE || type == CIRCSTRINGTYPE )
	{
		LWLINE *line = (LWLINE*)lwgeom;
		if ( line->points )
			lwpoint = lwline_get_lwpoint((LWLINE*)lwgeom, line->points->npoints - 1);
	}

	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(geom, 0);

	if ( ! lwpoint )
		PG_RETURN_NULL();

	PG_RETURN_POINTER(pglwgeom_serialize(lwpoint_as_lwgeom(lwpoint)));
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
	char *wkt = text2cstring(wkttext);
	LWGEOM_PARSER_RESULT lwg_parser_result;
	PG_LWGEOM *geom_result = NULL;
	LWGEOM *lwgeom;

	POSTGIS_DEBUG(2, "LWGEOM_from_text");
	POSTGIS_DEBUGF(3, "wkt: [%s]", wkt);

	if (lwgeom_parse_wkt(&lwg_parser_result, wkt, LW_PARSER_CHECK_ALL) == LW_FAILURE)
		PG_PARSER_ERROR(lwg_parser_result);

	lwgeom = lwg_parser_result.geom;

	if ( lwgeom->srid != SRID_UNKNOWN )
	{
		elog(WARNING, "OGC WKT expected, EWKT provided - use GeomFromEWKT() for this");
	}

	/* read user-requested SRID if any */
	if ( PG_NARGS() > 1 ) 
		lwgeom_set_srid(lwgeom, PG_GETARG_INT32(1));

	geom_result = pglwgeom_serialize(lwgeom);
	lwgeom_parser_result_free(&lwg_parser_result);

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
	int32 srid;
	PG_LWGEOM *result = NULL;

	geom = (PG_LWGEOM *)DatumGetPointer(DirectFunctionCall1(
	                                        LWGEOMFromWKB, PG_GETARG_DATUM(0)));

	if ( pglwgeom_get_srid(geom) != SRID_UNKNOWN || pglwgeom_has_z(geom) != 0 )
	{
		elog(WARNING, "OGC WKB expected, EWKB provided - use GeometryFromEWKB() for this");
	}


	/* read user-requested SRID if any */
	if ( PG_NARGS() > 1 )
	{
		srid = PG_GETARG_INT32(1);
		if ( srid != pglwgeom_get_srid(geom) )
		{
			result = pglwgeom_set_srid(geom, srid);
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
	PG_LWGEOM *geom;
	LWGEOM *lwgeom;
	char *wkt;
	size_t wkt_size;
	text *result;

	POSTGIS_DEBUG(2, "LWGEOM_asText called.");

	geom = (PG_LWGEOM*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	lwgeom = pglwgeom_deserialize(geom);

	/* Get a 2D version of the geometry if necessary */
	if ( FLAGS_NDIMS(lwgeom->flags) > 2 )
	{
		LWGEOM *lwgeom2d = lwgeom_force_2d(lwgeom);
		lwgeom_free(lwgeom);
		lwgeom = lwgeom2d;
	}

	/* Write to WKT and free the geometry */
	wkt = lwgeom_to_wkt(lwgeom, WKT_ISO, DBL_DIG, &wkt_size);
	lwgeom_free(lwgeom);

	/* Write to text and free the WKT */
	result = cstring2text(wkt);
	pfree(wkt);

	/* Return the text */
	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_TEXT_P(result);
}

/** convert LWGEOM to wkb (in BINARY format) */
PG_FUNCTION_INFO_V1(LWGEOM_asBinary);
Datum LWGEOM_asBinary(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	LWGEOM *lwgeom;
	uint8_t *wkb;
	size_t wkb_size;
	bytea *result;
	/* By default we are currently emitting OGC WKB (2D) only */
	uint8_t variant = WKB_SFSQL;

	/* Get a 2D version of the geometry */
	geom = (PG_LWGEOM*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	lwgeom = pglwgeom_deserialize(geom);

	/* Get a 2D version of the geometry if necessary */
	if ( FLAGS_NDIMS(lwgeom->flags) > 2 )
	{
		LWGEOM *lwgeom2d = lwgeom_force_2d(lwgeom);
		lwgeom_free(lwgeom);
		lwgeom = lwgeom2d;
	}

	/* If user specified endianness, respect it */
	if ( (PG_NARGS()>1) && (!PG_ARGISNULL(1)) )
	{
		text *wkb_endian = PG_GETARG_TEXT_P(1);

		if  ( ! strncmp(VARDATA(wkb_endian), "xdr", 3) ||
		      ! strncmp(VARDATA(wkb_endian), "XDR", 3) )
		{
			variant = variant | WKB_XDR;
		}
		else
		{
			variant = variant | WKB_NDR;
		}
	}
	
	/* Write to WKB and free the geometry */
	wkb = lwgeom_to_wkb(lwgeom, variant, &wkb_size);
	lwgeom_free(lwgeom);

	/* Write to text and free the WKT */
	result = palloc(wkb_size + VARHDRSZ);
	memcpy(VARDATA(result), wkb, wkb_size);
	SET_VARSIZE(result, wkb_size + VARHDRSZ);
	pfree(wkb);

	/* Return the text */
	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_BYTEA_P(result);
}



/**
 * @brief IsClosed(GEOMETRY) if geometry is a linestring then returns
 * 		startpoint == endpoint.  If its not a linestring then return NULL.
 * 		If it's a collection containing multiple linestrings,
 * @return true only if all the linestrings have startpoint=endpoint.
 */
PG_FUNCTION_INFO_V1(LWGEOM_isclosed);
Datum LWGEOM_isclosed(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *lwgeom = pglwgeom_deserialize(geom);
	int closed = lwgeom_is_closed(lwgeom);
	
	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_BOOL(closed);
}	
