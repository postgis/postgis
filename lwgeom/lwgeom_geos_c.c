#include "postgres.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "fmgr.h"


#include "lwgeom_pg.h"
#include "liblwgeom.h"
#include "profile.h"
#include "wktparse.h"
#include "geos_c.h"
#include "lwgeom_rtree.h"

#include <string.h>


#ifdef PROFILE
#warning PROFILE enabled!
#endif

/*
 * Define this to have have many notices printed
 * during postgis->geos and geos->postgis conversions
 */
/* #define PGIS_DEBUG_CONVERTER 1 */
#ifdef PGIS_DEBUG_CONVERTER
#define PGIS_DEBUG_POSTGIS2GEOS 1
#define PGIS_DEBUG_GEOS2POSTGIS 1
#endif /* PGIS_DEBUG_CONVERTER */


#include "utils/memutils.h"
#include "executor/spi.h"
#include "access/hash.h"
#include "utils/hsearch.h"

/*
** GEOS prepared geometry is only available from GEOS 3.1 onwards
*/
#if GEOS_VERNUM >= 31
#define PREPARED_GEOM 1
#warning COMPILING PREPARED GEOMETRY
#endif

/* 
** Cache structure. Keys are unique for a unique geometry, usually the row
** primary key is passed in. This avoid the need to do a memcmp to test
** for geometry equality at each function invocation. The argnum gives
** the number of function arguments we are caching. Intersects requires that
** both arguments be checked for cacheability, while Contains only requires
** that the containing argument be checked. Both the Geometry and the 
** PreparedGeometry have to be cached, because the PreparedGeometry
** contains a reference to the geometry.
*/
#ifdef PREPARED_GEOM
typedef struct
{
    char type;
	PG_LWGEOM*                    pg_geom1;
	PG_LWGEOM*                    pg_geom2;
	size_t                        pg_geom1_size;
	size_t                        pg_geom2_size;
	int32                         argnum;
	const GEOSPreparedGeometry*   prepared_geom;
	GEOSGeometry*                 geom;
	MemoryContext                 context;
} PrepGeomCache;

/* Utility function to pull or prepare the current cache */
PrepGeomCache *GetPrepGeomCache(FunctionCallInfoData *fcinfo, PG_LWGEOM *pg_geom1, PG_LWGEOM *pg_geom2);
#endif

/* #define PGIS_DEBUG 1 */
/* #define WKB_CONVERSION 1 */

/*
 *
 * WARNING: buffer-based GeomUnion has been disabled due to
 *          limitations in the GEOS code (it would only work
 *	    against polygons)
 *
 * Fuzzy way of finding out how many points to stuff
 * in each chunk: 680 * Mb of memory
 *
 * The example below is for about 32 MB (fuzzy pragmatic check)
 *
 */
/* #define UNITE_USING_BUFFER 1 */
/* #define MAXGEOMSPOINTS 21760 */

/* PROTOTYPES start */

Datum relate_full(PG_FUNCTION_ARGS);
Datum relate_pattern(PG_FUNCTION_ARGS);
Datum disjoint(PG_FUNCTION_ARGS);
Datum touches(PG_FUNCTION_ARGS);
Datum intersects(PG_FUNCTION_ARGS);
Datum crosses(PG_FUNCTION_ARGS);
Datum within(PG_FUNCTION_ARGS);
Datum contains(PG_FUNCTION_ARGS);
Datum containsproperly(PG_FUNCTION_ARGS);
Datum covers(PG_FUNCTION_ARGS);
Datum overlaps(PG_FUNCTION_ARGS);
Datum isvalid(PG_FUNCTION_ARGS);
Datum buffer(PG_FUNCTION_ARGS);
Datum intersection(PG_FUNCTION_ARGS);
Datum convexhull(PG_FUNCTION_ARGS);
Datum topologypreservesimplify(PG_FUNCTION_ARGS);
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
Datum LWGEOM_buildarea(PG_FUNCTION_ARGS);
Datum linemerge(PG_FUNCTION_ARGS);



LWGEOM *GEOS2LWGEOM(GEOSGeom geom, char want3d);
PG_LWGEOM *GEOS2POSTGIS(GEOSGeom geom, char want3d);
GEOSGeom POSTGIS2GEOS(PG_LWGEOM *g);
GEOSGeom LWGEOM2GEOS(LWGEOM *g);

void errorIfGeometryCollection(PG_LWGEOM *g1, PG_LWGEOM *g2);
int point_in_polygon_rtree(RTREE_NODE **root, int ringCount, LWPOINT *point);
int point_in_multipolygon_rtree(RTREE_NODE **root, int polyCount, int ringCount, LWPOINT *point);
int point_in_polygon(LWPOLY *polygon, LWPOINT *point);
int point_in_multipolygon(LWMPOLY *mpolygon, LWPOINT *pont);

/* PROTOTYPES end */

PG_FUNCTION_INFO_V1(postgis_geos_version);
Datum postgis_geos_version(PG_FUNCTION_ARGS)
{
	const char *ver = GEOSversion();
	text *result;
	result = (text *) palloc(VARHDRSZ  + strlen(ver));
	SET_VARSIZE(result, VARHDRSZ + strlen(ver));
	memcpy(VARDATA(result), ver, strlen(ver));
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
	GEOSGeom g1, g2, geos_result=NULL;
	int SRID=-1;
	size_t offset;
#ifdef PGIS_DEBUG
	static int call=1;
#endif

#ifdef PGIS_DEBUG
	call++;
	lwnotice("GEOS incremental union (call %d)", call);
#endif


	datum = PG_GETARG_DATUM(0);

	/* Null array, null geometry (should be empty?) */
	if ( (Pointer *)datum == NULL ) PG_RETURN_NULL();

	array = DatumGetArrayTypeP(datum);

	nelems = ArrayGetNItems(ARR_NDIM(array), ARR_DIMS(array));

#ifdef PGIS_DEBUG
	elog(NOTICE, "unite_garray: number of elements: %d", nelems);
#endif

	if ( nelems == 0 ) PG_RETURN_NULL();

	/* One-element union is the element itself */
	if ( nelems == 1 ) PG_RETURN_POINTER((PG_LWGEOM *)(ARR_DATA_PTR(array)));

	/* Ok, we really need geos now ;) */
	initGEOS(lwnotice, lwnotice);

	offset = 0;
	for (i=0; i<nelems; i++)
	{
		PG_LWGEOM *geom = (PG_LWGEOM *)(ARR_DATA_PTR(array)+offset);
		offset += INTALIGN(VARSIZE(geom));

		pgis_geom = geom;

#ifdef PGIS_DEBUG
		elog(NOTICE, "geom %d @ %p", i, geom);
#endif

		/* Check is3d flag */
		if ( TYPE_HASZ(geom->type) ) is3d = 1;

		/* Check SRID homogeneity and initialize geos result */
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
			errorIfSRIDMismatch(SRID, pglwgeom_getSRID(geom));
		}
		
		g1 = POSTGIS2GEOS(pgis_geom);

#ifdef PGIS_DEBUG
		elog(NOTICE, "unite_garray(%d): adding geom %d to union (%s)",
				call, i, lwgeom_typename(TYPE_GETTYPE(geom->type)));
#endif

		g2 = GEOSUnion(g1,geos_result);
		if ( g2 == NULL )
		{
			GEOSGeom_destroy(g1);
			GEOSGeom_destroy(geos_result);
			elog(ERROR,"GEOS union() threw an error!");
		}
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(geos_result);
		geos_result = g2;
	}

	GEOSSetSRID(geos_result, SRID);
	result = GEOS2POSTGIS(geos_result, is3d);
	GEOSGeom_destroy(geos_result);
	if ( result == NULL )
	{
		elog(ERROR, "GEOS2POSTGIS returned an error");
		PG_RETURN_NULL(); /* never get here */
	}

	/* compressType(result); */

	PG_RETURN_POINTER(result);

}

#else /* def UNITE_USING_BUFFER */

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
	GEOSGeom *geoms, collection;
	GEOSGeom g1, geos_result=NULL;
	int SRID=-1;
	size_t offset;
#ifdef PGIS_DEBUG
	static int call=1;
#endif

#ifdef PGIS_DEBUG
	call++;
	lwnotice("GEOS buffer union (call %d)", call);
#endif


	datum = PG_GETARG_DATUM(0);

	/* Null array, null geometry (should be empty?) */
	if ( (Pointer *)datum == NULL ) PG_RETURN_NULL();

	array = DatumGetArrayTypeP(datum);

	nelems = ArrayGetNItems(ARR_NDIM(array), ARR_DIMS(array));

#ifdef PGIS_DEBUG
	elog(NOTICE, "unite_garray: number of elements: %d", nelems);
#endif

	if ( nelems == 0 ) PG_RETURN_NULL();

	/* One-element union is the element itself */
	if ( nelems == 1 ) PG_RETURN_POINTER((PG_LWGEOM *)(ARR_DATA_PTR(array)));

	geoms = lwalloc(sizeof(GEOSGeom)*nelems);

	/* We need geos here */
	initGEOS(lwnotice, lwnotice);

	offset = 0; i=0;
	ngeoms = 0; npoints=0;
#ifdef PGIS_DEBUG
 	lwnotice("Nelems %d, MAXGEOMSPOINST %d", nelems, MAXGEOMSPOINTS);
#endif
	while (!result) 
	{
		PG_LWGEOM *geom = (PG_LWGEOM *)(ARR_DATA_PTR(array)+offset);
		offset += INTALIGN(VARSIZE(geom));

		/* Check is3d flag */
		if ( TYPE_HASZ(geom->type) ) is3d = 1;

		/* Check SRID homogeneity  */
		if ( ! i ) SRID = pglwgeom_getSRID(geom);
		else errorIfSRIDMismatch(SRID, pglwgeom_getSRID(geom));

		geoms[ngeoms] = g1 = POSTGIS2GEOS(geom);

		npoints += GEOSGetNumCoordinate(geoms[ngeoms]);

		++ngeoms;
		++i;

#if PGIS_DEBUG > 1
		lwnotice("Loop %d, npoints: %d", i, npoints);
#endif

		/*
		 * Maximum count of geometry points reached
		 * or end of them, collect and buffer(0).
		 */
		if ( (npoints>=MAXGEOMSPOINTS && ngeoms>1) || i==nelems)
		{
#if PGIS_DEBUG > 1
			lwnotice(" CHUNK (ngeoms:%d, npoints:%d, left:%d)",
				ngeoms, npoints, nelems-i);
#endif

			collection = GEOSMakeCollection(GEOS_GEOMETRYCOLLECTION,
				geoms, ngeoms);

			geos_result = GEOSBuffer(collection, 0, 0);
			if ( geos_result == NULL )
			{
				GEOSGeom_destroy(g1);
				lwerror("GEOS buffer() threw an error!");
			}
			GEOSGeom_destroy(collection);

#if PGIS_DEBUG > 1
			lwnotice(" Buffer() executed");
#endif

			/*
			 * If there are no other geoms in input
			 * we've finished, otherwise we push
			 * the result back on the input stack.
			 */
			if ( i == nelems )
			{
#if PGIS_DEBUG > 1
i				lwnotice("  Final result points: %d",
					GEOSGetNumCoordinate(geos_result));
#endif
				GEOSSetSRID(geos_result, SRID);
				result = GEOS2POSTGIS(geos_result, is3d);
				GEOSGeom_destroy(geos_result);

#if PGIS_DEBUG > 1
				lwnotice(" Result computed");
#endif

			}
			else
			{
				geoms[0] = geos_result;
				ngeoms=1;
				npoints = GEOSGetNumCoordinate(geoms[0]);
#if PGIS_DEBUG > 1
	lwnotice("  Result pushed back on lwgeoms array (npoints:%d)", npoints);
#endif
			}
		}
	}


	/* compressType(result); */

	PG_RETURN_POINTER(result);

}

