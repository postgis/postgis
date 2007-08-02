#include <stdio.h>

#include <string>
#include <iostream>
#include <fstream>

#pragma GCC java_exceptions

#include <jts.h>
#include <java/lang/String.h>
#include <java/lang/System.h>
#include <java/io/PrintStream.h>
#include <java/lang/Throwable.h>

//#define DEBUG_JARRAY 1

using namespace com::vividsolutions::jts::geom::prep;
using namespace com::vividsolutions::jts::geom;
using namespace com::vividsolutions::jts::io;
using namespace com::vividsolutions::jts;
using namespace java::lang;
using namespace std;

//for getting things to align properly  double are on 8byte align on solaris machines, and 4bytes on intel

//---- Definitions found in lwgeom.h (and postgis)

#define TYPE_NDIMS(t) ((((t)&0x20)>>5)+(((t)&0x10)>>4)+2)
#define TYPE_HASZ(t) ( ((t)&0x20)>>5 )

typedef unsigned int uint32;
typedef int int32;

typedef struct
{
        double xmin, ymin, zmin;
        double xmax, ymax, zmax;
} BOX3D;

typedef struct { float xmin, ymin, xmax, ymax; } BOX2DFLOAT4;

typedef struct { double	x,y,z; } POINT3D;

typedef struct
{
	char  *serialized_pointlist; 
	unsigned char dims;
	uint32 npoints;
}  POINTARRAY;

typedef struct
{
	unsigned char type; 
	BOX2DFLOAT4 *bbox;
   	uint32 SRID;	
   	POINTARRAY *point;  // hide 2d/3d (this will be an array of 1 point)
}  LWPOINT; // "light-weight point"

// LINETYPE
typedef struct
{
	unsigned char type; 
	BOX2DFLOAT4 *bbox;
   	uint32 SRID;	
	POINTARRAY    *points; // array of POINT3D
} LWLINE; //"light-weight line"

// POLYGONTYPE
typedef struct
{
	unsigned char type; 
	BOX2DFLOAT4 *bbox;
   	uint32 SRID;	
	int  nrings;
	POINTARRAY **rings; // list of rings (list of points)
} LWPOLY; // "light-weight polygon"

extern "C" int getPoint3dz_p(POINTARRAY *pa, int n, POINT3D *);

//---- End of definitions found in lwgeom.h

#define	POINTTYPE	1
#define	LINETYPE	2
#define	POLYGONTYPE	3
#define	MULTIPOINTTYPE	4
#define	MULTILINETYPE	5
#define	MULTIPOLYGONTYPE	6
#define	COLLECTIONTYPE	7

//###########################################################

typedef void (*noticefunc)(const char *fmt, ...);

extern "C" char *JTSrelate(Geometry *g1, Geometry*g2);
extern "C" void initJTS(noticefunc);
extern "C" void finishJTS(void);


extern "C" void JTSSetSRID(Geometry *g, int SRID);
extern "C" void JTSdeleteChar(char *a);
extern "C" void JTSdeleteGeometry(Geometry *a);
extern "C" char JTSrelatePattern(Geometry *g1, Geometry*g2,char *pat);
extern "C" char JTSrelateDisjoint(Geometry *g1, Geometry*g2);
extern "C" char JTSrelateTouches(Geometry *g1, Geometry*g2);
extern "C" char JTSrelateIntersects(Geometry *g1, Geometry*g2);
extern "C" char JTSrelateCrosses(Geometry *g1, Geometry*g2);
extern "C" char JTSrelateWithin(Geometry *g1, Geometry*g2);
extern "C" char JTSrelateContains(Geometry *g1, Geometry*g2);
extern "C" char JTSrelateOverlaps(Geometry *g1, Geometry*g2);
extern "C" Geometry *JTSPolygonize(Geometry **geoms, unsigned int ngeoms);
extern "C" Geometry *JTSLineMerge(Geometry *geoms);
extern "C" char *JTSversion();
extern "C" char *JTSjtsport();
extern "C" int JTSGeometryTypeId(Geometry *g1);

