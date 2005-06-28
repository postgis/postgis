
#include "postgres.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "fmgr.h"

#include "lwgeom_pg.h"
#include "liblwgeom.h"
#include "profile.h"
#include "wktparse.h"

// Fuzzy way of finding out how many points to stuff
// in each chunk: 680 * Mb of memory
//
// The example below is for about 32 MB (fuzzy pragmatic check)
//
#define UNITE_USING_BUFFER 1
#define MAXGEOMSPOINTS 21760

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
Datum unite_garray(PG_FUNCTION_ARGS);
Datum issimple(PG_FUNCTION_ARGS);
Datum isring(PG_FUNCTION_ARGS);
Datum geomequals(PG_FUNCTION_ARGS);
Datum pointonsurface(PG_FUNCTION_ARGS);
Datum GEOSnoop(PG_FUNCTION_ARGS);
Datum postgis_geos_version(PG_FUNCTION_ARGS);
Datum postgis_jts_version(PG_FUNCTION_ARGS);
Datum centroid(PG_FUNCTION_ARGS);
Datum polygonize_garray(PG_FUNCTION_ARGS);



/*
 * Define this to have have many notices printed
 * during postgis->geos and geos->postgis conversions
 */
//#define PGIS_DEBUG_CONVERTER 1
#ifdef PGIS_DEBUG_CONVERTER
#define PGIS_DEBUG_POSTGIS2GEOS 1
#define PGIS_DEBUG_GEOS2POSTGIS 1
#endif // PGIS_DEBUG_CONVERTER
//#define PGIS_DEBUG 1

typedef  struct Geometry Geometry;

extern const char * createGEOSPoint(POINT3D *pt);
extern void initGEOS(int maxalign);
extern char *GEOSrelate(Geometry *g1, Geometry*g2);
extern char GEOSrelatePattern(Geometry *g1, Geometry*g2,char *pat);
extern char GEOSrelateDisjoint(Geometry *g1, Geometry*g2);
extern char GEOSrelateTouches(Geometry *g1, Geometry*g2);
extern char GEOSrelateIntersects(Geometry *g1, Geometry*g2);
extern char GEOSrelateCrosses(Geometry *g1, Geometry*g2);
extern char GEOSrelateWithin(Geometry *g1, Geometry*g2);
extern char GEOSrelateContains(Geometry *g1, Geometry*g2);
extern char GEOSrelateOverlaps(Geometry *g1, Geometry*g2);
extern char *GEOSasText(Geometry *g1);
extern char GEOSisEmpty(Geometry *g1);
extern char *GEOSGeometryType(Geometry *g1);
extern int GEOSGeometryTypeId(Geometry *g1);
extern char *GEOSversion();
extern char *GEOSjtsport();
extern char GEOSisvalid(Geometry *g1);
extern Geometry *GEOSIntersection(Geometry *g1, Geometry *g2);
extern Geometry *GEOSBuffer(Geometry *g1,double width, int quadsegs);
extern Geometry *GEOSConvexHull(Geometry *g1);
extern Geometry *GEOSDifference(Geometry *g1,Geometry *g2);
extern Geometry *GEOSBoundary(Geometry *g1);
extern Geometry *GEOSSymDifference(Geometry *g1,Geometry *g2);
extern Geometry *GEOSUnion(Geometry *g1,Geometry *g2);
extern char GEOSequals(Geometry *g1, Geometry*g2);
extern char GEOSisSimple(Geometry *g1);
extern char GEOSisRing(Geometry *g1);
extern Geometry *GEOSpointonSurface(Geometry *g1);
extern Geometry *GEOSGetCentroid(Geometry *g, int *failure);
extern bool GEOSHasZ(Geometry *g1);

extern void GEOSSetSRID(Geometry *g, int SRID);
extern void GEOSdeleteChar(char *a);
extern void GEOSdeleteGeometry(Geometry *a);

extern POINT3D  *GEOSGetCoordinate(Geometry *g1);
extern POINT3D  *GEOSGetCoordinates(Geometry *g1);
extern int      GEOSGetNumCoordinate(Geometry *g1);
extern Geometry	*GEOSGetGeometryN(Geometry *g1, int n);
extern Geometry	*GEOSGetExteriorRing(Geometry *g1);
extern Geometry	*GEOSGetInteriorRingN(Geometry *g1,int n);
extern Geometry *GEOSpolygonize(Geometry **geoms, unsigned int ngeoms);
extern int	GEOSGetNumInteriorRings(Geometry *g1);
extern int      GEOSGetSRID(Geometry *g1);
extern int      GEOSGetNumGeometries(Geometry *g1);

extern Geometry *PostGIS2GEOS_point(const LWPOINT *point);
extern Geometry *PostGIS2GEOS_linestring(const LWLINE *line);
extern Geometry *PostGIS2GEOS_polygon(const LWPOLY *polygon);
extern Geometry *PostGIS2GEOS_multipolygon(LWPOLY **geoms, uint32 ngeoms, int SRID, int is3d);
extern Geometry *PostGIS2GEOS_multilinestring(LWLINE **geoms, uint32 ngeoms, int SRID, int is3d);
extern Geometry *PostGIS2GEOS_multipoint(LWPOINT **geoms, uint32 ngeoms, int SRID, int is3d);
extern Geometry *PostGIS2GEOS_collection(int type, Geometry **geoms, int ngeoms, int SRID, bool is3d);

void NOTICE_MESSAGE(char *msg);
PG_LWGEOM *GEOS2POSTGIS(Geometry *geom, char want3d);
Geometry * POSTGIS2GEOS(PG_LWGEOM *g);
Geometry * LWGEOM2GEOS(LWGEOM *g);
void errorIfGeometryCollection(PG_LWGEOM *g1, PG_LWGEOM *g2);
LWPOINT *lwpoint_from_geometry(Geometry *g, char want3d);
LWLINE *lwline_from_geometry(Geometry *g, char want3d);
LWPOLY *lwpoly_from_geometry(Geometry *g, char want3d);
LWCOLLECTION *lwcollection_from_geometry(Geometry *geom, char want3d);
LWGEOM *lwgeom_from_geometry(Geometry *g, char want3d);

void NOTICE_MESSAGE(char *msg)
{
	elog(NOTICE,msg);
}

PG_FUNCTION_INFO_V1(postgis_geos_version);
Datum postgis_geos_version(PG_FUNCTION_ARGS)
{
	char *ver = GEOSversion();
	text *result;
	result = (text *) palloc(VARHDRSZ  + strlen(ver));
	VARATT_SIZEP(result) = VARHDRSZ + strlen(ver) ;
	memcpy(VARDATA(result), ver, strlen(ver));
	free(ver);
	PG_RETURN_POINTER(result);
}



#ifndef UNITE_USING_BUFFER
/*
 * This is the final function for GeomUnion
 * aggregate. Will have as input an array of Geometries.
 * Will iteratively call GEOSUnion on the GEOS-converted
 * versions of them and return PGIS-converted version back.
 * Changing combination order *might* speed up performance.
 */
