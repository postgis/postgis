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
 * $Log$
 * Revision 1.6  2003/08/05 18:27:21  dblasby
 * Added null implementations of new GEOS-returning-geometry functions (ie.
 * buffer).
 *
 * Revision 1.5  2003/08/01 23:58:08  dblasby
 * added the functionality to convert GEOS->PostGIS geometries.  Added those geos
 * functions to postgis.
 *
 * Revision 1.4  2003/07/01 18:30:55  pramsey
 * Added CVS revision headers.
 *
 *
 **********************************************************************/


//--------------------------------------------------------------------------
//
#ifdef USE_GEOS

#include "postgres.h"


#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "fmgr.h"

#include "postgis.h"
#include "utils/elog.h"

#include "access/gist.h"
#include "access/itup.h"
#include "access/rtree.h"

#include "utils/builtins.h"



typedef  struct Geometry Geometry;


extern const char * createGEOSPoint(POINT3D *pt);
extern void initGEOS(int maxalign);
extern char *GEOSrelate(Geometry *g1, Geometry*g2);
extern char GEOSrelatePattern(Geometry *g1, Geometry*g2,char *pat);
extern  char GEOSrelateDisjoint(Geometry *g1, Geometry*g2);
extern  char GEOSrelateTouches(Geometry *g1, Geometry*g2);
extern  char GEOSrelateIntersects(Geometry *g1, Geometry*g2);
extern  char GEOSrelateCrosses(Geometry *g1, Geometry*g2);
extern  char GEOSrelateWithin(Geometry *g1, Geometry*g2);
extern  char GEOSrelateContains(Geometry *g1, Geometry*g2);
extern  char GEOSrelateOverlaps(Geometry *g1, Geometry*g2);

extern char *GEOSasText(Geometry *g1);
extern char GEOSisEmpty(Geometry *g1);
extern char *GEOSGeometryType(Geometry *g1);


extern void GEOSdeleteChar(char *a);
extern void GEOSdeleteGeometry(Geometry *a);

extern  Geometry *PostGIS2GEOS_point(POINT3D *point,int SRID, bool is3d);
extern  Geometry *PostGIS2GEOS_linestring(LINE3D *line,int SRID, bool is3d);
extern  Geometry *PostGIS2GEOS_polygon(POLYGON3D *polygon,int SRID, bool is3d);
extern  Geometry *PostGIS2GEOS_multipolygon(POLYGON3D **polygons,int npolys, int SRID, bool is3d);
extern Geometry *PostGIS2GEOS_multilinestring(LINE3D **lines,int nlines, int SRID, bool is3d);
extern Geometry *PostGIS2GEOS_multipoint(POINT3D **points,int npoints, int SRID, bool is3d);
extern Geometry *PostGIS2GEOS_box3d(BOX3D *box, int SRID);
extern Geometry *PostGIS2GEOS_collection(Geometry **geoms, int ngeoms,int SRID, bool is3d);

extern char GEOSisvalid(Geometry *g1);



extern char *throw_exception(Geometry *g);
extern Geometry *GEOSIntersection(Geometry *g1, Geometry *g2);


extern POINT3D  *GEOSGetCoordinate(Geometry *g1);
extern POINT3D  *GEOSGetCoordinates(Geometry *g1);
extern int      GEOSGetNumCoordinate(Geometry *g1);
extern Geometry *GEOSGetGeometryN(Geometry *g1, int n);
extern Geometry *GEOSGetExteriorRing(Geometry *g1);
extern Geometry *GEOSGetInteriorRingN(Geometry *g1,int n);
extern int      GEOSGetNumInteriorRings(Geometry *g1);
extern int      GEOSGetSRID(Geometry *g1);
extern int      GEOSGetNumGeometries(Geometry *g1);

extern  Geometry *GEOSBuffer(Geometry *g1,double width);
extern  Geometry *GEOSConvexHull(Geometry *g1);
extern  Geometry *GEOSDifference(Geometry *g1,Geometry *g2);
extern  Geometry *GEOSBoundary(Geometry *g1);
extern  Geometry *GEOSSymDifference(Geometry *g1,Geometry *g2);
extern  Geometry *GEOSUnion(Geometry *g1,Geometry *g2);




Datum relate_full(PG_FUNCTION_ARGS);
Datum relate_pattern(PG_FUNCTION_ARGS);
Datum disjoint(PG_FUNCTION_ARGS);
Datum touches(PG_FUNCTION_ARGS);
Datum intersects(PG_FUNCTION_ARGS);
Datum crosses(PG_FUNCTION_ARGS);
Datum within(PG_FUNCTION_ARGS);
Datum contains(PG_FUNCTION_ARGS);
Datum overlaps(PG_FUNCTION_ARGS);
Datum isvalid(PG_FUNCTION_ARGS);


Datum buffer(PG_FUNCTION_ARGS);
Datum intersection(PG_FUNCTION_ARGS);
Datum convexhull(PG_FUNCTION_ARGS);
Datum difference(PG_FUNCTION_ARGS);
Datum boundary(PG_FUNCTION_ARGS);
Datum symdifference(PG_FUNCTION_ARGS);
Datum geomunion(PG_FUNCTION_ARGS);

Geometry *POSTGIS2GEOS(GEOMETRY *g);
void errorIfGeometryCollection(GEOMETRY *g1, GEOMETRY *g2);
GEOMETRY *GEOS2POSTGIS(Geometry *g, char want3d );

POLYGON3D *PolyFromGeometry(Geometry *g, int *size);
LINE3D *LineFromGeometry(Geometry *g, int *size);


