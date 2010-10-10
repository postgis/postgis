%{

/* The lexer */

#include <stdio.h>
#include <string.h>
#include "../libgeom.h" 
#include "g_wkt_parse.h"

static YY_BUFFER_STATE wkt_yy_buf_state; 
void wkt_lexer_init(char *src) { wkt_yy_buf_state = wkt_yy_scan_string(src); } 
void wkt_lexer_close() { wkt_yy_delete_buffer(wkt_yy_buf_state); } 


%}

%option noyywrap
%option prefix="wkt_yy"

%%

SRID= { printf("lex: SRID\n"); return SRID_TOK; }
GEOMETRYCOLLECTION { printf("lex: GEOMETRYCOLLECTION\n"); return COLLECTION_TOK; }
MULTISURFACE { printf("lex: MULTISURFACE\n"); return MSURFACE_TOK; }
MULTIPOLYGON { printf("lex: MULTIPOLYGON\n"); return MPOLYGON_TOK; }
MULTICURVE { printf("lex: MULTICURVE\n"); return MCURVE_TOK; }
MULTILINESTRING { printf("lex: MULTILINESTRING\n"); return MLINESTRING_TOK; }
MULTIPOINT { printf("lex: MULTIPOINT\n"); return MPOINT_TOK; }
CURVEPOLYGON { printf("lex: CURVEPOLYGON\n"); return CURVEPOLYGON_TOK; }
POLYGON { printf("lex: POLYGON\n"); return POLYGON_TOK; }
COMPOUNDCURVE { printf("lex: COMPOUNDCURVE\n"); return COMPOUNDCURVE_TOK; }
CIRCULARSTRING { printf("lex: CIRCULARSTRING\n"); return CIRCULARSTRING_TOK; }
LINESTRING { printf("lex: LINESTRING\n"); return LINESTRING_TOK; }
POINT { printf("lex: POINT\n"); return POINT_TOK; }
Z|M|ZM { wkt_yylval.stringvalue = strdup(wkt_yytext); printf("lex: DIMENSIONALITY\n"); return DIMENSIONALITY_TOK; }
EMPTY { printf("lex: EMPTY\n"); return EMPTY_TOK; }

-?(([0-9]+)|([0-9]*\.[0-9]+)([eE][-+]?[0-9]+)?) {  /* LWDEBUGF(5,"Lex got double: %s",wkt_yytext); */  printf("lex: DOUBLE\n"); wkt_yylval.doublevalue = atof(wkt_yytext); return DOUBLE_TOK; }

-?([0-9]+) {  /* LWDEBUGF(5,"Lex got integer: %s",wkt_yytext); */  printf("lex: INTEGER\n"); wkt_yylval.integervalue = atoi(wkt_yytext); return INTEGER_TOK; }

\( { printf("lex: LBRACKET\n"); return LBRACKET_TOK; }
\) { printf("lex: RBRACKET\n"); return RBRACKET_TOK; }
, { printf("lex: COMMA\n"); return COMMA_TOK; }
\; { printf("lex: SEMICOLON\n"); return SEMICOLON_TOK; }

[ \t]+  /* ignore whitespace */;

	
%%
