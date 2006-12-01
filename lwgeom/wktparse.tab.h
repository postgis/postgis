/* A Bison parser, made by GNU Bison 1.875c.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     POINT = 258,
     LINESTRING = 259,
     POLYGON = 260,
     MULTIPOINT = 261,
     MULTILINESTRING = 262,
     MULTIPOLYGON = 263,
     GEOMETRYCOLLECTION = 264,
     CIRCULARSTRING = 265,
     COMPOUNDCURVE = 266,
     CURVEPOLYGON = 267,
     MULTICURVE = 268,
     MULTISURFACE = 269,
     POINTM = 270,
     LINESTRINGM = 271,
     POLYGONM = 272,
     MULTIPOINTM = 273,
     MULTILINESTRINGM = 274,
     MULTIPOLYGONM = 275,
     GEOMETRYCOLLECTIONM = 276,
     CIRCULARSTRINGM = 277,
     COMPOUNDCURVEM = 278,
     CURVEPOLYGONM = 279,
     MULTICURVEM = 280,
     MULTISURFACEM = 281,
     SRID = 282,
     EMPTY = 283,
     VALUE = 284,
     LPAREN = 285,
     RPAREN = 286,
     COMMA = 287,
     EQUALS = 288,
     SEMICOLON = 289,
     WKB = 290
   };
#endif
#define POINT 258
#define LINESTRING 259
#define POLYGON 260
#define MULTIPOINT 261
#define MULTILINESTRING 262
#define MULTIPOLYGON 263
#define GEOMETRYCOLLECTION 264
#define CIRCULARSTRING 265
#define COMPOUNDCURVE 266
#define CURVEPOLYGON 267
#define MULTICURVE 268
#define MULTISURFACE 269
#define POINTM 270
#define LINESTRINGM 271
#define POLYGONM 272
#define MULTIPOINTM 273
#define MULTILINESTRINGM 274
#define MULTIPOLYGONM 275
#define GEOMETRYCOLLECTIONM 276
#define CIRCULARSTRINGM 277
#define COMPOUNDCURVEM 278
#define CURVEPOLYGONM 279
#define MULTICURVEM 280
#define MULTISURFACEM 281
#define SRID 282
#define EMPTY 283
#define VALUE 284
#define LPAREN 285
#define RPAREN 286
#define COMMA 287
#define EQUALS 288
#define SEMICOLON 289
#define WKB 290




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 20 "wktparse.y"
typedef union YYSTYPE {
	double value;
	const char* wkb;
} YYSTYPE;
/* Line 1275 of yacc.c.  */
#line 112 "y.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE lwg_parse_yylval;



