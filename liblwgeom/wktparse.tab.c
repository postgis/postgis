/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 1

/* Substitute the variable and function names.  */
#define yyparse lwg_parse_yyparse
#define yylex   lwg_parse_yylex
#define yyerror lwg_parse_yyerror
#define yylval  lwg_parse_yylval
#define yychar  lwg_parse_yychar
#define yydebug lwg_parse_yydebug
#define yynerrs lwg_parse_yynerrs
#define yylloc lwg_parse_yylloc

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
     POLYHEDRALSURFACEM = 283,
     SRID = 284,
     EMPTY = 285,
     VALUE = 286,
     LPAREN = 287,
     RPAREN = 288,
     COMMA = 289,
     EQUALS = 290,
     SEMICOLON = 291,
     WKB = 292
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
#define POLYHEDRALSURFACEM 283
#define SRID 284
#define EMPTY 285
#define VALUE 286
#define LPAREN 287
#define RPAREN 288
#define COMMA 289
#define EQUALS 290
#define SEMICOLON 291
#define WKB 292




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

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 22 "wktparse.y"
{
	double value;
	const char* wkb;
}
/* Line 193 of yacc.c.  */
#line 192 "y.tab.c"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

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


/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 217 "y.tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
	     && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss;
  YYSTYPE yyvs;
    YYLTYPE yyls;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
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
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  6
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   215

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  38
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  132
/* YYNRULES -- Number of rules.  */
#define YYNRULES  204
/* YYNRULES -- Number of states.  */
#define YYNSTATES  285

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   292

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
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
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     4,     9,    10,    13,    15,    17,    19,
      21,    23,    25,    27,    29,    31,    33,    35,    37,    39,
      41,    45,    47,    50,    51,    55,    57,    59,    60,    63,
      64,    67,    71,    72,    76,    77,    81,    83,    84,    89,
      91,    95,    97,    98,   101,   104,   105,   109,   111,   113,
     114,   117,   118,   121,   122,   125,   126,   131,   133,   137,
     140,   141,   145,   148,   149,   153,   155,   157,   159,   161,
     162,   165,   166,   169,   170,   173,   174,   179,   181,   185,
     188,   189,   193,   196,   197,   201,   203,   205,   207,   209,
     210,   213,   214,   217,   218,   221,   222,   225,   226,   231,
     233,   235,   239,   243,   244,   248,   249,   253,   255,   256,
     261,   263,   267,   268,   272,   273,   277,   279,   280,   285,
     287,   289,   293,   297,   300,   301,   305,   307,   309,   310,
     313,   314,   317,   318,   323,   325,   329,   330,   334,   335,
     339,   341,   342,   347,   349,   351,   353,   357,   361,   365,
     366,   370,   371,   375,   377,   378,   383,   385,   389,   390,
     394,   395,   399,   401,   402,   407,   409,   411,   415,   419,
     420,   423,   424,   429,   431,   435,   436,   441,   443,   447,
     448,   452,   453,   457,   459,   460,   465,   467,   471,   472,
     476,   477,   481,   483,   484,   489,   491,   493,   497,   499,
     501,   503,   506,   510,   515
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
      39,     0,    -1,    -1,    43,    36,    40,    42,    -1,    -1,
      41,    42,    -1,    44,    -1,    45,    -1,    61,    -1,    73,
      -1,   117,    -1,    88,    -1,   127,    -1,    53,    -1,   105,
      -1,   111,    -1,   133,    -1,   139,    -1,   153,    -1,   159,
      -1,    29,    35,    31,    -1,    37,    -1,     3,    47,    -1,
      -1,    16,    46,    47,    -1,    48,    -1,    50,    -1,    -1,
      49,   169,    -1,    -1,    51,    52,    -1,    32,   165,    33,
      -1,    -1,     6,    54,    56,    -1,    -1,    19,    55,    56,
      -1,   169,    -1,    -1,    57,    32,    58,    33,    -1,    59,
      -1,    58,    34,    59,    -1,    50,    -1,    -1,    60,   165,
      -1,     4,    63,    -1,    -1,    17,    62,    63,    -1,    64,
      -1,    66,    -1,    -1,    65,   169,    -1,    -1,    67,    70,
      -1,    -1,    69,    70,    -1,    -1,    71,    32,    72,    33,
      -1,   165,    -1,    72,    34,   165,    -1,    10,    77,    -1,
      -1,    23,    74,    77,    -1,    10,    78,    -1,    -1,    23,
      76,    78,    -1,    79,    -1,    81,    -1,    79,    -1,    83,
      -1,    -1,    80,   169,    -1,    -1,    82,    85,    -1,    -1,
      84,    85,    -1,    -1,    86,    32,    87,    33,    -1,   165,
      -1,    87,    34,   165,    -1,    11,    92,    -1,    -1,    24,
      89,    92,    -1,    11,    93,    -1,    -1,    24,    91,    93,
      -1,    94,    -1,    98,    -1,    96,    -1,   100,    -1,    -1,
      95,   169,    -1,    -1,    97,   169,    -1,    -1,    99,   102,
      -1,    -1,   101,   102,    -1,    -1,   103,    32,   104,    33,
      -1,    66,    -1,    73,    -1,   104,    34,    66,    -1,   104,
      34,    73,    -1,    -1,     7,   106,   108,    -1,    -1,    20,
     107,   108,    -1,   169,    -1,    -1,   109,    32,   110,    33,
      -1,    66,    -1,   110,    34,    66,    -1,    -1,    13,   112,
     114,    -1,    -1,    26,   113,   114,    -1,   169,    -1,    -1,
     115,    32,   116,    33,    -1,    66,    -1,    73,    -1,   116,
      34,    66,    -1,   116,    34,    73,    -1,     5,   119,    -1,
      -1,    18,   118,   119,    -1,   120,    -1,   122,    -1,    -1,
     121,   169,    -1,    -1,   123,   124,    -1,    -1,   125,    32,
     126,    33,    -1,    70,    -1,   126,    34,    70,    -1,    -1,
      12,   128,   130,    -1,    -1,    25,   129,   130,    -1,   169,
      -1,    -1,   131,    32,   132,    33,    -1,    68,    -1,    75,
      -1,    90,    -1,   132,    34,    68,    -1,   132,    34,    75,
      -1,   132,    34,    90,    -1,    -1,     8,   134,   136,    -1,
      -1,    21,   135,   136,    -1,   169,    -1,    -1,   137,    32,
     138,    33,    -1,   122,    -1,   138,    34,   122,    -1,    -1,
      14,   140,   142,    -1,    -1,    27,   141,   142,    -1,   169,
      -1,    -1,   143,    32,   144,    33,    -1,   122,    -1,   127,
      -1,   144,    34,   122,    -1,   144,    34,   127,    -1,    -1,
     146,   147,    -1,    -1,   148,    32,   149,    33,    -1,   150,
      -1,   149,    34,   150,    -1,    -1,   151,    32,   152,    33,
      -1,   165,    -1,   152,    34,   165,    -1,    -1,    15,   154,
     156,    -1,    -1,    28,   155,   156,    -1,   169,    -1,    -1,
     157,    32,   158,    33,    -1,   145,    -1,   158,    34,   145,
      -1,    -1,     9,   160,   162,    -1,    -1,    22,   161,   162,
      -1,   169,    -1,    -1,   163,    32,   164,    33,    -1,   169,
      -1,    42,    -1,   164,    34,    42,    -1,   166,    -1,   167,
      -1,   168,    -1,    31,    31,    -1,    31,    31,    31,    -1,
      31,    31,    31,    31,    -1,    30,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,    38,    38,    38,    40,    40,    43,    45,    47,    49,
      51,    53,    55,    57,    59,    61,    63,    65,    67,    69,
      72,    75,    81,    83,    83,    86,    88,    91,    91,    94,
      94,    97,   102,   102,   104,   104,   107,   109,   109,   112,
     114,   117,   120,   120,   126,   128,   128,   131,   133,   136,
     136,   139,   139,   142,   142,   145,   145,   148,   150,   155,
     157,   157,   160,   162,   162,   165,   167,   170,   172,   175,
     175,   178,   178,   181,   181,   184,   184,   187,   189,   194,
     196,   196,   199,   201,   201,   204,   206,   209,   211,   214,
     214,   217,   217,   220,   220,   223,   223,   226,   226,   229,
     231,   233,   235,   240,   240,   243,   243,   247,   249,   249,
     252,   254,   259,   259,   262,   262,   266,   268,   268,   271,
     273,   275,   277,   282,   284,   284,   287,   289,   292,   292,
     295,   295,   298,   298,   302,   305,   310,   310,   312,   312,
     316,   318,   318,   321,   323,   325,   327,   329,   331,   336,
     336,   338,   338,   342,   344,   344,   347,   349,   354,   354,
     356,   356,   360,   362,   362,   365,   367,   369,   371,   375,
     375,   378,   378,   381,   383,   386,   386,   389,   391,   394,
     394,   396,   396,   399,   401,   401,   404,   406,   412,   412,
     415,   415,   419,   421,   421,   425,   427,   429,   433,   435,
     437,   440,   443,   446,   449
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "POINT", "LINESTRING", "POLYGON",
  "MULTIPOINT", "MULTILINESTRING", "MULTIPOLYGON", "GEOMETRYCOLLECTION",
  "CIRCULARSTRING", "COMPOUNDCURVE", "CURVEPOLYGON", "MULTICURVE",
  "MULTISURFACE", "POLYHEDRALSURFACE", "POINTM", "LINESTRINGM", "POLYGONM",
  "MULTIPOINTM", "MULTILINESTRINGM", "MULTIPOLYGONM",
  "GEOMETRYCOLLECTIONM", "CIRCULARSTRINGM", "COMPOUNDCURVEM",
  "CURVEPOLYGONM", "MULTICURVEM", "MULTISURFACEM", "POLYHEDRALSURFACEM",
  "SRID", "EMPTY", "VALUE", "LPAREN", "RPAREN", "COMMA", "EQUALS",
  "SEMICOLON", "WKB", "$accept", "geometry", "@1", "@2", "geometry_int",
  "srid", "geom_wkb", "geom_point", "@3", "point", "empty_point", "@4",
  "nonempty_point", "@5", "point_int", "geom_multipoint", "@6", "@7",
  "multipoint", "@8", "multipoint_int", "mpoint_element", "@9",
  "geom_linestring", "@10", "linestring", "empty_linestring", "@11",
  "nonempty_linestring", "@12", "nonempty_linestring_closed", "@13",
  "linestring_1", "@14", "linestring_int", "geom_circularstring", "@15",
  "geom_circularstring_closed", "@16", "circularstring",
  "circularstring_closed", "empty_circularstring", "@17",
  "nonempty_circularstring", "@18", "nonempty_circularstring_closed",
  "@19", "circularstring_1", "@20", "circularstring_int",
  "geom_compoundcurve", "@21", "geom_compoundcurve_closed", "@22",
  "compoundcurve", "compoundcurve_closed", "empty_compoundcurve", "@23",
  "empty_compoundcurve_closed", "@24", "nonempty_compoundcurve", "@25",
  "nonempty_compoundcurve_closed", "@26", "compoundcurve_1", "@27",
  "compoundcurve_int", "geom_multilinestring", "@28", "@29",
  "multilinestring", "@30", "multilinestring_int", "geom_multicurve",
  "@31", "@32", "multicurve", "@33", "multicurve_int", "geom_polygon",
  "@34", "polygon", "empty_polygon", "@35", "nonempty_polygon", "@36",
  "polygon_1", "@37", "polygon_int", "geom_curvepolygon", "@38", "@39",
  "curvepolygon", "@40", "curvepolygon_int", "geom_multipolygon", "@41",
  "@42", "multipolygon", "@43", "multipolygon_int", "geom_multisurface",
  "@44", "@45", "multisurface", "@46", "multisurface_int", "face", "@47",
  "face_1", "@48", "face_rings", "face_int", "@49", "face_int1",
  "geom_polyhedralsurface", "@50", "@51", "polyhedralsurface", "@52",
  "polyhedralsurface_int", "geom_geometrycollection", "@53", "@54",
  "geometrycollection", "@55", "geometrycollection_int", "a_point",
  "point_2d", "point_3d", "point_4d", "empty", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    38,    40,    39,    41,    39,    42,    42,    42,    42,
      42,    42,    42,    42,    42,    42,    42,    42,    42,    42,
      43,    44,    45,    46,    45,    47,    47,    49,    48,    51,
      50,    52,    54,    53,    55,    53,    56,    57,    56,    58,
      58,    59,    60,    59,    61,    62,    61,    63,    63,    65,
      64,    67,    66,    69,    68,    71,    70,    72,    72,    73,
      74,    73,    75,    76,    75,    77,    77,    78,    78,    80,
      79,    82,    81,    84,    83,    86,    85,    87,    87,    88,
      89,    88,    90,    91,    90,    92,    92,    93,    93,    95,
      94,    97,    96,    99,    98,   101,   100,   103,   102,   104,
     104,   104,   104,   106,   105,   107,   105,   108,   109,   108,
     110,   110,   112,   111,   113,   111,   114,   115,   114,   116,
     116,   116,   116,   117,   118,   117,   119,   119,   121,   120,
     123,   122,   125,   124,   126,   126,   128,   127,   129,   127,
     130,   131,   130,   132,   132,   132,   132,   132,   132,   134,
     133,   135,   133,   136,   137,   136,   138,   138,   140,   139,
     141,   139,   142,   143,   142,   144,   144,   144,   144,   146,
     145,   148,   147,   149,   149,   151,   150,   152,   152,   154,
     153,   155,   153,   156,   157,   156,   158,   158,   160,   159,
     161,   159,   162,   163,   162,   164,   164,   164,   165,   165,
     165,   166,   167,   168,   169
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     4,     0,     2,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       3,     1,     2,     0,     3,     1,     1,     0,     2,     0,
       2,     3,     0,     3,     0,     3,     1,     0,     4,     1,
       3,     1,     0,     2,     2,     0,     3,     1,     1,     0,
       2,     0,     2,     0,     2,     0,     4,     1,     3,     2,
       0,     3,     2,     0,     3,     1,     1,     1,     1,     0,
       2,     0,     2,     0,     2,     0,     4,     1,     3,     2,
       0,     3,     2,     0,     3,     1,     1,     1,     1,     0,
       2,     0,     2,     0,     2,     0,     2,     0,     4,     1,
       1,     3,     3,     0,     3,     0,     3,     1,     0,     4,
       1,     3,     0,     3,     0,     3,     1,     0,     4,     1,
       1,     3,     3,     2,     0,     3,     1,     1,     0,     2,
       0,     2,     0,     4,     1,     3,     0,     3,     0,     3,
       1,     0,     4,     1,     1,     1,     3,     3,     3,     0,
       3,     0,     3,     1,     0,     4,     1,     3,     0,     3,
       0,     3,     1,     0,     4,     1,     1,     3,     3,     0,
       2,     0,     4,     1,     3,     0,     4,     1,     3,     0,
       3,     0,     3,     1,     0,     4,     1,     3,     0,     3,
       0,     3,     1,     0,     4,     1,     1,     3,     1,     1,
       1,     2,     3,     4,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       4,     0,     0,     0,     0,     0,     1,    27,    49,   128,
      32,   103,   149,   188,    69,    89,   136,   112,   158,   179,
      23,    45,   124,    34,   105,   151,   190,    60,    80,   138,
     114,   160,   181,    21,     5,     6,     7,    13,     8,     9,
      11,    14,    15,    10,    12,    16,    17,    18,    19,     2,
      20,    22,    25,     0,    26,     0,    44,    47,     0,    48,
      55,   123,   126,     0,   127,   132,    37,   108,   154,   193,
      59,    65,     0,    66,    75,    79,    85,     0,    86,    97,
     141,   117,   163,   184,    27,    49,   128,    37,   108,   154,
     193,    69,    89,   141,   117,   163,   184,     0,   204,    28,
       0,    30,    50,    52,     0,   129,   131,     0,    33,     0,
      36,   104,     0,   107,   150,     0,   153,   189,     0,   192,
      70,    72,     0,    90,    94,     0,   137,     0,   140,   113,
       0,   116,   159,     0,   162,   180,     0,   183,    24,    46,
     125,    35,   106,   152,   191,    61,    81,   139,   115,   161,
     182,     3,     0,     0,   198,   199,   200,     0,    55,    29,
      51,   130,     0,     0,    51,    53,    51,   130,   169,   201,
      31,     0,    57,   134,     0,    41,     0,    39,     0,   110,
       0,   156,     0,   196,     0,   195,     0,    77,    99,   100,
       0,    69,    91,    63,    83,   143,    55,   144,   145,     0,
     119,   120,     0,   165,   166,     0,   186,   171,     0,   202,
      56,     0,   133,    55,    38,    29,    43,   109,    51,   155,
     130,   194,     0,    76,     0,    98,    51,    62,    67,    68,
      75,    82,    87,     0,    88,    97,    69,    91,    54,   142,
      53,   118,    51,   164,   130,   170,     0,   185,   169,   203,
      58,   135,    40,   111,   157,   197,    78,   101,   102,    74,
      92,    96,    64,    84,   146,   147,   148,   121,   122,   167,
     168,   175,   187,     0,   173,     0,   172,   175,     0,   174,
       0,   177,   176,     0,   178
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,    97,     3,    34,     4,    35,    36,    84,    51,
      52,    53,    54,    55,   101,    37,    66,    87,   108,   109,
     176,   177,   178,    38,    85,    56,    57,    58,    59,    60,
     195,   196,   103,   104,   171,    39,    91,   197,   236,    70,
     227,    71,    72,    73,    74,   229,   230,   121,   122,   186,
      40,    92,   198,   237,    75,   231,    76,    77,   232,   233,
      78,    79,   234,   235,   124,   125,   190,    41,    67,    88,
     111,   112,   180,    42,    81,    94,   129,   130,   202,    43,
      86,    61,    62,    63,    64,    65,   106,   107,   174,    44,
      80,    93,   126,   127,   199,    45,    68,    89,   114,   115,
     182,    46,    82,    95,   132,   133,   205,   206,   207,   245,
     246,   273,   274,   275,   280,    47,    83,    96,   135,   136,
     208,    48,    69,    90,   117,   118,   184,   153,   154,   155,
     156,   110
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -179
static const yytype_int16 yypact[] =
{
      -9,     9,    49,   178,    12,    22,  -179,    38,    42,    52,
    -179,  -179,  -179,  -179,    54,    62,  -179,  -179,  -179,  -179,
    -179,  -179,  -179,  -179,  -179,  -179,  -179,  -179,  -179,  -179,
    -179,  -179,  -179,  -179,  -179,  -179,  -179,  -179,  -179,  -179,
    -179,  -179,  -179,  -179,  -179,  -179,  -179,  -179,  -179,  -179,
    -179,  -179,  -179,    24,  -179,    63,  -179,  -179,    24,  -179,
    -179,  -179,  -179,    24,  -179,  -179,    24,    24,    24,    24,
    -179,  -179,    24,  -179,  -179,  -179,  -179,    24,  -179,  -179,
      24,    24,    24,    24,    38,    42,    52,    24,    24,    24,
      24,    54,    62,    24,    24,    24,    24,   178,  -179,  -179,
      65,  -179,  -179,  -179,    66,  -179,  -179,    68,  -179,    69,
    -179,  -179,    70,  -179,  -179,    71,  -179,  -179,    72,  -179,
    -179,  -179,    73,  -179,  -179,    74,  -179,    75,  -179,  -179,
      76,  -179,  -179,    78,  -179,  -179,    79,  -179,  -179,  -179,
    -179,  -179,  -179,  -179,  -179,  -179,  -179,  -179,  -179,  -179,
    -179,  -179,    81,    64,  -179,  -179,  -179,    65,  -179,    82,
    -179,  -179,   127,    65,     8,    15,     8,    20,  -179,    83,
    -179,     0,  -179,  -179,    13,  -179,    18,  -179,    65,  -179,
      23,  -179,    26,  -179,    30,  -179,    32,  -179,  -179,  -179,
      39,    84,    85,  -179,  -179,  -179,  -179,  -179,  -179,    43,
    -179,  -179,    45,  -179,  -179,    48,  -179,  -179,    55,    87,
    -179,    65,  -179,  -179,  -179,    82,  -179,  -179,  -179,  -179,
    -179,  -179,   178,  -179,    65,  -179,     8,  -179,  -179,  -179,
    -179,  -179,  -179,    24,  -179,  -179,    84,    85,  -179,  -179,
      15,  -179,     8,  -179,    20,  -179,    88,  -179,  -179,  -179,
    -179,  -179,  -179,  -179,  -179,  -179,  -179,  -179,  -179,  -179,
    -179,  -179,  -179,  -179,  -179,  -179,  -179,  -179,  -179,  -179,
    -179,  -179,  -179,    57,  -179,    89,  -179,  -179,    65,  -179,
      59,  -179,  -179,    65,  -179
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -179,  -179,  -179,  -179,   -93,  -179,  -179,  -179,  -179,    31,
    -179,  -179,  -153,  -179,  -179,  -179,  -179,  -179,    36,  -179,
    -179,   -96,  -179,  -179,  -179,    40,  -179,  -179,  -143,  -179,
    -116,  -179,  -146,  -179,  -179,  -155,  -179,  -114,  -179,    37,
     -80,  -178,  -179,  -179,  -179,  -179,  -179,   -72,  -179,  -179,
    -179,  -179,   -81,  -179,    77,   -77,  -179,  -179,  -179,  -179,
    -179,  -179,  -179,  -179,   -74,  -179,  -179,  -179,  -179,  -179,
      80,  -179,  -179,  -179,  -179,  -179,   113,  -179,  -179,  -179,
    -179,    86,  -179,  -179,  -159,  -179,  -179,  -179,  -179,  -164,
    -179,  -179,   115,  -179,  -179,  -179,  -179,  -179,    90,  -179,
    -179,  -179,  -179,  -179,    67,  -179,  -179,   -85,  -179,  -179,
    -179,  -179,  -112,  -179,  -179,  -179,  -179,  -179,   114,  -179,
    -179,  -179,  -179,  -179,   119,  -179,  -179,  -156,  -179,  -179,
    -179,   -53
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -131
static const yytype_int16 yytable[] =
{
      99,   172,   181,   204,   151,   102,   175,   187,   203,   189,
     105,   201,   173,   228,   113,   116,   119,   179,    14,   120,
       1,   188,   216,   200,   123,   191,   192,   128,   131,   134,
     137,    27,    16,   210,   211,   113,   116,   119,   193,   194,
     128,   131,   134,   137,     5,    29,   212,   213,    49,     6,
     238,   214,   215,    50,    98,   250,   217,   218,   228,   219,
     220,   254,   175,   221,   222,   223,   224,   251,   256,   183,
     -29,   258,   225,   226,   -51,   253,   239,   240,   241,   242,
     270,   243,   244,   257,  -130,   269,   -71,   268,   247,   248,
     276,   277,   282,   283,   -93,   100,   152,   170,   157,   267,
     158,   159,   160,   161,   162,   163,   164,   165,   166,   185,
     167,   168,   169,   -42,   209,   138,   -73,   -95,   249,   252,
     271,   278,   281,   141,   264,   139,   265,   284,   145,   255,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,   262,    98,   259,   266,
     263,   261,   149,   272,    33,   279,     0,     0,   142,   146,
       0,     0,   140,     0,     0,     0,     0,     0,     0,   143,
     260,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,   148,   147,   144,
     150,     0,     0,     0,     0,    33
};

static const yytype_int16 yycheck[] =
{
      53,   157,   161,   167,    97,    58,   159,   163,   167,   164,
      63,   166,   158,   191,    67,    68,    69,   160,    10,    72,
      29,   164,   178,   166,    77,    10,    11,    80,    81,    82,
      83,    23,    12,    33,    34,    88,    89,    90,    23,    24,
      93,    94,    95,    96,    35,    25,    33,    34,    36,     0,
     196,    33,    34,    31,    30,   211,    33,    34,   236,    33,
      34,   220,   215,    33,    34,    33,    34,   213,   224,   162,
      32,   226,    33,    34,    32,   218,    33,    34,    33,    34,
     244,    33,    34,   226,    32,   244,    32,   242,    33,    34,
      33,    34,    33,    34,    32,    32,    31,    33,    32,   242,
      32,    32,    32,    32,    32,    32,    32,    32,    32,   162,
      32,    32,    31,    31,    31,    84,    32,    32,    31,   215,
      32,    32,   278,    87,   240,    85,   240,   283,    91,   222,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,   236,    30,   230,   240,
     237,   235,    95,   248,    37,   277,    -1,    -1,    88,    92,
      -1,    -1,    86,    -1,    -1,    -1,    -1,    -1,    -1,    89,
     233,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    94,    93,    90,
      96,    -1,    -1,    -1,    -1,    37
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    29,    39,    41,    43,    35,     0,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    37,    42,    44,    45,    53,    61,    73,
      88,   105,   111,   117,   127,   133,   139,   153,   159,    36,
      31,    47,    48,    49,    50,    51,    63,    64,    65,    66,
      67,   119,   120,   121,   122,   123,    54,   106,   134,   160,
      77,    79,    80,    81,    82,    92,    94,    95,    98,    99,
     128,   112,   140,   154,    46,    62,   118,    55,   107,   135,
     161,    74,    89,   129,   113,   141,   155,    40,    30,   169,
      32,    52,   169,    70,    71,   169,   124,   125,    56,    57,
     169,   108,   109,   169,   136,   137,   169,   162,   163,   169,
     169,    85,    86,   169,   102,   103,   130,   131,   169,   114,
     115,   169,   142,   143,   169,   156,   157,   169,    47,    63,
     119,    56,   108,   136,   162,    77,    92,   130,   114,   142,
     156,    42,    31,   165,   166,   167,   168,    32,    32,    32,
      32,    32,    32,    32,    32,    32,    32,    32,    32,    31,
      33,    72,   165,    70,   126,    50,    58,    59,    60,    66,
     110,   122,   138,    42,   164,   169,    87,   165,    66,    73,
     104,    10,    11,    23,    24,    68,    69,    75,    90,   132,
      66,    73,   116,   122,   127,   144,   145,   146,   158,    31,
      33,    34,    33,    34,    33,    34,   165,    33,    34,    33,
      34,    33,    34,    33,    34,    33,    34,    78,    79,    83,
      84,    93,    96,    97,   100,   101,    76,    91,    70,    33,
      34,    33,    34,    33,    34,   147,   148,    33,    34,    31,
     165,    70,    59,    66,   122,    42,   165,    66,    73,    85,
     169,   102,    78,    93,    68,    75,    90,    66,    73,   122,
     127,    32,   145,   149,   150,   151,    33,    34,    32,   150,
     152,   165,    33,    34,   165
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


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
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
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
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, Location); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (yylocationp);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, yylocationp)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *bottom, yytype_int16 *top)
#else
static void
yy_stack_print (bottom, top)
    yytype_int16 *bottom;
    yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yylsp, yyrule)
    YYSTYPE *yyvsp;
    YYLTYPE *yylsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       , &(yylsp[(yyi + 1) - (yynrhs)])		       );
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, yylsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
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
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, yylocationp)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    YYLTYPE *yylocationp;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;
/* Location data for the look-ahead symbol.  */
YYLTYPE yylloc;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;

  /* The location stack.  */
  YYLTYPE yylsa[YYINITDEPTH];
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;
  /* The locations where the error started and ended.  */
  YYLTYPE yyerror_range[2];

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

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
  yylsp = yyls;