extern "C" char JTSpreparedIntersects(PreparedGeometry *pg, Geometry* g);
extern "C" PreparedGeometry *JTSPrepareGeometry(Geometry* g);
extern "C" void JTSgc(void);
extern "C" void JTSPrintPreparedGeometry(PreparedGeometry *pg);
extern "C" PreparedGeometry *JTSGetRHSCache();
extern "C" PreparedGeometry *JTSGetLHSCache();
extern "C" void JTSPutRHSCache(PreparedGeometry *rhs);
extern "C" void JTSPutLHSCache(PreparedGeometry *lhs);

extern "C" char JTSisvalid(Geometry *g1);

/* Converters */
extern "C" Geometry *PostGIS2JTS_collection(int type, Geometry **geoms, int ngeoms, int SRID, bool is3d);
extern "C" Geometry *PostGIS2JTS_point(const LWPOINT *point);
extern "C" Geometry *PostGIS2JTS_linestring(const LWLINE *line);
extern "C" Geometry *PostGIS2JTS_polygon(const LWPOLY *polygon);

extern "C" char *JTSasText(Geometry *g1);

extern "C" char JTSisEmpty(Geometry *g1);
extern "C" char *JTSGeometryType(Geometry *g1);


extern "C" char *throw_exception(Geometry *g);

extern "C" Geometry *JTSIntersection(Geometry *g1,Geometry *g1);
extern "C" Geometry *JTSBuffer(Geometry *g1,double width,int quadsegs);
extern "C" Geometry *JTSConvexHull(Geometry *g1);
extern "C" Geometry *JTSDifference(Geometry *g1,Geometry *g2);
extern "C" Geometry *JTSBoundary(Geometry *g1);
extern "C" Geometry *JTSSymDifference(Geometry *g1,Geometry *g2);
extern "C" Geometry *JTSUnion(Geometry *g1,Geometry *g2);


extern "C" POINT3D  *JTSGetCoordinate(Geometry *g1);
extern "C" POINT3D  *JTSGetCoordinates(Geometry *g1);
extern "C" int      JTSGetNumCoordinate(Geometry *g1);
extern "C" Geometry *JTSGetGeometryN(Geometry *g1, int n);
extern "C" Geometry *JTSGetExteriorRing(Geometry *g1);
extern "C" Geometry *JTSGetInteriorRingN(Geometry *g1, int n);
extern "C" int JTSGetNumInteriorRings(Geometry *g1);
extern "C" int JTSGetSRID(Geometry *g1);
extern "C" int JTSGetNumGeometries(Geometry *g1);

extern "C" char JTSisSimple(Geometry *g1);
extern "C" char JTSequals(Geometry *g1, Geometry*g2);
extern "C" char JTSisRing(Geometry *g1);
extern "C" Geometry *JTSpointonSurface(Geometry *g1);
extern "C" Geometry *JTSGetCentroid(Geometry *g1, int *failure);
extern "C" bool JTSHasZ(Geometry *g1);
extern "C" Geometry * JTSGeometryFromWKT(const char *wkt);

//###########################################################

char * StringToChar(String *s);
static GeometryFactory *jtsGeomFactory = NULL;
static noticefunc NOTICE_MESSAGE;

void
initJTS(noticefunc nf)
{
	if (jtsGeomFactory == NULL)
	{
		JvCreateJavaVM(NULL);
	        JvAttachCurrentThread(NULL, NULL);	
		JvInitClass(&Geometry::class$);
		JvInitClass(&GeometryFactory::class$);
		JvInitClass(&Coordinate::class$);
		JvInitClass(&JTSVersion::class$);
                JvInitClass(&PreparedGeometry::class$);
                JvInitClass(&PreparedGeometryFactory::class$);
                JvInitClass(&PreparedGeometryCache::class$);

		// NOTE: SRID will have to be changed after geometry creation
		jtsGeomFactory = new GeometryFactory( new PrecisionModel(), -1);

	}
	NOTICE_MESSAGE = nf;
}


