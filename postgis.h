//Geometric types for Postgis
//
// point3d, line3d, and polygon3d are the base types.
// 
// Everything is stored in a geometry3d, which is just a conglomeration
// of the base types (and a little bit of other info).



#include "utils/geo_decls.h"



#define	POINTTYPE	1
#define	LINETYPE	2
#define	POLYGONTYPE	3
#define	MULTIPOINTTYPE	4
#define	MULTILINETYPE	5
#define	MULTIPOLYGONTYPE	6
#define	COLLECTIONTYPE	7
#define	BBOXONLYTYPE	99


//standard definition of an ellipsoid (what wkt calls a spheroid)
//	f = (a-b)/a
//	e_sq = (a*a - b*b)/(a*a)
//	b = a - fa
typedef struct
{
	double	a;	//semimajor axis
	double	b; 	//semiminor axis
	double	f;	//flattening
	double	e;	//eccentricity (first)
	double	e_sq; //eccentricity (first), squared
	char		name[20]; //name of ellipse 
} SPHEROID;


/*---------------------------------------------------------------------
 * POINT3D - (x,y,z)
 *            Base type for all geometries
 *		  Also used for POINT type and MULTIPOINT type
 *-------------------------------------------------------------------*/
typedef struct
{
	double		x,y,z;  //for lat/long   x=long, y=lat
} POINT3D;

/*---------------------------------------------------------------------
 * BOX3D	- Specified by two corner points, which are
 *		 sorted to save calculation time later.
 *
 *           LLB -- lower right bottom point (ie. South West, Z low)
 *           URT -- upper right top point    (ie. North East, Z high)
 *
 *           this is the long diagonal
 *
 * example:
 *   'BOX([0.0,0.0,0.0],[10.0,10.0,10.0])'
 *
 *	NOTE:  You CAN make columns in your database of this type
 *		 IT IS NOT A TRUE GEOMETRY!
 *-------------------------------------------------------------------*/
typedef struct
{
	POINT3D		LLB,URT; /* corner POINT3Ds on long diagonal */
} BOX3D;




/*---------------------------------------------------------------------
 * LINE  - set of points (directed arcs)
 *		P1->P2->P3->P4...->Pn
 *-------------------------------------------------------------------*/
typedef struct
{
	int32 	npoints; // how many points in the line
	int32 	junk;	   // double-word alignment	
	POINT3D  	points[1]; // array of actual points
} LINE3D;

/*---------------------------------------------------------------------
 * POLYGON  - set of closed rings.  First ring in outer boundary
 *						other rings are "holes"
 *
 * NOTE: this is acually stored a bit differently.  It is more like:
 *		int32 nrings
 *		char	filler[]
 *
 *  where the 1st 4 byes of filler[] is npoints[0] the next 4 bytes
 *  are npoints[1], etc...
 *
 *  points[0] is either at filler[ 4 * nrings] 
 *            or at filler [ 4* nrings + 4]
 *		  Which ever one is double-word-aligned
 *-------------------------------------------------------------------*/
typedef struct
{
	int32 	nrings;	 // how many rings in this polygon
	int32		npoints[1]; //how many points in each ring
	/* could be 4 byes of filler here to make sure points[] is 
         double-word aligned*/
	POINT3D  	points[1]; // array of actual points
} POLYGON3D;


/*---------------------------------------------------------------------
 * Geometry - all columns in postgres are of this type
 *
 *		Geometries are collections of simple geometry types
 *		(point, line, polygon).
 *	
 *		A Point is a geometry with a single point in it.
 *		A MultiPoint is a geometry with a list of 'point' in it.
 *		A Line is a geometry with a single line in it.
 *		A MultiLine is a geometry with a list of 'line' in it.
 *		A Polygon is a geometry with a single polygon in it.
 *		A MultiPolygon is a geometry with a list of 'polygon' in it.
 *		A Collection is a geometry with a (mixed) list of 
 *				point, line, and polygon.
 *
 * 		The bvol is the bounding volume of all the subobjects.
 *		
 *		is3d is true if the original data was sent in a 3d data.
 *		2d data is 3d data with z=0.0.
 *
 *	'type' has values:
 *		Point			-> POINTTYPE
 *		MultiPoint		-> MULTIPOINTTYPE
 * 		Line			-> LINETYPE
 * 		MultiLine		-> MULTILINETYPE
 *		Polygon		-> POLYGONTYPE
 *		MultiPolygon	-> MULTIPOLYGONTYPE
 *		Collection		-> COLLECTIOMTYPE
 *
 *	'objType' has values:
 *		Point			-> POINTTYPE
 * 		Line			-> LINETYPE
 *		Polygon		-> POLYGONTYPE
 *
 *	'objOffset' is an offset (in bytes) into this structure of where
 *	the subobject is defined.  THESE ARE ALWAYS DOUBLE-WORD ALIGNED.
 *
 *
 *	In reality the structure looks like:
 *		int32 	size;
 *		int32		type;	
 * 		bool		is3d;
 *		<there is almost certainly some type of padding here>
 *		BOX3D		bvol;	
 *		int32		nobjs;
 *		char		data[...];
 *
 * 	AND:
 *		&objType[0] = &data[0]
 *		&objType[1] = &data[4] 
 *			...
 *		&obgOffset[0] = &data[ 4* nobjs]
 *		&obgOffset[1] = &data[ 4* nobjs + 4]
 *			...
 *		&objData[0] = &GEOMETRY + objOffset[0]  //always double-word aligned
 *		&objData[1] = &GEOMETRY + objOffset[1]  //always double-word aligned
 *			...	
 *
 * 	ALL GEOMETRY COLUMNS IN YOUR DATABASE SHOULD BE OF THIS TYPE
 *-------------------------------------------------------------------*/
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