PG_FUNCTION_INFO_V1(unite_garray);
Datum unite_garray(PG_FUNCTION_ARGS)
{
	Datum datum;
	ArrayType *array;
	int is3d = 0;
	int nelems, i;
	PG_LWGEOM *result, *pgis_geom;
	Geometry *g1, *g2, *geos_result=NULL;
	int SRID=-1;
	size_t offset;
#ifdef PGIS_DEBUG
	static int call=1;
#endif

#ifdef PGIS_DEBUG
	call++;
#endif

	//lwnotice("GEOS incremental union");

	datum = PG_GETARG_DATUM(0);

	/* Null array, null geometry (should be empty?) */
	if ( (Pointer *)datum == NULL ) PG_RETURN_NULL();

	array = (ArrayType *) PG_DETOAST_DATUM(datum);

	nelems = ArrayGetNItems(ARR_NDIM(array), ARR_DIMS(array));

#ifdef PGIS_DEBUG
	elog(NOTICE, "unite_garray: number of elements: %d", nelems);
#endif

	if ( nelems == 0 ) PG_RETURN_NULL();

	/* One-element union is the element itself */
	if ( nelems == 1 ) PG_RETURN_POINTER((PG_LWGEOM *)(ARR_DATA_PTR(array)));

	/* Ok, we really need geos now ;) */
	initGEOS(MAXIMUM_ALIGNOF);

	offset = 0;
	for (i=0; i<nelems; i++)
	{
		PG_LWGEOM *geom = (PG_LWGEOM *)(ARR_DATA_PTR(array)+offset);
		offset += INTALIGN(geom->size);

		pgis_geom = geom;

#ifdef PGIS_DEBUG
		elog(NOTICE, "geom %d @ %p", i, geom);
#endif

		// Check is3d flag
		if ( TYPE_NDIMS(geom->type) > 2 ) is3d = 1;

		// Check SRID homogeneity and initialize geos result
		if ( ! i )
		{
			geos_result = POSTGIS2GEOS(geom);
			SRID = pglwgeom_getSRID(geom);
#ifdef PGIS_DEBUG
		elog(NOTICE, "first geom is a %s", lwgeom_typename(TYPE_GETTYPE(geom->type)));
#endif
			continue;
		}
		else
		{
			if ( SRID != pglwgeom_getSRID(geom) )
			{
				elog(ERROR,
					"Operation on mixed SRID geometries");
				PG_RETURN_NULL();
			}
		}
		
		g1 = POSTGIS2GEOS(pgis_geom);

#ifdef PGIS_DEBUG
		elog(NOTICE, "unite_garray(%d): adding geom %d to union (%s)",
				call, i, lwgeom_typename(TYPE_GETTYPE(geom->type)));
#endif

		g2 = GEOSUnion(g1,geos_result);
		if ( g2 == NULL )
		{
			GEOSdeleteGeometry(g1);
			GEOSdeleteGeometry(geos_result);
			elog(ERROR,"GEOS union() threw an error!");
		}
		GEOSdeleteGeometry(g1);
		GEOSdeleteGeometry(geos_result);
		geos_result = g2;
	}

	GEOSSetSRID(geos_result, SRID);
	result = GEOS2POSTGIS(geos_result, is3d);
	GEOSdeleteGeometry(geos_result);
	if ( result == NULL )
	{
		elog(ERROR, "GEOS2POSTGIS returned an error");
		PG_RETURN_NULL(); //never get here
	}

	//compressType(result);

	PG_RETURN_POINTER(result);

}

#else // def UNITE_USING_BUFFER

/*
 * This is the final function for GeomUnion
 * aggregate. Will have as input an array of Geometries.
 * Builds a GEOMETRYCOLLECTION from input and call
 * GEOSBuffer(collection, 0) on the GEOS-converted
 * versions of it. Returns PGIS-converted version back.
 */
PG_FUNCTION_INFO_V1(unite_garray);
Datum unite_garray(PG_FUNCTION_ARGS)
{
	Datum datum;
	ArrayType *array;
	int is3d = 0;
	int nelems, i, ngeoms, npoints;
	PG_LWGEOM *result=NULL;
	Geometry **geoms, *collection;
	Geometry *g1, *geos_result=NULL;
	int SRID=-1;
	size_t offset;
#ifdef PGIS_DEBUG
	static int call=1;
#endif

#ifdef PGIS_DEBUG
	call++;
#endif

	//lwnotice("GEOS buffer union");

	datum = PG_GETARG_DATUM(0);

	/* Null array, null geometry (should be empty?) */
	if ( (Pointer *)datum == NULL ) PG_RETURN_NULL();

	array = (ArrayType *) PG_DETOAST_DATUM(datum);

	nelems = ArrayGetNItems(ARR_NDIM(array), ARR_DIMS(array));

#ifdef PGIS_DEBUG
	elog(NOTICE, "unite_garray: number of elements: %d", nelems);
#endif

	if ( nelems == 0 ) PG_RETURN_NULL();

	/* One-element union is the element itself */
	if ( nelems == 1 ) PG_RETURN_POINTER((PG_LWGEOM *)(ARR_DATA_PTR(array)));

	geoms = lwalloc(sizeof(Geometry *)*nelems);

	/* We need geos here */
	initGEOS(MAXIMUM_ALIGNOF);

	offset = 0; i=0;
	ngeoms = 0; npoints=0;
//lwnotice("Nelems %d, MAXGEOMSPOINST %d", nelems, MAXGEOMSPOINTS);
	while (!result) 
	{
		PG_LWGEOM *geom = (PG_LWGEOM *)(ARR_DATA_PTR(array)+offset);
		offset += INTALIGN(geom->size);

		// Check is3d flag
		if ( TYPE_NDIMS(geom->type) > 2 ) is3d = 1;

		// Check SRID homogeneity 
		if ( ! i ) SRID = pglwgeom_getSRID(geom);
		else if ( SRID != pglwgeom_getSRID(geom) )
			lwerror("Operation on mixed SRID geometries");

		geoms[ngeoms] =
			g1 = POSTGIS2GEOS(geom);
			//lwgeom_deserialize(SERIALIZED_FORM(geom));

		npoints += GEOSGetNumCoordinate(geoms[ngeoms]);

		++ngeoms;
		++i;

//lwnotice("Loop %d, npoints: %d", i, npoints);

		/*
		 * Maximum count of geometry points reached
		 * or end of them, collect and buffer(0).
		 */
		if ( (npoints>=MAXGEOMSPOINTS && ngeoms>1) || i==nelems)
		{
//lwnotice(" CHUNK (ngeoms:%d, npoints:%d, left:%d)", ngeoms, npoints, nelems-i);

			//collection = (LWGEOM *)
			//	lwcollection_construct(COLLECTIONTYPE,
			//	-1, NULL, ngeoms, lwgeoms);
			collection = PostGIS2GEOS_collection(COLLECTIONTYPE,
				geoms, ngeoms, -1, is3d);

			//g1 = LWGEOM2GEOS(collection);

			//lwgeom_release(collection);

			//geos_result = GEOSBuffer(g1, 0, 0);
			geos_result = GEOSBuffer(collection, 0, 0);
			if ( geos_result == NULL )
			{
				GEOSdeleteGeometry(g1);
				lwerror("GEOS buffer() threw an error!");
			}
			//GEOSdeleteGeometry(g1);
			GEOSdeleteGeometry(collection);

//lwnotice(" Buffer() executed");

			/*
			 * If there are no other geoms in input
			 * we've finished, otherwise we push
			 * the result back on the input stack.
			 */
			if ( i == nelems )
			{
//lwnotice("  Final result points: %d", GEOSGetNumCoordinate(geos_result));
				GEOSSetSRID(geos_result, SRID);
				result = GEOS2POSTGIS(geos_result, is3d);
				GEOSdeleteGeometry(geos_result);
//lwnotice(" Result computed");
			}
			else
			{
				//lwgeoms = lwalloc(sizeof(LWGEOM *)*MAXGEOMS);
				//lwgeoms[0] = lwgeom_from_geometry(geos_result, is3d);
				geoms[0] = geos_result;
				ngeoms=1;
				npoints = GEOSGetNumCoordinate(geoms[0]);
//lwnotice("  Result pushed back on lwgeoms array (npoints:%d)", npoints);
			}
		}
	}


	//compressType(result);

	PG_RETURN_POINTER(result);

}

#endif // def UNITE_USING_BUFFER


