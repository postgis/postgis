
#include "postgres.h"
#include "utils/array.h"
#include "utils/builtins.h"

#include "lwgeom.h"
#include "wktparse.h"

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
Datum centroid(PG_FUNCTION_ARGS);



#if USE_GEOS

/*
 * Define this to have have many notices printed
 * during postgis->geos and geos->postgis conversions
 */
#undef DEBUG_CONVERTER
#undef DEBUG_POSTGIS2GEOS
#undef DEBUG_GEOS2POSTGIS

/*
 * Define this to have profiling enabled
 */
#define PROFILE

#ifdef PROFILE
#include <sys/time.h>
#define PROF_P2G 0
#define PROF_G2P 1
#define PROF_GRUN 2
struct timeval profstart, profstop;
long proftime[3];
long profipts, profopts;
#define profstart(x) do { gettimeofday(&profstart, NULL); } while (0);
#define profstop(x) do { gettimeofday(&profstop, NULL); \
	proftime[x] = ( profstop.tv_sec*1000000+profstop.tv_usec) - \
		( profstart.tv_sec*1000000+profstart.tv_usec); \
	} while (0);
#define profreport(x) do { \
	long int conv = proftime[PROF_P2G]+proftime[PROF_G2P]; \
	long int run = proftime[PROF_GRUN]; \
	long int tot = conv + run; \
	int convpercent = (((double)conv/(double)tot)*100); \
	int runpercent = (((double)run/(double)tot)*100); \
	elog(NOTICE, "PROF_DET: npts:%lu+%lu=%lu cnv:%lu+%lu run:%lu", \
		profipts, profopts, profipts+profopts, \
		proftime[PROF_P2G], proftime[PROF_G2P], \
		proftime[PROF_GRUN]); \
	elog(NOTICE, "PROF_SUM: pts %lu cnv %d%% run %d%%", \
		profipts+profopts, \
		convpercent, \
		runpercent); \
	} while (0);
#endif

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
extern Geometry *GEOSBuffer(Geometry *g1,double width);
extern Geometry *GEOSConvexHull(Geometry *g1);
extern Geometry *GEOSDifference(Geometry *g1,Geometry *g2);
extern Geometry *GEOSBoundary(Geometry *g1);
extern Geometry *GEOSSymDifference(Geometry *g1,Geometry *g2);
extern Geometry *GEOSUnion(Geometry *g1,Geometry *g2);
extern char GEOSequals(Geometry *g1, Geometry*g2);
extern char GEOSisSimple(Geometry *g1);
extern char GEOSisRing(Geometry *g1);
extern Geometry *GEOSpointonSurface(Geometry *g1);
extern Geometry *GEOSGetCentroid(Geometry *g);

extern void GEOSdeleteChar(char *a);
extern void GEOSdeleteGeometry(Geometry *a);

extern POINT3D  *GEOSGetCoordinate(Geometry *g1);
extern POINT3D  *GEOSGetCoordinates(Geometry *g1);
extern int      GEOSGetNumCoordinate(Geometry *g1);
extern Geometry	*GEOSGetGeometryN(Geometry *g1, int n);
extern Geometry	*GEOSGetExteriorRing(Geometry *g1);
extern Geometry	*GEOSGetInteriorRingN(Geometry *g1,int n);
extern int	GEOSGetNumInteriorRings(Geometry *g1);
extern int      GEOSGetSRID(Geometry *g1);
extern int      GEOSGetNumGeometries(Geometry *g1);

extern Geometry *PostGIS2GEOS_point(const LWPOINT *point);
extern Geometry *PostGIS2GEOS_linestring(const LWLINE *line);
extern Geometry *PostGIS2GEOS_polygon(const LWPOLY *polygon);
extern Geometry *PostGIS2GEOS_multipolygon(LWPOLY *const *const geoms, uint32 ngeoms, int SRID, int is3d);
extern Geometry *PostGIS2GEOS_multilinestring(LWLINE *const *const geoms, uint32 ngeoms, int SRID, int is3d);
extern Geometry *PostGIS2GEOS_multipoint(LWPOINT *const *const geoms, uint32 ngeoms, int SRID, int is3d);
extern Geometry *PostGIS2GEOS_collection(Geometry **geoms, int ngeoms, int SRID, bool is3d);

void NOTICE_MESSAGE(char *msg);
LWGEOM *GEOS2POSTGIS(Geometry *geom, char want3d);
Geometry * POSTGIS2GEOS(LWGEOM *g);
void errorIfGeometryCollection(LWGEOM *g1, LWGEOM *g2);
char *PointFromGeometry(Geometry *g, char want3d);
char *LineFromGeometry(Geometry *g, char want3d);
char *PolyFromGeometry(Geometry *g, char want3d);
void addToExploded_recursive(Geometry *geom, LWGEOM_EXPLODED *exp);

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


/*
 * This is the final function for union/fastunite/geomunion
 * aggregate (still discussing the name). Will have
 * as input an array of Geometry pointers.
 * Will iteratively call GEOSUnion on the GEOS-converted
 * versions of them and return PGIS-converted version back.
 * Changing combination order *might* speed up performance.
 *
 * Geometries in the array are pfree'd as soon as possible.
 *
 */
