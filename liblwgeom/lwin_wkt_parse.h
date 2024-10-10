/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_WKT_YY_LWIN_WKT_PARSE_H_INCLUDED
# define YY_WKT_YY_LWIN_WKT_PARSE_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int wkt_yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    POINT_TOK = 258,               /* POINT_TOK  */
    LINESTRING_TOK = 259,          /* LINESTRING_TOK  */
    POLYGON_TOK = 260,             /* POLYGON_TOK  */
    MPOINT_TOK = 261,              /* MPOINT_TOK  */
    MLINESTRING_TOK = 262,         /* MLINESTRING_TOK  */
    MPOLYGON_TOK = 263,            /* MPOLYGON_TOK  */
    MSURFACE_TOK = 264,            /* MSURFACE_TOK  */
    MCURVE_TOK = 265,              /* MCURVE_TOK  */
    CURVEPOLYGON_TOK = 266,        /* CURVEPOLYGON_TOK  */
    COMPOUNDCURVE_TOK = 267,       /* COMPOUNDCURVE_TOK  */
    CIRCULARSTRING_TOK = 268,      /* CIRCULARSTRING_TOK  */
    COLLECTION_TOK = 269,          /* COLLECTION_TOK  */
    RBRACKET_TOK = 270,            /* RBRACKET_TOK  */
    LBRACKET_TOK = 271,            /* LBRACKET_TOK  */
    COMMA_TOK = 272,               /* COMMA_TOK  */
    EMPTY_TOK = 273,               /* EMPTY_TOK  */
    SEMICOLON_TOK = 274,           /* SEMICOLON_TOK  */
    TRIANGLE_TOK = 275,            /* TRIANGLE_TOK  */
    TIN_TOK = 276,                 /* TIN_TOK  */
    POLYHEDRALSURFACE_TOK = 277,   /* POLYHEDRALSURFACE_TOK  */
    DOUBLE_TOK = 278,              /* DOUBLE_TOK  */
    DIMENSIONALITY_TOK = 279,      /* DIMENSIONALITY_TOK  */
    SRID_TOK = 280                 /* SRID_TOK  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 112 "lwin_wkt_parse.y"

	int integervalue;
	double doublevalue;
	char *stringvalue;
	LWGEOM *geometryvalue;
	POINT coordinatevalue;
	POINTARRAY *ptarrayvalue;

#line 98 "lwin_wkt_parse.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


extern YYSTYPE wkt_yylval;
extern YYLTYPE wkt_yylloc;

int wkt_yyparse (void);


#endif /* !YY_WKT_YY_LWIN_WKT_PARSE_H_INCLUDED  */