#endif /* def UNITE_USING_BUFFER */


/*
 * select geomunion(
 *      'POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))',
 *      'POLYGON((5 5, 15 5, 15 7, 5 7, 5 5))'
 * );
 *
 */
PG_FUNCTION_INFO_V1(geomunion);
Datum geomunion(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	int is3d;
	int SRID;
	GEOSGeom g1,g2,g3;
	PG_LWGEOM *result;

#ifdef PGIS_DEBUG
	elog(NOTICE,"in geomunion");
#endif

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	is3d = ( TYPE_HASZ(geom1->type) ) ||
		( TYPE_HASZ(geom2->type) );

	SRID = pglwgeom_getSRID(geom1);
	errorIfSRIDMismatch(SRID, pglwgeom_getSRID(geom2));

	initGEOS(lwnotice, lwnotice);

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

#ifdef PGIS_DEBUG
	elog(NOTICE,"g1=%s", GEOSGeomToWKT(g1));
	elog(NOTICE,"g2=%s", GEOSGeomToWKT(g2));
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	g3 = GEOSUnion(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

#ifdef PGIS_DEBUG
	elog(NOTICE,"g3=%s", GEOSGeomToWKT(g3));
#endif

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (g3 == NULL)
	{
		elog(ERROR,"GEOS union() threw an error!");
		PG_RETURN_NULL(); /* never get here */
	}


	GEOSSetSRID(g3, SRID);

#ifdef PROFILE
	profstart(PROF_G2P);
#endif
	result = GEOS2POSTGIS(g3, is3d);
#ifdef PROFILE
	profstop(PROF_G2P);
#endif

	GEOSGeom_destroy(g3);

	if (result == NULL)
	{
		elog(ERROR,"GEOS union() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); /*never get here */
	}

	/* compressType(result); */

#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("geos",geom1, geom2, result);
#endif

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_POINTER(result);
}


/*
 *  select symdifference(
 *      'POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))',
 *      'POLYGON((5 5, 15 5, 15 7, 5 7, 5 5))');
 */
PG_FUNCTION_INFO_V1(symdifference);
Datum symdifference(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	GEOSGeom g1,g2,g3;
	PG_LWGEOM *result;
	int is3d;
	int SRID;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	is3d = ( TYPE_HASZ(geom1->type) ) ||
		( TYPE_HASZ(geom2->type) );

	SRID = pglwgeom_getSRID(geom1);
	errorIfSRIDMismatch(SRID, pglwgeom_getSRID(geom2));

	initGEOS(lwnotice, lwnotice);

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
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		PG_RETURN_NULL(); /*never get here */
	}

#ifdef PGIS_DEBUG
	elog(NOTICE,"result: %s", GEOSGeomToWKT(g3));
#endif

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
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		GEOSGeom_destroy(g3);
		elog(ERROR,"GEOS symdifference() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); /*never get here */
	}

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);
	GEOSGeom_destroy(g3);

	/* compressType(result); */

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
	GEOSGeom g1,g3;
	PG_LWGEOM *result;
	int SRID;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	SRID = pglwgeom_getSRID(geom1);

	initGEOS(lwnotice, lwnotice);

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
		GEOSGeom_destroy(g1);
		PG_RETURN_NULL(); /* never get here */
	}

#ifdef PGIS_DEBUG
	elog(NOTICE,"result: %s", GEOSGeomToWKT(g3));
#endif

	GEOSSetSRID(g3, SRID);

#ifdef PROFILE
	profstart(PROF_G2P);
#endif
	result = GEOS2POSTGIS(g3, TYPE_HASZ(geom1->type));
#ifdef PROFILE
	profstart(PROF_P2G1);
#endif

	if (result == NULL)
	{
		GEOSGeom_destroy(g1);

		GEOSGeom_destroy(g3);
		elog(ERROR,"GEOS boundary() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); /* never get here */
	}



	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g3);

	/* compressType(result);   */

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
	GEOSGeom g1, g3;
	PG_LWGEOM *result;
	LWGEOM *lwout;
	int SRID;
	BOX2DFLOAT4 bbox;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	SRID = pglwgeom_getSRID(geom1);

	initGEOS(lwnotice, lwnotice);

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
		GEOSGeom_destroy(g1);
		PG_RETURN_NULL(); /* never get here */
	}


#ifdef PGIS_DEBUG
	elog(NOTICE,"result: %s", GEOSGeomToWKT(g3));
#endif

	GEOSSetSRID(g3, SRID);

#ifdef PROFILE
	profstart(PROF_G2P);
#endif
	lwout = GEOS2LWGEOM(g3, TYPE_HASZ(geom1->type));
#ifdef PROFILE
	profstop(PROF_G2P);
#endif
	if (lwout == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g3);
		elog(ERROR,"convexhull() failed to convert GEOS geometry to LWGEOM");
		PG_RETURN_NULL(); /* never get here */
	}

	/* Copy input bbox if any */
	if ( getbox2d_p(SERIALIZED_FORM(geom1), &bbox) )
	{
		lwout->bbox = box2d_clone(&bbox);
	}

	result = pglwgeom_serialize(lwout);
	if (result == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g3);
		elog(ERROR,"GEOS convexhull() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); /* never get here */
	}
	lwgeom_release(lwout);
	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g3);


	/* compressType(result);   */

#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("geos",geom1, NULL, result);
#endif

	PG_FREE_IF_COPY(geom1, 0);

	PG_RETURN_POINTER(result);

}

#if GEOS_VERNUM >= 30 

PG_FUNCTION_INFO_V1(topologypreservesimplify);
Datum topologypreservesimplify(PG_FUNCTION_ARGS)
{
	PG_LWGEOM	*geom1;
	double	tolerance;
	GEOSGeom g1,g3;
	PG_LWGEOM *result;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	tolerance = PG_GETARG_FLOAT8(1);

	initGEOS(lwnotice, lwnotice);

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
	g3 = GEOSTopologyPreserveSimplify(g1,tolerance);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	if (g3 == NULL)
	{
		elog(ERROR,"GEOS topologypreservesimplify() threw an error!");
		GEOSGeom_destroy(g1);
		PG_RETURN_NULL(); /* never get here */
	}


#ifdef PGIS_DEBUG
	elog(NOTICE,"result: %s", GEOSGeomToWKT(g3));
#endif

	GEOSSetSRID(g3, pglwgeom_getSRID(geom1));

#ifdef PROFILE
	profstart(PROF_G2P);
#endif
	result = GEOS2POSTGIS(g3, TYPE_HASZ(geom1->type));
#ifdef PROFILE
	profstop(PROF_G2P);
#endif
	if (result == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g3);
		elog(ERROR,"GEOS topologypreservesimplify() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); /* never get here */
	}
	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g3);


	/* compressType(result); */

#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("geos",geom1, NULL, result);
#endif

	PG_FREE_IF_COPY(geom1, 0);

	PG_RETURN_POINTER(result);
}
#endif

PG_FUNCTION_INFO_V1(buffer);
Datum buffer(PG_FUNCTION_ARGS)
{
	PG_LWGEOM	*geom1;
	double	size;
	GEOSGeom g1,g3;
	PG_LWGEOM *result;
	int quadsegs = 8; /* the default */

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	size = PG_GETARG_FLOAT8(1);
	if ( PG_NARGS() > 2 ) quadsegs = PG_GETARG_INT32(2);

	initGEOS(lwnotice, lwnotice);

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
		GEOSGeom_destroy(g1);
		PG_RETURN_NULL(); /* never get here */
	}


#ifdef PGIS_DEBUG
	elog(NOTICE,"result: %s", GEOSGeomToWKT(g3));
#endif

	GEOSSetSRID(g3, pglwgeom_getSRID(geom1));

#ifdef PROFILE
	profstart(PROF_G2P);
#endif
	result = GEOS2POSTGIS(g3, TYPE_HASZ(geom1->type));
#ifdef PROFILE
	profstop(PROF_G2P);
#endif
	if (result == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g3);
		elog(ERROR,"GEOS buffer() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); /* never get here */
	}
	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g3);


	/* compressType(result); */

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
	GEOSGeom g1,g2,g3;
	PG_LWGEOM *result;
	int is3d;
	int SRID;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	is3d = ( TYPE_HASZ(geom1->type) ) ||
		( TYPE_HASZ(geom2->type) );

	SRID = pglwgeom_getSRID(geom1);
	errorIfSRIDMismatch(SRID, pglwgeom_getSRID(geom2));

	initGEOS(lwnotice, lwnotice);

#ifdef PGIS_DEBUG
	elog(NOTICE,"intersection() START");
#endif

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

#ifdef PGIS_DEBUG
	elog(NOTICE," constructed geometrys - calling geos");
	elog(NOTICE," g1 = %s", GEOSGeomToWKT(g1));
	elog(NOTICE," g2 = %s", GEOSGeomToWKT(g2));
/*elog(NOTICE,"g2 is valid = %i",GEOSisvalid(g2)); */
/*elog(NOTICE,"g1 is valid = %i",GEOSisvalid(g1)); */
#endif




#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	g3 = GEOSIntersection(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

#ifdef PGIS_DEBUG
	elog(NOTICE," intersection finished");
#endif

	if (g3 == NULL)
	{
		elog(ERROR,"GEOS Intersection() threw an error!");
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		PG_RETURN_NULL(); /* never get here */
	}


#ifdef PGIS_DEBUG
	elog(NOTICE,"result: %s", GEOSGeomToWKT(g3) ) ;
#endif

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
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		GEOSGeom_destroy(g3);
		elog(ERROR,"GEOS Intersection() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); /* never get here */
	}



	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);
	GEOSGeom_destroy(g3);

	/* compressType(result); */

#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("geos",geom1, geom2, result);
#endif

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_POINTER(result);
}

/*
 * select difference(
 *      'POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))',
 *	'POLYGON((5 5, 15 5, 15 7, 5 7, 5 5))');
 */
PG_FUNCTION_INFO_V1(difference);
Datum difference(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	GEOSGeom g1,g2,g3;
	PG_LWGEOM *result;
	int is3d;
	int SRID;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	is3d = ( TYPE_HASZ(geom1->type) ) ||
		( TYPE_HASZ(geom2->type) );

	SRID = pglwgeom_getSRID(geom1);
	errorIfSRIDMismatch(SRID, pglwgeom_getSRID(geom2));

	initGEOS(lwnotice, lwnotice);

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
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		PG_RETURN_NULL(); /* never get here */
	}

#ifdef PGIS_DEBUG
  	elog(NOTICE,"result: %s", GEOSGeomToWKT(g3) ) ;
#endif

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
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		GEOSGeom_destroy(g3);
		elog(ERROR,"GEOS difference() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); /* never get here */
	}

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);
	GEOSGeom_destroy(g3);

	/* compressType(result); */

