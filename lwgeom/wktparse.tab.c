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
#define YYLAST   122

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  26
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  57
/* YYNRULES -- Number of rules. */
#define YYNRULES  89
/* YYNRULES -- Number of states. */
#define YYNSTATES  130

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
      44,    47,    49,    53,    54,    58,    59,    63,    65,    66,
      71,    73,    77,    79,    80,    83,    86,    87,    91,    92,
      95,    97,    98,   103,   105,   109,   110,   114,   115,   119,
     121,   122,   127,   129,   133,   136,   137,   141,   142,   145,
     147,   148,   153,   155,   159,   160,   164,   165,   169,   171,
     172,   177,   179,   183,   184,   188,   189,   193,   195,   196,
     201,   203,   205,   209,   211,   213,   215,   218,   222,   227
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      27,     0,    -1,    -1,    31,    24,    28,    30,    -1,    -1,
      29,    30,    -1,    32,    -1,    33,    -1,    46,    -1,    59,
      -1,    38,    -1,    53,    -1,    66,    -1,    72,    -1,    17,
      23,    19,    -1,    25,    -1,     3,    35,    -1,    -1,    10,
      34,    35,    -1,    -1,    36,    37,    -1,    82,    -1,    20,
      78,    21,    -1,    -1,     6,    39,    41,    -1,    -1,    13,
      40,    41,    -1,    82,    -1,    -1,    42,    20,    43,    21,
      -1,    44,    -1,    43,    22,    44,    -1,    35,    -1,    -1,
      45,    78,    -1,     4,    48,    -1,    -1,    11,    47,    48,
      -1,    -1,    49,    50,    -1,    82,    -1,    -1,    51,    20,
      52,    21,    -1,    78,    -1,    52,    22,    78,    -1,    -1,
       7,    54,    56,    -1,    -1,    14,    55,    56,    -1,    82,
      -1,    -1,    57,    20,    58,    21,    -1,    48,    -1,    58,
      22,    48,    -1,     5,    61,    -1,    -1,    12,    60,    61,
      -1,    -1,    62,    63,    -1,    82,    -1,    -1,    64,    20,
      65,    21,    -1,    50,    -1,    65,    22,    50,    -1,    -1,
       8,    67,    69,    -1,    -1,    15,    68,    69,    -1,    82,
      -1,    -1,    70,    20,    71,    21,    -1,    61,    -1,    71,
      22,    61,    -1,    -1,     9,    73,    75,    -1,    -1,    16,
      74,    75,    -1,    82,    -1,    -1,    76,    20,    77,    21,
      -1,    82,    -1,    30,    -1,    77,    22,    30,    -1,    79,
      -1,    80,    -1,    81,    -1,    19,    19,    -1,    19,    19,
      19,    -1,    19,    19,    19,    19,    -1,    18,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned char yyrline[] =
{
       0,    35,    35,    35,    36,    36,    38,    38,    38,    38,
      38,    39,    39,    39,    41,    43,    48,    48,    48,    50,
      50,    52,    52,    56,    56,    57,    57,    59,    59,    59,
      61,    61,    63,    63,    63,    68,    68,    68,    70,    70,
      72,    72,    72,    74,    74,    78,    78,    78,    78,    80,
      80,    80,    82,    82,    87,    87,    87,    89,    89,    91,
      91,    91,    93,    93,    97,    97,    97,    97,    99,    99,
      99,   101,   101,   107,   107,   107,   107,   109,   109,   109,
     111,   111,   111,   114,   114,   114,   116,   118,   120,   122
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
  "geom_point", "@3", "point", "@4", "point_int", "geom_multipoint", "@5", 
  "@6", "multipoint", "@7", "multipoint_int", "mpoint", "@8", 
  "geom_linestring", "@9", "linestring", "@10", "linestring_1", "@11", 
  "linestring_int", "geom_multilinestring", "@12", "@13", 
  "multilinestring", "@14", "multilinestring_int", "geom_polygon", "@15", 
  "polygon", "@16", "polygon_1", "@17", "polygon_int", 
  "geom_multipolygon", "@18", "@19", "multipolygon", "@20", 
  "multipolygon_int", "geom_geometrycollection", "@21", "@22", 
  "geometrycollection", "@23", "geometrycollection_int", "a_point", 
  "point_2d", "point_3d", "point_4d", "empty", 0
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
      30,    30,    30,    30,    31,    32,    33,    34,    33,    36,
      35,    37,    37,    39,    38,    40,    38,    41,    42,    41,
      43,    43,    44,    45,    44,    46,    47,    46,    49,    48,
      50,    51,    50,    52,    52,    54,    53,    55,    53,    56,
      57,    56,    58,    58,    59,    60,    59,    62,    61,    63,
      64,    63,    65,    65,    67,    66,    68,    66,    69,    70,
      69,    71,    71,    73,    72,    74,    72,    75,    76,    75,
      77,    77,    77,    78,    78,    78,    79,    80,    81,    82
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     0,     4,     0,     2,     1,     1,     1,     1,
       1,     1,     1,     1,     3,     1,     2,     0,     3,     0,
       2,     1,     3,     0,     3,     0,     3,     1,     0,     4,
       1,     3,     1,     0,     2,     2,     0,     3,     0,     2,
       1,     0,     4,     1,     3,     0,     3,     0,     3,     1,
       0,     4,     1,     3,     2,     0,     3,     0,     2,     1,
       0,     4,     1,     3,     0,     3,     0,     3,     1,     0,
       4,     1,     3,     0,     3,     0,     3,     1,     0,     4,
       1,     1,     3,     1,     1,     1,     2,     3,     4,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       4,     0,     0,     0,     0,     0,     1,    19,    38,    57,
      23,    45,    64,    73,    17,    36,    55,    25,    47,    66,
      75,    15,     5,     6,     7,    10,     8,    11,     9,    12,
      13,     2,    14,    16,     0,    35,    41,    54,    60,    28,
      50,    69,    78,    19,    38,    57,    28,    50,    69,    78,
       0,    89,     0,    20,    21,    39,     0,    40,    58,     0,
      59,    24,     0,    27,    46,     0,    49,    65,     0,    68,
      74,     0,    77,    18,    37,    56,    26,    48,    67,    76,
       3,     0,     0,    83,    84,    85,     0,    41,    19,    38,
      57,     0,    86,    22,     0,    43,    62,     0,    32,     0,
      30,     0,    52,     0,    71,     0,    81,     0,    80,    87,
      42,     0,    61,    41,    29,    19,    34,    51,    38,    70,
      57,    79,     0,    88,    44,    63,    31,    53,    72,    82
};