//select geomunion('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))','POLYGON((5 5, 15 5, 15 7, 5 7, 5 5))');
PG_FUNCTION_INFO_V1(geomunion);
Datum geomunion(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	int is3d;
	int SRID;
	Geometry *g1,*g2,*g3;
	PG_LWGEOM *result;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	is3d = ( TYPE_NDIMS(geom1->type) > 2 ) ||
		( TYPE_NDIMS(geom2->type) > 2 );

	SRID = pglwgeom_getSRID(geom1);
	if ( SRID != pglwgeom_getSRID(geom2) )
	{
		elog(ERROR, "Operation on two geometries with different SRIDs");
		PG_RETURN_NULL();
	}

	initGEOS(MAXIMUM_ALIGNOF);
//elog(NOTICE,"in geomunion");

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2GEOS(geom1);
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif
#ifdef PROFILE
	profstart(PROF_P2G2);
#endif
	g2 = POSTGIS2GEOS(geom2);
#ifdef PROFILE
	profstop(PROF_P2G2);
#endif

//elog(NOTICE,"g1=%s",GEOSasText(g1));
//elog(NOTICE,"g2=%s",GEOSasText(g2));
#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	g3 = GEOSUnion(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

//elog(NOTICE,"g3=%s",GEOSasText(g3));

	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g2);

	if (g3 == NULL)
	{
		elog(ERROR,"GEOS union() threw an error!");
		PG_RETURN_NULL(); //never get here
	}


	GEOSSetSRID(g3, SRID);

//elog(NOTICE,"result: %s", GEOSasText(g3) ) ;

#ifdef PROFILE
	profstart(PROF_G2P);
#endif
	result = GEOS2POSTGIS(g3, is3d);
#ifdef PROFILE
	profstop(PROF_G2P);
#endif

	GEOSdeleteGeometry(g3);

	if (result == NULL)
	{
		elog(ERROR,"GEOS union() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); //never get here
	}

	//compressType(result);  // convert multi* to single item if appropriate

#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("geos",geom1, geom2, result);
#endif

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_POINTER(result);
}