#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("geos",geom1, geom2, result);
#endif

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_POINTER(result);
}


/* select pointonsurface('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))'); */
PG_FUNCTION_INFO_V1(pointonsurface);
Datum pointonsurface(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	GEOSGeom g1,g3;
	PG_LWGEOM *result;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	initGEOS(lwnotice, lwnotice);

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
	g3 = GEOSPointOnSurface(g1);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	if (g3 == NULL)
	{
		elog(ERROR,"GEOS pointonsurface() threw an error!");
		GEOSGeom_destroy(g1);
		PG_RETURN_NULL(); /* never get here */
	}

#ifdef PGIS_DEBUG
	elog(NOTICE,"result: %s", GEOSGeomToWKT(g3) ) ;
#endif

	GEOSSetSRID(g3, pglwgeom_getSRID(geom1));
#ifdef PROFILE
	profstart(PROF_G2P);
#endif
	result = GEOS2POSTGIS(g3, TYPE_HASZ(geom1->type));
#ifdef PROFILE
	profstop(PROF_G2P);
#endif
	if (result == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g3);
		elog(ERROR,"GEOS pointonsurface() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); /* never get here */
	}

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g3);

	/* compressType(result);  */

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
	GEOSGeom geosgeom, geosresult;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	initGEOS(lwnotice, lwnotice);

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
	geosresult = GEOSGetCentroid(geosgeom);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	if ( geosresult == NULL )
	{
		GEOSGeom_destroy(geosgeom);
		elog(ERROR,"GEOS getCentroid() threw an error!");
		PG_RETURN_NULL(); 
	}

	GEOSSetSRID(geosresult, pglwgeom_getSRID(geom));

#ifdef PROFILE
	profstart(PROF_G2P);
#endif
	result = GEOS2POSTGIS(geosresult, TYPE_HASZ(geom->type));
#ifdef PROFILE
	profstop(PROF_G2P);
#endif
	if (result == NULL)
	{
		GEOSGeom_destroy(geosgeom);
		GEOSGeom_destroy(geosresult);
		elog(ERROR,"Error in GEOS-PGIS conversion");
		PG_RETURN_NULL(); 
	}
	GEOSGeom_destroy(geosgeom);
	GEOSGeom_destroy(geosresult);

#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("geos",geom, NULL, result);
#endif

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(result);
}



/*---------------------------------------------*/



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
	GEOSGeom g1;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	initGEOS(lwnotice, lwnotice);

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
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = GEOSisValid(g1);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	GEOSGeom_destroy(g1);
	if (result == 2)
	{
		elog(ERROR,"GEOS isvalid() threw an error!");
		PG_RETURN_NULL(); /*never get here */
	}


#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("geos",geom1, NULL, NULL);
#endif

	PG_FREE_IF_COPY(geom1, 0);

	PG_RETURN_BOOL(result);
}


/*
 * overlaps(PG_LWGEOM g1,PG_LWGEOM g2)
 * returns  if GEOS::g1->overlaps(g2) returns true
 * throws an error (elog(ERROR,...)) if GEOS throws an error
 */
PG_FUNCTION_INFO_V1(overlaps);
Datum overlaps(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	GEOSGeom g1,g2;
	bool result;
	BOX2DFLOAT4 box1, box2;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);
	errorIfSRIDMismatch(pglwgeom_getSRID(geom1), pglwgeom_getSRID(geom2));

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

	initGEOS(lwnotice, lwnotice);

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
	result = GEOSOverlaps(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);
	if (result == 2)
	{
		elog(ERROR,"GEOS overlaps() threw an error!");
		PG_RETURN_NULL(); /* never get here */
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
	GEOSGeom g1,g2;
	bool result;
	BOX2DFLOAT4 box1, box2;
        int type1, type2;
    	LWGEOM *lwgeom;
        /* LWMPOLY *mpoly; */
        LWPOINT *point;
        RTREE_POLY_CACHE *poly_cache;
        MemoryContext old_context;
#ifdef PREPARED_GEOM
	PrepGeomCache *prep_cache;
#endif

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);
	errorIfSRIDMismatch(pglwgeom_getSRID(geom1), pglwgeom_getSRID(geom2));

#ifdef PGIS_DEBUG
    lwnotice("Contains: entered", type1, type2);
#endif

	/*
	 * short-circuit 1: if geom2 bounding box is not completely inside
	 * geom1 bounding box we can prematurely return FALSE.
	 * Do the test IFF BOUNDING BOX AVAILABLE.
	 */
    if ( getbox2d_p(SERIALIZED_FORM(geom1), &box1) &&
         getbox2d_p(SERIALIZED_FORM(geom2), &box2) )
	{
		if ( ( box2.xmin < box1.xmin ) || ( box2.xmax > box1.xmax ) ||
		     ( box2.ymin < box1.ymin ) || ( box2.ymax > box1.ymax ) ) 
		{
#ifdef PGIS_DEBUG
    lwnotice("Contains: bbox short circuit", type1, type2);
#endif
		    PG_RETURN_BOOL(FALSE);
		}
	}
        /*
         * short-circuit 2: if geom2 is a point and geom1 is a polygon
         * call the point-in-polygon function.
         */
        type1 = lwgeom_getType((uchar)SERIALIZED_FORM(geom1)[0]);
        type2 = lwgeom_getType((uchar)SERIALIZED_FORM(geom2)[0]);
#ifdef PGIS_DEBUG
    lwnotice("Contains: type1: %d, type2: %d", type1, type2);
#endif
    	if((type1 == POLYGONTYPE || type1 == MULTIPOLYGONTYPE) && type2 == POINTTYPE)
		{
#ifdef PGIS_DEBUG
                lwnotice("Point in Polygon test requested...short-circuiting.");
#endif

        		lwgeom = lwgeom_deserialize(SERIALIZED_FORM(geom1));
                point = lwpoint_deserialize(SERIALIZED_FORM(geom2));

                /*
                 * Switch the context to the function-scope context,
                 * retrieve the appropriate cache object, cache it for 
                 * future use, then switch back to the local context.
                 */                 
                old_context = MemoryContextSwitchTo(fcinfo->flinfo->fn_mcxt);
                poly_cache = retrieveCache(lwgeom, SERIALIZED_FORM(geom1), fcinfo->flinfo->fn_extra);
                fcinfo->flinfo->fn_extra = poly_cache;
                MemoryContextSwitchTo(old_context);

				if( poly_cache->ringIndices ) 
				{
#ifdef PGIS_DEBUG
                lwnotice("R-Tree Point in Polygon test.");
#endif
				    
					result = point_in_multipolygon_rtree(poly_cache->ringIndices, poly_cache->polyCount, poly_cache->ringCount, point);
				}
				else if ( type1 == POLYGONTYPE ) 
				{
#ifdef PGIS_DEBUG
                lwnotice("Brute force Point in Polygon test.");
#endif
					result = point_in_polygon((LWPOLY*)lwgeom, point);
				}
				else if ( type1 == MULTIPOLYGONTYPE ) 
				{
#ifdef PGIS_DEBUG
                lwnotice("Brute force Point in Polygon test.");
#endif
					result = point_in_multipolygon((LWMPOLY*)lwgeom, point);
				}
				else {
				/* Gulp! Should not be here... */
					elog(ERROR,"Type isn't poly or multipoly!");
					PG_RETURN_NULL(); 
				}
        		PG_FREE_IF_COPY(geom1, 0);
        		PG_FREE_IF_COPY(geom2, 1);
        		lwgeom_release((LWGEOM *)lwgeom);
        		lwgeom_release((LWGEOM *)point);
				if( result == 1 ) /* completely inside */
				{
					PG_RETURN_BOOL(TRUE);
				}
				else
				{
					PG_RETURN_BOOL(FALSE);
				}
        } 
        else 
        {
        }
        
	initGEOS(lwnotice, lwnotice);

#ifdef PREPARED_GEOM
	prep_cache = GetPrepGeomCache( fcinfo, geom1, 0 );

	if ( prep_cache && prep_cache->prepared_geom && prep_cache->argnum == 1 )
	{
		g1 = POSTGIS2GEOS(geom2);
#ifdef PGIS_DEBUG
		lwnotice("containsPrepared: cache is live, running preparedcontains");
#endif
		result = GEOSPreparedContains( prep_cache->prepared_geom, g1);
		GEOSGeom_destroy(g1);
	}
	else
#endif
	{
	g1 = POSTGIS2GEOS(geom1);
	g2 = POSTGIS2GEOS(geom2);
	result = GEOSContains(g1,g2);
	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);
	}
	
	if (result == 2)
	{
		elog(ERROR,"GEOS contains() threw an error!");
		PG_RETURN_NULL(); /* never get here */
	}

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}


/*
 * Described at 
 * http://lin-ear-th-inking.blogspot.com/2007/06/subtleties-of-ogc-covers-spatial.html
 */
PG_FUNCTION_INFO_V1(covers);
Datum covers(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	GEOSGeom g1,g2;
	bool result;
	BOX2DFLOAT4 box1, box2;
        int type1, type2;
        LWGEOM *lwgeom;
        /* LWMPOLY *mpoly; */
        LWPOINT *point;
        RTREE_POLY_CACHE *poly_cache;
        MemoryContext old_context;
        char *patt = "******FF*";
#ifdef PREPARED_GEOM
	PrepGeomCache *prep_cache;
#endif

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);
	errorIfSRIDMismatch(pglwgeom_getSRID(geom1), pglwgeom_getSRID(geom2));

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
        /*
         * short-circuit 2: if geom2 is a point and geom1 is a polygon
         * call the point-in-polygon function.
         */
        type1 = lwgeom_getType((uchar)SERIALIZED_FORM(geom1)[0]);
        type2 = lwgeom_getType((uchar)SERIALIZED_FORM(geom2)[0]);
    	if((type1 == POLYGONTYPE || type1 == MULTIPOLYGONTYPE) && type2 == POINTTYPE)
        {
#ifdef PGIS_DEBUG
                lwnotice("Point in Polygon test requested...short-circuiting.");
#endif

                lwgeom = lwgeom_deserialize(SERIALIZED_FORM(geom1));
                point = lwpoint_deserialize(SERIALIZED_FORM(geom2));
#ifdef PGIS_DEBUG
                lwnotice("Precall point_in_polygon %p, %p", lwgeom, point);
#endif

                /*
                 * Switch the context to the function-scope context,
                 * retrieve the appropriate cache object, cache it for 
                 * future use, then switch back to the local context.
                 */                 
                old_context = MemoryContextSwitchTo(fcinfo->flinfo->fn_mcxt);
                poly_cache = retrieveCache(lwgeom, SERIALIZED_FORM(geom1), fcinfo->flinfo->fn_extra);
                fcinfo->flinfo->fn_extra = poly_cache;
                MemoryContextSwitchTo(old_context);

				if( poly_cache->ringIndices ) 
				{
					result = point_in_multipolygon_rtree(poly_cache->ringIndices, poly_cache->polyCount, poly_cache->ringCount, point);
				}
				else if ( type1 == POLYGONTYPE ) 
				{
					result = point_in_polygon((LWPOLY*)lwgeom, point);
				}
				else if ( type1 == MULTIPOLYGONTYPE ) 
				{
					result = point_in_multipolygon((LWMPOLY*)lwgeom, point);
				}
				else {
					/* Gulp! Should not be here... */
					elog(ERROR,"Type isn't poly or multipoly!");
					PG_RETURN_NULL(); 
				}

                PG_FREE_IF_COPY(geom1, 0);
                PG_FREE_IF_COPY(geom2, 1);
				lwgeom_release((LWGEOM *)lwgeom);
				lwgeom_release((LWGEOM *)point);
				if( result != -1 ) /* not outside */
				{
					PG_RETURN_BOOL(TRUE);
				}
				else
				{
					PG_RETURN_BOOL(FALSE);
				}
        } 
        else 
        {
#ifdef PGIS_DEBUG
                lwnotice("Covers: type1: %d, type2: %d", type1, type2);
#endif
        }
        
	initGEOS(lwnotice, lwnotice);
