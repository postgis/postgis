/* A Bison parser, made from wktparse.y
   by GNU bison 1.32.  */

#define YYBISON 1  /* Identify Bison output.  */

#define yyparse lwg_parse_yyparse
#define yylex lwg_parse_yylex
#define yyerror lwg_parse_yyerror
#define yylval lwg_parse_yylval
#define yychar lwg_parse_yychar
#define yydebug lwg_parse_yydebug
#define yynerrs lwg_parse_yynerrs
# define	POINT	257
# define	LINESTRING	258
# define	POLYGON	259
# define	MULTIPOINT	260
# define	MULTILINESTRING	261
# define	MULTIPOLYGON	262
# define	GEOMETRYCOLLECTION	263
# define	SRID	264
# define	EMPTY	265
# define	VALUE	266
# define	LPAREN	267
# define	RPAREN	268
# define	COMMA	269
# define	EQUALS	270
# define	SEMICOLON	271
# define	WKB	272

#line 9 "wktparse.y"

#include "wktparse.h"


#line 16 "wktparse.y"
#ifndef YYSTYPE
typedef union {
	double value;
	const char* wkb;
} yystype;
# define YYSTYPE yystype
#endif
#ifndef YYDEBUG
# define YYDEBUG 0
#endif



#define	YYFINAL		108
#define	YYFLAG		-32768
#define	YYNTBASE	19

/* YYTRANSLATE(YYLEX) -- Bison token number corresponding to YYLEX. */
#define YYTRANSLATE(x) ((unsigned)(x) <= 272 ? yytranslate[x] : 68)

/* YYTRANSLATE[YYLEX] -- Bison token number corresponding to YYLEX. */
static const char yytranslate[] =
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
       2,     2,     2,     2,     2,     2,     1,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18
};

#if YYDEBUG
static const short yyprhs[] =
{
       0,     0,     1,     6,     7,    10,    12,    14,    16,    18,
      20,    22,    24,    26,    30,    32,    35,    36,    39,    41,
      45,    46,    50,    52,    53,    58,    60,    64,    66,    67,
      70,    73,    74,    77,    79,    80,    85,    87,    91,    92,
      96,    98,    99,   104,   106,   110,   113,   114,   117,   119,
     120,   125,   127,   131,   132,   136,   138,   139,   144,   146,
     150,   151,   155,   157,   158,   163,   165,   169,   171,   173,
     175,   178,   182,   187
};
static const short yyrhs[] =
{
      -1,    23,    17,    20,    22,     0,     0,    21,    22,     0,
      24,     0,    25,     0,    36,     0,    47,     0,    29,     0,
      42,     0,    53,     0,    58,     0,    10,    16,    12,     0,
      18,     0,     3,    26,     0,     0,    27,    28,     0,    67,
       0,    13,    63,    14,     0,     0,     6,    30,    31,     0,
      67,     0,     0,    32,    13,    33,    14,     0,    34,     0,
      33,    15,    34,     0,    26,     0,     0,    35,    63,     0,
       4,    37,     0,     0,    38,    39,     0,    67,     0,     0,
      40,    13,    41,    14,     0,    63,     0,    41,    15,    63,
       0,     0,     7,    43,    44,     0,    67,     0,     0,    45,
      13,    46,    14,     0,    37,     0,    46,    15,    37,     0,
       5,    48,     0,     0,    49,    50,     0,    67,     0,     0,
      51,    13,    52,    14,     0,    39,     0,    52,    15,    39,
       0,     0,     8,    54,    55,     0,    67,     0,     0,    56,
      13,    57,    14,     0,    48,     0,    57,    15,    48,     0,
       0,     9,    59,    60,     0,    67,     0,     0,    61,    13,
      62,    14,     0,    22,     0,    62,    15,    22,     0,    64,
       0,    65,     0,    66,     0,    12,    12,     0,    12,    12,
      12,     0,    12,    12,    12,    12,     0,    11,     0
};

#endif

#if YYDEBUG
/* YYRLINE[YYN] -- source line where rule number YYN was defined. */
static const short yyrline[] =
{
       0,    30,    30,    31,    31,    33,    33,    33,    33,    33,
      34,    34,    34,    36,    38,    43,    45,    45,    47,    47,
      51,    51,    53,    53,    53,    55,    55,    57,    57,    57,
      62,    64,    64,    66,    66,    66,    68,    68,    72,    72,
      74,    74,    74,    76,    76,    81,    83,    83,    85,    85,
      85,    87,    87,    91,    91,    93,    93,    93,    95,    95,
     101,   101,   103,   103,   103,   105,   105,   108,   108,   108,
     110,   112,   114,   116
};
#endif