void
finishJTS(void)
{
	//System::gc();
	JvDetachCurrentThread();
}

void
JTSgc(void)
{
        System::gc();
}

Geometry *
JTSGeometryFromWKT(const char *wkt)
{
	try {
#ifdef DEBUG_POSTGIS2GEOS
		NOTICE_MESSAGE("JTSGeometryFromWKT called");
#endif
		static WKTReader *r = new WKTReader;
		jstring wkt_j = JvNewStringLatin1(wkt);
		Geometry *g = r->read(wkt_j);
		return g;
	} catch (Throwable *t) {
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return NULL;
	}
}

//-----------------------------------------------------------
// relate()-related functions
//  return 0 = false, 1 = true, 2 = error occured
//-----------------------------------------------------------

char JTSrelateDisjoint(Geometry *g1, Geometry*g2)
{
	try {
		bool result;
		result = g1->disjoint(g2);
		return result;
	}
	catch (Throwable *t)
	{
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return 2;
	}
}

char JTSrelateTouches(Geometry *g1, Geometry*g2)
{
	try {
		bool result;
		result =  g1->touches(g2);
		return result;
	}
	catch (Throwable *t)
	{
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return 2;
	}
}

char JTSrelateIntersects(Geometry *g1, Geometry*g2)
{
	try {
		bool result;
		result = g1->intersects(g2);
		return result;
	}
	catch (Throwable *t)
	{
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return 2;
	}
}

char JTSpreparedIntersects(PreparedGeometry *pg, Geometry* g)
{
        try {
                bool result;
                result = pg->intersects(g);
                return result;
        }
        catch (Throwable *t)
        {
                NOTICE_MESSAGE(StringToChar(t->getMessage()));
                return 2;
        }
}

PreparedGeometry *JTSPrepareGeometry(Geometry* g)
{
        try {
                PreparedGeometryFactory *pgFact;
                PreparedGeometry *pg;
                pgFact = new PreparedGeometryFactory();
                pg = pgFact->create(g);
                return pg;
        }
        catch (Throwable *t)
        {
                NOTICE_MESSAGE(StringToChar(t->getMessage()));
                return NULL;
        }
}

void JTSPrintPreparedGeometry(PreparedGeometry *pg)
{
        try {
                NOTICE_MESSAGE(StringToChar(pg->getGeometry()->toString()));
        }
        catch (Throwable *t)
        {
                NOTICE_MESSAGE(StringToChar(t->getMessage()));
        }
}

PreparedGeometry *JTSGetRHSCache()
{
        try {
                PreparedGeometryCache *cache;
                PreparedGeometry *pg;
                cache = PreparedGeometryCache::instance;
                pg = cache->getRightHandSide();
                return pg;
        }
        catch (Throwable *t)
        {
                NOTICE_MESSAGE(StringToChar(t->getMessage()));
                return NULL;
        }
}

PreparedGeometry *JTSGetLHSCache()
{
        try {
                PreparedGeometryCache *cache;
                PreparedGeometry *pg;
                cache = PreparedGeometryCache::instance;
                pg = cache->getLeftHandSide();
                return pg;
        }
        catch (Throwable *t)
        {
                NOTICE_MESSAGE(StringToChar(t->getMessage()));
                return NULL;
        }
}

void JTSPutRHSCache(PreparedGeometry *rhs)
{
        try {
                PreparedGeometryCache *cache;
                cache = PreparedGeometryCache::instance;
                cache->putRightHandSide(rhs);
        }
        catch (Throwable *t)
        {
                NOTICE_MESSAGE(StringToChar(t->getMessage()));
        }
}
void JTSPutLHSCache(PreparedGeometry *lhs)
{
        try {
                PreparedGeometryCache *cache;
                cache = PreparedGeometryCache::instance;
                cache->putLeftHandSide(lhs);
        }
        catch (Throwable *t)
        {
                NOTICE_MESSAGE(StringToChar(t->getMessage()));
        }
}

