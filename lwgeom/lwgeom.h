//lwgeom.h


// basic API for handling the LWGEOM, BOX2DFLOAT4, LWPOINT, LWLINE, and LWPOLY.
// See below for other support types like POINTARRAY and LWGEOM_INSPECTED

#include <sys/types.h>
#include "utils/geo_decls.h"


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


// POINT3D already defined in postgis.h
// ALL LWGEOM structures will use POINT3D as an abstract point.
// This means a 2d geometry will be stored as (x,y) in its serialized
// form, but all functions will work on (x,y,0).  This keeps all the
// analysis functions simple.
// NOTE: for GEOS integration, we'll probably set z=NaN
//        so look out - z might be NaN for 2d geometries!
typedef struct { double	x,y,z; } POINT3D;


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

// Point array abstracts a lot of the complexity of points and point lists.
// It handles miss-alignment in the serialized form, 2d/3d translation
//    (2d points converted to 3d will have z=0 or NaN)

typedef struct
{
    char  *serialized_pointlist; // probably missaligned. 2d or 3d.  points to a double
    char  ndims; // 2=2d, 3=3d, 4=4d, 5=undef
    uint32 npoints;
}  POINTARRAY;

// copies a point from the point array into the parameter point
// will set point's z=0 (or NaN) if pa is 2d
// will set point's m=0 (or NaN( if pa is 3d or 2d
// NOTE: point is a real POINT3D *not* a pointer
extern POINT4D getPoint4d(POINTARRAY *pa, int n);

// copies a point from the point array into the parameter point
// will set point's z=0 (or NaN) if pa is 2d
// will set point's m=0 (or NaN( if pa is 3d or 2d
// NOTE: this will modify the point4d pointed to by 'point'.
extern void getPoint4d_p(POINTARRAY *pa, int n, char *point);

// copies a point from the point array into the parameter point
// will set point's z=0 (or NaN) if pa is 2d
// NOTE: point is a real POINT3D *not* a pointer
extern POINT3D getPoint3d(POINTARRAY *pa, int n);

// copies a point from the point array into the parameter point
// will set point's z=0 (or NaN) if pa is 2d
// NOTE: this will modify the point3d pointed to by 'point'.
extern void getPoint3d_p(POINTARRAY *pa, int n, char *point);


// copies a point from the point array into the parameter point
// z value (if present is not returned)
// NOTE: point is a real POINT3D *not* a pointer
extern POINT2D getPoint2d(POINTARRAY *pa, int n);

// copies a point from the point array into the parameter point
// z value (if present is not returned)
// NOTE: this will modify the point2d pointed to by 'point'.
extern void getPoint2d_p(POINTARRAY *pa, int n, char *point);

// get a pointer to nth point of a POINTARRAY
// You'll need to cast it to appropriate dimensioned point.
// Note that if you cast to a higher dimensional point you'll
// possibly corrupt the POINTARRAY.
extern char *getPoint(POINTARRAY *pa, int n);
//--- here is a macro equivalent, for speed...
//#define getPoint(x,n) &( (x)->serialized_pointlist[((x)->ndims*8)*(n)] )


// constructs a POINTARRAY.
// NOTE: points is *not* copied, so be careful about modification (can be aligned/missaligned)
// NOTE: ndims is descriptive - it describes what type of data 'points'
//       points to.  No data conversion is done.
extern POINTARRAY *pointArray_construct(char *points, int ndims, uint32 npoints);

//calculate the bounding box of a set of points
// returns a 3d box
// if pa is 2d, then box3d's zmin/zmax will be either 0 or NaN
// dont call on an empty pa
extern BOX3D *pointArray_bbox(POINTARRAY *pa);

//size of point represeneted in the POINTARRAY
// 16 for 2d, 24 for 3d, 32 for 4d
extern int pointArray_ptsize(POINTARRAY *pa);



