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

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0

/* If NAME_PREFIX is specified substitute the variables and functions
   names.  */
#define yyparse lwg_parse_yyparse
#define yylex   lwg_parse_yylex
#define yyerror lwg_parse_yyerror
#define yylval  lwg_parse_yylval
#define yychar  lwg_parse_yychar
#define yydebug lwg_parse_yydebug
#define yynerrs lwg_parse_yynerrs


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




/* Copy the first part of user declarations.  */
#line 9 "wktparse.y"

#include "wktparse.h"
#include <unistd.h>
#include <stdio.h>

void set_zm(char z, char m);
int lwg_parse_yylex(void);


/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 20 "wktparse.y"
typedef union YYSTYPE {
	double value;
	const char* wkb;
} YYSTYPE;
/* Line 191 of yacc.c.  */
#line 148 "y.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 160 "y.tab.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# if YYSTACK_USE_ALLOCA
#  define YYSTACK_ALLOC alloca
# else
#  ifndef YYSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define YYSTACK_ALLOC alloca
#   else
#    ifdef __GNUC__
#     define YYSTACK_ALLOC __builtin_alloca
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  6
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   104

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  26
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  66
/* YYNRULES -- Number of rules. */
#define YYNRULES  98
/* YYNRULES -- Number of states. */
#define YYNSTATES  139

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   280

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned char yyprhs[] =
{
       0,     0,     3,     4,     9,    10,    13,    15,    17,    19,
      21,    23,    25,    27,    29,    33,    35,    38,    39,    43,
      45,    47,    48,    51,    52,    55,    59,    60,    64,    65,
      69,    71,    72,    77,    79,    83,    85,    86,    89,    92,
      93,    97,    99,   101,   102,   105,   106,   109,   110,   115,
     117,   121,   122,   126,   127,   131,   133,   134,   139,   141,
     145,   148,   149,   153,   155,   157,   158,   161,   162,   165,
     166,   171,   173,   177,   178,   182,   183,   187,   189,   190,
     195,   197,   201,   202,   206,   207,   211,   213,   214,   219,
     221,   223,   227,   229,   231,   233,   236,   240,   245
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      27,     0,    -1,    -1,    31,    24,    28,    30,    -1,    -1,
      29,    30,    -1,    32,    -1,    33,    -1,    49,    -1,    65,
      -1,    41,    -1,    59,    -1,    75,    -1,    81,    -1,    17,
      23,    19,    -1,    25,    -1,     3,    35,    -1,    -1,    10,
      34,    35,    -1,    36,    -1,    38,    -1,    -1,    37,    91,
      -1,    -1,    39,    40,    -1,    20,    87,    21,    -1,    -1,
       6,    42,    44,    -1,    -1,    13,    43,    44,    -1,    91,
      -1,    -1,    45,    20,    46,    21,    -1,    47,    -1,    46,
      22,    47,    -1,    38,    -1,    -1,    48,    87,    -1,     4,
      51,    -1,    -1,    11,    50,    51,    -1,    52,    -1,    54,
      -1,    -1,    53,    91,    -1,    -1,    55,    56,    -1,    -1,
      57,    20,    58,    21,    -1,    87,    -1,    58,    22,    87,
      -1,    -1,     7,    60,    62,    -1,    -1,    14,    61,    62,
      -1,    91,    -1,    -1,    63,    20,    64,    21,    -1,    54,
      -1,    64,    22,    54,    -1,     5,    67,    -1,    -1,    12,
      66,    67,    -1,    68,    -1,    70,    -1,    -1,    69,    91,
      -1,    -1,    71,    72,    -1,    -1,    73,    20,    74,    21,
      -1,    56,    -1,    74,    22,    56,    -1,    -1,     8,    76,
      78,    -1,    -1,    15,    77,    78,    -1,    91,    -1,    -1,
      79,    20,    80,    21,    -1,    70,    -1,    80,    22,    70,
      -1,    -1,     9,    82,    84,    -1,    -1,    16,    83,    84,
      -1,    91,    -1,    -1,    85,    20,    86,    21,    -1,    91,
      -1,    30,    -1,    86,    22,    30,    -1,    88,    -1,    89,
      -1,    90,    -1,    19,    19,    -1,    19,    19,    19,    -1,
      19,    19,    19,    19,    -1,    18,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned char yyrline[] =
{
       0,    36,    36,    36,    38,    38,    41,    43,    45,    47,
      49,    51,    53,    55,    58,    61,    67,    69,    69,    72,
      74,    77,    77,    80,    80,    84,    89,    89,    91,    91,
      94,    96,    96,    99,   101,   104,   107,   107,   113,   115,
     115,   118,   120,   123,   123,   126,   126,   129,   129,   132,
     134,   139,   139,   142,   142,   146,   148,   148,   151,   153,
     159,   161,   161,   164,   166,   169,   169,   172,   172,   175,
     175,   178,   180,   185,   185,   187,   187,   191,   193,   193,
     196,   198,   204,   204,   207,   207,   211,   213,   213,   217,
     219,   221,   225,   227,   229,   232,   235,   238,   241
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "POINT", "LINESTRING", "POLYGON", 
  "MULTIPOINT", "MULTILINESTRING", "MULTIPOLYGON", "GEOMETRYCOLLECTION", 
  "POINTM", "LINESTRINGM", "POLYGONM", "MULTIPOINTM", "MULTILINESTRINGM", 
  "MULTIPOLYGONM", "GEOMETRYCOLLECTIONM", "SRID", "EMPTY", "VALUE", 
  "LPAREN", "RPAREN", "COMMA", "EQUALS", "SEMICOLON", "WKB", "$accept", 
  "geometry", "@1", "@2", "geometry_int", "srid", "geom_wkb", 
  "geom_point", "@3", "point", "empty_point", "@4", "nonempty_point", 
  "@5", "point_int", "geom_multipoint", "@6", "@7", "multipoint", "@8", 
  "multipoint_int", "mpoint_element", "@9", "geom_linestring", "@10", 
  "linestring", "empty_linestring", "@11", "nonempty_linestring", "@12", 
  "linestring_1", "@13", "linestring_int", "geom_multilinestring", "@14", 
  "@15", "multilinestring", "@16", "multilinestring_int", "geom_polygon", 
  "@17", "polygon", "empty_polygon", "@18", "nonempty_polygon", "@19", 
  "polygon_1", "@20", "polygon_int", "geom_multipolygon", "@21", "@22", 
  "multipolygon", "@23", "multipolygon_int", "geom_geometrycollection", 
  "@24", "@25", "geometrycollection", "@26", "geometrycollection_int", 
  "a_point", "point_2d", "point_3d", "point_4d", "empty", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    26,    28,    27,    29,    27,    30,    30,    30,    30,
      30,    30,    30,    30,    31,    32,    33,    34,    33,    35,
      35,    37,    36,    39,    38,    40,    42,    41,    43,    41,
      44,    45,    44,    46,    46,    47,    48,    47,    49,    50,
      49,    51,    51,    53,    52,    55,    54,    57,    56,    58,
      58,    60,    59,    61,    59,    62,    63,    62,    64,    64,
      65,    66,    65,    67,    67,    69,    68,    71,    70,    73,
      72,    74,    74,    76,    75,    77,    75,    78,    79,    78,
      80,    80,    82,    81,    83,    81,    84,    85,    84,    86,
      86,    86,    87,    87,    87,    88,    89,    90,    91
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     0,     4,     0,     2,     1,     1,     1,     1,
       1,     1,     1,     1,     3,     1,     2,     0,     3,     1,
       1,     0,     2,     0,     2,     3,     0,     3,     0,     3,
       1,     0,     4,     1,     3,     1,     0,     2,     2,     0,
       3,     1,     1,     0,     2,     0,     2,     0,     4,     1,
       3,     0,     3,     0,     3,     1,     0,     4,     1,     3,
       2,     0,     3,     1,     1,     0,     2,     0,     2,     0,
       4,     1,     3,     0,     3,     0,     3,     1,     0,     4,
       1,     3,     0,     3,     0,     3,     1,     0,     4,     1,
       1,     3,     1,     1,     1,     2,     3,     4,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       4,     0,     0,     0,     0,     0,     1,    21,    43,    65,
      26,    51,    73,    82,    17,    39,    61,    28,    53,    75,
      84,    15,     5,     6,     7,    10,     8,    11,     9,    12,
      13,     2,    14,    16,    19,     0,    20,     0,    38,    41,
       0,    42,    47,    60,    63,     0,    64,    69,    31,    56,
      78,    87,    21,    43,    65,    31,    56,    78,    87,     0,
      98,    22,     0,    24,    44,    46,     0,    66,    68,     0,
      27,     0,    30,    52,     0,    55,    74,     0,    77,    83,
       0,    86,    18,    40,    62,    29,    54,    76,    85,     3,
       0,     0,    92,    93,    94,     0,    47,    23,    45,    67,
       0,    95,    25,     0,    49,    71,     0,    35,     0,    33,
       0,    58,     0,    80,     0,    90,     0,    89,    96,    48,
       0,    70,    47,    32,    23,    37,    57,    45,    79,    67,
      88,     0,    97,    50,    72,    34,    59,    81,    91
};

/* YYDEFGOTO[NTERM-NUM]. */
static const yysigned_char yydefgoto[] =
{
      -1,     2,    59,     3,    22,     4,    23,    24,    52,    33,
      34,    35,    36,    37,    63,    25,    48,    55,    70,    71,
     108,   109,   110,    26,    53,    38,    39,    40,    41,    42,
      65,    66,   103,    27,    49,    56,    73,    74,   112,    28,
      54,    43,    44,    45,    46,    47,    68,    69,   106,    29,
      50,    57,    76,    77,   114,    30,    51,    58,    79,    80,
     116,    91,    92,    93,    94,    72
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -98
static const yysigned_char yypact[] =
{
     -12,    13,    14,    70,     4,    18,   -98,    19,    20,    36,
     -98,   -98,   -98,   -98,   -98,   -98,   -98,   -98,   -98,   -98,
     -98,   -98,   -98,   -98,   -98,   -98,   -98,   -98,   -98,   -98,
     -98,   -98,   -98,   -98,   -98,    40,   -98,    41,   -98,   -98,
      40,   -98,   -98,   -98,   -98,    40,   -98,   -98,    40,    40,
      40,    40,    19,    20,    36,    40,    40,    40,    40,    70,
     -98,   -98,    43,   -98,   -98,   -98,    45,   -98,   -98,    47,
     -98,    48,   -98,   -98,    49,   -98,   -98,    50,   -98,   -98,
      51,   -98,   -98,   -98,   -98,   -98,   -98,   -98,   -98,   -98,
      44,    17,   -98,   -98,   -98,    43,   -98,    68,   -98,   -98,
      39,    69,   -98,   -13,   -98,   -98,    -9,   -98,    -3,   -98,
      43,   -98,    -1,   -98,     5,   -98,     9,   -98,    71,   -98,
      43,   -98,   -98,   -98,    68,   -98,   -98,   -98,   -98,   -98,
     -98,    70,   -98,   -98,   -98,   -98,   -98,   -98,   -98
};

/* YYPGOTO[NTERM-NUM].  */
static const yysigned_char yypgoto[] =
{
     -98,   -98,   -98,   -98,   -59,   -98,   -98,   -98,   -98,     7,
     -98,   -98,   -90,   -98,   -98,   -98,   -98,   -98,    34,   -98,
     -98,   -64,   -98,   -98,   -98,    38,   -98,   -98,   -94,   -98,
     -93,   -98,   -98,   -98,   -98,   -98,    37,   -98,   -98,   -98,
     -98,    42,   -98,   -98,   -97,   -98,   -98,   -98,   -98,   -98,
     -98,   -98,    35,   -98,   -98,   -98,   -98,   -98,    46,   -98,
     -98,   -85,   -98,   -98,   -98,   -34
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -68
static const short yytable[] =
{
      89,    61,   113,   105,   111,     1,    64,   107,   119,   120,
     104,    67,   121,   122,     6,    75,    78,    81,   123,   124,
     126,   127,    75,    78,    81,   125,   128,   129,    31,   134,
     130,   131,   137,   136,   107,   133,     5,    32,   102,   -23,
     -45,   115,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,   -67,    60,    60,    82,
     135,    62,    90,   101,    21,    95,   117,    96,    97,    98,
      99,   100,   138,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,   -36,   118,    85,
     132,    83,    87,    86,     0,    21,    84,     0,     0,     0,
       0,     0,     0,     0,    88
};

static const short yycheck[] =
{
      59,    35,    99,    96,    98,    17,    40,    97,    21,    22,
      95,    45,    21,    22,     0,    49,    50,    51,    21,    22,
      21,    22,    56,    57,    58,   110,    21,    22,    24,   122,
      21,    22,   129,   127,   124,   120,    23,    19,    21,    20,
      20,   100,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    20,    18,    18,    52,
     124,    20,    19,    19,    25,    20,   100,    20,    20,    20,
      20,    20,   131,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    19,    19,    55,
      19,    53,    57,    56,    -1,    25,    54,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    58
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    17,    27,    29,    31,    23,     0,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    25,    30,    32,    33,    41,    49,    59,    65,    75,
      81,    24,    19,    35,    36,    37,    38,    39,    51,    52,
      53,    54,    55,    67,    68,    69,    70,    71,    42,    60,
      76,    82,    34,    50,    66,    43,    61,    77,    83,    28,
      18,    91,    20,    40,    91,    56,    57,    91,    72,    73,
      44,    45,    91,    62,    63,    91,    78,    79,    91,    84,
      85,    91,    35,    51,    67,    44,    62,    78,    84,    30,
      19,    87,    88,    89,    90,    20,    20,    20,    20,    20,
      20,    19,    21,    58,    87,    56,    74,    38,    46,    47,
      48,    54,    64,    70,    80,    30,    86,    91,    19,    21,
      22,    21,    22,    21,    22,    87,    21,    22,    21,    22,
      21,    22,    19,    87,    56,    47,    54,    70,    30
};

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrlab1

/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)         \
  Current.first_line   = Rhs[1].first_line;      \
  Current.first_column = Rhs[1].first_column;    \
  Current.last_line    = Rhs[N].last_line;       \
  Current.last_column  = Rhs[N].last_column;
#endif

/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)

# define YYDSYMPRINT(Args)			\
do {						\
  if (yydebug)					\
    yysymprint Args;				\
} while (0)

# define YYDSYMPRINTF(Title, Token, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr, 					\
                  Token, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (cinluded).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short *bottom, short *top)