#if (YYDEBUG) || defined YYERROR_VERBOSE

/* YYTNAME[TOKEN_NUM] -- String name of the token TOKEN_NUM. */
static const char *const yytname[] =
{
  "$", "error", "$undefined.", "POINT", "LINESTRING", "POLYGON", 
  "MULTIPOINT", "MULTILINESTRING", "MULTIPOLYGON", "GEOMETRYCOLLECTION", 
  "SRID", "EMPTY", "VALUE", "LPAREN", "RPAREN", "COMMA", "EQUALS", 
  "SEMICOLON", "WKB", "geometry", "@1", "@2", "geometry_int", "srid", 
  "geom_wkb", "geom_point", "point", "@3", "point_int", "geom_multipoint", 
  "@4", "multipoint", "@5", "multipoint_int", "mpoint", "@6", 
  "geom_linestring", "linestring", "@7", "linestring_1", "@8", 
  "linestring_int", "geom_multilinestring", "@9", "multilinestring", 
  "@10", "multilinestring_int", "geom_polygon", "polygon", "@11", 
  "polygon_1", "@12", "polygon_int", "geom_multipolygon", "@13", 
  "multipolygon", "@14", "multipolygon_int", "geom_geometrycollection", 
  "@15", "geometrycollection", "@16", "geometrycollection_int", "a_point", 
  "point_2d", "point_3d", "point_4d", "empty", NULL
};
#endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives. */
static const short yyr1[] =
{
       0,    20,    19,    21,    19,    22,    22,    22,    22,    22,
      22,    22,    22,    23,    24,    25,    27,    26,    28,    28,
      30,    29,    31,    32,    31,    33,    33,    34,    35,    34,
      36,    38,    37,    39,    40,    39,    41,    41,    43,    42,
      44,    45,    44,    46,    46,    47,    49,    48,    50,    51,
      50,    52,    52,    54,    53,    55,    56,    55,    57,    57,
      59,    58,    60,    61,    60,    62,    62,    63,    63,    63,
      64,    65,    66,    67
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN. */
static const short yyr2[] =
{
       0,     0,     4,     0,     2,     1,     1,     1,     1,     1,
       1,     1,     1,     3,     1,     2,     0,     2,     1,     3,
       0,     3,     1,     0,     4,     1,     3,     1,     0,     2,
       2,     0,     2,     1,     0,     4,     1,     3,     0,     3,
       1,     0,     4,     1,     3,     2,     0,     2,     1,     0,
       4,     1,     3,     0,     3,     1,     0,     4,     1,     3,
       0,     3,     1,     0,     4,     1,     3,     1,     1,     1,
       2,     3,     4,     1
};

/* YYDEFACT[S] -- default rule to reduce with in state S when YYTABLE
   doesn't specify something else to do.  Zero means the default is an
   error. */
static const short yydefact[] =
{
       3,     0,     0,     0,     0,    16,    31,    46,    20,    38,
      53,    60,    14,     4,     5,     6,     9,     7,    10,     8,
      11,    12,     1,    13,    15,     0,    30,    34,    45,    49,
      23,    41,    56,    63,     0,    73,     0,    17,    18,    32,
       0,    33,    47,     0,    48,    21,     0,    22,    39,     0,
      40,    54,     0,    55,    61,     0,    62,     2,     0,     0,
      67,    68,    69,     0,    34,    16,    31,    46,     0,    70,
      19,     0,    36,    51,     0,    27,     0,    25,     0,    43,
       0,    58,     0,    65,     0,    71,    35,     0,    50,    34,
      24,    16,    29,    42,    31,    57,    46,    64,     0,    72,
      37,    52,    26,    44,    59,    66,     0,     0,     0
};

static const short yydefgoto[] =
{
     106,    34,     2,    13,     3,    14,    15,    75,    25,    37,
      16,    30,    45,    46,    76,    77,    78,    17,    26,    27,
      39,    40,    71,    18,    31,    48,    49,    80,    19,    28,
      29,    42,    43,    74,    20,    32,    51,    52,    82,    21,
      33,    54,    55,    84,    59,    60,    61,    62,    41
};

static const short yypact[] =
{
       3,    -1,     2,     9,     4,-32768,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,-32768,-32768,     1,-32768,     7,-32768,     7,
       7,     7,     7,     7,     2,-32768,    17,-32768,-32768,-32768,
      32,-32768,-32768,    33,-32768,-32768,    34,-32768,-32768,    35,
  -32768,-32768,    36,-32768,-32768,    37,-32768,-32768,    39,    38,
  -32768,-32768,-32768,    17,     7,    41,-32768,-32768,     2,    42,
  -32768,    18,-32768,-32768,    21,-32768,    23,-32768,    17,-32768,
      25,-32768,    27,-32768,    29,    43,-32768,    17,-32768,     7,
  -32768,    41,-32768,-32768,-32768,-32768,-32768,-32768,     2,-32768,
  -32768,-32768,-32768,-32768,-32768,-32768,    56,    57,-32768
};

static const short yypgoto[] =
{
  -32768,-32768,-32768,   -34,-32768,-32768,-32768,    53,-32768,-32768,
  -32768,-32768,-32768,-32768,-32768,   -32,-32768,-32768,   -63,-32768,
     -62,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,   -66,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,-32768,   -59,-32768,-32768,-32768,    -8
};


#define	YYLAST		64


static const short yytable[] =
{
      57,    81,    73,    79,    72,     5,     6,     7,     8,     9,
      10,    11,    35,     1,    36,     4,    23,    38,    35,    92,
      12,    44,    47,    50,    53,    56,    22,   101,   100,    58,
     104,   103,    86,    87,    83,    88,    89,    90,    91,    93,
      94,    95,    96,    97,    98,    63,    64,    65,    66,    67,
      68,    69,    70,   -28,    85,    99,   107,   108,    24,   102,
       0,     0,     0,     0,   105
};

static const short yycheck[] =
{
      34,    67,    64,    66,    63,     3,     4,     5,     6,     7,
       8,     9,    11,    10,    13,    16,    12,    25,    11,    78,
      18,    29,    30,    31,    32,    33,    17,    89,    87,    12,
      96,    94,    14,    15,    68,    14,    15,    14,    15,    14,
      15,    14,    15,    14,    15,    13,    13,    13,    13,    13,
      13,    12,    14,    12,    12,    12,     0,     0,     5,    91,
      -1,    -1,    -1,    -1,    98
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/share/bison/bison.simple"

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001 Free Software Foundation, Inc.

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

/* This is the parser code that is written into each bison parser when
   the %semantic_parser declaration is not specified in the grammar.
   It was written by Richard Stallman by simplifying the hairy parser
   used when %semantic_parser is specified.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

#ifdef __cplusplus
# define YYSTD(x) std::x
#else
# define YYSTD(x) x
#endif

#if ! defined (yyoverflow) || defined (YYERROR_VERBOSE)

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# if YYSTACK_USE_ALLOCA
#  define YYSTACK_ALLOC alloca
#  define YYSIZE_T YYSTD (size_t)
# else
#  ifndef YYSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define YYSTACK_ALLOC alloca
#    define YYSIZE_T YYSTD (size_t)
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
#  ifdef __cplusplus
#   include <cstdlib> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T std::size_t
#  else
#   ifdef __STDC__
#    include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#    define YYSIZE_T size_t
#   endif
#  endif
#  define YYSTACK_ALLOC YYSTD (malloc)
#  define YYSTACK_FREE YYSTD (free)
# endif

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
# if YYLSP_NEEDED
  YYLTYPE yyls;
# endif
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAX (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# if YYLSP_NEEDED
#  define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE) + sizeof (YYLTYPE))	\
      + 2 * YYSTACK_GAP_MAX)
# else
#  define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAX)
# endif

/* Relocate the TYPE STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Type, Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	yymemcpy ((char *) yyptr, (char *) (Stack),			\
		  yysize * (YYSIZE_T) sizeof (Type));			\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (Type) + YYSTACK_GAP_MAX;	\
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif /* ! defined (yyoverflow) || defined (YYERROR_VERBOSE) */


#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# ifdef __cplusplus
#  include <cstddef> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T std::size_t
# else
#  ifdef __STDC__
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	goto yyacceptlab
#define YYABORT 	goto yyabortlab
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
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");			\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).

   When YYLLOC_DEFAULT is run, CURRENT is set the location of the
   first token.  By default, to implement support for ranges, extend
   its range to the last symbol.  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)       	\
   Current.last_line   = Rhs[N].last_line;	\
   Current.last_column = Rhs[N].last_column;
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#if YYPURE
# if YYLSP_NEEDED
#  ifdef YYLEX_PARAM
#   define YYLEX		yylex (&yylval, &yylloc, YYLEX_PARAM)
#  else
#   define YYLEX		yylex (&yylval, &yylloc)
#  endif
# else /* !YYLSP_NEEDED */
#  ifdef YYLEX_PARAM
#   define YYLEX		yylex (&yylval, YYLEX_PARAM)
#  else
#   define YYLEX		yylex (&yylval)
#  endif
# endif /* !YYLSP_NEEDED */
#else /* !YYPURE */
# define YYLEX			yylex ()
#endif /* !YYPURE */


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  ifdef __cplusplus
#   include <cstdio>  /* INFRINGES ON USER NAME SPACE */
#  else
#   include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYFPRINTF YYSTD (fprintf)
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)
/* Nonzero means print parse trace. [The following comment makes no
   sense to me.  Could someone clarify it?  --akim] Since this is
   uninitialized, it does not stop multiple parsers from coexisting.
   */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
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

