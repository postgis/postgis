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


//#define PGIS_DEBUG

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
// ---- AsBinary(geometry, [XDR|NDR])
Datum LWGEOM_asBinary(PG_FUNCTION_ARGS);
// ---- GeometryFromText(text, integer)
Datum LWGEOM_from_text(PG_FUNCTION_ARGS);
// ---- GeomFromWKB(bytea, integer)
Datum LWGEOM_from_WKB(PG_FUNCTION_ARGS);
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
	PG_LWGEOM *pglwgeom=(PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int srid = pglwgeom_getSRID (pglwgeom);
	PG_FREE_IF_COPY(pglwgeom,0);
	PG_RETURN_INT32(srid);
}

//setSRID(lwgeom, int4) :: lwgeom
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

//returns a string representation of this geometry's type
PG_FUNCTION_INFO_V1(LWGEOM_getTYPE);
Datum LWGEOM_getTYPE(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *lwgeom;
	char *text_ob;
	char *result;
	int32 size;
	uchar type;

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

	if ( TYPE_HASM(lwgeom->type) && ! TYPE_HASZ(lwgeom->type) )
		strcat(result, "M");

	size = strlen(result) +4 ;

	memcpy(text_ob, &size,4); // size of string

	PG_FREE_IF_COPY(lwgeom, 0);

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

		pfree_inspected(inspected);	

		return npoints;
	}

	pfree_inspected(inspected);	

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
	char *serialized = SERIALIZED_FORM(geom);

	type = lwgeom_getType(geom->type);
	if ( type >= 4 )
	{
		ret = lwgeom_getnumgeometries(serialized);
		PG_FREE_IF_COPY(geom, 0);
		PG_RETURN_INT32(ret);
	}
	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_NULL();
}

// 1-based offset
PG_FUNCTION_INFO_V1(LWGEOM_geometryn_collection);
Datum LWGEOM_geometryn_collection(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *result;
	int type = lwgeom_getType(geom->type);
	int32 idx;
	LWCOLLECTION *coll;
	LWGEOM *subgeom;

	//elog(NOTICE, "GeometryN called");

	// call is valid on multi* geoms only
	if ( type < 4 )
	{
		//elog(NOTICE, "geometryn: geom is of type %d, requires >=4", type);
		PG_RETURN_NULL();
	}

	idx = PG_GETARG_INT32(1);
	idx -= 1; // index is 1-based

	coll = (LWCOLLECTION *)lwgeom_deserialize(SERIALIZED_FORM(geom));

	if ( idx < 0 ) PG_RETURN_NULL();
	if ( idx >= coll->ngeoms ) PG_RETURN_NULL();

	subgeom = coll->geoms[idx];
	subgeom->SRID = coll->SRID;

	//COMPUTE_BBOX==TAINTING
	if ( coll->bbox ) lwgeom_addBBOX(subgeom);

	result = pglwgeom_serialize(subgeom);

	lwgeom_release((LWGEOM *)coll);
	PG_FREE_IF_COPY(geom, 0);

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
			if ( subgeom == NULL ) {
				pfree_inspected(inspected);
				return -2;
			}

			dims = lwgeom_dimension_recursive(subgeom);
		}

		if ( dims == 2 ) { // nothing can be higher
				pfree_inspected(inspected);
				return 2;
		}
		if ( dims > ret ) ret = dims;
	}

	pfree_inspected(inspected);

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
		PG_FREE_IF_COPY(geom, 0);
		elog(ERROR, "Something went wrong in dimension computation");
		PG_RETURN_NULL();
	}

	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_INT32(dimension);
}