//-----------------------------------------------
// return a GEOS Geometry from a POSTGIS GEOMETRY
//----------------------------------------------


//select geomunion('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))','POLYGON((5 5, 15 5, 15 7, 5 7, 5 5))');
PG_FUNCTION_INFO_V1(geomunion);
Datum geomunion(PG_FUNCTION_ARGS)
{
		GEOMETRY		*geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		GEOMETRY		*geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

		Geometry *g1,*g2,*g3;
		GEOMETRY *result;
		char empty;

		initGEOS(MAXIMUM_ALIGNOF);

		g1 = 	POSTGIS2GEOS(geom1 );
		g2 = 	POSTGIS2GEOS(geom2 );
		g3 =    GEOSUnion(g1,g2);

	if (g3 == NULL)
	{
		elog(ERROR,"GEOS union() threw an error!");
		GEOSdeleteGeometry(g1);
		GEOSdeleteGeometry(g2);
		PG_RETURN_NULL(); //never get here
	}


	empty = GEOSisEmpty(g3);
	if (empty ==2)
	{
		GEOSdeleteGeometry(g1);
		GEOSdeleteGeometry(g2);
		GEOSdeleteGeometry(g3);
		elog(ERROR,"GEOS union() threw an error (couldnt test empty on result)!");
		PG_RETURN_NULL(); //never get here
	}
	if (empty)
	{
		PG_RETURN_NULL();
	}

//	elog(NOTICE,"result: %s", GEOSasText(g3) ) ;

	result = GEOS2POSTGIS(g3, geom1->is3d || geom2->is3d);
	if (result == NULL)
	{
		GEOSdeleteGeometry(g1);
		GEOSdeleteGeometry(g2);
		GEOSdeleteGeometry(g3);
		elog(ERROR,"GEOS union() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); //never get here
	}



		GEOSdeleteGeometry(g1);
		GEOSdeleteGeometry(g2);
		GEOSdeleteGeometry(g3);

		PG_RETURN_POINTER(result);
}


// select symdifference('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))','POLYGON((5 5, 15 5, 15 7, 5 7, 5 5))');
PG_FUNCTION_INFO_V1(symdifference);
Datum symdifference(PG_FUNCTION_ARGS)
{
		GEOMETRY		*geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		GEOMETRY		*geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

		Geometry *g1,*g2,*g3;
		GEOMETRY *result;
		char empty;

		initGEOS(MAXIMUM_ALIGNOF);

		g1 = 	POSTGIS2GEOS(geom1 );
		g2 = 	POSTGIS2GEOS(geom2 );
		g3 =    GEOSSymDifference(g1,g2);

	if (g3 == NULL)
	{
		elog(ERROR,"GEOS symdifference() threw an error!");
		GEOSdeleteGeometry(g1);
		GEOSdeleteGeometry(g2);
		PG_RETURN_NULL(); //never get here
	}


	empty = GEOSisEmpty(g3);
	if (empty ==2)
	{
		GEOSdeleteGeometry(g1);
		GEOSdeleteGeometry(g2);
		GEOSdeleteGeometry(g3);
		elog(ERROR,"GEOS symdifference() threw an error (couldnt test empty on result)!");
		PG_RETURN_NULL(); //never get here
	}
	if (empty)
	{
		PG_RETURN_NULL();
	}

//	elog(NOTICE,"result: %s", GEOSasText(g3) ) ;

	result = GEOS2POSTGIS(g3, geom1->is3d || geom2->is3d);
	if (result == NULL)
	{
		GEOSdeleteGeometry(g1);
		GEOSdeleteGeometry(g2);
		GEOSdeleteGeometry(g3);
		elog(ERROR,"GEOS symdifference() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); //never get here
	}



		GEOSdeleteGeometry(g1);
		GEOSdeleteGeometry(g2);
		GEOSdeleteGeometry(g3);

		PG_RETURN_POINTER(result);
}


PG_FUNCTION_INFO_V1(boundary);
Datum boundary(PG_FUNCTION_ARGS)
{
		GEOMETRY		*geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

		Geometry *g1,*g3;
		GEOMETRY *result;
		char empty;

		initGEOS(MAXIMUM_ALIGNOF);

		g1 = 	POSTGIS2GEOS(geom1 );
		g3 =    GEOSBoundary(g1);

	if (g3 == NULL)
	{
		elog(ERROR,"GEOS bounary() threw an error!");
		GEOSdeleteGeometry(g1);
		PG_RETURN_NULL(); //never get here
	}


	empty = GEOSisEmpty(g3);
	if (empty ==2)
	{
		GEOSdeleteGeometry(g1);
		GEOSdeleteGeometry(g3);
		elog(ERROR,"GEOS bounary() threw an error (couldnt test empty on result)!");
		PG_RETURN_NULL(); //never get here
	}
	if (empty)
	{
		PG_RETURN_NULL();
	}

//	elog(NOTICE,"result: %s", GEOSasText(g3) ) ;

	result = GEOS2POSTGIS(g3, geom1->is3d);
	if (result == NULL)
	{
		GEOSdeleteGeometry(g1);

		GEOSdeleteGeometry(g3);
		elog(ERROR,"GEOS bounary() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); //never get here
	}



		GEOSdeleteGeometry(g1);
		GEOSdeleteGeometry(g3);

		PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(convexhull);
Datum convexhull(PG_FUNCTION_ARGS)
{
		GEOMETRY		*geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		char empty;
		Geometry *g1,*g3;
		GEOMETRY *result;

		initGEOS(MAXIMUM_ALIGNOF);

		g1 = 	POSTGIS2GEOS(geom1 );
		g3 =    GEOSConvexHull(g1);


	if (g3 == NULL)
	{
		elog(ERROR,"GEOS convexhull() threw an error!");
		GEOSdeleteGeometry(g1);
		PG_RETURN_NULL(); //never get here
	}

	empty = GEOSisEmpty(g3);
	if (empty ==2)
	{
		GEOSdeleteGeometry(g1);
		GEOSdeleteGeometry(g3);
		elog(ERROR,"GEOS convexhull() threw an error (couldnt test empty on result)!");
		PG_RETURN_NULL(); //never get here
	}
	if (empty)
	{
		PG_RETURN_NULL();
	}

//	elog(NOTICE,"result: %s", GEOSasText(g3) ) ;

	result = GEOS2POSTGIS(g3, geom1->is3d);
	if (result == NULL)
	{
		GEOSdeleteGeometry(g1);
		GEOSdeleteGeometry(g3);
		elog(ERROR,"GEOS convexhull() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); //never get here
	}
		GEOSdeleteGeometry(g1);
		GEOSdeleteGeometry(g3);

		PG_RETURN_POINTER(result);

}

PG_FUNCTION_INFO_V1(buffer);
Datum buffer(PG_FUNCTION_ARGS)
{
		GEOMETRY		*geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		double			size   = PG_GETARG_FLOAT8(1);
		char empty;
		Geometry *g1,*g3;
		GEOMETRY *result;

		initGEOS(MAXIMUM_ALIGNOF);

		g1 = 	POSTGIS2GEOS(geom1 );
		g3 =    GEOSBuffer(g1,size);


	if (g3 == NULL)
	{
		elog(ERROR,"GEOS buffer() threw an error!");
		GEOSdeleteGeometry(g1);
		PG_RETURN_NULL(); //never get here
	}

	empty = GEOSisEmpty(g3);
	if (empty ==2)
	{
		GEOSdeleteGeometry(g1);
		GEOSdeleteGeometry(g3);
		elog(ERROR,"GEOS buffer() threw an error (couldnt test empty on result)!");
		PG_RETURN_NULL(); //never get here
	}
	if (empty)
	{
		PG_RETURN_NULL();
	}

//	elog(NOTICE,"result: %s", GEOSasText(g3) ) ;

	result = GEOS2POSTGIS(g3, geom1->is3d);
	if (result == NULL)
	{
		GEOSdeleteGeometry(g1);
		GEOSdeleteGeometry(g3);
		elog(ERROR,"GEOS buffer() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); //never get here
	}
		GEOSdeleteGeometry(g1);
		GEOSdeleteGeometry(g3);

		PG_RETURN_POINTER(result);

}


//select intersection('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))','POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))');
//select intersection('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))','POLYGON((5 5, 15 5, 15 7, 5 7, 5 5))');
//select intersection('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))','POLYGON((25 5, 35 5, 35 7, 25 7, 25 5))');
//select intersection('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))','POINT(5 5)');
//select intersection('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))','LINESTRING(5 5, 10 10)');
//select intersection('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))','MULTIPOINT(5 5, 7 7, 9 9, 10 10, 11 11)');
//select intersection('POLYGON(( 0 0, 10 0, 10 10, 0 10, 0 0))','POLYGON((5 5, 15 5, 15 7, 5 7, 5 5 ),(6 6,6.5 6, 6.5 6.5,6 6.5,6 6))');


////select intersection('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))','MULTIPOINT(5 5, 7 7, 9 9, 10 10, 11 11)');
// select intersection('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))','MULTILINESTRING((5 5, 10 10),(1 1, 2 2) )');
//select intersection('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))','MULTILINESTRING((5 5, 10 10),(1 1, 2 2) )');
//select intersection('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))','MULTIPOLYGON(((5 5, 15 5, 15 7, 5 7, 5 5)),((1 1,1 2,2 2,1 2, 1 1)))');
PG_FUNCTION_INFO_V1(intersection);
Datum intersection(PG_FUNCTION_ARGS)
{
		GEOMETRY		*geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		GEOMETRY		*geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

		Geometry *g1,*g2,*g3;
		GEOMETRY *result;
		char empty;

		initGEOS(MAXIMUM_ALIGNOF);

		g1 = 	POSTGIS2GEOS(geom1 );
		g2 = 	POSTGIS2GEOS(geom2 );
		g3 =    GEOSIntersection(g1,g2);

	if (g3 == NULL)
	{
		elog(ERROR,"GEOS Intersection() threw an error!");
		GEOSdeleteGeometry(g1);
		GEOSdeleteGeometry(g2);
		PG_RETURN_NULL(); //never get here
	}


	empty = GEOSisEmpty(g3);
	if (empty ==2)
	{
		GEOSdeleteGeometry(g1);
		GEOSdeleteGeometry(g2);
		GEOSdeleteGeometry(g3);
		elog(ERROR,"GEOS Intersection() threw an error (couldnt test empty on result)!");
		PG_RETURN_NULL(); //never get here
	}
	if (empty)
	{
		PG_RETURN_NULL();
	}

//	elog(NOTICE,"result: %s", GEOSasText(g3) ) ;

	result = GEOS2POSTGIS(g3, geom1->is3d || geom2->is3d);
	if (result == NULL)
	{
		GEOSdeleteGeometry(g1);
		GEOSdeleteGeometry(g2);
		GEOSdeleteGeometry(g3);
		elog(ERROR,"GEOS Intersection() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); //never get here
	}



		GEOSdeleteGeometry(g1);
		GEOSdeleteGeometry(g2);
		GEOSdeleteGeometry(g3);

		PG_RETURN_POINTER(result);
}

//select difference('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))','POLYGON((5 5, 15 5, 15 7, 5 7, 5 5))');
PG_FUNCTION_INFO_V1(difference);
Datum difference(PG_FUNCTION_ARGS)
{
		GEOMETRY		*geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		GEOMETRY		*geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

		Geometry *g1,*g2,*g3;
		GEOMETRY *result;
		char empty;

		initGEOS(MAXIMUM_ALIGNOF);

		g1 = 	POSTGIS2GEOS(geom1 );
		g2 = 	POSTGIS2GEOS(geom2 );
		g3 =    GEOSDifference(g1,g2);

	if (g3 == NULL)
	{
		elog(ERROR,"GEOS difference() threw an error!");
		GEOSdeleteGeometry(g1);
		GEOSdeleteGeometry(g2);
		PG_RETURN_NULL(); //never get here
	}


	empty = GEOSisEmpty(g3);
	if (empty ==2)
	{
		GEOSdeleteGeometry(g1);
		GEOSdeleteGeometry(g2);
		GEOSdeleteGeometry(g3);
		elog(ERROR,"GEOS difference() threw an error (couldnt test empty on result)!");
		PG_RETURN_NULL(); //never get here
	}
	if (empty)
	{
		PG_RETURN_NULL();
	}

//	elog(NOTICE,"result: %s", GEOSasText(g3) ) ;

	result = GEOS2POSTGIS(g3, geom1->is3d || geom2->is3d);
	if (result == NULL)
	{
		GEOSdeleteGeometry(g1);
		GEOSdeleteGeometry(g2);
		GEOSdeleteGeometry(g3);
		elog(ERROR,"GEOS difference() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); //never get here
	}



		GEOSdeleteGeometry(g1);
		GEOSdeleteGeometry(g2);
		GEOSdeleteGeometry(g3);

		PG_RETURN_POINTER(result);
}



//----------------------------------------------



void errorIfGeometryCollection(GEOMETRY *g1, GEOMETRY *g2)
{
	if (  (g1->type == COLLECTIONTYPE) || (g2->type == COLLECTIONTYPE) )
		elog(ERROR,"Relate Operation called with a GEOMETRYCOLLECTION type.  This is unsupported");
}

PG_FUNCTION_INFO_V1(isvalid);
Datum isvalid(PG_FUNCTION_ARGS)
{
		GEOMETRY		*geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		bool result;
		Geometry *g1;

		initGEOS(MAXIMUM_ALIGNOF);

		g1 = 	POSTGIS2GEOS(geom1 );

		result = GEOSisvalid(g1);
		GEOSdeleteGeometry(g1);
	if (result == 2)
	{
		elog(ERROR,"GEOS isvalid() threw an error!");
		PG_RETURN_NULL(); //never get here
	}


	PG_RETURN_BOOL(result);
}


// overlaps(GEOMETRY g1,GEOMETRY g2)
// returns  if GEOS::g1->overlaps(g2) returns true
// throws an error (elog(ERROR,...)) if GEOS throws an error
PG_FUNCTION_INFO_V1(overlaps);
Datum overlaps(PG_FUNCTION_ARGS)
{
	GEOMETRY		*geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY		*geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));


	Geometry *g1,*g2;
	bool result;

	errorIfGeometryCollection(geom1,geom2);
	initGEOS(MAXIMUM_ALIGNOF);

	g1 = 	POSTGIS2GEOS(geom1 );
	g2 = 	POSTGIS2GEOS(geom2 );

	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g2);

	result = GEOSrelateOverlaps(g1,g2);
	if (result == 2)
	{
		elog(ERROR,"GEOS overlaps() threw an error!");
		PG_RETURN_NULL(); //never get here
	}

	PG_RETURN_BOOL(result);
}



PG_FUNCTION_INFO_V1(contains);
Datum contains(PG_FUNCTION_ARGS)
{
	GEOMETRY		*geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY		*geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));


	Geometry *g1,*g2;
	bool result;

	errorIfGeometryCollection(geom1,geom2);
	initGEOS(MAXIMUM_ALIGNOF);

	g1 = 	POSTGIS2GEOS(geom1 );
	g2 = 	POSTGIS2GEOS(geom2 );

	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g2);

	result = GEOSrelateContains(g1,g2);
	if (result == 2)
	{
		elog(ERROR,"GEOS contains() threw an error!");
		PG_RETURN_NULL(); //never get here
	}



	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(within);
Datum within(PG_FUNCTION_ARGS)
{
	GEOMETRY		*geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY		*geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));


	Geometry *g1,*g2;
	bool result;

	errorIfGeometryCollection(geom1,geom2);
	initGEOS(MAXIMUM_ALIGNOF);

	g1 = 	POSTGIS2GEOS(geom1 );
	g2 = 	POSTGIS2GEOS(geom2 );

	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g2);
	result = GEOSrelateWithin(g1,g2);
	if (result == 2)
	{
		elog(ERROR,"GEOS within() threw an error!");
		PG_RETURN_NULL(); //never get here
	}



	PG_RETURN_BOOL(result);
}