typedef struct chiptag
{
	int	size; //unused (for use by postgresql)

	int	endian_hint;  // the number '1' in the endian of this datastruct

	BOX3D	 bvol;
	int	 SRID;
	char	 future[4];	
	float  factor;           //usually 1.0.  Integer values are multiplied by this number
                              //to get the actual height value (for sub-meter accuracy
					//height data).

	int 	datatype;	 // 1 = float32, 5 = 24bit integer, 6 = 16bit integer (short)
				 // 101 = float32 (NDR), 105 = 24bit integer (NDR), 106=16bit int (NDR)
	int 	height;
	int 	width;
	int   compression;	//# 0 = no compression, 1 = differencer
								//     0x80 = new value
								//     0x7F = nodata

		// this is provided for convenience, it should be set to 
		//  sizeof(chip) bytes into the struct because the serialized form is:
		//    <header><data>
		// NULL when serialized	
	void  *data;	 	// data[0] = bottm left, data[width] = 1st pixel, 2nd row (uncompressed)
} CHIP;


//for GiST indexing

//This is the BOX type from geo_decls.h
// Its included here for the GiST index. 
//  Originally, we used BOXONLYTYPE geometries as our keys, but after
//   Oleg and teodor (http://www.sai.msu.su/~megera/postgres/gist/)
//   have released a more generic rtree/gist index for geo_decls.h polygon
//   type.  I'm using a slightly modified version of this, so 
//   it will be easier to maintain.
//
//   Their indexing is based on the BOX object, so we include it here.


// ONLY FOR INDEXING
typedef struct geomkey {
	int32 size; /* size in varlena terms */ 
	BOX	key;
	int32	SRID; //spatial reference system identifier
} GEOMETRYKEY; 


// WKB structure  -- exactly the same as TEXT
typedef struct Well_known_bin {
	int32 size;    // total size of this structure
	unsigned char  data[1]; //THIS HOLD VARIABLE LENGTH DATA
} WellKnownBinary;


//prototypes 

     int isspace(int c);


/* constructors*/
POLYGON3D	*make_polygon(int nrings, int *pts_per_ring, POINT3D *pts, int npoints, int *size);
void set_point( POINT3D *pt,double x, double y, double z);
GEOMETRY	*make_oneobj_geometry(int sub_obj_size, char *sub_obj, int type, bool is3d, int SRID,double scale, double offx,double offy);