// select symdifference('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))','POLYGON((5 5, 15 5, 15 7, 5 7, 5 5))');
PG_FUNCTION_INFO_V1(symdifference);
Datum symdifference(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	Geometry *g1,*g2,*g3;
	PG_LWGEOM *result;
	int is3d;
	int SRID;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	is3d = ( TYPE_NDIMS(geom1->type) > 2 ) ||
		( TYPE_NDIMS(geom2->type) > 2 );

	SRID = pglwgeom_getSRID(geom1);
	if ( SRID != pglwgeom_getSRID(geom2) )
	{
		elog(ERROR, "Operation on two geometries with different SRIDs");
		PG_RETURN_NULL();
	}

	initGEOS(MAXIMUM_ALIGNOF);

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2GEOS(geom1);
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif
#ifdef PROFILE
	profstart(PROF_P2G2);
#endif
	g2 = POSTGIS2GEOS(geom2);
#ifdef PROFILE
	profstop(PROF_P2G2);
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	g3 = GEOSSymDifference(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	if (g3 == NULL)
	{
		elog(ERROR,"GEOS symdifference() threw an error!");
		GEOSdeleteGeometry(g1);
		GEOSdeleteGeometry(g2);
		PG_RETURN_NULL(); //never get here
	}

//	elog(NOTICE,"result: %s", GEOSasText(g3) ) ;

	GEOSSetSRID(g3, SRID);

#ifdef PROFILE
	profstart(PROF_G2P);
#endif
	result = GEOS2POSTGIS(g3, is3d);
#ifdef PROFILE
	profstop(PROF_G2P);
#endif

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

	//compressType(result);  // convert multi* to single item if appropriate

#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("geos",geom1, geom2, result);
#endif

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_POINTER(result);
}


PG_FUNCTION_INFO_V1(boundary);
Datum boundary(PG_FUNCTION_ARGS)
{
	PG_LWGEOM	*geom1;
	Geometry *g1,*g3;
	PG_LWGEOM *result;
	int SRID;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	SRID = pglwgeom_getSRID(geom1);

	initGEOS(MAXIMUM_ALIGNOF);

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2GEOS(geom1 );
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	g3 = GEOSBoundary(g1);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	if (g3 == NULL)
	{
		elog(ERROR,"GEOS bounary() threw an error!");
		GEOSdeleteGeometry(g1);
		PG_RETURN_NULL(); //never get here
	}

//	elog(NOTICE,"result: %s", GEOSasText(g3) ) ;

	GEOSSetSRID(g3, SRID);

#ifdef PROFILE
	profstart(PROF_G2P);
#endif
	result = GEOS2POSTGIS(g3, TYPE_NDIMS(geom1->type) > 2);
#ifdef PROFILE
	profstart(PROF_P2G1);
#endif

	if (result == NULL)
	{
		GEOSdeleteGeometry(g1);

		GEOSdeleteGeometry(g3);
		elog(ERROR,"GEOS boundary() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); //never get here
	}



	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g3);

	//compressType(result);  // convert multi* to single item if appropriate

#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("geos",geom1, NULL, result);
#endif

	PG_FREE_IF_COPY(geom1, 0);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(convexhull);
Datum convexhull(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	Geometry *g1, *g3;
	PG_LWGEOM *result;
	LWGEOM *lwout;
	int SRID;
	BOX2DFLOAT4 bbox;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	SRID = pglwgeom_getSRID(geom1);

	initGEOS(MAXIMUM_ALIGNOF);

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2GEOS(geom1);
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	g3 = GEOSConvexHull(g1);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	if (g3 == NULL)
	{
		elog(ERROR,"GEOS convexhull() threw an error!");
		GEOSdeleteGeometry(g1);
		PG_RETURN_NULL(); //never get here
	}


	//elog(NOTICE,"result: %s", GEOSasText(g3) ) ;
	GEOSSetSRID(g3, SRID);

#ifdef PROFILE
	profstart(PROF_G2P);
#endif
	lwout = lwgeom_from_geometry(g3, TYPE_NDIMS(geom1->type) > 2);
#ifdef PROFILE
	profstop(PROF_G2P);
#endif
	if (lwout == NULL)
	{
		GEOSdeleteGeometry(g1);
		GEOSdeleteGeometry(g3);
		elog(ERROR,"convexhull() failed to convert GEOS geometry to LWGEOM");
		PG_RETURN_NULL(); //never get here
	}

	/* Copy input bbox if any */
	if ( getbox2d_p(SERIALIZED_FORM(geom1), &bbox) )
	{
		lwout->bbox = &bbox;
		/* Mark lwgeom bbox to be externally owned */
		TYPE_SETHASBBOX(lwout->type, 1);
	}

	result = pglwgeom_serialize(lwout);
	if (result == NULL)
	{
		GEOSdeleteGeometry(g1);
		GEOSdeleteGeometry(g3);
		elog(ERROR,"GEOS convexhull() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); //never get here
	}
	lwgeom_release(lwout);
	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g3);


	//compressType(result);  // convert multi* to single item if appropriate

#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("geos",geom1, NULL, result);
#endif

	PG_FREE_IF_COPY(geom1, 0);

	PG_RETURN_POINTER(result);

}

PG_FUNCTION_INFO_V1(buffer);
Datum buffer(PG_FUNCTION_ARGS)
{
	PG_LWGEOM	*geom1;
	double	size;
	Geometry *g1,*g3;
	PG_LWGEOM *result;
	int quadsegs = 8; // the default

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	size = PG_GETARG_FLOAT8(1);
	if ( PG_NARGS() > 2 ) quadsegs = PG_GETARG_INT32(2);

	initGEOS(MAXIMUM_ALIGNOF);

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2GEOS(geom1);
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	g3 = GEOSBuffer(g1,size,quadsegs);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	if (g3 == NULL)
	{
		elog(ERROR,"GEOS buffer() threw an error!");
		GEOSdeleteGeometry(g1);
		PG_RETURN_NULL(); //never get here
	}


//	elog(NOTICE,"result: %s", GEOSasText(g3) ) ;

	GEOSSetSRID(g3, pglwgeom_getSRID(geom1));

#ifdef PROFILE
	profstart(PROF_G2P);
#endif
	result = GEOS2POSTGIS(g3, TYPE_NDIMS(geom1->type) > 2);
#ifdef PROFILE
	profstop(PROF_G2P);
#endif
	if (result == NULL)
	{
		GEOSdeleteGeometry(g1);
		GEOSdeleteGeometry(g3);
		elog(ERROR,"GEOS buffer() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); //never get here
	}
	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g3);


	//compressType(result);  // convert multi* to single item if appropriate

#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("geos",geom1, NULL, result);
#endif

	PG_FREE_IF_COPY(geom1, 0);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(intersection);
Datum intersection(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	Geometry *g1,*g2,*g3;
	PG_LWGEOM *result;
	int is3d;
	int SRID;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	is3d = ( TYPE_NDIMS(geom1->type) > 2 ) ||
		( TYPE_NDIMS(geom2->type) > 2 );

	SRID = pglwgeom_getSRID(geom1);
	if ( SRID != pglwgeom_getSRID(geom2) )
	{
		elog(ERROR, "Operation on two geometries with different SRIDs");
		PG_RETURN_NULL();
	}

	initGEOS(MAXIMUM_ALIGNOF);

//elog(NOTICE,"intersection() START");

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2GEOS(geom1);
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif

#ifdef PROFILE
	profstart(PROF_P2G2);
#endif
	g2 = POSTGIS2GEOS(geom2);
#ifdef PROFILE
	profstop(PROF_P2G2);
#endif

//elog(NOTICE,"               constructed geometrys - calling geos");

//elog(NOTICE,"g1 = %s",GEOSasText(g1));
//elog(NOTICE,"g2 = %s",GEOSasText(g2));


//if (g1==NULL)
//	elog(NOTICE,"g1 is null");
//if (g2==NULL)
//	elog(NOTICE,"g2 is null");
//elog(NOTICE,"g2 is valid = %i",GEOSisvalid(g2));
//elog(NOTICE,"g1 is valid = %i",GEOSisvalid(g1));


#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	g3 = GEOSIntersection(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

//elog(NOTICE,"               intersection finished");

	if (g3 == NULL)
	{
		elog(ERROR,"GEOS Intersection() threw an error!");
		GEOSdeleteGeometry(g1);
		GEOSdeleteGeometry(g2);
		PG_RETURN_NULL(); //never get here
	}


	//elog(NOTICE,"result: %s", GEOSasText(g3) ) ;
	GEOSSetSRID(g3, SRID);

#ifdef PROFILE
	profstart(PROF_G2P);
#endif
	result = GEOS2POSTGIS(g3, is3d);
#ifdef PROFILE
	profstop(PROF_G2P);
#endif

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

	//compressType(result);  // convert multi* to single item if appropriate

#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("geos",geom1, geom2, result);
#endif

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_POINTER(result);
}

//select difference('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))','POLYGON((5 5, 15 5, 15 7, 5 7, 5 5))');
PG_FUNCTION_INFO_V1(difference);
Datum difference(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	Geometry *g1,*g2,*g3;
	PG_LWGEOM *result;
	int is3d;
	int SRID;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	is3d = ( TYPE_NDIMS(geom1->type) > 2 ) ||
		( TYPE_NDIMS(geom2->type) > 2 );

	SRID = pglwgeom_getSRID(geom1);
	if ( SRID != pglwgeom_getSRID(geom2) )
	{
		elog(ERROR, "Operation on two geometries with different SRIDs");
		PG_RETURN_NULL();
	}

	initGEOS(MAXIMUM_ALIGNOF);

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2GEOS(geom1);
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif
#ifdef PROFILE
	profstart(PROF_P2G2);
#endif
	g2 = POSTGIS2GEOS(geom2);
#ifdef PROFILE
	profstop(PROF_P2G2);
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	g3 = GEOSDifference(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	if (g3 == NULL)
	{
		elog(ERROR,"GEOS difference() threw an error!");
		GEOSdeleteGeometry(g1);
		GEOSdeleteGeometry(g2);
		PG_RETURN_NULL(); //never get here
	}

//	elog(NOTICE,"result: %s", GEOSasText(g3) ) ;

	GEOSSetSRID(g3, SRID);

#ifdef PROFILE
	profstart(PROF_G2P);
#endif
	result = GEOS2POSTGIS(g3, is3d);
#ifdef PROFILE
	profstop(PROF_G2P);
#endif
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

	////compressType(result);  // convert multi* to single item if appropriate

#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("geos",geom1, geom2, result);
#endif

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_POINTER(result);
}


//select pointonsurface('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))');
PG_FUNCTION_INFO_V1(pointonsurface);
Datum pointonsurface(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	Geometry *g1,*g3;
	PG_LWGEOM *result;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	initGEOS(MAXIMUM_ALIGNOF);

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2GEOS(geom1);
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	g3 =    GEOSpointonSurface(g1);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	if (g3 == NULL)
	{
		elog(ERROR,"GEOS pointonsurface() threw an error!");
		GEOSdeleteGeometry(g1);
		PG_RETURN_NULL(); //never get here
	}

//	elog(NOTICE,"result: %s", GEOSasText(g3) ) ;

	GEOSSetSRID(g3, pglwgeom_getSRID(geom1));
#ifdef PROFILE
	profstart(PROF_G2P);
#endif
	result = GEOS2POSTGIS(g3, (TYPE_NDIMS(geom1->type) > 2));
#ifdef PROFILE
	profstop(PROF_G2P);
#endif
	if (result == NULL)
	{
		GEOSdeleteGeometry(g1);
		GEOSdeleteGeometry(g3);
		elog(ERROR,"GEOS pointonsurface() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); //never get here
	}

	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g3);

	// convert multi* to single item if appropriate
	//compressType(result); 

#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("geos",geom1, NULL, result);
#endif

	PG_FREE_IF_COPY(geom1, 0);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(centroid);
Datum centroid(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom, *result;
	Geometry *geosgeom, *geosresult;
	int failure;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	initGEOS(MAXIMUM_ALIGNOF);

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	geosgeom = POSTGIS2GEOS(geom);
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	geosresult = GEOSGetCentroid(geosgeom, &failure);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	if ( geosresult == NULL )
	{
		if ( failure ) {
			GEOSdeleteGeometry(geosgeom);
			elog(ERROR,"GEOS getCentroid() threw an error!");
		}
		GEOSdeleteGeometry(geosgeom);
		PG_RETURN_NULL(); 
	}

	GEOSSetSRID(geosresult, pglwgeom_getSRID(geom));

#ifdef PROFILE
	profstart(PROF_G2P);
#endif
	result = GEOS2POSTGIS(geosresult, (TYPE_NDIMS(geom->type) > 2));
#ifdef PROFILE
	profstop(PROF_G2P);
#endif
	if (result == NULL)
	{
		GEOSdeleteGeometry(geosgeom);
		GEOSdeleteGeometry(geosresult);
		elog(ERROR,"Error in GEOS-PGIS conversion");
		PG_RETURN_NULL(); 
	}
	GEOSdeleteGeometry(geosgeom);
	GEOSdeleteGeometry(geosresult);

#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("geos",geom, NULL, result);
#endif

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(result);
}



//----------------------------------------------



void errorIfGeometryCollection(PG_LWGEOM *g1, PG_LWGEOM *g2)
{
	int t1 = lwgeom_getType(g1->type);
	int t2 = lwgeom_getType(g2->type);

	if (  (t1 == COLLECTIONTYPE) || (t2 == COLLECTIONTYPE) )
		elog(ERROR,"Relate Operation called with a LWGEOMCOLLECTION type.  This is unsupported");
}

PG_FUNCTION_INFO_V1(isvalid);
Datum isvalid(PG_FUNCTION_ARGS)
{
	PG_LWGEOM	*geom1;
	LWGEOM *lwgeom;
	bool result;
	Geometry *g1;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	initGEOS(MAXIMUM_ALIGNOF);

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	lwgeom = lwgeom_deserialize(SERIALIZED_FORM(geom1));
	if ( ! lwgeom )
	{
		lwerror("unable to deserialize input");
	}
	g1 = LWGEOM2GEOS(lwgeom);
	if ( ! g1 )
	{
		lwgeom_release(lwgeom);
		PG_RETURN_BOOL(FALSE);
	}
	lwgeom_release(lwgeom);
	//g1 = POSTGIS2GEOS(geom1);
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = GEOSisvalid(g1);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	GEOSdeleteGeometry(g1);
	if (result == 2)
	{
		elog(ERROR,"GEOS isvalid() threw an error!");
		PG_RETURN_NULL(); //never get here
	}


#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("geos",geom1, NULL, NULL);
#endif

	PG_FREE_IF_COPY(geom1, 0);

	PG_RETURN_BOOL(result);
}


// overlaps(PG_LWGEOM g1,PG_LWGEOM g2)
// returns  if GEOS::g1->overlaps(g2) returns true
// throws an error (elog(ERROR,...)) if GEOS throws an error
PG_FUNCTION_INFO_V1(overlaps);
Datum overlaps(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	Geometry *g1,*g2;
	bool result;
	BOX2DFLOAT4 box1, box2;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);

	/*
	 * short-circuit 1: if geom2 bounding box does not overlap
	 * geom1 bounding box we can prematurely return FALSE.
	 * Do the test IFF BOUNDING BOX AVAILABLE.
	 */
	if ( getbox2d_p(SERIALIZED_FORM(geom1), &box1) &&
		getbox2d_p(SERIALIZED_FORM(geom2), &box2) )
	{
		if ( box2.xmax < box1.xmin ) PG_RETURN_BOOL(FALSE);
		if ( box2.xmin > box1.xmax ) PG_RETURN_BOOL(FALSE);
		if ( box2.ymax < box1.ymin ) PG_RETURN_BOOL(FALSE);
		if ( box2.ymin > box2.ymax ) PG_RETURN_BOOL(FALSE);
	}

	initGEOS(MAXIMUM_ALIGNOF);

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2GEOS(geom1);
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif
#ifdef PROFILE
	profstart(PROF_P2G2);
#endif
	g2 = POSTGIS2GEOS(geom2);
#ifdef PROFILE
	profstop(PROF_P2G2);
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = GEOSrelateOverlaps(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g2);
	if (result == 2)
	{
		elog(ERROR,"GEOS overlaps() threw an error!");
		PG_RETURN_NULL(); //never get here
	}

#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("geos",geom1, geom2, NULL);
#endif

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}



PG_FUNCTION_INFO_V1(contains);
Datum contains(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	Geometry *g1,*g2;
	bool result;
	BOX2DFLOAT4 box1, box2;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);

	/*
	 * short-circuit 1: if geom2 bounding box is not completely inside
	 * geom1 bounding box we can prematurely return FALSE.
	 * Do the test IFF BOUNDING BOX AVAILABLE.
	 */
	if ( getbox2d_p(SERIALIZED_FORM(geom1), &box1) &&
		getbox2d_p(SERIALIZED_FORM(geom2), &box2) )
	{
		if ( box2.xmin < box1.xmin ) PG_RETURN_BOOL(FALSE);
		if ( box2.xmax > box1.xmax ) PG_RETURN_BOOL(FALSE);
		if ( box2.ymin < box1.ymin ) PG_RETURN_BOOL(FALSE);
		if ( box2.ymax > box1.ymax ) PG_RETURN_BOOL(FALSE);
	}

	initGEOS(MAXIMUM_ALIGNOF);

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2GEOS(geom1);
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif
#ifdef PROFILE
	profstart(PROF_P2G2);
#endif
	g2 = POSTGIS2GEOS(geom2);
#ifdef PROFILE
	profstop(PROF_P2G2);
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = GEOSrelateContains(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g2);

	if (result == 2)
	{
		elog(ERROR,"GEOS contains() threw an error!");
		PG_RETURN_NULL(); //never get here
	}

#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("geos",geom1, geom2, NULL);
#endif

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(within);
Datum within(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	Geometry *g1,*g2;
	bool result;
	BOX2DFLOAT4 box1, box2;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);

	/*
	 * short-circuit 1: if geom1 bounding box is not completely inside
	 * geom2 bounding box we can prematurely return FALSE.
	 * Do the test IFF BOUNDING BOX AVAILABLE.
	 */
	if ( getbox2d_p(SERIALIZED_FORM(geom1), &box1) &&
		getbox2d_p(SERIALIZED_FORM(geom2), &box2) )
	{
		if ( box1.xmin < box2.xmin ) PG_RETURN_BOOL(FALSE);
		if ( box1.xmax > box2.xmax ) PG_RETURN_BOOL(FALSE);
		if ( box1.ymin < box2.ymin ) PG_RETURN_BOOL(FALSE);
		if ( box1.ymax > box2.ymax ) PG_RETURN_BOOL(FALSE);
	}

	initGEOS(MAXIMUM_ALIGNOF);

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2GEOS(geom1);
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif
#ifdef PROFILE
	profstart(PROF_P2G2);
#endif
	g2 = POSTGIS2GEOS(geom2);
#ifdef PROFILE
	profstop(PROF_P2G2);
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = GEOSrelateWithin(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g2);

	if (result == 2)
	{
		elog(ERROR,"GEOS within() threw an error!");
		PG_RETURN_NULL(); //never get here
	}

#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("geos",geom1, geom2, NULL);
#endif

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}



PG_FUNCTION_INFO_V1(crosses);
Datum crosses(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	Geometry *g1,*g2;
	bool result;
	BOX2DFLOAT4 box1, box2;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);

	/*
	 * short-circuit 1: if geom2 bounding box does not overlap
	 * geom1 bounding box we can prematurely return FALSE.
	 * Do the test IFF BOUNDING BOX AVAILABLE.
	 */
	if ( getbox2d_p(SERIALIZED_FORM(geom1), &box1) &&
		getbox2d_p(SERIALIZED_FORM(geom2), &box2) )
	{
		if ( box2.xmax < box1.xmin ) PG_RETURN_BOOL(FALSE);
		if ( box2.xmin > box1.xmax ) PG_RETURN_BOOL(FALSE);
		if ( box2.ymax < box1.ymin ) PG_RETURN_BOOL(FALSE);
		if ( box2.ymin > box2.ymax ) PG_RETURN_BOOL(FALSE);
	}

	initGEOS(MAXIMUM_ALIGNOF);

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2GEOS(geom1);
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif
#ifdef PROFILE
	profstart(PROF_P2G2);
#endif
	g2 = POSTGIS2GEOS(geom2);
#ifdef PROFILE
	profstop(PROF_P2G2);
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = GEOSrelateCrosses(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g2);

	if (result == 2)
	{
		elog(ERROR,"GEOS crosses() threw an error!");
		PG_RETURN_NULL(); //never get here
	}

#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("geos",geom1, geom2, NULL);
#endif

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}



PG_FUNCTION_INFO_V1(intersects);
Datum intersects(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	Geometry *g1,*g2;
	bool result;
	BOX2DFLOAT4 box1, box2;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);

	/*
	 * short-circuit 1: if geom2 bounding box does not overlap
	 * geom1 bounding box we can prematurely return FALSE.
	 * Do the test IFF BOUNDING BOX AVAILABLE.
	 */
	if ( getbox2d_p(SERIALIZED_FORM(geom1), &box1) &&
		getbox2d_p(SERIALIZED_FORM(geom2), &box2) )
	{
		if ( box2.xmax < box1.xmin ) PG_RETURN_BOOL(FALSE);
		if ( box2.xmin > box1.xmax ) PG_RETURN_BOOL(FALSE);
		if ( box2.ymax < box1.ymin ) PG_RETURN_BOOL(FALSE);
		if ( box2.ymin > box2.ymax ) PG_RETURN_BOOL(FALSE);
	}

	initGEOS(MAXIMUM_ALIGNOF);

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2GEOS(geom1 );
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif
#ifdef PROFILE
	profstart(PROF_P2G2);
#endif
	g2 = POSTGIS2GEOS(geom2 );
#ifdef PROFILE
	profstop(PROF_P2G2);
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = GEOSrelateIntersects(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif
	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g2);
	if (result == 2)
	{
		elog(ERROR,"GEOS intersects() threw an error!");
		PG_RETURN_NULL(); //never get here
	}

#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("intr",geom1, geom2, NULL);
#endif

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(touches);
Datum touches(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	Geometry *g1,*g2;
	bool result;
	BOX2DFLOAT4 box1, box2;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);

	/*
	 * short-circuit 1: if geom2 bounding box does not overlap
	 * geom1 bounding box we can prematurely return FALSE.
	 * Do the test IFF BOUNDING BOX AVAILABLE.
	 */
	if ( getbox2d_p(SERIALIZED_FORM(geom1), &box1) &&
		getbox2d_p(SERIALIZED_FORM(geom2), &box2) )
	{
		if ( box2.xmax < box1.xmin ) PG_RETURN_BOOL(FALSE);
		if ( box2.xmin > box1.xmax ) PG_RETURN_BOOL(FALSE);
		if ( box2.ymax < box1.ymin ) PG_RETURN_BOOL(FALSE);
		if ( box2.ymin > box2.ymax ) PG_RETURN_BOOL(FALSE);
	}

	initGEOS(MAXIMUM_ALIGNOF);

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2GEOS(geom1 );
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif
#ifdef PROFILE
	profstart(PROF_P2G2);
#endif
	g2 = POSTGIS2GEOS(geom2 );
#ifdef PROFILE
	profstop(PROF_P2G2);
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = GEOSrelateTouches(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g2);

	if (result == 2)
	{
		elog(ERROR,"GEOS touches() threw an error!");
		PG_RETURN_NULL(); //never get here
	}

#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("geos",geom1, geom2, NULL);
#endif

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(disjoint);
Datum disjoint(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	Geometry *g1,*g2;
	bool result;
	BOX2DFLOAT4 box1, box2;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);

	/*
	 * short-circuit 1: if geom2 bounding box does not overlap
	 * geom1 bounding box we can prematurely return TRUE.
	 * Do the test IFF BOUNDING BOX AVAILABLE.
	 */
	if ( getbox2d_p(SERIALIZED_FORM(geom1), &box1) &&
		getbox2d_p(SERIALIZED_FORM(geom2), &box2) )
	{
		if ( box2.xmax < box1.xmin ) PG_RETURN_BOOL(TRUE);
		if ( box2.xmin > box1.xmax ) PG_RETURN_BOOL(TRUE);
		if ( box2.ymax < box1.ymin ) PG_RETURN_BOOL(TRUE);
		if ( box2.ymin > box2.ymax ) PG_RETURN_BOOL(TRUE);
	}

	initGEOS(MAXIMUM_ALIGNOF);

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2GEOS(geom1);
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif
#ifdef PROFILE
	profstart(PROF_P2G2);
#endif
	g2 = POSTGIS2GEOS(geom2);
#ifdef PROFILE
	profstop(PROF_P2G2);
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = GEOSrelateDisjoint(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif
	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g2);

	if (result == 2)
	{
		elog(ERROR,"GEOS disjoin() threw an error!");
		PG_RETURN_NULL(); //never get here
	}

#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("geos",geom1, geom2, NULL);
#endif

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(relate_pattern);
Datum relate_pattern(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	char *patt;
	bool result;
	Geometry *g1,*g2;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);

	initGEOS(MAXIMUM_ALIGNOF);

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2GEOS(geom1);
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif
#ifdef PROFILE
	profstart(PROF_P2G2);
#endif
	g2 = POSTGIS2GEOS(geom2);
#ifdef PROFILE
	profstop(PROF_P2G2);
#endif

	patt =  DatumGetCString(DirectFunctionCall1(textout,
                        PointerGetDatum(PG_GETARG_DATUM(2))));

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = GEOSrelatePattern(g1,g2,patt);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif
	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g2);
	pfree(patt);

	if (result == 2)
	{
		elog(ERROR,"GEOS relate_pattern() threw an error!");
		PG_RETURN_NULL(); //never get here
	}

#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("geos",geom1, geom2, NULL);
#endif

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}