PG_FUNCTION_INFO_V1(crosses);
Datum crosses(PG_FUNCTION_ARGS)
{
	GEOMETRY		*geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY		*geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));


	Geometry *g1,*g2;
	bool result;

	errorIfGeometryCollection(geom1,geom2);
	initGEOS(MAXIMUM_ALIGNOF);

	g1 = 	POSTGIS2GEOS(geom1 );
	g2 = 	POSTGIS2GEOS(geom2 );

	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g2);

	result = GEOSrelateCrosses(g1,g2);
	if (result == 2)
	{
		elog(ERROR,"GEOS crosses() threw an error!");
		PG_RETURN_NULL(); //never get here
	}



	PG_RETURN_BOOL(result);
}



PG_FUNCTION_INFO_V1(intersects);
Datum intersects(PG_FUNCTION_ARGS)
{
	GEOMETRY		*geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY		*geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));


	Geometry *g1,*g2;
	bool result;

	errorIfGeometryCollection(geom1,geom2);
	initGEOS(MAXIMUM_ALIGNOF);

	g1 = 	POSTGIS2GEOS(geom1 );
	g2 = 	POSTGIS2GEOS(geom2 );

	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g2);

	result = GEOSrelateIntersects(g1,g2);
	if (result == 2)
	{
		elog(ERROR,"GEOS intersects() threw an error!");
		PG_RETURN_NULL(); //never get here
	}



	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(touches);
