
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
 * Revision 1.4  2003/07/01 18:30:55  pramsey
 * Added CVS revision headers.
 *
 *
 **********************************************************************/

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
 

Geometry *POSTGIS2GEOS(GEOMETRY *g);
void errorIfGeometryCollection(GEOMETRY *g1, GEOMETRY *g2);

// return a GEOS Geometry from a POSTGIS GEOMETRY

 
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

