//  g++ postgis_GEOSwrapper.cpp -c -I/usr/local/include  -I/usr/local/include/geos -I/usr/local/src/postgresql-7.2.3//src/include

#include <stdio.h>

#include <string>
#include <iostream>
#include <fstream>

#include "postgis_geos_version.h"
#include "geos/geom.h"
#include "geos/util.h"
#if GEOS_FIRST_INTERFACE <= 3 && GEOS_LAST_INTERFACE >= 3
#include "geos/opValid.h"
#include "geos/opPolygonize.h"
#endif // GEOS_FIRST_INTERFACE <= 3 && GEOS_LAST_INTERFACE >= 3

//#define DEBUG_POSTGIS2GEOS 1
//#define DEBUG 1

using namespace geos;

//WARNING THIS *MUST* BE SET CORRECTLY.
int MAXIMUM_ALIGNOF = -999999;    // to be set during initialization - this will be either 4 (intel) or 8 (sparc)

//for getting things to align properly  double are on 8byte align on solaris machines, and 4bytes on intel

#define TYPEALIGN(ALIGNVAL,LEN) (((long)(LEN) + (ALIGNVAL-1)) & ~(ALIGNVAL-1))
#define MAXALIGN(LEN)           TYPEALIGN(MAXIMUM_ALIGNOF, (LEN))

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

extern "C" char *getPoint(POINTARRAY *pa, int n);

//---- End of definitions found in lwgeom.h

#define	POINTTYPE	1
#define	LINETYPE	2
#define	POLYGONTYPE	3
#define	MULTIPOINTTYPE	4
#define	MULTILINETYPE	5
#define	MULTIPOLYGONTYPE	6
#define	COLLECTIONTYPE	7

//###########################################################

extern "C" char *GEOSrelate(Geometry *g1, Geometry*g2);
extern "C" void initGEOS(int maxalign);


extern "C" void GEOSSetSRID(Geometry *g, int SRID);
extern "C" void GEOSdeleteChar(char *a);
extern "C" void GEOSdeleteGeometry(Geometry *a);
extern "C" char GEOSrelatePattern(Geometry *g1, Geometry*g2,char *pat);
extern "C" char GEOSrelateDisjoint(Geometry *g1, Geometry*g2);
extern "C" char GEOSrelateTouches(Geometry *g1, Geometry*g2);
extern "C" char GEOSrelateIntersects(Geometry *g1, Geometry*g2);
extern "C" char GEOSrelateCrosses(Geometry *g1, Geometry*g2);
extern "C" char GEOSrelateWithin(Geometry *g1, Geometry*g2);
extern "C" char GEOSrelateContains(Geometry *g1, Geometry*g2);
extern "C" char GEOSrelateOverlaps(Geometry *g1, Geometry*g2);
extern "C" Geometry *GEOSpolygonize(Geometry **geoms, unsigned int ngeoms);
extern "C" char *GEOSversion();
extern "C" char *GEOSjtsport();

extern "C" Geometry *PostGIS2GEOS_point(const LWPOINT *point);
extern "C" Geometry *PostGIS2GEOS_linestring(const LWLINE *line);
extern "C" Geometry *PostGIS2GEOS_polygon(const LWPOLY *polygon);
extern "C" Geometry *PostGIS2GEOS_collection(int type, Geometry **geoms, int ngeoms, int SRID, bool is3d);
extern "C" Geometry *PostGIS2GEOS_box3d(BOX3D *box, int SRID);

extern "C" char GEOSisvalid(Geometry *g1);


extern "C" char *GEOSasText(Geometry *g1);
extern "C" char GEOSisEmpty(Geometry *g1);
extern "C" char *GEOSGeometryType(Geometry *g1);
extern "C" int GEOSGeometryTypeId(Geometry *g1);


extern "C" char *throw_exception(Geometry *g);

extern "C" Geometry *GEOSIntersection(Geometry *g1,Geometry *g1);
extern "C" Geometry *GEOSBuffer(Geometry *g1,double width,int quadsegs);
extern "C" Geometry *GEOSConvexHull(Geometry *g1);
extern "C" Geometry *GEOSDifference(Geometry *g1,Geometry *g2);
extern "C" Geometry *GEOSBoundary(Geometry *g1);
extern "C" Geometry *GEOSSymDifference(Geometry *g1,Geometry *g2);
extern "C" Geometry *GEOSUnion(Geometry *g1,Geometry *g2);