Datum touches(PG_FUNCTION_ARGS)
{
	GEOMETRY		*geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY		*geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));


	Geometry *g1,*g2;
	bool result;

	errorIfGeometryCollection(geom1,geom2);
	initGEOS(MAXIMUM_ALIGNOF);

	g1 = 	POSTGIS2GEOS(geom1 );
	g2 = 	POSTGIS2GEOS(geom2 );

	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g2);

	result = GEOSrelateTouches(g1,g2);
	if (result == 2)
	{
		elog(ERROR,"GEOS touches() threw an error!");
		PG_RETURN_NULL(); //never get here
	}



	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(disjoint);
Datum disjoint(PG_FUNCTION_ARGS)
{
	GEOMETRY		*geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY		*geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));


	Geometry *g1,*g2;
	bool result;

	errorIfGeometryCollection(geom1,geom2);
	initGEOS(MAXIMUM_ALIGNOF);

	g1 = 	POSTGIS2GEOS(geom1 );
	g2 = 	POSTGIS2GEOS(geom2 );


	result = GEOSrelateDisjoint(g1,g2);
	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g2);

	if (result == 2)
	{
		elog(ERROR,"GEOS disjoin() threw an error!");
		PG_RETURN_NULL(); //never get here
	}

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(relate_pattern);
Datum relate_pattern(PG_FUNCTION_ARGS)
{
	GEOMETRY		*geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY		*geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	char *patt;
	bool result;

	Geometry *g1,*g2;


	errorIfGeometryCollection(geom1,geom2);
	initGEOS(MAXIMUM_ALIGNOF);

	g1 = 	POSTGIS2GEOS(geom1 );
	g2 = 	POSTGIS2GEOS(geom2 );


	patt =  DatumGetCString(DirectFunctionCall1(textout,
                        PointerGetDatum(PG_GETARG_DATUM(2))));

	result = GEOSrelatePattern(g1,g2,patt);
	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g2);
	pfree(patt);

	if (result == 2)
	{
		elog(ERROR,"GEOS relate_pattern() threw an error!");
		PG_RETURN_NULL(); //never get here
	}

	PG_RETURN_BOOL(result);


}