/*

 LWGEOM types are an 8-bit char in this format:

BSDDtttt

WHERE
    B = 16 byte BOX2DFLOAT4 follows (probably not aligned) [before SRID]
    S = 4 byte SRID attached (0= not attached (-1), 1= attached)
    DD = dimentionality (0=2d, 1=3d, 2= 4d)
    tttt = actual type (as per the WKB type):

    enum wkbGeometryType {
        wkbPoint = 1,
        wkbLineString = 2,
        wkbPolygon = 3,
        wkbMultiPoint = 4,
        wkbMultiLineString = 5,
        wkbMultiPolygon = 6,
        wkbGeometryCollection = 7
    };

*/

#define TYPE_SETTYPE(c,t) (((c)&0xF0)|t)
#define TYPE_SETDIMS(c,d) (((c)&0xCF)|d)
#define TYPE_SETHASBBOX(c,b) (((c)&0x7F)|b)
#define TYPE_SETHASSRID(c,s) (((c)&0xBF)|s)


// already defined in postgis.h

 #define	POINTTYPE	1
 #define	LINETYPE	2
 #define	POLYGONTYPE	3
 #define	MULTIPOINTTYPE	4
 #define	MULTILINETYPE	5
 #define	MULTIPOLYGONTYPE	6
 #define	COLLECTIONTYPE	7

/*
 * This is the binary representation of lwgeom compatible
 * with postgresql varlena struct
 */
typedef struct {
	int32 size;
	unsigned char type; // encodes ndims, type, bbox presence,
			    // srid presence
	char data[1];
} LWGEOM;

/*
 * Use this macro to extract the char * required
 * by most functions from an LWGEOM struct.
 * (which is an LWGEOM w/out int32 size casted to char *)
 */
#define SERIALIZED_FORM(x) ((char *)&((x)->type))


extern bool lwgeom_hasSRID(unsigned char type); // true iff S bit is set
extern bool lwgeom_hasBBOX(unsigned char type); // true iff B bit set
extern int  lwgeom_ndims(unsigned char type);   // returns the DD value
extern int  lwgeom_getType(unsigned char type); // returns the tttt value

extern unsigned char lwgeom_makeType(int ndims, char hasSRID, int type);
extern unsigned char lwgeom_makeType_full(int ndims, char hasSRID, int type, bool hasBBOX);

/*
 * Construct a full LWGEOM type (including size header)
 * from a serialized form.
 * The constructed LWGEOM object will be allocated using palloc
 * and the serialized form will be copied.
 * If you specify a SRID other then -1 it will be set.
 * If you request bbox (wantbbox=1) it will be extracted or computed
 * from the serialized form.
 */
extern LWGEOM *LWGEOM_construct(char *serialized, int SRID, int wantbbox);

// all the base types (point/line/polygon) will have a
// basic constructor, basic de-serializer, basic serializer, and
// bounding box finder.



//--------------------------------------------------------

typedef struct
{
	char  ndims; // 2=2d, 3=3d, 4=4d, 5=undef
	int  SRID;   // spatial ref sys -1=none
	POINTARRAY    *points; // array of POINT3D
} LWLINE; //"light-weight line"

// construct a new LWLINE.  points will *NOT* be copied
// use SRID=-1 for unknown SRID (will have 8bit type's S = 0)
extern LWLINE *lwline_construct(int ndims, int SRID, POINTARRAY *points);

// given the LWGEOM serialized form (or a pointer into a muli* one)
// construct a proper LWLINE.
// serialized_form should point to the 8bit type format (with type = 2)
// See serialized form doc
extern LWLINE *lwline_deserialize(char *serialized_form);

// convert this line into its serialize form
// result's first char will be the 8bit type.  See serialized form doc
// copies data.
extern char  *lwline_serialize(LWLINE *line);
// same as above, writes to buf
extern void lwline_serialize_buf(LWLINE *line, char *buf, int *size);

// find bounding box (standard one)  zmin=zmax=0 if 2d (might change to NaN)
extern BOX3D *lwline_findbbox(LWLINE *line);