extern "C" POINT3D  *GEOSGetCoordinate(Geometry *g1);
extern "C" POINT3D  *GEOSGetCoordinates_Polygon(Polygon *g1);
extern "C" POINT3D  *GEOSGetCoordinates(Geometry *g1);
extern "C" int      GEOSGetNumCoordinate(Geometry *g1);
extern "C" const Geometry *GEOSGetGeometryN(Geometry *g1, int n);
extern "C" const Geometry *GEOSGetExteriorRing(Geometry *g1);
extern "C" const Geometry *GEOSGetInteriorRingN(Geometry *g1, int n);
extern "C" int GEOSGetNumInteriorRings(Geometry *g1);
extern "C" int GEOSGetSRID(Geometry *g1);
extern "C" int GEOSGetNumGeometries(Geometry *g1);

extern "C" char GEOSisSimple(Geometry *g1);
extern "C" char GEOSequals(Geometry *g1, Geometry*g2);
extern "C" char GEOSisRing(Geometry *g1);
extern "C" Geometry *GEOSpointonSurface(Geometry *g1);
extern "C" Geometry *GEOSGetCentroid(Geometry *g1, int *failure);
extern "C" void NOTICE_MESSAGE(char *msg);
extern "C" bool GEOSHasZ(Geometry *g1);

//###########################################################
#if GEOS_LAST_INTERFACE < 2
# define CoordinateSequence CoordinateList
# define DefaultCoordinateSequence BasicCoordinateList
#endif

GeometryFactory *geomFactory = NULL;


void initGEOS (int maxalign)
{
	if (geomFactory == NULL)
	{
		geomFactory = new GeometryFactory( new PrecisionModel(), -1); // NOTE: SRID will have to be changed after geometry creation
		MAXIMUM_ALIGNOF = maxalign;
	}
}

// ------------------------------------------------------------------------------
// geometry constuctors - return NULL if there was an error
//-------------------------------------------------------------------------------


//note: you lose the 3d from this!
Geometry *PostGIS2GEOS_box3d(BOX3D *box, int SRID)
{
	DefaultCoordinateSequence  *cl = new DefaultCoordinateSequence(5);
	try {
		Geometry *g;
		Coordinate c;
		c.x = box->xmin; c.y = box->ymin;
		cl->setAt(c, 0);
		c.x = box->xmin; c.y = box->ymax;
		cl->setAt(c, 1);
		c.x = box->xmax; c.y = box->ymax;
		cl->setAt(c, 2);
		c.x = box->xmax; c.y = box->ymin;
		cl->setAt(c, 3);
		c.x = box->xmin; c.y = box->ymin;
		cl->setAt(c, 4);

		g = geomFactory->createLinearRing(cl);
#if GEOS_LAST_INTERFACE < 2
		delete cl;
#endif
		if (g==NULL) return NULL;
		g->setSRID(SRID);
		return g;
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete cl;
		return NULL;
	}
	catch (...)
	{
		delete cl;
		return NULL;
	}
}

Geometry *PostGIS2GEOS_point(const LWPOINT *lwpoint)
{
	POINT3D *point;
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

	point = (POINT3D *)getPoint(lwpoint->point, 0);
	SRID = lwpoint->SRID;
	is3d = TYPE_HASZ(lwpoint->type);

	try
	{
		Coordinate *c;

		if (is3d)
			c = new Coordinate(point->x, point->y, point->z);
		else
			c = new Coordinate(point->x, point->y);
		Geometry *g = geomFactory->createPoint(*c);
		delete c;
		if (g==NULL)
			return NULL;
		g->setSRID(SRID);
		return g;
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return NULL;
	}

	catch (...)
	{
		return NULL;
	}
}

/*
 * This function must return an all-new allocated object
 */
Geometry *
PostGIS2GEOS_linestring(const LWLINE *lwline)
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
		Coordinate c;
		POINT3D *p;

		//build coordinatelist & pre-allocate space
#if GEOS_LAST_INTERFACE >= 2
		vector<Coordinate> *vc = new vector<Coordinate>(pa->npoints);
#else
		DefaultCoordinateSequence  *coords = new DefaultCoordinateSequence(pa->npoints);
