//lwgeom.h

// basic API for handling the LWGEOM, BOX2DFLOAT4, LWPOINT, LWLINE, and LWPOLY.
// See below for other support types like POINTARRAY and LWGEOM_INSPECTED


typedef struct
{
	float xmin;
	float ymin;
	float xmax;
	float ymax;
} BOX2DFLOAT4;


// POINT3D already defined in postgis.h
// ALL LWGEOM structures will use POINT3D as an abstract point.
// This means a 2d geometry will be stored as (x,y) in its serialized
// form, but all functions will work on (x,y,0).  This keeps all the
// analysis functions simple.
// NOTE: for GEOS integration, we'll probably set z=NaN
//        so look out - z might be NaN for 2d geometries!

// typedef struct {	double	x,y,z;  } POINT3D;


// type for 2d points.  When you convert this to 3d, the
//   z component will be either 0 or NaN.
typedef struct
{
	 double x;
	 double y;
} POINT2D;


// Point array abstracts a lot of the complexity of points and point lists.
// It handles miss-alignment in the serialized form, 2d/3d translation
//    (2d points converted to 3d will have z=0 or NaN)

typedef struct
{
    char  *serialized_pointlist; // probably missaligned. 2d or 3d.  points to a double
    char  is3d; // true if these are 3d points
    int32 npoints;
}  POINTARRAY;

// copies a point from the point array into the parameter point
// will set point's z=0 (or NaN) if pa is 2d
// NOTE: point is a real POINT3D *not* a pointer
extern POINT3D getPoint3d(POINTARRAY pa, int n);

// copies a point from the point array into the parameter point
// will set point's z=0 (or NaN) if pa is 2d
// NOTE: this will modify the point3d pointed to by 'point'.
extern void getPoint3d_p(POINTARRAY pa, int n, POINT3D *point);


// copies a point from the point array into the parameter point
// z value (if present is not returned)
// NOTE: point is a real POINT3D *not* a pointer
extern POINT2D getPoint2d(POINTARRAY pa, int n);

// copies a point from the point array into the parameter point
// z value (if present is not returned)
// NOTE: this will modify the point2d pointed to by 'point'.
extern void getPoint2d_p(POINTARRAY pa, int n, POINT2D *point);


// constructs a POINTARRAY.
// NOTE: points is *not* copied, so be careful about modification (can be aligned/missaligned)
// NOTE: is3d is descriptive - it describes what type of data 'points'
//       points to.  No data conversion is done.
extern POINTARRAY pointArray_construct(char *points, char is3d, int npoints);


/*

 LWGEOM types are an 8-bit char in this format:

xxSDtttt

WHERE
    x = unused
    S = 4 byte SRID attached (0= not attached (-1), 1= attached)
    D = dimentionality (0=2d, 1=3d)
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


/* already defined in postgis.h

 #define	POINTTYPE	1
 #define	LINETYPE	2
 #define	POLYGONTYPE	3
 #define	MULTIPOINTTYPE	4
 #define	MULTILINETYPE	5
 #define	MULTIPOLYGONTYPE	6
 #define	COLLECTIONTYPE	7

*/


extern bool lwgeom_hasSRID(char type); // true iff S bit is set
extern bool lwgeom_is3d(char type);    // true iff D bit is set
extern int     lwgeom_getType(char type); // returns the tttt value



// all the base types (point/line/polygon) will have a
// basic constructor, basic de-serializer, basic serializer, and
// bounding box finder.



//--------------------------------------------------------

typedef struct
{
	char        is3d;   // true means points represent 3d points
	int         SRID;   // spatial ref sys -1=none
	POINTARRAY *points; // array of POINT3D
} LWLINE; //"light-weight line"

// construct a new LWLINE.  points will be copied
// use SRID=-1 for unknown SRID (will have 8bit type's S = 0)
extern LWLINE lwline_construct(char is3d, int SRID, int npoints, POINTARRAY *points);

// given the LWGEOM serialized form (or a pointer into a muli* one)
// construct a proper LWLINE.
// serialized_form should point to the 8bit type format (with type = 2)
// See serialized form doc
extern LWLINE lwline_deserialize(char *serialized_form);

// convert this line into its serialize form
// result's first char will be the 8bit type.  See serialized form doc
extern char  *lwline_serialize(LWLINE *line);

// find bounding box (standard one)  zmin=zmax=0 if 2d (might change to NaN)
extern BOX3D *lwline_findbbox(LWLINE *line);


//--------------------------------------------------------

typedef struct
{
   	char     is3d;   // true means points represent 3d points
   	int      SRID;   // spatial ref sys
   	POINTARRAY  *point;  // hide 2d/3d (this will be an array of 1 point)
}  LWPOINT; // "light-weight point"

// construct a new point.  point will be copied
// use SRID=-1 for unknown SRID (will have 8bit type's S = 0)
extern LWPOINT  lwpoint_construct(char is3d, int SRID, POINT3D *point);

// given the LWPOINT serialized form (or a pointer into a muli* one)
// construct a proper LWPOINT.
// serialized_form should point to the 8bit type format (with type = 1)
// See serialized form doc
extern LWPOINT lwpoint_deserialize(char *serialized_form);

// convert this line into its serialize form
// result's first char will be the 8bit type.  See serialized form doc
extern char  *lwpoint_serialize(LWPOINT *point);

// find bounding box (standard one)  zmin=zmax=0 if 2d (might change to NaN)
extern BOX3D *lwpoint_findbbox(LWPOINT *point);

// convenience functions to hide the POINTARRAY
extern POINT2D lwpoint_getPoint2d(LWPOINT *point);
extern POINT3D lwpoint_getPoint3d(LWPOINT *point);

