#ifndef _LIBLWGEOM_H
#define _LIBLWGEOM_H 1

#include <stdio.h>

#define INTEGRITY_CHECKS 1
//#define DEBUG_ALLOCS 1
//#define DEBUG 1
//#define DEBUG_CALLS 1

typedef void* (*lwallocator)(size_t size);
typedef void* (*lwreallocator)(void *mem, size_t size);
typedef void (*lwfreeor)(void* mem);
typedef void (*lwreporter)(const char* fmt, ...);

#ifndef C_H
typedef unsigned int uint32;
typedef int int32;
#endif

/* prototypes */
void *default_allocator(size_t size);
void *default_reallocator(void *mem, size_t size);
void default_freeor(void *ptr);
void default_errorreporter(const char *fmt, ...);
void default_noticereporter(const char *fmt, ...);

/* globals */
extern lwreallocator lwrealloc_var;
extern lwallocator lwalloc_var;
extern lwfreeor lwfree_var;
extern lwreporter lwerror;
extern lwreporter lwnotice;

//-----------------------------------------------------------------

typedef struct
{
	float xmin;
	float ymin;
	float xmax;
	float ymax;
} BOX2DFLOAT4;

typedef struct
{
        double xmin, ymin, zmin;
        double xmax, ymax, zmax;
} BOX3D;

typedef struct chiptag
{
	int size; //unused (for use by postgresql)

	int endian_hint;  // the number 1 in the endian of this datastruct

	BOX3D bvol;
	int SRID;
	char future[4];
	float factor;	// Usually 1.0.
			// Integer values are multiplied by this number
			// to get the actual height value
			// (for sub-meter accuracy height data).

	int datatype;	// 1 = float32,
			// 5 = 24bit integer,
			// 6 = 16bit integer (short)
			// 101 = float32 (NDR),
			// 105 = 24bit integer (NDR),
			// 106=16bit int (NDR)

	int height;
	int width;
	int compression;	// 0 = no compression, 1 = differencer
				// 0x80 = new value
				// 0x7F = nodata

	// this is provided for convenience, it should be set to
	//  sizeof(chip) bytes into the struct because the serialized form is:
	//    <header><data>
	// NULL when serialized
	void  *data;	// data[0] = bottm left,
			// data[width] = 1st pixel, 2nd row (uncompressed)

} CHIP;

/*
 * standard definition of an ellipsoid (what wkt calls a spheroid)
 *    f = (a-b)/a
 *    e_sq = (a*a - b*b)/(a*a)
 *    b = a - fa
 */
typedef struct
{
	double	a;	//semimajor axis
	double	b; 	//semiminor axis
	double	f;	//flattening
	double	e;	//eccentricity (first)
	double	e_sq;	//eccentricity (first), squared
	char		name[20]; //name of ellipse
} SPHEROID;


// ALL LWGEOM structures will use POINT3D as an abstract point.
// This means a 2d geometry will be stored as (x,y) in its serialized
// form, but all functions will work on (x,y,0).  This keeps all the
// analysis functions simple.
// NOTE: for GEOS integration, we'll probably set z=NaN
//        so look out - z might be NaN for 2d geometries!
typedef struct { double	x,y,z; } POINT3DZ;
typedef struct { double	x,y,z; } POINT3D; // alias for POINT3DZ
typedef struct { double	x,y,m; } POINT3DM;


// type for 2d points.  When you convert this to 3d, the
//   z component will be either 0 or NaN.
typedef struct
{
	 double x;
	 double y;
} POINT2D;

typedef struct
{
	 double x;
	 double y;
	 double z;
	 double m;
} POINT4D;

//-----------------------------------------------------------

// Point array abstracts a lot of the complexity of points and point lists.
// It handles miss-alignment in the serialized form, 2d/3d translation
//    (2d points converted to 3d will have z=0 or NaN)
// DONT MIX 2D and 3D POINTS!  *EVERYTHING* is either one or the other
typedef struct
{
    char  *serialized_pointlist; // array of POINT 2D, 3D or 4D.
    				 // probably missaligned.
    				 // points to a double
    unsigned char  dims; 	 // use TYPE_* macros to handle
    uint32 npoints;
}  POINTARRAY;

//-----------------------------------------------------------

// LWGEOM (any type)
typedef struct
{
	unsigned char type; 
	BOX2DFLOAT4 *bbox;
	uint32 SRID; // -1 == unneeded
	void *data;
} LWGEOM;

// POINTYPE
typedef struct
{
	unsigned char type; // POINTTYPE
	BOX2DFLOAT4 *bbox;
   	uint32 SRID;	
   	POINTARRAY *point;  // hide 2d/3d (this will be an array of 1 point)
}  LWPOINT; // "light-weight point"

// LINETYPE
typedef struct
{
	unsigned char type; // LINETYPE
	BOX2DFLOAT4 *bbox;
   	uint32 SRID;	
	POINTARRAY    *points; // array of POINT3D
} LWLINE; //"light-weight line"

// POLYGONTYPE
typedef struct
{
	unsigned char type; // POLYGONTYPE
	BOX2DFLOAT4 *bbox;
   	uint32 SRID;	
	int  nrings;
	POINTARRAY **rings; // list of rings (list of points)
} LWPOLY; // "light-weight polygon"

