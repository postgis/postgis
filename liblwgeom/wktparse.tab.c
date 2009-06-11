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
/* Line 187 of yacc.c.  */
#line 188 "y.tab.c"
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
}
YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 213 "y.tab.c"

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
# if YYENABLE_NLS
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
#define YYLAST   203

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  36
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  118
/* YYNRULES -- Number of rules.  */
#define YYNRULES  184
/* YYNRULES -- Number of states.  */
#define YYNSTATES  254

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   290

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
        35
    };

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
    {
        0,     0,     3,     4,     9,    10,    13,    15,    17,    19,
        21,    23,    25,    27,    29,    31,    33,    35,    37,    39,
        43,    45,    48,    49,    53,    55,    57,    58,    61,    62,
        65,    69,    70,    74,    75,    79,    81,    82,    87,    89,
        93,    95,    96,    99,   102,   103,   107,   109,   111,   112,
        115,   116,   119,   120,   123,   124,   129,   131,   135,   138,
        139,   143,   146,   147,   151,   153,   155,   157,   159,   160,
        163,   164,   167,   168,   171,   172,   177,   179,   183,   186,
        187,   191,   194,   195,   199,   201,   203,   205,   207,   208,
        211,   212,   215,   216,   219,   220,   223,   224,   229,   231,
        233,   237,   241,   242,   246,   247,   251,   253,   254,   259,
        261,   265,   266,   270,   271,   275,   277,   278,   283,   285,
        287,   291,   295,   298,   299,   303,   305,   307,   308,   311,
        312,   315,   316,   321,   323,   327,   328,   332,   333,   337,
        339,   340,   345,   347,   349,   351,   355,   359,   363,   364,
        368,   369,   373,   375,   376,   381,   383,   387,   388,   392,
        393,   397,   399,   400,   405,   407,   409,   413,   417,   418,
        422,   423,   427,   429,   430,   435,   437,   439,   443,   445,
        447,   449,   452,   456,   461
    };

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
    {
        37,     0,    -1,    -1,    41,    34,    38,    40,    -1,    -1,
        39,    40,    -1,    42,    -1,    43,    -1,    59,    -1,    71,
        -1,   115,    -1,    86,    -1,   125,    -1,    51,    -1,   103,
        -1,   109,    -1,   131,    -1,   137,    -1,   143,    -1,    27,
        33,    29,    -1,    35,    -1,     3,    45,    -1,    -1,    15,
        44,    45,    -1,    46,    -1,    48,    -1,    -1,    47,   153,
        -1,    -1,    49,    50,    -1,    30,   149,    31,    -1,    -1,
        6,    52,    54,    -1,    -1,    18,    53,    54,    -1,   153,
        -1,    -1,    55,    30,    56,    31,    -1,    57,    -1,    56,
        32,    57,    -1,    48,    -1,    -1,    58,   149,    -1,     4,
        61,    -1,    -1,    16,    60,    61,    -1,    62,    -1,    64,
        -1,    -1,    63,   153,    -1,    -1,    65,    68,    -1,    -1,
        67,    68,    -1,    -1,    69,    30,    70,    31,    -1,   149,
        -1,    70,    32,   149,    -1,    10,    75,    -1,    -1,    22,
        72,    75,    -1,    10,    76,    -1,    -1,    22,    74,    76,
        -1,    77,    -1,    79,    -1,    77,    -1,    81,    -1,    -1,
        78,   153,    -1,    -1,    80,    83,    -1,    -1,    82,    83,
        -1,    -1,    84,    30,    85,    31,    -1,   149,    -1,    85,
        32,   149,    -1,    11,    90,    -1,    -1,    23,    87,    90,
        -1,    11,    91,    -1,    -1,    23,    89,    91,    -1,    92,
        -1,    96,    -1,    94,    -1,    98,    -1,    -1,    93,   153,
        -1,    -1,    95,   153,    -1,    -1,    97,   100,    -1,    -1,
        99,   100,    -1,    -1,   101,    30,   102,    31,    -1,    64,
        -1,    71,    -1,   102,    32,    64,    -1,   102,    32,    71,
        -1,    -1,     7,   104,   106,    -1,    -1,    19,   105,   106,
        -1,   153,    -1,    -1,   107,    30,   108,    31,    -1,    64,
        -1,   108,    32,    64,    -1,    -1,    13,   110,   112,    -1,
        -1,    25,   111,   112,    -1,   153,    -1,    -1,   113,    30,
        114,    31,    -1,    64,    -1,    71,    -1,   114,    32,    64,
        -1,   114,    32,    71,    -1,     5,   117,    -1,    -1,    17,
        116,   117,    -1,   118,    -1,   120,    -1,    -1,   119,   153,
        -1,    -1,   121,   122,    -1,    -1,   123,    30,   124,    31,
        -1,    68,    -1,   124,    32,    68,    -1,    -1,    12,   126,
        128,    -1,    -1,    24,   127,   128,    -1,   153,    -1,    -1,
        129,    30,   130,    31,    -1,    66,    -1,    73,    -1,    88,
        -1,   130,    32,    66,    -1,   130,    32,    73,    -1,   130,
        32,    88,    -1,    -1,     8,   132,   134,    -1,    -1,    20,
        133,   134,    -1,   153,    -1,    -1,   135,    30,   136,    31,
        -1,   120,    -1,   136,    32,   120,    -1,    -1,    14,   138,
        140,    -1,    -1,    26,   139,   140,    -1,   153,    -1,    -1,
        141,    30,   142,    31,    -1,   120,    -1,   125,    -1,   142,
        32,   120,    -1,   142,    32,   125,    -1,    -1,     9,   144,
        146,    -1,    -1,    21,   145,   146,    -1,   153,    -1,    -1,
        147,    30,   148,    31,    -1,   153,    -1,    40,    -1,   148,
        32,    40,    -1,   150,    -1,   151,    -1,   152,    -1,    29,
        29,    -1,    29,    29,    29,    -1,    29,    29,    29,    29,
        -1,    28,    -1
    };

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
    {
        0,    38,    38,    38,    40,    40,    43,    45,    47,    49,
        51,    53,    55,    57,    59,    61,    63,    65,    67,    70,
        73,    79,    81,    81,    84,    86,    89,    89,    92,    92,
        95,   100,   100,   102,   102,   105,   107,   107,   110,   112,
        115,   118,   118,   124,   126,   126,   129,   131,   134,   134,
        137,   137,   140,   140,   143,   143,   146,   148,   153,   155,
        155,   158,   160,   160,   163,   165,   168,   170,   173,   173,
        176,   176,   179,   179,   182,   182,   185,   187,   192,   194,
        194,   197,   199,   199,   202,   204,   207,   209,   212,   212,
        215,   215,   218,   218,   221,   221,   224,   224,   227,   229,
        231,   233,   238,   238,   241,   241,   245,   247,   247,   250,
        252,   257,   257,   260,   260,   264,   266,   266,   269,   271,
        273,   275,   280,   282,   282,   285,   287,   290,   290,   293,
        293,   296,   296,   300,   303,   308,   308,   310,   310,   314,
        316,   316,   319,   321,   323,   325,   327,   329,   334,   334,
        336,   336,   340,   342,   342,   345,   347,   352,   352,   354,
        354,   358,   360,   360,   363,   365,   367,   369,   374,   374,
        377,   377,   381,   383,   383,   387,   389,   391,   395,   397,
        399,   402,   405,   408,   411
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
        "MULTISURFACE", "POINTM", "LINESTRINGM", "POLYGONM", "MULTIPOINTM",
        "MULTILINESTRINGM", "MULTIPOLYGONM", "GEOMETRYCOLLECTIONM",
        "CIRCULARSTRINGM", "COMPOUNDCURVEM", "CURVEPOLYGONM", "MULTICURVEM",
        "MULTISURFACEM", "SRID", "EMPTY", "VALUE", "LPAREN", "RPAREN", "COMMA",
        "EQUALS", "SEMICOLON", "WKB", "$accept", "geometry", "@1", "@2",
        "geometry_int", "srid", "geom_wkb", "geom_point", "@3", "point",
        "empty_point", "@4", "nonempty_point", "@5", "point_int",
        "geom_multipoint", "@6", "@7", "multipoint", "@8", "multipoint_int",
        "mpoint_element", "@9", "geom_linestring", "@10", "linestring",
        "empty_linestring", "@11", "nonempty_linestring", "@12",
        "nonempty_linestring_closed", "@13", "linestring_1", "@14",
        "linestring_int", "geom_circularstring", "@15",
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
        "@44", "@45", "multisurface", "@46", "multisurface_int",
        "geom_geometrycollection", "@47", "@48", "geometrycollection", "@49",
        "geometrycollection_int", "a_point", "point_2d", "point_3d", "point_4d",
        "empty", 0
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
        285,   286,   287,   288,   289,   290
    };
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
    {
        0,    36,    38,    37,    39,    37,    40,    40,    40,    40,
        40,    40,    40,    40,    40,    40,    40,    40,    40,    41,
        42,    43,    44,    43,    45,    45,    47,    46,    49,    48,
        50,    52,    51,    53,    51,    54,    55,    54,    56,    56,
        57,    58,    57,    59,    60,    59,    61,    61,    63,    62,
        65,    64,    67,    66,    69,    68,    70,    70,    71,    72,
        71,    73,    74,    73,    75,    75,    76,    76,    78,    77,
        80,    79,    82,    81,    84,    83,    85,    85,    86,    87,
        86,    88,    89,    88,    90,    90,    91,    91,    93,    92,
        95,    94,    97,    96,    99,    98,   101,   100,   102,   102,
        102,   102,   104,   103,   105,   103,   106,   107,   106,   108,
        108,   110,   109,   111,   109,   112,   113,   112,   114,   114,
        114,   114,   115,   116,   115,   117,   117,   119,   118,   121,
        120,   123,   122,   124,   124,   126,   125,   127,   125,   128,
        129,   128,   130,   130,   130,   130,   130,   130,   132,   131,
        133,   131,   134,   135,   134,   136,   136,   138,   137,   139,
        137,   140,   141,   140,   142,   142,   142,   142,   144,   143,
        145,   143,   146,   147,   146,   148,   148,   148,   149,   149,
        149,   150,   151,   152,   153
    };

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
    {
        0,     2,     0,     4,     0,     2,     1,     1,     1,     1,
        1,     1,     1,     1,     1,     1,     1,     1,     1,     3,
        1,     2,     0,     3,     1,     1,     0,     2,     0,     2,
        3,     0,     3,     0,     3,     1,     0,     4,     1,     3,
        1,     0,     2,     2,     0,     3,     1,     1,     0,     2,
        0,     2,     0,     2,     0,     4,     1,     3,     2,     0,
        3,     2,     0,     3,     1,     1,     1,     1,     0,     2,
        0,     2,     0,     2,     0,     4,     1,     3,     2,     0,
        3,     2,     0,     3,     1,     1,     1,     1,     0,     2,
        0,     2,     0,     2,     0,     2,     0,     4,     1,     1,
        3,     3,     0,     3,     0,     3,     1,     0,     4,     1,
        3,     0,     3,     0,     3,     1,     0,     4,     1,     1,
        3,     3,     2,     0,     3,     1,     1,     0,     2,     0,
        2,     0,     4,     1,     3,     0,     3,     0,     3,     1,
        0,     4,     1,     1,     1,     3,     3,     3,     0,     3,
        0,     3,     1,     0,     4,     1,     3,     0,     3,     0,
        3,     1,     0,     4,     1,     1,     3,     3,     0,     3,
        0,     3,     1,     0,     4,     1,     1,     3,     1,     1,
        1,     2,     3,     4,     1
    };

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
    {
        4,     0,     0,     0,     0,     0,     1,    26,    48,   127,
        31,   102,   148,   168,    68,    88,   135,   111,   157,    22,
        44,   123,    33,   104,   150,   170,    59,    79,   137,   113,
        159,    20,     5,     6,     7,    13,     8,     9,    11,    14,
        15,    10,    12,    16,    17,    18,     2,    19,    21,    24,
        0,    25,     0,    43,    46,     0,    47,    54,   122,   125,
        0,   126,   131,    36,   107,   153,   173,    58,    64,     0,
        65,    74,    78,    84,     0,    85,    96,   140,   116,   162,
        26,    48,   127,    36,   107,   153,   173,    68,    88,   140,
        116,   162,     0,   184,    27,     0,    29,    49,    51,     0,
        128,   130,     0,    32,     0,    35,   103,     0,   106,   149,
        0,   152,   169,     0,   172,    69,    71,     0,    89,    93,
        0,   136,     0,   139,   112,     0,   115,   158,     0,   161,
        23,    45,   124,    34,   105,   151,   171,    60,    80,   138,
        114,   160,     3,     0,     0,   178,   179,   180,     0,    54,
        28,    50,   129,     0,     0,    50,    52,    50,   129,   181,
        30,     0,    56,   133,     0,    40,     0,    38,     0,   109,
        0,   155,     0,   176,     0,   175,     0,    76,    98,    99,
        0,    68,    90,    62,    82,   142,    54,   143,   144,     0,
        118,   119,     0,   164,   165,     0,   182,    55,     0,   132,
        54,    37,    28,    42,   108,    50,   154,   129,   174,     0,
        75,     0,    97,    50,    61,    66,    67,    74,    81,    86,
        0,    87,    96,    68,    90,    53,   141,    52,   117,    50,
        163,   129,   183,    57,   134,    39,   110,   156,   177,    77,
        100,   101,    73,    91,    95,    63,    83,   145,   146,   147,
        120,   121,   166,   167
    };

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
    {
        -1,     2,    92,     3,    32,     4,    33,    34,    80,    48,
        49,    50,    51,    52,    96,    35,    63,    83,   103,   104,
        166,   167,   168,    36,    81,    53,    54,    55,    56,    57,
        185,   186,    98,    99,   161,    37,    87,   187,   223,    67,
        214,    68,    69,    70,    71,   216,   217,   116,   117,   176,
        38,    88,   188,   224,    72,   218,    73,    74,   219,   220,
        75,    76,   221,   222,   119,   120,   180,    39,    64,    84,
        106,   107,   170,    40,    78,    90,   124,   125,   192,    41,
        82,    58,    59,    60,    61,    62,   101,   102,   164,    42,
        77,    89,   121,   122,   189,    43,    65,    85,   109,   110,
        172,    44,    79,    91,   127,   128,   195,    45,    66,    86,
        112,   113,   174,   144,   145,   146,   147,   105
    };

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -176
static const yytype_int16 yypact[] =
    {
        -5,    16,    52,   168,    13,    31,  -176,    33,    35,    37,
        -176,  -176,  -176,  -176,    50,    54,  -176,  -176,  -176,  -176,
        -176,  -176,  -176,  -176,  -176,  -176,  -176,  -176,  -176,  -176,
        -176,  -176,  -176,  -176,  -176,  -176,  -176,  -176,  -176,  -176,
        -176,  -176,  -176,  -176,  -176,  -176,  -176,  -176,  -176,  -176,
        58,  -176,    57,  -176,  -176,    58,  -176,  -176,  -176,  -176,
        58,  -176,  -176,    58,    58,    58,    58,  -176,  -176,    58,
        -176,  -176,  -176,  -176,    58,  -176,  -176,    58,    58,    58,
        33,    35,    37,    58,    58,    58,    58,    50,    54,    58,
        58,    58,   168,  -176,  -176,    59,  -176,  -176,  -176,    60,
        -176,  -176,    61,  -176,    63,  -176,  -176,    64,  -176,  -176,
        65,  -176,  -176,    66,  -176,  -176,  -176,    67,  -176,  -176,
        68,  -176,    69,  -176,  -176,    70,  -176,  -176,    71,  -176,
        -176,  -176,  -176,  -176,  -176,  -176,  -176,  -176,  -176,  -176,
        -176,  -176,  -176,    73,    74,  -176,  -176,  -176,    59,  -176,
        75,  -176,  -176,   116,    59,    11,    15,    11,    18,    77,
        -176,     0,  -176,  -176,    12,  -176,    14,  -176,    59,  -176,
        19,  -176,    24,  -176,    27,  -176,    39,  -176,  -176,  -176,
        41,    78,    79,  -176,  -176,  -176,  -176,  -176,  -176,    43,
        -176,  -176,    47,  -176,  -176,    51,    81,  -176,    59,  -176,
        -176,  -176,    75,  -176,  -176,  -176,  -176,  -176,  -176,   168,
        -176,    59,  -176,    11,  -176,  -176,  -176,  -176,  -176,  -176,
        58,  -176,  -176,    78,    79,  -176,  -176,    15,  -176,    11,
        -176,    18,  -176,  -176,  -176,  -176,  -176,  -176,  -176,  -176,
        -176,  -176,  -176,  -176,  -176,  -176,  -176,  -176,  -176,  -176,
        -176,  -176,  -176,  -176
    };

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
    {
        -176,  -176,  -176,  -176,   -91,  -176,  -176,  -176,  -176,     9,
        -176,  -176,  -138,  -176,  -176,  -176,  -176,  -176,    28,  -176,
        -176,   -95,  -176,  -176,  -176,    32,  -176,  -176,  -144,  -176,
        -115,  -176,  -132,  -176,  -176,  -137,  -176,  -113,  -176,    29,
        -108,  -175,  -176,  -176,  -176,  -176,  -176,  -100,  -176,  -176,
        -176,  -176,   -84,  -176,    62,   -79,  -176,  -176,  -176,  -176,
        -176,  -176,  -176,  -176,   -76,  -176,  -176,  -176,  -176,  -176,
        72,  -176,  -176,  -176,  -176,  -176,    76,  -176,  -176,  -176,
        -176,    80,  -176,  -176,  -150,  -176,  -176,  -176,  -176,  -154,
        -176,  -176,   106,  -176,  -176,  -176,  -176,  -176,    82,  -176,
        -176,  -176,  -176,  -176,    56,  -176,  -176,  -176,  -176,  -176,
        83,  -176,  -176,  -145,  -176,  -176,  -176,   -50
    };

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -130
static const yytype_int16 yytable[] =
    {
        94,   142,   171,   162,   194,    97,   215,   169,   193,   177,
        100,   178,   165,   190,   108,   111,   114,   163,   179,   115,
        191,    14,     1,   203,   118,   181,   182,   123,   126,   129,
        16,   197,   198,    26,   108,   111,   114,   183,   184,   123,
        126,   129,    28,   199,   200,   201,   202,    46,   215,     5,
        204,   205,     6,   233,   225,   206,   207,   237,   208,   209,
        47,   236,   173,   -28,   165,   -50,   239,  -129,   234,   240,
        210,   211,   212,   213,   226,   227,   241,   253,   228,   229,
        -70,   252,   230,   231,   -92,   250,    93,    95,   143,   130,
        148,   149,   251,   150,   151,   152,   153,   154,   155,   156,
        157,   158,   159,   175,   -41,   160,   196,   235,   -72,   -94,
        232,   133,   247,   131,   248,   245,   137,   242,   238,     7,
        8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
        18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
        28,    29,    30,   249,    93,   246,   244,   141,     0,     0,
        138,    31,     0,     0,     0,     0,   134,     0,     0,     0,
        0,     0,   132,     0,     0,     0,   140,   135,     0,   136,
        243,     7,     8,     9,    10,    11,    12,    13,    14,    15,
        16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
        26,    27,    28,    29,    30,   139,     0,     0,     0,     0,
        0,     0,     0,    31
    };

