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

%locations

%union {
	double value;
	const char* wkb;
}

%token POINT LINESTRING POLYGON MULTIPOINT MULTILINESTRING MULTIPOLYGON GEOMETRYCOLLECTION CIRCULARSTRING COMPOUNDCURVE CURVEPOLYGON MULTICURVE MULTISURFACE
%token POINTM LINESTRINGM POLYGONM MULTIPOINTM MULTILINESTRINGM MULTIPOLYGONM GEOMETRYCOLLECTIONM CIRCULARSTRINGM COMPOUNDCURVEM CURVEPOLYGONM  MULTICURVEM MULTISURFACEM
%token SRID      
%token EMPTY
%token <value> VALUE
%token LPAREN RPAREN COMMA EQUALS SEMICOLON
%token  <wkb> WKB

%%

geometry :
	srid  SEMICOLON  { alloc_lwgeom(srid); } geometry_int 
	|
	{ alloc_lwgeom(-1); } geometry_int

geometry_int :
	geom_wkb
	|
	geom_point
	|
	geom_linestring
	|
        geom_circularstring
        |
	geom_polygon
	|
        geom_compoundcurve
        |
        geom_curvepolygon
        |
	geom_multipoint 
	|
	geom_multilinestring
        |
        geom_multicurve
	|
	geom_multipolygon
	|
        geom_multisurface
        |
	geom_geometrycollection

srid :
	SRID EQUALS VALUE { set_srid($3); } 

geom_wkb :
	WKB { alloc_wkb($1); }


/* POINT */

geom_point :
	POINT point
	|
	POINTM { set_zm(0, 1); } point 

point :
	empty_point
	|
	nonempty_point

empty_point :
	{ alloc_point(); } empty { pop(); } 

nonempty_point :
	{ alloc_point(); } point_int { pop(); } 

point_int :
	LPAREN a_point RPAREN

/* MULTIPOINT */

geom_multipoint :
	MULTIPOINT { alloc_multipoint(); } multipoint  { pop(); }
	| 
	MULTIPOINTM { set_zm(0, 1); alloc_multipoint(); } multipoint {pop(); }

multipoint :
	empty
	|
	{ alloc_counter(); } LPAREN multipoint_int RPAREN { pop(); } 

multipoint_int :
	mpoint_element
	|
	multipoint_int COMMA mpoint_element

mpoint_element :
	nonempty_point
	|
	/* this is to allow MULTIPOINT(0 0, 1 1) */
	{ alloc_point(); } a_point { pop(); }


/* LINESTRING */

geom_linestring :
	LINESTRING linestring
	|
	LINESTRINGM { set_zm(0, 1); } linestring

linestring :
	empty_linestring
	|
	nonempty_linestring

empty_linestring :
	{ alloc_linestring(); } empty { pop(); } 

nonempty_linestring :
	{ alloc_linestring(); } linestring_1 { pop(); } 

nonempty_linestring_closed :
        { alloc_linestring_closed(); } linestring_1 { pop(); }

linestring_1 :
	{ alloc_counter(); } LPAREN linestring_int RPAREN { popc(); }

linestring_int :
	a_point
	|
	linestring_int COMMA a_point;

/* CIRCULARSTRING */

geom_circularstring :
        CIRCULARSTRING circularstring
        |
        CIRCULARSTRINGM {set_zm(0, 1); } circularstring

geom_circularstring_closed :
        CIRCULARSTRING circularstring_closed
        |
        CIRCULARSTRINGM {set_zm(0, 1); } circularstring_closed

circularstring :
        empty_circularstring
        |
        nonempty_circularstring

circularstring_closed :
        empty_circularstring
        |
        nonempty_circularstring_closed

empty_circularstring :
        { alloc_circularstring(); } empty { pop(); }

nonempty_circularstring :
        { alloc_circularstring(); } circularstring_1 { pop(); }

nonempty_circularstring_closed :
        { alloc_circularstring_closed(); } circularstring_1 { pop(); }

circularstring_1 :
        { alloc_counter(); } LPAREN circularstring_int RPAREN { popc(); }

circularstring_int :
        a_point
        |
        circularstring_int COMMA a_point;

/* COMPOUNDCURVE */

geom_compoundcurve:
        COMPOUNDCURVE { alloc_compoundcurve(); } compoundcurve { pop(); }
        |
        COMPOUNDCURVEM {set_zm(0, 1); alloc_compoundcurve(); } compoundcurve { pop(); }

compoundcurve:
        empty
        |
        { alloc_counter(); } LPAREN compoundcurve_int RPAREN { pop(); }