// MULTIPOINTTYPE
typedef struct
{
	unsigned char type; 
	BOX2DFLOAT4 *bbox;
   	uint32 SRID;	
	int  ngeoms;
	LWPOINT **geoms;
} LWMPOINT; 

// MULTILINETYPE
typedef struct
{  
	unsigned char type; 
	BOX2DFLOAT4 *bbox;
   	uint32 SRID;	
	int  ngeoms;
	LWLINE **geoms;
} LWMLINE; 

// MULTIPOLYGONTYPE
typedef struct
{  
	unsigned char type; 
	BOX2DFLOAT4 *bbox;
   	uint32 SRID;	
	int  ngeoms;
	LWPOLY **geoms;
} LWMPOLY; 

// COLLECTIONTYPE
typedef struct
{   
	unsigned char type; 
	BOX2DFLOAT4 *bbox;
   	uint32 SRID;	
	int  ngeoms;
	LWGEOM **geoms;
} LWCOLLECTION; 

// Casts LWGEOM->LW* (return NULL if cast is illegal)
extern LWMPOLY *lwgeom_as_lwmpoly(LWGEOM *lwgeom);
extern LWMLINE *lwgeom_as_lwmline(LWGEOM *lwgeom);
extern LWMPOINT *lwgeom_as_lwmpoint(LWGEOM *lwgeom);
extern LWCOLLECTION *lwgeom_as_lwcollection(LWGEOM *lwgeom);
extern LWPOLY *lwgeom_as_lwpoly(LWGEOM *lwgeom);
extern LWLINE *lwgeom_as_lwline(LWGEOM *lwgeom);
extern LWPOINT *lwgeom_as_lwpoint(LWGEOM *lwgeom);

// Casts LW*->LWGEOM (always cast)
extern LWGEOM *lwmpoly_as_lwgeom(LWMPOLY *obj);
extern LWGEOM *lwmline_as_lwgeom(LWMLINE *obj);
extern LWGEOM *lwmpoint_as_lwgeom(LWMPOINT *obj);
extern LWGEOM *lwcollection_as_lwgeom(LWCOLLECTION *obj);
extern LWGEOM *lwpoly_as_lwgeom(LWPOLY *obj);
extern LWGEOM *lwline_as_lwgeom(LWLINE *obj);
extern LWGEOM *lwpoint_as_lwgeom(LWPOINT *obj);

// Call this function everytime LWGEOM coordinates 
// change so to invalidate bounding box
extern void lwgeom_changed(LWGEOM *lwgeom);

// Call this function to drop BBOX and SRID
// from LWGEOM. If LWGEOM type is *not* flagged
// with the HASBBOX flag and has a bbox, it
// will be released.
extern void lwgeom_dropBBOX(LWGEOM *lwgeom);
// Compute a bbox if not already computed
extern void lwgeom_addBBOX(LWGEOM *lwgeom);
extern void lwgeom_dropSRID(LWGEOM *lwgeom);

//-------------------------------------------------------------

// copies a point from the point array into the parameter point
// will set point's z=0 (or NaN) if pa is 2d
// will set point's m=0 (or NaN( if pa is 3d or 2d
// NOTE: point is a real POINT3D *not* a pointer
extern POINT4D getPoint4d(const POINTARRAY *pa, int n);

// copies a point from the point array into the parameter point
// will set point's z=0 (or NaN) if pa is 2d
// will set point's m=0 (or NaN) if pa is 3d or 2d
// NOTE: this will modify the point4d pointed to by 'point'.
extern int getPoint4d_p(const POINTARRAY *pa, int n, POINT4D *point);

// copies a point from the point array into the parameter point
// will set point's z=0 (or NaN) if pa is 2d
// NOTE: point is a real POINT3D *not* a pointer
extern POINT3DZ getPoint3dz(const POINTARRAY *pa, int n);
extern POINT3DM getPoint3dm(const POINTARRAY *pa, int n);

// copies a point from the point array into the parameter point
// will set point's z=0 (or NaN) if pa is 2d
// NOTE: this will modify the point3d pointed to by 'point'.
extern int getPoint3dz_p(const POINTARRAY *pa, int n, POINT3DZ *point);
extern int getPoint3dm_p(const POINTARRAY *pa, int n, POINT3DM *point);


// copies a point from the point array into the parameter point
// z value (if present is not returned)
// NOTE: point is a real POINT3D *not* a pointer
extern POINT2D getPoint2d(const POINTARRAY *pa, int n);

// copies a point from the point array into the parameter point
// z value (if present is not returned)
// NOTE: this will modify the point2d pointed to by 'point'.
extern int getPoint2d_p(const POINTARRAY *pa, int n, POINT2D *point);

// get a pointer to nth point of a POINTARRAY
// You'll need to cast it to appropriate dimensioned point.
// Note that if you cast to a higher dimensional point you'll
// possibly corrupt the POINTARRAY.
extern char *getPoint(const POINTARRAY *pa, int n);
//--- here is a macro equivalent, for speed...
//#define getPoint(x,n) &( (x)->serialized_pointlist[((x)->ndims*8)*(n)] )


// constructs a POINTARRAY.
// NOTE: points is *not* copied, so be careful about modification (can be aligned/missaligned)
// NOTE: hasz and hasm are descriptive - it describes what type of data
//	 'points' points to.  No data conversion is done.
extern POINTARRAY *pointArray_construct(char *points, char hasz, char hasm, uint32 npoints);

