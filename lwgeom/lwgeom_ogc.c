#include "postgres.h"

#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "access/gist.h"
#include "access/itup.h"
#include "access/rtree.h"

#include "fmgr.h"
#include "utils/elog.h"

#include "liblwgeom.h"
#include "lwgeom_pg.h"


//#define DEBUG

#include "wktparse.h"

// ---- SRID(geometry)
Datum LWGEOM_getSRID(PG_FUNCTION_ARGS);
// ---- SetSRID(geometry, integer)
Datum LWGEOM_setSRID(PG_FUNCTION_ARGS);
// ---- GeometryType(geometry)
Datum LWGEOM_getTYPE(PG_FUNCTION_ARGS);
// ---- NumPoints(geometry)
Datum LWGEOM_numpoints_linestring(PG_FUNCTION_ARGS);
// ---- NumGeometries(geometry)
Datum LWGEOM_numgeometries_collection(PG_FUNCTION_ARGS);
// ---- GeometryN(geometry, integer)
Datum LWGEOM_geometryn_collection(PG_FUNCTION_ARGS);
// ---- Dimension(geometry)
Datum LWGEOM_dimension(PG_FUNCTION_ARGS);
// ---- ExteriorRing(geometry)
Datum LWGEOM_exteriorring_polygon(PG_FUNCTION_ARGS);
// ---- InteriorRingN(geometry, integer)
Datum LWGEOM_interiorringn_polygon(PG_FUNCTION_ARGS);
// ---- NumInteriorRings(geometry)
Datum LWGEOM_numinteriorrings_polygon(PG_FUNCTION_ARGS);
// ---- PointN(geometry, integer)
Datum LWGEOM_pointn_linestring(PG_FUNCTION_ARGS);
// ---- X(geometry)
Datum LWGEOM_x_point(PG_FUNCTION_ARGS);
// ---- Y(geometry)
Datum LWGEOM_y_point(PG_FUNCTION_ARGS);
// ---- Z(geometry)
Datum LWGEOM_z_point(PG_FUNCTION_ARGS);
// ---- StartPoint(geometry)
Datum LWGEOM_startpoint_linestring(PG_FUNCTION_ARGS);
// ---- EndPoint(geometry)
Datum LWGEOM_endpoint_linestring(PG_FUNCTION_ARGS);
// ---- AsText(geometry)
Datum LWGEOM_asText(PG_FUNCTION_ARGS);
// ---- GeometryFromText(text, integer)
Datum LWGEOM_from_text(PG_FUNCTION_ARGS);
// ---- IsClosed(geometry)
Datum LWGEOM_isclosed_linestring(PG_FUNCTION_ARGS);

// internal
int32 lwgeom_numpoints_linestring_recursive(char *serialized);
int32 lwgeom_dimension_recursive(char *serialized);
char line_is_closed(LWLINE *line);

/*------------------------------------------------------------------*/

// getSRID(lwgeom) :: int4
PG_FUNCTION_INFO_V1(LWGEOM_getSRID);
Datum LWGEOM_getSRID(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *lwgeom = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int srid = lwgeom_getSRID (lwgeom);

	PG_RETURN_INT32(srid);
}

//setSRID(lwgeom, int4) :: lwgeom
PG_FUNCTION_INFO_V1(LWGEOM_setSRID);
Datum LWGEOM_setSRID(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *lwgeom = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int newSRID = PG_GETARG_INT32(1);
	PG_LWGEOM *result;

	result = lwgeom_setSRID(lwgeom, newSRID);

	PG_RETURN_POINTER(result);
}

//returns a string representation of this geometry's type
PG_FUNCTION_INFO_V1(LWGEOM_getTYPE);
Datum LWGEOM_getTYPE(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *lwgeom;
	char *text_ob;
	char *result;
	int32 size;
	unsigned char type;

	init_pg_func();

	lwgeom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	text_ob = lwalloc(20+4);
	result = text_ob+4;

	//type = lwgeom_getType(*(lwgeom+4));
	type = lwgeom_getType(lwgeom->type);

	memset(result, 0, 20);

	if (type == POINTTYPE)
		strcpy(result,"POINT");
	else if (type == MULTIPOINTTYPE)
		strcpy(result,"MULTIPOINT");
	else if (type == LINETYPE)
		strcpy(result,"LINESTRING");
	else if (type == MULTILINETYPE)
		strcpy(result,"MULTILINESTRING");
	else if (type == POLYGONTYPE)
		strcpy(result,"POLYGON");
	else if (type == MULTIPOLYGONTYPE)
		strcpy(result,"MULTIPOLYGON");
	else if (type == COLLECTIONTYPE)
		strcpy(result,"GEOMETRYCOLLECTION");
	else
		strcpy(result,"UNKNOWN");

	size = strlen(result) +4 ;

	memcpy(text_ob, &size,4); // size of string

	PG_RETURN_POINTER(text_ob);
}

