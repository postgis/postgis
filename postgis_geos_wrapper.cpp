//  g++ postgis_GEOSwrapper.cpp -c -I/usr/local/include  -I/usr/local/include/geos -I/usr/local/src/postgresql-7.2.3//src/include


#include <stdio.h>

#include <string>
#include <iostream>
#include <fstream>

//#include "geom.h"
//#include "util.h"
//#include "graph.h"
#include "io.h"
//#include "opRelate.h"



//WARNING THIS *MUST* BE SET CORRECTLY.
int MAXIMUM_ALIGNOF = -999999;    // to be set during initialization - this will be either 4 (intel) or 8 (sparc)

//for getting things to align properly  double are on 8byte align on solaris machines, and 4bytes on intel

#define TYPEALIGN(ALIGNVAL,LEN) (((long)(LEN) + (ALIGNVAL-1)) & ~(ALIGNVAL-1))
#define MAXALIGN(LEN)           TYPEALIGN(MAXIMUM_ALIGNOF, (LEN))

typedef  int int32;

typedef struct
{
	double		x,y,z;  //for lat/long   x=long, y=lat
} POINT3D;

typedef struct
{
	POINT3D		LLB,URT; /* corner POINT3Ds on long diagonal */
} BOX3D;

typedef struct
{
	int32 	npoints; // how many points in the line
	int32 	junk;	   // double-word alignment
	POINT3D  	points[1]; // array of actual points
} LINE3D;


typedef struct
{
	int32 	nrings;	 // how many rings in this polygon
	int32		npoints[1]; //how many points in each ring
	/* could be 4 byes of filler here to make sure points[] is
         double-word aligned*/
	POINT3D  	points[1]; // array of actual points
} POLYGON3D;

typedef struct
{
	int32		size;		// postgres variable-length type requirement
	int32		SRID;		// spatial reference system id
	double	offsetX;	// for precision grid (future improvement)
	double	offsetY;	// for precision grid (future improvement)
	double	scale;	// for precision grid (future improvement)
	int32		type;		// this type of geometry
	bool		is3d;		// true if the points are 3d (only for output)
	BOX3D		bvol;		// bounding volume of all the geo objects
	int32		nobjs;	// how many sub-objects in this object
	int32		objType[1];	// type of object
	int32		objOffset[1];// offset (in bytes) into this structure where
					 // the object is located
	char		objData[1];  // store for actual objects

} GEOMETRY;


//###########################################################

extern "C" char *GEOSrelate(Geometry *g1, Geometry*g2);
extern "C" void initGEOS(int maxalign);


extern "C" void GEOSdeleteChar(char *a);
extern "C" void GEOSdeleteGeometry(Geometry *a);
extern "C" bool GEOSrelatePattern(Geometry *g1, Geometry*g2,char *pat);
extern "C" bool GEOSrelateDisjoint(Geometry *g1, Geometry*g2);
extern "C" bool GEOSrelateTouches(Geometry *g1, Geometry*g2);
extern "C" bool GEOSrelateIntersects(Geometry *g1, Geometry*g2);
extern "C" bool GEOSrelateCrosses(Geometry *g1, Geometry*g2);
extern "C" bool GEOSrelateWithin(Geometry *g1, Geometry*g2);
extern "C" bool GEOSrelateContains(Geometry *g1, Geometry*g2);
extern "C" bool GEOSrelateOverlaps(Geometry *g1, Geometry*g2);


extern "C" Geometry *PostGIS2GEOS_point(POINT3D *point,int SRID, bool is3d);
extern "C" Geometry *PostGIS2GEOS_linestring(LINE3D *line,int SRID, bool is3d);
extern "C" Geometry *PostGIS2GEOS_polygon(POLYGON3D *polygon,int SRID, bool is3d);
extern "C" Geometry *PostGIS2GEOS_multipolygon(POLYGON3D **polygons,int npolys, int SRID, bool is3d);
extern "C" Geometry *PostGIS2GEOS_multilinestring(LINE3D **lines,int nlines, int SRID, bool is3d);
extern "C" Geometry *PostGIS2GEOS_multipoint(POINT3D **points,int npoints, int SRID, bool is3d);

