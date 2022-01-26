/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * PostGIS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * PostGIS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PostGIS.  If not, see <http://www.gnu.org/licenses/>.
 *
 **********************************************************************
 *
 * Copyright 2001-2005 Refractions Research Inc.
 *
 **********************************************************************/


#include "postgres.h"

#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>

#include "access/gist.h"
#include "access/itup.h"

#include "fmgr.h"
#include "utils/builtins.h"
#include "utils/elog.h"

#include "../postgis_config.h"

#include "liblwgeom.h"
#include "liblwgeom_internal.h"
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
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_HEADER(0);
	int32_t srid = gserialized_get_srid(geom);
	PG_FREE_IF_COPY(geom,0);
	PG_RETURN_INT32(srid);
}

/* setSRID(lwgeom, int4) :: lwgeom */
PG_FUNCTION_INFO_V1(LWGEOM_set_srid);
Datum LWGEOM_set_srid(PG_FUNCTION_ARGS)
{
	GSERIALIZED *g = (GSERIALIZED *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));
	int32_t srid = PG_GETARG_INT32(1);
	gserialized_set_srid(g, srid);
	PG_RETURN_POINTER(g);
}

/* returns a string representation of this geometry's type */
PG_FUNCTION_INFO_V1(LWGEOM_getTYPE);
Datum LWGEOM_getTYPE(PG_FUNCTION_ARGS)
{
	GSERIALIZED *gser;
	text *text_ob;
	char *result;
	uint8_t type;
	static int maxtyplen = 20;

	gser = PG_GETARG_GSERIALIZED_HEADER(0);
	text_ob = palloc0(VARHDRSZ + maxtyplen);
	result = VARDATA(text_ob);

	type = gserialized_get_type(gser);

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

	if ( gserialized_has_m(gser) && ! gserialized_has_z(gser) )
		strcat(result, "M");

	SET_VARSIZE(text_ob, strlen(result) + VARHDRSZ); /* size of string */

	PG_FREE_IF_COPY(gser, 0);

	PG_RETURN_TEXT_P(text_ob);
}

/* Matches lwutil.c::lwgeomTypeName */
static char *stTypeName[] = {"Unknown",
			     "ST_Point",
			     "ST_LineString",
			     "ST_Polygon",
			     "ST_MultiPoint",
			     "ST_MultiLineString",
			     "ST_MultiPolygon",
			     "ST_GeometryCollection",
			     "ST_CircularString",
			     "ST_CompoundCurve",
			     "ST_CurvePolygon",
			     "ST_MultiCurve",
			     "ST_MultiSurface",
			     "ST_PolyhedralSurface",
			     "ST_Triangle",
			     "ST_Tin"};

/* returns a string representation of this geometry's type */
PG_FUNCTION_INFO_V1(geometry_geometrytype);
Datum geometry_geometrytype(PG_FUNCTION_ARGS)
{
	GSERIALIZED *gser;
	text *type_text;

	/* Read just the header from the toasted tuple */
	gser = PG_GETARG_GSERIALIZED_HEADER(0);

	/* Build a text type to store things in */
	type_text = cstring_to_text(stTypeName[gserialized_get_type(gser)]);

	PG_FREE_IF_COPY(gser, 0);
	PG_RETURN_TEXT_P(type_text);
}