PG_FUNCTION_INFO_V1(relate_full);
Datum relate_full(PG_FUNCTION_ARGS)
{
	GEOMETRY		*geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY		*geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	Geometry *g1,*g2;
	char	*relate_str;
	int len;
	char *result;

//elog(NOTICE,"in relate_full()");

	errorIfGeometryCollection(geom1,geom2);


	initGEOS(MAXIMUM_ALIGNOF);

//elog(NOTICE,"GEOS init()");

	g1 = 	POSTGIS2GEOS(geom1 );
	g2 = 	POSTGIS2GEOS(geom2 );

//elog(NOTICE,"constructed geometries ");





if ((g1==NULL) || (g2 == NULL))
	elog(NOTICE,"g1 or g2 are null");

//elog(NOTICE,GEOSasText(g1));
//elog(NOTICE,GEOSasText(g2));

//elog(NOTICE,"valid g1 = %i", GEOSisvalid(g1));
//elog(NOTICE,"valid g2 = %i",GEOSisvalid(g2));

//elog(NOTICE,"about to relate()");


		relate_str = GEOSrelate(g1, g2);

//elog(NOTICE,"finished relate()");

	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g2);



	if (relate_str == NULL)
	{
		//free(relate_str);
		elog(ERROR,"GEOS relate() threw an error!");
		PG_RETURN_NULL(); //never get here
	}


	len = strlen(relate_str) + 4;

	result= palloc(len);
	*((int *) result) = len;

	memcpy(result +4, relate_str, len-4);

	free(relate_str);


	PG_RETURN_POINTER(result);
}


POLYGON3D *PolyFromGeometry(Geometry *g, int *size)
{

		int ninteriorrings =GEOSGetNumInteriorRings(g);
		POINT3D *pts;
		int		*pts_per_ring;
		int t;
		POLYGON3D *poly;
		int npoints;


		npoints = 	GEOSGetNumCoordinate(g);
		pts = GEOSGetCoordinates(g);
		if (npoints <3)
		{
			GEOSdeleteChar( (char*) pts);
			return NULL;
		}

		pts_per_ring = palloc(sizeof(int) *  (ninteriorrings+1));
		pts_per_ring[0] = GEOSGetNumCoordinate(GEOSGetExteriorRing(g));

		for (t=0;t<ninteriorrings;t++)
		{
			pts_per_ring[t+1] = GEOSGetNumCoordinate(GEOSGetInteriorRingN(g,t));
		}



		poly = make_polygon( ninteriorrings+1, pts_per_ring,
							pts, GEOSGetNumCoordinate(g), size);



		GEOSdeleteChar( (char*) pts);
		return poly;

}



