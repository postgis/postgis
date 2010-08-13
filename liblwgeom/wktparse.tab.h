/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

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
     POLYHEDRALSURFACE = 270,
     TRIANGLE = 271,
     TIN = 272,
     POINTM = 273,
     LINESTRINGM = 274,
     POLYGONM = 275,
     MULTIPOINTM = 276,
     MULTILINESTRINGM = 277,
     MULTIPOLYGONM = 278,
     GEOMETRYCOLLECTIONM = 279,
     CIRCULARSTRINGM = 280,
     COMPOUNDCURVEM = 281,
     CURVEPOLYGONM = 282,
     MULTICURVEM = 283,
     MULTISURFACEM = 284,
     POLYHEDRALSURFACEM = 285,
     TRIANGLEM = 286,
     TINM = 287,
     SRID = 288,
     EMPTY = 289,
     VALUE = 290,
     LPAREN = 291,
     RPAREN = 292,
     COMMA = 293,
     EQUALS = 294,
     SEMICOLON = 295,
     WKB = 296
   };
#endif
/* Tokens.  */
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
#define POLYHEDRALSURFACE 270
#define TRIANGLE 271
#define TIN 272
#define POINTM 273
#define LINESTRINGM 274
#define POLYGONM 275
#define MULTIPOINTM 276
#define MULTILINESTRINGM 277
#define MULTIPOLYGONM 278
#define GEOMETRYCOLLECTIONM 279
#define CIRCULARSTRINGM 280
#define COMPOUNDCURVEM 281
#define CURVEPOLYGONM 282
#define MULTICURVEM 283
#define MULTISURFACEM 284
#define POLYHEDRALSURFACEM 285
#define TRIANGLEM 286
#define TINM 287
#define SRID 288
#define EMPTY 289
#define VALUE 290
#define LPAREN 291
#define RPAREN 292
#define COMMA 293
#define EQUALS 294
#define SEMICOLON 295
#define WKB 296




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 22 "wktparse.y"
{
	double value;
	const char* wkb;
}
/* Line 1529 of yacc.c.  */
#line 136 "y.tab.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE lwg_parse_yylval;

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif

extern YYLTYPE lwg_parse_yylloc;
