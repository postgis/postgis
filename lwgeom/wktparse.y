/*
 * Written by Ralph Mason ralph.mason<at>telogis.com
 *
 * Copyright Telogis 2004
 * www.telogis.com
 *
 */

%{
#include "wktparse.h"
#include <unistd.h>
#include <stdio.h>

void set_zm(char z, char m);
int lwg_parse_yylex(void);
%}

%start geometry

%union {
	double value;
	const char* wkb;
}

%token POINT LINESTRING POLYGON MULTIPOINT MULTILINESTRING MULTIPOLYGON GEOMETRYCOLLECTION
%token POINTM LINESTRINGM POLYGONM MULTIPOINTM MULTILINESTRINGM MULTIPOLYGONM GEOMETRYCOLLECTIONM
%token SRID      
%token EMPTY
%token <value> VALUE
%token LPAREN RPAREN COMMA EQUALS SEMICOLON
%token  <wkb> WKB

%%

geometry : srid  SEMICOLON  { alloc_lwgeom(srid); } geometry_int 
	| { alloc_lwgeom(-1); } geometry_int	;

geometry_int : geom_wkb | geom_point | geom_linestring | geom_polygon | geom_multipoint 
	| geom_multilinestring | geom_multipolygon| geom_geometrycollection;

srid : SRID EQUALS VALUE { set_srid($3); } ; 

geom_wkb : WKB {alloc_wkb($1) ; } ;


/* POINT */

geom_point : POINT point | POINTM { set_zm(0, 1); } point ;

point : { alloc_point(); } point_int { pop();} ;

point_int : empty | LPAREN a_point RPAREN;

/* MULTIPOINT */

geom_multipoint : MULTIPOINT { alloc_multipoint(); } multipoint  { pop();} | 
	MULTIPOINTM { set_zm(0, 1); alloc_multipoint(); } multipoint {pop();};

multipoint : empty | { alloc_counter(); } LPAREN multipoint_int RPAREN {pop();} ;

multipoint_int : mpoint | multipoint_int COMMA mpoint;

mpoint : point  |  { alloc_point(); } a_point { pop();} ;


/* LINESTRING */

geom_linestring : LINESTRING linestring | LINESTRINGM { set_zm(0, 1); } linestring ;

linestring : { alloc_linestring(); } linestring_1 {pop();} ;

linestring_1 : empty | { alloc_counter(); } LPAREN linestring_int RPAREN {popc(); };

linestring_int : a_point | linestring_int COMMA a_point;

/* MULTILINESTRING */

geom_multilinestring : MULTILINESTRING { alloc_multilinestring(); } multilinestring  { pop();} | MULTILINESTRINGM { set_zm(0, 1); alloc_multilinestring(); } multilinestring { pop(); } ;

multilinestring : empty | { alloc_counter(); } LPAREN multilinestring_int RPAREN{ pop();} ;

multilinestring_int : linestring | multilinestring_int COMMA linestring;


/* POLYGON */

geom_polygon : POLYGON polygon | POLYGONM { set_zm(0, 1); } polygon ;

polygon : { alloc_polygon(); } polygon_1  { pop();} ;

polygon_1 : empty | { alloc_counter(); } LPAREN polygon_int RPAREN { pop();} ;

polygon_int : linestring_1 | polygon_int COMMA linestring_1;
                                                                                                          
/* MULTIPOLYGON */

geom_multipolygon : MULTIPOLYGON { alloc_multipolygon(); } multipolygon   { pop();} | MULTIPOLYGONM { set_zm(0, 1); alloc_multipolygon(); } multipolygon  { pop();} ;

multipolygon : empty | { alloc_counter(); } LPAREN multipolygon_int RPAREN { pop();} ; 

multipolygon_int : polygon | multipolygon_int COMMA polygon;



/* GEOMETRYCOLLECTION */

geom_geometrycollection : GEOMETRYCOLLECTION { alloc_geomertycollection(); } geometrycollection { pop();} | GEOMETRYCOLLECTIONM { set_zm(0, 1); alloc_geomertycollection(); } geometrycollection { pop();} ;

geometrycollection : empty | { alloc_counter(); } LPAREN geometrycollection_int RPAREN { pop();} ;

geometrycollection_int  : empty | geometry_int | geometrycollection_int  COMMA geometry_int;


a_point : point_2d | point_3d | point_4d ;

point_2d : VALUE VALUE {alloc_point_2d($1,$2); };

point_3d : VALUE VALUE VALUE {alloc_point_3d($1,$2,$3); };

point_4d : VALUE VALUE VALUE VALUE {alloc_point_4d($1,$2,$3,$4); };

empty : EMPTY  {alloc_empty(); };
%%