PG_FUNCTION_INFO_V1(unite_garray);
Datum unite_garray(PG_FUNCTION_ARGS)
{
	Datum datum;
	ArrayType *array;
	int is3d = 0;
	int nelems, i;
	LWGEOM **geoms, *result, *pgis_geom;
	Geometry *g1, *g2, *geos_result;
#ifdef DEBUG
	static int call=1;
#endif

#ifdef DEBUG
	call++;
#endif

	datum = PG_GETARG_DATUM(0);

	/* Null array, null geometry (should be empty?) */
	if ( (Pointer *)datum == NULL ) PG_RETURN_NULL();

	array = (ArrayType *) PG_DETOAST_DATUM(datum);

	nelems = ArrayGetNItems(ARR_NDIM(array), ARR_DIMS(array));

#ifdef DEBUG
	elog(NOTICE, "unite_garray: number of elements: %d", nelems);
#endif

	if ( nelems == 0 ) PG_RETURN_NULL();

	geoms = (LWGEOM **)ARR_DATA_PTR(array);

	/* One-element union is the element itself */
	if ( nelems == 1 ) PG_RETURN_POINTER(geoms[0]);

	/* Ok, we really need geos now ;) */
	initGEOS(MAXIMUM_ALIGNOF);

	if ( lwgeom_ndims(geoms[0]->type) > 2 ) is3d = 1;
	geos_result = POSTGIS2GEOS(geoms[0]);
	pfree(geoms[0]);
	for (i=1; i<nelems; i++)
	{
		pgis_geom = geoms[i];
		
		g1 = POSTGIS2GEOS(pgis_geom);
		/*
		 * If we free this memory now we'll have
		 * more space for the growing result geometry.
		 * We don't need it anyway.
		 */
		pfree(pgis_geom);

#ifdef DEBUG
		elog(NOTICE, "unite_garray(%d): adding geom %d to union",
				call, i);
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


//select geomunion('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))','POLYGON((5 5, 15 5, 15 7, 5 7, 5 5))');
PG_FUNCTION_INFO_V1(geomunion);
Datum geomunion(PG_FUNCTION_ARGS)
{
	LWGEOM *geom1 = (LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *geom2 = (LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	int is3d = ( lwgeom_ndims(geom1->type) > 2 ) ||
		( lwgeom_ndims(geom2->type) > 2 );

	Geometry *g1,*g2,*g3;
	LWGEOM *result;

	initGEOS(MAXIMUM_ALIGNOF);
//elog(NOTICE,"in geomunion");

	g1 = POSTGIS2GEOS(geom1);
	g2 = POSTGIS2GEOS(geom2);

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

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);
	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g2);

	if (g3 == NULL)
	{
		elog(ERROR,"GEOS union() threw an error!");
		PG_RETURN_NULL(); //never get here
	}

//elog(NOTICE,"result: %s", GEOSasText(g3) ) ;

	result = GEOS2POSTGIS(g3, is3d);

	GEOSdeleteGeometry(g3);

	if (result == NULL)
	{
		elog(ERROR,"GEOS union() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); //never get here
	}

	//compressType(result);  // convert multi* to single item if appropriate
	PG_RETURN_POINTER(result);
}


// select symdifference('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))','POLYGON((5 5, 15 5, 15 7, 5 7, 5 5))');
PG_FUNCTION_INFO_V1(symdifference);
Datum symdifference(PG_FUNCTION_ARGS)
{
	LWGEOM	*geom1 = (LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM	*geom2 = (LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	Geometry *g1,*g2,*g3;
	LWGEOM *result;
	int is3d = ( lwgeom_ndims(geom1->type) > 2 ) ||
		( lwgeom_ndims(geom2->type) > 2 );

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

//	elog(NOTICE,"result: %s", GEOSasText(g3) ) ;

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = GEOS2POSTGIS(g3, is3d);
#ifdef PROFILE
	profstop(PROF_GRUN);
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

		PG_RETURN_POINTER(result);
}


PG_FUNCTION_INFO_V1(boundary);
Datum boundary(PG_FUNCTION_ARGS)
{
		LWGEOM		*geom1 = (LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

		Geometry *g1,*g3;
		LWGEOM *result;
		initGEOS(MAXIMUM_ALIGNOF);

		g1 = 	POSTGIS2GEOS(geom1 );
#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
		g3 =    GEOSBoundary(g1);
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

	result = GEOS2POSTGIS(g3, lwgeom_ndims(geom1->type) > 2);
	if (result == NULL)
	{
		GEOSdeleteGeometry(g1);

		GEOSdeleteGeometry(g3);
		elog(ERROR,"GEOS bounary() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); //never get here
	}



		GEOSdeleteGeometry(g1);
		GEOSdeleteGeometry(g3);

		//compressType(result);  // convert multi* to single item if appropriate

		PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(convexhull);
Datum convexhull(PG_FUNCTION_ARGS)
{
		LWGEOM		*geom1 = (LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		Geometry *g1,*g3;
		LWGEOM *result;

		initGEOS(MAXIMUM_ALIGNOF);

		g1 = 	POSTGIS2GEOS(geom1 );
		g3 =    GEOSConvexHull(g1);


	if (g3 == NULL)
	{
		elog(ERROR,"GEOS convexhull() threw an error!");
		GEOSdeleteGeometry(g1);
		PG_RETURN_NULL(); //never get here
	}


//	elog(NOTICE,"result: %s", GEOSasText(g3) ) ;

	result = GEOS2POSTGIS(g3, lwgeom_ndims(geom1->type) > 2);
	if (result == NULL)
	{
		GEOSdeleteGeometry(g1);
		GEOSdeleteGeometry(g3);
		elog(ERROR,"GEOS convexhull() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); //never get here
	}
		GEOSdeleteGeometry(g1);
		GEOSdeleteGeometry(g3);


		//compressType(result);  // convert multi* to single item if appropriate

		PG_RETURN_POINTER(result);

}

PG_FUNCTION_INFO_V1(buffer);
Datum buffer(PG_FUNCTION_ARGS)
{
		LWGEOM		*geom1 = (LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		double			size   = PG_GETARG_FLOAT8(1);
		Geometry *g1,*g3;
		LWGEOM *result;

		initGEOS(MAXIMUM_ALIGNOF);

		g1 = 	POSTGIS2GEOS(geom1 );
		g3 =    GEOSBuffer(g1,size);


	if (g3 == NULL)
	{
		elog(ERROR,"GEOS buffer() threw an error!");
		GEOSdeleteGeometry(g1);
		PG_RETURN_NULL(); //never get here
	}


//	elog(NOTICE,"result: %s", GEOSasText(g3) ) ;

	result = GEOS2POSTGIS(g3, lwgeom_ndims(geom1->type) > 2);
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
		PG_RETURN_POINTER(result);

}

PG_FUNCTION_INFO_V1(intersection);
Datum intersection(PG_FUNCTION_ARGS)
{
	LWGEOM *geom1 = (LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *geom2 = (LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	Geometry *g1,*g2,*g3;
	LWGEOM *result;
	int is3d = ( lwgeom_ndims(geom1->type) > 2 ) ||
		( lwgeom_ndims(geom2->type) > 2 );

	initGEOS(MAXIMUM_ALIGNOF);

//elog(NOTICE,"intersection() START");

	g1 = POSTGIS2GEOS(geom1);
	g2 = POSTGIS2GEOS(geom2);

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
	g3 =   GEOSIntersection(g1,g2);
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

	result = GEOS2POSTGIS(g3, is3d);
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

		PG_RETURN_POINTER(result);
}

//select difference('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))','POLYGON((5 5, 15 5, 15 7, 5 7, 5 5))');
PG_FUNCTION_INFO_V1(difference);
Datum difference(PG_FUNCTION_ARGS)
{
	LWGEOM *geom1 = (LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *geom2 = (LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	Geometry *g1,*g2,*g3;
	LWGEOM *result;
	int is3d = ( lwgeom_ndims(geom1->type) > 2 ) ||
		( lwgeom_ndims(geom2->type) > 2 );

	initGEOS(MAXIMUM_ALIGNOF);

	g1 = POSTGIS2GEOS(geom1);
	g2 = POSTGIS2GEOS(geom2);

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

	result = GEOS2POSTGIS(g3, is3d);
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

	PG_RETURN_POINTER(result);
}


//select pointonsurface('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))');
PG_FUNCTION_INFO_V1(pointonsurface);
Datum pointonsurface(PG_FUNCTION_ARGS)
{
	LWGEOM *geom1 = (LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	Geometry *g1,*g3;
	LWGEOM *result;

	initGEOS(MAXIMUM_ALIGNOF);

	g1 = 	POSTGIS2GEOS(geom1 );

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

	result = GEOS2POSTGIS(g3, (lwgeom_ndims(geom1->type) > 2));
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

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(centroid);
Datum centroid(PG_FUNCTION_ARGS)
{
	LWGEOM *geom, *result;
	Geometry *geosgeom, *geosresult;

	geom = (LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	initGEOS(MAXIMUM_ALIGNOF);

	geosgeom = POSTGIS2GEOS(geom);

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	geosresult = GEOSGetCentroid(geosgeom);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	if ( geosresult == NULL )
	{
		GEOSdeleteGeometry(geosgeom);
		elog(ERROR,"GEOS getCentroid() threw an error!");
		PG_RETURN_NULL(); 
	}

	result = GEOS2POSTGIS(geosresult, (lwgeom_ndims(geom->type) > 2));
	if (result == NULL)
	{
		GEOSdeleteGeometry(geosgeom);
		GEOSdeleteGeometry(geosresult);
		elog(ERROR,"Error in GEOS-PGIS conversion");
		PG_RETURN_NULL(); 
	}
	GEOSdeleteGeometry(geosgeom);
	GEOSdeleteGeometry(geosresult);

	PG_RETURN_POINTER(result);
}



//----------------------------------------------



void errorIfGeometryCollection(LWGEOM *g1, LWGEOM *g2)
{
	int t1 = lwgeom_getType(g1->type);
	int t2 = lwgeom_getType(g2->type);

	if (  (t1 == COLLECTIONTYPE) || (t2 == COLLECTIONTYPE) )
		elog(ERROR,"Relate Operation called with a LWGEOMCOLLECTION type.  This is unsupported");
}

PG_FUNCTION_INFO_V1(isvalid);
Datum isvalid(PG_FUNCTION_ARGS)
{
	LWGEOM		*geom1 = (LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	bool result;
	Geometry *g1;

	initGEOS(MAXIMUM_ALIGNOF);

	g1 = 	POSTGIS2GEOS(geom1 );

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = GEOSisvalid(g1);
#ifdef PROFILE
	profstop(PROF_GRUN);
	proftime[PROF_G2P]=0; profopts=0;
	profreport();
#endif

	GEOSdeleteGeometry(g1);
	if (result == 2)
	{
		elog(ERROR,"GEOS isvalid() threw an error!");
		PG_RETURN_NULL(); //never get here
	}


	PG_RETURN_BOOL(result);
}


// overlaps(LWGEOM g1,LWGEOM g2)
// returns  if GEOS::g1->overlaps(g2) returns true
// throws an error (elog(ERROR,...)) if GEOS throws an error
PG_FUNCTION_INFO_V1(overlaps);
Datum overlaps(PG_FUNCTION_ARGS)
{
	LWGEOM *geom1 = (LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *geom2 = (LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	Geometry *g1,*g2;
	bool result;
	const BOX2DFLOAT4 *box1, *box2;

	errorIfGeometryCollection(geom1,geom2);

	/*
	 * short-circuit 1: if geom2 bounding box does not overlap
	 * geom1 bounding box we can prematurely return FALSE.
	 * Do the test IFF BOUNDING BOX AVAILABLE.
	 */
	if ( (box1=getbox2d_internal(SERIALIZED_FORM(geom1))) &&
		(box2=getbox2d_internal(SERIALIZED_FORM(geom2))) )
	{
		if ( box2->xmax < box1->xmin ) PG_RETURN_BOOL(FALSE);
		if ( box2->xmin > box1->xmax ) PG_RETURN_BOOL(FALSE);
		if ( box2->ymax < box1->ymin ) PG_RETURN_BOOL(FALSE);
		if ( box2->ymin > box2->ymax ) PG_RETURN_BOOL(FALSE);
	}

	initGEOS(MAXIMUM_ALIGNOF);

	g1 = POSTGIS2GEOS(geom1 );
	g2 = POSTGIS2GEOS(geom2 );

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = GEOSrelateOverlaps(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
	proftime[PROF_G2P] = 0; profopts=0;
	profreport();
#endif

	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g2);
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
	LWGEOM *geom1 = (LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *geom2 = (LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	Geometry *g1,*g2;
	bool result;
	const BOX2DFLOAT4 *box1, *box2;

	errorIfGeometryCollection(geom1,geom2);

	/*
	 * short-circuit 1: if geom2 bounding box is not completely inside
	 * geom1 bounding box we can prematurely return FALSE.
	 * Do the test IFF BOUNDING BOX AVAILABLE.
	 */
	if ( (box1=getbox2d_internal(SERIALIZED_FORM(geom1))) &&
		(box2=getbox2d_internal(SERIALIZED_FORM(geom2))) )
	{
		if ( box2->xmin < box1->xmin ) PG_RETURN_BOOL(FALSE);
		if ( box2->xmax > box1->xmax ) PG_RETURN_BOOL(FALSE);
		if ( box2->ymin < box1->ymin ) PG_RETURN_BOOL(FALSE);
		if ( box2->ymax > box1->ymax ) PG_RETURN_BOOL(FALSE);
	}

	initGEOS(MAXIMUM_ALIGNOF);

	g1 = 	POSTGIS2GEOS(geom1 );
	g2 = 	POSTGIS2GEOS(geom2 );

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = GEOSrelateContains(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
	proftime[PROF_G2P] = 0; profopts=0;
	profreport();
#endif

	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g2);

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
	LWGEOM *geom1 = (LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *geom2 = (LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	Geometry *g1,*g2;
	bool result;
	const BOX2DFLOAT4 *box1, *box2;

	errorIfGeometryCollection(geom1,geom2);

	/*
	 * short-circuit 1: if geom1 bounding box is not completely inside
	 * geom2 bounding box we can prematurely return FALSE.
	 * Do the test IFF BOUNDING BOX AVAILABLE.
	 */
	if ( (box1=getbox2d_internal(SERIALIZED_FORM(geom1))) &&
		(box2=getbox2d_internal(SERIALIZED_FORM(geom2))) )
	{
		if ( box1->xmin < box2->xmin ) PG_RETURN_BOOL(FALSE);
		if ( box1->xmax > box2->xmax ) PG_RETURN_BOOL(FALSE);
		if ( box1->ymin < box2->ymin ) PG_RETURN_BOOL(FALSE);
		if ( box1->ymax > box2->ymax ) PG_RETURN_BOOL(FALSE);
	}

	initGEOS(MAXIMUM_ALIGNOF);

	g1 = 	POSTGIS2GEOS(geom1 );
	g2 = 	POSTGIS2GEOS(geom2 );

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = GEOSrelateWithin(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
	proftime[PROF_G2P] = 0; profopts=0;
	profreport();
#endif

	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g2);

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
	LWGEOM *geom1 = (LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *geom2 = (LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	Geometry *g1,*g2;
	bool result;
	const BOX2DFLOAT4 *box1, *box2;

	errorIfGeometryCollection(geom1,geom2);

	/*
	 * short-circuit 1: if geom2 bounding box does not overlap
	 * geom1 bounding box we can prematurely return FALSE.
	 * Do the test IFF BOUNDING BOX AVAILABLE.
	 */
	if ( (box1=getbox2d_internal(SERIALIZED_FORM(geom1))) &&
		(box2=getbox2d_internal(SERIALIZED_FORM(geom2))) )
	{
		if ( box2->xmax < box1->xmin ) PG_RETURN_BOOL(FALSE);
		if ( box2->xmin > box1->xmax ) PG_RETURN_BOOL(FALSE);
		if ( box2->ymax < box1->ymin ) PG_RETURN_BOOL(FALSE);
		if ( box2->ymin > box2->ymax ) PG_RETURN_BOOL(FALSE);
	}

	initGEOS(MAXIMUM_ALIGNOF);

	g1 = 	POSTGIS2GEOS(geom1 );
	g2 = 	POSTGIS2GEOS(geom2 );

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = GEOSrelateCrosses(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
	proftime[PROF_G2P] = 0; profopts=0;
	profreport();
#endif

	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g2);

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
	LWGEOM *geom1 = (LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *geom2 = (LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	Geometry *g1,*g2;
	bool result;
	const BOX2DFLOAT4 *box1, *box2;

	errorIfGeometryCollection(geom1,geom2);

	/*
	 * short-circuit 1: if geom2 bounding box does not overlap
	 * geom1 bounding box we can prematurely return FALSE.
	 * Do the test IFF BOUNDING BOX AVAILABLE.
	 */
	if ( (box1=getbox2d_internal(SERIALIZED_FORM(geom1))) &&
		(box2=getbox2d_internal(SERIALIZED_FORM(geom2))) )
	{
		if ( box2->xmax < box1->xmin ) PG_RETURN_BOOL(FALSE);
		if ( box2->xmin > box1->xmax ) PG_RETURN_BOOL(FALSE);
		if ( box2->ymax < box1->ymin ) PG_RETURN_BOOL(FALSE);
		if ( box2->ymin > box2->ymax ) PG_RETURN_BOOL(FALSE);
	}

	initGEOS(MAXIMUM_ALIGNOF);

	g1 = 	POSTGIS2GEOS(geom1 );
	g2 = 	POSTGIS2GEOS(geom2 );

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = GEOSrelateIntersects(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
	proftime[PROF_G2P] = 0; profopts=0;
	profreport();
#endif
	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g2);
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
	LWGEOM *geom1 = (LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *geom2 = (LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	Geometry *g1,*g2;
	bool result;
	const BOX2DFLOAT4 *box1, *box2;

	errorIfGeometryCollection(geom1,geom2);

	/*
	 * short-circuit 1: if geom2 bounding box does not overlap
	 * geom1 bounding box we can prematurely return FALSE.
	 * Do the test IFF BOUNDING BOX AVAILABLE.
	 */
	if ( (box1=getbox2d_internal(SERIALIZED_FORM(geom1))) &&
		(box2=getbox2d_internal(SERIALIZED_FORM(geom2))) )
	{
		if ( box2->xmax < box1->xmin ) PG_RETURN_BOOL(FALSE);
		if ( box2->xmin > box1->xmax ) PG_RETURN_BOOL(FALSE);
		if ( box2->ymax < box1->ymin ) PG_RETURN_BOOL(FALSE);
		if ( box2->ymin > box2->ymax ) PG_RETURN_BOOL(FALSE);
	}

	initGEOS(MAXIMUM_ALIGNOF);

	g1 = POSTGIS2GEOS(geom1 );
	g2 = POSTGIS2GEOS(geom2 );

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = GEOSrelateTouches(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
	proftime[PROF_G2P] = 0; profopts=0;
	profreport();
#endif

	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g2);

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
	LWGEOM *geom1 = (LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *geom2 = (LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	Geometry *g1,*g2;
	bool result;
	const BOX2DFLOAT4 *box1, *box2;

	errorIfGeometryCollection(geom1,geom2);

	/*
	 * short-circuit 1: if geom2 bounding box does not overlap
	 * geom1 bounding box we can prematurely return TRUE.
	 * Do the test IFF BOUNDING BOX AVAILABLE.
	 */
	if ( (box1=getbox2d_internal(SERIALIZED_FORM(geom1))) &&
		(box2=getbox2d_internal(SERIALIZED_FORM(geom2))) )
	{
		if ( box2->xmax < box1->xmin ) PG_RETURN_BOOL(TRUE);
		if ( box2->xmin > box1->xmax ) PG_RETURN_BOOL(TRUE);
		if ( box2->ymax < box1->ymin ) PG_RETURN_BOOL(TRUE);
		if ( box2->ymin > box2->ymax ) PG_RETURN_BOOL(TRUE);
	}

	initGEOS(MAXIMUM_ALIGNOF);

	g1 = POSTGIS2GEOS(geom1 );
	g2 = POSTGIS2GEOS(geom2 );

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = GEOSrelateDisjoint(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
	proftime[PROF_G2P] = 0; profopts=0;
	profreport();
#endif
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
	LWGEOM *geom1 = (LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *geom2 = (LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	char *patt;
	bool result;

	Geometry *g1,*g2;

	errorIfGeometryCollection(geom1,geom2);
	initGEOS(MAXIMUM_ALIGNOF);

	g1 = POSTGIS2GEOS(geom1);
	g2 = POSTGIS2GEOS(geom2);

	patt =  DatumGetCString(DirectFunctionCall1(textout,
                        PointerGetDatum(PG_GETARG_DATUM(2))));

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = GEOSrelatePattern(g1,g2,patt);
#ifdef PROFILE
	profstop(PROF_GRUN);
	proftime[PROF_G2P] = 0; profopts=0;
	profreport();
#endif
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
	LWGEOM		*geom1 = (LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM		*geom2 = (LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

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


#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
		relate_str = GEOSrelate(g1, g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
	proftime[PROF_G2P] = 0; profopts=0;
	profreport();
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


	len = strlen(relate_str) + 4;

	result= palloc(len);
	*((int *) result) = len;

	memcpy(result +4, relate_str, len-4);

	free(relate_str);


	PG_RETURN_POINTER(result);
}

//==============================

PG_FUNCTION_INFO_V1(geomequals);
Datum geomequals(PG_FUNCTION_ARGS)
{
	LWGEOM *geom1 = (LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *geom2 = (LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	Geometry *g1,*g2;
	bool result;
	const BOX2DFLOAT4 *box1, *box2;

	errorIfGeometryCollection(geom1,geom2);

	/*
	 * short-circuit 1: if geom2 bounding box does not equal
	 * geom1 bounding box we can prematurely return FALSE.
	 * Do the test IFF BOUNDING BOX AVAILABLE.
	 */
	if ( (box1=getbox2d_internal(SERIALIZED_FORM(geom1))) &&
		(box2=getbox2d_internal(SERIALIZED_FORM(geom2))) )
	{
		if ( box2->xmax != box1->xmax ) PG_RETURN_BOOL(FALSE);
		if ( box2->xmin != box1->xmin ) PG_RETURN_BOOL(FALSE);
		if ( box2->ymax != box1->ymax ) PG_RETURN_BOOL(FALSE);
		if ( box2->ymin != box2->ymin ) PG_RETURN_BOOL(FALSE);
	}

	initGEOS(MAXIMUM_ALIGNOF);

	g1 = POSTGIS2GEOS(geom1 );
	g2 = POSTGIS2GEOS(geom2 );

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = GEOSequals(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
	proftime[PROF_G2P] = 0; profopts=0;
	profreport();
#endif
	GEOSdeleteGeometry(g1);
	GEOSdeleteGeometry(g2);

	if (result == 2)
	{
		elog(ERROR,"GEOS equals() threw an error!");
		PG_RETURN_NULL(); //never get here
	}

	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(issimple);
Datum issimple(PG_FUNCTION_ARGS)
{
	LWGEOM *geom = (LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	Geometry *g1;
	int result;

	if (lwgeom_getnumgeometries(SERIALIZED_FORM(geom)) == 0)
		PG_RETURN_BOOL(true);

	initGEOS(MAXIMUM_ALIGNOF);

	//elog(NOTICE,"GEOS init()");

	g1 = POSTGIS2GEOS(geom );

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = GEOSisSimple(g1);
#ifdef PROFILE
	profstop(PROF_GRUN);
	proftime[PROF_G2P] = 0; profopts=0;
	profreport();
#endif
	GEOSdeleteGeometry(g1);

	if (result == 2)
	{
		elog(ERROR,"GEOS issimple() threw an error!");
		PG_RETURN_NULL(); //never get here
	}

	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(isring);
Datum isring(PG_FUNCTION_ARGS)
{
	LWGEOM	*geom = (LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	Geometry *g1;
	int result;

	if (lwgeom_getType(geom->type) != LINETYPE)
	{
		elog(ERROR,"isring() should only be called on a LINE");
	}

	if (lwgeom_getnumgeometries(SERIALIZED_FORM(geom)) == 0)
		PG_RETURN_BOOL(false);

	initGEOS(MAXIMUM_ALIGNOF);

	//elog(NOTICE,"GEOS init()");

	g1 = POSTGIS2GEOS(geom );

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = GEOSisRing(g1);
#ifdef PROFILE
	profstop(PROF_GRUN);
	proftime[PROF_G2P] = 0; profopts = 0;
	profreport();
#endif
	GEOSdeleteGeometry(g1);

	if (result == 2)
	{
		elog(ERROR,"GEOS isring() threw an error!");
		PG_RETURN_NULL(); //never get here
	}

	PG_RETURN_BOOL(result);
}



//= GEOS <=> POSTGIS CONVERSION =========================

// Return a serialized line from a GEOS geometry (no SRID, no BBOX)
char *PointFromGeometry(Geometry *g, char want3d)
{
	POINTARRAY *pa;
	LWPOINT *point;
	char *srl;
	POINT3D *pts;
	int ptsize = want3d ? sizeof(POINT3D) : sizeof(POINT2D);

	// Construct point array
	pa = (POINTARRAY *)palloc(sizeof(POINTARRAY));
	pa->ndims = want3d ? 3 : 2;
	pa->npoints = 1;

	// Fill point array
	pa->serialized_pointlist = palloc(ptsize);
	pts = GEOSGetCoordinates(g);
	memcpy(pa->serialized_pointlist, pts, ptsize);
	GEOSdeleteChar( (char*) pts);

	// Construct LWPOINT
	point = lwpoint_construct(pa->ndims, -1, pa);

	// Serialize LWPOINT
	srl = lwpoint_serialize(point);

	return srl;
}

// Return a serialized line from a GEOS geometry (no SRID no BBOX)
char *LineFromGeometry(Geometry *g, char want3d)
{
	POINTARRAY *pa;
	LWLINE *line;
	int npoints;
	char *srl;
	POINT3D *pts, *ip, *op;
	int ptsize = want3d ? sizeof(POINT3D) : sizeof(POINT2D);
	int i;

	npoints = GEOSGetNumCoordinate(g);
	if (npoints <2) return NULL;

	// Construct point array
	pa = (POINTARRAY *)palloc(sizeof(POINTARRAY));
	pa->ndims = want3d ? 3 : 2;
	pa->npoints = npoints;

	// Fill point array
	pa->serialized_pointlist = palloc(ptsize*npoints);
	pts = GEOSGetCoordinates(g);
	for (i=0; i<npoints; i++)
	{
		ip = &(pts[i]);
		op = (POINT3D *)getPoint(pa, i);
		memcpy(op, ip, ptsize);
	}
	GEOSdeleteChar( (char*) pts);

	// Construct LWPOINT
	line = lwline_construct(pa->ndims, -1, pa);

	// Serialize LWPOINT
	srl = lwline_serialize(line);

	return srl;
}

// Return a serialized polygon from a GEOS geometry (no SRID no BBOX)
char *PolyFromGeometry(Geometry *g, char want3d)
{
	POINTARRAY **rings, *pa;
	LWPOLY *poly;
	int ndims = want3d ? 3 : 2;
	int nrings;
	int npoints;
	int i, j;
	char *srl;
	POINT3D *pts, *ip, *op;
	int ptoff=0; // point offset inside POINT3D *
	int ptsize = 8*ndims;

	// Get number of rings, and pointlist
	pts = GEOSGetCoordinates(g);
	nrings = GEOSGetNumInteriorRings(g); 
	rings = (POINTARRAY **)palloc(sizeof(POINTARRAY *)*(nrings+1));

	// Exterior ring
	npoints = GEOSGetNumCoordinate(GEOSGetExteriorRing(g));
	rings[0] = (POINTARRAY *)palloc(sizeof(POINTARRAY));
	pa = rings[0];
	pa->ndims = ndims;
	pa->npoints = npoints;

	// Fill point array
	pa->serialized_pointlist = palloc(ptsize*npoints);
	for (i=0; i<npoints; i++)
	{
		ip = &(pts[i+ptoff]);
		op = (POINT3D *)getPoint(pa, i);
		memcpy(op, ip, ptsize);
	}
	ptoff+=npoints;

	// Interior rings
	for (j=0; j<nrings; j++)
	{
		npoints = GEOSGetNumCoordinate(GEOSGetInteriorRingN(g, j));
		rings[j+1] = (POINTARRAY *)palloc(sizeof(POINTARRAY));
		pa = rings[j+1];
		pa->ndims = ndims;
		pa->npoints = npoints;

		// Fill point array
		pa->serialized_pointlist = palloc(ptsize*npoints);
		for (i=0; i<npoints; i++)
		{
			ip = &(pts[i+ptoff]);
			op = (POINT3D *)getPoint(pa, i);
			memcpy(op, ip, ptsize);
		}
		ptoff+=npoints;
	}

	// Construct LWPOLY
	poly = lwpoly_construct(pa->ndims, -1, nrings+1, rings);

	// Serialize LWPOLY
	srl = lwpoly_serialize(poly);

	// Get rid of GEOS points...
	GEOSdeleteChar( (char*) pts);

	return srl;
}

//-----=GEOS2POSTGIS=

/*
 * Recursively add a Geometry to the LWGEOM_EXPLODED structure
 * The exploded struct contains note about the number of dimensions
 * requested.
 */
void
addToExploded_recursive(Geometry *geom, LWGEOM_EXPLODED *exp)
{
	char *srl;
	int type = GEOSGeometryTypeId(geom) ;
	char want3d = exp->ndims > 2 ? 1 : 0;
	int ngeoms;
	int t;


	switch (type)
	{
		/* From slower to faster.. compensation rule :) */

		case COLLECTIONTYPE:
		case MULTIPOLYGONTYPE:
		case MULTILINETYPE:
		case MULTIPOINTTYPE:

			//this is more difficult because GEOS allows GCs of GCs
			ngeoms = GEOSGetNumGeometries(geom);
#ifdef DEBUG_GEOS2POSTGIS
	elog(NOTICE, "GEOS2POSTGIS: It's a MULTI (of %d elems)", ngeoms);
#endif
			if (ngeoms == 0)
			{
				return; // skip EMPTY geoms
			}
			if (ngeoms == 1) {
				Geometry *g = GEOSGetGeometryN(geom, 0);
				// short cut!
				return addToExploded_recursive(g, exp);
			}
			for (t=0; t<ngeoms; t++) {
				Geometry *g = GEOSGetGeometryN(geom, t);
				addToExploded_recursive(g, exp);
			}
			return;

		case POLYGONTYPE:
#ifdef DEBUG_GEOS2POSTGIS
			elog(NOTICE, "GEOS2POSTGIS: It's a POLYGON");
#endif
			srl = PolyFromGeometry(geom, want3d);
			if (srl == NULL) return;
			exp->npolys++;
			exp->polys = repalloc(exp->polys,
				sizeof(char *)*exp->npolys);
			exp->polys[exp->npolys-1] = srl;
			return;

		case LINETYPE:
#ifdef DEBUG_GEOS2POSTGIS
			elog(NOTICE, "GEOS2POSTGIS: It's a LINE");
#endif
			srl = LineFromGeometry(geom, want3d);
			if (srl == NULL) return;
			exp->nlines++;
			exp->lines = repalloc(exp->lines,
				sizeof(char *)*exp->nlines);
			exp->lines[exp->nlines-1] = srl;
			return;

		case POINTTYPE: 
#ifdef DEBUG_GEOS2POSTGIS
			elog(NOTICE, "GEOS2POSTGIS: It's a POINT");
#endif
			srl = PointFromGeometry(geom, want3d);
			if (srl == NULL) return;
			exp->npoints++;
			exp->points = repalloc(exp->points,
				sizeof(char *)*exp->npoints);
			exp->points[exp->npoints-1] = srl;
			return;

		default:
#ifdef DEBUG_GEOS2POSTGIS
			elog(NOTICE, "GEOS2POSTGIS: It's UNKNOWN!");
#endif
			return;

	}

}

LWGEOM *
GEOS2POSTGIS(Geometry *geom, char want3d)
{
	LWGEOM *result;
	LWGEOM_EXPLODED *oexp;
	int SRID = GEOSGetSRID(geom);
	int wantbbox = 0; // might as well be 1 ...
	int size;

#ifdef PROFILE
	profstart(PROF_G2P);
#endif

	// Initialize exploded lwgeom
	oexp = (LWGEOM_EXPLODED *)palloc(sizeof(LWGEOM_EXPLODED));
	oexp->SRID = SRID;
	oexp->ndims = want3d ? 3 : 2;
	oexp->npoints = 0;
	oexp->points = palloc(sizeof(char *));
	oexp->nlines = 0;
	oexp->lines = palloc(sizeof(char *));
	oexp->npolys = 0;
	oexp->polys = palloc(sizeof(char *));

	addToExploded_recursive(geom, oexp);

	size = lwexploded_findlength(oexp, wantbbox);
	size += 4; // postgresql size header
	result = palloc(size);
	result->size = size;
	lwexploded_serialize_buf(oexp, wantbbox, SERIALIZED_FORM(result), NULL);

#ifdef PROFILE
	profstop(PROF_G2P);
	profopts = lwgeom_npoints_recursive(SERIALIZED_FORM(result));
	profreport();
#endif

	return result;
}

//-----=POSTGIS2GEOS=

Geometry *
POSTGIS2GEOS(LWGEOM *geom)
{
	LWPOINT *point;
	LWLINE *line;
	LWPOLY *poly;
	LWGEOM_EXPLODED *exploded;
	int type;
	int i, j;
	Geometry **collected;
	int ncollected;
	Geometry *ret=NULL;

#ifdef PROFILE
	profipts = lwgeom_npoints_recursive(SERIALIZED_FORM(geom));
	profstart(PROF_P2G);
#endif

	type = lwgeom_getType(geom->type);

	switch (type)
	{
		case POINTTYPE:
#ifdef DEBUG_POSTGIS2GEOS
			elog(NOTICE, "POSTGIS2GEOS: it's a point");
#endif
			point = lwpoint_deserialize(SERIALIZED_FORM(geom));
			ret = PostGIS2GEOS_point(point);
			break;

		case LINETYPE:
#ifdef DEBUG_POSTGIS2GEOS
			elog(NOTICE, "POSTGIS2GEOS: it's a line");
#endif
			line = lwline_deserialize(SERIALIZED_FORM(geom));
			ret = PostGIS2GEOS_linestring(line);
			break;

		case POLYGONTYPE:
#ifdef DEBUG_POSTGIS2GEOS
			elog(NOTICE, "POSTGIS2GEOS: it's a polygon");
#endif
			poly = lwpoly_deserialize(SERIALIZED_FORM(geom));
			ret = PostGIS2GEOS_polygon(poly);
			break;

		default:
			break;
	}

	if ( ret )
	{
#ifdef PROFILE
		profstop(PROF_P2G);
#endif
		return ret;
	}

#ifdef DEBUG_POSTGIS2GEOS
	elog(NOTICE, "POSTGIS2GEOS: out of switch");
#endif

	// Since it is not a base type, let's explode...
	exploded = lwgeom_explode(SERIALIZED_FORM(geom));

#ifdef DEBUG_POSTGIS2GEOS
	elog(NOTICE, "POSTGIS2GEOS: exploded produced");
#endif

	ncollected = exploded->npoints + exploded->nlines + exploded->npolys;

#ifdef DEBUG_POSTGIS2GEOS
	elog(NOTICE, "POSTGIS2GEOS: it's a collection of %d elems", ncollected);
#endif

	collected = (Geometry **)palloc(sizeof(Geometry *)*ncollected);

	j=0;
	for (i=0; i<exploded->npoints; i++)
	{
		point = lwpoint_deserialize(exploded->points[i]);
		collected[j++] = PostGIS2GEOS_point(point);
	}
	for (i=0; i<exploded->nlines; i++)
	{
		line = lwline_deserialize(exploded->lines[j]);
		collected[j++] = PostGIS2GEOS_linestring(line);
	}
	for (i=0; i<exploded->npolys; i++)
	{
		poly = lwpoly_deserialize(exploded->polys[i]);
		collected[j++] = PostGIS2GEOS_polygon(poly);
	}

	ret = PostGIS2GEOS_collection(collected, ncollected,
		exploded->SRID, exploded->ndims > 2);

#ifdef PROFILE
	profstop(PROF_P2G);
#endif

	return ret;

}

PG_FUNCTION_INFO_V1(GEOSnoop);
Datum GEOSnoop(PG_FUNCTION_ARGS)
{
	LWGEOM *geom;
	Geometry *geosgeom;
	LWGEOM *result;

	initGEOS(MAXIMUM_ALIGNOF);

	geom = (LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
#ifdef DEBUG_CONVERTER
	elog(NOTICE, "GEOSnoop: IN: %s", unparse_WKT((char *)geom, malloc, free));
#endif

	geosgeom = POSTGIS2GEOS(geom);
	//if ( (Pointer *)geom != (Pointer *)PG_GETARG_DATUM(0) ) pfree(geom);
	if ( ! geosgeom ) PG_RETURN_NULL();

#ifdef PROFILE
	profstart(PROF_GRUN);
	profstop(PROF_GRUN);
#endif

	result = GEOS2POSTGIS(geosgeom, lwgeom_ndims(geom->type) > 2);
	GEOSdeleteGeometry(geosgeom);

#ifdef DEBUG_CONVERTER
	elog(NOTICE, "GEOSnoop: OUT: %s", unparse_WKT((char *)result, malloc, free));
#endif

	PG_RETURN_POINTER(result);
}

#else // ! USE_GEOS

PG_FUNCTION_INFO_V1(postgis_geos_version);
Datum postgis_geos_version(PG_FUNCTION_ARGS)
{
	//elog(ERROR,"GEOSversion:: operation not implemented - compile PostGIS with GEOS support");
	PG_RETURN_NULL();
}

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

PG_FUNCTION_INFO_V1(issimple);
Datum issimple(PG_FUNCTION_ARGS)
{
	elog(ERROR,"issimple:: operation not implemented - compile PostGIS with GEOS support");
	PG_RETURN_NULL(); // never get here
}
PG_FUNCTION_INFO_V1(geomequals);
Datum geomequals(PG_FUNCTION_ARGS)
{
	elog(ERROR,"geomequals:: operation not implemented - compile PostGIS with GEOS support");
	PG_RETURN_NULL(); // never get here
}
PG_FUNCTION_INFO_V1(isring);
Datum isring(PG_FUNCTION_ARGS)
{
	elog(ERROR,"isring:: operation not implemented - compile PostGIS with GEOS support");
	PG_RETURN_NULL(); // never get here
}

PG_FUNCTION_INFO_V1(pointonsurface);
Datum pointonsurface(PG_FUNCTION_ARGS)
{
	elog(ERROR,"pointonsurface:: operation not implemented - compile PostGIS with GEOS support");
	PG_RETURN_NULL(); // never get here
}

PG_FUNCTION_INFO_V1(unite_garray);
Datum unite_garray(PG_FUNCTION_ARGS)
{
	elog(ERROR,"unite_garray:: operation not implemented - compile PostGIS with GEOS support");
	PG_RETURN_NULL(); // never get here
}

#endif // ! USE_GEOS
