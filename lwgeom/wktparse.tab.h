/* A Bison parser, made by GNU Bison 1.875.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software Foundation, Inc.

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
     POINTM = 265,
     LINESTRINGM = 266,
     POLYGONM = 267,
     MULTIPOINTM = 268,
     MULTILINESTRINGM = 269,
     MULTIPOLYGONM = 270,
     GEOMETRYCOLLECTIONM = 271,
     SRID = 272,
     EMPTY = 273,
     VALUE = 274,
     LPAREN = 275,
     RPAREN = 276,
     COMMA = 277,
     EQUALS = 278,
     SEMICOLON = 279,
     WKB = 280
   };
#endif
#define POINT 258
#define LINESTRING 259
#define POLYGON 260
#define MULTIPOINT 261
#define MULTILINESTRING 262
#define MULTIPOLYGON 263
#define GEOMETRYCOLLECTION 264
#define POINTM 265
#define LINESTRINGM 266
#define POLYGONM 267
#define MULTIPOINTM 268
#define MULTILINESTRINGM 269
#define MULTIPOLYGONM 270
#define GEOMETRYCOLLECTIONM 271
#define SRID 272
#define EMPTY 273
#define VALUE 274
#define LPAREN 275
#define RPAREN 276
#define COMMA 277
#define EQUALS 278
#define SEMICOLON 279
#define WKB 280




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 20 "wktparse.y"
typedef union YYSTYPE {
	double value;
	const char* wkb;
} YYSTYPE;
/* Line 1248 of yacc.c.  */
#line 91 "y.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE lwg_parse_yylval;