#endif
		if (is3d)
		{
			for (t=0; t<pa->npoints; t++)
			{
				p = (POINT3D *)getPoint(pa, t);
#if GEOS_LAST_INTERFACE >= 2
				(*vc)[t].x = p->x;
				(*vc)[t].y = p->y;
				(*vc)[t].z = p->z;
#else
				c.x = p->x;
				c.y = p->y;
				c.z = p->z;
				coords->setAt(c ,t);
#endif
			}
		}
		else  //make 2d points
		{
			for (t=0; t<pa->npoints; t++)
			{
				p = (POINT3D *)getPoint(pa, t);
#if GEOS_LAST_INTERFACE >= 2
				(*vc)[t].x = p->x;
				(*vc)[t].y = p->y;
				(*vc)[t].z = DoubleNotANumber;
#else
				c.x = p->x;
				c.y = p->y;
				c.z = DoubleNotANumber;
				coords->setAt(c ,t);
#endif
			}
		}
#if GEOS_LAST_INTERFACE >= 2
		CoordinateSequence *coords = DefaultCoordinateSequenceFactory::instance()->create(vc);
#endif
		Geometry *g = geomFactory->createLineString(coords);
#if GEOS_LAST_INTERFACE < 2
		delete coords;
#endif
		if (g==NULL) return NULL;
		g->setSRID(SRID);
		return g;
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return NULL;
	}

	catch (...)
	{
		return NULL;
	}
}

Geometry *PostGIS2GEOS_polygon(const LWPOLY *lwpoly)
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
		Coordinate c;
		uint32 t;
		int ring;
		Geometry *g;
		LinearRing *outerRing;
		LinearRing *innerRing;
		CoordinateSequence *cl;
		POINT3D *p;
		vector<Geometry *> *innerRings;

		// make outerRing
		pa = lwpoly->rings[0];
#if GEOS_LAST_INTERFACE >= 2
		vector<Coordinate> *vc;
		vc = new vector<Coordinate>(pa->npoints);
#else
		cl = new DefaultCoordinateSequence(pa->npoints);
#endif
		if (is3d)
		{
			for(t=0; t<pa->npoints; t++)
			{
				p = (POINT3D *)getPoint(pa, t);
#if GEOS_LAST_INTERFACE >= 2
				(*vc)[t].x = p->x;
				(*vc)[t].y = p->y;
				(*vc)[t].z = p->z;
#else
				c.x = p->x;
				c.y = p->y;
				c.z = p->z;
				cl->setAt( c ,t);
#endif
			}
		}
		else
		{
			for(t=0; t<pa->npoints; t++)
			{
				p = (POINT3D *)getPoint(pa, t);
#if GEOS_LAST_INTERFACE >= 2
				(*vc)[t].x = p->x;
				(*vc)[t].y = p->y;
				(*vc)[t].z = DoubleNotANumber;
#else
				c.x = p->x;
				c.y = p->y;
				c.z = DoubleNotANumber;
				cl->setAt(c ,t);
#endif
			}
		}
#if GEOS_LAST_INTERFACE >= 2
		cl = DefaultCoordinateSequenceFactory::instance()->create(vc);
#endif
		outerRing = (LinearRing*) geomFactory->createLinearRing(cl);
#if GEOS_LAST_INTERFACE < 2
		delete cl;