//calculate the bounding box of a set of points
// returns a 3d box
// if pa is 2d, then box3d's zmin/zmax will be either 0 or NaN
// dont call on an empty pa
extern BOX3D *pointArray_bbox(const POINTARRAY *pa);

//size of point represeneted in the POINTARRAY
// 16 for 2d, 24 for 3d, 32 for 4d
extern int pointArray_ptsize(const POINTARRAY *pa);


/*
 *
 * LWGEOM types are an 8-bit char in this format:
 *
 * BSZMtttt
 *
 * WHERE
 *    B = 16 byte BOX2DFLOAT4 follows (probably not aligned) [before SRID]
 *    S = 4 byte SRID attached (0= not attached (-1), 1= attached)
 *    ZM = dimentionality (hasZ, hasM)
 *    tttt = actual type (as per the WKB type):
 *
 *    enum wkbGeometryType {
 *        wkbPoint = 1,
 *        wkbLineString = 2,
 *        wkbPolygon = 3,
 *        wkbMultiPoint = 4,
 *        wkbMultiLineString = 5,
 *        wkbMultiPolygon = 6,
 *        wkbGeometryCollection = 7
 *    };
 *
 */
#define	POINTTYPE	1
#define	LINETYPE	2
#define	POLYGONTYPE	3
#define	MULTIPOINTTYPE	4
#define	MULTILINETYPE	5
#define	MULTIPOLYGONTYPE	6
#define	COLLECTIONTYPE	7

#define WKBZOFFSET 0x80000000
#define WKBMOFFSET 0x40000000

#define TYPE_SETTYPE(c,t) ((c)=(((c)&0xF0)|t))
#define TYPE_SETZM(t,z,m) ((t)=(((t)&0xCF)|(z<<5)|(m<<4)))
#define TYPE_SETHASBBOX(t,b) ((t)=(((t)&0x7F)|(b<<7)))
#define TYPE_SETHASSRID(t,s) ((t)=(((t)&0xBF)|(s<<6)))

#define TYPE_HASZ(t) ( ((t)&0x20)>>5 )
#define TYPE_HASM(t) ( ((t)&0x10)>>4 )
#define TYPE_HASBBOX(t) ( ((t)&0x80)>>7 )
#define TYPE_HASSRID(t) ( (((t)&0x40))>>6 )
#define TYPE_NDIMS(t) ((((t)&0x20)>>5)+(((t)&0x10)>>4)+2)
#define TYPE_GETTYPE(t) ((t)&0x0F)
#define TYPE_GETZM(t) (((t)&0x30)>>4)

extern char lwgeom_hasBBOX(unsigned char type); // true iff B bit set
extern int  lwgeom_ndims(unsigned char type);   // returns 2,3 or 4
extern int  lwgeom_hasZ(unsigned char type);    // has Z ?
extern int  lwgeom_hasM(unsigned char type);    // has M ?
extern int  lwgeom_getType(unsigned char type); // returns the tttt value

extern unsigned char lwgeom_makeType(char hasZ, char hasM, char hasSRID, int type);
extern unsigned char lwgeom_makeType_full(char hasZ, char hasM, char hasSRID, int type, char hasBBOX);
extern char lwgeom_hasSRID(unsigned char type); // true iff S bit is set
extern char lwgeom_hasBBOX(unsigned char type); // true iff B bit set



/*
 * This is the binary representation of lwgeom compatible
 * with postgresql varlena struct
 */
typedef struct {
	uint32 size;
	unsigned char type; // encodes ndims, type, bbox presence,
			    // srid presence
	char data[1];
} PG_LWGEOM;

/*
 * Construct a full LWGEOM type (including size header)
 * from a serialized form.
 * The constructed LWGEOM object will be allocated using palloc
 * and the serialized form will be copied.
 * If you specify a SRID other then -1 it will be set.
 * If you request bbox (wantbbox=1) it will be extracted or computed
 * from the serialized form.
 */
extern PG_LWGEOM *PG_LWGEOM_construct(char *serialized, int SRID, int wantbbox);

/*
 * Use this macro to extract the char * required
 * by most functions from an PG_LWGEOM struct.
 * (which is an PG_LWGEOM w/out int32 size casted to char *)
 */
#define SERIALIZED_FORM(x) ((char *)(x))+4


/*
 * This function computes the size in bytes
 * of the serialized geometries.
 */
extern size_t lwgeom_size(const char *serialized_form);
extern size_t lwgeom_size_subgeom(const char *serialized_form, int geom_number);
extern size_t lwgeom_size_line(const char *serialized_line);
extern size_t lwgeom_size_point(const char *serialized_point);
extern size_t lwgeom_size_poly(const char *serialized_line);


//--------------------------------------------------------
// all the base types (point/line/polygon) will have a
// basic constructor, basic de-serializer, basic serializer,
// bounding box finder and (TODO) serialized form size finder.
//--------------------------------------------------------

// given the LWPOINT serialized form (or a pointer into a muli* one)
// construct a proper LWPOINT.
// serialized_form should point to the 8bit type format (with type = 1)
// Returns NULL if serialized form is not a point.
// See serialized form doc
extern LWPOINT *lwpoint_deserialize(char *serialized_form);

// Find size this point would get when serialized (no BBOX)
extern size_t lwpoint_serialize_size(LWPOINT *point);

