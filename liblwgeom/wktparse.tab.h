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
     POINTM = 271,
     LINESTRINGM = 272,
     POLYGONM = 273,
     MULTIPOINTM = 274,
     MULTILINESTRINGM = 275,
     MULTIPOLYGONM = 276,
     GEOMETRYCOLLECTIONM = 277,
     CIRCULARSTRINGM = 278,
     COMPOUNDCURVEM = 279,
     CURVEPOLYGONM = 280,
     MULTICURVEM = 281,
     MULTISURFACEM = 282,
     SRID = 283,
     EMPTY = 284,
     VALUE = 285,
     LPAREN = 286,
     RPAREN = 287,
     COMMA = 288,
     EQUALS = 289,
     SEMICOLON = 290,
     WKB = 291
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
#define POINTM 271
#define LINESTRINGM 272
#define POLYGONM 273
#define MULTIPOINTM 274
#define MULTILINESTRINGM 275
#define MULTIPOLYGONM 276
#define GEOMETRYCOLLECTIONM 277
#define CIRCULARSTRINGM 278
#define COMPOUNDCURVEM 279
#define CURVEPOLYGONM 280
#define MULTICURVEM 281
#define MULTISURFACEM 282
#define SRID 283
#define EMPTY 284
#define VALUE 285
#define LPAREN 286
#define RPAREN 287
#define COMMA 288
#define EQUALS 289
#define SEMICOLON 290
#define WKB 291




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 22 "wktparse.y"
{
	double value;
	const char* wkb;
}
/* Line 1529 of yacc.c.  */
#line 126 "y.tab.h"
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
