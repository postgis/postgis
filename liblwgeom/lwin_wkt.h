#include "libgeom.h"



/*
* Coordinate object to hold information about last coordinate temporarily.
* We need to know how many dimensions there are at any given time.
*/
typedef struct
{
	uchar flags;
	double x;
	double y;
	double z;
	double m;
}	
POINT;

/*
* Global that holds the final output geometry for the WKT parser.
*/
extern LWGEOM_PARSER_RESULT global_parser_result;

/*
* Prototypes for the lexer
*/
extern void wkt_lexer_init(char *str);
extern void wkt_lexer_close(void);


/*
* Functions called from within the bison parser to construct geometries.
*/
int wkt_lexer_read_srid(char *str);
POINT wkt_parser_coord_2(double c1, double c2);
POINT wkt_parser_coord_3(double c1, double c2, double c3);
POINT wkt_parser_coord_4(double c1, double c2, double c3, double c4);
void wkt_parser_ptarray_add_coord(POINTARRAY *pa, POINT p);
POINTARRAY* wkt_parser_ptarray_new(POINT p);
LWGEOM* wkt_parser_point_new(POINTARRAY *pa, char *dimensionality);
LWGEOM* wkt_parser_linestring_new(POINTARRAY *pa, char *dimensionality);
LWGEOM* wkt_parser_circularstring_new(POINTARRAY *pa, char *dimensionality);
LWGEOM* wkt_parser_multipoint_new(POINTARRAY *pa, char *dimensionality);
LWGEOM* wkt_parser_collection_new(LWGEOM *geom);
LWGEOM* wkt_parser_collection_finalize(int lwtype, LWGEOM *col, char *dimensionality);
void wkt_parser_collection_add_geom(LWGEOM *col, LWGEOM *geom);
void wkt_parser_geometry_new(LWGEOM *geom, int srid);