char JTSrelateCrosses(Geometry *g1, Geometry*g2)
{
	try {
		bool result;
		result = g1->crosses(g2);
		return result;
	}
	catch (Throwable *t)
	{
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return 2;
	}
}

char JTSrelateWithin(Geometry *g1, Geometry*g2)
{
	try {
		bool result;
		result = g1->within(g2);
		return result;
	}
	catch (Throwable *t)
	{
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return 2;
	}
}

// call g1->contains(g2)
// returns 0 = false
//         1 = true
//         2 = error was trapped
char JTSrelateContains(Geometry *g1, Geometry*g2)
{
	try {
		bool result;
		result = g1->contains(g2);
		return result;
	}
	catch (Throwable *t)
	{
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return 2;
	}
}

char JTSrelateOverlaps(Geometry *g1, Geometry*g2)
{
	try {
		bool result;
		result = g1->overlaps(g2);
		return result;
	}
	catch (Throwable *t)
	{
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return 2;
	}
}


//-------------------------------------------------------------------
// low-level relate functions
//------------------------------------------------------------------

char
JTSrelatePattern(Geometry *g1, Geometry*g2,char *pat)
{
	try {
		bool result;
		string s = pat;
		result = g1->relate(g2,JvNewStringLatin1(pat));
		return result;
	}
	catch (Throwable *t)
	{
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return 2;
	}
}

char *
JTSrelate(Geometry *g1, Geometry*g2)
{

	try {

		IntersectionMatrix *im = g1->relate(g2);

		if (im == NULL) return NULL;

		jstring s = im->toString();
		return StringToChar(s);
	}
	catch (Throwable *t)
	{
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return NULL;
	}
}



//-----------------------------------------------------------------
// isValid
//-----------------------------------------------------------------


char JTSisvalid(Geometry *g1)
{
#if JTS_FIRST_INTERFACE <= 3 && JTS_LAST_INTERFACE >= 3
	IsValidOp ivo(g1);
#endif
	bool result;
	try {
#if JTS_FIRST_INTERFACE <= 3 && JTS_LAST_INTERFACE >= 3
		result = ivo.isValid();
		if ( result == 0 )
		{
			TopologyValidationError *err = ivo.getValidationError();
			if ( err ) {
				NOTICE_MESSAGE(StringToChar(err->getMessage()));
			}
		}
#else // JTS_INTERFACE 3 not supported
		result = g1->isValid();
#endif
		return result;
	}
	catch (Throwable *t)
	{
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return 2;
	}

}


//-----------------------------------------------------------------
// general purpose
//-----------------------------------------------------------------

char JTSequals(Geometry *g1, Geometry*g2)
{
	try {
		bool result;
		result = g1->equals(g2);
		return result;
	}
	catch (Throwable *t)
	{
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return 2;
	}
}



char *
JTSasText(Geometry *g1)
{
	try {
		NOTICE_MESSAGE("JTSasText called");
		jstring s = g1->toString();
		return StringToChar(s);
	} catch (Throwable *t) {
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return NULL;
	}
}

char
JTSisEmpty(Geometry *g1)
{
	try {
		return g1->isEmpty();
	} catch (Throwable *t) {
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return 2;
	}
}

char
JTSisSimple(Geometry *g1)
{
	try {
		return g1->isSimple();
	} catch (Throwable *t) {
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return 2;
	}
}

char
JTSisRing(Geometry *g1)
{
	try {
		return (( (LinearRing*)g1)->isRing());
	} catch (Throwable *t) {
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return 2;
	}
}



//free the result of this
char *
JTSGeometryType(Geometry *g1)
{
	try {
		jstring s = g1->getGeometryType();
		return StringToChar(s);
	} catch (Throwable *t) {
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return NULL;
	}
}


//-------------------------------------------------------------------
// JTS functions that return geometries
//-------------------------------------------------------------------

Geometry *
JTSIntersection(Geometry *g1,Geometry *g2)
{
	try
	{
		Geometry *g3 = g1->intersection(g2);
		return g3;
	} catch (Throwable *t) {
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return NULL;
	}
}