/**
* numpoints(LINESTRING) -- return the number of points in the
* linestring, or NULL if it is not a linestring
*/
PG_FUNCTION_INFO_V1(LWGEOM_numpoints_linestring);
Datum LWGEOM_numpoints_linestring(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	LWGEOM *lwgeom = lwgeom_from_gserialized(geom);
	int count = -1;
	int type = lwgeom->type;

	if ( type == LINETYPE || type == CIRCSTRINGTYPE || type == COMPOUNDTYPE )
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
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	LWGEOM *lwgeom;
	int32 ret = 1;

	lwgeom = lwgeom_from_gserialized(geom);
	if (lwgeom_is_empty(lwgeom))
	{
		ret = 0;
	}
	else if (lwgeom_is_collection(lwgeom))
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
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *result;
	int type = gserialized_get_type(geom);
	int32 idx;
	LWCOLLECTION *coll;
	LWGEOM *subgeom;

	POSTGIS_DEBUG(2, "LWGEOM_geometryn_collection called.");

	/* elog(NOTICE, "GeometryN called"); */

	idx = PG_GETARG_INT32(1);
	idx -= 1; /* index is 1-based */

	if (gserialized_is_empty(geom))
	{
		PG_RETURN_NULL();
	}

	/* call is valid on multi* geoms only */
	if (type==POINTTYPE     || type==LINETYPE    || type==CIRCSTRINGTYPE ||
	    type==COMPOUNDTYPE  || type==POLYGONTYPE ||
	    type==CURVEPOLYTYPE || type==TRIANGLETYPE)
	{
		if ( idx == 0 ) PG_RETURN_POINTER(geom);
		PG_RETURN_NULL();
	}

	coll = lwgeom_as_lwcollection(lwgeom_from_gserialized(geom));

	if ( idx < 0 ) PG_RETURN_NULL();
	if ( idx >= (int32) coll->ngeoms ) PG_RETURN_NULL();

	subgeom = coll->geoms[idx];
	subgeom->srid = coll->srid;

	/* COMPUTE_BBOX==TAINTING */
	if ( coll->bbox ) lwgeom_add_bbox(subgeom);

	result = geometry_serialize(subgeom);

	lwcollection_free(coll);
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
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	LWGEOM *lwgeom = lwgeom_from_gserialized(geom);
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
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *result;
	POINTARRAY *extring;
	LWGEOM *lwgeom;
	LWLINE *line;
	GBOX *bbox=NULL;
	int type = gserialized_get_type(geom);

	POSTGIS_DEBUG(2, "LWGEOM_exteriorring_polygon called.");

	if ( (type != POLYGONTYPE) &&
	     (type != CURVEPOLYTYPE) &&
	     (type != TRIANGLETYPE))
	{
		PG_RETURN_NULL();
	}

	lwgeom = lwgeom_from_gserialized(geom);

	if( lwgeom_is_empty(lwgeom) )
	{
		line = lwline_construct_empty(lwgeom->srid,
		                              lwgeom_has_z(lwgeom),
		                              lwgeom_has_m(lwgeom));
		result = geometry_serialize(lwline_as_lwgeom(line));
	}
	else if ( type == POLYGONTYPE )
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
		result = geometry_serialize((LWGEOM *)line);

		lwgeom_release((LWGEOM *)line);
	}
	else if ( type == TRIANGLETYPE )
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

		result = geometry_serialize((LWGEOM *)line);

		lwgeom_release((LWGEOM *)line);
	}
	else
	{
		LWCURVEPOLY *curvepoly = lwgeom_as_lwcurvepoly(lwgeom);
		result = geometry_serialize(curvepoly->rings[0]);
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
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	int type = gserialized_get_type(geom);
	LWGEOM *lwgeom;
	int result = -1;

	if ( (type != POLYGONTYPE) &&
	     (type != CURVEPOLYTYPE) &&
	     (type != TRIANGLETYPE))
	{
		PG_RETURN_NULL();
	}

	lwgeom = lwgeom_from_gserialized(geom);
	if ( lwgeom_is_empty(lwgeom) )
	{
		result = 0;
	}
	else
	{
		const LWPOLY *poly = (LWPOLY*)lwgeom;
		result = poly->nrings - 1;
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
	GSERIALIZED *geom;
	int32 wanted_index;
	LWCURVEPOLY *curvepoly = NULL;
	LWPOLY *poly = NULL;
	POINTARRAY *ring;
	LWLINE *line;
	LWGEOM *lwgeom;
	GSERIALIZED *result;
	GBOX *bbox = NULL;
	int type;

	POSTGIS_DEBUG(2, "LWGEOM_interiorringn_polygon called.");

	wanted_index = PG_GETARG_INT32(1);
	if ( wanted_index < 1 )
	{
		PG_RETURN_NULL(); /* index out of range */
	}

	geom = PG_GETARG_GSERIALIZED_P(0);
	type = gserialized_get_type(geom);

	if ( (type != POLYGONTYPE) && (type != CURVEPOLYTYPE) )
	{
		PG_FREE_IF_COPY(geom, 0);
		PG_RETURN_NULL();
	}

	lwgeom = lwgeom_from_gserialized(geom);
	if( lwgeom_is_empty(lwgeom) )
	{
		lwpoly_free(poly);
		PG_FREE_IF_COPY(geom, 0);
		PG_RETURN_NULL();
	}

	if ( type == POLYGONTYPE)
	{
		poly = lwgeom_as_lwpoly(lwgeom_from_gserialized(geom));

		/* Ok, now we have a polygon. Let's see if it has enough holes */
		if ( wanted_index >= (int32)poly->nrings )
		{
			lwpoly_free(poly);
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


		result = geometry_serialize((LWGEOM *)line);
		lwline_release(line);
		lwpoly_free(poly);
	}
	else
	{
		curvepoly = lwgeom_as_lwcurvepoly(lwgeom_from_gserialized(geom));

		if (wanted_index >= (int32)curvepoly->nrings)
		{
			PG_FREE_IF_COPY(geom, 0);
			lwgeom_release((LWGEOM *)curvepoly);
			PG_RETURN_NULL();
		}

		result = geometry_serialize(curvepoly->rings[wanted_index]);
		lwgeom_free((LWGEOM*)curvepoly);
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
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	int where = PG_GETARG_INT32(1);
	LWGEOM *lwgeom = lwgeom_from_gserialized(geom);
	LWPOINT *lwpoint = NULL;
	int type = lwgeom->type;

	/* If index is negative, count backward */
	if( where < 1 )
	{
		int count = -1;
		if ( type == LINETYPE || type == CIRCSTRINGTYPE || type == COMPOUNDTYPE )
			count = lwgeom_count_vertices(lwgeom);
		if(count >0)
		{
			/* only work if we found the total point number */
			/* converting where to positive backward indexing, +1 because 1 indexing */
			where = where + count + 1;
		}
		if (where < 1)
			PG_RETURN_NULL();
	}

	if ( type == LINETYPE || type == CIRCSTRINGTYPE )
	{
		/* OGC index starts at one, so we substract first. */
		lwpoint = lwline_get_lwpoint((LWLINE*)lwgeom, where - 1);
	}
	else if ( type == COMPOUNDTYPE )
	{
		lwpoint = lwcompound_get_lwpoint((LWCOMPOUND*)lwgeom, where - 1);
	}

	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(geom, 0);

	if ( ! lwpoint )
		PG_RETURN_NULL();

	PG_RETURN_POINTER(geometry_serialize(lwpoint_as_lwgeom(lwpoint)));
}

/**
 * X(GEOMETRY) -- return X value of the point.
 * @return an error if input is not a point.
 */
PG_FUNCTION_INFO_V1(LWGEOM_x_point);
Datum LWGEOM_x_point(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	POINT4D pt;

	if (gserialized_get_type(geom) != POINTTYPE)
		lwpgerror("Argument to ST_X() must have type POINT");

	if (gserialized_peek_first_point(geom, &pt) == LW_FAILURE)
	{
		PG_RETURN_NULL();
	}
	PG_RETURN_FLOAT8(pt.x);
}

/**
 * Y(GEOMETRY) -- return Y value of the point.
 * 	Raise an error if input is not a point.
 */
PG_FUNCTION_INFO_V1(LWGEOM_y_point);
Datum LWGEOM_y_point(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	POINT4D pt;

	if (gserialized_get_type(geom) != POINTTYPE)
		lwpgerror("Argument to ST_Y() must have type POINT");

	if (gserialized_peek_first_point(geom, &pt) == LW_FAILURE)
	{
		PG_RETURN_NULL();
	}
	PG_RETURN_FLOAT8(pt.y);
}

/**
 * Z(GEOMETRY) -- return Z value of the point.
 * @return NULL if there is no Z in the point.
 * 		Raise an error if input is not a point.
 */
PG_FUNCTION_INFO_V1(LWGEOM_z_point);
Datum LWGEOM_z_point(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	POINT4D pt;

	if (gserialized_get_type(geom) != POINTTYPE)
		lwpgerror("Argument to ST_Z() must have type POINT");

	if (!gserialized_has_z(geom) || (gserialized_peek_first_point(geom, &pt) == LW_FAILURE))
	{
		PG_RETURN_NULL();
	}
	PG_RETURN_FLOAT8(pt.z);
}

/**  M(GEOMETRY) -- find the first POINT(..) in GEOMETRY, returns its M value.
 * @return NULL if there is no POINT(..) in GEOMETRY.
 * 		Return NULL if there is no M in this geometry.
 */
PG_FUNCTION_INFO_V1(LWGEOM_m_point);
Datum LWGEOM_m_point(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	POINT4D pt;

	if (gserialized_get_type(geom) != POINTTYPE)
		lwpgerror("Argument to ST_M() must have type POINT");

	if (!gserialized_has_m(geom) || (gserialized_peek_first_point(geom, &pt) == LW_FAILURE))
	{
		PG_RETURN_NULL();
	}
	PG_RETURN_FLOAT8(pt.m);
}

/**
* ST_StartPoint(GEOMETRY)
* @return the first point of a geometry.
*/
PG_FUNCTION_INFO_V1(LWGEOM_startpoint_linestring);
Datum LWGEOM_startpoint_linestring(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *ret;
	LWGEOM *lwgeom = lwgeom_from_gserialized(geom);
	LWGEOM *lwpoint = NULL;
	POINT4D pt;

	if (lwgeom_startpoint(lwgeom, &pt) == LW_FAILURE)
	{
		PG_RETURN_NULL();
	}

	lwpoint = (LWGEOM *)lwpoint_make(lwgeom->srid, lwgeom_has_z(lwgeom), lwgeom_has_m(lwgeom), &pt);
	ret = geometry_serialize(lwpoint);

	lwgeom_free(lwgeom);
	lwgeom_free(lwpoint);

	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_POINTER(ret);
}

/** EndPoint(GEOMETRY) -- find the first linestring in GEOMETRY,
 * @return the last point.
 * 	Return NULL if there is no LINESTRING(..) in GEOMETRY
 */
PG_FUNCTION_INFO_V1(LWGEOM_endpoint_linestring);
Datum LWGEOM_endpoint_linestring(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	LWGEOM *lwgeom = lwgeom_from_gserialized(geom);
	LWPOINT *lwpoint = NULL;
	int type = lwgeom->type;

	if ( type == LINETYPE || type == CIRCSTRINGTYPE )
	{
		LWLINE *line = (LWLINE*)lwgeom;
		if ( line->points )
			lwpoint = lwline_get_lwpoint((LWLINE*)lwgeom, line->points->npoints - 1);
	}
	else if ( type == COMPOUNDTYPE )
	{
		lwpoint = lwcompound_get_endpoint((LWCOMPOUND*)lwgeom);
	}

	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(geom, 0);

	if ( ! lwpoint )
		PG_RETURN_NULL();

	PG_RETURN_POINTER(geometry_serialize(lwpoint_as_lwgeom(lwpoint)));
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
	char *wkt = text_to_cstring(wkttext);
	LWGEOM_PARSER_RESULT lwg_parser_result;
	GSERIALIZED *geom_result = NULL;
	LWGEOM *lwgeom;

	POSTGIS_DEBUG(2, "LWGEOM_from_text");
	POSTGIS_DEBUGF(3, "wkt: [%s]", wkt);

	if (lwgeom_parse_wkt(&lwg_parser_result, wkt, LW_PARSER_CHECK_ALL) == LW_FAILURE )
		PG_PARSER_ERROR(lwg_parser_result);

	lwgeom = lwg_parser_result.geom;

	if ( lwgeom->srid != SRID_UNKNOWN )
	{
		elog(WARNING, "OGC WKT expected, EWKT provided - use GeomFromEWKT() for this");
	}

	/* read user-requested SRID if any */
	if ( PG_NARGS() > 1 )
		lwgeom_set_srid(lwgeom, PG_GETARG_INT32(1));

	geom_result = geometry_serialize(lwgeom);
	lwgeom_parser_result_free(&lwg_parser_result);

	PG_RETURN_POINTER(geom_result);
}

/**
 * Given an OGC WKB (and optionally a SRID)
 * 		return a geometry.
 *
 * @note that this is a wrapper around
 * 		lwgeom_from_wkb, where we throw
 *      a warning if ewkb passed in
 * 		accept EWKB.
 */
PG_FUNCTION_INFO_V1(LWGEOM_from_WKB);
Datum LWGEOM_from_WKB(PG_FUNCTION_ARGS)
{
	bytea *bytea_wkb = PG_GETARG_BYTEA_P(0);
	int32 srid = 0;
	GSERIALIZED *geom;
	LWGEOM *lwgeom;
	uint8_t *wkb = (uint8_t*)VARDATA(bytea_wkb);

	lwgeom = lwgeom_from_wkb(wkb, VARSIZE_ANY_EXHDR(bytea_wkb), LW_PARSER_CHECK_ALL);
	if (!lwgeom)
		lwpgerror("Unable to parse WKB");

	geom = geometry_serialize(lwgeom);
	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(bytea_wkb, 0);

	if ( gserialized_get_srid(geom) != SRID_UNKNOWN )
	{
		elog(WARNING, "OGC WKB expected, EWKB provided - use GeometryFromEWKB() for this");
	}

	if ( PG_NARGS() > 1 )
	{
		srid = PG_GETARG_INT32(1);
		if ( srid != gserialized_get_srid(geom) )
			gserialized_set_srid(geom, srid);
	}

	PG_RETURN_POINTER(geom);
}

/** convert LWGEOM to wkt (in TEXT format) */
PG_FUNCTION_INFO_V1(LWGEOM_asText);
Datum LWGEOM_asText(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	LWGEOM *lwgeom = lwgeom_from_gserialized(geom);

	int dbl_dig_for_wkt = OUT_DEFAULT_DECIMAL_DIGITS;
	if (PG_NARGS() > 1) dbl_dig_for_wkt = PG_GETARG_INT32(1);

	PG_RETURN_TEXT_P(lwgeom_to_wkt_varlena(lwgeom, WKT_ISO, dbl_dig_for_wkt));
}


/** convert LWGEOM to wkb (in BINARY format) */
PG_FUNCTION_INFO_V1(LWGEOM_asBinary);
Datum LWGEOM_asBinary(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom;
	LWGEOM *lwgeom;
	uint8_t variant = WKB_ISO;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	/* Get a 2D version of the geometry */
	geom = PG_GETARG_GSERIALIZED_P(0);
	lwgeom = lwgeom_from_gserialized(geom);


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
	PG_RETURN_BYTEA_P(lwgeom_to_wkb_varlena(lwgeom, variant));
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
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	LWGEOM *lwgeom = lwgeom_from_gserialized(geom);
	int closed = lwgeom_is_closed(lwgeom);

	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_BOOL(closed);
}