#if ! defined (yyoverflow) && ! defined (yymemcpy)
# if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#  define yymemcpy __builtin_memcpy
# else				/* not GNU C or C++ */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
#  if defined (__STDC__) || defined (__cplusplus)
yymemcpy (char *yyto, const char *yyfrom, YYSIZE_T yycount)
#  else
yymemcpy (yyto, yyfrom, yycount)
     char *yyto;
     const char *yyfrom;
     YYSIZE_T yycount;
#  endif
{
  register const char *yyf = yyfrom;
  register char *yyt = yyto;
  register YYSIZE_T yyi = yycount;

  while (yyi-- != 0)
    *yyt++ = *yyf++;
}
# endif
#endif

#ifdef YYERROR_VERBOSE

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
#endif

#line 341 "/usr/share/bison/bison.simple"


/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
# ifdef __cplusplus
#  define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#  define YYPARSE_PARAM_DECL
# else /* !__cplusplus */
#  define YYPARSE_PARAM_ARG YYPARSE_PARAM
#  define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
# endif /* !__cplusplus */
#else /* !YYPARSE_PARAM */
# define YYPARSE_PARAM_ARG
# define YYPARSE_PARAM_DECL
#endif /* !YYPARSE_PARAM */

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
# ifdef YYPARSE_PARAM
int yyparse (void *);
# else
int yyparse (void);
# endif
#endif