Geometry *
JTSBuffer(Geometry *g1, double width, int quadrantsegments)
{
	try
	{
		Geometry *g3 = g1->buffer(width, quadrantsegments);
		return g3;
	} catch (Throwable *t) {
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return NULL;
	}
}

Geometry *
JTSConvexHull(Geometry *g1)
{
	try
	{
		Geometry *g3 = g1->convexHull();
		return g3;
	} catch (Throwable *t) {
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return NULL;
	}
}

Geometry *
JTSDifference(Geometry *g1,Geometry *g2)
{
	try {
		Geometry *g3 = g1->difference(g2);
		return g3;
	} catch (Throwable *t) {
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return NULL;
	}
}

Geometry *
JTSBoundary(Geometry *g1)
{
	try {
		Geometry *g3 = g1->getBoundary();
		return g3;
	} catch (Throwable *t) {
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return NULL;
	}
}

Geometry *
JTSSymDifference(Geometry *g1,Geometry *g2)
{
	try {
		Geometry *g3 = g1->symDifference(g2);
		return g3;
	} catch (Throwable *t) {
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return NULL;
	}
}

Geometry *
JTSUnion(Geometry *g1,Geometry *g2)
{
	try {
		Geometry *g3 = g1->union$(g2);
		return g3;
	} catch (Throwable *t) {
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return NULL;
	} 
}


Geometry *
JTSpointonSurface(Geometry *g1)
{
	try {
		Geometry *g3 = g1->getInteriorPoint();
		return g3;
	} catch (Throwable *t) {
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return NULL;
	}
}





//-------------------------------------------------------------------
// memory management functions
//------------------------------------------------------------------


//BUG:: this leaks memory, but delete kills the PrecisionModel for ALL the geometries
void
JTSdeleteGeometry(Geometry *a)
{
	cerr<<"Don't call JTSdeleteGeometry, the GC will do.."<<endl;
	//finishJTS();
	//try { delete a; } catch(...){}
	//return; // run finishJTS() when done
}

void
JTSSetSRID(Geometry *g, int SRID)
{
	g->setSRID(SRID);
}

void JTSdeleteChar(char *a)
{
	free(a);
}


//-------------------------------------------------------------------
//JTS => POSTGIS conversions
//-------------------------------------------------------------------


// free the result when done!
// g1 must be a point
POINT3D *
JTSGetCoordinate(Geometry *g1)
{
	try{
		POINT3D		*result = (POINT3D*) malloc (sizeof(POINT3D));
		const Coordinate *c =g1->getCoordinate();

		result->x = c->x;
		result->y = c->y;
		result->z = c->z;
		return result;
	} catch (Throwable *t) {
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return NULL;
	}
}


//must free the result when done
// result is an array length g1->getNumCoordinates()
POINT3D *
JTSGetCoordinates(Geometry *g1)
{
	try {
		int numPoints = g1->getNumPoints();
		POINT3D *result = (POINT3D*)malloc(sizeof(POINT3D)*numPoints);

		JArray<Coordinate *> *cl = g1->getCoordinates();
		Coordinate **coords = elements(cl);
		Coordinate *c;

		for (int t=0; t<numPoints; t++)
		{
			c = coords[t];

			result[t].x = c->x;
			result[t].y = c->y;
			result[t].z = c->z;
		}

		return result;
	} catch (Throwable *t) {
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return NULL;
	}

}

int
JTSGetNumCoordinate(Geometry *g1)
{
	try{
		return g1->getNumPoints();
	} catch (Throwable *t) {
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return 0;
	}
}

int
JTSGetNumInteriorRings(Geometry *g1)
{
	try{
		Polygon *p = (Polygon *) g1;
		return p->getNumInteriorRing();
	} catch (Throwable *t) {
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return 0;
	}
}


//only call on GCs (or multi*)
int
JTSGetNumGeometries(Geometry *g1)
{
	try {
		GeometryCollection *gc = (GeometryCollection *) g1;
		return gc->getNumGeometries();
	} catch (Throwable *t) {
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return 0;
	}
}