#endif
		if (outerRing == NULL) return NULL;
		outerRing->setSRID(SRID);

		innerRings=new vector<Geometry *>(lwpoly->nrings-1);
		for(ring=1; ring<lwpoly->nrings; ring++)
		{
			pa = lwpoly->rings[ring];
#if GEOS_LAST_INTERFACE >= 2
			vector<Coordinate> *vc = new vector<Coordinate>(pa->npoints);
#else
			cl = new DefaultCoordinateSequence(pa->npoints);
#endif
			if (is3d)
			{
				for(t=0; t<pa->npoints; t++)
				{
					p = (POINT3D *)getPoint(pa, t);
#if GEOS_LAST_INTERFACE >= 2
					(*vc)[t].x = p->x;
					(*vc)[t].y = p->y;
					(*vc)[t].z = p->z;
#else
					c.x = p->x;
					c.y = p->y;
					c.z = p->z;
					cl->setAt(c ,t);
#endif
				}
			}
			else
			{
				for(t=0; t<pa->npoints; t++)
				{
					p = (POINT3D *)getPoint(pa, t);
#if GEOS_LAST_INTERFACE >= 2
					(*vc)[t].x = p->x;
					(*vc)[t].y = p->y;
					(*vc)[t].z = DoubleNotANumber;
#else
					c.x = p->x;
					c.y = p->y;
					c.z = DoubleNotANumber;
					cl->setAt(c ,t);
#endif
				}
			}
#if GEOS_LAST_INTERFACE >= 2
			CoordinateSequence *cl = DefaultCoordinateSequenceFactory::instance()->create(vc);
#endif
			innerRing = (LinearRing *) geomFactory->createLinearRing(cl);
#if GEOS_LAST_INTERFACE < 2
			delete cl;
#endif
			if (innerRing == NULL)
			{
				delete outerRing;
				return NULL;
			}
			innerRing->setSRID(SRID);
			(*innerRings)[ring-1] = innerRing;
		}

		g = geomFactory->createPolygon(outerRing, innerRings);
		if (g==NULL) return NULL;
		g->setSRID(SRID);
		return g;
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return NULL;
	}

	catch (...)
	{
		return NULL;
	}
}

Geometry *PostGIS2GEOS_multipoint(LWPOINT **geoms, uint32 ngeoms, int SRID, bool is3d)
{
	try
	{
		uint32 t;
		vector<Geometry *> *subGeoms=NULL;
		Geometry *g;

		subGeoms=new vector<Geometry *>(ngeoms);

		for (t=0; t<ngeoms; t++)
		{
			(*subGeoms)[t] = PostGIS2GEOS_point(geoms[t]);
		}
		g = geomFactory->createMultiPoint(subGeoms);
#if GEOS_LAST_INTERFACE < 2
		delete subGeoms;
#endif

		if (g== NULL)
			return NULL;
		g->setSRID(SRID);
		return g;
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return NULL;
	}

	catch (...)
	{
		return NULL;
	}
}

Geometry *PostGIS2GEOS_multilinestring(LWLINE **geoms, uint32 ngeoms, int SRID, bool is3d)
{
	try
	{
		uint32 t;
		vector<Geometry *> *subGeoms=NULL;
		Geometry *g;

		subGeoms=new vector<Geometry *>(ngeoms);

		for (t=0; t<ngeoms; t++)
		{
			(*subGeoms)[t] = PostGIS2GEOS_linestring(geoms[t]);
		}
		g = geomFactory->createMultiLineString(subGeoms);
#if GEOS_LAST_INTERFACE < 2
		delete subGeoms;
#endif

		if (g== NULL)
			return NULL;
		g->setSRID(SRID);
		return g;
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return NULL;
	}

	catch (...)
	{
		return NULL;
	}
}

Geometry *PostGIS2GEOS_multipolygon(LWPOLY **polygons, uint32 npolys, int SRID, bool is3d)
{
	try
	{
		uint32 t;
		vector<Geometry *> *subPolys=NULL;
		Geometry *g;

		subPolys=new vector<Geometry *>(npolys);

		for (t =0; t< npolys; t++)
		{
			(*subPolys)[t] = PostGIS2GEOS_polygon(polygons[t]);
		}
		g = geomFactory->createMultiPolygon(subPolys);
#if GEOS_LAST_INTERFACE < 2
		delete subPolys;
#endif

		if (g== NULL)
			return NULL;
		g->setSRID(SRID);
		return g;
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return NULL;
	}

	catch (...)
	{
		return NULL;
	}
}