/* YY_DECL_VARIABLES -- depending whether we use a pure parser,
   variables are global, or local to YYPARSE.  */

#define YY_DECL_NON_LSP_VARIABLES			\
/* The lookahead symbol.  */				\
int yychar;						\
							\
/* The semantic value of the lookahead symbol. */	\
YYSTYPE yylval;						\
							\
/* Number of parse errors so far.  */			\
int yynerrs;

#if YYLSP_NEEDED
# define YY_DECL_VARIABLES			\
YY_DECL_NON_LSP_VARIABLES			\
						\
/* Location data for the lookahead symbol.  */	\
YYLTYPE yylloc;
#else
# define YY_DECL_VARIABLES			\
YY_DECL_NON_LSP_VARIABLES
#endif


/* If nonreentrant, generate the variables here. */

#if !YYPURE
YY_DECL_VARIABLES
#endif  /* !YYPURE */

int
yyparse (YYPARSE_PARAM_ARG)
     YYPARSE_PARAM_DECL
{
  /* If reentrant, generate the variables here. */
#if YYPURE
  YY_DECL_VARIABLES
#endif  /* !YYPURE */

  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yychar1 = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack. */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;

#if YYLSP_NEEDED
  /* The location stack.  */
  YYLTYPE yylsa[YYINITDEPTH];
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;
#endif

#if YYLSP_NEEDED
# define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
# define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  YYSIZE_T yystacksize = YYINITDEPTH;


  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
#if YYLSP_NEEDED
  YYLTYPE yyloc;
#endif

  /* When reducing, the number of symbols on the RHS of the reduced
     rule. */
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
#if YYLSP_NEEDED
  yylsp = yyls;
#endif
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

  if (yyssp >= yyss + yystacksize - 1)
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
	   data in use in that stack, in bytes.  */
# if YYLSP_NEEDED
	YYLTYPE *yyls1 = yyls;
	/* This used to be a conditional around just the two extra args,
	   but that might be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp),
		    &yystacksize);
	yyls = yyls1;
# else
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);
# endif
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (short, yyss);
	YYSTACK_RELOCATE (YYSTYPE, yyvs);
# if YYLSP_NEEDED
	YYSTACK_RELOCATE (YYLTYPE, yyls);
# endif
# undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
#if YYLSP_NEEDED
      yylsp = yyls + yysize - 1;
#endif

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyssp >= yyss + yystacksize - 1)
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
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yychar1 = YYTRANSLATE (yychar);

#if YYDEBUG
     /* We have to keep this `#if YYDEBUG', since we use variables
	which are defined only if `YYDEBUG' is set.  */
      if (yydebug)
	{
	  YYFPRINTF (stderr, "Next token is %d (%s",
		     yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise
	     meaning of a token, for further debugging info.  */
# ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
# endif
	  YYFPRINTF (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %d (%s), ",
	      yychar, yytname[yychar1]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#if YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

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

     Otherwise, the following line sets YYVAL to the semantic value of
     the lookahead token.  This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

#if YYLSP_NEEDED
  /* Similarly for the default location.  Let the user run additional
     commands if for instance locations are ranges.  */
  yyloc = yylsp[1-yylen];
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
#endif

#if YYDEBUG
  /* We have to keep this `#if YYDEBUG', since we use variables which
     are defined only if `YYDEBUG' is set.  */
  if (yydebug)
    {
      int yyi;

      YYFPRINTF (stderr, "Reducing via rule %d (line %d), ",
		 yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (yyi = yyprhs[yyn]; yyrhs[yyi] > 0; yyi++)
	YYFPRINTF (stderr, "%s ", yytname[yyrhs[yyi]]);
      YYFPRINTF (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif

  switch (yyn) {

case 1:
#line 30 "wktparse.y"
{ alloc_lwgeom(srid); ;
    break;}
case 3:
#line 31 "wktparse.y"
{ alloc_lwgeom(-1); ;
    break;}
case 13:
#line 36 "wktparse.y"
{ set_srid(yyvsp[0].value); ;
    break;}
case 14:
#line 38 "wktparse.y"
{alloc_wkb(yyvsp[0].wkb) ; ;
    break;}
case 16:
#line 45 "wktparse.y"
{ alloc_point(); ;
    break;}
case 17:
#line 45 "wktparse.y"
{ pop();;
    break;}
case 20:
#line 51 "wktparse.y"
{ alloc_multipoint(); ;
    break;}
case 21:
#line 51 "wktparse.y"
{ pop();;
    break;}
case 23:
#line 53 "wktparse.y"
{ alloc_counter(); ;
    break;}
case 24:
#line 53 "wktparse.y"
{pop();;
    break;}
case 28:
#line 57 "wktparse.y"
{ alloc_point(); ;
    break;}
case 29:
#line 57 "wktparse.y"
{ pop();;
    break;}
case 31:
#line 64 "wktparse.y"
{ alloc_linestring(); ;
    break;}
case 32:
#line 64 "wktparse.y"
{pop();;
    break;}
case 34:
#line 66 "wktparse.y"
{ alloc_counter(); ;
    break;}
case 35:
#line 66 "wktparse.y"
{popc(); ;
    break;}
case 38:
#line 72 "wktparse.y"
{ alloc_multilinestring(); ;
    break;}
case 39:
#line 72 "wktparse.y"
{ pop();;
    break;}
case 41:
#line 74 "wktparse.y"
{ alloc_counter(); ;
    break;}
case 42:
#line 74 "wktparse.y"
{ pop();;
    break;}
case 46:
#line 83 "wktparse.y"
{ alloc_polygon(); ;
    break;}
case 47:
#line 83 "wktparse.y"
{ pop();;
    break;}
case 49:
#line 85 "wktparse.y"
{ alloc_counter(); ;
    break;}
case 50:
#line 85 "wktparse.y"
{ pop();;
    break;}
case 53:
#line 91 "wktparse.y"
{ alloc_multipolygon(); ;
    break;}
case 54:
#line 91 "wktparse.y"
{ pop();;
    break;}
case 56:
#line 93 "wktparse.y"
{ alloc_counter(); ;
    break;}
case 57:
#line 93 "wktparse.y"
{ pop();;
    break;}
case 60:
#line 101 "wktparse.y"
{ alloc_geomertycollection(); ;
    break;}
case 61:
#line 101 "wktparse.y"
{ pop();;
    break;}
case 63:
#line 103 "wktparse.y"
{ alloc_counter(); ;
    break;}
case 64:
#line 103 "wktparse.y"
{ pop();;
    break;}
case 70:
#line 110 "wktparse.y"
{alloc_point_2d(yyvsp[-1].value,yyvsp[0].value) ;
    break;}
case 71:
#line 112 "wktparse.y"
{alloc_point_3d(yyvsp[-2].value,yyvsp[-1].value,yyvsp[0].value) ;
    break;}
case 72:
#line 114 "wktparse.y"
{alloc_point_4d(yyvsp[-3].value,yyvsp[-2].value,yyvsp[-1].value,yyvsp[0].value) ;
    break;}
case 73:
#line 116 "wktparse.y"
{alloc_empty() ;
    break;}
}

#line 727 "/usr/share/bison/bison.simple"


  yyvsp -= yylen;
  yyssp -= yylen;
#if YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG
  if (yydebug)
    {
      short *yyssp1 = yyss - 1;
      YYFPRINTF (stderr, "state stack now");
      while (yyssp1 != yyssp)
	YYFPRINTF (stderr, " %d", *++yyssp1);
      YYFPRINTF (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;
#if YYLSP_NEEDED
  *++yylsp = yyloc;
#endif

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  char *yymsg;
	  int yyx, yycount;

	  yycount = 0;
	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  for (yyx = yyn < 0 ? -yyn : 0;
	       yyx < (int) (sizeof (yytname) / sizeof (char *)); yyx++)
	    if (yycheck[yyx + yyn] == yyx)
	      yysize += yystrlen (yytname[yyx]) + 15, yycount++;
	  yysize += yystrlen ("parse error, unexpected ") + 1;
	  yysize += yystrlen (yytname[YYTRANSLATE (yychar)]);
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "parse error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[YYTRANSLATE (yychar)]);

	      if (yycount < 5)
		{
		  yycount = 0;
		  for (yyx = yyn < 0 ? -yyn : 0;
		       yyx < (int) (sizeof (yytname) / sizeof (char *));
		       yyx++)
		    if (yycheck[yyx + yyn] == yyx)
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
	    yyerror ("parse error; also virtual memory exhausted");
	}
      else
#endif /* defined (YYERROR_VERBOSE) */
	yyerror ("parse error");
    }
  goto yyerrlab1;


/*--------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action |
`--------------------------------------------------*/
yyerrlab1:
  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;
      YYDPRINTF ((stderr, "Discarding token %d (%s).\n",
		  yychar, yytname[yychar1]));
      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;


/*-------------------------------------------------------------------.
| yyerrdefault -- current state does not do anything special for the |
| error token.                                                       |
`-------------------------------------------------------------------*/
yyerrdefault:
#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */

  /* If its default is to accept any token, ok.  Otherwise pop it.  */
  yyn = yydefact[yystate];
  if (yyn)
    goto yydefault;
#endif


/*---------------------------------------------------------------.
| yyerrpop -- pop the current state because it cannot handle the |
| error token                                                    |
`---------------------------------------------------------------*/
yyerrpop:
  if (yyssp == yyss)
    YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#if YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG
  if (yydebug)
    {
      short *yyssp1 = yyss - 1;
      YYFPRINTF (stderr, "Error: state stack now");
      while (yyssp1 != yyssp)
	YYFPRINTF (stderr, " %d", *++yyssp1);
      YYFPRINTF (stderr, "\n");
    }
#endif

/*--------------.
| yyerrhandle.  |
`--------------*/
yyerrhandle:
  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;
#if YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

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

/*---------------------------------------------.
| yyoverflowab -- parser overflow comes here.  |
`---------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}
#line 117 "wktparse.y"