#else
static void
yy_stack_print (bottom, top)
    short *bottom;
    short *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned int yylineno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylineno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YYDSYMPRINT(Args)
# define YYDSYMPRINTF(Title, Token, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

#endif /* !YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    {
      YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
# ifdef YYPRINT
      YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
    }
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yytype, yyvaluep)
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YYDSYMPRINTF ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %s, ", yytname[yytoken]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 36 "wktparse.y"
    { alloc_lwgeom(srid); }
    break;

  case 4:
#line 38 "wktparse.y"
    { alloc_lwgeom(-1); }
    break;

  case 14:
#line 58 "wktparse.y"
    { set_srid(yyvsp[0].value); }
    break;

  case 15:
#line 61 "wktparse.y"
    { alloc_wkb(yyvsp[0].wkb); }
    break;

  case 17:
#line 69 "wktparse.y"
    { set_zm(0, 1); }
    break;

  case 21:
#line 77 "wktparse.y"
    { alloc_point(); }
    break;

  case 22:
#line 77 "wktparse.y"
    { pop(); }
    break;

  case 23:
#line 80 "wktparse.y"
    { alloc_point(); }
    break;

  case 24:
#line 80 "wktparse.y"
    { pop(); }
    break;

  case 26:
#line 89 "wktparse.y"
    { alloc_multipoint(); }
    break;

  case 27:
#line 89 "wktparse.y"
    { pop(); }
    break;

  case 28:
#line 91 "wktparse.y"
    { set_zm(0, 1); alloc_multipoint(); }
    break;

  case 29:
#line 91 "wktparse.y"
    {pop(); }
    break;

  case 31:
#line 96 "wktparse.y"
    { alloc_counter(); }
    break;

  case 32:
#line 96 "wktparse.y"
    { pop(); }
    break;

  case 36:
#line 107 "wktparse.y"
    { alloc_point(); }
    break;

  case 37:
#line 107 "wktparse.y"
    { pop(); }
    break;

  case 39:
#line 115 "wktparse.y"
    { set_zm(0, 1); }
    break;

  case 43:
#line 123 "wktparse.y"
    { alloc_linestring(); }
    break;

  case 44:
#line 123 "wktparse.y"
    { pop(); }
    break;

  case 45:
#line 126 "wktparse.y"
    { alloc_linestring(); }
    break;

  case 46:
#line 126 "wktparse.y"
    { pop(); }
    break;

  case 47:
#line 129 "wktparse.y"
    { alloc_counter(); }
    break;

  case 48:
#line 129 "wktparse.y"
    { popc(); }
    break;

  case 51:
#line 139 "wktparse.y"
    { alloc_multilinestring(); }
    break;

  case 52:
#line 140 "wktparse.y"
    { pop(); }
    break;

  case 53:
#line 142 "wktparse.y"
    { set_zm(0, 1); alloc_multilinestring(); }
    break;

  case 54:
#line 143 "wktparse.y"
    { pop(); }
    break;

  case 56:
#line 148 "wktparse.y"
    { alloc_counter(); }
    break;

  case 57:
#line 148 "wktparse.y"
    { pop();}
    break;

  case 61:
#line 161 "wktparse.y"
    { set_zm(0, 1); }
    break;

  case 65:
#line 169 "wktparse.y"
    { alloc_polygon(); }
    break;

  case 66:
#line 169 "wktparse.y"
    { pop(); }
    break;

  case 67:
#line 172 "wktparse.y"
    { alloc_polygon(); }
    break;

  case 68:
#line 172 "wktparse.y"
    { pop(); }
    break;

  case 69:
#line 175 "wktparse.y"
    { alloc_counter(); }
    break;

  case 70:
#line 175 "wktparse.y"
    { pop();}
    break;

  case 73:
#line 185 "wktparse.y"
    { alloc_multipolygon(); }
    break;

  case 74:
#line 185 "wktparse.y"
    { pop(); }
    break;

  case 75:
#line 187 "wktparse.y"
    { set_zm(0, 1); alloc_multipolygon(); }
    break;

  case 76:
#line 188 "wktparse.y"
    { pop();}
    break;

  case 78:
#line 193 "wktparse.y"
    { alloc_counter(); }
    break;

  case 79:
#line 193 "wktparse.y"
    { pop(); }
    break;

  case 82:
#line 204 "wktparse.y"
    { alloc_geomertycollection(); }
    break;

  case 83:
#line 205 "wktparse.y"
    { pop(); }
    break;

  case 84:
#line 207 "wktparse.y"
    { set_zm(0, 1); alloc_geomertycollection(); }
    break;

  case 85:
#line 208 "wktparse.y"
    { pop();}
    break;

  case 87:
#line 213 "wktparse.y"
    { alloc_counter(); }
    break;

  case 88:
#line 213 "wktparse.y"
    { pop(); }
    break;

  case 95:
#line 232 "wktparse.y"
    {alloc_point_2d(yyvsp[-1].value,yyvsp[0].value); }
    break;

  case 96:
#line 235 "wktparse.y"
    {alloc_point_3d(yyvsp[-2].value,yyvsp[-1].value,yyvsp[0].value); }
    break;

  case 97:
#line 238 "wktparse.y"
    {alloc_point_4d(yyvsp[-3].value,yyvsp[-2].value,yyvsp[-1].value,yyvsp[0].value); }
    break;

  case 98:
#line 241 "wktparse.y"
    { alloc_empty(); }
    break;


    }

/* Line 991 of yacc.c.  */
#line 1436 "y.tab.c"

  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  int yytype = YYTRANSLATE (yychar);
	  char *yymsg;
	  int yyx, yycount;

	  yycount = 0;
	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  for (yyx = yyn < 0 ? -yyn : 0;
	       yyx < (int) (sizeof (yytname) / sizeof (char *)); yyx++)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      yysize += yystrlen (yytname[yyx]) + 15, yycount++;
	  yysize += yystrlen ("syntax error, unexpected ") + 1;
	  yysize += yystrlen (yytname[yytype]);
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yycount = 0;
		  for (yyx = yyn < 0 ? -yyn : 0;
		       yyx < (int) (sizeof (yytname) / sizeof (char *));
		       yyx++)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
		      {
			const char *yyq = ! yycount ? ", expecting " : " or ";
			yyp = yystpcpy (yyp, yyq);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yycount++;
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("syntax error; also virtual memory exhausted");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("syntax error");
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      /* Return failure if at end of input.  */
      if (yychar == YYEOF)
        {
	  /* Pop the error token.  */
          YYPOPSTACK;
	  /* Pop the rest of the stack.  */
	  while (yyss < yyssp)
	    {
	      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
	      yydestruct (yystos[*yyssp], yyvsp);
	      YYPOPSTACK;
	    }
	  YYABORT;
        }

      YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
      yydestruct (yytoken, &yylval);
      yychar = YYEMPTY;

    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab2;


/*----------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action.  |
`----------------------------------------------------*/
yyerrlab1:

  /* Suppress GCC warning that yyerrlab1 is unused when no action
     invokes YYERROR.  */
#if defined (__GNUC_MINOR__) && 2093 <= (__GNUC__ * 1000 + __GNUC_MINOR__)
  __attribute__ ((__unused__))
#endif


  goto yyerrlab2;


/*---------------------------------------------------------------.
| yyerrlab2 -- pop states until the error token can be shifted.  |
`---------------------------------------------------------------*/
yyerrlab2:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
      yydestruct (yystos[yystate], yyvsp);
      yyvsp--;
      yystate = *--yyssp;

      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;


  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*----------------------------------------------.
| yyoverflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 35 "wktparse.y"