// Find first linestring in serialized geometry and return
// the number of points in it. If no linestrings are found
// return -1.
int32
lwgeom_numpoints_linestring_recursive(char *serialized)
{
	LWGEOM_INSPECTED *inspected = lwgeom_inspect(serialized);
	int i;

	for (i=0; i<inspected->ngeometries; i++)
	{
		int32 npoints;
		int type;
		LWLINE *line=NULL;
		char *subgeom;

		line = lwgeom_getline_inspected(inspected, i);
		if (line != NULL)
		{
			return line->points->npoints;
		}

		subgeom = lwgeom_getsubgeometry_inspected(inspected, i);
		if ( subgeom == NULL )
		{
	elog(ERROR, "What ? lwgeom_getsubgeometry_inspected returned NULL??");
		}

		type = lwgeom_getType(subgeom[0]);

		// MULTILINESTRING && GEOMETRYCOLLECTION are worth checking
		if ( type != 7 && type != 5 ) continue;

		npoints = lwgeom_numpoints_linestring_recursive(subgeom);
		if ( npoints == -1 ) continue;
		return npoints;
	}

	return -1;
}

//numpoints(GEOMETRY) -- find the first linestring in GEOMETRY, return
//the number of points in it.  Return NULL if there is no LINESTRING(..)
//in GEOMETRY
PG_FUNCTION_INFO_V1(LWGEOM_numpoints_linestring);
Datum LWGEOM_numpoints_linestring(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int32 ret;
	ret = lwgeom_numpoints_linestring_recursive(SERIALIZED_FORM(geom));
	if ( ret == -1 ) PG_RETURN_NULL();
	PG_RETURN_INT32(ret);
}

PG_FUNCTION_INFO_V1(LWGEOM_numgeometries_collection);
Datum LWGEOM_numgeometries_collection(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int type;
	char *serialized = SERIALIZED_FORM(geom);

	type = lwgeom_getType(geom->type);
	if ( type >= 4 )
	{
		PG_RETURN_INT32(lwgeom_getnumgeometries(serialized));
	}
	else
	{
		PG_RETURN_NULL();
	}
}

PG_FUNCTION_INFO_V1(LWGEOM_geometryn_collection);
Datum LWGEOM_geometryn_collection(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *result;
	int type = lwgeom_getType(geom->type);
	int32 idx;
	char *serialized;
	char *subgeom;

	//elog(NOTICE, "GeometryN called");

	// call is valid on multi* geoms only
	if ( type < 4 )
	{
		//elog(NOTICE, "geometryn: geom is of type %d, requires >=4", type);
		PG_RETURN_NULL();
	}

	idx = PG_GETARG_INT32(1);
	serialized = SERIALIZED_FORM(geom);

	subgeom = lwgeom_getsubgeometry(serialized, idx);
	if ( subgeom == NULL )
	{
		//elog(NOTICE, "geometryn: subgeom %d does not exist", idx);
		PG_RETURN_NULL();
	}

	// we have it, not it's time to make an PG_LWGEOM

	// Here is the actual of the line
	result = PG_LWGEOM_construct(subgeom, lwgeom_getSRID(geom), lwgeom_hasBBOX(geom->type));

	//elog(NOTICE, "geomtryN about to return");
	PG_RETURN_POINTER(result);

}