void print_box(BOX3D *box);
void print_box_oneline(BOX3D *box);
void print_point(char *result, POINT3D *pt,bool is3d);
void print_many_points(char *result, POINT3D *pt ,int npoints, bool is3d);
void swap(double *d1, double *d2);
int	numb_points_in_list(char	*str);
bool	parse_points_in_list(char	*str, POINT3D	*points, int32	max_points, bool *is3d);
bool	parse_points_in_list_exact(char	*str, POINT3D	*points, int32	max_points, bool *is3d);
int	find_outer_list_length(char *str);
bool	points_per_sublist( char *str, int32 *npoints, int32 max_lists);
char	*scan_to_same_level(char	*str);
int objects_inside_point(char *str);
int objects_inside_line(char *str);
int objects_inside_polygon(char *str);
int objects_inside_multipoint(char *str);
int objects_inside_multiline(char *str);
int objects_inside_multipolygon(char *str);
int objects_inside_collection(char *str);
int objects_inside(char *str);
bool parse_objects_inside_point(int32 *obj_size,char **objs, int32 *obj_types, int32 nobjs, char *str, int *offset, bool* is3d);
bool parse_objects_inside_multipoint(int32 *obj_size,char **objs, int32 *obj_types, int32 nobjs, char *str, int *offset, bool* is3d);
bool parse_objects_inside_line(int32 *obj_size,char **objs, int32 *obj_types, int32 nobjs, char *str, int *offset, bool* is3d);
bool parse_objects_inside_multiline(int32 *obj_size,char **objs, int32 *obj_types, int32 nobjs, char *str, int *offset, bool* is3d);
bool parse_objects_inside_polygon(int32 *obj_size,char **objs, int32 *obj_types, int32 nobjs, char *str, int *offset, bool* is3d);
bool parse_objects_inside_multipolygon(int32 *obj_size,char **objs, int32 *obj_types, int32 nobjs, char *str, int *offset, bool* is3d);
bool parse_objects(int32 *obj_size,char **objs,int32	*obj_types,int32 nobjs,char *str, int *offset, bool *is3d);
bool parse_objects_inside_collection(int32 *obj_size,char **objs, int32 *obj_types, int32 nobjs, char *str, int *offset, bool* is3d);
BOX3D	*bbox_of_point(POINT3D *pt);
BOX3D	*bbox_of_polygon(POLYGON3D *polygon);
BOX3D	*bbox_of_line(LINE3D *line);
BOX3D *union_box3d(BOX3D *a, BOX3D *b);
BOX3D	*bbox_of_geometry(GEOMETRY *geom);
GEOMETRY *make_bvol_geometry(BOX3D *box);
 bool box3d_ov(BOX3D *box1, BOX3D *box2);
bool is_same_point(POINT3D	*p1, POINT3D	*p2);
bool is_same_line(LINE3D	*l1, LINE3D	*l2);
bool is_same_polygon(POLYGON3D	*poly1, POLYGON3D	*poly2);
BOX	*convert_box3d_to_box(BOX3D *in);
double line_length2d(LINE3D *line);
double line_length3d(LINE3D *line);
double polygon_area2d_old(POLYGON3D *poly1);
double 	polygon_perimeter3d(POLYGON3D	*poly1);
double 	polygon_perimeter2d(POLYGON3D	*poly1);
int PIP( POINT3D *P, POINT3D *V, int n );
bool point_truely_inside(POINT3D	*point, BOX3D	*box);
int	compute_outcode( POINT3D *p, BOX3D *box);
bool lineseg_inside_box( POINT3D *P1, POINT3D *P2, BOX3D *box);
bool	linestring_inside_box(POINT3D *pts, int npoints, BOX3D *box);
bool polygon_truely_inside(POLYGON3D	*poly, BOX3D *box);
bool line_truely_inside( LINE3D *line, BOX3D 	*box);
void	translate_points(POINT3D *pt, int npoints,double x_off, double y_off, double z_off);
int	size_subobject (char *sub_obj, int type);
GEOMETRY	*add_to_geometry(GEOMETRY *geom,int sub_obj_size, char *sub_obj, int type);
LINE3D	*make_line(int	npoints, POINT3D	*pts, int	*size);
char  *print_geometry(GEOMETRY *geom);

void  swap_char(char *a, char*b);
void	flip_endian_double(char	*dd);
void		flip_endian_int32(char 	*ii);

char	*to_wkb(GEOMETRY *geom, bool flip_endian);
char	*wkb_multipolygon(POLYGON3D	**polys,int numb_polys,int32 *size, bool flipbytes, char byte_order,bool use3d);
char	*wkb_polygon(POLYGON3D	*poly,int32 *size, bool flipbytes, char byte_order,bool use3d, char *mem);
char	*wkb_multiline(LINE3D **lines,int32 *size, int numb_lines, bool flipbytes, char byte_order,bool use3d);
char	*wkb_line(LINE3D *line,int32 *size, bool flipbytes, char byte_order,bool use3d, char *mem);
char	*wkb_point(POINT3D *pt,int32 *size, bool flipbytes, char byte_order, bool use3d);
char	*wkb_multipoint(POINT3D *pt,int32 numb_points,int32 *size, bool flipbytes, char byte_order,bool use3d);

char *to_wkb_collection(GEOMETRY *geom, bool flip_endian, int32 *size);
char	*to_wkb_sub(GEOMETRY *geom, bool flip_endian, int32 *wkb_size);

void decode_wkb_collection(char *wkb,int	*size);
void decode_wkb(char *wkb, int *size);
void dump_bytes( char *a, int numb);