static const yytype_int16 yycheck[] =
    {
        50,    92,   152,   148,   158,    55,   181,   151,   158,   154,
        60,   155,   150,   157,    64,    65,    66,   149,   155,    69,
        157,    10,    27,   168,    74,    10,    11,    77,    78,    79,
        12,    31,    32,    22,    84,    85,    86,    22,    23,    89,
        90,    91,    24,    31,    32,    31,    32,    34,   223,    33,
        31,    32,     0,   198,   186,    31,    32,   207,    31,    32,
        29,   205,   153,    30,   202,    30,   211,    30,   200,   213,
        31,    32,    31,    32,    31,    32,   213,   231,    31,    32,
        30,   231,    31,    32,    30,   229,    28,    30,    29,    80,
        30,    30,   229,    30,    30,    30,    30,    30,    30,    30,
        30,    30,    29,   153,    29,    31,    29,   202,    30,    30,
        29,    83,   227,    81,   227,   223,    87,   217,   209,     3,
        4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
        14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
        24,    25,    26,   227,    28,   224,   222,    91,    -1,    -1,
        88,    35,    -1,    -1,    -1,    -1,    84,    -1,    -1,    -1,
        -1,    -1,    82,    -1,    -1,    -1,    90,    85,    -1,    86,
        220,     3,     4,     5,     6,     7,     8,     9,    10,    11,
        12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
        22,    23,    24,    25,    26,    89,    -1,    -1,    -1,    -1,
        -1,    -1,    -1,    35
    };

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
    {
        0,    27,    37,    39,    41,    33,     0,     3,     4,     5,
        6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
        16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
        26,    35,    40,    42,    43,    51,    59,    71,    86,   103,
        109,   115,   125,   131,   137,   143,    34,    29,    45,    46,
        47,    48,    49,    61,    62,    63,    64,    65,   117,   118,
        119,   120,   121,    52,   104,   132,   144,    75,    77,    78,
        79,    80,    90,    92,    93,    96,    97,   126,   110,   138,
        44,    60,   116,    53,   105,   133,   145,    72,    87,   127,
        111,   139,    38,    28,   153,    30,    50,   153,    68,    69,
        153,   122,   123,    54,    55,   153,   106,   107,   153,   134,
        135,   153,   146,   147,   153,   153,    83,    84,   153,   100,
        101,   128,   129,   153,   112,   113,   153,   140,   141,   153,
        45,    61,   117,    54,   106,   134,   146,    75,    90,   128,
        112,   140,    40,    29,   149,   150,   151,   152,    30,    30,
        30,    30,    30,    30,    30,    30,    30,    30,    30,    29,
        31,    70,   149,    68,   124,    48,    56,    57,    58,    64,
        108,   120,   136,    40,   148,   153,    85,   149,    64,    71,
        102,    10,    11,    22,    23,    66,    67,    73,    88,   130,
        64,    71,   114,   120,   125,   142,    29,    31,    32,    31,
        32,    31,    32,   149,    31,    32,    31,    32,    31,    32,
        31,    32,    31,    32,    76,    77,    81,    82,    91,    94,
        95,    98,    99,    74,    89,    68,    31,    32,    31,    32,
        31,    32,    29,   149,    68,    57,    64,   120,    40,   149,
        64,    71,    83,   153,   100,    76,    91,    66,    73,    88,
        64,    71,   120,   125
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
# if YYLTYPE_IS_TRIVIAL
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
do_not_strip_quotes:
		;
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
#if YYLTYPE_IS_TRIVIAL
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
		{
			alloc_lwgeom(srid);
		}
		break;

	case 4:
#line 40 "wktparse.y"
		{
			alloc_lwgeom(-1);
		}
		break;

	case 19:
#line 70 "wktparse.y"
		{
			set_srid((yyvsp[(3) - (3)].value));
		}
		break;

	case 20:
#line 73 "wktparse.y"
		{
			alloc_wkb((yyvsp[(1) - (1)].wkb));
		}
		break;

	case 22:
#line 81 "wktparse.y"
		{
			set_zm(0, 1);
		}
		break;

	case 26:
#line 89 "wktparse.y"
		{
			alloc_point();
		}
		break;

	case 27:
#line 89 "wktparse.y"
		{
			pop();
		}
		break;

	case 28:
#line 92 "wktparse.y"
		{
			alloc_point();
		}
		break;

	case 29:
#line 92 "wktparse.y"
		{
			pop();
		}
		break;

	case 31:
#line 100 "wktparse.y"
		{
			alloc_multipoint();
		}
		break;

	case 32:
#line 100 "wktparse.y"
		{
			pop();
		}
		break;

	case 33:
#line 102 "wktparse.y"
		{
			set_zm(0, 1);
			alloc_multipoint();
		}
		break;

	case 34:
#line 102 "wktparse.y"
		{
			pop();
		}
		break;

	case 36:
#line 107 "wktparse.y"
		{
			alloc_counter();
		}
		break;

	case 37:
#line 107 "wktparse.y"
		{
			pop();
		}
		break;

	case 41:
#line 118 "wktparse.y"
		{
			alloc_point();
		}
		break;

	case 42:
#line 118 "wktparse.y"
		{
			pop();
		}
		break;

	case 44:
#line 126 "wktparse.y"
		{
			set_zm(0, 1);
		}
		break;

	case 48:
#line 134 "wktparse.y"
		{
			alloc_linestring();
		}
		break;

	case 49:
#line 134 "wktparse.y"
		{
			pop();
		}
		break;

	case 50:
#line 137 "wktparse.y"
		{
			alloc_linestring();
		}
		break;

	case 51:
#line 137 "wktparse.y"
		{
			check_linestring();
			pop();
		}
		break;

	case 52:
#line 140 "wktparse.y"
		{
			alloc_linestring_closed();
		}
		break;

	case 53:
#line 140 "wktparse.y"
		{
			check_closed_linestring();
			pop();
		}
		break;

	case 54:
#line 143 "wktparse.y"
		{
			alloc_counter();
		}
		break;

	case 55:
#line 143 "wktparse.y"
		{
			pop();
		}
		break;

	case 59:
#line 155 "wktparse.y"
		{
			set_zm(0, 1);
		}
		break;

	case 62:
#line 160 "wktparse.y"
		{
			set_zm(0, 1);
		}
		break;

	case 68:
#line 173 "wktparse.y"
		{
			alloc_circularstring();
		}
		break;

	case 69:
#line 173 "wktparse.y"
		{
			pop();
		}
		break;

	case 70:
#line 176 "wktparse.y"
		{
			alloc_circularstring();
		}
		break;

	case 71:
#line 176 "wktparse.y"
		{
			check_circularstring();
			pop();
		}
		break;

	case 72:
#line 179 "wktparse.y"
		{
			alloc_circularstring_closed();
		}
		break;

	case 73:
#line 179 "wktparse.y"
		{
			check_closed_circularstring();
			pop();
		}
		break;

	case 74:
#line 182 "wktparse.y"
		{
			alloc_counter();
		}
		break;

	case 75:
#line 182 "wktparse.y"
		{
			pop();
		}
		break;

	case 79:
#line 194 "wktparse.y"
		{
			set_zm(0, 1);
		}
		break;

	case 82:
#line 199 "wktparse.y"
		{
			set_zm(0, 1);
		}
		break;

	case 88:
#line 212 "wktparse.y"
		{
			alloc_compoundcurve();
		}
		break;

	case 89:
#line 212 "wktparse.y"
		{
			pop();
		}
		break;

	case 90:
#line 215 "wktparse.y"
		{
			alloc_compoundcurve_closed();
		}
		break;

	case 91:
#line 215 "wktparse.y"
		{
			pop();
		}
		break;

	case 92:
#line 218 "wktparse.y"
		{
			alloc_compoundcurve();
		}
		break;

	case 93:
#line 218 "wktparse.y"
		{
			check_compoundcurve();
			pop();
		}
		break;

	case 94:
#line 221 "wktparse.y"
		{
			alloc_compoundcurve_closed();
		}
		break;

	case 95:
#line 221 "wktparse.y"
		{
			check_closed_compoundcurve();
			pop();
		}
		break;

	case 96:
#line 224 "wktparse.y"
		{
			alloc_counter();
		}
		break;

	case 97:
#line 224 "wktparse.y"
		{
			pop();
		}
		break;

	case 102:
#line 238 "wktparse.y"
		{
			alloc_multilinestring();
		}
		break;

	case 103:
#line 239 "wktparse.y"
		{
			pop();
		}
		break;

	case 104:
#line 241 "wktparse.y"
		{
			set_zm(0, 1);
			alloc_multilinestring();
		}
		break;

	case 105:
#line 242 "wktparse.y"
		{
			pop();
		}
		break;

	case 107:
#line 247 "wktparse.y"
		{
			alloc_counter();
		}
		break;

	case 108:
#line 247 "wktparse.y"
		{
			pop();
		}
		break;

	case 111:
#line 257 "wktparse.y"
		{
			alloc_multicurve();
		}
		break;

	case 112:
#line 258 "wktparse.y"
		{
			pop();
		}
		break;

	case 113:
#line 260 "wktparse.y"
		{
			set_zm(0, 1);
			alloc_multicurve();
		}
		break;

	case 114:
#line 261 "wktparse.y"
		{
			pop();
		}
		break;

	case 116:
#line 266 "wktparse.y"
		{
			alloc_counter();
		}
		break;

	case 117:
#line 266 "wktparse.y"
		{
			pop();
		}
		break;

	case 123:
#line 282 "wktparse.y"
		{
			set_zm(0, 1);
		}
		break;

	case 127:
#line 290 "wktparse.y"
		{
			alloc_polygon();
		}
		break;

	case 128:
#line 290 "wktparse.y"
		{
			pop();
		}
		break;

	case 129:
#line 293 "wktparse.y"
		{
			alloc_polygon();
		}
		break;

	case 130:
#line 293 "wktparse.y"
		{
			check_polygon();
			pop();
		}
		break;

	case 131:
#line 296 "wktparse.y"
		{
			alloc_counter();
		}
		break;

	case 132:
#line 296 "wktparse.y"
		{
			pop();
		}
		break;

	case 135:
#line 308 "wktparse.y"
		{
			alloc_curvepolygon();
		}
		break;

	case 136:
#line 308 "wktparse.y"
		{
			check_curvepolygon();
			pop();
		}
		break;

	case 137:
#line 310 "wktparse.y"
		{
			set_zm(0, 1);
			alloc_curvepolygon();
		}
		break;

	case 138:
#line 311 "wktparse.y"
		{
			check_curvepolygon();
			pop();
		}
		break;

	case 140:
#line 316 "wktparse.y"
		{
			alloc_counter();
		}
		break;

	case 141:
#line 316 "wktparse.y"
		{
			pop();
		}
		break;

	case 148:
#line 334 "wktparse.y"
		{
			alloc_multipolygon();
		}
		break;

	case 149:
#line 334 "wktparse.y"
		{
			pop();
		}
		break;

	case 150:
#line 336 "wktparse.y"
		{
			set_zm(0, 1);
			alloc_multipolygon();
		}
		break;

	case 151:
#line 337 "wktparse.y"
		{
			pop();
		}
		break;

	case 153:
#line 342 "wktparse.y"
		{
			alloc_counter();
		}
		break;

	case 154:
#line 342 "wktparse.y"
		{
			pop();
		}
		break;

	case 157:
#line 352 "wktparse.y"
		{
			alloc_multisurface();
		}
		break;

	case 158:
#line 352 "wktparse.y"
		{
			pop();
		}
		break;

	case 159:
#line 354 "wktparse.y"
		{
			set_zm(0, 1);
			alloc_multisurface();
		}
		break;

	case 160:
#line 355 "wktparse.y"
		{
			pop();
		}
		break;

	case 162:
#line 360 "wktparse.y"
		{
			alloc_counter();
		}
		break;

	case 163:
#line 360 "wktparse.y"
		{
			pop();
		}
		break;

	case 168:
#line 374 "wktparse.y"
		{
			alloc_geomertycollection();
		}
		break;

	case 169:
#line 375 "wktparse.y"
		{
			pop();
		}
		break;

	case 170:
#line 377 "wktparse.y"
		{
			set_zm(0, 1);
			alloc_geomertycollection();
		}
		break;

	case 171:
#line 378 "wktparse.y"
		{
			pop();
		}
		break;

	case 173:
#line 383 "wktparse.y"
		{
			alloc_counter();
		}
		break;

	case 174:
#line 383 "wktparse.y"
		{
			pop();
		}
		break;

	case 181:
#line 402 "wktparse.y"
		{
			alloc_point_2d((yyvsp[(1) - (2)].value),(yyvsp[(2) - (2)].value));
		}
		break;

	case 182:
#line 405 "wktparse.y"
		{
			alloc_point_3d((yyvsp[(1) - (3)].value),(yyvsp[(2) - (3)].value),(yyvsp[(3) - (3)].value));
		}
		break;

	case 183:
#line 408 "wktparse.y"
		{
			alloc_point_4d((yyvsp[(1) - (4)].value),(yyvsp[(2) - (4)].value),(yyvsp[(3) - (4)].value),(yyvsp[(4) - (4)].value));
		}
		break;

	case 184:
#line 411 "wktparse.y"
		{
			alloc_empty();
		}
		break;


		/* Line 1267 of yacc.c.  */
#line 2185 "y.tab.c"
	default:
		break;
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


#line 412 "wktparse.y"