#ifdef PREPARED_GEOM
	prep_cache = GetPrepGeomCache( fcinfo, geom1, 0 );

	if ( prep_cache && prep_cache->prepared_geom && prep_cache->argnum == 1 )
	{
		GEOSGeom g1 = POSTGIS2GEOS(geom2);
		result = GEOSPreparedCovers( prep_cache->prepared_geom, g1);
		GEOSGeom_destroy(g1);
	}
	else
#endif
	{
	g1 = POSTGIS2GEOS(geom1);
	g2 = POSTGIS2GEOS(geom2);
	result = GEOSRelatePattern(g1,g2,patt);
	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);
	}
	
	if (result == 2)
	{
		elog(ERROR,"GEOS covers() threw an error!");
		PG_RETURN_NULL(); /* never get here */
	}

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(within);
Datum within(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	GEOSGeom g1,g2;
	bool result;
	BOX2DFLOAT4 box1, box2;
        LWGEOM *lwgeom;
        LWPOINT *point;
        int type1, type2;
        MemoryContext old_context;
        RTREE_POLY_CACHE *poly_cache;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);
	errorIfSRIDMismatch(pglwgeom_getSRID(geom1), pglwgeom_getSRID(geom2));

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
        /*
         * short-circuit 2: if geom1 is a point and geom2 is a polygon
         * call the point-in-polygon function.
         */
        type1 = lwgeom_getType((uchar)SERIALIZED_FORM(geom1)[0]);
        type2 = lwgeom_getType((uchar)SERIALIZED_FORM(geom2)[0]);
    	if((type2 == POLYGONTYPE || type2 == MULTIPOLYGONTYPE) && type1 == POINTTYPE)
        {
#ifdef PGIS_DEBUG
                lwnotice("Point in Polygon test requested...short-circuiting.");
#endif

                point = lwpoint_deserialize(SERIALIZED_FORM(geom1));
                lwgeom = lwgeom_deserialize(SERIALIZED_FORM(geom2));

                /*
                 * Switch the context to the function-scope context,
                 * retrieve the appropriate cache object, cache it for 
                 * future use, then switch back to the local context.
                 */                 
                old_context = MemoryContextSwitchTo(fcinfo->flinfo->fn_mcxt);
                poly_cache = retrieveCache(lwgeom, SERIALIZED_FORM(geom2), fcinfo->flinfo->fn_extra);
                fcinfo->flinfo->fn_extra = poly_cache;
                MemoryContextSwitchTo(old_context);

				if( poly_cache->ringIndices ) 
				{
					result = point_in_multipolygon_rtree(poly_cache->ringIndices, poly_cache->polyCount, poly_cache->ringCount, point);
				}
				else if ( type2 == POLYGONTYPE ) 
				{
					result = point_in_polygon((LWPOLY*)lwgeom, point);
				}
				else if ( type2 == MULTIPOLYGONTYPE ) 
				{
					result = point_in_multipolygon((LWMPOLY*)lwgeom, point);
				}
				else {
					/* Gulp! Should not be here... */
					elog(ERROR,"Type isn't poly or multipoly!");
					PG_RETURN_NULL(); 
				}

                PG_FREE_IF_COPY(geom1, 0);
                PG_FREE_IF_COPY(geom2, 1);
                lwgeom_release((LWGEOM *)lwgeom);
                lwgeom_release((LWGEOM *)point);
				if( result == 1 ) /* completely inside */
				{
					PG_RETURN_BOOL(TRUE);
				}
				else
				{
					PG_RETURN_BOOL(FALSE);
				}
        }

	initGEOS(lwnotice, lwnotice);

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
	result = GEOSWithin(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (result == 2)
	{
		elog(ERROR,"GEOS within() threw an error!");
		PG_RETURN_NULL(); /* never get here */
	}

#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("geos",geom1, geom2, NULL);
#endif

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}


/*
 * Described at:
 * http://lin-ear-th-inking.blogspot.com/2007/06/subtleties-of-ogc-covers-spatial.html
 */
PG_FUNCTION_INFO_V1(coveredby);
Datum coveredby(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	GEOSGeom g1,g2;
	bool result;
	BOX2DFLOAT4 box1, box2;
        LWGEOM *lwgeom;
        LWPOINT *point;
        int type1, type2;
        MemoryContext old_context;
        RTREE_POLY_CACHE *poly_cache;
        char *patt = "**F**F***";

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);
	errorIfSRIDMismatch(pglwgeom_getSRID(geom1), pglwgeom_getSRID(geom2));

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

#ifdef PGIS_DEBUG
                lwnotice("bounding box short-circuit missed.");
#endif
	}
        /*
         * short-circuit 2: if geom1 is a point and geom2 is a polygon
         * call the point-in-polygon function.
         */
        type1 = lwgeom_getType((uchar)SERIALIZED_FORM(geom1)[0]);
        type2 = lwgeom_getType((uchar)SERIALIZED_FORM(geom2)[0]);
    	if((type2 == POLYGONTYPE || type2 == MULTIPOLYGONTYPE) && type1 == POINTTYPE)
        {
#ifdef PGIS_DEBUG
                lwnotice("Point in Polygon test requested...short-circuiting.");
#endif

                point = lwpoint_deserialize(SERIALIZED_FORM(geom1));
                lwgeom = lwgeom_deserialize(SERIALIZED_FORM(geom2));

                /*
                 * Switch the context to the function-scope context,
                 * retrieve the appropriate cache object, cache it for 
                 * future use, then switch back to the local context.
                 */                 
                old_context = MemoryContextSwitchTo(fcinfo->flinfo->fn_mcxt);
                poly_cache = retrieveCache(lwgeom, SERIALIZED_FORM(geom2), fcinfo->flinfo->fn_extra);
                fcinfo->flinfo->fn_extra = poly_cache;
                MemoryContextSwitchTo(old_context);

				if( poly_cache->ringIndices ) 
				{
					result = point_in_multipolygon_rtree(poly_cache->ringIndices, poly_cache->polyCount, poly_cache->ringCount, point);
				}
				else if ( type2 == POLYGONTYPE ) 
				{
					result = point_in_polygon((LWPOLY*)lwgeom, point);
				}
				else if ( type2 == MULTIPOLYGONTYPE ) 
				{
					result = point_in_multipolygon((LWMPOLY*)lwgeom, point);
				}
				else {
					/* Gulp! Should not be here... */
					elog(ERROR,"Type isn't poly or multipoly!");
					PG_RETURN_NULL(); 
				}
				
                PG_FREE_IF_COPY(geom1, 0);
                PG_FREE_IF_COPY(geom2, 1);
                lwgeom_release((LWGEOM *)lwgeom);
                lwgeom_release((LWGEOM *)point);
				if( result != -1 ) /* not outside */
				{
					PG_RETURN_BOOL(TRUE);
				}
				else
				{
					PG_RETURN_BOOL(FALSE);
				}
        }

	initGEOS(lwnotice, lwnotice);

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
	result = GEOSRelatePattern(g1,g2,patt);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (result == 2)
	{
		elog(ERROR,"GEOS coveredby() threw an error!");
		PG_RETURN_NULL(); /* never get here */
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
	GEOSGeom g1,g2;
	bool result;
	BOX2DFLOAT4 box1, box2;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);
	errorIfSRIDMismatch(pglwgeom_getSRID(geom1), pglwgeom_getSRID(geom2));

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

	initGEOS(lwnotice, lwnotice);

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
	result = GEOSCrosses(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (result == 2)
	{
		elog(ERROR,"GEOS crosses() threw an error!");
		PG_RETURN_NULL(); /* never get here */
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
	uchar *serialized_poly;
	GEOSGeom g1,g2;
	bool result;
	BOX2DFLOAT4 box1, box2;
	int type1, type2, polytype;
	LWPOINT *point;
	LWGEOM *lwgeom;
        MemoryContext old_context;
        RTREE_POLY_CACHE *poly_cache;
#ifdef PREPARED_GEOM
	PrepGeomCache *prep_cache;
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);
	errorIfSRIDMismatch(pglwgeom_getSRID(geom1), pglwgeom_getSRID(geom2));

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

	/*
	 * short-circuit 2: if the geoms are a point and a polygon,
	 * call the point_outside_polygon function.
	 */
	type1 = lwgeom_getType((uchar)SERIALIZED_FORM(geom1)[0]);
	type2 = lwgeom_getType((uchar)SERIALIZED_FORM(geom2)[0]);
	if( (type1 == POINTTYPE && (type2 == POLYGONTYPE || type2 == MULTIPOLYGONTYPE)) || 
	    (type2 == POINTTYPE && (type1 == POLYGONTYPE || type1 == MULTIPOLYGONTYPE)))
	{
#ifdef PGIS_DEBUG
		lwnotice("Point in Polygon test requested...short-circuiting.");
#endif
        if( type1 == POINTTYPE ) {
		    point = lwpoint_deserialize(SERIALIZED_FORM(geom1));
		    lwgeom = lwgeom_deserialize(SERIALIZED_FORM(geom2));
			serialized_poly = SERIALIZED_FORM(geom2);
			polytype = type2;
        } else {
		    point = lwpoint_deserialize(SERIALIZED_FORM(geom2));
		    lwgeom = lwgeom_deserialize(SERIALIZED_FORM(geom1));
			serialized_poly = SERIALIZED_FORM(geom1);
			polytype = type1;
		}

        /*
         * Switch the context to the function-scope context,
         * retrieve the appropriate cache object, cache it for 
         * future use, then switch back to the local context.
         */                 
		old_context = MemoryContextSwitchTo(fcinfo->flinfo->fn_mcxt);
		poly_cache = retrieveCache(lwgeom, SERIALIZED_FORM(geom2), fcinfo->flinfo->fn_extra);
		fcinfo->flinfo->fn_extra = poly_cache;
		MemoryContextSwitchTo(old_context);

		if( poly_cache->ringIndices ) 
		{
			result = point_in_multipolygon_rtree(poly_cache->ringIndices, poly_cache->polyCount, poly_cache->ringCount, point);
		}
		else if ( polytype == POLYGONTYPE ) 
		{
			result = point_in_polygon((LWPOLY*)lwgeom, point);
		}
		else if ( polytype == MULTIPOLYGONTYPE ) 
		{
			result = point_in_multipolygon((LWMPOLY*)lwgeom, point);
		}
		else {
			/* Gulp! Should not be here... */
			elog(ERROR,"Type isn't poly or multipoly!");
			PG_RETURN_NULL(); 
		}

		PG_FREE_IF_COPY(geom1, 0);
		PG_FREE_IF_COPY(geom2, 1);
		lwgeom_release((LWGEOM *)lwgeom);
		lwgeom_release((LWGEOM *)point);
		if( result != -1 ) /* not outside */ 
		{
			PG_RETURN_BOOL(TRUE);
		}
		else {
			PG_RETURN_BOOL(FALSE);
		}
	}


	initGEOS(lwnotice, lwnotice);
#ifdef PREPARED_GEOM
	prep_cache = GetPrepGeomCache( fcinfo, geom1, geom2 );

	if ( prep_cache && prep_cache->prepared_geom )
	{
		if ( prep_cache->argnum == 1 )
		{
			GEOSGeom g = POSTGIS2GEOS(geom2);
			result = GEOSPreparedIntersects( prep_cache->prepared_geom, g);
			GEOSGeom_destroy(g);
		}
		else
		{
			GEOSGeom g = POSTGIS2GEOS(geom1);
			result = GEOSPreparedIntersects( prep_cache->prepared_geom, g);
			GEOSGeom_destroy(g);
		}
	}
	else
#endif
	{
	g1 = POSTGIS2GEOS(geom1 );
	g2 = POSTGIS2GEOS(geom2 );
	result = GEOSIntersects(g1,g2);
	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);
	}	

	if (result == 2)
	{
		elog(ERROR,"GEOS intersects() threw an error!");
		PG_RETURN_NULL(); /* never get here */
	}

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(touches);
Datum touches(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	GEOSGeom g1,g2;
	bool result;
	BOX2DFLOAT4 box1, box2;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);
	errorIfSRIDMismatch(pglwgeom_getSRID(geom1), pglwgeom_getSRID(geom2));

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

	initGEOS(lwnotice, lwnotice);

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
	result = GEOSTouches(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (result == 2)
	{
		elog(ERROR,"GEOS touches() threw an error!");
		PG_RETURN_NULL(); /* never get here */
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
	GEOSGeom g1,g2;
	bool result;
	BOX2DFLOAT4 box1, box2;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);
	errorIfSRIDMismatch(pglwgeom_getSRID(geom1), pglwgeom_getSRID(geom2));

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
		if ( box2.ymin > box1.ymax ) PG_RETURN_BOOL(TRUE);
	}

	initGEOS(lwnotice, lwnotice);

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
	result = GEOSDisjoint(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif
	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (result == 2)
	{
		elog(ERROR,"GEOS disjoin() threw an error!");
		PG_RETURN_NULL(); /* never get here */
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
	GEOSGeom g1,g2;
	int i;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);
	errorIfSRIDMismatch(pglwgeom_getSRID(geom1), pglwgeom_getSRID(geom2));

	initGEOS(lwnotice, lwnotice);

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

	/*
	** Need to make sure 't' and 'f' are upper-case before handing to GEOS 
	*/
	for( i = 0; i < strlen(patt); i++ ) {
		if( patt[i] == 't' ) patt[i] = 'T';
		if( patt[i] == 'f' ) patt[i] = 'F';
	}


#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = GEOSRelatePattern(g1,g2,patt);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif
	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);
	pfree(patt);

	if (result == 2)
	{
		elog(ERROR,"GEOS relate_pattern() threw an error!");
		PG_RETURN_NULL(); /* never get here */
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
	GEOSGeom g1,g2;
	char *relate_str;
	int len;
	text *result;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

#ifdef PGIS_DEBUG
	elog(NOTICE,"in relate_full()");
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);
	errorIfSRIDMismatch(pglwgeom_getSRID(geom1), pglwgeom_getSRID(geom2));


	initGEOS(lwnotice, lwnotice);

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