/* YYDEFGOTO[NTERM-NUM]. */
static const yysigned_char yydefgoto[] =
{
      -1,     2,    50,     3,    22,     4,    23,    24,    43,    98,
      34,    53,    25,    39,    46,    61,    62,    99,   100,   101,
      26,    44,    35,    36,    55,    56,    94,    27,    40,    47,
      64,    65,   103,    28,    45,    37,    38,    58,    59,    97,
      29,    41,    48,    67,    68,   105,    30,    42,    49,    70,
      71,   107,    82,    83,    84,    85,    57
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -83
static const yysigned_char yypact[] =
{
     -11,    -1,    20,    97,     6,    17,   -83,   -83,   -83,   -83,
     -83,   -83,   -83,   -83,   -83,   -83,   -83,   -83,   -83,   -83,
     -83,   -83,   -83,   -83,   -83,   -83,   -83,   -83,   -83,   -83,
     -83,   -83,   -83,   -83,     1,   -83,    21,   -83,    21,    21,
      21,    21,    21,   -83,   -83,   -83,    21,    21,    21,    21,
      97,   -83,    19,   -83,   -83,   -83,    23,   -83,   -83,    24,
     -83,   -83,    26,   -83,   -83,    28,   -83,   -83,    29,   -83,
     -83,    30,   -83,   -83,   -83,   -83,   -83,   -83,   -83,   -83,
     -83,    22,    31,   -83,   -83,   -83,    19,    21,    32,   -83,
     -83,    74,    34,   -83,    -8,   -83,   -83,     2,   -83,     4,
     -83,    19,   -83,     7,   -83,    11,   -83,    13,   -83,    35,
     -83,    19,   -83,    21,   -83,    32,   -83,   -83,   -83,   -83,
     -83,   -83,    97,   -83,   -83,   -83,   -83,   -83,   -83,   -83
};

/* YYPGOTO[NTERM-NUM].  */
static const yysigned_char yypgoto[] =
{
     -83,   -83,   -83,   -83,   -49,   -83,   -83,   -83,   -83,    -3,
     -83,   -83,   -83,   -83,   -83,     9,   -83,   -83,   -59,   -83,
     -83,   -83,   -42,   -83,   -82,   -83,   -83,   -83,   -83,   -83,
      10,   -83,   -83,   -83,   -83,   -45,   -83,   -83,   -83,   -83,
     -83,   -83,   -83,    14,   -83,   -83,   -83,   -83,   -83,    12,
     -83,   -83,   -74,   -83,   -83,   -83,   -31
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -34
static const short yytable[] =
{
      75,    80,    74,    54,    33,    96,     1,    60,    63,    66,
      69,    72,    95,   110,   111,    63,    66,    69,    72,    51,
       6,    52,     5,   112,   113,   114,   115,   116,   117,   118,
      31,   125,   119,   120,   121,   122,    32,   124,    81,    51,
      73,    92,   106,    86,    87,   104,    88,   102,    89,    90,
      91,   -33,    93,   109,   123,    76,   126,    77,     0,     0,
     108,    79,    78,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   129,     0,   128,   127,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,     0,    51,     0,     0,     0,     0,     0,     0,    21,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,     0,     0,     0,     0,     0,     0,
       0,     0,    21
};

static const yysigned_char yycheck[] =
{
      45,    50,    44,    34,     7,    87,    17,    38,    39,    40,
      41,    42,    86,    21,    22,    46,    47,    48,    49,    18,
       0,    20,    23,    21,    22,    21,    22,   101,    21,    22,
      24,   113,    21,    22,    21,    22,    19,   111,    19,    18,
      43,    19,    91,    20,    20,    90,    20,    89,    20,    20,
      20,    19,    21,    19,    19,    46,   115,    47,    -1,    -1,
      91,    49,    48,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   122,    -1,   120,   118,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    -1,    18,    -1,    -1,    -1,    -1,    -1,    -1,    25,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    25
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    17,    27,    29,    31,    23,     0,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    25,    30,    32,    33,    38,    46,    53,    59,    66,
      72,    24,    19,    35,    36,    48,    49,    61,    62,    39,
      54,    67,    73,    34,    47,    60,    40,    55,    68,    74,
      28,    18,    20,    37,    82,    50,    51,    82,    63,    64,
      82,    41,    42,    82,    56,    57,    82,    69,    70,    82,
      75,    76,    82,    35,    48,    61,    41,    56,    69,    75,
      30,    19,    78,    79,    80,    81,    20,    20,    20,    20,
      20,    20,    19,    21,    52,    78,    50,    65,    35,    43,
      44,    45,    48,    58,    61,    71,    30,    77,    82,    19,
      21,    22,    21,    22,    21,    22,    78,    21,    22,    21,
      22,    21,    22,    19,    78,    50,    44,    48,    61,    30
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
#line 35 "wktparse.y"
    { alloc_lwgeom(srid); }
    break;

  case 4:
#line 36 "wktparse.y"
    { alloc_lwgeom(-1); }
    break;

  case 14:
#line 41 "wktparse.y"
    { set_srid(yyvsp[0].value); }
    break;

  case 15:
#line 43 "wktparse.y"
    {alloc_wkb(yyvsp[0].wkb) ; }
    break;

  case 17:
#line 48 "wktparse.y"
    { set_zm(0, 1); }
    break;

  case 19:
#line 50 "wktparse.y"
    { alloc_point(); }
    break;

  case 20:
#line 50 "wktparse.y"
    { pop();}
    break;

  case 23:
#line 56 "wktparse.y"
    { alloc_multipoint(); }
    break;

  case 24:
#line 56 "wktparse.y"
    { pop();}
    break;

  case 25:
#line 57 "wktparse.y"
    { set_zm(0, 1); alloc_multipoint(); }
    break;

  case 26:
#line 57 "wktparse.y"
    {pop();}
    break;

  case 28:
#line 59 "wktparse.y"
    { alloc_counter(); }
    break;

  case 29:
#line 59 "wktparse.y"
    {pop();}
    break;

  case 33:
#line 63 "wktparse.y"
    { alloc_point(); }
    break;

  case 34:
#line 63 "wktparse.y"
    { pop();}
    break;

  case 36:
#line 68 "wktparse.y"
    { set_zm(0, 1); }
    break;

  case 38:
#line 70 "wktparse.y"
    { alloc_linestring(); }
    break;

  case 39:
#line 70 "wktparse.y"
    {pop();}
    break;

  case 41:
#line 72 "wktparse.y"
    { alloc_counter(); }
    break;

  case 42:
#line 72 "wktparse.y"
    {popc(); }
    break;

  case 45:
#line 78 "wktparse.y"
    { alloc_multilinestring(); }
    break;

  case 46:
#line 78 "wktparse.y"
    { pop();}
    break;

  case 47:
#line 78 "wktparse.y"
    { set_zm(0, 1); alloc_multilinestring(); }
    break;

  case 48:
#line 78 "wktparse.y"
    { pop(); }
    break;

  case 50:
#line 80 "wktparse.y"
    { alloc_counter(); }
    break;

  case 51:
#line 80 "wktparse.y"
    { pop();}
    break;

  case 55:
#line 87 "wktparse.y"
    { set_zm(0, 1); }
    break;

  case 57:
#line 89 "wktparse.y"
    { alloc_polygon(); }
    break;

  case 58:
#line 89 "wktparse.y"
    { pop();}
    break;

  case 60:
#line 91 "wktparse.y"
    { alloc_counter(); }
    break;

  case 61:
#line 91 "wktparse.y"
    { pop();}
    break;

  case 64:
#line 97 "wktparse.y"
    { alloc_multipolygon(); }
    break;

  case 65:
#line 97 "wktparse.y"
    { pop();}
    break;

  case 66:
#line 97 "wktparse.y"
    { set_zm(0, 1); alloc_multipolygon(); }
    break;

  case 67:
#line 97 "wktparse.y"
    { pop();}
    break;

  case 69:
#line 99 "wktparse.y"
    { alloc_counter(); }
    break;

  case 70:
#line 99 "wktparse.y"
    { pop();}
    break;

  case 73:
#line 107 "wktparse.y"
    { alloc_geomertycollection(); }
    break;

  case 74:
#line 107 "wktparse.y"
    { pop();}
    break;

  case 75:
#line 107 "wktparse.y"
    { set_zm(0, 1); alloc_geomertycollection(); }
    break;

  case 76:
#line 107 "wktparse.y"
    { pop();}
    break;

  case 78:
#line 109 "wktparse.y"
    { alloc_counter(); }
    break;

  case 79:
#line 109 "wktparse.y"
    { pop();}
    break;

  case 86:
#line 116 "wktparse.y"
    {alloc_point_2d(yyvsp[-1].value,yyvsp[0].value); }
    break;

  case 87:
#line 118 "wktparse.y"
    {alloc_point_3d(yyvsp[-2].value,yyvsp[-1].value,yyvsp[0].value); }
    break;

  case 88:
#line 120 "wktparse.y"
    {alloc_point_4d(yyvsp[-3].value,yyvsp[-2].value,yyvsp[-1].value,yyvsp[0].value); }
    break;

  case 89:
#line 122 "wktparse.y"
    {alloc_empty(); }
    break;


    }

/* Line 991 of yacc.c.  */
#line 1398 "y.tab.c"

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


#line 123 "wktparse.y"