// convert this point into its serialize form
// result's first char will be the 8bit type.  See serialized form doc
extern char  *lwpoint_serialize(LWPOINT *point);

// same as above, writes to buf
extern void lwpoint_serialize_buf(LWPOINT *point, char *buf, size_t *size);

// find bounding box (standard one)  zmin=zmax=0 if 2d (might change to NaN)
extern BOX3D *lwpoint_findbbox(LWPOINT *point);

// convenience functions to hide the POINTARRAY
extern int lwpoint_getPoint2d_p(const LWPOINT *point, POINT2D *out);
extern int lwpoint_getPoint3dz_p(const LWPOINT *point, POINT3DZ *out);
extern int lwpoint_getPoint3dm_p(const LWPOINT *point, POINT3DM *out);
extern int lwpoint_getPoint4d_p(const LWPOINT *point, POINT4D *out);

//--------------------------------------------------------


// given the LWGEOM serialized form (or a pointer into a muli* one)
// construct a proper LWLINE.
// serialized_form should point to the 8bit type format (with type = 2)
// See serialized form doc
extern LWLINE *lwline_deserialize(char *serialized_form);

// find the size this line would get when serialized (no BBOX)
extern size_t lwline_serialize_size(LWLINE *line);

// convert this line into its serialize form
// result's first char will be the 8bit type.  See serialized form doc
// copies data.
extern char  *lwline_serialize(LWLINE *line);

// same as above, writes to buf
extern void lwline_serialize_buf(LWLINE *line, char *buf, size_t *size);

// find bounding box (standard one)  zmin=zmax=0 if 2d (might change to NaN)
extern BOX3D *lwline_findbbox(LWLINE *line);

//--------------------------------------------------------


// given the LWPOLY serialized form (or a pointer into a muli* one)
// construct a proper LWPOLY.
// serialized_form should point to the 8bit type format (with type = 3)
// See serialized form doc
extern LWPOLY *lwpoly_deserialize(char *serialized_form);

// find the size this polygon would get when serialized (no bbox!)
extern size_t lwpoly_serialize_size(LWPOLY *poly);

// create the serialized form of the polygon
// result's first char will be the 8bit type.  See serialized form doc
// points copied
extern char *lwpoly_serialize(LWPOLY *poly);

// same as above, writes to buf
extern void lwpoly_serialize_buf(LWPOLY *poly, char *buf, size_t *size);

// find bounding box (standard one)  zmin=zmax=0 if 2d (might change to NaN)
extern BOX3D *lwpoly_findbbox(LWPOLY *poly);

//--------------------------------------------------------


extern size_t lwgeom_serialize_size(LWGEOM *geom);
extern size_t lwcollection_serialize_size(LWCOLLECTION *coll);
extern void lwgeom_serialize_buf(LWGEOM *geom, char *buf, size_t *size);
extern char *lwgeom_serialize(LWGEOM *geom);
extern void lwcollection_serialize_buf(LWCOLLECTION *mcoll, char *buf, size_t *size);

//-----------------------------------------------------

// Deserialize an lwgeom serialized form.
// The deserialized (recursive) structure will store
// pointers to the serialized form (POINTARRAYs).
LWGEOM *lwgeom_deserialize(char *serializedform);

// Release memory associated with LWGEOM.
// POINTARRAYs are not released as they are usually
// pointers to user-managed memory.
void lwgeom_release(LWGEOM *lwgeom);

LWMPOINT *lwmpoint_deserialize(char *serializedform);
LWMLINE *lwmline_deserialize(char *serializedform);
LWMPOLY *lwmpoly_deserialize(char *serializedform);
LWCOLLECTION *lwcollection_deserialize(char *serializedform);
LWGEOM *lwcollection_getsubgeom(LWCOLLECTION *, int);


//------------------------------------------------------

//------------------------------------------------------
// Multi-geometries
//
// These are all handled equivelently so its easy to write iterator code.
//  NOTE NOTE: you can hand in a non-multigeometry to most of these functions
//             and get usual behavior (ie. get geometry 0 on a POINT
//	       will return the point).
//             This makes coding even easier since you dont have to necessarily
//             differenciate between the multi* and non-multi geometries.
//
// NOTE: these usually work directly off the serialized form, so
//	they're a little more difficult to handle (and slower)
// NOTE NOTE: the get functions maybe slow, so we may want to have an "analysed"
//            lwgeom that would just have pointer to the start of each sub-geometry.
//------------------------------------------------------



// use this version for speed.  READ-ONLY!
typedef struct
{
	int   SRID;
	const char  *serialized_form; // orginal structure
	unsigned char  type;            // 8-bit type for the LWGEOM
	int ngeometries;     	// number of sub-geometries
	char **sub_geoms;  // list of pointers (into serialized_form) of the sub-geoms
} LWGEOM_INSPECTED;

extern int lwgeom_size_inspected(const LWGEOM_INSPECTED *inspected, int geom_number);

/*
 * This structure is intended to be used for geometry collection construction.
 * Does not allow specification of collection structure
 * (serialization chooses the simpler form)
 */
typedef struct
{
	int SRID;
	unsigned char dims;
	uint32 npoints;
	char **points;
	uint32 nlines;
	char **lines;
	uint32 npolys;
	char **polys;
} LWGEOM_EXPLODED;

