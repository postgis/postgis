/*
 * Written by Ralph Mason ralph.mason<at>telogis.com
 *
 * Copyright Telogis 2004
 * www.telogis.com
 *
 */

%x vals_ok
%{
#include "wktparse.tab.h"
#include <unistd.h>
#include <stdlib.h> /* need stdlib for atof() definition */

void init_parser(const char *src);
void close_parser(void);
int lwg_parse_yywrap(void);
int lwg_parse_yylex(void);

static YY_BUFFER_STATE buf_state;
   void init_parser(const char *src) { BEGIN(0);buf_state = lwg_parse_yy_scan_string(src); }
   void close_parser() { lwg_parse_yy_delete_buffer(buf_state); }
   int lwg_parse_yywrap(void){ return 1; }

/* Macro to keep track of the current parse position */
#define UPDATE_YYLLOC() (lwg_parse_yylloc.last_column += yyleng)

%}

%%

<vals_ok>[-|\+]?[0-9]+(\.[0-9]+)?([Ee](\+|-)?[0-9]+)? { lwg_parse_yylval.value=atof(lwg_parse_yytext); UPDATE_YYLLOC(); return VALUE; }
<vals_ok>[-|\+]?(\.[0-9]+)([Ee](\+|-)?[0-9]+)? { lwg_parse_yylval.value=atof(lwg_parse_yytext); UPDATE_YYLLOC(); return VALUE; }

<INITIAL>00[0-9A-F]* {  lwg_parse_yylval.wkb=lwg_parse_yytext; return WKB;}
<INITIAL>01[0-9A-F]* {  lwg_parse_yylval.wkb=lwg_parse_yytext; return WKB;}

<*>POINT 	{ UPDATE_YYLLOC(); return POINT; }
<*>POINTM 	{ UPDATE_YYLLOC(); return POINTM; }
<*>LINESTRING { UPDATE_YYLLOC(); return LINESTRING; }
<*>LINESTRINGM { UPDATE_YYLLOC(); return LINESTRINGM; }
<*>CIRCULARSTRING { UPDATE_YYLLOC(); return CIRCULARSTRING; }
<*>CIRCULARSTRINGM { UPDATE_YYLLOC(); return CIRCULARSTRINGM; }
<*>POLYGON { UPDATE_YYLLOC(); return POLYGON; }
<*>POLYGONM { UPDATE_YYLLOC(); return POLYGONM; }
<*>COMPOUNDCURVE { UPDATE_YYLLOC(); return COMPOUNDCURVE; }
<*>COMPOUNDCURVEM { UPDATE_YYLLOC(); return COMPOUNDCURVEM; }
<*>CURVEPOLYGON { UPDATE_YYLLOC(); return CURVEPOLYGON; }
<*>CURVEPOLYGONM { UPDATE_YYLLOC(); return CURVEPOLYGONM; }
<*>MULTIPOINT { UPDATE_YYLLOC(); return MULTIPOINT; }
<*>MULTIPOINTM { UPDATE_YYLLOC(); return MULTIPOINTM; }
<*>MULTILINESTRING { UPDATE_YYLLOC(); return MULTILINESTRING; }
<*>MULTILINESTRINGM { UPDATE_YYLLOC(); return MULTILINESTRINGM; }
<*>MULTICURVE { UPDATE_YYLLOC(); return MULTICURVE; }
<*>MULTICURVEM { UPDATE_YYLLOC(); return MULTICURVEM; }
<*>MULTIPOLYGON { UPDATE_YYLLOC(); return MULTIPOLYGON; }
<*>MULTIPOLYGONM { UPDATE_YYLLOC(); return MULTIPOLYGONM; }
<*>MULTISURFACE { UPDATE_YYLLOC(); return MULTISURFACE; }
<*>MULTISURFACEM { UPDATE_YYLLOC(); return MULTISURFACEM; }
<*>GEOMETRYCOLLECTION { UPDATE_YYLLOC(); return GEOMETRYCOLLECTION; }
<*>GEOMETRYCOLLECTIONM { UPDATE_YYLLOC(); return GEOMETRYCOLLECTIONM; }
<*>SRID { BEGIN(vals_ok); UPDATE_YYLLOC(); return SRID; }
<*>EMPTY { UPDATE_YYLLOC(); return EMPTY; }

<*>\(	{ BEGIN(vals_ok); UPDATE_YYLLOC(); return LPAREN; }
<*>\)	{ UPDATE_YYLLOC(); return RPAREN; }
<*>,	{ UPDATE_YYLLOC(); return COMMA ; }
<*>=	{ UPDATE_YYLLOC(); return EQUALS ; }
<*>;	{ BEGIN(0); UPDATE_YYLLOC(); return SEMICOLON; }
<*>[ \t\n\r]+ /*eat whitespace*/ { UPDATE_YYLLOC(); }
<*>.	{ return lwg_parse_yytext[0]; }

%%