Geometry *PostGIS2GEOS_collection(int type, Geometry **geoms, int ngeoms, int SRID, bool is3d)
{
#ifdef DEBUG_POSTGIS2GEOS
	char buf[256];
	sprintf(buf, "PostGIS2GEOS_collection: requested type %d, ngeoms: %d",
			type, ngeoms);
	NOTICE_MESSAGE(buf);
#endif

	try
	{
		Geometry *g;
		int t;
		vector<Geometry *> *subGeos=new vector<Geometry *>(ngeoms);

		for (t =0; t<ngeoms; t++)
		{
			(*subGeos)[t] = geoms[t];
		}
		//g = geomFactory->buildGeometry(subGeos);
		switch (type)
		{
			case COLLECTIONTYPE:
				g = geomFactory->createGeometryCollection(subGeos);
				break;
			case MULTIPOINTTYPE:
				g = geomFactory->createMultiPoint(subGeos);
				break;
			case MULTILINETYPE:
				g = geomFactory->createMultiLineString(subGeos);
				break;
			case MULTIPOLYGONTYPE:
				g = geomFactory->createMultiPolygon(subGeos);
				break;
			default:
				NOTICE_MESSAGE("Unsupported type request for PostGIS2GEOS_collection");
				g = NULL;
				
		}
#if GEOS_LAST_INTERFACE < 2
		delete subGeos;
#endif
		if (g==NULL)
			return NULL;
		g->setSRID(SRID);
		return g;
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return NULL;
	}

	catch (...)
	{
		return NULL;
	}
}


//-----------------------------------------------------------
// relate()-related functions
//  return 0 = false, 1 = true, 2 = error occured
//-----------------------------------------------------------

char GEOSrelateDisjoint(Geometry *g1, Geometry*g2)
{
	try {
		bool result;
		result = g1->disjoint(g2);
		return result;
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return 2;
	}
	catch (...)
	{
		return 2;
	}
}

char GEOSrelateTouches(Geometry *g1, Geometry*g2)
{
	try {
		bool result;
		result =  g1->touches(g2);
		return result;
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return 2;
	}

	catch (...)
	{
		return 2;
	}
}

char GEOSrelateIntersects(Geometry *g1, Geometry*g2)
{
	try {
		bool result;
		result = g1->intersects(g2);
		return result;
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return 2;
	}

	catch (...)
	{
		return 2;
	}
}

char GEOSrelateCrosses(Geometry *g1, Geometry*g2)
{
	try {
		bool result;
		result = g1->crosses(g2);
		return result;
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return 2;
	}

	catch (...)
	{
		return 2;
	}
}

char GEOSrelateWithin(Geometry *g1, Geometry*g2)
{
	try {
		bool result;
		result = g1->within(g2);
		return result;
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return 2;
	}

	catch (...)
	{
		return 2;
	}
}

// call g1->contains(g2)
// returns 0 = false
//         1 = true
//         2 = error was trapped
char GEOSrelateContains(Geometry *g1, Geometry*g2)
{
	try {
		bool result;
		result = g1->contains(g2);
		return result;
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return 2;
	}

	catch (...)
	{
		return 2;
	}
}

char GEOSrelateOverlaps(Geometry *g1, Geometry*g2)
{
	try {
		bool result;
		result = g1->overlaps(g2);
		return result;
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return 2;
	}

	catch (...)
	{
		return 2;
	}
}


//-------------------------------------------------------------------
// low-level relate functions
//------------------------------------------------------------------

char GEOSrelatePattern(Geometry *g1, Geometry*g2,char *pat)
{
	try {
		bool result;
		string s = pat;
		result = g1->relate(g2,pat);
		return result;
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return 2;
	}

	catch (...)
	{
		return 2;
	}
}

char *GEOSrelate(Geometry *g1, Geometry*g2)
{

	try {

		IntersectionMatrix *im = g1->relate(g2);

		string s;
		char *result;
		if (im == NULL)
				return NULL;

		s= im->toString();
		result = (char*) malloc( s.length() + 1);
		strcpy(result, s.c_str() );

		return result;
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return NULL;
	}

	catch (...)
	{
		return NULL;
	}
}



//-----------------------------------------------------------------
// isValid
//-----------------------------------------------------------------


char GEOSisvalid(Geometry *g1)
{
#if GEOS_FIRST_INTERFACE <= 3 && GEOS_LAST_INTERFACE >= 3
	IsValidOp ivo(g1);
#endif
	bool result;
	try {
#if GEOS_FIRST_INTERFACE <= 3 && GEOS_LAST_INTERFACE >= 3
		result = ivo.isValid();
		if ( result == 0 )
		{
			TopologyValidationError *err = ivo.getValidationError();
			if ( err ) {
				string errmsg = err->getMessage();
				NOTICE_MESSAGE((char *)errmsg.c_str());
			}
		}
#else // GEOS_INTERFACE 3 not supported
		result = g1->isValid();
#endif
		return result;
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return 2;
	}

	catch (...)
	{
		return 2;
	}

}


