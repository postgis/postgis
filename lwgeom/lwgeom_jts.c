
#include "postgres.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "fmgr.h"

#include "lwgeom_pg.h"
#include "liblwgeom.h"
#include "profile.h"
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
Datum JTSnoop(PG_FUNCTION_ARGS);
Datum postgis_geos_version(PG_FUNCTION_ARGS);
Datum centroid(PG_FUNCTION_ARGS);
Datum JTS_polygonize_garray(PG_FUNCTION_ARGS);



/*
 * Define this to have have many notices printed
 * during postgis->geos and geos->postgis conversions
 */
//#define PGIS_DEBUG_CONVERTER 1
#ifdef PGIS_DEBUG_CONVERTER
#define PGIS_DEBUG_POSTGIS2JTS 1
#define PGIS_DEBUG_JTS2POSTGIS 1
#endif // PGIS_DEBUG_CONVERTER
//#define PGIS_DEBUG 1

/*
 * If you're having problems with JTS<->POSTGIS conversions
 * you can define these to use WKT
 */
#define WKT_J2P 0
#define WKT_P2J 0

typedef void (*noticefunc)(const char *fmt, ...);
typedef  struct JTSGeometry JTSGeometry;

extern const char * createJTSPoint(POINT3D *pt);
extern void initJTS(noticefunc);
extern void finishJTS(void);
extern char *JTSrelate(JTSGeometry *g1, JTSGeometry*g2);
extern char JTSrelatePattern(JTSGeometry *g1, JTSGeometry*g2,char *pat);
extern char JTSrelateDisjoint(JTSGeometry *g1, JTSGeometry*g2);
extern char JTSrelateTouches(JTSGeometry *g1, JTSGeometry*g2);
extern char JTSrelateIntersects(JTSGeometry *g1, JTSGeometry*g2);
extern char JTSrelateCrosses(JTSGeometry *g1, JTSGeometry*g2);
extern char JTSrelateWithin(JTSGeometry *g1, JTSGeometry*g2);
extern char JTSrelateContains(JTSGeometry *g1, JTSGeometry*g2);
extern char JTSrelateOverlaps(JTSGeometry *g1, JTSGeometry*g2);
extern char *JTSasText(JTSGeometry *g1);
extern char JTSisEmpty(JTSGeometry *g1);
extern char *JTSGeometryType(JTSGeometry *g1);
extern int JTSGeometryTypeId(JTSGeometry *g1);
extern char *JTSversion();
extern char *JTSjtsport();
extern char JTSisvalid(JTSGeometry *g1);
extern JTSGeometry *JTSIntersection(JTSGeometry *g1, JTSGeometry *g2);
extern JTSGeometry *JTSBuffer(JTSGeometry *g1,double width, int quadsegs);
extern JTSGeometry *JTSConvexHull(JTSGeometry *g1);
extern JTSGeometry *JTSDifference(JTSGeometry *g1,JTSGeometry *g2);
extern JTSGeometry *JTSBoundary(JTSGeometry *g1);
extern JTSGeometry *JTSSymDifference(JTSGeometry *g1,JTSGeometry *g2);
extern JTSGeometry *JTSUnion(JTSGeometry *g1,JTSGeometry *g2);
extern char JTSequals(JTSGeometry *g1, JTSGeometry*g2);
extern char JTSisSimple(JTSGeometry *g1);
extern char JTSisRing(JTSGeometry *g1);
extern JTSGeometry *JTSpointonSurface(JTSGeometry *g1);
extern JTSGeometry *JTSGetCentroid(JTSGeometry *g, int *failure);
extern bool JTSHasZ(JTSGeometry *g1);

extern void JTSSetSRID(JTSGeometry *g, int SRID);
extern void JTSdeleteChar(char *a);

extern POINT3D  *JTSGetCoordinate(JTSGeometry *g1);
extern POINT3D  *JTSGetCoordinates(JTSGeometry *g1);
extern int      JTSGetNumCoordinate(JTSGeometry *g1);
extern JTSGeometry	*JTSGetGeometryN(JTSGeometry *g1, int n);
extern JTSGeometry	*JTSGetExteriorRing(JTSGeometry *g1);
extern JTSGeometry	*JTSGetInteriorRingN(JTSGeometry *g1,int n);
extern JTSGeometry *JTSpolygonize(JTSGeometry **geoms, unsigned int ngeoms);
extern int	JTSGetNumInteriorRings(JTSGeometry *g1);
extern int      JTSGetSRID(JTSGeometry *g1);
extern int      JTSGetNumGeometries(JTSGeometry *g1);

extern JTSGeometry *PostGIS2JTS_point(const LWPOINT *point);
extern JTSGeometry *PostGIS2JTS_linestring(const LWLINE *line);
extern JTSGeometry *PostGIS2JTS_polygon(const LWPOLY *polygon);
extern JTSGeometry *PostGIS2JTS_multipolygon(LWPOLY **geoms, uint32 ngeoms, int SRID, int is3d);
extern JTSGeometry *PostGIS2JTS_multilinestring(LWLINE **geoms, uint32 ngeoms, int SRID, int is3d);
extern JTSGeometry *PostGIS2JTS_multipoint(LWPOINT **geoms, uint32 ngeoms, int SRID, int is3d);
extern JTSGeometry *PostGIS2JTS_collection(int type, JTSGeometry **geoms, int ngeoms, int SRID, bool is3d);
extern JTSGeometry *JTSGeometryFromWKT(const char *wkt);