void pfree_exploded(LWGEOM_EXPLODED *exploded);

// Returns a 'palloced' union of the two input exploded geoms.
// Returns NULL if SRID or ndims do not match.
LWGEOM_EXPLODED * lwexploded_sum(LWGEOM_EXPLODED *exp1, LWGEOM_EXPLODED *exp2);

/*
 * This function recursively scan the given serialized geometry
 * and returns a list of _all_ subgeoms in it (deep-first)
 */
extern LWGEOM_EXPLODED *lwgeom_explode(char *serialized);

/*
 * Return the length of the serialized form corresponding
 * to this exploded structure.
 */
extern uint32 lwexploded_findlength(LWGEOM_EXPLODED *exp, int wantbbox);

// Serialize an LWGEOM_EXPLODED object.
// SRID and ndims will be taken from exploded structure.
// wantbbox will determine result bbox.
extern char *lwexploded_serialize(LWGEOM_EXPLODED *exploded, int wantbbox);

// Same as lwexploded_serialize but writing to pre-allocated space
extern void lwexploded_serialize_buf(LWGEOM_EXPLODED *exploded, int wantbbox, char *buf, size_t *retsize);

// note - for a simple type (ie. point), this will have sub_geom[0] = serialized_form.
// for multi-geomtries sub_geom[0] will be a few bytes into the serialized form
// This function just computes the length of each sub-object and pre-caches this info.
// For a geometry collection of multi* geometries, you can inspect the sub-components
// as well.
extern LWGEOM_INSPECTED *lwgeom_inspect(const char *serialized_form);


// 1st geometry has geom_number = 0
// if the actual sub-geometry isnt a POINT, null is returned (see _gettype()).
// if there arent enough geometries, return null.
// this is fine to call on a point (with geom_num=0), multipoint or geometrycollection
extern LWPOINT *lwgeom_getpoint(char *serialized_form, int geom_number);
extern LWPOINT *lwgeom_getpoint_inspected(LWGEOM_INSPECTED *inspected, int geom_number);

// 1st geometry has geom_number = 0
// if the actual geometry isnt a LINE, null is returned (see _gettype()).
// if there arent enough geometries, return null.
// this is fine to call on a line, multiline or geometrycollection
extern LWLINE *lwgeom_getline(char *serialized_form, int geom_number);
extern LWLINE *lwgeom_getline_inspected(LWGEOM_INSPECTED *inspected, int geom_number);

// 1st geometry has geom_number = 0
// if the actual geometry isnt a POLYGON, null is returned (see _gettype()).
// if there arent enough geometries, return null.
// this is fine to call on a polygon, multipolygon or geometrycollection
extern LWPOLY *lwgeom_getpoly(char *serialized_form, int geom_number);
extern LWPOLY *lwgeom_getpoly_inspected(LWGEOM_INSPECTED *inspected, int geom_number);

// this gets the serialized form of a sub-geometry
// 1st geometry has geom_number = 0
// if this isnt a multi* geometry, and geom_number ==0 then it returns
// itself
// returns null on problems.
// in the future this is how you would access a muli* portion of a
// geometry collection.
//    GEOMETRYCOLLECTION(MULTIPOINT(0 0, 1 1), LINESTRING(0 0, 1 1))
//   ie. lwgeom_getpoint( lwgeom_getsubgeometry( serialized, 0), 1)
//           --> POINT(1 1)
// you can inspect the sub-geometry as well if you wish.
extern char *lwgeom_getsubgeometry(const char *serialized_form, int geom_number);
extern char *lwgeom_getsubgeometry_inspected(LWGEOM_INSPECTED *inspected, int geom_number);


// 1st geometry has geom_number = 0
//  use geom_number = -1 to find the actual type of the serialized form.
//    ie lwgeom_gettype( <'MULTIPOINT(0 0, 1 1)'>, -1)
//                 --> multipoint
//   ie lwgeom_gettype( <'MULTIPOINT(0 0, 1 1)'>, 0)
//                 --> point
// gets the 8bit type of the geometry at location geom_number
extern char lwgeom_getsubtype(char *serialized_form, int geom_number);
extern char lwgeom_getsubtype_inspected(LWGEOM_INSPECTED *inspected, int geom_number);


// how many sub-geometries are there?
//  for point,line,polygon will return 1.
extern int lwgeom_getnumgeometries(char *serialized_form);
extern int lwgeom_getnumgeometries_inspected(LWGEOM_INSPECTED *inspected);



// set finalType to COLLECTIONTYPE or 0 (0 means choose a best type)
//   (ie. give it 2 points and ask it to be a multipoint)
//  use SRID=-1 for unknown SRID  (will have 8bit type's S = 0)
// all subgeometries must have the same SRID
// if you want to construct an inspected, call this then inspect the result...
extern char *lwgeom_serialized_construct(int SRID, int finalType, char hasz, char hasm, int nsubgeometries, char **serialized_subs);


// construct the empty geometry (GEOMETRYCOLLECTION(EMPTY))
extern char *lwgeom_constructempty(int SRID, char hasz, char hasm);
extern void lwgeom_constructempty_buf(int SRID, char hasz, char hasm, char *buf, int *size);
int lwgeom_empty_length(int SRID);