PG_FUNCTION_INFO_V1(relate_full);
Datum relate_full(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	Geometry *g1,*g2;
	char *relate_str;
	int len;
	text *result;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

//elog(NOTICE,"in relate_full()");

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);


	initGEOS(MAXIMUM_ALIGNOF);

//elog(NOTICE,"GEOS init()");

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2GEOS(geom1 );
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif
#ifdef PROFILE
	profstart(PROF_P2G2);
#endif
	g2 = POSTGIS2GEOS(geom2 );
#ifdef PROFILE
	profstop(PROF_P2G2);
#endif

//elog(NOTICE,"constructed geometries ");

	if ((g1==NULL) || (g2 == NULL))
		elog(NOTICE,"g1 or g2 are null");

//elog(NOTICE,GEOSasText(g1));
//elog(NOTICE,GEOSasText(g2));

//elog(NOTICE,"valid g1 = %i", GEOSisvalid(g1));
//elog(NOTICE,"valid g2 = %i",GEOSisvalid(g2));

//elog(NOTICE,"about to relate()");


#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	relate_str = GEOSrelate(g1, g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

//elog(NOTICE,"finished relate()");

	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g2);

	if (relate_str == NULL)
	{
		//free(relate_str);
		elog(ERROR,"GEOS relate() threw an error!");
		PG_RETURN_NULL(); //never get here
	}

	len = strlen(relate_str) + VARHDRSZ;

	result= palloc(len);
	VARATT_SIZEP(result) = len;
	//*((int *) result) = len;

	memcpy(VARDATA(result), relate_str, len-VARHDRSZ);

	free(relate_str);