//-----------------------------------------------------------------
// general purpose
//-----------------------------------------------------------------

char GEOSequals(Geometry *g1, Geometry*g2)
{
	try {
		bool result;
		result = g1->equals(g2);
		return result;
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return 2;
	}

	catch (...)
	{
		return 2;
	}
}



char *GEOSasText(Geometry *g1)
{
	try
	{
		string s = g1->toString();


		char *result;
		result = (char*) malloc( s.length() + 1);
		strcpy(result, s.c_str() );
		return result;
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return NULL;
	}

	catch (...)
	{
		return NULL;
	}
}

char GEOSisEmpty(Geometry *g1)
{
	try
	{
		return g1->isEmpty();
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return 2;
	}

	catch (...)
	{
		return 2;
	}
}

char GEOSisSimple(Geometry *g1)
{
	try
	{
		return g1->isSimple();
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return 2;
	}

	catch (...)
	{
		return 2;
	}
}

char GEOSisRing(Geometry *g1)
{
	try
	{
		return (( (LinearRing*)g1)->isRing());
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return 2;
	}

	catch (...)
	{
		return 2;
	}
}



//free the result of this
char *GEOSGeometryType(Geometry *g1)
{
	try
	{
		string s = g1->getGeometryType();


		char *result;
		result = (char*) malloc( s.length() + 1);
		strcpy(result, s.c_str() );
		return result;
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return NULL;
	}

	catch (...)
	{
		return NULL;
	}
}

// Return postgis geometry type index
int GEOSGeometryTypeId(Geometry *g1)
{
	try
	{
		GeometryTypeId id = g1->getGeometryTypeId();
		switch (id)
		{
			case GEOS_POINT:
				return POINTTYPE;
			case GEOS_LINESTRING:
			case GEOS_LINEARRING:
				return LINETYPE;
			case GEOS_POLYGON:
				return POLYGONTYPE;
			case GEOS_MULTIPOINT:
				return MULTIPOINTTYPE;
			case GEOS_MULTILINESTRING:
				return MULTILINETYPE;
			case GEOS_MULTIPOLYGON:
				return MULTIPOLYGONTYPE;
			case GEOS_GEOMETRYCOLLECTION:
				return COLLECTIONTYPE;
			default:
				return 0;
		}
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return -1;
	}

	catch (...)
	{
		return -1;
	}
}




//-------------------------------------------------------------------
// GEOS functions that return geometries
//-------------------------------------------------------------------

Geometry *GEOSIntersection(Geometry *g1,Geometry *g2)
{
	try
	{
		Geometry *g3 = g1->intersection(g2);
		return g3;
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return NULL;
	}

	catch (...)
	{
		return NULL;
	}
}

Geometry *
GEOSBuffer(Geometry *g1, double width, int quadrantsegments)
{
	try
	{
		Geometry *g3 = g1->buffer(width, quadrantsegments);
		return g3;
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return NULL;
	}

	catch (...)
	{
		return NULL;
	}
}

Geometry *GEOSConvexHull(Geometry *g1)
{
	try
	{
		Geometry *g3 = g1->convexHull();
		return g3;
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return NULL;
	}

	catch (...)
	{
		return NULL;
	}
}

Geometry *GEOSDifference(Geometry *g1,Geometry *g2)
{
	try
	{
		Geometry *g3 = g1->difference(g2);
		return g3;
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return NULL;
	}

	catch (...)
	{
		return NULL;
	}
}

Geometry *GEOSBoundary(Geometry *g1)
{
	try
	{
		Geometry *g3 = g1->getBoundary();
		return g3;
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return NULL;
	}

	catch (...)
	{
		return NULL;
	}
}

Geometry *GEOSSymDifference(Geometry *g1,Geometry *g2)
{
	try
	{
		Geometry *g3 = g1->symDifference(g2);
		return g3;
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return NULL;
	}

	catch (...)
	{
		return NULL;
	}
}

Geometry *GEOSUnion(Geometry *g1,Geometry *g2)
{
	try
	{
		Geometry *g3 = g1->Union(g2);
		return g3;
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return NULL;
	}
	catch (...)
	{
		return NULL;
	}
}