//find length of this serialized line
extern uint32 lwline_findlength(char *serialized_line);

//--------------------------------------------------------

typedef struct
{
   	char     ndims; // 2=2d, 3=3d, 4=4d, 5=undef
   	int      SRID;   // spatial ref sys
   	POINTARRAY  *point;  // hide 2d/3d (this will be an array of 1 point)
}  LWPOINT; // "light-weight point"

// construct a new point.  point will NOT be copied
// use SRID=-1 for unknown SRID (will have 8bit type's S = 0)
extern LWPOINT  *lwpoint_construct(int ndims, int SRID, POINTARRAY *point);

// given the LWPOINT serialized form (or a pointer into a muli* one)
// construct a proper LWPOINT.
// serialized_form should point to the 8bit type format (with type = 1)
// Returns NULL if serialized form is not a point.
// See serialized form doc
extern LWPOINT *lwpoint_deserialize(char *serialized_form);

// convert this point into its serialize form
// result's first char will be the 8bit type.  See serialized form doc
extern char  *lwpoint_serialize(LWPOINT *point);
// same as above, writes to buf
extern void lwpoint_serialize_buf(LWPOINT *point, char *buf, int *size);

// find bounding box (standard one)  zmin=zmax=0 if 2d (might change to NaN)
extern BOX3D *lwpoint_findbbox(LWPOINT *point);

// convenience functions to hide the POINTARRAY
extern POINT2D lwpoint_getPoint2d(LWPOINT *point);
extern POINT3D lwpoint_getPoint3d(LWPOINT *point);

//find length of this serialized point
extern uint32 lwpoint_findlength(char *serialized_point);

//--------------------------------------------------------

//DONT MIX 2D and 3D POINTS!  *EVERYTHING* is either one or the other

typedef struct
{
	int32 SRID;
	char ndims;
	int  nrings;
	POINTARRAY **rings; // list of rings (list of points)
} LWPOLY; // "light-weight polygon"

// construct a new LWPOLY.  arrays (points/points per ring) will NOT be copied
// use SRID=-1 for unknown SRID (will have 8bit type's S = 0)
extern LWPOLY *lwpoly_construct(int ndims, int SRID, int nrings,POINTARRAY **points);


// given the LWPOLY serialized form (or a pointer into a muli* one)
// construct a proper LWPOLY.
// serialized_form should point to the 8bit type format (with type = 3)
// See serialized form doc
extern LWPOLY *lwpoly_deserialize(char *serialized_form);

// create the serialized form of the polygon
// result's first char will be the 8bit type.  See serialized form doc
// points copied
extern char *lwpoly_serialize(LWPOLY *poly);
// same as above, writes to buf
extern void lwpoly_serialize_buf(LWPOLY *poly, char *buf, int *size);


// find bounding box (standard one)  zmin=zmax=0 if 2d (might change to NaN)
extern BOX3D *lwpoly_findbbox(LWPOLY *poly);


//find length of this serialized polygon
extern uint32 lwpoly_findlength(char *serialized_line);


//------------------------------------------------------
// Multi-geometries

// These are all handled equivelently so its easy to write iterator code.
//  NOTE NOTE: you can hand in a non-multigeometry to most of these functions
//             and get usual behavior (ie. get geometry 0 on a POINT will return the
//             point).  This makes coding even easier since you dont have to necessarily
//             differenciate between the multi* and non-multi geometries.
//
// NOTE: these usually work directly off the serialized form, so they're a little more
//        difficult to handle (and slower)
// NOTE NOTE: the get functions maybe slow, so we may want to have an "analysed"
//            lwgeom that would just have pointer to the start of each sub-geometry.



// use this version for speed.  READ-ONLY!
typedef struct
{
	int   SRID;
	char  *serialized_form; // orginal structure
	unsigned char  type;            // 8-bit type for the LWGEOM
	int   ngeometries;     // number of sub-geometries
	char  **sub_geoms;    // list of pointers (into serialized_form) of the sub-geoms
} LWGEOM_INSPECTED;

