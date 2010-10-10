%{

/* WKT Parser */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "g_wkt_parse_utils.h"


char *wkt_yyerror_str = NULL;
int wkt_yydebug = 1;
 
void wkt_yyerror(const char *str)
{
	if ( wkt_yyerror_str ) free(wkt_yyerror_str);
	asprintf(&wkt_yyerror_str,"Parse error: %s",str);
}
 
int wkt_yywrap()
{
	return 1;
} 

/*



*/
extern int ggeometry_from_wkt_string(char *str, G_GEOMETRY **geom, char **errstr)
{
	int rv = 0;

	*geom = NULL;
	*errstr = NULL;
	wkt_lexer_init(str);
	rv = wkt_yyparse();
	LWDEBUGF(4,"wkt_yyparse returned %d", rv);
	wkt_lexer_close();
	if ( rv ) 
	{
		*errstr = wkt_yyerror_str;
		return G_FAILURE;
	}
	*geom = globalgeom;
	globalgeom = NULL;
	return G_SUCCESS;
}

%}

%error-verbose
%name-prefix="wkt_yy"

%union {
	integer integervalue;
	double doublevalue;
	char *stringvalue;
	LWGEOM *geometryvalue;
	POINT coordinatevalue;
	POINTARRAY *ptarrayvalue;
}

%token POINT_TOK LINESTRING_TOK POLYGON_TOK 
%token MPOINT_TOK MLINESTRING_TOK MPOLYGON_TOK 
%token MSURFACE_TOK MCURVE_TOK CURVEPOLYGON_TOK COMPOUNDCURVE_TOK CIRCULARSTRING_TOK
%token COLLECTION_TOK 
%token RBRACKET_TOK LBRACKET_TOK COMMA_TOK EMPTY_TOK
%token SRID_TOK SEMICOLON_TOK

%token <doublevalue> DOUBLE_TOK
%token <integervalue> INTEGER_TOK
%token <stringvalue> STRING_TOK
%token <stringvalue> DIMENSIONALITY_TOK

%type <geometryvalue> geometry
%type <geometryvalue> geometry_no_srid
%type <geometryvalue> geometrycollection
%type <geometryvalue> multisurface
%type <geometryvalue> multicurve
%type <geometryvalue> curvepolygon
%type <geometryvalue> compoundcurve
%type <geometryvalue> geometry_list
%type <geometryvalue> surface_list
%type <geometryvalue> polygon_list
%type <geometryvalue> curve_list
%type <geometryvalue> linestring_list
%type <geometryvalue> curvering_list
%type <geometryvalue> ring_list
%type <geometryvalue> point
%type <geometryvalue> circularstring
%type <geometryvalue> linestring
%type <geometryvalue> linestring_untagged
%type <geometryvalue> ring
%type <geometryvalue> polygon
%type <geometryvalue> polygon_untagged
%type <geometryvalue> multipoint
%type <geometryvalue> multilinestring
%type <geometryvalue> multipolygon
%type <ptarrayvalue> ptarray
%type <coordinatevalue> coordinate

%%

geometry:
	geometry_no_srid {} |
	SRID_TOK integer SEMICOLON_TOK geometry_no_srid {} ;

geometry_no_srid : 
	point {} | 
	linestring {} | 
	circularstring {} | 
	compoundcurve {} | 
	polygon {} | 
	curvepolygon {} | 
	multipoint {} |
	multilinestring {} | 
	multipolygon {} |
	multisurface {} |
	multicurve {} |
	geometrycollection {} ;
	
geometrycollection :
	COLLECTION_TOK LBRACKET_TOK geometry_list RBRACKET_TOK {} |
	COLLECTION_TOK DIMENSIONALITY_TOK LBRACKET_TOK geometry_list RBRACKET_TOK {} |
	COLLECTION_TOK EMPTY_TOK {} ;
	
geometry_list :
	geometry_list COMMA_TOK geometry {} |
	geometry {} ;

multisurface :
	MSURFACE_TOK LBRACKET_TOK surface_list RBRACKET_TOK {} |
	MSURFACE_TOK DIMENSIONALITY_TOK LBRACKET_TOK surface_list RBRACKET_TOK {} |
	MSURFACE_TOK EMPTY_TOK {} ;
	
surface_list :
	surface_list COMMA_TOK polygon {} |
	surface_list COMMA_TOK curvepolygon {} |
	surface_list COMMA_TOK polygon_untagged {} |
	polygon {} |
	curvepolygon {} |
	polygon_untagged {} ;

multipolygon :
	MPOLYGON_TOK LBRACKET_TOK polygon_list RBRACKET_TOK {} |
	MPOLYGON_TOK DIMENSIONALITY_TOK LBRACKET_TOK polygon_list RBRACKET_TOK {} |
	MPOLYGON_TOK EMPTY_TOK {} ;

polygon_list :
	polygon_list COMMA_TOK polygon_untagged {} |
	polygon_untagged {} ;