//returns 0 for points, 1 for lines, 2 for polygons.
//returns max dimension for a collection.
//returns -1 for the empty geometry (TODO)
//returns -2 on error
int32
lwgeom_dimension_recursive(char *serialized)
{
	LWGEOM_INSPECTED *inspected;
	int ret = -1;
	int i;

	inspected = lwgeom_inspect(serialized);
	for (i=0; i<inspected->ngeometries; i++)
	{
		char *subgeom;
		char typeflags = lwgeom_getsubtype_inspected(inspected, i);
		int type = lwgeom_getType(typeflags);
		int dims=-1;

		if ( type == POINTTYPE ) dims = 0;
		else if ( type == MULTIPOINTTYPE ) dims=0;
		else if ( type == LINETYPE ) dims=1;
		else if ( type == MULTILINETYPE ) dims=1;
		else if ( type == POLYGONTYPE ) dims=2;
		else if ( type == MULTIPOLYGONTYPE ) dims=2;
		else if ( type == COLLECTIONTYPE ) 
		{
			subgeom = lwgeom_getsubgeometry_inspected(inspected, i);
			if ( subgeom == NULL ) return -2;

			dims = lwgeom_dimension_recursive(subgeom);
		}

		if ( dims == 2 ) return 2; // nothing can be higher
		if ( dims > ret ) ret = dims;
	}

	return ret;
}

//returns 0 for points, 1 for lines, 2 for polygons.
//returns max dimension for a collection.
//returns -1 for the empty geometry (TODO)
PG_FUNCTION_INFO_V1(LWGEOM_dimension);
Datum LWGEOM_dimension(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int dimension;

	dimension = lwgeom_dimension_recursive(SERIALIZED_FORM(geom));
	if ( dimension == -1 )
	{
		elog(ERROR, "Something went wrong in dimension computation");
		PG_RETURN_NULL();
	}

	PG_RETURN_INT32(dimension);
}


// exteriorRing(GEOMETRY) -- find the first polygon in GEOMETRY, return
// its exterior ring (as a linestring).
// Return NULL if there is no POLYGON(..) in (first level of) GEOMETRY.
PG_FUNCTION_INFO_V1(LWGEOM_exteriorring_polygon);
Datum LWGEOM_exteriorring_polygon(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM_INSPECTED *inspected = lwgeom_inspect(SERIALIZED_FORM(geom));
	LWPOLY *poly = NULL;
	POINTARRAY *extring;
	LWLINE *line;
	char *serializedline;
	PG_LWGEOM *result;
	int i;

	for (i=0; i<inspected->ngeometries; i++)
	{
		poly = lwgeom_getpoly_inspected(inspected, i);
		if ( poly ) break;
	}

	if ( poly == NULL ) PG_RETURN_NULL();

	// Ok, now we have a polygon. Here is its exterior ring.
	extring = poly->rings[0];

	// This is a LWLINE constructed by exterior ring POINTARRAY
	line = lwline_construct(
		TYPE_HASZ(poly->type),
		TYPE_HASM(poly->type),
		poly->SRID,
		lwgeom_hasBBOX(geom->type), extring);

	// Now we serialized it (copying data)
	serializedline = lwline_serialize(line);

	// And we construct the line (copy again)
	result = PG_LWGEOM_construct(serializedline, lwgeom_getSRID(geom),
		lwgeom_hasBBOX(geom->type));

	pfree(serializedline);
	pfree(line);

	PG_RETURN_POINTER(result);
}

// NumInteriorRings(GEOMETRY) -- find the first polygon in GEOMETRY, return
// the number of its interior rings (holes).
// Return NULL if there is no POLYGON(..) in (first level of) GEOMETRY.
PG_FUNCTION_INFO_V1(LWGEOM_numinteriorrings_polygon);
Datum LWGEOM_numinteriorrings_polygon(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM_INSPECTED *inspected = lwgeom_inspect(SERIALIZED_FORM(geom));
	LWPOLY *poly = NULL;
	int32 result;
	int i;

	for (i=0; i<inspected->ngeometries; i++)
	{
		poly = lwgeom_getpoly_inspected(inspected, i);
		if ( poly ) break;
	}

	if ( poly == NULL ) PG_RETURN_NULL();

	// Ok, now we have a polygon. Here is its number of holes
	result = poly->nrings-1;

	PG_RETURN_INT32(result);
}