Geometry *GEOSpointonSurface(Geometry *g1)
{
	try
	{
		Geometry *g3 = g1->getInteriorPoint();
		return g3;
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return NULL;
	}

	catch (...)
	{
		return NULL;
	}
}





//-------------------------------------------------------------------
// memory management functions
//------------------------------------------------------------------


//BUG:: this leaks memory, but delete kills the PrecisionModel for ALL the geometries
void GEOSdeleteGeometry(Geometry *a)
{
	try{
		delete a;
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		//return NULL;
	}

	catch(...)
	{
		// do nothing!
	}
}

void
GEOSSetSRID(Geometry *g, int SRID)
{
	g->setSRID(SRID);
}

void GEOSdeleteChar(char *a)
{
	try{
	   free(a);
	}
	catch (GEOSException *ge) // ???
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		//return NULL;
	}

	catch(...)
	{
		// do nothing!
	}
}


//-------------------------------------------------------------------
//GEOS => POSTGIS conversions
//-------------------------------------------------------------------


// free the result when done!
// g1 must be a point
POINT3D  *GEOSGetCoordinate(Geometry *g1)
{
	try{
		POINT3D		*result = (POINT3D*) malloc (sizeof(POINT3D));
		const Coordinate *c =g1->getCoordinate();

		result->x = c->x;
		result->y = c->y;
		result->z = c->z;
		return result;
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return NULL;
	}

	catch(...)
	{
		return NULL;
	}

}


//must free the result when done
// result is an array length g1->getNumCoordinates()
POINT3D  *GEOSGetCoordinates(Geometry *g1)
{
	if ( g1->getGeometryTypeId() == GEOS_POLYGON )
	{
		return GEOSGetCoordinates_Polygon((Polygon *)g1);
	}

	try {
		int numPoints = g1->getNumPoints();
		POINT3D *result = (POINT3D*) malloc (sizeof(POINT3D) * numPoints );
		int t;
		CoordinateSequence *cl = g1->getCoordinates();
		Coordinate		c;

		for (t=0;t<numPoints;t++)
		{
			c =cl->getAt(t);

			result[t].x = c.x;
			result[t].y = c.y;
			result[t].z = c.z;
		}

		delete cl;
		return result;
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return NULL;
	}

	catch(...)
	{
		return NULL;
	}

}

// A somewhat optimized version for polygon types.
POINT3D  *GEOSGetCoordinates_Polygon(Polygon *g1)
{
	try {
		int t, r, outidx=0;
		const CoordinateSequence *cl;
		Coordinate c;
		const LineString *lr;
		int npts, nrings;
		POINT3D *result;
		
		npts = g1->getNumPoints();
		result = (POINT3D*) malloc (sizeof(POINT3D) * npts);
		
		// Exterior ring 
		lr = g1->getExteriorRing();
		cl = lr->getCoordinatesRO();
		npts = lr->getNumPoints();
		for (t=0; t<npts; t++)
		{
			c = cl->getAt(t);

			result[outidx].x = c.x;
			result[outidx].y = c.y;
			result[outidx].z = c.z;
			outidx++;
		}

		// Interior rings
		nrings = g1->getNumInteriorRing();
		for (r=0; r<nrings; r++)
		{
			lr = g1->getInteriorRingN(r);
			cl = lr->getCoordinatesRO();
			npts = lr->getNumPoints();
			for (t=0; t<npts; t++)
			{
				c = cl->getAt(t);
				result[outidx].x = c.x;
				result[outidx].y = c.y;
				result[outidx].z = c.z;
				outidx++;
			}
		}

		return result;
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return NULL;
	}

	catch(...)
	{
		return NULL;
	}

}




int      GEOSGetNumCoordinate(Geometry *g1)
{
	try{
		return g1->getNumPoints();
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return 0;
	}

	catch(...)
	{
		return 0;
	}
}

int      GEOSGetNumInteriorRings(Geometry *g1)
{
	try{
		Polygon *p = (Polygon *) g1;
		return p->getNumInteriorRing();
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return 0;
	}

	catch(...)
	{
		return 0;
	}
}


//only call on GCs (or multi*)
int      GEOSGetNumGeometries(Geometry *g1)
{
	try{
		GeometryCollection *gc = (GeometryCollection *) g1;
		return gc->getNumGeometries();
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return 0;
	}

	catch(...)
	{
		return 0;
	}
}