polygon : 
	POLYGON_TOK LBRACKET_TOK ring_list RBRACKET_TOK {} |
	POLYGON_TOK DIMENSIONALITY_TOK LBRACKET_TOK ring_list RBRACKET_TOK {} |
	POLYGON_TOK EMPTY_TOK {} ;

polygon_untagged : 
	LBRACKET_TOK ring_list RBRACKET_TOK {} ;

curvepolygon :
	CURVEPOLYGON_TOK LBRACKET_TOK curvering_list RBRACKET_TOK {} |
	CURVEPOLYGON_TOK DIMENSIONALITY_TOK LBRACKET_TOK curvering_list RBRACKET_TOK {} |
	CURVEPOLYGON_TOK EMPTY_TOK {} ;

curvering_list :
	curvering_list COMMA_TOK ring {} |
	curvering_list COMMA_TOK circularstring {} |
	ring {} |
	circularstring {} ;

ring_list :
	ring_list COMMA_TOK ring {} |
	ring {} ;

ring :
	LBRACKET_TOK ptarray RBRACKET_TOK {} ;

compoundcurve :
	COMPOUNDCURVE_TOK LBRACKET_TOK curve_list RBRACKET_TOK {} |
	COMPOUNDCURVE_TOK DIMENSIONALITY_TOK LBRACKET_TOK curve_list RBRACKET_TOK {} |
	COMPOUNDCURVE_TOK EMPTY_TOK {} ;
	
multicurve :
	MCURVE_TOK LBRACKET_TOK curve_list RBRACKET_TOK {} |
	MCURVE_TOK DIMENSIONALITY_TOK LBRACKET_TOK curve_list RBRACKET_TOK {} |
	MCURVE_TOK EMPTY_TOK {} ;

curve_list :
	curve_list COMMA_TOK circularstring {} |
	curve_list COMMA_TOK linestring {} |
	curve_list COMMA_TOK linestring_untagged {} |
	circularstring {} |
	linestring {} |
	linestring_untagged {} ;

multilinestring :
	MLINESTRING_TOK LBRACKET_TOK linestring_list RBRACKET_TOK {} |
	MLINESTRING_TOK DIMENSIONALITY_TOK LBRACKET_TOK linestring_list RBRACKET_TOK {} |
	MLINESTRING_TOK EMPTY_TOK {} ;

linestring_list :
	linestring_list COMMA_TOK linestring_untagged {} |
	linestring_untagged {} ;

circularstring : 
	CIRCULARSTRING_TOK LBRACKET_TOK ptarray RBRACKET_TOK {} |
	CIRCULARSTRING_TOK DIMENSIONALITY_TOK LBRACKET_TOK ptarray RBRACKET_TOK {} |
	CIRCULARSTRING_TOK EMPTY_TOK {} ;

linestring : 
	LINESTRING_TOK LBRACKET_TOK ptarray RBRACKET_TOK  
		{ $$ = wkt_parser_linestring($3, NULL); } | 
	LINESTRING_TOK DIMENSIONALITY_TOK LBRACKET_TOK ptarray RBRACKET_TOK  
		{ $$ = wkt_parser_linestring($4, $2); } |
	LINESTRING_TOK DIMENSIONALITY_TOK EMPTY_TOK  
		{ $$ = wkt_parser_linestring(NULL, $2); } |
	LINESTRING_TOK EMPTY_TOK  
		{ $$ = wkt_parser_linestring(NULL, NULL); } ;

linestring_untagged :
	LBRACKET_TOK ptarray RBRACKET_TOK 
		{ $$ = wkt_parser_linestring($2); } ;

multipoint :
	MPOINT_TOK LBRACKET_TOK ptarray RBRACKET_TOK {} |
	MPOINT_TOK DIMENSIONALITY_TOK LBRACKET_TOK ptarray RBRACKET_TOK {} |
	MPOINT_TOK EMPTY_TOK {} ;

	point : 
		POINT_TOK LBRACKET_TOK coordinate RBRACKET_TOK {} |
		POINT_TOK DIMENSIONALITY_TOK LBRACKET_TOK coordinate RBRACKET_TOK {} |
		POINT_TOK EMPTY_TOK {} ;

ptarray : 
	ptarray COMMA_TOK coordinate 
		{ wkt_parser_ptarray_add_coord($$, $3); } |
	coordinate 
		{ $$ = wkt_parser_ptarray_new(FLAGS_NDIMS(($1).flags); wkt_parser_ptarray_add_coord($$, $1); } ;

coordinate : 
	DOUBLE_TOK DOUBLE_TOK 
		{ $$ = wkt_parser_coord_2($1, $2); } | 
	DOUBLE_TOK DOUBLE_TOK DOUBLE_TOK 
		{ $$ = wkt_parser_coord_3($1, $2, $3); } | 
	DOUBLE_TOK DOUBLE_TOK DOUBLE_TOK DOUBLE_TOK 
		{ $$ = wkt_parser_coord_4($1, $2, $3, $4); } ;

%%