//call only on GEOMETRYCOLLECTION or MULTI*
Geometry *
JTSGetGeometryN(Geometry *g1, int n)
{
	try{
		return ((GeometryCollection *)g1)->getGeometryN(n);
	} catch (Throwable *t) {
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return NULL;
	}
}


//call only on polygon
Geometry *
JTSGetExteriorRing(Geometry *g1)
{
	try{
		Polygon *p = (Polygon *) g1;
		return p->getExteriorRing();
	} catch (Throwable *t) {
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return NULL;
	}
}

//call only on polygon
Geometry *
JTSGetInteriorRingN(Geometry *g1,int n)
{
	try {
		Polygon *p = (Polygon *) g1;
		return p->getInteriorRingN(n);
	} catch (Throwable *t) {
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return NULL;
	}
}

Geometry *
JTSPolygonize(Geometry **g, unsigned int ngeoms)
{
	NOTICE_MESSAGE("JTS Polygonize unimplemented");
	return NULL;
}

Geometry *
JTSLineMerge(Geometry *g)
{
	NOTICE_MESSAGE("JTS LineMerge unimplemented");
	return NULL;
}

Geometry *
JTSGetCentroid(Geometry *g, int *failure)
{
	try{
		Geometry *ret = g->getCentroid();
		*failure = 0;
		return ret;
	} catch (Throwable *t) {
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return NULL;
	}
}

int
JTSGetSRID(Geometry *g1)
{
	try {
		return g1->getSRID();
	} catch (Throwable *t) {
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return 0;
	}
}

char *
JTSversion()
{
	//char *res = strdup("unknown");
        JTSVersion *v = JTSVersion::CURRENT_VERSION;
        if ( ! v ) return "UNDEFINED";
        return StringToChar(v->toString());
	//return res;
}

bool
ensureMinimumVersion(int major, int minor, int patch)
{
        JTSVersion *v = JTSVersion::CURRENT_VERSION;
        if (!v) return false;
        return false;
}

bool 
JTSHasZ(Geometry *g)
{
	//double az = g->getCoordinate()->z;
	//return (finite(az) && az != DoubleNotANumber);
	return false;
}

int
JTSGeometryTypeId(Geometry *g1)
{
	jstring s = g1->getGeometryType();
	char *type = StringToChar(s);
	if ( ! strcmp(type, "Point") ) return POINTTYPE;
	if ( ! strcmp(type, "Polygon") ) return POLYGONTYPE;
	if ( ! strcmp(type, "LineString") ) return LINETYPE;
	if ( ! strcmp(type, "LinearRing") ) return LINETYPE;
	if ( ! strcmp(type, "MultiLineString") ) return MULTILINETYPE;
	if ( ! strcmp(type, "MultiPoint") ) return MULTIPOINTTYPE;
	if ( ! strcmp(type, "MultiPolygon") ) return MULTIPOLYGONTYPE;
	if ( ! strcmp(type, "GeometryCollection") ) return COLLECTIONTYPE;
	else
	{
		NOTICE_MESSAGE("JTSGeometryTypeId: unknown geometry type: %s",
			type);
		return -1; // unknown type
	}
}

char *
StringToChar(String *s)
{
	int len = JvGetStringUTFLength(s);
	char *result = (char *)malloc(len+1);
	JvGetStringUTFRegion(s, 0, len, result);
	result[len] = '\0';
	return result;
}

/* CONVERTERS */