double deltaLongitude(double azimuth, double sigma, double tsm,SPHEROID *sphere);
double bigA(double u2);
double bigB(double u2);
double	distance_ellipse(double lat1, double long1,
					double lat2, double long2,
					SPHEROID *sphere);
double mu2(double azimuth,SPHEROID *sphere);

double length2d_ellipse_linestring(LINE3D	*line, SPHEROID  	*sphere);
double length3d_ellipse_linestring(LINE3D	*line, SPHEROID  	*sphere);

double distance_pt_pt(POINT3D *p1, POINT3D *p2);
double distance_pt_line(POINT3D *p1, LINE3D *l2);
double distance_pt_poly(POINT3D *p1, POLYGON3D *poly2);
double distance_line_line(LINE3D *l1, LINE3D *l2);
double distance_line_poly(LINE3D *l1, POLYGON3D *poly2);
double distance_poly_poly(POLYGON3D *poly1, POLYGON3D *poly2);
double distance_pt_seg(POINT3D *p, POINT3D *A, POINT3D *B);
double distance_seg_seg(POINT3D *A, POINT3D *B, POINT3D *C, POINT3D *D);
bool point_in_poly(POINT3D *p, POLYGON3D *poly);

POINT3D	*segmentize_ring(POINT3D	*points, double dist, int num_points_in, int *num_points_out);


Datum optimistic_overlap(PG_FUNCTION_ARGS);


void print_point_debug(POINT3D *p);
unsigned char	parse_hex(char *str);
void deparse_hex(unsigned char str, unsigned char *result);


char *geometry_to_text(GEOMETRY  *geometry);




//exposed to psql

Datum box3d_in(PG_FUNCTION_ARGS);
Datum box3d_out(PG_FUNCTION_ARGS);
Datum geometry_in(PG_FUNCTION_ARGS);
Datum geometry_out(PG_FUNCTION_ARGS);

Datum astext_geometry(PG_FUNCTION_ARGS);

Datum geometry_text(PG_FUNCTION_ARGS);


Datum get_bbox_of_geometry(PG_FUNCTION_ARGS);
Datum get_geometry_of_bbox(PG_FUNCTION_ARGS);
Datum box3d_same(PG_FUNCTION_ARGS);
Datum geometry_overleft(PG_FUNCTION_ARGS);
Datum geometry_left(PG_FUNCTION_ARGS);
Datum geometry_right(PG_FUNCTION_ARGS);
Datum geometry_overright(PG_FUNCTION_ARGS);
Datum geometry_contained(PG_FUNCTION_ARGS);
Datum geometry_contain(PG_FUNCTION_ARGS);
Datum geometry_overlap(PG_FUNCTION_ARGS);
Datum geometry_same(PG_FUNCTION_ARGS);
Datum box3d_overlap(PG_FUNCTION_ARGS);
Datum box3d_overleft(PG_FUNCTION_ARGS);
Datum box3d_right(PG_FUNCTION_ARGS);
Datum box3d_contain(PG_FUNCTION_ARGS);
Datum geometry_union(PG_FUNCTION_ARGS);
Datum geometry_inter(PG_FUNCTION_ARGS);
Datum geometry_size(PG_FUNCTION_ARGS);
Datum length3d(PG_FUNCTION_ARGS);
Datum length2d(PG_FUNCTION_ARGS);
Datum area2d(PG_FUNCTION_ARGS);
Datum perimeter3d(PG_FUNCTION_ARGS);
Datum perimeter2d(PG_FUNCTION_ARGS);
Datum truly_inside(PG_FUNCTION_ARGS);

Datum geometry_lt(PG_FUNCTION_ARGS);
Datum geometry_gt(PG_FUNCTION_ARGS);
Datum geometry_eq(PG_FUNCTION_ARGS);

Datum npoints(PG_FUNCTION_ARGS);
Datum nrings(PG_FUNCTION_ARGS);
Datum mem_size(PG_FUNCTION_ARGS);
Datum numb_sub_objs(PG_FUNCTION_ARGS);
Datum summary(PG_FUNCTION_ARGS);
Datum translate(PG_FUNCTION_ARGS);

Datum asbinary_specify(PG_FUNCTION_ARGS);
Datum asbinary_simple(PG_FUNCTION_ARGS);

Datum force_2d(PG_FUNCTION_ARGS);
Datum force_3d(PG_FUNCTION_ARGS);
Datum force_collection(PG_FUNCTION_ARGS);

Datum combine_bbox(PG_FUNCTION_ARGS);

Datum dimension(PG_FUNCTION_ARGS);
Datum geometrytype(PG_FUNCTION_ARGS);
Datum envelope(PG_FUNCTION_ARGS);