// exteriorRing(GEOMETRY) -- find the first polygon in GEOMETRY, return
// its exterior ring (as a linestring).
// Return NULL if there is no POLYGON(..) in (first level of) GEOMETRY.
PG_FUNCTION_INFO_V1(LWGEOM_exteriorring_polygon);
Datum LWGEOM_exteriorring_polygon(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWPOLY *poly = NULL;
	POINTARRAY *extring;
	LWLINE *line;
	PG_LWGEOM *result;
	BOX2DFLOAT4 bbox, *bbox2=NULL;

	if ( TYPE_GETTYPE(geom->type) != POLYGONTYPE )
	{
		elog(ERROR, "ExteriorRing: geom is not a polygon");
		PG_RETURN_NULL();
	}
	poly = lwpoly_deserialize(SERIALIZED_FORM(geom));

	// Ok, now we have a polygon. Here is its exterior ring.
	extring = poly->rings[0];

	// If the input geom has a bbox, use it for 
	// the output geom, as exterior ring makes it up !
	// COMPUTE_BBOX==WHEN_SIMPLE
	if ( getbox2d_p(SERIALIZED_FORM(geom), &bbox) )
	{
		bbox2 = &bbox;
	}

	// This is a LWLINE constructed by exterior ring POINTARRAY
	line = lwline_construct(poly->SRID, bbox2, extring);

	// Copy SRID from polygon
	line->SRID = poly->SRID;

	result = pglwgeom_serialize((LWGEOM *)line);

	lwgeom_release((LWGEOM *)line);
	lwgeom_release((LWGEOM *)poly);
	PG_FREE_IF_COPY(geom, 0);


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

	if ( poly == NULL ) {
		PG_FREE_IF_COPY(geom, 0);
		pfree_inspected(inspected);
		PG_RETURN_NULL();
	}

	// Ok, now we have a polygon. Here is its number of holes
	result = poly->nrings-1;

	PG_FREE_IF_COPY(geom, 0);
	pfree_inspected(inspected);
	lwgeom_release((LWGEOM *)poly);

	PG_RETURN_INT32(result);
}

// InteriorRingN(GEOMETRY) -- find the first polygon in GEOMETRY, return
// its Nth interior ring (as a linestring).
// Return NULL if there is no POLYGON(..) in (first level of) GEOMETRY.
// Index is 1-based
PG_FUNCTION_INFO_V1(LWGEOM_interiorringn_polygon);
Datum LWGEOM_interiorringn_polygon(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	int32 wanted_index;
	LWPOLY *poly = NULL;
	POINTARRAY *ring;
	LWLINE *line;
	PG_LWGEOM *result;
	BOX2DFLOAT4 *bbox = NULL;

	wanted_index = PG_GETARG_INT32(1);
	if ( wanted_index < 1 )
	{
		//elog(ERROR, "InteriorRingN: ring number is 1-based");
		PG_RETURN_NULL(); // index out of range
	}

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	if ( TYPE_GETTYPE(geom->type) != POLYGONTYPE )
	{
		PG_FREE_IF_COPY(geom, 0);
		elog(ERROR, "InteriorRingN: geom is not a polygon");
		PG_RETURN_NULL();
	}
	poly = lwpoly_deserialize(SERIALIZED_FORM(geom));

	// Ok, now we have a polygon. Let's see if it has enough holes
	if ( wanted_index >= poly->nrings )
	{
		PG_FREE_IF_COPY(geom, 0);
		lwgeom_release((LWGEOM *)poly);
		PG_RETURN_NULL();
	}

	ring = poly->rings[wanted_index];

	// COMPUTE_BBOX==TAINTING
	if ( poly->bbox ) bbox = ptarray_compute_bbox(ring);

	// This is a LWLINE constructed by interior ring POINTARRAY
	line = lwline_construct(poly->SRID, bbox, ring);

	// Copy SRID from polygon
	line->SRID = poly->SRID;

	result = pglwgeom_serialize((LWGEOM *)line);

	lwgeom_release((LWGEOM *)line);
	lwgeom_release((LWGEOM *)poly);
	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(result);
}