Geometry *
PostGIS2JTS_point(const LWPOINT *lwpoint)
{
	POINT3D point;
	int SRID;
	bool is3d;

#ifdef DEBUG_POSTGIS2GEOS
	char buf[256];
	sprintf(buf, "PostGIS2GEOS_point got lwpoint[%p]", lwpoint);
	NOTICE_MESSAGE(buf);
#endif

	if ( lwpoint == NULL )
	{
		NOTICE_MESSAGE("PostGIS2GEOS_point got a NULL lwpoint");
		return NULL;
	}

	getPoint3dz_p(lwpoint->point, 0, &point);
	SRID = lwpoint->SRID;
	is3d = TYPE_HASZ(lwpoint->type);

	try
	{
		Coordinate *c;

		if (is3d)
			c = new Coordinate(point.x, point.y, point.z);
		else
			c = new Coordinate(point.x, point.y);
		Geometry *g = jtsGeomFactory->createPoint(c);
		if (g==NULL) return NULL;
		g->setSRID(SRID);
		return g;
	} catch (Throwable *t) {
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return NULL;
	}
}

/*
 * This function must return an all-new allocated object
 */
Geometry *
PostGIS2JTS_linestring(const LWLINE *lwline)
{
	POINTARRAY *pa = lwline->points;
	bool is3d = TYPE_HASZ(pa->dims);
	int SRID = lwline->SRID;

#ifdef DEBUG_POSTGIS2GEOS
	char buf[256];
	sprintf(buf, "PostGIS2GEOS_line got lwline[%p]", lwline);
	NOTICE_MESSAGE(buf);
#endif

	try{
		uint32 t;
		JArray<Coordinate *>*coordlist;
		POINT3D p;

		// build Coordinate[]
		jobjectArray vc = JvNewObjectArray(pa->npoints,
			&Coordinate::class$, NULL);
#ifdef DEBUG_JARRAY
		//std::cout<<"PostGIS2JTS_linestring: vc Jarray: "<<vc<<endl;
#endif

		if (is3d)
		{
			for (t=0; t<pa->npoints; t++)
			{
				getPoint3dz_p(pa, t, &p);
				elements(vc)[t] = new Coordinate(p.x, p.y, p.z);
			}
		}
		else  //make 2d points
		{
			for (t=0; t<pa->npoints; t++)
			{
				getPoint3dz_p(pa, t, &p);
				elements(vc)[t] = new Coordinate(p.x, p.y);
			}
		}

		coordlist = reinterpret_cast<JArray<Coordinate *>*>(vc);
		CoordinateSequence *coords = jtsGeomFactory->getCoordinateSequenceFactory()->create(coordlist);
		Geometry *g = jtsGeomFactory->createLineString(coords);
		if (g==NULL) return NULL;
		g->setSRID(SRID);
		return g;
	} catch (Throwable *t) {
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return NULL;
	}
}