/*
 * This structure is intended to be used for geometry collection construction
 */
typedef struct
{
	int SRID;
	int ndims;
	int npoints;
	char **points;
	int nlines;
	char **lines;
	int npolys;
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
LWGEOM_EXPLODED *lwgeom_explode(char *serialized);

// Serialize an LWGEOM_EXPLODED object.
// SRID and ndims will be taken from exploded structure.
// wantbbox will determine result bbox.
char *lwexploded_serialize(LWGEOM_EXPLODED *exploded, int wantbbox);

// note - for a simple type (ie. point), this will have sub_geom[0] = serialized_form.
// for multi-geomtries sub_geom[0] will be a few bytes into the serialized form
// This function just computes the length of each sub-object and pre-caches this info.
// For a geometry collection of multi* geometries, you can inspect the sub-components
// as well.
extern LWGEOM_INSPECTED *lwgeom_inspect(char *serialized_form);


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
extern char *lwgeom_getsubgeometry(char *serialized_form, int geom_number);
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
extern char *lwgeom_construct(int SRID,int finalType,int ndims, int nsubgeometries, char **serialized_subs);


// construct the empty geometry (GEOMETRYCOLLECTION(EMPTY))
extern char *lwgeom_constructempty(int SRID,int ndims);

// helper function (not for general use)
// find the size a geometry (or a sub-geometry)
// 1st geometry has geom_number = 0
//  use geom_number = -1 to find the actual type of the serialized form.
//    ie lwgeom_gettype( <'MULTIPOINT(0 0, 1 1)'>, -1)
//                 --> size of the multipoint
//   ie lwgeom_gettype( <'MULTIPOINT(0 0, 1 1)'>, 0)
//                 --> size of the point
extern int lwgeom_seralizedformlength_simple(char *serialized_form);
extern int lwgeom_seralizedformlength(char *serialized_form, int geom_number);
extern int lwgeom_seralizedformlength_inspected(LWGEOM_INSPECTED *inspected, int geom_number);

// get the SRID from the LWGEOM
// none present => -1
extern int lwgeom_getSRID(LWGEOM *lwgeom);
extern int lwgeom_getsrid(char *serialized);
extern LWGEOM *lwgeom_setSRID(LWGEOM *lwgeom, int32 newSRID);

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
extern BOX2DFLOAT4 *box_to_box2df(BOX *box);  // postgresql standard type

extern BOX3D box2df_to_box3d(BOX2DFLOAT4 *box);
extern void box2df_to_box3d_p(BOX2DFLOAT4 *box, BOX3D *box3d);
extern BOX box2df_to_box(BOX2DFLOAT4 *box);  // postgresql standard type
extern void box2df_to_box_p(BOX2DFLOAT4 *box, BOX *out); // postgresql standard type

extern BOX3D *combine_boxes(BOX3D *b1, BOX3D *b2);


// returns a real entity so it doesnt leak
// if this has a pre-built BOX2d, then we use it,
// otherwise we need to compute it.
// WARNING! the EMPTY geom will result in a random BOX2D returned
extern BOX2DFLOAT4 getbox2d(char *serialized_form);

// this function writes to 'box' and returns 0 if serialized_form
// does not have a bounding box (empty geom)
extern int getbox2d_p(char *serialized_form, BOX2DFLOAT4 *box);

// this function returns a pointer to the 'internal' bounding
// box of a serialized-form geometry. If the geometry does
// not have an embedded bounding box the function returns NULL.
// READ-ONLY!
extern const BOX2DFLOAT4 * getbox2d_internal(char *serialized_form);

// Expand given box of 'd' units in all directions 
void expand_box2d(BOX2DFLOAT4 *box, double d);
void expand_box3d(BOX3D *box, double d);

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

extern uint32 get_uint32(char *loc);
extern int32 get_int32(char *loc);
extern void printPA(POINTARRAY *pa);
extern void printLWPOINT(LWPOINT *point);
extern void printLWLINE(LWLINE *line);
extern void printLWPOLY(LWPOLY *poly);
extern void printBYTES(unsigned char *a, int n);
extern void printMULTI(char *serialized);
extern void deparse_hex(unsigned char str, unsigned char *result);
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




// other forwards (for indirect function calls)

Datum box2d_same(PG_FUNCTION_ARGS);
Datum box2d_overlap(PG_FUNCTION_ARGS);
Datum box2d_overleft(PG_FUNCTION_ARGS);
Datum box2d_left(PG_FUNCTION_ARGS);
Datum box2d_right(PG_FUNCTION_ARGS);
Datum box2d_overright(PG_FUNCTION_ARGS);
Datum box2d_contained(PG_FUNCTION_ARGS);
Datum box2d_contain(PG_FUNCTION_ARGS);
Datum box2d_inter(PG_FUNCTION_ARGS);
Datum box2d_union(PG_FUNCTION_ARGS);

Datum gist_lwgeom_compress(PG_FUNCTION_ARGS);
Datum gist_lwgeom_consistent(PG_FUNCTION_ARGS);
Datum gist_rtree_decompress(PG_FUNCTION_ARGS);
Datum lwgeom_box_union(PG_FUNCTION_ARGS);
Datum lwgeom_box_penalty(PG_FUNCTION_ARGS);
Datum lwgeom_gbox_same(PG_FUNCTION_ARGS);
Datum lwgeom_gbox_picksplit(PG_FUNCTION_ARGS);


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
double lwgeom_polygon_area(LWPOLY *poly);
double lwgeom_polygon_perimeter(LWPOLY *poly);
double lwgeom_polygon_perimeter2d(LWPOLY *poly);
double lwgeom_pointarray_length2d(POINTARRAY *pts);
double lwgeom_pointarray_length(POINTARRAY *pts);
void lwgeom_force2d_recursive(char *serialized, char *optr, int *retsize);
void lwgeom_force3d_recursive(char *serialized, char *optr, int *retsize);
void lwgeom_force4d_recursive(char *serialized, char *optr, int *retsize);
double distance2d_pt_pt(POINT2D *p1, POINT2D *p2);
double distance2d_pt_seg(POINT2D *p, POINT2D *A, POINT2D *B);
double distance2d_seg_seg(POINT2D *A, POINT2D *B, POINT2D *C, POINT2D *D);
double distance2d_pt_ptarray(POINT2D *p, POINTARRAY *pa);
double distance2d_ptarray_ptarray(POINTARRAY *l1, POINTARRAY *l2);
int pt_in_ring_2d(POINT2D *p, POINTARRAY *ring);
int pt_in_poly_2d(POINT2D *p, LWPOLY *poly);
double distance2d_ptarray_poly(POINTARRAY *pa, LWPOLY *poly);
double distance2d_point_point(LWPOINT *point1, LWPOINT *point2);
double distance2d_point_line(LWPOINT *point, LWLINE *line);
double distance2d_line_line(LWLINE *line1, LWLINE *line2);
double distance2d_point_poly(LWPOINT *point, LWPOLY *poly);
double distance2d_poly_poly(LWPOLY *poly1, LWPOLY *poly2);
double distance2d_line_poly(LWLINE *line, LWPOLY *poly);
double lwgeom_mindistance2d_recursive(char *lw1, char *lw2);
void lwgeom_translate_recursive(char *serialized, double xoff, double yoff, double zoff);
void lwgeom_translate_ptarray(POINTARRAY *pa, double xoff, double yoff, double zoff);
int lwgeom_pt_inside_circle(POINT2D *p, double cx, double cy, double rad);
POINTARRAY *segmentize2d_ptarray(POINTARRAY *ipa, double dist);