// get the SRID from the LWGEOM
// none present => -1
extern int lwgeom_getSRID(PG_LWGEOM *lwgeom);
extern int lwgeom_getsrid(char *serialized);
extern PG_LWGEOM *lwgeom_setSRID(PG_LWGEOM *lwgeom, int32 newSRID);

//get bounding box of LWGEOM (automatically calls the sub-geometries bbox generators)
extern BOX3D *lw_geom_getBB(char *serialized_form);
extern BOX3D *lw_geom_getBB_inspected(LWGEOM_INSPECTED *inspected);


//------------------------------------------------------
// other stuff

// handle the double-to-float conversion.  The results of this
// will usually be a slightly bigger box because of the difference
// between float8 and float4 representations.

extern BOX2DFLOAT4 *box3d_to_box2df(BOX3D *box);
extern int box3d_to_box2df_p(BOX3D *box, BOX2DFLOAT4 *res);

extern BOX3D box2df_to_box3d(BOX2DFLOAT4 *box);
extern void box2df_to_box3d_p(BOX2DFLOAT4 *box, BOX3D *box3d);

extern BOX3D *combine_boxes(BOX3D *b1, BOX3D *b2);


// returns a real entity so it doesnt leak
// if this has a pre-built BOX2d, then we use it,
// otherwise we need to compute it.
// WARNING! the EMPTY geom will result in a random BOX2D returned
extern BOX2DFLOAT4 getbox2d(char *serialized_form);

// Returns a pointer to the BBOX internal to the serialized form.
// READ-ONLY!
// Or NULL if serialized form does not have a BBOX
extern BOX2DFLOAT4 *getbox2d_internal(char *serialized_form);

// this function writes to 'box' and returns 0 if serialized_form
// does not have a bounding box (empty geom)
extern int getbox2d_p(char *serialized_form, BOX2DFLOAT4 *box);

// Expand given box of 'd' units in all directions 
void expand_box2d(BOX2DFLOAT4 *box, double d);
void expand_box3d(BOX3D *box, double d);

// Check if to boxes are equal (considering FLOAT approximations)
char box2d_same(BOX2DFLOAT4 *box1, BOX2DFLOAT4 *box2);

//****************************************************************
// memory management -- these only delete the memory associated
//  directly with the structure - NOT the stuff pointing into
//  the original de-serialized info

extern void pfree_inspected(LWGEOM_INSPECTED *inspected);
extern void pfree_point    (LWPOINT *pt);
extern void pfree_line     (LWLINE  *line);
extern void pfree_polygon  (LWPOLY  *poly);
extern void pfree_POINTARRAY(POINTARRAY *pa);


//***********************************************************
// utility

extern uint32 get_uint32(const char *loc);
extern int32 get_int32(const char *loc);
extern void printPA(POINTARRAY *pa);
extern void printLWPOINT(LWPOINT *point);
extern void printLWLINE(LWLINE *line);
extern void printLWPOLY(LWPOLY *poly);
extern void printBYTES(unsigned char *a, int n);
extern void printMULTI(char *serialized);
extern void printType(unsigned char str);


//------------------------------------------------------------
//------------------------------------------------------------
// On serialized form  (see top for the 8bit type implementation)

// NOTE: contrary to the original proposal, bounding boxes are *never*
//       included in the geometry.  You must either refer to the index
//       or compute it on demand.


// The serialized form is always a stream of bytes.  The first four are always
// the memory size of the LWGEOM (including the 4 byte memory size).

// The easiest way to describe the serialed form is with examples:
// (more examples are available in the postgis mailing list)

//3D point w/o bounding box::
//<int32> size = 29 bytes
//<char> type:  S=0,D=1, tttt= 1
//<double> X
//<double> Y
//<double> Z

//2D line String
//<int32> size = ...
//<char> type:  S=0,D=0, tttt= 2
//<uint32> npoints
//<double> X0
//<double> Y0
//<double> X1
//<double> Y1
//<double> X2
//<double> Y2
//...

//3D polygon w/o bounding box
//<int32> size = ...
//<char> type:  S=0,D=0, tttt= 3
//<uint32> nrings
//<uint32> npoints in ring0
//<double> X0
//<double> Y0
//<double> X1
//<double> Y1
//<double> X2
//<double> Y2
//...
//<uint32> npoints in ring1
//<double> X0
//<double> Y0
//<double> X1
//<double> Y1
//<double> X2
//<double> Y2
//...
//...


// the multi* representations are very simple

//<int32> size = ...
//<char> type:  ... with  tttt= <multi* or geometrycollection>
//<int32> ngeometries
// <geometry zero, serialized form>
// <geometry one, serialized form>
// <geometry two, serialzied form>
// ...



// see implementation for more exact details.


//----------------------------------------------------------------
// example function (computes total length of the lines in a LWGEOM).
//   This works for a LINESTRING, MULTILINESTRING, OR GEOMETRYCOLLECTION



// 	  char *serialized_form = (char *)  [[get from database]]
//
//    double total_length_so_far = 0;
//    for (int t=0;t< lwgeom_getnumgeometries(serialized_form) ; t++)
//    {
//         LWLINE *line = lwgeom_getline(serialized_form, t);
//         if (line != NULL)
//		   {
//		        double length = findlength( POINT_ARRAY(line->points) ); //2d/3d aware
//         	    total_length_so_far + = length;
//         }
//    }
//    return total_length_so_far;


// using the LWGEOM_INSPECTED way:


// 	  char *serialized_form = (char *)  [[get from datbase]]
//    LWGEOM_INSPECTED inspected_geom = lwgeom_inspect(serialized_form);
//
//    double total_length_so_far = 0;
//    for (int t=0;t< lwgeom_getnumgeometries(inspected_geom) ; t++)
//    {
//         LWLINE *line = lwgeom_getline(inspected_geom, t);
//         if (line != NULL)
//		   {
//		         double length = findlength( POINT_ARRAY(line->points) ); //2d/3d aware
//         	    total_length_so_far + = length;
//         }
//    }
//    return total_length_so_far;


// the findlength() function could be written like based on functions like:
//
//   POINT3D getPoint3d(POINTARRAY pa, int n);  (for a 2d/3d point and 3d length)
//   POINT2D getPoint2d(POINTARRAY pa, int n);  (for a 2d/3d point and 2d length)
// NOTE: make sure your findlength() function knows what to do with z=NaN.


extern float LWGEOM_Minf(float a, float b);
extern float LWGEOM_Maxf(float a, float b);
extern double LWGEOM_Mind(double a, double b);
extern double LWGEOM_Maxd(double a, double b);

extern BOX3D *lw_geom_getBB_simple(char *serialized_form);

extern float  nextDown_f(double d);
extern float  nextUp_f(double d);
extern double nextDown_d(float d);
extern double nextUp_d(float d);



#if ! defined(__MINGW32__)
#define max(a,b)		((a) >	(b) ? (a) : (b))
#define min(a,b)		((a) <= (b) ? (a) : (b))
#endif
#define abs(a)			((a) <	(0) ? (-a) : (a))


// general utilities 
extern double lwgeom_polygon_area(LWPOLY *poly);
extern double lwgeom_polygon_perimeter(LWPOLY *poly);
extern double lwgeom_polygon_perimeter2d(LWPOLY *poly);
extern double lwgeom_pointarray_length2d(POINTARRAY *pts);
extern double lwgeom_pointarray_length(POINTARRAY *pts);
extern void lwgeom_force2d_recursive(char *serialized, char *optr, size_t *retsize);
extern void lwgeom_force3dz_recursive(char *serialized, char *optr, size_t *retsize);
extern void lwgeom_force3dm_recursive(unsigned char *serialized, char *optr, size_t *retsize);
extern void lwgeom_force4d_recursive(char *serialized, char *optr, size_t *retsize);
extern double distance2d_pt_pt(POINT2D *p1, POINT2D *p2);
extern double distance2d_pt_seg(POINT2D *p, POINT2D *A, POINT2D *B);
extern double distance2d_seg_seg(POINT2D *A, POINT2D *B, POINT2D *C, POINT2D *D);
extern double distance2d_pt_ptarray(POINT2D *p, POINTARRAY *pa);
extern double distance2d_ptarray_ptarray(POINTARRAY *l1, POINTARRAY *l2);
extern int pt_in_ring_2d(POINT2D *p, POINTARRAY *ring);
extern int pt_in_poly_2d(POINT2D *p, LWPOLY *poly);
extern double distance2d_ptarray_poly(POINTARRAY *pa, LWPOLY *poly);
extern double distance2d_point_point(LWPOINT *point1, LWPOINT *point2);
extern double distance2d_point_line(LWPOINT *point, LWLINE *line);
extern double distance2d_line_line(LWLINE *line1, LWLINE *line2);
extern double distance2d_point_poly(LWPOINT *point, LWPOLY *poly);
extern double distance2d_poly_poly(LWPOLY *poly1, LWPOLY *poly2);
extern double distance2d_line_poly(LWLINE *line, LWPOLY *poly);
extern double lwgeom_mindistance2d_recursive(char *lw1, char *lw2);
extern void lwgeom_translate_recursive(char *serialized, double xoff, double yoff, double zoff);
extern void lwgeom_translate_ptarray(POINTARRAY *pa, double xoff, double yoff, double zoff);
extern int lwgeom_pt_inside_circle(POINT2D *p, double cx, double cy, double rad);
extern int32 lwgeom_npoints(char *serialized);
extern char ptarray_isccw(const POINTARRAY *pa);
extern void lwgeom_reverse(LWGEOM *lwgeom);
extern void lwline_reverse(LWLINE *line);
extern void lwpoly_reverse(LWPOLY *poly);
extern void lwpoly_forceRHR(LWPOLY *poly);
extern void lwgeom_forceRHR(LWGEOM *lwgeom);
extern char *lwgeom_summary(LWGEOM *lwgeom, int offset);
extern const char *lwgeom_typename(int type);
extern int ptarray_compute_bbox_p(const POINTARRAY *pa, BOX2DFLOAT4 *result);
extern BOX2DFLOAT4 *ptarray_compute_bbox(const POINTARRAY *pa);
extern int lwpoint_compute_bbox_p(LWPOINT *point, BOX2DFLOAT4 *box);
extern int lwline_compute_bbox_p(LWLINE *line, BOX2DFLOAT4 *box);
extern int lwpoly_compute_bbox_p(LWPOLY *poly, BOX2DFLOAT4 *box);
extern int lwcollection_compute_bbox_p(LWCOLLECTION *col, BOX2DFLOAT4 *box);
extern BOX2DFLOAT4 *lwgeom_compute_bbox(LWGEOM *lwgeom);
int lwgeom_compute_bbox_p(LWGEOM *lwgeom, BOX2DFLOAT4 *buf);
// return alloced memory
extern BOX2DFLOAT4 *box2d_union(BOX2DFLOAT4 *b1, BOX2DFLOAT4 *b2);
// args may overlap !
extern int box2d_union_p(BOX2DFLOAT4 *b1, BOX2DFLOAT4 *b2, BOX2DFLOAT4 *ubox);
extern int lwgeom_compute_bbox_p(LWGEOM *lwgeom, BOX2DFLOAT4 *box);