//call only on GEOMETRYCOLLECTION or MULTI*
const Geometry *GEOSGetGeometryN(Geometry *g1, int n)
{
	try{
		const GeometryCollection *gc = (GeometryCollection *) g1;
		return gc->getGeometryN(n);
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return NULL;
	}

	catch(...)
	{
		return NULL;
	}
}


//call only on polygon
const Geometry *GEOSGetExteriorRing(Geometry *g1)
{
	try{
		Polygon *p = (Polygon *) g1;
		return p->getExteriorRing();
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return 0;
	}

	catch(...)
	{
		return 0;
	}
}

//call only on polygon
const Geometry *GEOSGetInteriorRingN(Geometry *g1,int n)
{
	try{
		Polygon *p = (Polygon *) g1;
		return p->getInteriorRingN(n);
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return NULL;
	}

	catch(...)
	{
		return NULL;
	}
}

Geometry *GEOSGetCentroid(Geometry *g, int *failure)
{
	try{
		Geometry *ret = g->getCentroid();
		*failure = 0;
		return ret;
	}
	catch (GEOSException *ge)
	{
		*failure = 1;
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return NULL;
	}
	catch(...)
	{
		*failure = 1;
		return NULL;
	}
}

#if GEOS_FIRST_INTERFACE <= 3 && GEOS_LAST_INTERFACE >= 3
Geometry *GEOSpolygonize(Geometry **g, unsigned int ngeoms)
{
	unsigned int i;
	Geometry *multipoly = NULL;

	// construct vector
	vector<Geometry *> *geoms = new vector<Geometry *>(ngeoms);
	for (i=0; i<ngeoms; i++) (*geoms)[i] = g[i];

#if DEBUG
	NOTICE_MESSAGE("geometry vector constructed");
#endif

	try{
		// Polygonize
		Polygonizer plgnzr;
		plgnzr.add(geoms);
#if DEBUG
	NOTICE_MESSAGE("geometry vector added to polygonizer");
#endif

		vector<Polygon *>*polys = plgnzr.getPolygons();

#if DEBUG
	NOTICE_MESSAGE("output polygons got");
#endif

		delete geoms;

#if DEBUG
	NOTICE_MESSAGE("geometry vector deleted");
#endif

		geoms = new vector<Geometry *>(polys->size());
		for (i=0; i<polys->size(); i++) (*geoms)[i] = (*polys)[i];
		multipoly = geomFactory->createMultiPolygon(geoms);
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return NULL;
	}
	catch(...)
	{
		return NULL;
	}

	return multipoly;
}
#else // ! (GEOS_FIRST_INTERFACE <= 3 && GEOS_LAST_INTERFACE >= 3)
Geometry *GEOSpolygonize(Geometry **g, unsigned int ngeoms)
{
	NOTICE_MESSAGE("GEOS library does not support required interface 3");
	return NULL;
}
#endif // ! (GEOS_FIRST_INTERFACE <= 3 && GEOS_LAST_INTERFACE >= 3)


int      GEOSGetSRID(Geometry *g1)
{
	try{
		return g1->getSRID();
	}
	catch (GEOSException *ge)
	{
		NOTICE_MESSAGE((char *)ge->toString().c_str());
		delete ge;
		return 0;
	}

	catch(...)
	{
		return 0;
	}
}

char *
GEOSversion()
{
#if GEOS_LAST_INTERFACE < 2
	/*
	 * GEOS upgrade needs postgis re-build, so this static
	 * assignment is not going to be a problem
	 */
	char *res = strdup("1.0.0");
#else
	string version = geosversion();
	char *res = strdup(version.c_str());
#endif
	return res;
}

char *
GEOSjtsport()
{
#if GEOS_LAST_INTERFACE < 2
	/*
	 * GEOS upgrade needs postgis re-build, so this static
	 * assignment is not going to be a problem
	 */
	char *res = strdup("1.3");
#else
	string version = jtsport();
	char *res = strdup(version.c_str());
#endif
	return res;
}


bool 
GEOSHasZ(Geometry *g)
{
	//char msg[256];
	double az = g->getCoordinate()->z;
	//sprintf(msg, "ZCoord: %g", az);
	//NOTICE_MESSAGE(msg);
	return (finite(az) && az != DoubleNotANumber);
}