LINE3D *LineFromGeometry(Geometry *g,int *size)
{
		POINT3D *pts = GEOSGetCoordinates(g);

		LINE3D	*line;
		int npoints = GEOSGetNumCoordinate(g);

		if (npoints <2)
		{
			GEOSdeleteChar( (char*) pts);
			return NULL;
		}

		line = make_line(npoints, pts, size);
		GEOSdeleteChar( (char*) pts);
		return line;
}




GEOMETRY *GEOS2POSTGIS(Geometry *g,char want3d)
{
	char *type = GEOSGeometryType(g) ;
	GEOMETRY *result = NULL;

	if (strcmp(type,"Point") ==0 )
	{
		POINT3D *pt = GEOSGetCoordinate(g);

		result = make_oneobj_geometry(sizeof(POINT3D),
						        (char *) pt,
							   POINTTYPE,  want3d, GEOSGetSRID(g),1.0, 0.0, 0.0
						);


		GEOSdeleteChar( (char*) pt);
		return result;
	}
	else if (strcmp(type,"LineString")==0)
	{
		LINE3D	*line;
		int size;

		line = LineFromGeometry(g,&size);
		if (line == NULL)
			return NULL;


		result = make_oneobj_geometry(size,
						        (char *) line,
							   LINETYPE,  want3d, GEOSGetSRID(g),1.0, 0.0, 0.0
						);
		return result;
	}
	else if (strcmp(type,"Polygon")==0)
	{

		int size;
		POLYGON3D *poly;



		poly = PolyFromGeometry(g,&size);
		if (poly == NULL)
			return NULL;

		result = make_oneobj_geometry(size,
						        (char *) poly,
							   POLYGONTYPE,  want3d, GEOSGetSRID(g),1.0, 0.0, 0.0
						);
		return result;
	}
	else if (strcmp(type,"MultiPoint")==0)
	{
		int ngeoms,t;
		POINT3D *pts;
		GEOMETRY *g_new,*g_old;

		g_new = NULL;

		ngeoms = 	GEOSGetNumGeometries(g);
		if (ngeoms ==0)
		{
			return NULL;
		}
		pts = GEOSGetCoordinates(g);

		g_old = make_oneobj_geometry(sizeof(POINT3D),
						        (char *) pts,
							   MULTIPOINTTYPE,  want3d, GEOSGetSRID(g),1.0, 0.0, 0.0
						);

		for (t=1;t<ngeoms;t++)
		{
			g_new=	add_to_geometry(g_old,sizeof(POINT3D), (char*)&pts[t], POINTTYPE);
			pfree(g_old);
			g_old =g_new;
		}

		GEOSdeleteChar( (char*) pts);
		return g_new;
	}
	else if (strcmp(type,"MultiLineString")==0)
	{
		int ngeoms,t,size;
		LINE3D *line;
		GEOMETRY *g_old;


		ngeoms = 	GEOSGetNumGeometries(g);
		if (ngeoms ==0)
		{
			return NULL;
		}
		for (t=0;t<ngeoms;t++)
		{
			if (t==0)
			{
				line = LineFromGeometry(GEOSGetGeometryN(g,0) ,&size);
				result = make_oneobj_geometry(size,
						        (char *) line,
							   MULTILINETYPE,  want3d, GEOSGetSRID(g),1.0, 0.0, 0.0
						);
			}
			else
			{
				line = LineFromGeometry(GEOSGetGeometryN(g,t) ,&size);
				g_old = result;
				result = 	add_to_geometry(g_old,size, (char*) line, LINETYPE);
				pfree(g_old);
			}
		}
		return result;

	}
	else if (strcmp(type,"MultiPolygon")==0)
	{
		int ngeoms,t,size;
		POLYGON3D *poly;
		GEOMETRY *g_old;


		ngeoms = 	GEOSGetNumGeometries(g);
		if (ngeoms ==0)
		{
			return NULL;
		}
		for (t=0;t<ngeoms;t++)
		{
			if (t==0)
			{
				poly = PolyFromGeometry(GEOSGetGeometryN(g,0) ,&size);
				result = make_oneobj_geometry(size,
						        (char *) poly,
							   MULTIPOLYGONTYPE,  want3d, GEOSGetSRID(g),1.0, 0.0, 0.0
						);
			}
			else
			{
				poly = PolyFromGeometry(GEOSGetGeometryN(g,t) ,&size);
				g_old = result;
				result = 	add_to_geometry(g_old,size, (char*) poly, POLYGONTYPE);
				pfree(g_old);
			}
		}
		return result;
	}
	else if (strcmp(type,"GeometryCollection")==0)
	{
		//this is more difficult because GEOS allows GCs of GCs
		int ngeoms = 	GEOSGetNumGeometries(g);
		GEOMETRY *geom, *g2, *r;
		int t;

		if (ngeoms ==0)
		{
			return NULL;
		}
		if (ngeoms == 1)
		{
			return GEOS2POSTGIS(GEOSGetGeometryN(g,0) , want3d);  // short cut!
		}
		geom = GEOS2POSTGIS(GEOSGetGeometryN(g,0) , want3d);
		for (t=1;t<ngeoms;t++)
		{
			g2 = GEOS2POSTGIS(GEOSGetGeometryN(g,t) , want3d);
			r = geom;
			geom = (GEOMETRY *)
				DatumGetPointer(
						DirectFunctionCall2(collector,
								PointerGetDatum(geom),PointerGetDatum(g2)
							)
						);
			pfree(r);
			pfree(g2);
		}
		return geom;
	}
	return NULL;
}