//--------------------------------------------------------


typedef struct
{
	char is3d;
	int  SRID;
	int  nrings;
	POINTARRAY **rings; // list of rings (list of points)
} LWPOLY; // "light-weight polygon"

// construct a new LWLINE.  arrays (points/points per ring) will NOT be copied
// use SRID=-1 for unknown SRID (will have 8bit type's S = 0)
extern LWLINE lwpoly_construct(char is3d, int SRID, int nrings,POINTARRAY **points);


// given the LWPOLY serialized form (or a pointer into a muli* one)
// construct a proper LWPOLY.
// serialized_form should point to the 8bit type format (with type = 3)
// See serialized form doc
extern LWPOLY lwpoly_deserialize(char *serialized_form);

// find bounding box (standard one)  zmin=zmax=0 if 2d (might change to NaN)
extern BOX3D *lwpoly_findbbox(LWPOLY *line);



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
	char  *serialized_form; // orginal structure
	char  type;            // 8-bit type for the LWGEOM
	int   ngeometries;     // number of sub-geometries
	char  **sub_geoms;    // list of pointers (into serialized_form) of the sub-geoms
} LWGEOM_INSPECTED;

// note - for a simple type (ie. point), this will have sub_geom[0] = serialized_form.
// for multi-geomtries sub_geom[0] will be a few bytes into the serialized form
// This function just computes the length of each sub-object and pre-caches this info.
// For a geometry collection of multi* geometries, you can inspect the sub-components
// as well.
extern LWGEOM_INSPECTED lwgeom_inspect(char *serialized_form);


// 1st geometry has geom_number = 0
// if the actual sub-geometry isnt a POINT, null is returned (see _gettype()).
// if there arent enough geometries, return null.
// this is fine to call on a point (with geom_num=0), multipoint or geometrycollection
extern LWPOINT lwgeom_getpoint(char *serialized_form, int geom_number);
extern LWPOINT lwgeom_getpoint_inspected(LWGEOM_INSPECTED *inspected, int geom_number);

// 1st geometry has geom_number = 0
// if the actual geometry isnt a LINE, null is returned (see _gettype()).
// if there arent enough geometries, return null.
// this is fine to call on a line, multiline or geometrycollection
extern LWLINE lwgeom_getline(char *serialized_form, int geom_number);
extern LWLINE lwgeom_getline_inspected(LWGEOM_INSPECTED *inspected, int geom_number);

// 1st geometry has geom_number = 0
// if the actual geometry isnt a POLYGON, null is returned (see _gettype()).
// if there arent enough geometries, return null.
// this is fine to call on a polygon, multipolygon or geometrycollection
extern LWPOLY lwgeom_getpoly(char *serialized_form, int geom_number);
extern LWPOLY lwgeom_getpoly_inspected(LWGEOM_INSPECTED *inspected, int geom_number);

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
extern char lwgeom_gettype(char *serialized_form, int geom_number);
extern char lwgeom_gettype_inspected(LWGEOM_INSPECTED *inspected, int geom_number);


// how many sub-geometries are there?
//  for point,line,polygon will return 1.
extern int lwgeom_getnumgeometries(char *serialized_form);
extern int lwgeom_getnumgeometries_inspected(LWGEOM_INSPECTED *inspected);


// set finalType to 0 and this will choose an appropriate type
//  if you specify a wrong type, the best type will be chosen
//   (ie. give it 2 points and ask it to be a multilinestring)
//  use SRID=-1 for unknown SRID  (will have 8bit type's S = 0)
// all subgeometries must have the same SRID
// if you want to construct an inspected, call this then inspect the result...
extern char *lwgeom_construct(int SRID,int finalType, int nsubgeometries, char **serialized_subs);


// construct the empty geometry (GEOMETRYCOLLECTION(EMPTY))
extern char *lwgeom_constructempty();

// helper function (not for general use)
// find the size a geometry (or a sub-geometry)
// 1st geometry has geom_number = 0
//  use geom_number = -1 to find the actual type of the serialized form.
//    ie lwgeom_gettype( <'MULTIPOINT(0 0, 1 1)'>, -1)
//                 --> size of the multipoint
//   ie lwgeom_gettype( <'MULTIPOINT(0 0, 1 1)'>, 0)
//                 --> size of the point
extern int lwgeom_seralizedformlength(char *serialized_form, int geom_number);
extern int lwgeom_seralizedformlength_inspected(LWGEOM_INSPECTED *inspected, int geom_number);


//get bounding box of LWGEOM (automatically calls the sub-geometries bbox generators)
extern BOX3D lw_geom_getBB(char *serialized_form);
extern BOX3D lw_geom_getBB_inspected(LWGEOM_INSPECTED *inspected);


//------------------------------------------------------
// other stuff

// handle the double-to-float conversion.  The results of this
// will usually be a slightly bigger box because of the difference
// between float8 and float4 representations.

extern BOX2DFLOAT4 box3d_to_box2df(BOX3D *box);
extern BOX2DFLOAT4 box_to_box2df(BOX *box);  // postgresql standard type

extern BOX3D box2df_to_box3d(BOX2DFLOAT4 *box);
extern BOX   box2df_to_box(BOX *box);  // postgresql standard type



//------------------------------------------------------------
//------------------------------------------------------------
// On serialized form  (see top for the 8bit type implementation)

// NOTE: contrary to the original proposal, bounding boxes are *never*
//       included in the geometry.  You must either refer to the index
//       or compute it on demand.


// The serialized form is always a stream of bytes.  The first four are always
// the memory size of the LWGEOM (including the 4 byte memory size).

// The easiest way to describe the serialed form is with examples:

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

