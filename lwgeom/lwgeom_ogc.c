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


#define DEBUG

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


// internal
int32 lwgeom_numpoints_linestring_recursive(char *serialized);

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
	int size;
	int type = lwgeom_getType(geom->type);
	int32 idx;
	char *serialized;
	char *subgeom;

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
	size = lwgeom_seralizedformlength_simple(subgeom);
	result = palloc(size);
	memcpy(result, &size, 4);
	memcpy(SERIALIZED_FORM(result), subgeom, size);
	PG_RETURN_POINTER(result);

}