//BBOXONLYTYPE -> returns as a 2d polygon
Geometry *POSTGIS2GEOS(GEOMETRY *g)
{
	POINT3D *pt;
	LINE3D *line;
	POLYGON3D *poly;
	POLYGON3D **polys;
	LINE3D **lines;
	POINT3D **points;
	Geometry **geoms;
	Geometry *geos;
	char     *obj;
	int      obj_type;
	int t;
	Geometry	*result;

	int32 *offsets1 = (int32 *) ( ((char *) &(g->objType[0] ))+ sizeof(int32) * g->nobjs ) ;

	switch(g->type)
	{
		case POINTTYPE:
							pt = (POINT3D*) ((char *) g +offsets1[0]) ;
							result =  PostGIS2GEOS_point(pt,g->SRID,g->is3d);
							if (result == NULL)
							{
								elog(ERROR,"Couldnt convert the postgis geometry to GEOS!");
							}
							return result;
							break;
		case LINETYPE:
							line = (LINE3D*) ((char *) g +offsets1[0]) ;
							result =  PostGIS2GEOS_linestring(line,g->SRID,g->is3d);
							if (result == NULL)
							{
								elog(ERROR,"Couldnt convert the postgis geometry to GEOS!");
							}
							return result;
							break;
		case POLYGONTYPE:
							poly = (POLYGON3D*) ((char *) g +offsets1[0]) ;
							result =  PostGIS2GEOS_polygon(poly,g->SRID,g->is3d);
							if (result == NULL)
							{
								elog(ERROR,"Couldnt convert the postgis geometry to GEOS!");
							}
							return result;
							break;
		case MULTIPOLYGONTYPE:
									//make an array of POLYGON3Ds
							polys = (POLYGON3D**) palloc(sizeof (POLYGON3D*) * g->nobjs);
							for (t=0;t<g->nobjs;t++)
							{
								polys[t] = 	(POLYGON3D*) ((char *) g +offsets1[t]) ;
							}
							geos= PostGIS2GEOS_multipolygon(polys, g->nobjs, g->SRID,g->is3d);
							pfree(polys);
							if (geos == NULL)
							{
								elog(ERROR,"Couldnt convert the postgis geometry to GEOS!");
							}
							return geos;
							break;
		case MULTILINETYPE:
								//make an array of POLYGON3Ds
							lines = (LINE3D**) palloc(sizeof (LINE3D*) * g->nobjs);
							for (t=0;t<g->nobjs;t++)
							{
								lines[t] = 	(LINE3D*) ((char *) g +offsets1[t]) ;
							}
							geos= PostGIS2GEOS_multilinestring(lines, g->nobjs, g->SRID,g->is3d);
							pfree(lines);
							if (geos == NULL)
							{
								elog(ERROR,"Couldnt convert the postgis geometry to GEOS!");
							}
							return geos;
							break;
		case MULTIPOINTTYPE:
								//make an array of POINT3Ds
							points = (POINT3D**) palloc(sizeof (POINT3D*) * g->nobjs);
							for (t=0;t<g->nobjs;t++)
							{
								points[t] = 	(POINT3D*) ((char *) g +offsets1[t]) ;
							}
							geos= PostGIS2GEOS_multipoint(points, g->nobjs,g->SRID,g->is3d);
							pfree(points);
							if (geos == NULL)
							{
								elog(ERROR,"Couldnt convert the postgis geometry to GEOS!");
							}
							return geos;
							break;
		case BBOXONLYTYPE:
							result =   PostGIS2GEOS_box3d(&g->bvol, g->SRID);
							if (result == NULL)
							{
								elog(ERROR,"Couldnt convert the postgis geometry to GEOS!");
							}
							return result;
							break;
		case COLLECTIONTYPE:
								//make an array of GEOS Geometrys
							geoms = (Geometry**) palloc(sizeof (Geometry*) * g->nobjs);
							for (t=0;t<g->nobjs;t++)
							{
								obj = ((char *) g +offsets1[t]);
								obj_type =  g->objType[t];
								switch (obj_type)
								{
									case POINTTYPE:
													pt = (POINT3D*) obj ;
													geoms[t] = PostGIS2GEOS_point(pt,g->SRID,g->is3d);
													if (geoms[t] == NULL)
													{
														pfree(geoms);
														elog(ERROR,"Couldnt convert the postgis geometry to GEOS!");
														return NULL;
													}
													break;
									case LINETYPE:
													line = (LINE3D*) obj ;
													geoms[t] = PostGIS2GEOS_linestring(line,g->SRID,g->is3d);
													if (geoms[t] == NULL)
													{
														pfree(geoms);
														elog(ERROR,"Couldnt convert the postgis geometry to GEOS!");
														return NULL;
													}
													break;
									case POLYGONTYPE:
													poly = (POLYGON3D*) obj ;
													geoms[t] = PostGIS2GEOS_polygon(poly,g->SRID,g->is3d);
													if (geoms[t] == NULL)
													{
														pfree(geoms);
														elog(ERROR,"Couldnt convert the postgis geometry to GEOS!");
														return NULL;
													}
													break;
								}
							}
							geos= PostGIS2GEOS_collection(geoms,g->nobjs,g->SRID,g->is3d);
							pfree(geoms);
							if (geos == NULL)
							{
								elog(ERROR,"Couldnt convert the postgis geometry to GEOS!");
							}
							return geos;
							break;

	}
	return NULL;
}