#if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  /* Initialize the default location before parsing starts.  */
  yylloc.first_line   = yylloc.last_line   = 1;
  yylloc.first_column = yylloc.last_column = 0;
#endif

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;
	YYLTYPE *yyls1 = yyls;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp),
		    &yystacksize);
	yyls = yyls1;
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);
	YYSTACK_RELOCATE (yyls);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

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

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
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
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
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

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;
  *++yylsp = yylloc;
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

  /* Default location.  */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 38 "wktparse.y"
    { alloc_lwgeom(srid); }
    break;

  case 4:
#line 40 "wktparse.y"
    { alloc_lwgeom(-1); }
    break;

  case 20:
#line 72 "wktparse.y"
    { set_srid((yyvsp[(3) - (3)].value)); }
    break;

  case 21:
#line 75 "wktparse.y"
    { alloc_wkb((yyvsp[(1) - (1)].wkb)); }
    break;

  case 23:
#line 83 "wktparse.y"
    { set_zm(0, 1); }
    break;

  case 27:
#line 91 "wktparse.y"
    { alloc_point(); }
    break;

  case 28:
#line 91 "wktparse.y"
    { pop(); }
    break;

  case 29:
#line 94 "wktparse.y"
    { alloc_point(); }
    break;

  case 30:
#line 94 "wktparse.y"
    { pop(); }
    break;

  case 32:
#line 102 "wktparse.y"
    { alloc_multipoint(); }
    break;

  case 33:
#line 102 "wktparse.y"
    { pop(); }
    break;

  case 34:
#line 104 "wktparse.y"
    { set_zm(0, 1); alloc_multipoint(); }
    break;

  case 35:
#line 104 "wktparse.y"
    {pop(); }
    break;

  case 37:
#line 109 "wktparse.y"
    { alloc_counter(); }
    break;

  case 38:
#line 109 "wktparse.y"
    { pop(); }
    break;

  case 42:
#line 120 "wktparse.y"
    { alloc_point(); }
    break;

  case 43:
#line 120 "wktparse.y"
    { pop(); }
    break;

  case 45:
#line 128 "wktparse.y"
    { set_zm(0, 1); }
    break;

  case 49:
#line 136 "wktparse.y"
    { alloc_linestring(); }
    break;

  case 50:
#line 136 "wktparse.y"
    { pop(); }
    break;

  case 51:
#line 139 "wktparse.y"
    { alloc_linestring(); }
    break;

  case 52:
#line 139 "wktparse.y"
    { check_linestring(); pop(); }
    break;

  case 53:
#line 142 "wktparse.y"
    { alloc_linestring_closed(); }
    break;

  case 54:
#line 142 "wktparse.y"
    { check_closed_linestring(); pop(); }
    break;

  case 55:
#line 145 "wktparse.y"
    { alloc_counter(); }
    break;

  case 56:
#line 145 "wktparse.y"
    { pop(); }
    break;

  case 60:
#line 157 "wktparse.y"
    {set_zm(0, 1); }
    break;

  case 63:
#line 162 "wktparse.y"
    {set_zm(0, 1); }
    break;

  case 69:
#line 175 "wktparse.y"
    { alloc_circularstring(); }
    break;

  case 70:
#line 175 "wktparse.y"
    { pop(); }
    break;

  case 71:
#line 178 "wktparse.y"
    { alloc_circularstring(); }
    break;

  case 72:
#line 178 "wktparse.y"
    { check_circularstring(); pop(); }
    break;

  case 73:
#line 181 "wktparse.y"
    { alloc_circularstring_closed(); }
    break;

  case 74:
#line 181 "wktparse.y"
    { check_closed_circularstring(); pop(); }
    break;

  case 75:
#line 184 "wktparse.y"
    { alloc_counter(); }
    break;

  case 76:
#line 184 "wktparse.y"
    { pop(); }
    break;

  case 80:
#line 196 "wktparse.y"
    { set_zm(0, 1); }
    break;

  case 83:
#line 201 "wktparse.y"
    { set_zm(0, 1); }
    break;

  case 89:
#line 214 "wktparse.y"
    { alloc_compoundcurve(); }
    break;

  case 90:
#line 214 "wktparse.y"
    { pop(); }
    break;

  case 91:
#line 217 "wktparse.y"
    { alloc_compoundcurve_closed(); }
    break;

  case 92:
#line 217 "wktparse.y"
    { pop(); }
    break;

  case 93:
#line 220 "wktparse.y"
    { alloc_compoundcurve(); }
    break;

  case 94:
#line 220 "wktparse.y"
    {  check_compoundcurve(); pop(); }
    break;

  case 95:
#line 223 "wktparse.y"
    { alloc_compoundcurve_closed(); }
    break;

  case 96:
#line 223 "wktparse.y"
    {  check_closed_compoundcurve(); pop(); }
    break;

  case 97:
#line 226 "wktparse.y"
    { alloc_counter(); }
    break;

  case 98:
#line 226 "wktparse.y"
    {pop();}
    break;

  case 103:
#line 240 "wktparse.y"
    { alloc_multilinestring(); }
    break;

  case 104:
#line 241 "wktparse.y"
    { pop(); }
    break;

  case 105:
#line 243 "wktparse.y"
    { set_zm(0, 1); alloc_multilinestring(); }
    break;

  case 106:
#line 244 "wktparse.y"
    { pop(); }
    break;

  case 108:
#line 249 "wktparse.y"
    { alloc_counter(); }
    break;

  case 109:
#line 249 "wktparse.y"
    { pop();}
    break;

  case 112:
#line 259 "wktparse.y"
    { alloc_multicurve(); }
    break;

  case 113:
#line 260 "wktparse.y"
    { pop(); }
    break;

  case 114:
#line 262 "wktparse.y"
    { set_zm(0, 1); alloc_multicurve(); }
    break;

  case 115:
#line 263 "wktparse.y"
    { pop(); }
    break;

  case 117:
#line 268 "wktparse.y"
    { alloc_counter(); }
    break;

  case 118:
#line 268 "wktparse.y"
    { pop(); }
    break;

  case 124:
#line 284 "wktparse.y"
    { set_zm(0, 1); }
    break;

  case 128:
#line 292 "wktparse.y"
    { alloc_polygon(); }
    break;

  case 129:
#line 292 "wktparse.y"
    { pop(); }
    break;

  case 130:
#line 295 "wktparse.y"
    { alloc_polygon(); }
    break;

  case 131:
#line 295 "wktparse.y"
    { check_polygon(); pop(); }
    break;

  case 132:
#line 298 "wktparse.y"
    { alloc_counter(); }
    break;

  case 133:
#line 298 "wktparse.y"
    { pop();}
    break;

  case 136:
#line 310 "wktparse.y"
    { alloc_curvepolygon(); }
    break;

  case 137:
#line 310 "wktparse.y"
    { check_curvepolygon(); pop(); }
    break;

  case 138:
#line 312 "wktparse.y"
    { set_zm(0, 1); alloc_curvepolygon(); }
    break;

  case 139:
#line 313 "wktparse.y"
    { check_curvepolygon(); pop(); }
    break;

  case 141:
#line 318 "wktparse.y"
    { alloc_counter(); }
    break;

  case 142:
#line 318 "wktparse.y"
    { pop(); }
    break;

  case 149:
#line 336 "wktparse.y"
    { alloc_multipolygon(); }
    break;

  case 150:
#line 336 "wktparse.y"
    { pop(); }
    break;

  case 151:
#line 338 "wktparse.y"
    { set_zm(0, 1); alloc_multipolygon(); }
    break;

  case 152:
#line 339 "wktparse.y"
    { pop();}
    break;

  case 154:
#line 344 "wktparse.y"
    { alloc_counter(); }
    break;

  case 155:
#line 344 "wktparse.y"
    { pop(); }
    break;

  case 158:
#line 354 "wktparse.y"
    {alloc_multisurface(); }
    break;

  case 159:
#line 354 "wktparse.y"
    { pop(); }
    break;

  case 160:
#line 356 "wktparse.y"
    { set_zm(0, 1); alloc_multisurface(); }
    break;

  case 161:
#line 357 "wktparse.y"
    { pop(); }
    break;

  case 163:
#line 362 "wktparse.y"
    { alloc_counter(); }
    break;

  case 164:
#line 362 "wktparse.y"
    { pop(); }
    break;

  case 169:
#line 375 "wktparse.y"
    { alloc_polygon(); }
    break;

  case 170:
#line 375 "wktparse.y"
    { pop(); }
    break;

  case 171:
#line 378 "wktparse.y"
    { alloc_counter(); }
    break;

  case 172:
#line 378 "wktparse.y"
    { check_polyhedralsurface_face(); pop(); }
    break;

  case 175:
#line 386 "wktparse.y"
    { alloc_counter(); }
    break;

  case 176:
#line 386 "wktparse.y"
    { pop(); }
    break;

  case 179:
#line 394 "wktparse.y"
    {alloc_polyhedralsurface(); }
    break;

  case 180:
#line 394 "wktparse.y"
    { pop(); }
    break;

  case 181:
#line 396 "wktparse.y"
    {set_zm(0, 1); alloc_polyhedralsurface(); }
    break;

  case 182:
#line 396 "wktparse.y"
    { pop(); }
    break;

  case 184:
#line 401 "wktparse.y"
    { alloc_counter(); }
    break;

  case 185:
#line 401 "wktparse.y"
    { pop(); }
    break;

  case 188:
#line 412 "wktparse.y"
    { alloc_geomertycollection(); }
    break;

  case 189:
#line 413 "wktparse.y"
    { pop(); }
    break;

  case 190:
#line 415 "wktparse.y"
    { set_zm(0, 1); alloc_geomertycollection(); }
    break;

  case 191:
#line 416 "wktparse.y"
    { pop();}
    break;

  case 193:
#line 421 "wktparse.y"
    { alloc_counter(); }
    break;

  case 194:
#line 421 "wktparse.y"
    { pop(); }
    break;

  case 201:
#line 440 "wktparse.y"
    {alloc_point_2d((yyvsp[(1) - (2)].value),(yyvsp[(2) - (2)].value)); }
    break;

  case 202:
#line 443 "wktparse.y"
    {alloc_point_3d((yyvsp[(1) - (3)].value),(yyvsp[(2) - (3)].value),(yyvsp[(3) - (3)].value)); }
    break;

  case 203:
#line 446 "wktparse.y"
    {alloc_point_4d((yyvsp[(1) - (4)].value),(yyvsp[(2) - (4)].value),(yyvsp[(3) - (4)].value),(yyvsp[(4) - (4)].value)); }
    break;

  case 204:
#line 449 "wktparse.y"
    { alloc_empty(); }
    break;


/* Line 1267 of yacc.c.  */
#line 2279 "y.tab.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;
  *++yylsp = yyloc;

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
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }

  yyerror_range[0] = yylloc;

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval, &yylloc);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  yyerror_range[0] = yylsp[1-yylen];
  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
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

      yyerror_range[0] = *yylsp;
      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, yylsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;

  yyerror_range[1] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the look-ahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, (yyerror_range - 1), 2);
  *++yylsp = yyloc;

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

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
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, &yylloc);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, yylsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


#line 450 "wktparse.y"