extern "C" Geometry *PostGIS2GEOS_box3d(BOX3D *box, int SRID);
extern "C" Geometry *PostGIS2GEOS_collection(Geometry **geoms, int ngeoms,int SRID, bool is3d);

extern "C" bool GEOSisvalid(Geometry *g1);


//###########################################################

GeometryFactory *geomFactory = NULL;

void initGEOS (int maxalign)
{
	if (geomFactory == NULL)
	{
		geomFactory = new GeometryFactory( new PrecisionModel(), -1); // NOTE: SRID will have to be changed after geometry creation
		MAXIMUM_ALIGNOF = maxalign;
	}
}

		//note: you lose the 3d from this!
Geometry *PostGIS2GEOS_box3d(BOX3D *box, int SRID)
{
	Geometry *g;
	Envelope *envelope = new Envelope(box->LLB.x,box->URT.x ,box->LLB.y,box->URT.y);
	g = geomFactory->toGeometry(envelope,geomFactory->getPrecisionModel(), SRID);
	delete envelope;
	return g;
}

Geometry *PostGIS2GEOS_collection(Geometry **geoms, int ngeoms,int SRID, bool is3d)
{
	Geometry *g;
	int t;
	vector<Geometry *> *subGeos=new vector<Geometry *>;

	for (t =0; t< ngeoms; t++)
	{
		subGeos->push_back(geoms[t]);	
	}
	g = geomFactory->buildGeometry(subGeos);
	g->setSRID(SRID);
	return g;

}

Geometry *PostGIS2GEOS_point(POINT3D *point,int SRID, bool is3d)
{
	Coordinate *c;

	if (is3d)
		c = new Coordinate(point->x, point->y);	
	else
		c = new Coordinate(point->x, point->y, point->z);
	Geometry *g = geomFactory->createPoint(*c);
	g->setSRID(SRID);
	return g;
}


Geometry *PostGIS2GEOS_linestring(LINE3D *line,int SRID, bool is3d)
{
	int t;
	Coordinate c;

	//build coordinatelist & pre-allocate space
	BasicCoordinateList  *coords = new BasicCoordinateList(line->npoints);
	if (is3d)
	{
		for (t=0;t<line->npoints;t++)
		{
			c.x = line->points[t].x;
			c.y = line->points[t].y;
			c.z = line->points[t].z;
			coords->setAt( c ,t);
		}

	}
	else  //make 2d points
	{
		for (t=0;t<line->npoints;t++)
		{
			c.x = line->points[t].x;
			c.y = line->points[t].y;
			c.z = DoubleNotANumber;
			coords->setAt( c ,t);
		}

	}
	Geometry *g = geomFactory->createLineString(coords);
	g->setSRID(SRID);
	return g;
}

	//polygons is an array of pointers to polygons
Geometry *PostGIS2GEOS_multipolygon(POLYGON3D **polygons,int npolys, int SRID, bool is3d)
{
	int t;
	vector<Geometry *> *subPolys=new vector<Geometry *>;
	Geometry *g;

	for (t =0; t< npolys; t++)
	{
		subPolys->push_back(PostGIS2GEOS_polygon(polygons[t], SRID,is3d ));	
	}
	g = geomFactory->createMultiPolygon(subPolys);
	g->setSRID(SRID);
	return g;
}

	//lines is an array of pointers to line3d
Geometry *PostGIS2GEOS_multilinestring(LINE3D **lines,int nlines, int SRID, bool is3d)
{
	int t;
	vector<Geometry *> *subLines =new vector<Geometry *>;
	Geometry *g;

	for (t =0; t< nlines; t++)
	{
		subLines->push_back(PostGIS2GEOS_linestring(lines[t], SRID,is3d ));	
	}
	g = geomFactory->createMultiLineString(subLines);
	g->setSRID(SRID);
	return g;
}

Geometry *PostGIS2GEOS_multipoint(POINT3D **points,int npoints, int SRID, bool is3d)
{
	int t;
	vector<Geometry *> *subPoints =new vector<Geometry *>;
	Geometry *g;

	for (t =0; t< npoints; t++)
	{
		subPoints->push_back(PostGIS2GEOS_point(points[t], SRID,is3d ));	
	}
	g = geomFactory->createMultiPoint(subPoints);
	g->setSRID(SRID);
	return g;

}