// Is lwgeom1 geometrically equal to lwgeom2 ?
char lwgeom_same(const LWGEOM *lwgeom1, const LWGEOM *lwgeom2);
char ptarray_same(const POINTARRAY *pa1, const POINTARRAY *pa2);
char lwpoint_same(const LWPOINT *p1, const LWPOINT *p2);
char lwline_same(const LWLINE *p1, const LWLINE *p2);
char lwpoly_same(const LWPOLY *p1, const LWPOLY *p2);
char lwcollection_same(const LWCOLLECTION *p1, const LWCOLLECTION *p2);

// Add 'what' to 'to' at position 'where'.
// where=0 == prepend
// where=-1 == append
// Mix of dimensions is not allowed (TODO: allow it?).
// Returns a newly allocated LWGEOM (with NO BBOX)
extern LWGEOM *lwgeom_add(const LWGEOM *to, uint32 where, const LWGEOM *what);

LWGEOM *lwpoint_add(const LWPOINT *to, uint32 where, const LWGEOM *what);
LWGEOM *lwline_add(const LWLINE *to, uint32 where, const LWGEOM *what);
LWGEOM *lwpoly_add(const LWPOLY *to, uint32 where, const LWGEOM *what);
LWGEOM *lwmpoly_add(const LWMPOLY *to, uint32 where, const LWGEOM *what);
LWGEOM *lwmline_add(const LWMLINE *to, uint32 where, const LWGEOM *what);
LWGEOM *lwmpoint_add(const LWMPOINT *to, uint32 where, const LWGEOM *what);
LWGEOM *lwcollection_add(const LWCOLLECTION *to, uint32 where, const LWGEOM *what);

// Clone an LWGEOM
// pointarray are not copied.
// BBOXes are copied only if HASBBOX flag is 0 (meaning bbox ownership)
extern LWGEOM *lwgeom_clone(const LWGEOM *lwgeom);
extern LWPOINT *lwpoint_clone(const LWPOINT *lwgeom);
extern LWLINE *lwline_clone(const LWLINE *lwgeom);
extern LWPOLY *lwpoly_clone(const LWPOLY *lwgeom);
extern LWCOLLECTION *lwcollection_clone(const LWCOLLECTION *lwgeom);
extern BOX2DFLOAT4 *box2d_clone(const BOX2DFLOAT4 *lwgeom);

// Geometry constructors
// Take ownership of arguments
extern LWPOINT  *lwpoint_construct(int SRID, BOX2DFLOAT4 *bbox,
	POINTARRAY *point);
extern LWLINE *lwline_construct(int SRID, BOX2DFLOAT4 *bbox,
	POINTARRAY *points);
extern LWPOLY *lwpoly_construct(int SRID, BOX2DFLOAT4 *bbox,
	unsigned int nrings, POINTARRAY **points);
extern LWCOLLECTION *lwcollection_construct(unsigned int type, int SRID,
	BOX2DFLOAT4 *bbox, unsigned int ngeoms, LWGEOM **geoms);
extern LWCOLLECTION *lwcollection_construct_empty(int SRID,
	char hasZ, char hasM);

// Return a char string with ASCII versionf of type flags
extern const char *lwgeom_typeflags(unsigned char type);

// Construct an empty pointarray
extern POINTARRAY *ptarray_construct(char hasz, char hasm, unsigned int npoints);
extern POINTARRAY *ptarray_construct2d(uint32 npoints, const POINT2D *pts);

extern int32 lwgeom_nrings_recursive(char *serialized);
extern void dump_lwexploded(LWGEOM_EXPLODED *exploded);
extern void ptarray_reverse(POINTARRAY *pa);

// Ensure every segment is at most 'dist' long.
// Returned LWGEOM might is unchanged if a POINT.
extern LWGEOM *lwgeom_segmentize2d(LWGEOM *line, double dist);
extern POINTARRAY *ptarray_segmentize2d(POINTARRAY *ipa, double dist);
extern LWLINE *lwline_segmentize2d(LWLINE *line, double dist);
extern LWPOLY *lwpoly_segmentize2d(LWPOLY *line, double dist);
extern LWCOLLECTION *lwcollection_segmentize2d(LWCOLLECTION *coll, double dist);

extern unsigned char	parse_hex(char *str);
extern void deparse_hex(unsigned char str, unsigned char *result);
extern char *parse_lwgeom_wkt(char *wkt_input);
extern char *lwgeom_to_wkt(LWGEOM *lwgeom);
extern char *lwgeom_to_hexwkb(LWGEOM *lwgeom, unsigned int byteorder);

extern void *lwalloc(size_t size);
extern void *lwrealloc(void *mem, size_t size);
extern void lwfree(void *mem);

#endif // !defined _LIBLWGEOM_H 