// InteriorRingN(GEOMETRY) -- find the first polygon in GEOMETRY, return
// its Nth interior ring (as a linestring).
// Return NULL if there is no POLYGON(..) in (first level of) GEOMETRY.
PG_FUNCTION_INFO_V1(LWGEOM_interiorringn_polygon);
Datum LWGEOM_interiorringn_polygon(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	int32 wanted_index;
	LWGEOM_INSPECTED *inspected;
	LWPOLY *poly = NULL;
	POINTARRAY *ring;
	LWLINE *line;
	char *serializedline;
	PG_LWGEOM *result;
	int i;

	wanted_index = PG_GETARG_INT32(1);
	if ( wanted_index < 0 )
		PG_RETURN_NULL(); // index out of range

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	inspected = lwgeom_inspect(SERIALIZED_FORM(geom));

	for (i=0; i<inspected->ngeometries; i++)
	{
		poly = lwgeom_getpoly_inspected(inspected, i);
		if ( poly ) break;
	}

	if ( poly == NULL ) PG_RETURN_NULL();

	// Ok, now we have a polygon. Let's see if it has enough holes
	if ( wanted_index > poly->nrings-2 )
	{
		pfree_inspected(inspected);
		PG_RETURN_NULL();
	}
	pfree_inspected(inspected);

	ring = poly->rings[wanted_index+1];

	// This is a LWLINE constructed by exterior ring POINTARRAY
	line = lwline_construct(
		TYPE_HASZ(poly->type),
		TYPE_HASM(poly->type),
		poly->SRID,
		lwgeom_hasBBOX(geom->type), ring);

	// Now we serialized it (copying data)
	serializedline = lwline_serialize(line);

	// And we construct the line (copy again)
	result = PG_LWGEOM_construct(serializedline, lwgeom_getSRID(geom),
		lwgeom_hasBBOX(geom->type));

	pfree(serializedline);
	pfree(line);

	PG_RETURN_POINTER(result);
}

// PointN(GEOMETRY,INTEGER) -- find the first linestring in GEOMETRY,
// return the point at index INTEGER (0 is 1st point).  Return NULL if
// there is no LINESTRING(..) in GEOMETRY or INTEGER is out of bounds.
PG_FUNCTION_INFO_V1(LWGEOM_pointn_linestring);
Datum LWGEOM_pointn_linestring(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	int32 wanted_index;
	LWGEOM_INSPECTED *inspected;
	LWLINE *line = NULL;
	POINTARRAY *pts;
	LWPOINT *point;
	char *serializedpoint;
	PG_LWGEOM *result;
	int i;

	wanted_index = PG_GETARG_INT32(1);
	if ( wanted_index < 0 )
		PG_RETURN_NULL(); // index out of range

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	inspected = lwgeom_inspect(SERIALIZED_FORM(geom));

	for (i=0; i<inspected->ngeometries; i++)
	{
		line = lwgeom_getline_inspected(inspected, i);
		if ( line ) break;
	}

	if ( line == NULL ) PG_RETURN_NULL();

	// Ok, now we have a line. Let's see if it has enough points.
	if ( wanted_index > line->points->npoints-1 )
	{
		pfree_inspected(inspected);
		PG_RETURN_NULL();
	}
	pfree_inspected(inspected);

	// Construct a point array
	pts = pointArray_construct(getPoint(line->points, wanted_index),
		TYPE_HASZ(line->type), TYPE_HASM(line->type), 1);

	// Construct an LWPOINT
	point = lwpoint_construct(TYPE_HASZ(line->type), TYPE_HASM(line->type),
		lwgeom_getSRID(geom), lwgeom_hasBBOX(geom->type), pts);

	// Serialized the point
	serializedpoint = lwpoint_serialize(point);

	// And we construct the line (copy again)
	result = PG_LWGEOM_construct(serializedpoint, lwgeom_getSRID(geom),
		lwgeom_hasBBOX(geom->type));

	pfree(point);
	pfree(serializedpoint);

	PG_RETURN_POINTER(result);
}

// X(GEOMETRY) -- find the first POINT(..) in GEOMETRY, returns its X value.
// Return NULL if there is no POINT(..) in GEOMETRY
PG_FUNCTION_INFO_V1(LWGEOM_x_point);
Datum LWGEOM_x_point(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	LWGEOM_INSPECTED *inspected;
	LWPOINT *point = NULL;
	POINT2D *p;
	int i;

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	inspected = lwgeom_inspect(SERIALIZED_FORM(geom));

	for (i=0; i<inspected->ngeometries; i++)
	{
		point = lwgeom_getpoint_inspected(inspected, i);
		if ( point ) break;
	}
	pfree_inspected(inspected);

	if ( point == NULL ) PG_RETURN_NULL();

	// Ok, now we have a point, let's get X
	p = (POINT2D *)getPoint(point->point, 0);

	PG_RETURN_FLOAT8(p->x);
}