Geometry *PostGIS2GEOS_polygon(POLYGON3D *polygon,int SRID, bool is3d)
{
		POINT3D *pts;
		Coordinate c;
		int  ring,t;
		Geometry *g;
		LinearRing *outerRing;
		LinearRing *innerRing;
		BasicCoordinateList *cl;
		int pointOffset =0; // first point that we're looking at.  a POLYGON3D has all its points smooshed together
		vector<Geometry *> *innerRings=new vector<Geometry *>;


		pts = (POINT3D *) ( (char *)&(polygon->npoints[polygon->nrings] )  );
		pts = (POINT3D *) MAXALIGN(pts);
	
			// make outerRing
			cl = new BasicCoordinateList(polygon->npoints[0]);
			if (is3d)
			{
				for(t=0;t<polygon->npoints[0];t++)
				{
						c.x = pts[t].x;
						c.y = pts[t].y;
						c.z = pts[t].z;
						cl->setAt( c ,t);
				}
			}
			else
			{
				for(t=0;t<polygon->npoints[0];t++)
				{
						c.x = pts[t].x;
						c.y = pts[t].y;
						c.z = DoubleNotANumber;
						cl->setAt( c ,t);
				}
			}
			outerRing = geomFactory->createLinearRing(cl);
			outerRing->setSRID(SRID);
			pointOffset = polygon->npoints[0];

		for(ring =1; ring< polygon->nrings; ring++)
		{
			cl = new BasicCoordinateList(polygon->npoints[ring]);
			if (is3d)
			{
				for(t=0;t<polygon->npoints[ring];t++)
				{
						c.x = pts[t+pointOffset].x;
						c.y = pts[t+pointOffset].y;
						c.z = pts[t+pointOffset].z;
						cl->setAt( c ,t);
				}
			}
			else
			{
				for(t=0;t<polygon->npoints[ring];t++)
				{
						c.x = pts[t+pointOffset].x;
						c.y = pts[t+pointOffset].y;
						c.z = DoubleNotANumber;
						cl->setAt( c ,t);
				}
			}
			innerRing = geomFactory->createLinearRing(cl);
			innerRing->setSRID(SRID);
			innerRings->push_back(innerRing);
			pointOffset += polygon->npoints[ring];
		}

	g = geomFactory->createPolygon(outerRing, innerRings);
	g->setSRID(SRID);
	return g;
}



bool GEOSrelateDisjoint(Geometry *g1, Geometry*g2)
{
	return g1->disjoint(g2);
}

bool GEOSrelateTouches(Geometry *g1, Geometry*g2)
{
	return g1->touches(g2);
}

bool GEOSrelateIntersects(Geometry *g1, Geometry*g2)
{
	return g1->intersects(g2);
}

bool GEOSrelateCrosses(Geometry *g1, Geometry*g2)
{
	return g1->crosses(g2);
}

bool GEOSrelateWithin(Geometry *g1, Geometry*g2)
{
	return g1->within(g2);
}

bool GEOSrelateContains(Geometry *g1, Geometry*g2)
{
	return g1->contains(g2);
}

bool GEOSrelateOverlaps(Geometry *g1, Geometry*g2)
{
	return g1->overlaps(g2);
}

//BUG:: this leaks memory, but delete kills the PrecisionModel for ALL the geometries
void GEOSdeleteGeometry(Geometry *a)
{
//	delete a;
}

void GEOSdeleteChar(char *a)
{
	free(a);
}


bool GEOSrelatePattern(Geometry *g1, Geometry*g2,char *pat)
{
	string s = pat;
	return g1->relate(g2,pat);
}


char *GEOSrelate(Geometry *g1, Geometry*g2)
{
	IntersectionMatrix *im = g1->relate(g2);
	string s;
	char *result;

	s= im->toString();
	result = (char*) malloc( s.length() + 1);
	strcpy(result, s.c_str() );


	return result;
}

bool GEOSisvalid(Geometry *g1)
{
	return g1->isValid();
}




char *GEOSasText(Geometry *g1)
{
	string s = g1->toString();

	return (char *) s.c_str() ;
}