PG_LWGEOM *JTS2POSTGIS(JTSGeometry *geom, char want3d);
JTSGeometry * POSTGIS2JTS(PG_LWGEOM *g);
JTSGeometry * LWGEOM2JTS(LWGEOM *g);
void errorIfJTSGeometryCollection(PG_LWGEOM *g1, PG_LWGEOM *g2);
LWPOINT *lwpoint_from_geometry(JTSGeometry *g, char want3d);
LWLINE *lwline_from_geometry(JTSGeometry *g, char want3d);
LWPOLY *lwpoly_from_geometry(JTSGeometry *g, char want3d);
LWCOLLECTION *lwcollection_from_geometry(JTSGeometry *geom, char want3d);
LWGEOM *JTS2LWGEOM(JTSGeometry *g, char want3d);


/*
 * This is the final function for union/fastunite/geomunion
 * aggregate (still discussing the name). Will have
 * as input an array of JTSGeometry objects.
 * Will iteratively call JTSUnion on the JTS-converted
 * versions of them and return PGIS-converted version back.
 * Changing combination order *might* speed up performance.
 *
 */
PG_FUNCTION_INFO_V1(unite_garray);
Datum unite_garray(PG_FUNCTION_ARGS)
{
	Datum datum;
	ArrayType *array;
	int is3d = 0;
	int nelems, i;
	PG_LWGEOM *result, *pgis_geom;
	JTSGeometry *g1, *g2, *geos_result=NULL;
	int SRID=-1;
	size_t offset;
#ifdef PGIS_DEBUG
	static int call=1;
#endif

#ifdef PGIS_DEBUG
	call++;
#endif

	//lwnotice("unite_garray (jts) invoked");

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
	initJTS(lwnotice);

	offset = 0;
	for (i=0; i<nelems; i++)
	{
		PG_LWGEOM *geom = (PG_LWGEOM *)(ARR_DATA_PTR(array)+offset);
		offset += INTALIGN(geom->size);

		//lwnotice("unite_garray starting iteration %d of %d", i, nelems);

		pgis_geom = geom;

#ifdef PGIS_DEBUG
		lwnotice("geom %d @ %p", i, geom);
#endif

		// Check is3d flag
		if ( TYPE_NDIMS(geom->type) > 2 ) is3d = 1;

		// Check SRID homogeneity and initialize geos result
		if ( ! i )
		{
			geos_result = POSTGIS2JTS(geom);
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
				finishJTS();
				lwerror("Operation on mixed SRID geometries");
				PG_RETURN_NULL();
			}
		}
		
		g1 = POSTGIS2JTS(pgis_geom);
		if ( g1 == NULL )
		{
			finishJTS();
			lwerror("unite_garray: error converting geometry %d to JTS", i);
		}

#ifdef PGIS_DEBUG
		lwnotice( "unite_garray(%d): adding geom %d to union (%s)",
				call, i, lwgeom_typename(TYPE_GETTYPE(geom->type)));
#endif

		g2 = JTSUnion(g1,geos_result);
		if ( g2 == NULL )
		{
			finishJTS();
			lwerror("JTS union() threw an error!");
			PG_RETURN_NULL();
		}

		//finishJTS();
		geos_result = g2;
		g2 = NULL; // FOR GC
	}

	lwnotice("unite_garray finished all elements (%d)", nelems);

	JTSSetSRID(geos_result, SRID);
	result = JTS2POSTGIS(geos_result, is3d);
	if ( result == NULL ) {
		finishJTS();
		lwerror("JTS2POSTGIS returned an error");
		PG_RETURN_NULL();
	}

	finishJTS();
	PG_RETURN_POINTER(result);

}


//select geomunion('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))','POLYGON((5 5, 15 5, 15 7, 5 7, 5 5))');
PG_FUNCTION_INFO_V1(geomunion);
Datum geomunion(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	int is3d;
	int SRID;
	JTSGeometry *g1,*g2,*g3;
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

	initJTS(lwnotice);
//elog(NOTICE,"in geomunion");

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2JTS(geom1);
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif
#ifdef PROFILE
	profstart(PROF_P2G2);
#endif
	g2 = POSTGIS2JTS(geom2);
#ifdef PROFILE
	profstop(PROF_P2G2);
#endif

//elog(NOTICE,"g1=%s",JTSasText(g1));
//elog(NOTICE,"g2=%s",JTSasText(g2));
#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	g3 = JTSUnion(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

//elog(NOTICE,"g3=%s",JTSasText(g3));

	if (g3 == NULL)
	{
		finishJTS();
		lwerror("JTS union() threw an error!");
		PG_RETURN_NULL(); //never get here
	}


	JTSSetSRID(g3, SRID);

//elog(NOTICE,"result: %s", JTSasText(g3) ) ;

#ifdef PROFILE
	profstart(PROF_G2P);
#endif
	result = JTS2POSTGIS(g3, is3d);
#ifdef PROFILE
	profstop(PROF_G2P);
#endif

	if (result == NULL)
	{
		elog(ERROR,"JTS union() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); //never get here
	}

#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("geos",geom1, geom2, result);
#endif

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);
	finishJTS();

	PG_RETURN_POINTER(result);
}


