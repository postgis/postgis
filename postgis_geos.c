

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
extern bool GEOSrelatePattern(Geometry *g1, Geometry*g2,char *pat);
extern  bool GEOSrelateDisjoint(Geometry *g1, Geometry*g2);
extern  bool GEOSrelateTouches(Geometry *g1, Geometry*g2);
extern  bool GEOSrelateIntersects(Geometry *g1, Geometry*g2);
extern  bool GEOSrelateCrosses(Geometry *g1, Geometry*g2);
extern  bool GEOSrelateWithin(Geometry *g1, Geometry*g2);
extern  bool GEOSrelateContains(Geometry *g1, Geometry*g2);
extern  bool GEOSrelateOverlaps(Geometry *g1, Geometry*g2);

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

extern bool GEOSisvalid(Geometry *g1);


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

	PG_RETURN_BOOL(result);
}

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


	result = GEOSrelateOverlaps(g1,g2);

	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g2);

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


	result = GEOSrelateContains(g1,g2);

	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g2);

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


	result = GEOSrelateWithin(g1,g2);

	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g2);

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


	result = GEOSrelateCrosses(g1,g2);

	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g2);

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


	result = GEOSrelateIntersects(g1,g2);

	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g2);

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


	result = GEOSrelateTouches(g1,g2);

	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g2);

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

	errorIfGeometryCollection(geom1,geom2);

//elog(NOTICE,"in relate_full: \n");
	initGEOS(MAXIMUM_ALIGNOF);

//elog(NOTICE,"making GOES geometries: \n");
	g1 = 	POSTGIS2GEOS(geom1 );
	g2 = 	POSTGIS2GEOS(geom2 );
//	elog(NOTICE,"geometries made, here's what they are: \n");
//g1 = createGEOSFromText("POINT(1 1)");
//g2 = createGEOSFromText("POINT(1 1)");
//elog(NOTICE,GEOSasText(g1));
//elog(NOTICE,GEOSasText(g2));

//elog(NOTICE,"calling relate: \n");
	relate_str = GEOSrelate(g1, g2);

//elog(NOTICE,"relate finished \n");


	len = strlen(relate_str) + 4;

	result= palloc(len);
	*((int *) result) = len;

	memcpy(result +4, relate_str, len-4);

	free(relate_str);
//	GEOSdeleteGeometry(g1);
//	GEOSdeleteGeometry(g2);

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

	int32 *offsets1 = (int32 *) ( ((char *) &(g->objType[0] ))+ sizeof(int32) * g->nobjs ) ;

	switch(g->type)
	{
		case POINTTYPE:	
							pt = (POINT3D*) ((char *) g +offsets1[0]) ;
							return PostGIS2GEOS_point(pt,g->SRID,g->is3d);
							break;
		case LINETYPE:		
							line = (LINE3D*) ((char *) g +offsets1[0]) ;
							return PostGIS2GEOS_linestring(line,g->SRID,g->is3d);
							break;
		case POLYGONTYPE:   
							poly = (POLYGON3D*) ((char *) g +offsets1[0]) ;
							return PostGIS2GEOS_polygon(poly,g->SRID,g->is3d);
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
							return geos;
							break;
		case BBOXONLYTYPE:
							 return PostGIS2GEOS_box3d(&g->bvol, g->SRID);
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
													break;
									case LINETYPE:
													line = (LINE3D*) obj ;
													geoms[t] = PostGIS2GEOS_linestring(line,g->SRID,g->is3d);
													break;
									case POLYGONTYPE:
													poly = (POLYGON3D*) obj ;
													geoms[t] = PostGIS2GEOS_polygon(poly,g->SRID,g->is3d);
													break;
								}
							}
							geos= PostGIS2GEOS_collection(geoms,g->nobjs,g->SRID,g->is3d);
							pfree(geoms);
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