//----------------------------------------------------------------------------
// NULL implementation here
// ---------------------------------------------------------------------------
#else


#include "postgres.h"


#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "fmgr.h"


Datum relate_full(PG_FUNCTION_ARGS);
Datum relate_pattern(PG_FUNCTION_ARGS);
Datum disjoint(PG_FUNCTION_ARGS);
Datum touches(PG_FUNCTION_ARGS);
Datum intersects(PG_FUNCTION_ARGS);
Datum crosses(PG_FUNCTION_ARGS);
Datum within(PG_FUNCTION_ARGS);
Datum contains(PG_FUNCTION_ARGS);
Datum overlaps(PG_FUNCTION_ARGS);
Datum isvalid(PG_FUNCTION_ARGS);


Datum buffer(PG_FUNCTION_ARGS);
Datum intersection(PG_FUNCTION_ARGS);
Datum convexhull(PG_FUNCTION_ARGS);
Datum difference(PG_FUNCTION_ARGS);
Datum boundary(PG_FUNCTION_ARGS);
Datum symdifference(PG_FUNCTION_ARGS);
Datum geomunion(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(intersection);
Datum intersection(PG_FUNCTION_ARGS)
{
	elog(ERROR,"intersection:: operation not implemented - compile PostGIS with GEOS support");
	PG_RETURN_NULL(); // never get here
}
PG_FUNCTION_INFO_V1(convexhull);
Datum convexhull(PG_FUNCTION_ARGS)
{
	elog(ERROR,"convexhull:: operation not implemented - compile PostGIS with GEOS support");
	PG_RETURN_NULL(); // never get here
}
PG_FUNCTION_INFO_V1(difference);
Datum difference(PG_FUNCTION_ARGS)
{
	elog(ERROR,"difference:: operation not implemented - compile PostGIS with GEOS support");
	PG_RETURN_NULL(); // never get here
}
PG_FUNCTION_INFO_V1(boundary);
Datum boundary(PG_FUNCTION_ARGS)
{
	elog(ERROR,"boundary:: operation not implemented - compile PostGIS with GEOS support");
	PG_RETURN_NULL(); // never get here
}
PG_FUNCTION_INFO_V1(symdifference);
Datum symdifference(PG_FUNCTION_ARGS)
{
	elog(ERROR,"symdifference:: operation not implemented - compile PostGIS with GEOS support");
	PG_RETURN_NULL(); // never get here
}
PG_FUNCTION_INFO_V1(geomunion);
Datum geomunion(PG_FUNCTION_ARGS)
{
	elog(ERROR,"geomunion:: operation not implemented - compile PostGIS with GEOS support");
	PG_RETURN_NULL(); // never get here
}

PG_FUNCTION_INFO_V1(buffer);
Datum buffer(PG_FUNCTION_ARGS)
{
	elog(ERROR,"buffer:: operation not implemented - compile PostGIS with GEOS support");
	PG_RETURN_NULL(); // never get here
}



PG_FUNCTION_INFO_V1(relate_full);
Datum relate_full(PG_FUNCTION_ARGS)
{
	elog(ERROR,"relate_full:: operation not implemented - compile PostGIS with GEOS support");
	PG_RETURN_NULL(); // never get here
}
PG_FUNCTION_INFO_V1(relate_pattern);
Datum relate_pattern(PG_FUNCTION_ARGS)
{
	elog(ERROR,"relate_pattern:: operation not implemented - compile PostGIS with GEOS support");
	PG_RETURN_NULL(); // never get here
}
PG_FUNCTION_INFO_V1(disjoint);
Datum disjoint(PG_FUNCTION_ARGS)
{
	elog(ERROR,"disjoint:: operation not implemented - compile PostGIS with GEOS support");
	PG_RETURN_NULL(); // never get here
}
PG_FUNCTION_INFO_V1(intersects);
Datum intersects(PG_FUNCTION_ARGS)
{
	elog(ERROR,"intersects:: operation not implemented - compile PostGIS with GEOS support");
	PG_RETURN_NULL(); // never get here
}
PG_FUNCTION_INFO_V1(touches);
Datum touches(PG_FUNCTION_ARGS)
{
	elog(ERROR,"touches:: operation not implemented - compile PostGIS with GEOS support");
	PG_RETURN_NULL(); // never get here
}
PG_FUNCTION_INFO_V1(crosses);
Datum crosses(PG_FUNCTION_ARGS)
{
	elog(ERROR,"crosses:: operation not implemented - compile PostGIS with GEOS support");
	PG_RETURN_NULL(); // never get here
}
PG_FUNCTION_INFO_V1(within);
Datum within(PG_FUNCTION_ARGS)
{
	elog(ERROR,"within:: operation not implemented - compile PostGIS with GEOS support");
	PG_RETURN_NULL(); // never get here
}
PG_FUNCTION_INFO_V1(contains);
Datum contains(PG_FUNCTION_ARGS)
{
	elog(ERROR,"contains:: operation not implemented - compile PostGIS with GEOS support");
	PG_RETURN_NULL(); // never get here
}
PG_FUNCTION_INFO_V1(overlaps);
Datum overlaps(PG_FUNCTION_ARGS)
{
	elog(ERROR,"overlaps:: operation not implemented - compile PostGIS with GEOS support");
	PG_RETURN_NULL(); // never get here
}
PG_FUNCTION_INFO_V1(isvalid);
Datum isvalid(PG_FUNCTION_ARGS)
{
	elog(ERROR,"isvalid:: operation not implemented - compile PostGIS with GEOS support");
	PG_RETURN_NULL(); // never get here
}

#endif