// PointN(GEOMETRY,INTEGER) -- find the first linestring in GEOMETRY,
// return the point at index INTEGER (1 is 1st point).  Return NULL if
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
	if ( wanted_index < 1 )
		PG_RETURN_NULL(); // index out of range

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	inspected = lwgeom_inspect(SERIALIZED_FORM(geom));

	for (i=0; i<inspected->ngeometries; i++)
	{
		line = lwgeom_getline_inspected(inspected, i);
		if ( line ) break;
	}

	if ( line == NULL ) {
		pfree_inspected(inspected);
		PG_FREE_IF_COPY(geom, 0);
		PG_RETURN_NULL();
	}

	// Ok, now we have a line. Let's see if it has enough points.
	if ( wanted_index > line->points->npoints )
	{
		pfree_inspected(inspected);
		PG_FREE_IF_COPY(geom, 0);
		lwgeom_release((LWGEOM *)line);
		PG_RETURN_NULL();
	}
	pfree_inspected(inspected);

	// Construct a point array
	pts = pointArray_construct(getPoint_internal(line->points,
		wanted_index-1),
		TYPE_HASZ(line->type), TYPE_HASM(line->type), 1);

	// Construct an LWPOINT
	point = lwpoint_construct(pglwgeom_getSRID(geom),
		NULL, pts);

	// Serialized the point
	serializedpoint = lwpoint_serialize(point);

	// And we construct the line
	// TODO: use serialize_buf above, instead ..
	result = PG_LWGEOM_construct(serializedpoint,
		pglwgeom_getSRID(geom), 0);

	pfree(point);
	pfree(serializedpoint);
	lwgeom_release((LWGEOM *)line);
	PG_FREE_IF_COPY(geom, 0);

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
	POINT2D p;
	int i;

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	inspected = lwgeom_inspect(SERIALIZED_FORM(geom));

	for (i=0; i<inspected->ngeometries; i++)
	{
		point = lwgeom_getpoint_inspected(inspected, i);
		if ( point ) break;
	}
	pfree_inspected(inspected);

	if ( point == NULL ) {
		PG_FREE_IF_COPY(geom, 0);
		PG_RETURN_NULL();
	}

	// Ok, now we have a point, let's get X
	getPoint2d_p(point->point, 0, &p);

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_FLOAT8(p.x);
}

// Y(GEOMETRY) -- find the first POINT(..) in GEOMETRY, returns its Y value.
// Return NULL if there is no POINT(..) in GEOMETRY
PG_FUNCTION_INFO_V1(LWGEOM_y_point);
Datum LWGEOM_y_point(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	LWGEOM_INSPECTED *inspected;
	LWPOINT *point = NULL;
	POINT2D p;
	int i;

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	inspected = lwgeom_inspect(SERIALIZED_FORM(geom));

	for (i=0; i<inspected->ngeometries; i++)
	{
		point = lwgeom_getpoint_inspected(inspected, i);
		if ( point ) break;
	}
	pfree_inspected(inspected);

	if ( point == NULL ) {
		PG_FREE_IF_COPY(geom, 0);
		PG_RETURN_NULL();
	}

	// Ok, now we have a point, let's get X
	getPoint2d_p(point->point, 0, &p);

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_FLOAT8(p.y);
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

	if ( point == NULL ) {
		PG_FREE_IF_COPY(geom, 0);
		PG_RETURN_NULL();
	}

	// Ok, now we have a point, let's get X
	lwpoint_getPoint3dz_p(point, &p);

	PG_FREE_IF_COPY(geom, 0);

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

	if ( line == NULL ) {
		PG_FREE_IF_COPY(geom, 0);
		PG_RETURN_NULL();
	}

	// Ok, now we have a line. 

	// Construct a point array
	pts = pointArray_construct(getPoint_internal(line->points, 0),
		TYPE_HASZ(line->type),
		TYPE_HASM(line->type), 1);

	// Construct an LWPOINT
	point = lwpoint_construct(pglwgeom_getSRID(geom), NULL, pts);

	// Construct a PG_LWGEOM
	result = pglwgeom_serialize((LWGEOM *)point);

	lwgeom_release((LWGEOM *)line);
	lwgeom_release((LWGEOM *)point);
	PG_FREE_IF_COPY(geom, 0);

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
	LWGEOM *point;
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

	if ( line == NULL ) {
		PG_FREE_IF_COPY(geom, 0);
		PG_RETURN_NULL();
	}


	// Ok, now we have a line. 

	// Construct a point array
	pts = pointArray_construct(
		getPoint_internal(line->points, line->points->npoints-1),
		TYPE_HASZ(line->type),
		TYPE_HASM(line->type), 1);

	// Construct an LWPOINT
	point = (LWGEOM *)lwpoint_construct(pglwgeom_getSRID(geom), NULL, pts);

	// Serialize an PG_LWGEOM
	result = pglwgeom_serialize(point);

	lwgeom_release(point);
	lwgeom_release((LWGEOM *)line);
	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(result);
}