// Y(GEOMETRY) -- find the first POINT(..) in GEOMETRY, returns its Y value.
// Return NULL if there is no POINT(..) in GEOMETRY
PG_FUNCTION_INFO_V1(LWGEOM_y_point);
Datum LWGEOM_y_point(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	LWGEOM_INSPECTED *inspected;
	LWPOINT *point = NULL;
	POINT2D *p;
	int i;

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	inspected = lwgeom_inspect(SERIALIZED_FORM(geom));

	for (i=0; i<inspected->ngeometries; i++)
	{
		point = lwgeom_getpoint_inspected(inspected, i);
		if ( point ) break;
	}
	pfree_inspected(inspected);

	if ( point == NULL ) PG_RETURN_NULL();

	// Ok, now we have a point, let's get X
	p = (POINT2D *)getPoint(point->point, 0);

	PG_RETURN_FLOAT8(p->y);
}

// Z(GEOMETRY) -- find the first POINT(..) in GEOMETRY, returns its Z value.
// Return NULL if there is no POINT(..) in GEOMETRY.
// Return 0 if there is no Z in this geometry.
PG_FUNCTION_INFO_V1(LWGEOM_z_point);
Datum LWGEOM_z_point(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	LWGEOM_INSPECTED *inspected;
	LWPOINT *point = NULL;
	POINT3DZ p;
	int i;

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	// if there's no Z return 0
	if ( ! TYPE_HASZ(geom->type) ) PG_RETURN_FLOAT8(0.0);

	inspected = lwgeom_inspect(SERIALIZED_FORM(geom));

	for (i=0; i<inspected->ngeometries; i++)
	{
		point = lwgeom_getpoint_inspected(inspected, i);
		if ( point ) break;
	}
	pfree_inspected(inspected);

	if ( point == NULL ) PG_RETURN_NULL();

	// Ok, now we have a point, let's get X
	lwpoint_getPoint3dz_p(point, &p);

	PG_RETURN_FLOAT8(p.z);
}

// StartPoint(GEOMETRY) -- find the first linestring in GEOMETRY,
// return the first point.
// Return NULL if there is no LINESTRING(..) in GEOMETRY 
PG_FUNCTION_INFO_V1(LWGEOM_startpoint_linestring);
Datum LWGEOM_startpoint_linestring(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	LWGEOM_INSPECTED *inspected;
	LWLINE *line = NULL;
	POINTARRAY *pts;
	LWPOINT *point;
	char *serializedpoint;
	PG_LWGEOM *result;
	int i;

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	inspected = lwgeom_inspect(SERIALIZED_FORM(geom));

	for (i=0; i<inspected->ngeometries; i++)
	{
		line = lwgeom_getline_inspected(inspected, i);
		if ( line ) break;
	}
	pfree_inspected(inspected);

	if ( line == NULL ) PG_RETURN_NULL();

	// Ok, now we have a line. 

	// Construct a point array
	pts = pointArray_construct(getPoint(line->points, 0),
		TYPE_HASZ(line->type),
		TYPE_HASM(line->type), 1);

	// Construct an LWPOINT
	point = lwpoint_construct(
		TYPE_HASZ(line->type),
		TYPE_HASM(line->type),
		lwgeom_getSRID(geom),
		lwgeom_hasBBOX(geom->type), pts);

	// Serialized the point
	serializedpoint = lwpoint_serialize(point);

	// And we construct the line (copy again)
	result = PG_LWGEOM_construct(serializedpoint, lwgeom_getSRID(geom),
		lwgeom_hasBBOX(geom->type));

	pfree(point);
	pfree(serializedpoint);

	PG_RETURN_POINTER(result);
}