compoundcurve_int:
        nonempty_linestring
        |
        geom_circularstring
        |
        compoundcurve_int COMMA nonempty_linestring
        |
        compoundcurve_int COMMA geom_circularstring

/* MULTILINESTRING */

geom_multilinestring :
	MULTILINESTRING { alloc_multilinestring(); }
		multilinestring  { pop(); }
	|
	MULTILINESTRINGM { set_zm(0, 1); alloc_multilinestring(); }
		multilinestring { pop(); } 

multilinestring :
	empty
	|
	{ alloc_counter(); } LPAREN multilinestring_int RPAREN{ pop();}

multilinestring_int :
	nonempty_linestring
	|
	multilinestring_int COMMA nonempty_linestring

/* MULTICURVESTRING */

geom_multicurve :
        MULTICURVE { alloc_multicurve(); }
                multicurve { pop(); }
        |
        MULTICURVEM { set_zm(0, 1); alloc_multicurve(); }
                multicurve { pop(); }

multicurve :
        empty
        |
        { alloc_counter(); } LPAREN multicurve_int RPAREN { pop(); }

multicurve_int :
        nonempty_linestring
        |
        geom_circularstring
        |
        multicurve_int COMMA nonempty_linestring
        |
        multicurve_int COMMA geom_circularstring

/* POLYGON */

geom_polygon :
	POLYGON polygon
	|
	POLYGONM { set_zm(0, 1); } polygon 

polygon :
	empty_polygon
	|
	nonempty_polygon

empty_polygon :
	{ alloc_polygon(); } empty  { pop(); } 

nonempty_polygon :
	{ alloc_polygon(); } polygon_1  { pop(); } 

polygon_1 :
	{ alloc_counter(); } LPAREN polygon_int RPAREN { pop();} 

polygon_int :
	linestring_1
	|
	polygon_int COMMA linestring_1

/* CURVEPOLYGON */

geom_curvepolygon :
        CURVEPOLYGON { alloc_curvepolygon(); } curvepolygon { pop(); }
        |
        CURVEPOLYGONM { set_zm(0, 1); alloc_curvepolygon(); } 
                        curvepolygon { pop(); }

curvepolygon :
        empty
        |
        { alloc_counter(); } LPAREN curvepolygon_int RPAREN { pop(); }

curvepolygon_int :
        nonempty_linestring_closed
        |
        geom_circularstring_closed
        |
        curvepolygon_int COMMA nonempty_linestring_closed
        |
        curvepolygon_int COMMA geom_circularstring_closed

/* MULTIPOLYGON */

geom_multipolygon :
	MULTIPOLYGON { alloc_multipolygon(); } multipolygon { pop(); }
	|
	MULTIPOLYGONM { set_zm(0, 1); alloc_multipolygon(); }
		multipolygon { pop();} 

multipolygon :
	empty
	|
	{ alloc_counter(); } LPAREN multipolygon_int RPAREN { pop(); }

multipolygon_int :
	nonempty_polygon
	|
	multipolygon_int COMMA nonempty_polygon

/* MULTISURFACE */

geom_multisurface :
        MULTISURFACE {alloc_multisurface(); } multisurface { pop(); }
        |
        MULTISURFACEM { set_zm(0, 1); alloc_multisurface(); }
                multisurface { pop(); }

multisurface :
        empty
        |
        { alloc_counter(); } LPAREN multisurface_int RPAREN { pop(); }

multisurface_int :
        nonempty_polygon
        |
        geom_curvepolygon
        |
        multisurface_int COMMA nonempty_polygon
        |
        multisurface_int COMMA geom_curvepolygon

/* GEOMETRYCOLLECTION */

geom_geometrycollection :
	GEOMETRYCOLLECTION { alloc_geomertycollection(); }
		geometrycollection { pop(); }
	|
	GEOMETRYCOLLECTIONM { set_zm(0, 1); alloc_geomertycollection(); }
		geometrycollection { pop();}

geometrycollection :
	empty
	|
	{ alloc_counter(); } LPAREN geometrycollection_int RPAREN { pop(); }

geometrycollection_int :
	/* to support GEOMETRYCOLLECTION(EMPTY) for backward compatibility */
	empty
	|
	geometry_int
	|
	geometrycollection_int COMMA geometry_int


a_point :
	point_2d
	|
	point_3d
	|
	point_4d 

point_2d :
	VALUE VALUE {alloc_point_2d($1,$2); }

point_3d :
	VALUE VALUE VALUE {alloc_point_3d($1,$2,$3); }

point_4d :
	VALUE VALUE VALUE VALUE {alloc_point_4d($1,$2,$3,$4); }

empty :
	EMPTY { alloc_empty(); }
%%