// select symdifference('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))','POLYGON((5 5, 15 5, 15 7, 5 7, 5 5))');
PG_FUNCTION_INFO_V1(symdifference);
Datum symdifference(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	JTSGeometry *g1,*g2,*g3;
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

	initJTS(lwnotice);

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2JTS(geom1);
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif
#ifdef PROFILE
	profstart(PROF_P2G2);
#endif
	g2 = POSTGIS2JTS(geom2);
#ifdef PROFILE
	profstop(PROF_P2G2);
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	g3 = JTSSymDifference(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	if (g3 == NULL)
	{
		finishJTS();
		lwerror("JTS symdifference() threw an error!");
		PG_RETURN_NULL(); //never get here
	}

//	elog(NOTICE,"result: %s", JTSasText(g3) ) ;

	JTSSetSRID(g3, SRID);

#ifdef PROFILE
	profstart(PROF_G2P);
#endif
	result = JTS2POSTGIS(g3, is3d);
#ifdef PROFILE
	profstop(PROF_G2P);
#endif

	if (result == NULL)
	{
		finishJTS();
		lwerror("JTS symdifference() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); //never get here
	}

#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("geos",geom1, geom2, result);
#endif

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);
	finishJTS();

	PG_RETURN_POINTER(result);
}


PG_FUNCTION_INFO_V1(boundary);
Datum boundary(PG_FUNCTION_ARGS)
{
	PG_LWGEOM	*geom1;
	JTSGeometry *g1,*g3;
	PG_LWGEOM *result;
	int SRID;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	SRID = pglwgeom_getSRID(geom1);

	initJTS(lwnotice);

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2JTS(geom1 );
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	g3 = JTSBoundary(g1);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	if (g3 == NULL)
	{
		finishJTS();
		lwerror("JTS boundary() threw an error!");
		PG_RETURN_NULL(); //never get here
	}

//	elog(NOTICE,"result: %s", JTSasText(g3) ) ;

	JTSSetSRID(g3, SRID);

#ifdef PROFILE
	profstart(PROF_G2P);
#endif
	result = JTS2POSTGIS(g3, TYPE_NDIMS(geom1->type) > 2);
#ifdef PROFILE
	profstart(PROF_P2G1);
#endif

	if (result == NULL)
	{
		finishJTS();
		lwerror("JTS boundary() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); //never get here
	}


	finishJTS();

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
	JTSGeometry *g1, *g3;
	PG_LWGEOM *result;
	LWGEOM *lwout;
	int SRID;
	BOX2DFLOAT4 bbox;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	SRID = pglwgeom_getSRID(geom1);

	initJTS(lwnotice);

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2JTS(geom1);
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif
	if (g1 == NULL)
	{
		finishJTS();
		lwerror("JTS convexhull() threw an error!");
		PG_RETURN_NULL(); //never get here
	}

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	g3 = JTSConvexHull(g1);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	if (g3 == NULL)
	{
		finishJTS();
		lwerror("JTS convexhull() threw an error!");
		PG_RETURN_NULL(); //never get here
	}


	//elog(NOTICE,"result: %s", JTSasText(g3) ) ;
	JTSSetSRID(g3, SRID);

#ifdef PROFILE
	profstart(PROF_G2P);
#endif
	lwout = JTS2LWGEOM(g3, TYPE_NDIMS(geom1->type) > 2);
#ifdef PROFILE
	profstop(PROF_G2P);
#endif
	if (lwout == NULL)
	{
		finishJTS();
		lwerror("convexhull() failed to convert JTS geometry to LWGEOM");
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
		finishJTS();
		lwerror("JTS convexhull() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); //never get here
	}

	lwgeom_release(lwout);
	finishJTS();

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
	JTSGeometry *g1,*g3;
	PG_LWGEOM *result;
	int quadsegs = 8; // the default

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	size = PG_GETARG_FLOAT8(1);
	if ( PG_NARGS() > 2 ) quadsegs = PG_GETARG_INT32(2);

	initJTS(lwnotice);

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2JTS(geom1);
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	g3 = JTSBuffer(g1,size,quadsegs);
	g1=NULL; // for GC
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	if (g3 == NULL)
	{
		finishJTS();
		lwerror("JTS buffer() threw an error!");
		PG_RETURN_NULL(); //never get here
	}


//	elog(NOTICE,"result: %s", JTSasText(g3) ) ;

	JTSSetSRID(g3, pglwgeom_getSRID(geom1));

#ifdef PROFILE
	profstart(PROF_G2P);
#endif
	result = JTS2POSTGIS(g3, TYPE_NDIMS(geom1->type) > 2);
#ifdef PROFILE
	profstop(PROF_G2P);
#endif
	if (result == NULL)
	{
		finishJTS();
		lwerror("JTS buffer() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); //never get here
	}

	g3=NULL; // for GC
	finishJTS();


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
	JTSGeometry *g1,*g2,*g3;
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

	initJTS(lwnotice);

//elog(NOTICE,"intersection() START");

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2JTS(geom1);
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif

#ifdef PROFILE
	profstart(PROF_P2G2);
#endif
	g2 = POSTGIS2JTS(geom2);
#ifdef PROFILE
	profstop(PROF_P2G2);
#endif

//elog(NOTICE,"               constructed geometrys - calling geos");

//elog(NOTICE,"g1 = %s",JTSasText(g1));
//elog(NOTICE,"g2 = %s",JTSasText(g2));


//if (g1==NULL)
//	elog(NOTICE,"g1 is null");
//if (g2==NULL)
//	elog(NOTICE,"g2 is null");
//elog(NOTICE,"g2 is valid = %i",JTSisvalid(g2));
//elog(NOTICE,"g1 is valid = %i",JTSisvalid(g1));


#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	g3 = JTSIntersection(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

//elog(NOTICE,"               intersection finished");

	if (g3 == NULL)
	{
		finishJTS();
		lwerror("JTS Intersection() threw an error!");
		PG_RETURN_NULL(); //never get here
	}


	//elog(NOTICE,"result: %s", JTSasText(g3) ) ;
	JTSSetSRID(g3, SRID);

#ifdef PROFILE
	profstart(PROF_G2P);
#endif
	result = JTS2POSTGIS(g3, is3d);
#ifdef PROFILE
	profstop(PROF_G2P);
#endif

	if (result == NULL)
	{
		finishJTS();
		lwerror("JTS Intersection() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); //never get here
	}



	finishJTS();

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
	JTSGeometry *g1,*g2,*g3;
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

	initJTS(lwnotice);

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2JTS(geom1);
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif
#ifdef PROFILE
	profstart(PROF_P2G2);
#endif
	g2 = POSTGIS2JTS(geom2);
#ifdef PROFILE
	profstop(PROF_P2G2);
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	g3 = JTSDifference(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	if (g3 == NULL)
	{
		finishJTS();
		lwerror("JTS difference() threw an error!");
		PG_RETURN_NULL(); //never get here
	}

//	elog(NOTICE,"result: %s", JTSasText(g3) ) ;

	JTSSetSRID(g3, SRID);

#ifdef PROFILE
	profstart(PROF_G2P);
#endif
	result = JTS2POSTGIS(g3, is3d);
#ifdef PROFILE
	profstop(PROF_G2P);
#endif
	if (result == NULL)
	{
		finishJTS();
		lwerror("JTS difference() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); //never get here
	}

	finishJTS();


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
	JTSGeometry *g1,*g3;
	PG_LWGEOM *result;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	initJTS(lwnotice);

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2JTS(geom1);
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	g3 = JTSpointonSurface(g1);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	if (g3 == NULL)
	{
		finishJTS();
		lwerror("JTS pointonsurface() threw an error!");
		PG_RETURN_NULL(); //never get here
	}

//	elog(NOTICE,"result: %s", JTSasText(g3) ) ;

	JTSSetSRID(g3, pglwgeom_getSRID(geom1));
#ifdef PROFILE
	profstart(PROF_G2P);
#endif
	result = JTS2POSTGIS(g3, (TYPE_NDIMS(geom1->type) > 2));
#ifdef PROFILE
	profstop(PROF_G2P);
#endif
	if (result == NULL)
	{
		finishJTS();
		lwerror("JTS pointonsurface() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); //never get here
	}

	finishJTS();

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
	JTSGeometry *geosgeom, *geosresult;
	int failure;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	initJTS(lwnotice);

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	geosgeom = POSTGIS2JTS(geom);
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	geosresult = JTSGetCentroid(geosgeom, &failure);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	if ( geosresult == NULL )
	{
		finishJTS();
		if ( failure ) {
			lwerror("JTS getCentroid() threw an error!");
		}
		PG_RETURN_NULL(); 
	}

	JTSSetSRID(geosresult, pglwgeom_getSRID(geom));

#ifdef PROFILE
	profstart(PROF_G2P);
#endif
	result = JTS2POSTGIS(geosresult, (TYPE_NDIMS(geom->type) > 2));
#ifdef PROFILE
	profstop(PROF_G2P);
#endif
	if (result == NULL)
	{
		finishJTS();
		lwerror("Error in JTS-PGIS conversion");
		PG_RETURN_NULL(); 
	}

	finishJTS();

#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("geos",geom, NULL, result);
#endif

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(result);
}



//----------------------------------------------



void
errorIfJTSGeometryCollection(PG_LWGEOM *g1, PG_LWGEOM *g2)
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
	JTSGeometry *g1;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	initJTS(lwnotice);

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	lwgeom = lwgeom_deserialize(SERIALIZED_FORM(geom1));
	if ( ! lwgeom )
	{
		lwerror("unable to deserialize input");
	}
	g1 = LWGEOM2JTS(lwgeom);
	if ( ! g1 )
	{
		finishJTS();
		lwgeom_release(lwgeom);
		PG_RETURN_BOOL(FALSE);
	}
	lwgeom_release(lwgeom);
	//g1 = POSTGIS2JTS(geom1);
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = JTSisvalid(g1);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	finishJTS();

	if (result == 2)
	{
		elog(ERROR,"JTS isvalid() threw an error!");
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
// returns  if JTS::g1->overlaps(g2) returns true
// throws an error (elog(ERROR,...)) if JTS throws an error
PG_FUNCTION_INFO_V1(overlaps);
Datum overlaps(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	JTSGeometry *g1,*g2;
	bool result;
	BOX2DFLOAT4 box1, box2;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfJTSGeometryCollection(geom1,geom2);

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

	initJTS(lwnotice);

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2JTS(geom1);
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif
#ifdef PROFILE
	profstart(PROF_P2G2);
#endif
	g2 = POSTGIS2JTS(geom2);
#ifdef PROFILE
	profstop(PROF_P2G2);
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = JTSrelateOverlaps(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	finishJTS();
	if (result == 2)
	{
		lwerror("JTS overlaps() threw an error!");
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
	JTSGeometry *g1,*g2;
	bool result;
	BOX2DFLOAT4 box1, box2;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfJTSGeometryCollection(geom1,geom2);

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

	initJTS(lwnotice);

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2JTS(geom1);
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif
#ifdef PROFILE
	profstart(PROF_P2G2);
#endif
	g2 = POSTGIS2JTS(geom2);
#ifdef PROFILE
	profstop(PROF_P2G2);
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = JTSrelateContains(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	finishJTS();

	if (result == 2)
	{
		elog(ERROR,"JTS contains() threw an error!");
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
	JTSGeometry *g1,*g2;
	bool result;
	BOX2DFLOAT4 box1, box2;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfJTSGeometryCollection(geom1,geom2);

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

	initJTS(lwnotice);

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2JTS(geom1);
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif
#ifdef PROFILE
	profstart(PROF_P2G2);
#endif
	g2 = POSTGIS2JTS(geom2);
#ifdef PROFILE
	profstop(PROF_P2G2);
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = JTSrelateWithin(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	finishJTS();

	if (result == 2)
	{
		lwerror("JTS within() threw an error!");
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
	JTSGeometry *g1,*g2;
	bool result;
	BOX2DFLOAT4 box1, box2;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfJTSGeometryCollection(geom1,geom2);

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

	initJTS(lwnotice);

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2JTS(geom1);
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif
#ifdef PROFILE
	profstart(PROF_P2G2);
#endif
	g2 = POSTGIS2JTS(geom2);
#ifdef PROFILE
	profstop(PROF_P2G2);
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = JTSrelateCrosses(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	finishJTS();

	if (result == 2)
	{
		lwerror("JTS crosses() threw an error!");
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
	JTSGeometry *g1,*g2;
	bool result;
	BOX2DFLOAT4 box1, box2;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfJTSGeometryCollection(geom1,geom2);

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

	initJTS(lwnotice);

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2JTS(geom1 );
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif
#ifdef PROFILE
	profstart(PROF_P2G2);
#endif
	g2 = POSTGIS2JTS(geom2 );
#ifdef PROFILE
	profstop(PROF_P2G2);
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = JTSrelateIntersects(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif
	finishJTS();
	if (result == 2)
	{
		lwerror("JTS intersects() threw an error!");
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
	JTSGeometry *g1,*g2;
	bool result;
	BOX2DFLOAT4 box1, box2;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfJTSGeometryCollection(geom1,geom2);

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

	initJTS(lwnotice);

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2JTS(geom1 );
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif
#ifdef PROFILE
	profstart(PROF_P2G2);
#endif
	g2 = POSTGIS2JTS(geom2 );
#ifdef PROFILE
	profstop(PROF_P2G2);
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = JTSrelateTouches(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	finishJTS();

	if (result == 2)
	{
		lwerror("JTS touches() threw an error!");
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
	JTSGeometry *g1,*g2;
	bool result;
	BOX2DFLOAT4 box1, box2;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfJTSGeometryCollection(geom1,geom2);

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

	initJTS(lwnotice);

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2JTS(geom1);
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif
#ifdef PROFILE
	profstart(PROF_P2G2);
#endif
	g2 = POSTGIS2JTS(geom2);
#ifdef PROFILE
	profstop(PROF_P2G2);
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = JTSrelateDisjoint(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	finishJTS();

	if (result == 2)
	{
		lwerror("JTS disjoin() threw an error!");
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
	JTSGeometry *g1,*g2;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfJTSGeometryCollection(geom1,geom2);

	initJTS(lwnotice);

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2JTS(geom1);
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif
#ifdef PROFILE
	profstart(PROF_P2G2);
#endif
	g2 = POSTGIS2JTS(geom2);
#ifdef PROFILE
	profstop(PROF_P2G2);
#endif

	patt =  DatumGetCString(DirectFunctionCall1(textout,
                        PointerGetDatum(PG_GETARG_DATUM(2))));

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = JTSrelatePattern(g1,g2,patt);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	finishJTS();
	pfree(patt);

	if (result == 2)
	{
		elog(ERROR,"JTS relate_pattern() threw an error!");
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
	JTSGeometry *g1,*g2;
	char *relate_str;
	int len;
	text *result;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

//elog(NOTICE,"in relate_full()");

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfJTSGeometryCollection(geom1,geom2);


	initJTS(lwnotice);

//elog(NOTICE,"JTS init()");

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2JTS(geom1 );
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif
#ifdef PROFILE
	profstart(PROF_P2G2);
#endif
	g2 = POSTGIS2JTS(geom2 );
#ifdef PROFILE
	profstop(PROF_P2G2);
#endif

//elog(NOTICE,"constructed geometries ");

	if ((g1==NULL) || (g2 == NULL))
	{
		finishJTS();
		lwerror("g1 or g2 are null");
	}

//elog(NOTICE,JTSasText(g1));
//elog(NOTICE,JTSasText(g2));

//elog(NOTICE,"valid g1 = %i", JTSisvalid(g1));
//elog(NOTICE,"valid g2 = %i",JTSisvalid(g2));

//elog(NOTICE,"about to relate()");


#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	relate_str = JTSrelate(g1, g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

//lwnotice("finished relate()");

	g1=g2=NULL; // for GC
	finishJTS();

	if (relate_str == NULL)
	{
		lwerror("JTS relate() threw an error!");
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
	JTSGeometry *g1,*g2;
	bool result;
	BOX2DFLOAT4 box1, box2;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfJTSGeometryCollection(geom1,geom2);

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

	initJTS(lwnotice);

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2JTS(geom1);
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif
#ifdef PROFILE
	profstart(PROF_P2G2);
#endif
	g2 = POSTGIS2JTS(geom2);
#ifdef PROFILE
	profstop(PROF_P2G2);
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = JTSequals(g1,g2);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	finishJTS();

	if (result == 2)
	{
		lwerror("JTS equals() threw an error!");
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
	JTSGeometry *g1;
	int result;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	if (lwgeom_getnumgeometries(SERIALIZED_FORM(geom)) == 0)
		PG_RETURN_BOOL(true);

	initJTS(lwnotice);

	//elog(NOTICE,"JTS init()");

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2JTS(geom);
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = JTSisSimple(g1);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	finishJTS();

	if (result == 2)
	{
		elog(ERROR,"JTS issimple() threw an error!");
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
	JTSGeometry *g1;
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

	initJTS(lwnotice);

	//elog(NOTICE,"JTS init()");

#ifdef PROFILE
	profstart(PROF_P2G1);
#endif
	g1 = POSTGIS2JTS(geom );
#ifdef PROFILE
	profstop(PROF_P2G1);
#endif

#ifdef PROFILE
	profstart(PROF_GRUN);
#endif
	result = JTSisRing(g1);
#ifdef PROFILE
	profstop(PROF_GRUN);
#endif

	finishJTS();

	if (result == 2)
	{
		lwerror("JTS isring() threw an error!");
		PG_RETURN_NULL(); //never get here
	}

#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("geos",geom, NULL, NULL);
#endif

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_BOOL(result);
}



//= JTS <=> POSTGIS CONVERSION =========================

//-----=JTS2POSTGIS=

// Return a LWPOINT from a JTS Point.
LWPOINT *
lwpoint_from_geometry(JTSGeometry *g, char want3d)
{
	POINTARRAY *pa;
	LWPOINT *point;
	POINT3D *pts;
	size_t ptsize = want3d ? sizeof(POINT3DZ) : sizeof(POINT2D);
	int SRID = JTSGetSRID(g);

#ifdef PGIS_DEBUG_JTS2POSTGIS
	elog(NOTICE, "lwpoint_from_geometry: point size %d", ptsize);
#endif

	// Construct point array
	pa = (POINTARRAY *)lwalloc(sizeof(POINTARRAY));
	TYPE_SETZM(pa->dims, want3d, 0);
	pa->npoints = 1;

	// Fill point array
	pa->serialized_pointlist = lwalloc(ptsize);
	pts = JTSGetCoordinates(g);
	memcpy(pa->serialized_pointlist, pts, ptsize);
	JTSdeleteChar( (char*) pts);

	// Construct LWPOINT
	point = lwpoint_construct(SRID, NULL, pa);

	return point;
}

// Return a LWLINE from a JTS linestring
LWLINE *
lwline_from_geometry(JTSGeometry *g, char want3d)
{
	POINTARRAY *pa;
	LWLINE *line;
	int npoints;
	POINT3D *pts, *ip;
	int ptsize = want3d ? sizeof(POINT3D) : sizeof(POINT2D);
	int i;
	int SRID = JTSGetSRID(g);

#ifdef PGIS_DEBUG_JTS2POSTGIS
	elog(NOTICE, "lwline_from_geometry: point size %d", ptsize);
#endif

	npoints = JTSGetNumCoordinate(g);
	if (npoints <2) return NULL;

	// Construct point array
	pa = (POINTARRAY *)palloc(sizeof(POINTARRAY));
	TYPE_SETZM(pa->dims, want3d, 0);
	pa->npoints = npoints;

	// Fill point array
	pa->serialized_pointlist = palloc(ptsize*npoints);
	pts = JTSGetCoordinates(g);
	for (i=0; i<npoints; i++)
	{
		ip = &(pts[i]);
		memcpy(getPoint_internal(pa, i), ip, ptsize);
	}
	JTSdeleteChar( (char*) pts);

	// Construct LWPOINT
	line = lwline_construct(SRID, NULL, pa);

	return line;
}

// Return a LWPOLY from a JTS polygon
LWPOLY *
lwpoly_from_geometry(JTSGeometry *g, char want3d)
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
	int SRID = JTSGetSRID(g);

#ifdef PGIS_DEBUG_JTS2POSTGIS
	elog(NOTICE, "lwpoly_from_geometry: point size %d", ptsize);
#endif

	// Get number of rings, and pointlist
	pts = JTSGetCoordinates(g);
	nrings = JTSGetNumInteriorRings(g); 
	rings = (POINTARRAY **)palloc(sizeof(POINTARRAY *)*(nrings+1));

	// Exterior ring
	npoints = JTSGetNumCoordinate(JTSGetExteriorRing(g));
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
		npoints = JTSGetNumCoordinate(JTSGetInteriorRingN(g, j));
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

	JTSdeleteChar( (char*) pts);

	// Construct LWPOLY
	poly = lwpoly_construct(SRID, NULL, nrings+1, rings);

	return poly;
}

// Return a lwcollection from a JTS multi*
LWCOLLECTION *
lwcollection_from_geometry(JTSGeometry *geom, char want3d)
{
	uint32 ngeoms;
	LWGEOM **geoms = NULL;
	LWCOLLECTION *ret;
	int type = JTSGeometryTypeId(geom) ;
	int SRID = JTSGetSRID(geom);
	int i;

	ngeoms = JTSGetNumGeometries(geom);

#ifdef PGIS_DEBUG_JTS2POSTGIS
	lwnotice("lwcollection_from_geometry: type: %s, geoms %d",
		lwgeom_typename(type), ngeoms);
#endif

	if ( ngeoms ) geoms = lwalloc(sizeof(LWGEOM *)*ngeoms);

	for (i=0; i<ngeoms; i++)
	{
		JTSGeometry *g = JTSGetGeometryN(geom, i);
#ifdef PGIS_DEBUG_JTS2POSTGIS
		lwnotice("lwcollection_from_geometry: geom %d is a %s", i, lwgeom_typename(JTSGeometryTypeId(g)));
#endif
		geoms[i] = JTS2LWGEOM(g, want3d);
#ifdef PGIS_DEBUG_JTS2POSTGIS
		lwnotice("lwcollection_from_geometry: geoms[%d] is a %s", i, lwgeom_typename(TYPE_GETTYPE(geoms[i]->type)));
#endif
	}

	ret = lwcollection_construct(type, SRID, NULL, ngeoms, geoms);
	return ret;
}

#if WKT_J2P

// Return an LWGEOM from a JTSGeometry
LWGEOM *
JTS2LWGEOM(JTSGeometry *geom, char want3d)
{
	char *wkt;
	PG_LWGEOM *pglwgeom;
	LWGEOM *ret;

	//lwnotice("JTS2LWGEOM(WKT) called");

	wkt = JTSasText(geom);

#ifdef PGIS_DEBUG_JTS2POSTGIS
	lwnotice("JTS2LWGEOM: wkt: %d bytes", strlen(wkt));
#endif

	pglwgeom = (PG_LWGEOM *)parse_lwgeom_wkt(wkt);

	//lwnotice("JTS2LWGEOM(WKT): wkt parsed (got pglwgeom)");

#ifdef PGIS_DEBUG_JTS2POSTGIS
	lwnotice("JTS2LWGEOM: parsed: %s",
		lwgeom_typename(TYPE_GETTYPE(pglwgeom->type)));
#endif
	ret = lwgeom_deserialize(SERIALIZED_FORM(pglwgeom));
	ret->SRID = JTSGetSRID(geom);
	//lwnotice("JTS2LWGEOM: return SRID: %d", ret->SRID);

	return ret;

}

#else // ! WKT_J2P

// Return an LWGEOM from a JTSGeometry
LWGEOM *
JTS2LWGEOM(JTSGeometry *geom, char want3d)
{
	int type = JTSGeometryTypeId(geom) ;
	bool hasZ = JTSHasZ(geom);

	//lwnotice("JTS2LWGEOM(!WKT) called");

	if ( ! hasZ )
	{
		if ( want3d )
		{
			//elog(NOTICE, "JTSGeometry has no Z, won't provide one");
			want3d = 0;
		}
	}

#ifdef PGIS_DEBUG_JTS2POSTGIS
	lwnotice("JTS2LWGEOM: it's a %s", lwgeom_typename(type));
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
			lwerror("JTS2LWGEOM: unknown geometry type: %d", type);
			return NULL;

	}

}

#endif // ! WKT_J2P


PG_LWGEOM *
JTS2POSTGIS(JTSGeometry *geom, char want3d)
{
	LWGEOM *lwgeom;
	PG_LWGEOM *result;

#ifdef PGIS_DEBUG_JTS2POSTGIS
	lwnotice("JTS2POSTGIS: called");
#endif

	lwgeom = JTS2LWGEOM(geom, want3d);
	if ( ! lwgeom )
	{
		lwerror("JTS2POSTGIS: JTS2LWGEOM returned NULL");
		return NULL;
	}

#ifdef PGIS_DEBUG_JTS2POSTGIS
	lwnotice("JTS2POSTGIS: JTS2LWGEOM returned a %s", lwgeom_summary(lwgeom, 0)); 
#endif

	if ( is_worth_caching_lwgeom_bbox(lwgeom) )
	{
		lwgeom_addBBOX(lwgeom);
	}

	result = pglwgeom_serialize(lwgeom);

	return result;
}


//-----=POSTGIS2JTS=

JTSGeometry *LWGEOM2JTS(LWGEOM *lwgeom);

#if WKT_P2J

JTSGeometry *
LWGEOM2JTS(LWGEOM *lwgeom)
{
	PG_LWGEOM *geom;
	PG_LWGEOM *ogcgeom;
	char *wkt, *ogcwkt, *loc;
	JTSGeometry *g;

	//lwnotice("LWGEOM2JTS: WKT version");

	geom = pglwgeom_serialize(lwgeom);
	if ( ! geom ) {
		lwerror("LWGEOM2JTS(WKT): couldn't serialized lwgeom");
		return NULL;
	}

	/* Force to 2d */
	ogcgeom = (PG_LWGEOM *)DatumGetPointer(DirectFunctionCall1(
		LWGEOM_force_2d, PointerGetDatum(geom)));
	wkt = unparse_WKT(SERIALIZED_FORM(ogcgeom), lwalloc, lwfree);

	lwfree(ogcgeom);
	if ( geom != ogcgeom ) lwfree(geom);

	loc = strchr(wkt,';');
	if (loc != NULL) ogcwkt = loc+1;
	else ogcwkt=wkt;

	g = JTSGeometryFromWKT(ogcwkt);
	JTSSetSRID(g, lwgeom->SRID);

	lwfree(wkt);
#ifdef PGIS_DEBUG_POSTGIS2JTS
	lwnotice("LWGEOM2JTS: returning JTS JTSGeometry");
#endif
	return g;
}

#else  // ! WKT_P2J

JTSGeometry *
LWGEOM2JTS(LWGEOM *lwgeom)
{
	uint32 i;
	JTSGeometry **collected;
	LWCOLLECTION *col;

	//lwnotice("LWGEOM2JTS: !WKT version");

	if ( ! lwgeom ) return NULL;

#ifdef PGIS_DEBUG_POSTGIS2JTS
	lwnotice("LWGEOM2JTS: got lwgeom[%p]", lwgeom);
#endif

	switch (TYPE_GETTYPE(lwgeom->type))
	{
		case POINTTYPE:
#ifdef PGIS_DEBUG_POSTGIS2JTS
			lwnotice("LWGEOM2JTS: point[%p]", lwgeom);
#endif
			return PostGIS2JTS_point((LWPOINT *)lwgeom);

		case LINETYPE:
#ifdef PGIS_DEBUG_POSTGIS2JTS
			lwnotice("LWGEOM2JTS: line[%p]", lwgeom);
#endif
			return PostGIS2JTS_linestring((LWLINE *)lwgeom);

		case POLYGONTYPE:
#ifdef PGIS_DEBUG_POSTGIS2JTS
			lwnotice("LWGEOM2JTS: poly[%p]", lwgeom);
#endif
			return PostGIS2JTS_polygon((LWPOLY *)lwgeom);

		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case COLLECTIONTYPE:
			col = (LWCOLLECTION *)lwgeom;
#ifdef PGIS_DEBUG_POSTGIS2JTS
			lwnotice("LWGEOM2JTS: %s with %d subgeoms", lwgeom_typename(TYPE_GETTYPE(col->type)), col->ngeoms);
#endif
			collected = (JTSGeometry **)lwalloc(sizeof(JTSGeometry *)*col->ngeoms);
			for (i=0; i<col->ngeoms; i++)
			{
				collected[i] = LWGEOM2JTS(col->geoms[i]);
			}
			return PostGIS2JTS_collection(TYPE_GETTYPE(col->type),
				collected, col->ngeoms, col->SRID,
				TYPE_NDIMS(col->type)>2);

		default:
			lwerror("Unknown geometry type: %d",
				TYPE_GETTYPE(lwgeom->type));
			return NULL;
	}

}

#endif // ! WKT_P2J

JTSGeometry *
POSTGIS2JTS(PG_LWGEOM *geom)
{
	JTSGeometry *ret;
	LWGEOM *lwgeom;

#ifdef PGIS_DEBUG_POSTGIS2JTS
	lwnotice("POSTGIS2JTS: called");
#endif

	lwgeom = lwgeom_deserialize(SERIALIZED_FORM(geom));
	if ( ! lwgeom )
	{
		lwerror("POSTGIS2JTS: unable to deserialize input");
		return NULL;
	}
	ret = LWGEOM2JTS(lwgeom);

	lwgeom_release(lwgeom);
	if ( ! ret )  {
		lwerror("POSTGIS2JTS conversion failed");
		return NULL;
	}
#ifdef PGIS_DEBUG_POSTGIS2JTS
	lwnotice("POSTGIS2JTS: returning %p", ret);
#endif
	return ret;
}

PG_FUNCTION_INFO_V1(JTSnoop);
Datum JTSnoop(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	JTSGeometry *geosgeom;
	PG_LWGEOM *result;

	initJTS(lwnotice);

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
#ifdef PGIS_DEBUG_CONVERTER
	lwnotice("JTSnoop: IN: %s", lwgeom_summary(lwgeom_deserialize(SERIALIZED_FORM(geom)), 0));
#endif

	geosgeom = POSTGIS2JTS(geom);
	if ( ! geosgeom ) PG_RETURN_NULL();

#ifdef PROFILE
	profstart(PROF_GRUN);
	profstop(PROF_GRUN);
#endif

	result = JTS2POSTGIS(geosgeom, TYPE_NDIMS(geom->type) > 2);

	finishJTS();

#ifdef PGIS_DEBUG_CONVERTER
	elog(NOTICE, "JTSnoop: OUT: %s", unparse_WKT(SERIALIZED_FORM(result), lwalloc, lwfree));
#endif

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(JTS_polygonize_garray);
Datum JTS_polygonize_garray(PG_FUNCTION_ARGS)
{
	Datum datum;
	ArrayType *array;
	int is3d = 0;
	unsigned int nelems, i;
	PG_LWGEOM *result;
	JTSGeometry *geos_result;
	JTSGeometry **vgeoms;
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
	elog(NOTICE, "JTS_polygonize_garray: number of elements: %d", nelems);
#endif

	if ( nelems == 0 ) PG_RETURN_NULL();

	/* Ok, we really need geos now ;) */
	initJTS(lwnotice);

	vgeoms = palloc(sizeof(JTSGeometry *)*nelems);
	offset = 0;
	for (i=0; i<nelems; i++)
	{
		PG_LWGEOM *geom = (PG_LWGEOM *)(ARR_DATA_PTR(array)+offset);
		offset += INTALIGN(geom->size);

		vgeoms[i] = POSTGIS2JTS(geom);
		if ( ! i )
		{
			SRID = pglwgeom_getSRID(geom);
		}
		else
		{
			if ( SRID != pglwgeom_getSRID(geom) )
			{
	finishJTS();
	lwerror("polygonize: operation on mixed SRID geometries");
	PG_RETURN_NULL();
			}
		}
	}

#ifdef PGIS_DEBUG
	elog(NOTICE, "JTS_polygonize_garray: invoking JTSpolygonize");
#endif

	geos_result = JTSpolygonize(vgeoms, nelems);
#ifdef PGIS_DEBUG
	elog(NOTICE, "JTS_polygonize_garray: JTSpolygonize returned");
#endif
	//pfree(vgeoms);
	if ( ! geos_result ) {
		finishJTS();
		PG_RETURN_NULL();
	}

	JTSSetSRID(geos_result, SRID);
	result = JTS2POSTGIS(geos_result, is3d);
	finishJTS();
	if ( result == NULL )
	{
		elog(ERROR, "JTS2POSTGIS returned an error");
		PG_RETURN_NULL(); //never get here
	}

	//compressType(result);

	PG_RETURN_POINTER(result);

}

Datum GEOS_polygonize_garray(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(GEOS_polygonize_garray);
Datum GEOS_polygonize_garray(PG_FUNCTION_ARGS)
{
	elog(NOTICE, "GEOS support is disabled, use JTS_polygonize_garray instead");
	PG_RETURN_NULL();
}

Datum GEOSnoop(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(GEOSnoop);
Datum GEOSnoop(PG_FUNCTION_ARGS)
{
	elog(NOTICE, "GEOS support is disabled, use JTSnoop instead");
	PG_RETURN_NULL();
}

PG_FUNCTION_INFO_V1(postgis_geos_version);
Datum postgis_geos_version(PG_FUNCTION_ARGS)
{
	PG_RETURN_NULL();
}