#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("geos",geom1, geom2, NULL);
#endif

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_POINTER(result);
}

//==============================

PG_FUNCTION_INFO_V1(geomequals);
Datum geomequals(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	Geometry *g1,*g2;
	bool result;
	BOX2DFLOAT4 box1, box2;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);

	/*
	 * short-circuit 1: if geom2 bounding box does not equal
	 * geom1 bounding box we can prematurely return FALSE.
	 * Do the test IFF BOUNDING BOX AVAILABLE.
	 */
	if ( getbox2d_p(SERIALIZED_FORM(geom1), &box1) &&
		getbox2d_p(SERIALIZED_FORM(geom2), &box2) )
	{
		if ( box2.xmax != box1.xmax ) PG_RETURN_BOOL(FALSE);
		if ( box2.xmin != box1.xmin ) PG_RETURN_BOOL(FALSE);
		if ( box2.ymax != box1.ymax ) PG_RETURN_BOOL(FALSE);
		if ( box2.ymin != box2.ymin ) PG_RETURN_BOOL(FALSE);
	}

	initGEOS(MAXIMUM_ALIGNOF);

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2GEOS(geom1);
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif
#ifdef PROFILE
	profstart(PROF_P2G2);
#endif
	g2 = POSTGIS2GEOS(geom2);
#ifdef PROFILE
	profstop(PROF_P2G2);
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = GEOSequals(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif
	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g2);

	if (result == 2)
	{
		elog(ERROR,"GEOS equals() threw an error!");
		PG_RETURN_NULL(); //never get here
	}