//given wkt and SRID, return a geometry
//actually we cheat, postgres will convert the string to a geometry for us...
PG_FUNCTION_INFO_V1(LWGEOM_from_text);
Datum LWGEOM_from_text(PG_FUNCTION_ARGS)
{
	text *wkttext = PG_GETARG_TEXT_P(0);
	char *wkt, fc;
	size_t size;
	PG_LWGEOM *geom;
	PG_LWGEOM *result = NULL;
	LWGEOM *lwgeom;

	size = VARSIZE(wkttext)-VARHDRSZ;

	//lwnotice("size: %d", size);

	if ( size < 10 )
	{
		lwerror("Invalid OGC WKT (too short)");
		PG_RETURN_NULL();
	}

	fc=*(VARDATA(wkttext));
	// 'S' is accepted for backward compatibility, a WARNING
	// is issued later.
	if ( fc!='P' && fc!='L' && fc!='M' && fc!='G' && fc!='S' )
	{
		lwerror("Invalid OGC WKT (does not start with P,L,M or G)");
		PG_RETURN_NULL();
	}

	wkt = lwalloc(size+1);
	memcpy(wkt, VARDATA(wkttext), size);
	wkt[size]='\0';

	//lwnotice("wkt: [%s]", wkt);

	geom = (PG_LWGEOM *)parse_lwgeom_wkt(wkt);

	lwgeom = lwgeom_deserialize(SERIALIZED_FORM(geom));

	if ( lwgeom->SRID != -1 || TYPE_GETZM(lwgeom->type) != 0 )
	{
		elog(WARNING, "OGC WKT expected, EWKT provided - use GeomFromEWKT() for this");
	}

	// read user-requested SRID if any
	if ( PG_NARGS() > 1 ) lwgeom->SRID = PG_GETARG_INT32(1);

	result = pglwgeom_serialize(lwgeom);

	pfree(geom);
	lwgeom_release(lwgeom);

	PG_RETURN_POINTER(result);
}

//given wkb and SRID, return a geometry
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


	// read user-requested SRID if any
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

//convert LWGEOM to wkt (in TEXT format)
PG_FUNCTION_INFO_V1(LWGEOM_asText);
Datum LWGEOM_asText(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *lwgeom;
	PG_LWGEOM *ogclwgeom;
	char *result_cstring;
	int len;
        char *result,*loc_wkt;
	char *semicolonLoc;

	init_pg_func();

	lwgeom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* Force to 2d */
	ogclwgeom = (PG_LWGEOM *)DatumGetPointer(DirectFunctionCall1(
		LWGEOM_force_2d, PointerGetDatum(lwgeom)));

	result_cstring =  unparse_WKT(SERIALIZED_FORM(ogclwgeom),lwalloc,lwfree);

	semicolonLoc = strchr(result_cstring,';');

	//loc points to start of wkt
	if (semicolonLoc == NULL) loc_wkt = result_cstring;
	else loc_wkt = semicolonLoc +1;

	len = strlen(loc_wkt)+4;
	result = palloc(len);
	memcpy(result, &len, 4);

	memcpy(result+4,loc_wkt, len-4);

	pfree(result_cstring);
	PG_FREE_IF_COPY(lwgeom, 0);

	PG_RETURN_POINTER(result);
}

//convert LWGEOM to wkb (in BINARY format)
PG_FUNCTION_INFO_V1(LWGEOM_asBinary);
Datum LWGEOM_asBinary(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *ogclwgeom;
	char *result;

	init_pg_func();

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
			lwgeom_release((LWGEOM *)line);
			pfree_inspected(inspected);
			PG_FREE_IF_COPY(geom, 0);
			PG_RETURN_BOOL(FALSE);
		}
		lwgeom_release((LWGEOM *)line);
		linesfound++;
	}
	pfree_inspected(inspected);

	if ( ! linesfound ) {
			PG_FREE_IF_COPY(geom, 0);
			PG_RETURN_NULL();
	}
	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_BOOL(TRUE);

}
