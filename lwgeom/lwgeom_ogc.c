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


#include "lwgeom.h"


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


// internal
int32 lwgeom_numpoints_linestring_recursive(char *serialized);
int32 lwgeom_dimension_recursive(char *serialized);

/*------------------------------------------------------------------*/

// getSRID(lwgeom) :: int4
PG_FUNCTION_INFO_V1(LWGEOM_getSRID);
Datum LWGEOM_getSRID(PG_FUNCTION_ARGS)
{
	LWGEOM *lwgeom = (LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int srid = lwgeom_getSRID (lwgeom);

	PG_RETURN_INT32(srid);
}

//setSRID(lwgeom, int4) :: lwgeom
PG_FUNCTION_INFO_V1(LWGEOM_setSRID);
Datum LWGEOM_setSRID(PG_FUNCTION_ARGS)
{
	LWGEOM *lwgeom = (LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int newSRID = PG_GETARG_INT32(1);
	LWGEOM *result;

	result = lwgeom_setSRID(lwgeom, newSRID);

	PG_RETURN_POINTER(result);
}

//returns a string representation of this geometry's type
PG_FUNCTION_INFO_V1(LWGEOM_getTYPE);
Datum LWGEOM_getTYPE(PG_FUNCTION_ARGS)
{
	LWGEOM *lwgeom = (LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	char *text_ob = palloc(20+4);
	char *result = text_ob+4;
	int32 size;
	unsigned char type;

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
	LWGEOM *geom = (LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int32 ret;
	ret = lwgeom_numpoints_linestring_recursive(SERIALIZED_FORM(geom));
	if ( ret == -1 ) PG_RETURN_NULL();
	PG_RETURN_INT32(ret);
}

PG_FUNCTION_INFO_V1(LWGEOM_numgeometries_collection);
Datum LWGEOM_numgeometries_collection(PG_FUNCTION_ARGS)
{
	LWGEOM *geom = (LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
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
	LWGEOM *geom = (LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *result;
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

	// we have it, not it's time to make an LWGEOM

	// Here is the actual of the line
	result = LWGEOM_construct(subgeom, lwgeom_getSRID(geom), lwgeom_hasBBOX(geom->type));

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
	LWGEOM *geom = (LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
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
	LWGEOM *geom = (LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM_INSPECTED *inspected = lwgeom_inspect(SERIALIZED_FORM(geom));
	LWPOLY *poly = NULL;
	POINTARRAY *extring;
	LWLINE *line;
	char *serializedline;
	LWGEOM *result;
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
	line = lwline_construct(poly->ndims, poly->SRID, extring);

	// Now we serialized it (copying data)
	serializedline = lwline_serialize(line);

	// And we construct the line (copy again)
	result = LWGEOM_construct(serializedline, poly->SRID,
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
	LWGEOM *geom = (LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
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
	LWGEOM *geom;
	int32 wanted_index;
	LWGEOM_INSPECTED *inspected;
	LWPOLY *poly = NULL;
	POINTARRAY *ring;
	LWLINE *line;
	char *serializedline;
	LWGEOM *result;
	int i;

	wanted_index = PG_GETARG_INT32(1);
	if ( wanted_index < 0 )
		PG_RETURN_NULL(); // index out of range

	geom = (LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
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

	ring = poly->rings[wanted_index+1];

	// This is a LWLINE constructed by exterior ring POINTARRAY
	line = lwline_construct(poly->ndims, poly->SRID, ring);

	// Now we serialized it (copying data)
	serializedline = lwline_serialize(line);

	// And we construct the line (copy again)
	result = LWGEOM_construct(serializedline, poly->SRID,
		lwgeom_hasBBOX(geom->type));

	pfree(serializedline);
	pfree(line);

	PG_RETURN_POINTER(result);
}