#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("geos",geom1, geom2, NULL);
#endif

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(issimple);
Datum issimple(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	Geometry *g1;
	int result;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	if (lwgeom_getnumgeometries(SERIALIZED_FORM(geom)) == 0)
		PG_RETURN_BOOL(true);

	initGEOS(MAXIMUM_ALIGNOF);

	//elog(NOTICE,"GEOS init()");

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2GEOS(geom);
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = GEOSisSimple(g1);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif
	GEOSdeleteGeometry(g1);

	if (result == 2)
	{
		elog(ERROR,"GEOS issimple() threw an error!");
		PG_RETURN_NULL(); //never get here
	}

#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("geos",geom, NULL, NULL);
#endif

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(isring);
Datum isring(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	Geometry *g1;
	int result;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	if (lwgeom_getType(geom->type) != LINETYPE)
	{
		elog(ERROR,"isring() should only be called on a LINE");
	}

	if (lwgeom_getnumgeometries(SERIALIZED_FORM(geom)) == 0)
		PG_RETURN_BOOL(false);

	initGEOS(MAXIMUM_ALIGNOF);

	//elog(NOTICE,"GEOS init()");

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2GEOS(geom );
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = GEOSisRing(g1);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif
	GEOSdeleteGeometry(g1);

	if (result == 2)
	{
		elog(ERROR,"GEOS isring() threw an error!");
		PG_RETURN_NULL(); //never get here
	}

#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("geos",geom, NULL, NULL);
#endif

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_BOOL(result);
}



//= GEOS <=> POSTGIS CONVERSION =========================

//-----=GEOS2POSTGIS=

// Return a LWPOINT from a GEOS Point.
LWPOINT *
lwpoint_from_geometry(Geometry *g, char want3d)
{
	POINTARRAY *pa;
	LWPOINT *point;
	POINT3D *pts;
	size_t ptsize = want3d ? sizeof(POINT3DZ) : sizeof(POINT2D);
	int SRID = GEOSGetSRID(g);

#ifdef PGIS_DEBUG_GEOS2POSTGIS
	elog(NOTICE, "lwpoint_from_geometry: point size %d", ptsize);
#endif

	// Construct point array
	pa = (POINTARRAY *)lwalloc(sizeof(POINTARRAY));
	TYPE_SETZM(pa->dims, want3d, 0);
	pa->npoints = 1;

	// Fill point array
	pa->serialized_pointlist = lwalloc(ptsize);
	pts = GEOSGetCoordinates(g);
	memcpy(pa->serialized_pointlist, pts, ptsize);
	GEOSdeleteChar( (char*) pts);

	// Construct LWPOINT
	point = lwpoint_construct(SRID, NULL, pa);

	return point;
}

// Return a LWLINE from a GEOS linestring
LWLINE *
lwline_from_geometry(Geometry *g, char want3d)
{
	POINTARRAY *pa;
	LWLINE *line;
	int npoints;
	POINT3D *pts, *ip;
	int ptsize = want3d ? sizeof(POINT3D) : sizeof(POINT2D);
	int i;
	int SRID = GEOSGetSRID(g);

#ifdef PGIS_DEBUG_GEOS2POSTGIS
	elog(NOTICE, "lwline_from_geometry: point size %d", ptsize);
#endif

	npoints = GEOSGetNumCoordinate(g);
	if (npoints <2) return NULL;

	// Construct point array
	pa = (POINTARRAY *)palloc(sizeof(POINTARRAY));
	TYPE_SETZM(pa->dims, want3d, 0);
	pa->npoints = npoints;

	// Fill point array
	pa->serialized_pointlist = palloc(ptsize*npoints);
	pts = GEOSGetCoordinates(g);
	for (i=0; i<npoints; i++)
	{
		ip = &(pts[i]);
		memcpy(getPoint_internal(pa, i), ip, ptsize);
	}
	GEOSdeleteChar( (char*) pts);

	// Construct LWPOINT
	line = lwline_construct(SRID, NULL, pa);

	return line;
}

// Return a LWPOLY from a GEOS polygon
LWPOLY *
lwpoly_from_geometry(Geometry *g, char want3d)
{
	POINTARRAY **rings, *pa;
	LWPOLY *poly;
	int ndims = want3d ? 3 : 2;
	int nrings;
	int npoints;
	int i, j;
	POINT3D *pts, *ip;
	int ptoff=0; // point offset inside POINT3D *
	int ptsize = sizeof(double)*ndims;
	int SRID = GEOSGetSRID(g);

#ifdef PGIS_DEBUG_GEOS2POSTGIS
	elog(NOTICE, "lwpoly_from_geometry: point size %d", ptsize);
#endif

	// Get number of rings, and pointlist
	pts = GEOSGetCoordinates(g);
	nrings = GEOSGetNumInteriorRings(g); 
	rings = (POINTARRAY **)palloc(sizeof(POINTARRAY *)*(nrings+1));

	// Exterior ring
	npoints = GEOSGetNumCoordinate(GEOSGetExteriorRing(g));
	rings[0] = (POINTARRAY *)palloc(sizeof(POINTARRAY));
	pa = rings[0];
	TYPE_SETZM(pa->dims, want3d, 0);
	pa->npoints = npoints;

	// Fill point array
	pa->serialized_pointlist = palloc(ptsize*npoints);
	for (i=0; i<npoints; i++)
	{
		ip = &(pts[i+ptoff]);
		memcpy(getPoint_internal(pa, i), ip, ptsize);
	}
	ptoff+=npoints;

	// Interior rings
	for (j=0; j<nrings; j++)
	{
		npoints = GEOSGetNumCoordinate(GEOSGetInteriorRingN(g, j));
		rings[j+1] = (POINTARRAY *)palloc(sizeof(POINTARRAY));
		pa = rings[j+1];
		TYPE_SETZM(pa->dims, want3d, 0);
		pa->npoints = npoints;

		// Fill point array
		pa->serialized_pointlist = palloc(ptsize*npoints);
		for (i=0; i<npoints; i++)
		{
			ip = &(pts[i+ptoff]);
			memcpy(getPoint_internal(pa, i), ip, ptsize);
		}
		ptoff+=npoints;
	}

	GEOSdeleteChar( (char*) pts);

	// Construct LWPOLY
	poly = lwpoly_construct(SRID, NULL, nrings+1, rings);

	return poly;
}

// Return a lwcollection from a GEOS multi*
LWCOLLECTION *
lwcollection_from_geometry(Geometry *geom, char want3d)
{
	uint32 ngeoms;
	LWGEOM **geoms = NULL;
	LWCOLLECTION *ret;
	int type = GEOSGeometryTypeId(geom) ;
	int SRID = GEOSGetSRID(geom);
	int i;

	ngeoms = GEOSGetNumGeometries(geom);

#ifdef PGIS_DEBUG_GEOS2POSTGIS
	lwnotice("lwcollection_from_geometry: type: %s, geoms %d",
		lwgeom_typename(type), ngeoms);
#endif

	if ( ngeoms ) geoms = lwalloc(sizeof(LWGEOM *)*ngeoms);

	for (i=0; i<ngeoms; i++)
	{
		Geometry *g = GEOSGetGeometryN(geom, i);
#ifdef PGIS_DEBUG_GEOS2POSTGIS
		lwnotice("lwcollection_from_geometry: geom %d is a %s", i, lwgeom_typename(GEOSGeometryTypeId(g)));
#endif
		geoms[i] = lwgeom_from_geometry(g, want3d);
#ifdef PGIS_DEBUG_GEOS2POSTGIS
		lwnotice("lwcollection_from_geometry: geoms[%d] is a %s", i, lwgeom_typename(TYPE_GETTYPE(geoms[i]->type)));
#endif
	}

	ret = lwcollection_construct(type, SRID, NULL, ngeoms, geoms);
	return ret;
}


// Return an LWGEOM from a Geometry
LWGEOM *
lwgeom_from_geometry(Geometry *geom, char want3d)
{
	int type = GEOSGeometryTypeId(geom) ;
	bool hasZ = GEOSHasZ(geom);

	if ( ! hasZ )
	{
		if ( want3d )
		{
			//elog(NOTICE, "Geometry has no Z, won't provide one");
			want3d = 0;
		}
	}

#ifdef PGIS_DEBUG_GEOS2POSTGIS
	lwnotice("lwgeom_from_geometry: it's a %s", lwgeom_typename(type));
#endif
	switch (type)
	{
		/* From slower to faster.. compensation rule :) */

		case COLLECTIONTYPE:
		case MULTIPOLYGONTYPE:
		case MULTILINETYPE:
		case MULTIPOINTTYPE:
			return (LWGEOM *)lwcollection_from_geometry(geom, want3d);

		case POLYGONTYPE:
			return (LWGEOM *)lwpoly_from_geometry(geom, want3d);

		case LINETYPE:
			return (LWGEOM *)lwline_from_geometry(geom, want3d);

		case POINTTYPE: 
			return (LWGEOM *)lwpoint_from_geometry(geom, want3d);

		default:
			lwerror("lwgeom_from_geometry: unknown geometry type: %d", type);
			return NULL;

	}

}

PG_LWGEOM *
GEOS2POSTGIS(Geometry *geom, char want3d)
{
	LWGEOM *lwgeom;
	PG_LWGEOM *result;

	lwgeom = lwgeom_from_geometry(geom, want3d);
	if ( ! lwgeom )
	{
		lwerror("GEOS2POSTGIS: lwgeom_from_geometry returned NULL");
		return NULL;
	}

#ifdef PGIS_DEBUG_GEOS2POSTGIS
	lwnotice("GEOS2POSTGIS: lwgeom_from_geometry returned a %s", lwgeom_summary(lwgeom, 0)); 
#endif

	if ( is_worth_caching_lwgeom_bbox(lwgeom) )
	{
		lwgeom_addBBOX(lwgeom);
	}

	result = pglwgeom_serialize(lwgeom);

	return result;
}

//-----=POSTGIS2GEOS=

Geometry *LWGEOM2GEOS(LWGEOM *lwgeom);

Geometry *
LWGEOM2GEOS(LWGEOM *lwgeom)
{
	uint32 i;
	Geometry **collected;
	LWCOLLECTION *col;
	if ( ! lwgeom ) return NULL;

#ifdef PGIS_DEBUG_POSTGIS2GEOS
	lwnotice("LWGEOM2GEOS: got lwgeom[%p]", lwgeom);
#endif

	switch (TYPE_GETTYPE(lwgeom->type))
	{
		case POINTTYPE:
#ifdef PGIS_DEBUG_POSTGIS2GEOS
			lwnotice("LWGEOM2GEOS: point[%p]", lwgeom);
#endif
			return PostGIS2GEOS_point((LWPOINT *)lwgeom);

		case LINETYPE:
#ifdef PGIS_DEBUG_POSTGIS2GEOS
			lwnotice("LWGEOM2GEOS: line[%p]", lwgeom);
#endif
			return PostGIS2GEOS_linestring((LWLINE *)lwgeom);

		case POLYGONTYPE:
#ifdef PGIS_DEBUG_POSTGIS2GEOS
			lwnotice("LWGEOM2GEOS: poly[%p]", lwgeom);
#endif
			return PostGIS2GEOS_polygon((LWPOLY *)lwgeom);

		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case COLLECTIONTYPE:
			col = (LWCOLLECTION *)lwgeom;
#ifdef PGIS_DEBUG_POSTGIS2GEOS
			lwnotice("LWGEOM2GEOS: %s with %d subgeoms", lwgeom_typename(TYPE_GETTYPE(col->type)), col->ngeoms);
#endif
			collected = (Geometry **)lwalloc(sizeof(Geometry *)*col->ngeoms);
			for (i=0; i<col->ngeoms; i++)
			{
				collected[i] = LWGEOM2GEOS(col->geoms[i]);
			}
			return PostGIS2GEOS_collection(TYPE_GETTYPE(col->type),
				collected, col->ngeoms, col->SRID,
				TYPE_NDIMS(col->type)>2);

		default:
			lwerror("Unknown geometry type: %d",
				TYPE_GETTYPE(lwgeom->type));
			return NULL;
	}

}

Geometry *
POSTGIS2GEOS(PG_LWGEOM *geom)
{
	Geometry *ret;
	LWGEOM *lwgeom = lwgeom_deserialize(SERIALIZED_FORM(geom));
	if ( ! lwgeom )
	{
		lwerror("POSTGIS2GEOS: unable to deserialize input");
		return NULL;
	}
	ret = LWGEOM2GEOS(lwgeom);
	lwgeom_release(lwgeom);
	if ( ! ret )  {
		lwerror("POSTGIS2GEOS conversion failed");
		return NULL;
	}
	return ret;
}

PG_FUNCTION_INFO_V1(GEOSnoop);
Datum GEOSnoop(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	Geometry *geosgeom;
	PG_LWGEOM *result;

	initGEOS(MAXIMUM_ALIGNOF);

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
#ifdef PGIS_DEBUG_CONVERTER
	elog(NOTICE, "GEOSnoop: IN: %s", unparse_WKT(SERIALIZED_FORM(geom), malloc, free));
#endif

	geosgeom = POSTGIS2GEOS(geom);
	if ( ! geosgeom ) PG_RETURN_NULL();

#ifdef PROFILE
	profstart(PROF_GRUN);
	profstop(PROF_GRUN);
#endif

	result = GEOS2POSTGIS(geosgeom, TYPE_NDIMS(geom->type) > 2);
	GEOSdeleteGeometry(geosgeom);

#ifdef PGIS_DEBUG_CONVERTER
	elog(NOTICE, "GEOSnoop: OUT: %s", unparse_WKT(SERIALIZED_FORM(result), malloc, free));
#endif

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(polygonize_garray);
Datum polygonize_garray(PG_FUNCTION_ARGS)
{
	Datum datum;
	ArrayType *array;
	int is3d = 0;
	unsigned int nelems, i;
	PG_LWGEOM *result;
	Geometry *geos_result;
	Geometry **vgeoms;
	int SRID=-1;
	size_t offset;
#ifdef PGIS_DEBUG
	static int call=1;
#endif

#ifdef PGIS_DEBUG
	call++;
#endif

	datum = PG_GETARG_DATUM(0);

	/* Null array, null geometry (should be empty?) */
	if ( (Pointer *)datum == NULL ) PG_RETURN_NULL();

	array = (ArrayType *) PG_DETOAST_DATUM(datum);

	nelems = ArrayGetNItems(ARR_NDIM(array), ARR_DIMS(array));

#ifdef PGIS_DEBUG
	elog(NOTICE, "polygonize_garray: number of elements: %d", nelems);
#endif

	if ( nelems == 0 ) PG_RETURN_NULL();

	/* Ok, we really need geos now ;) */
	initGEOS(MAXIMUM_ALIGNOF);

	vgeoms = palloc(sizeof(Geometry *)*nelems);
	offset = 0;
	for (i=0; i<nelems; i++)
	{
		PG_LWGEOM *geom = (PG_LWGEOM *)(ARR_DATA_PTR(array)+offset);
		offset += INTALIGN(geom->size);

		vgeoms[i] = POSTGIS2GEOS(geom);
		if ( ! i )
		{
			SRID = pglwgeom_getSRID(geom);
		}
		else
		{
			if ( SRID != pglwgeom_getSRID(geom) )
			{
	elog(ERROR, "polygonize: operation on mixed SRID geometries");
	PG_RETURN_NULL();
			}
		}
	}

#ifdef PGIS_DEBUG
	elog(NOTICE, "polygonize_garray: invoking GEOSpolygonize");
#endif

	geos_result = GEOSpolygonize(vgeoms, nelems);
#ifdef PGIS_DEBUG
	elog(NOTICE, "polygonize_garray: GEOSpolygonize returned");
#endif
	//pfree(vgeoms);
	if ( ! geos_result ) PG_RETURN_NULL();

	GEOSSetSRID(geos_result, SRID);
	result = GEOS2POSTGIS(geos_result, is3d);
	GEOSdeleteGeometry(geos_result);
	if ( result == NULL )
	{
		elog(ERROR, "GEOS2POSTGIS returned an error");
		PG_RETURN_NULL(); //never get here
	}

	//compressType(result);

	PG_RETURN_POINTER(result);

}

Datum JTSnoop(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(JTSnoop);
Datum JTSnoop(PG_FUNCTION_ARGS)
{
	elog(ERROR, "JTS support is disabled");
	PG_RETURN_NULL();
}

PG_FUNCTION_INFO_V1(postgis_jts_version);
Datum postgis_jts_version(PG_FUNCTION_ARGS)
{
	PG_RETURN_NULL();
}