#ifdef PGIS_DEBUG
 	elog(NOTICE,"constructed geometries ");
#endif

	if ((g1==NULL) || (g2 == NULL))
		elog(NOTICE,"g1 or g2 are null");

#ifdef PGIS_DEBUG
	elog(NOTICE,GEOSGeomToWKT(g1));
	elog(NOTICE,GEOSGeomToWKT(g2));

	/*elog(NOTICE,"valid g1 = %i", GEOSisvalid(g1));*/
	/*elog(NOTICE,"valid g2 = %i",GEOSisvalid(g2));*/

	elog(NOTICE,"about to relate()");
#endif


#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	relate_str = GEOSRelate(g1, g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

#ifdef PGIS_DEBUG
 	elog(NOTICE,"finished relate()");
#endif

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (relate_str == NULL)
	{
		elog(ERROR,"GEOS relate() threw an error!");
		PG_RETURN_NULL(); /* never get here */
	}

	len = strlen(relate_str) + VARHDRSZ;

	result= palloc(len);
	SET_VARSIZE(result, len);

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

/*============================== */

PG_FUNCTION_INFO_V1(geomequals);
Datum geomequals(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	GEOSGeom g1,g2;
	bool result;
	BOX2DFLOAT4 box1, box2;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);
	errorIfSRIDMismatch(pglwgeom_getSRID(geom1), pglwgeom_getSRID(geom2));

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

	initGEOS(lwnotice, lwnotice);

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
	result = GEOSEquals(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif
	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (result == 2)
	{
		elog(ERROR,"GEOS equals() threw an error!");
		PG_RETURN_NULL(); /*never get here */
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
	GEOSGeom g1;
	int result;

#ifdef PGIS_DEBUG
	elog(NOTICE,"issimple called");
#endif

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	if (lwgeom_getnumgeometries(SERIALIZED_FORM(geom)) == 0)
		PG_RETURN_BOOL(true);

	initGEOS(lwnotice, lwnotice);

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
	GEOSGeom_destroy(g1);

	if (result == 2)
	{
		elog(ERROR,"GEOS issimple() threw an error!");
		PG_RETURN_NULL(); /*never get here */
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
	GEOSGeom g1;
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

	initGEOS(lwnotice, lwnotice);

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
	GEOSGeom_destroy(g1);

	if (result == 2)
	{
		elog(ERROR,"GEOS isring() threw an error!");
		PG_RETURN_NULL(); 
	}

#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("geos",geom, NULL, NULL);
#endif

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_BOOL(result);
}



/*= GEOS <=> POSTGIS CONVERSION ========================= */

/*-----=GEOS2POSTGIS= */

#ifdef WKB_CONVERSION

/* Return an LWGEOM from a GEOSGeom */
LWGEOM *
GEOS2LWGEOM(GEOSGeom geom, char want3d)
{
	size_t size;
	char *wkb;
	LWGEOM *lwgeom;

	if ( want3d ) GEOS_setWKBOutputDims(3);
	else GEOS_setWKBOutputDims(2);

	wkb = GEOSGeomToWKB_buf(geom, &size);
	lwgeom = lwgeom_from_ewkb(wkb, size);

	return lwgeom;
}

PG_LWGEOM *
GEOS2POSTGIS(GEOSGeom geom, char want3d)
{
	size_t size;
	char *wkb;
	PG_LWGEOM *pglwgeom, *ret;

	if ( want3d ) GEOS_setWKBOutputDims(3);
	else GEOS_setWKBOutputDims(2);

	wkb = GEOSGeomToWKB_buf(geom, &size);
	if ( ! wkb )
	{
		lwerror("GEOS failed to export WKB");
	}
	pglwgeom = pglwgeom_from_ewkb(wkb, size);
	if ( ! pglwgeom )
	{
		lwerror("GEOS2POSTGIS: lwgeom_from_ewkb returned NULL");
	}

	if ( is_worth_caching_pglwgeom_bbox(pglwgeom) )
	{
		ret = (PG_LWGEOM *)DatumGetPointer(DirectFunctionCall1(
			LWGEOM_addBBOX, PointerGetDatum(pglwgeom)));
		lwfree(pglwgeom);
	}
	else
	{
		ret = pglwgeom;
	}

	return ret;
}

#else /* !ndef WKB_CONVERSION */

/* Return a POINTARRAY from a GEOSCoordSeq */
POINTARRAY *
ptarray_from_GEOSCoordSeq(GEOSCoordSeq cs, char want3d)
{
	unsigned int dims=2;
	unsigned int size, i, ptsize;
	uchar *points, *ptr;
	POINTARRAY *ret;

#ifdef PGIS_DEBUG_GEOS2POSTGIS 
	lwnotice("ptarray_fromGEOSCoordSeq called");
#endif

	if ( ! GEOSCoordSeq_getSize(cs, &size) )
			lwerror("Exception thrown");

#ifdef PGIS_DEBUG_GEOS2POSTGIS 
	lwnotice(" GEOSCoordSeq size: %d", size);
#endif

	if ( want3d )
	{
		if ( ! GEOSCoordSeq_getDimensions(cs, &dims) )
			lwerror("Exception thrown");
#ifdef PGIS_DEBUG_GEOS2POSTGIS 
		lwnotice(" GEOSCoordSeq dimensions: %d", dims);
#endif
		/* forget higher dimensions (if any) */
		if ( dims > 3 ) dims = 3;
	}

#ifdef PGIS_DEBUG_GEOS2POSTGIS 
	lwnotice(" output dimensions: %d", dims);
#endif

	ptsize = sizeof(double)*dims;

	ret = ptarray_construct((dims==3), 0, size);

	points = ret->serialized_pointlist;
	ptr = points;
	for (i=0; i<size; i++)
	{
		POINT3DZ point;
		GEOSCoordSeq_getX(cs, i, &(point.x));
		GEOSCoordSeq_getY(cs, i, &(point.y));
		if ( dims >= 3 ) GEOSCoordSeq_getZ(cs, i, &(point.z));
		memcpy(ptr, &point, ptsize);
		ptr += ptsize;
	}

	return ret;
}

/* Return an LWGEOM from a Geometry */
LWGEOM *
GEOS2LWGEOM(GEOSGeom geom, char want3d)
{
	int type = GEOSGeomTypeId(geom) ;
	bool hasZ = GEOSHasZ(geom);
	int SRID = GEOSGetSRID(geom);

	/* GEOS's 0 is equivalent to our -1 as for SRID values */
	if ( SRID == 0 ) SRID = -1;

	if ( ! hasZ )
	{
		if ( want3d )
		{
#ifdef PGIS_DEBUG
			elog(NOTICE, "Geometry has no Z, won't provide one");
#endif
			want3d = 0;
		}
	}

	switch (type)
	{
		GEOSCoordSeq cs;
		POINTARRAY *pa, **ppaa;
		GEOSGeom g;
		LWGEOM **geoms;
		unsigned int i, ngeoms;

		case GEOS_POINT:
#ifdef PGIS_DEBUG_GEOS2POSTGIS
	lwnotice("lwgeom_from_geometry: it's a Point");
#endif
			cs = GEOSGeom_getCoordSeq(geom);
			pa = ptarray_from_GEOSCoordSeq(cs, want3d);
			return (LWGEOM *)lwpoint_construct(SRID, NULL, pa);
			
		case GEOS_LINESTRING:
		case GEOS_LINEARRING:
#ifdef PGIS_DEBUG_GEOS2POSTGIS
	lwnotice("lwgeom_from_geometry: it's a LineString or LinearRing");
#endif
			cs = GEOSGeom_getCoordSeq(geom);
			pa = ptarray_from_GEOSCoordSeq(cs, want3d);
			return (LWGEOM *)lwline_construct(SRID, NULL, pa);

		case GEOS_POLYGON:
#ifdef PGIS_DEBUG_GEOS2POSTGIS
	lwnotice("lwgeom_from_geometry: it's a Polygon");
#endif
			ngeoms = GEOSGetNumInteriorRings(geom);
			ppaa = lwalloc(sizeof(POINTARRAY *)*(ngeoms+1));
			g = GEOSGetExteriorRing(geom);
			cs = GEOSGeom_getCoordSeq(g);
			ppaa[0] = ptarray_from_GEOSCoordSeq(cs, want3d);
			for (i=0; i<ngeoms; i++)
			{
				g = GEOSGetInteriorRingN(geom, i);
				cs = GEOSGeom_getCoordSeq(g);
				ppaa[i+1] = ptarray_from_GEOSCoordSeq(cs,
					want3d);
			}
			return (LWGEOM *)lwpoly_construct(SRID, NULL,
				ngeoms+1, ppaa);

		case GEOS_MULTIPOINT:
		case GEOS_MULTILINESTRING:
		case GEOS_MULTIPOLYGON:
		case GEOS_GEOMETRYCOLLECTION:
#ifdef PGIS_DEBUG_GEOS2POSTGIS
	lwnotice("lwgeom_from_geometry: it's a Collection or Multi");
#endif
			ngeoms = GEOSGetNumGeometries(geom);
			geoms = NULL;
			if ( ngeoms )
			{
				geoms = lwalloc(sizeof(LWGEOM *)*ngeoms);
				for (i=0; i<ngeoms; i++)
				{
					g = GEOSGetGeometryN(geom, i);
					geoms[i] = GEOS2LWGEOM(g, want3d);
				}
			}
			return (LWGEOM *)lwcollection_construct(type,
				SRID, NULL, ngeoms, geoms);

		default:
			lwerror("GEOS2LWGEOM: unknown geometry type: %d", type);
			return NULL;

	}

}


PG_LWGEOM *
GEOS2POSTGIS(GEOSGeom geom, char want3d)
{
	LWGEOM *lwgeom;
	PG_LWGEOM *result;

	lwgeom = GEOS2LWGEOM(geom, want3d);
	if ( ! lwgeom )
	{
		lwerror("GEOS2POSTGIS: GEOS2LWGEOM returned NULL");
		return NULL;
	}

#ifdef PGIS_DEBUG_GEOS2POSTGIS
	lwnotice("GEOS2POSTGIS: GEOS2LWGEOM returned a %s", lwgeom_summary(lwgeom, 0)); 
#endif

	if ( is_worth_caching_lwgeom_bbox(lwgeom) )
	{
		lwgeom_addBBOX(lwgeom);
	}

	result = pglwgeom_serialize(lwgeom);

	return result;
}

#endif /* def WKB_CONVERSION */

/*-----=POSTGIS2GEOS= */


#ifdef WKB_CONVERSION

GEOSGeom LWGEOM2GEOS(LWGEOM *);

GEOSGeom 
LWGEOM2GEOS(LWGEOM *lwgeom)
{
	size_t size;
	char *wkb;
	GEOSGeom geom;

	wkb = lwgeom_to_ewkb(lwgeom, getMachineEndian(), &size);
	geom = GEOSGeomFromWKB_buf(wkb, size);
	return geom;
}

GEOSGeom 
POSTGIS2GEOS(PG_LWGEOM *pglwgeom)
{
	size_t size;
	char *wkb;
	GEOSGeom geom;

	wkb = pglwgeom_to_ewkb(pglwgeom, getMachineEndian(), &size);
	if ( ! wkb )
	{
		lwerror("Postgis failed to export EWKB %s:%d", __FILE__, __LINE__);
	}
	geom = GEOSGeomFromWKB_buf(wkb, size);
	lwfree(wkb);
	if ( ! geom ) {
		lwerror("POSTGIS2GEOS conversion failed");
	}

#ifdef PGIS_DEBUG_CONVERTER
	wkb = GEOSGeomToWKT(geom);
	lwnotice("GEOS geom: %s", wkb);
#endif

	return geom;
}

#else /* ndef WKB_CONVERSION */

GEOSCoordSeq ptarray_to_GEOSCoordSeq(POINTARRAY *);
GEOSGeom LWGEOM2GEOS(LWGEOM *lwgeom);

GEOSCoordSeq
ptarray_to_GEOSCoordSeq(POINTARRAY *pa)
{
	unsigned int dims = 2;
	unsigned int size, i;
	POINT3DZ p;
	GEOSCoordSeq sq;

	if ( TYPE_HASZ(pa->dims) ) dims = 3;
	size = pa->npoints;

	sq = GEOSCoordSeq_create(size, dims);
	if ( ! sq ) lwerror("Error creating GEOS Coordinate Sequence");

	for (i=0; i<size; i++)
	{
		getPoint3dz_p(pa, i, &p);
#ifdef PGIS_DEBUG_CONVERTER
lwnotice("Point: %g,%g,%g", p.x, p.y, p.z);
#endif
		GEOSCoordSeq_setX(sq, i, p.x);
		GEOSCoordSeq_setY(sq, i, p.y);
		if ( dims == 3 ) GEOSCoordSeq_setZ(sq, i, p.z);
	}
	return sq;
}

GEOSGeom
LWGEOM2GEOS(LWGEOM *lwgeom)
{
	GEOSCoordSeq sq;
	GEOSGeom g, shell, *geoms;
	/*
        LWGEOM *tmp;
	*/
	unsigned int ngeoms, i;
	int type;
	int geostype;
#ifdef PGIS_DEBUG_POSTGIS2GEOS 
	char *wkt;
#endif

#ifdef PGIS_DEBUG_POSTGIS2GEOS 
	lwnotice("LWGEOM2GEOS got a %s", lwgeom_typename(type));
#endif

        if(has_arc(lwgeom))
        {
#ifdef PGIS_DEBUG_CALLS
                lwnotice("LWGEOM2GEOS_c: arced geometry found.");
#endif
		lwerror("Exception in LWGEOM2GEOS: curved geometry not supported.");
		/*
                tmp = lwgeom;
                lwgeom = lwgeom_segmentize(tmp, 32);
#ifdef PGIS_DEBUG_CALLS
                lwnotice("LWGEOM2GEOM_c: was %p, is %p", tmp, lwgeom);
#endif
		*/
        }
        type = TYPE_GETTYPE(lwgeom->type);
	switch (type)
	{
		LWPOINT *lwp;
		LWPOLY *lwpoly;
		LWLINE *lwl;
		LWCOLLECTION *lwc;

		case POINTTYPE:
			lwp = (LWPOINT *)lwgeom;
			sq = ptarray_to_GEOSCoordSeq(lwp->point);
			g = GEOSGeom_createPoint(sq);
			if ( ! g ) lwerror("Exception in LWGEOM2GEOS");
			break;
		case LINETYPE:
			lwl = (LWLINE *)lwgeom;
			sq = ptarray_to_GEOSCoordSeq(lwl->points);
			g = GEOSGeom_createLineString(sq);
			if ( ! g ) lwerror("Exception in LWGEOM2GEOS");
			break;

		case POLYGONTYPE:
			lwpoly = (LWPOLY *)lwgeom;
			sq = ptarray_to_GEOSCoordSeq(lwpoly->rings[0]);
			shell = GEOSGeom_createLinearRing(sq);
	if ( ! shell ) return NULL;
	/*lwerror("LWGEOM2GEOS: exception during polygon shell conversion"); */
			ngeoms = lwpoly->nrings-1;
			geoms = malloc(sizeof(GEOSGeom)*ngeoms);
			for (i=1; i<lwpoly->nrings; ++i)
			{
				sq = ptarray_to_GEOSCoordSeq(lwpoly->rings[i]);
				geoms[i-1] = GEOSGeom_createLinearRing(sq);
	if ( ! geoms[i-1] ) return NULL;
	/*lwerror("LWGEOM2GEOS: exception during polygon hole conversion"); */
			}
			g = GEOSGeom_createPolygon(shell, geoms, ngeoms);
			if ( ! g ) return NULL;
			free(geoms);
			break;
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case COLLECTIONTYPE:
			if ( type == MULTIPOINTTYPE )
				geostype = GEOS_MULTIPOINT;
			else if ( type == MULTILINETYPE )
				geostype = GEOS_MULTILINESTRING;
			else if ( type == MULTIPOLYGONTYPE )
				geostype = GEOS_MULTIPOLYGON;
			else
				geostype = GEOS_GEOMETRYCOLLECTION;

			lwc = (LWCOLLECTION *)lwgeom;
			ngeoms = lwc->ngeoms;
			geoms = malloc(sizeof(GEOSGeom)*ngeoms);

			for (i=0; i<ngeoms; ++i)
			{
				geoms[i] = LWGEOM2GEOS(lwc->geoms[i]);
				if ( ! geoms[i] ) return NULL;
			}
			g = GEOSGeom_createCollection(geostype, geoms, ngeoms);
			if ( ! g ) return NULL;
			free(geoms);
			break;

		default:
#ifdef PGIS_DEBUG
                        lwerror("LWGEOM2GEOS_c: Unknown geometry type: %d", type);
#else 
			lwerror("Unknown geometry type: %d", type);
#endif
			return NULL;
	}

	GEOSSetSRID(g, lwgeom->SRID);

#ifdef PGIS_DEBUG_POSTGIS2GEOS 
	wkt = GEOSGeomToWKT(g);
	lwnotice("LWGEOM2GEOS: GEOSGeom: %s", wkt);
	/*
        if(tmp != NULL) lwgeom_release(tmp);
	*/
	free(wkt);
#endif

	return g;
}

GEOSGeom 
POSTGIS2GEOS(PG_LWGEOM *pglwgeom)
{
	GEOSGeom ret;
	LWGEOM *lwgeom = lwgeom_deserialize(SERIALIZED_FORM(pglwgeom));
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

#endif /* WKB_CONVERSION */

PG_FUNCTION_INFO_V1(GEOSnoop);
Datum GEOSnoop(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	GEOSGeom geosgeom;
	PG_LWGEOM *result;

	initGEOS(lwnotice, lwnotice);

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

	result = GEOS2POSTGIS(geosgeom, TYPE_HASZ(geom->type));
	GEOSGeom_destroy(geosgeom);

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
	GEOSGeom geos_result;
	GEOSGeom *vgeoms;
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

	array = DatumGetArrayTypeP(datum);

	nelems = ArrayGetNItems(ARR_NDIM(array), ARR_DIMS(array));

#ifdef PGIS_DEBUG
	elog(NOTICE, "polygonize_garray: number of elements: %d", nelems);
#endif

	if ( nelems == 0 ) PG_RETURN_NULL();

	/* Ok, we really need geos now ;) */
	initGEOS(lwnotice, lwnotice);

	vgeoms = palloc(sizeof(GEOSGeom)*nelems);
	offset = 0;
	for (i=0; i<nelems; i++)
	{
		PG_LWGEOM *geom = (PG_LWGEOM *)(ARR_DATA_PTR(array)+offset);
		offset += INTALIGN(VARSIZE(geom));

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

	geos_result = GEOSPolygonize(vgeoms, nelems);
#ifdef PGIS_DEBUG
	elog(NOTICE, "polygonize_garray: GEOSpolygonize returned");
#endif
	for (i=0; i<nelems; ++i) GEOSGeom_destroy(vgeoms[i]);
	pfree(vgeoms);

	if ( ! geos_result ) PG_RETURN_NULL();

	GEOSSetSRID(geos_result, SRID);
	result = GEOS2POSTGIS(geos_result, is3d);
	GEOSGeom_destroy(geos_result);
	if ( result == NULL )
	{
		elog(ERROR, "GEOS2POSTGIS returned an error");
		PG_RETURN_NULL(); /*never get here */
	}

	/*compressType(result); */

	PG_RETURN_POINTER(result);

}

PG_FUNCTION_INFO_V1(linemerge);
Datum linemerge(PG_FUNCTION_ARGS)
{
	PG_LWGEOM	*geom1;
	GEOSGeom g1,g3;
	PG_LWGEOM *result;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	initGEOS(lwnotice, lwnotice);

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
	g3 = GEOSLineMerge(g1);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	if (g3 == NULL)
	{
		elog(ERROR,"GEOS LineMerge() threw an error!");
		GEOSGeom_destroy(g1);
		PG_RETURN_NULL(); /*never get here */
	}


#ifdef PGIS_DEBUG
  	elog(NOTICE,"result: %s", GEOSGeomToWKT(g3) ) ;
#endif

	GEOSSetSRID(g3, pglwgeom_getSRID(geom1));

#ifdef PROFILE
	profstart(PROF_G2P);
#endif
	result = GEOS2POSTGIS(g3, TYPE_HASZ(geom1->type));
#ifdef PROFILE
	profstop(PROF_G2P);
#endif
	if (result == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g3);
		elog(ERROR,"GEOS LineMerge() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); /*never get here */
	}
	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g3);


	/* compressType(result); */

#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("geos",geom1, NULL, result);
#endif

	PG_FREE_IF_COPY(geom1, 0);

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

/*
 * Take a geometry and return an areal geometry
 * (Polygon or MultiPolygon).
 * Actually a wrapper around GEOSpolygonize, 
 * transforming the resulting collection into
 * a valid polygonzl Geometry.
 */
PG_FUNCTION_INFO_V1(LWGEOM_buildarea);
Datum LWGEOM_buildarea(PG_FUNCTION_ARGS)
{
	int is3d = 0;
	unsigned int i, ngeoms;
	PG_LWGEOM *result;
	LWGEOM *lwg;
	GEOSGeom geos_result, shp;
	GEOSGeom vgeoms[1];
	int SRID=-1;
#ifdef PGIS_DEBUG
	static int call=1;
#endif

#ifdef PGIS_DEBUG
	call++;
	lwnotice("buildarea called (call %d)", call);
#endif

	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	SRID = pglwgeom_getSRID(geom);
	is3d = TYPE_HASZ(geom->type);

#ifdef PGIS_DEBUG
	lwnotice("LWGEOM_buildarea got geom @ %x", geom);
#endif

	initGEOS(lwnotice, lwnotice);

	vgeoms[0] = POSTGIS2GEOS(geom);
	geos_result = GEOSPolygonize(vgeoms, 1);
	GEOSGeom_destroy(vgeoms[0]);

#ifdef PGIS_DEBUG
	lwnotice("GEOSpolygonize returned @ %x", geos_result);
#endif

	/* Null return from GEOSpolygonize */
	if ( ! geos_result ) PG_RETURN_NULL();

	/*
	 * We should now have a collection
	 */
#if PARANOIA_LEVEL > 0
	if ( GEOSGeometryTypeId(geos_result) != COLLECTIONTYPE )
	{
		GEOSGeom_destroy(geos_result);
		lwerror("Unexpected return from GEOSpolygonize");
		PG_RETURN_NULL();
	}
#endif

	ngeoms = GEOSGetNumGeometries(geos_result);

#ifdef PGIS_DEBUG
	lwnotice("GEOSpolygonize: ngeoms in polygonize output: %d", ngeoms);
#endif

	/*
	 * No geometries in collection, return NULL
	 */
	if ( ngeoms == 0 ) 
	{
		GEOSGeom_destroy(geos_result);
		PG_RETURN_NULL();
	}

	/*
	 * Return first geometry if we only have one in collection,
	 * to avoid the unnecessary Geometry clone below.
	 */
	if ( ngeoms == 1 )
	{
		shp = GEOSGetGeometryN(geos_result, 0);
		lwg = GEOS2LWGEOM(shp, is3d);
		lwg->SRID = SRID;
		result = pglwgeom_serialize(lwg);
		lwgeom_release(lwg);
		GEOSGeom_destroy(geos_result);
		PG_RETURN_POINTER(result);
	}

	/* 
	 * Iteratively invoke symdifference on outer rings
	 * as suggested by Carl Anderson:
	 * postgis-devel/2005-December/001805.html
	 */
	shp = NULL;
	for (i=0; i<ngeoms; ++i)
	{
		GEOSGeom extring, tmp;
		GEOSCoordSeq sq;

		/*
		 * Construct a Polygon from geometry i exterior ring
		 * We don't use GEOSGeom_clone on the ExteriorRing
		 * due to a bug in CAPI contained in GEOS 2.2 branch
		 * failing to properly return a LinearRing from
		 * a LinearRing clone.
		 */
		sq=GEOSCoordSeq_clone(GEOSGeom_getCoordSeq(
			GEOSGetExteriorRing(GEOSGetGeometryN( geos_result, i))
			));
		extring = GEOSGeom_createPolygon(
				GEOSGeom_createLinearRing(sq),
				NULL, 0
				);

		if ( extring == NULL ) /* exception */
		{
			lwerror("GEOSCreatePolygon threw an exception");
			PG_RETURN_NULL();
		}

		if ( shp == NULL )
		{
			shp = extring;
		}
		else
		{
			tmp = GEOSSymDifference(shp, extring);
			GEOSGeom_destroy(shp);
			GEOSGeom_destroy(extring);
			shp = tmp;
		}
	}

	GEOSGeom_destroy(geos_result);

	GEOSSetSRID(shp, SRID);
	result = GEOS2POSTGIS(shp, is3d);
	GEOSGeom_destroy(shp);

#if PARANOIA_LEVEL > 0
	if ( result == NULL )
	{
		lwerror("serialization error");
		PG_RETURN_NULL(); /*never get here */
	}

#endif

	PG_RETURN_POINTER(result);

}

/*********************************************************************************
**
**  PreparedGeometry implementations that cache intermediate indexed versions
**  of geometry in a special MemoryContext for re-used by future function
**  invocations.
**
**  By creating a memory context to hold the GEOS PreparedGeometry and Geometry
**  and making it a child of the fmgr memory context, we can get the memory held
**  by the GEOS objects released when the memory context delete callback is
**  invoked by the parent context.
**
*********************************************************************************/




#ifdef PREPARED_GEOM

/*
** Backend prepared hash table
**
** The memory context call-backs use a MemoryContext as the parameter
** so we need to map that over to actual references to GEOS objects to 
** delete.
**
** This hash table stores a key/value pair of MemoryContext/Geom* objects.
*/
static HTAB* PrepGeomHash = NULL;

#define PREPARED_BACKEND_HASH_SIZE	32		

typedef struct 
{
	MemoryContext context;
	const GEOSPreparedGeometry* prepared_geom;
	GEOSGeometry* geom;
} PrepGeomHashEntry;


/* Memory context hash table function prototypes */
uint32 mcxt_ptr_hasha(const void *key, Size keysize);
static void CreatePrepGeomHash(void);
static void AddPrepGeomHashEntry(PrepGeomHashEntry pghe);
static PrepGeomHashEntry *GetPrepGeomHashEntry(MemoryContext mcxt);
static void DeletePrepGeomHashEntry(MemoryContext mcxt);


/* Memory context cache function prototypes */
static void PreparedCacheInit(MemoryContext context);
static void PreparedCacheReset(MemoryContext context);
static void PreparedCacheDelete(MemoryContext context);
static bool PreparedCacheIsEmpty(MemoryContext context);
static void PreparedCacheStats(MemoryContext context, int level);
#ifdef MEMORY_CONTEXT_CHECKING
static void PreparedCacheCheck(MemoryContext context);
#endif

/* Memory context definition must match the current version of PostgreSQL */
static MemoryContextMethods PreparedCacheContextMethods = {
	NULL,
	NULL,
	NULL,
	PreparedCacheInit,
	PreparedCacheReset,
	PreparedCacheDelete,
	NULL,
	PreparedCacheIsEmpty,
	PreparedCacheStats
#ifdef MEMORY_CONTEXT_CHECKING
	, PreparedCacheCheck
#endif
};

static void
PreparedCacheInit(MemoryContext context)
{
	/*
	 * Do nothing as the cache is initialised when the transform()
	 * function is first called
	 */
}

static void
PreparedCacheDelete(MemoryContext context)
{
	PrepGeomHashEntry* pghe;

	/* Lookup the hash entry pointer in the global hash table so we can free it */
	pghe = GetPrepGeomHashEntry(context);

	if (!pghe)
		elog(ERROR, "PreparedCacheDelete: Trying to delete non-existant hash entry object with MemoryContext key (%p)", (void *)context);
#ifdef PGIS_DEBUG
	lwnotice("deleting geom object (%p) and prepared geom object (%p) with MemoryContext key (%p)", pghe->geom, pghe->prepared_geom, context);
#endif
	/* Free them */
	if( pghe->prepared_geom )
		GEOSPreparedGeom_destroy( pghe->prepared_geom );
	if( pghe->geom )
		GEOSGeom_destroy( pghe->geom );

	/* Remove the hash entry as it is no longer needed */
	DeletePrepGeomHashEntry(context);
}

static void
PreparedCacheReset(MemoryContext context)
{
	/*
	 * Do nothing, but we must supply a function since this call is mandatory according to tgl
	 * (see postgis-devel archives July 2007)
	 */
}

static bool
PreparedCacheIsEmpty(MemoryContext context)
{
	/*
	 * Always return false since this call is mandatory according to tgl
	 * (see postgis-devel archives July 2007)
	 */
	return FALSE;
}

static void
PreparedCacheStats(MemoryContext context, int level)
{
	/*
	 * Simple stats display function - we must supply a function since this call is mandatory according to tgl
	 * (see postgis-devel archives July 2007)
	 */

	fprintf(stderr, "%s: Prepared context\n", context->name);
}

#ifdef MEMORY_CONTEXT_CHECKING
static void
PreparedCacheCheck(MemoryContext context)
{
	/*
	 * Do nothing - stub required for when PostgreSQL is compiled
	 * with MEMORY_CONTEXT_CHECKING defined
	 */
}
#endif

/* TODO: put this in common are for both transform and prepared
** mcxt_ptr_hash
** Build a key from a pointer and a size value.
*/
uint32 
mcxt_ptr_hasha(const void *key, Size keysize)
{
	uint32 hashval;

	hashval = DatumGetUInt32(hash_any(key, keysize));

	return hashval;
}

static void
CreatePrepGeomHash(void)
{
	HASHCTL ctl;

	ctl.keysize = sizeof(MemoryContext);
	ctl.entrysize = sizeof(PrepGeomHashEntry);
	ctl.hash = mcxt_ptr_hasha;

	PrepGeomHash = hash_create("PostGIS Prepared Geometry Backend MemoryContext Hash", PREPARED_BACKEND_HASH_SIZE, &ctl, (HASH_ELEM | HASH_FUNCTION));
}

static void 
AddPrepGeomHashEntry(PrepGeomHashEntry pghe)
{
	bool found;
	void **key;
	PrepGeomHashEntry *he;

	/* The hash key is the MemoryContext pointer */
	key = (void *)&(pghe.context);
	
	he = (PrepGeomHashEntry *) hash_search(PrepGeomHash, key, HASH_ENTER, &found);
	if (!found)
	{
		/* Insert the entry into the new hash element */
		he->context = pghe.context;
		he->geom = pghe.geom;
		he->prepared_geom = pghe.prepared_geom;
	}
	else
	{
		elog(ERROR, "AddPrepGeomHashEntry: This memory context is already in use! (%p)", (void *)pghe.context);
	}
}

static PrepGeomHashEntry*
GetPrepGeomHashEntry(MemoryContext mcxt)
{
	void **key;
	PrepGeomHashEntry *he;

	/* The hash key is the MemoryContext pointer */
	key = (void *)&mcxt;

	/* Return the projection object from the hash */
	he = (PrepGeomHashEntry *) hash_search(PrepGeomHash, key, HASH_FIND, NULL);

	return he;
}


static void 
DeletePrepGeomHashEntry(MemoryContext mcxt)
{
	void **key;
	PrepGeomHashEntry *he;	

	/* The hash key is the MemoryContext pointer */
	key = (void *)&mcxt;

	/* Delete the projection object from the hash */
	he = (PrepGeomHashEntry *) hash_search(PrepGeomHash, key, HASH_REMOVE, NULL);

	he->prepared_geom = NULL;
	he->geom = NULL;

	if (!he)
		elog(ERROR, "DeletePrepGeomHashEntry: There was an error removing the geometry object from this MemoryContext (%p)", (void *)mcxt);
}

/*
** GetPrepGeomCache
**
** Pull the current prepared geometry from the cache or make
** one if there is not one available. Only prepare geometry
** if we are seeing a key for the second time. That way rapidly 
** cycling keys don't cause too much preparing.
**
*/
PrepGeomCache* 
GetPrepGeomCache(FunctionCallInfoData *fcinfo, PG_LWGEOM *pg_geom1, PG_LWGEOM *pg_geom2)
{
	MemoryContext old_context;
	PrepGeomCache* cache = fcinfo->flinfo->fn_extra;
	int copy_keys = 1;
	size_t pg_geom1_size = 0;
	size_t pg_geom2_size = 0;

    /* Make sure this isn't someone else's cache object. */
    if( cache && cache->type != 2 ) cache = NULL;

	if (!PrepGeomHash)
		CreatePrepGeomHash();

	if( pg_geom1 ) 
		pg_geom1_size = VARSIZE(pg_geom1) + VARHDRSZ;

	if( pg_geom2 ) 
		pg_geom2_size = VARSIZE(pg_geom2) + VARHDRSZ;

	if ( cache == NULL)
	{
		/*
		** Cache requested, but the cache isn't set up yet.
		** Set it up, but don't prepare the geometry yet.
		** That way if the next call is a cache miss we haven't
		** wasted time preparing a geometry we don't need.
		*/
		PrepGeomHashEntry pghe;
	
		old_context = MemoryContextSwitchTo(fcinfo->flinfo->fn_mcxt);
		cache = palloc(sizeof(PrepGeomCache));		
		MemoryContextSwitchTo(old_context);
	
        cache->type = 2;
		cache->prepared_geom = 0;
		cache->geom = 0;
		cache->argnum = 0;
		cache->pg_geom1 = 0;
		cache->pg_geom2 = 0;
		cache->pg_geom1_size = 0;
		cache->pg_geom2_size = 0;
		cache->context = MemoryContextCreate(T_AllocSetContext, 8192,
		                 &PreparedCacheContextMethods,
		                 fcinfo->flinfo->fn_mcxt,
		                 "PostGIS Prepared Geometry Context");

#ifdef PGIS_DEBUG 
		lwnotice("GetPrepGeomCache: creating cache: %p", cache);
#endif
		pghe.context = cache->context;
		pghe.geom = 0;
		pghe.prepared_geom = 0;
		AddPrepGeomHashEntry( pghe );

		fcinfo->flinfo->fn_extra = cache;

#ifdef PGIS_DEBUG 
		lwnotice("GetPrepGeomCache: adding context to hash: %p", cache);
#endif
	}
	else if ( pg_geom1 &&
	          cache->argnum != 2 &&
	          cache->pg_geom1_size == pg_geom1_size && 
	          memcmp(cache->pg_geom1, pg_geom1, pg_geom1_size) == 0)
	{
		if ( !cache->prepared_geom )
		{
			/*
			** Cache hit, but we haven't prepared our geometry yet.
			** Prepare it.
			*/
			PrepGeomHashEntry* pghe;
		
			cache->geom = POSTGIS2GEOS( pg_geom1 );
			cache->prepared_geom = GEOSPrepare( cache->geom );
			cache->argnum = 1;
#ifdef PGIS_DEBUG 
			lwnotice("GetPrepGeomCache: preparing obj in argument 1");
#endif

			pghe = GetPrepGeomHashEntry(cache->context);
			pghe->geom = cache->geom;
			pghe->prepared_geom = cache->prepared_geom;
#ifdef PGIS_DEBUG 
			lwnotice("GetPrepGeomCache: storing references to prepared obj in argument 1");
#endif
		}
		else
		{	
			/*
			** Cache hit, and we're good to go. Do nothing.
			*/
#ifdef PGIS_DEBUG 
			lwnotice("GetPrepGeomCache: cache hit, argument 1");
#endif
		}
		/* We don't need new keys until we have a cache miss */
		copy_keys = 0;
	}
	else if ( pg_geom2 && 
	          cache->argnum != 1 &&
	          cache->pg_geom2_size == pg_geom2_size && 
	          memcmp(cache->pg_geom2, pg_geom2, pg_geom2_size) == 0)
	{
		 	if ( !cache->prepared_geom )
			{
				/*
				** Cache hit on arg2, but we haven't prepared our geometry yet.
				** Prepare it.
				*/
				PrepGeomHashEntry* pghe;
				
				cache->geom = POSTGIS2GEOS( pg_geom2 );
				cache->prepared_geom = GEOSPrepare( cache->geom );
				cache->argnum = 2;
#ifdef PGIS_DEBUG 
				lwnotice("GetPrepGeomCache: preparing obj in argument 2");
#endif
			
				pghe = GetPrepGeomHashEntry(cache->context);
				pghe->geom = cache->geom;
				pghe->prepared_geom = cache->prepared_geom;
#ifdef PGIS_DEBUG 
				lwnotice("GetPrepGeomCache: storing references to prepared obj in argument 2");
#endif
			}
			else 
			{
				/*
				** Cache hit, and we're good to go. Do nothing.
				*/
#ifdef PGIS_DEBUG 
				lwnotice("GetPrepGeomCache: cache hit, argument 2");
#endif
			}
			/* We don't need new keys until we have a cache miss */
			copy_keys = 0;
	}
	else if ( cache->prepared_geom )
	{
		/*
		** No cache hits, so this must be a miss.
		** Destroy the GEOS objects, empty the cache.
		*/
		PrepGeomHashEntry* pghe;

		pghe = GetPrepGeomHashEntry(cache->context);
		pghe->geom = 0;
		pghe->prepared_geom = 0;

#ifdef PGIS_DEBUG 
		lwnotice("GetPrepGeomCache: cache miss, argument %d", cache->argnum);
#endif
		GEOSPreparedGeom_destroy( cache->prepared_geom );
		GEOSGeom_destroy( cache->geom );
		
		cache->prepared_geom = 0;
		cache->geom = 0;
		cache->argnum = 0;

	}
	
	if( copy_keys && pg_geom1 ) 
	{
		/*
		** If this is a new key (cache miss) we flip into the function
		** manager memory context and make a copy. We can't just store a pointer
		** because this copy will be pfree'd at the end of this function
		** call.
		*/
#ifdef PGIS_DEBUG 
		lwnotice("GetPrepGeomCache: copying pg_geom1 into cache");
#endif
		old_context = MemoryContextSwitchTo(fcinfo->flinfo->fn_mcxt);
		if( cache->pg_geom1 ) 
			pfree(cache->pg_geom1);
		cache->pg_geom1 = palloc(pg_geom1_size);
		MemoryContextSwitchTo(old_context);
		memcpy(cache->pg_geom1, pg_geom1, pg_geom1_size);
		cache->pg_geom1_size = pg_geom1_size;
	}
	if( copy_keys && pg_geom2 )
	{ 
#ifdef PGIS_DEBUG 
		lwnotice("GetPrepGeomCache: copying pg_geom2 into cache");
#endif
		old_context = MemoryContextSwitchTo(fcinfo->flinfo->fn_mcxt);
		if( cache->pg_geom2 ) 
			pfree(cache->pg_geom2);
		cache->pg_geom2 = palloc(pg_geom2_size);
		MemoryContextSwitchTo(old_context);
		memcpy(cache->pg_geom2, pg_geom2, pg_geom2_size);
		cache->pg_geom2_size = pg_geom2_size;
	}

	return cache;
	
}
#endif /* PREPARED_GEOM */

PG_FUNCTION_INFO_V1(containsproperly);
Datum containsproperly(PG_FUNCTION_ARGS)
{
#ifndef PREPARED_GEOM
	elog(ERROR,"Not implemented in this version!");
	PG_RETURN_NULL(); /* never get here */
#else
	PG_LWGEOM *				geom1;
	PG_LWGEOM *				geom2;
	bool 					result;
	BOX2DFLOAT4 			box1, box2;
	PrepGeomCache *	prep_cache;

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);
	errorIfSRIDMismatch(pglwgeom_getSRID(geom1), pglwgeom_getSRID(geom2));

	/*
	* short-circuit: if geom2 bounding box is not completely inside
	* geom1 bounding box we can prematurely return FALSE.
	* Do the test IFF BOUNDING BOX AVAILABLE.
	*/
	if ( getbox2d_p(SERIALIZED_FORM(geom1), &box1) &&
	                getbox2d_p(SERIALIZED_FORM(geom2), &box2) )
	{
		if (( box2.xmin < box1.xmin ) || ( box2.xmax > box1.xmax ) ||
		    ( box2.ymin < box1.ymin ) || ( box2.ymax > box1.ymax ))
			PG_RETURN_BOOL(FALSE);
	}

	prep_cache = GetPrepGeomCache( fcinfo, geom1, 0 );

	initGEOS(lwnotice, lwnotice);

	if ( prep_cache && prep_cache->prepared_geom && prep_cache->argnum == 1 )
	{
		GEOSGeom g = POSTGIS2GEOS(geom2);
		result = GEOSPreparedContainsProperly( prep_cache->prepared_geom, g);
		GEOSGeom_destroy(g);
	}
	else
	{
		GEOSGeom g1 = POSTGIS2GEOS(geom1);
		GEOSGeom g2 = POSTGIS2GEOS(geom2);
		result = GEOSRelatePattern( g1, g2, "T**FF*FF*" );
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
	}

	if (result == 2)
	{
		elog(ERROR,"GEOS contains() threw an error!");
		PG_RETURN_NULL(); /* never get here */
	}

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
#endif /* PREPARED_GEOM */
}