Geometry *
PostGIS2JTS_polygon(const LWPOLY *lwpoly)
{
	POINTARRAY *pa;
	int SRID = lwpoly->SRID;
	bool is3d = TYPE_HASZ(lwpoly->type);

#ifdef DEBUG_POSTGIS2GEOS
	char buf[256];
	sprintf(buf, "PostGIS2GEOS_poly got lwpoly[%p]", lwpoly);
	NOTICE_MESSAGE(buf);
#endif

	try
	{
		//Coordinate *c;
		uint32 t;
		int ring;
		Geometry *g;
		LinearRing *outerRing;
		LinearRing *innerRing;
		CoordinateSequence *cl;
		POINT3D p;
		
		// make outerRing
		pa = lwpoly->rings[0];
		jobjectArray vc = JvNewObjectArray(pa->npoints,
			&Coordinate::class$, NULL);
#ifdef DEBUG_JARRAY
		std::cout<<"PostGIS2JTS_polygon: vc Jarray: "<<vc;
		if ( is3d ) std::cout<<" (3d)";
		std::cout<<endl;
#endif

		if (is3d)
		{
			for(t=0; t<pa->npoints; t++)
			{
				getPoint3dz_p(pa, t, &p);
				elements(vc)[t] = new Coordinate(p.x,p.y,p.z);
			}
		}
		else
		{
			for(t=0; t<pa->npoints; t++)
			{
				getPoint3dz_p(pa, t, &p);
				elements(vc)[t] = new Coordinate(p.x,p.y);
#ifdef DEBUG_JARRAY
				std::cout<<" elements(vc)["<<t<<"] = "<<elements(vc)[t]<<endl;
#endif
			}
		}

		JArray<Coordinate *>*coordlist = reinterpret_cast<JArray<Coordinate *>*>(vc);
		cl = jtsGeomFactory->getCoordinateSequenceFactory()->create(coordlist);
		coordlist=NULL; vc=NULL;
		outerRing = jtsGeomFactory->createLinearRing(cl);
		cl = NULL;
		if (outerRing == NULL) return NULL;
		outerRing->setSRID(SRID);

		jobjectArray innerRings = JvNewObjectArray(lwpoly->nrings-1,
			&Geometry::class$, NULL);
#ifdef DEBUG_JARRAY
		std::cout<<"PostGIS2JTS_polygon: innerRings Jarray: "<<innerRings<<endl;
#endif
		for(ring=1; ring<lwpoly->nrings; ring++)
		{
			pa = lwpoly->rings[ring];
			vc = JvNewObjectArray(pa->npoints,
				&Coordinate::class$, NULL);
			if (is3d)
			{
				for(t=0; t<pa->npoints; t++)
				{
					getPoint3dz_p(pa, t, &p);
					elements(vc)[t] = new Coordinate(p.x,p.y,p.z);
				}
			}
			else
			{
				for(t=0; t<pa->npoints; t++)
				{
					getPoint3dz_p(pa, t, &p);
					elements(vc)[t] = new Coordinate(p.x,p.y);
				}
			}
			coordlist = reinterpret_cast<JArray<Coordinate *>*>(vc);
			cl = jtsGeomFactory->getCoordinateSequenceFactory()->create(coordlist);
			innerRing = (LinearRing *) jtsGeomFactory->createLinearRing(cl);
			if (innerRing == NULL)
			{
				return NULL;
			}
			innerRing->setSRID(SRID);
			elements(innerRings)[ring-1] = innerRing;
		}

		JArray<LinearRing *>*holesList = reinterpret_cast<JArray<LinearRing *>*>(innerRings);
		g = jtsGeomFactory->createPolygon(outerRing, holesList);
		if (g==NULL) return NULL;
		g->setSRID(SRID);
		return g;
	} catch (Throwable *t) {
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return NULL;
	}
}

Geometry *
PostGIS2JTS_collection(int type, Geometry **geoms, int ngeoms, int SRID, bool is3d)
{
#ifdef DEBUG_POSTGIS2JTS
	char buf[256];
	sprintf(buf, "PostGIS2JTS_collection: requested type %d, ngeoms: %d",
			type, ngeoms);
	NOTICE_MESSAGE(buf);
#endif

	try
	{
		Geometry *g;
		int t;
		jobjectArray subGeoms = JvNewObjectArray(ngeoms,
			&Geometry::class$, NULL);
#ifdef DEBUG_JARRAY
		//std::cout<<"PostGIS2JTS_collection: subGeoms Jarray: "<<subGeoms<<endl;
#endif

		for (t =0; t<ngeoms; t++)
		{
			elements(subGeoms)[t] = geoms[t];
		}

		switch (type)
		{
			case COLLECTIONTYPE:
				g = jtsGeomFactory->createGeometryCollection((JArray<Geometry *>*)subGeoms);
				break;
			case MULTIPOINTTYPE:
				g = jtsGeomFactory->createMultiPoint((JArray<Point *>*)subGeoms);
				break;
			case MULTILINETYPE:
				g = jtsGeomFactory->createMultiLineString((JArray<LineString *>*)subGeoms);
				break;
			case MULTIPOLYGONTYPE:
				g = jtsGeomFactory->createMultiPolygon((JArray<Polygon *>*)subGeoms);
				break;
			default:
				NOTICE_MESSAGE("Unsupported type request for PostGIS2JTS_collection");
				g = NULL;
				
		}
		if (g==NULL)
			return NULL;
		g->setSRID(SRID);
		return g;
	}
	catch (Throwable *t) {
		NOTICE_MESSAGE(StringToChar(t->getMessage()));
		return NULL;
	}
}