// EndPoint(GEOMETRY) -- find the first linestring in GEOMETRY,
// return the last point.
// Return NULL if there is no LINESTRING(..) in GEOMETRY 
PG_FUNCTION_INFO_V1(LWGEOM_endpoint_linestring);
Datum LWGEOM_endpoint_linestring(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	LWGEOM_INSPECTED *inspected;
	LWLINE *line = NULL;
	POINTARRAY *pts;
	LWPOINT *point;
	char *serializedpoint;
	PG_LWGEOM *result;
	int i;

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	inspected = lwgeom_inspect(SERIALIZED_FORM(geom));

	for (i=0; i<inspected->ngeometries; i++)
	{
		line = lwgeom_getline_inspected(inspected, i);
		if ( line ) break;
	}
	pfree_inspected(inspected);

	if ( line == NULL ) PG_RETURN_NULL();

	// Ok, now we have a line. 

	// Construct a point array
	pts = pointArray_construct(
		getPoint(line->points, line->points->npoints-1),
		TYPE_HASZ(line->type),
		TYPE_HASM(line->type), 1);

	// Construct an LWPOINT
	point = lwpoint_construct(
		TYPE_HASZ(line->type),
		TYPE_HASM(line->type),
		lwgeom_getSRID(geom),
		lwgeom_hasBBOX(geom->type), pts);

	// Serialized the point
	serializedpoint = lwpoint_serialize(point);

	// And we construct the line (copy again)
	result = PG_LWGEOM_construct(serializedpoint, lwgeom_getSRID(geom),
		lwgeom_hasBBOX(geom->type));

	pfree(point);
	pfree(serializedpoint);

	PG_RETURN_POINTER(result);
}

//given wkt and SRID, return a geometry
//actually we cheat, postgres will convert the string to a geometry for us...
PG_FUNCTION_INFO_V1(LWGEOM_from_text);
Datum LWGEOM_from_text(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));
	int32 SRID;
	PG_LWGEOM *result = NULL;

	// read user-requested SRID if any
#if USE_VERSION < 73
	if ( fcinfo->nargs > 1 )
#else
	if ( PG_NARGS() > 1 )
#endif
	{
		SRID = PG_GETARG_INT32(1);
		if ( SRID != lwgeom_getSRID(geom) )
		{
			result = lwgeom_setSRID(geom, SRID);
			pfree(geom);
		}
	}
	if ( ! result )	result = geom;

	PG_RETURN_POINTER(result);
}
//convert LWGEOM to wkt (in TEXT format)
PG_FUNCTION_INFO_V1(LWGEOM_asText);
Datum LWGEOM_asText(PG_FUNCTION_ARGS)
{
	char *lwgeom;
	char *result_cstring;
	int len;
        char *result,*loc_wkt;
	char *semicolonLoc;

	init_pg_func();

	lwgeom = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	result_cstring =  unparse_WKT(lwgeom,lwalloc,lwfree);

	semicolonLoc = strchr(result_cstring,';');

	//loc points to start of wkt
	if (semicolonLoc == NULL) loc_wkt = result_cstring;
	else loc_wkt = semicolonLoc +1;

	len = strlen(loc_wkt)+4;
	result = palloc(len);
	memcpy(result, &len, 4);

	memcpy(result+4,loc_wkt, len-4);

	pfree(result_cstring);
	PG_RETURN_POINTER(result);
}

char line_is_closed(LWLINE *line)
{
	POINT4D *sp, *ep;

	sp = (POINT4D *)getPoint(line->points, 0);
	ep = (POINT4D *)getPoint(line->points, line->points->npoints-1);

	if ( sp->x != ep->x ) return 0;
	if ( sp->y != ep->y ) return 0;
	if ( TYPE_HASZ(line->type) )
	{
		if ( sp->z != ep->z ) return 0;
	}

	return 1;
}

// IsClosed(GEOMETRY) if geometry is a linestring then returns
// startpoint == endpoint.  If its not a linestring then return NULL.
// If it's a collection containing multiple linestrings,
// return true only if all the linestrings have startpoint=endpoint.
PG_FUNCTION_INFO_V1(LWGEOM_isclosed_linestring);
Datum LWGEOM_isclosed_linestring(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	LWGEOM_INSPECTED *inspected;
	LWLINE *line = NULL;
	int linesfound=0;
	int i;

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	inspected = lwgeom_inspect(SERIALIZED_FORM(geom));

	for (i=0; i<inspected->ngeometries; i++)
	{
		line = lwgeom_getline_inspected(inspected, i);
		if ( line == NULL ) continue;
		if ( ! line_is_closed(line) )
		{
			pfree_inspected(inspected);
			PG_RETURN_BOOL(FALSE);
		}
		linesfound++;
	}
	pfree_inspected(inspected);

	if ( ! linesfound ) PG_RETURN_NULL();
	PG_RETURN_BOOL(TRUE);

}