Datum x_point(PG_FUNCTION_ARGS);
Datum y_point(PG_FUNCTION_ARGS);
Datum z_point(PG_FUNCTION_ARGS);

Datum numpoints_linestring(PG_FUNCTION_ARGS);
Datum pointn_linestring(PG_FUNCTION_ARGS);

Datum exteriorring_polygon(PG_FUNCTION_ARGS);
Datum numinteriorrings_polygon(PG_FUNCTION_ARGS);
Datum interiorringn_polygon(PG_FUNCTION_ARGS);

Datum numgeometries_collection(PG_FUNCTION_ARGS);
Datum geometryn_collection(PG_FUNCTION_ARGS);

Datum ellipsoid_out(PG_FUNCTION_ARGS);
Datum ellipsoid_in(PG_FUNCTION_ARGS);
Datum length_ellipsoid(PG_FUNCTION_ARGS);
Datum length3d_ellipsoid(PG_FUNCTION_ARGS);

Datum point_inside_circle(PG_FUNCTION_ARGS);
Datum distance(PG_FUNCTION_ARGS);

Datum expand_bbox(PG_FUNCTION_ARGS);
Datum srid_geom(PG_FUNCTION_ARGS);
Datum geometry_from_text(PG_FUNCTION_ARGS);

Datum startpoint(PG_FUNCTION_ARGS);

Datum endpoint(PG_FUNCTION_ARGS);
Datum isclosed(PG_FUNCTION_ARGS);

Datum centroid(PG_FUNCTION_ARGS);

Datum postgis_gist_sel(PG_FUNCTION_ARGS);

Datum WKB_in(PG_FUNCTION_ARGS);
Datum WKB_out(PG_FUNCTION_ARGS);

Datum CHIP_in(PG_FUNCTION_ARGS);
Datum CHIP_out(PG_FUNCTION_ARGS);
Datum CHIP_to_geom(PG_FUNCTION_ARGS);
Datum srid_chip(PG_FUNCTION_ARGS);
Datum setsrid_chip(PG_FUNCTION_ARGS);
Datum width_chip(PG_FUNCTION_ARGS);
Datum height_chip(PG_FUNCTION_ARGS);
Datum datatype_chip(PG_FUNCTION_ARGS);
Datum compression_chip(PG_FUNCTION_ARGS);
Datum setfactor_chip(PG_FUNCTION_ARGS);
Datum factor_chip(PG_FUNCTION_ARGS);


Datum segmentize(PG_FUNCTION_ARGS);



Datum transform_geom(PG_FUNCTION_ARGS);

Datum max_distance(PG_FUNCTION_ARGS);


//for GIST index
typedef char* (*BINARY_UNION)(char*, char*, int*);
typedef float (*SIZE_BOX)(char*);
typedef Datum (*RDF)(PG_FUNCTION_ARGS);





GISTENTRY * ggeometry_compress(PG_FUNCTION_ARGS);
GEOMETRYKEY *ggeometry_union(PG_FUNCTION_ARGS);
GIST_SPLITVEC * ggeometry_picksplit(PG_FUNCTION_ARGS);
bool ggeometry_consistent(PG_FUNCTION_ARGS);
float * ggeometry_penalty(PG_FUNCTION_ARGS);
bool * ggeometry_same(PG_FUNCTION_ARGS);

char * ggeometry_binary_union(char *r1, char *r2, int *sizep);
float size_geometrykey( char *pk );

Datum ggeometry_inter(PG_FUNCTION_ARGS);

/*
** Common rtree-function (for all ops)
*/
char * rtree_union(bytea *entryvec, int *sizep, BINARY_UNION bu);
float * rtree_penalty(GISTENTRY *origentry, GISTENTRY *newentry, float *result, BINARY_UNION bu, SIZE_BOX sb);
GIST_SPLITVEC * rtree_picksplit(bytea *entryvec, GIST_SPLITVEC *v, int keylen, BINARY_UNION bu, RDF interop, SIZE_BOX sb);
bool rtree_internal_consistent(BOX *key, BOX *query, StrategyNumber strategy);


GISTENTRY * rtree_decompress(PG_FUNCTION_ARGS);



/*--------------------------------------------------------------------
 * Useful floating point utilities and constants.
 * from postgres's geo_decls.c 
 * EPSILON modified to be more "double" friendly
 *-------------------------------------------------------------------*/



// from contrib/cube/cube.c

#define max(a,b)		((a) >	(b) ? (a) : (b)) 
#define min(a,b)		((a) <= (b) ? (a) : (b))
#define abs(a)			((a) <	(0) ? (-a) : (a))


