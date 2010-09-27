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
#line 200 "y.tab.c"
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
#line 225 "y.tab.c"

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
#define YYLAST   242

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  42
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  148
/* YYNRULES -- Number of rules.  */
#define YYNRULES  228
/* YYNRULES -- Number of states.  */
#define YYNSTATES  321

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   296

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
      35,    36,    37,    38,    39,    40,    41
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     4,     9,    10,    13,    15,    17,    19,
      21,    23,    25,    27,    29,    31,    33,    35,    37,    39,
      41,    43,    45,    49,    51,    54,    55,    59,    61,    63,
      64,    67,    68,    71,    75,    76,    80,    81,    85,    87,
      88,    93,    95,    99,   101,   102,   105,   108,   109,   113,
     115,   117,   118,   121,   122,   125,   126,   129,   130,   135,
     137,   141,   144,   145,   149,   152,   153,   157,   159,   161,
     163,   165,   166,   169,   170,   173,   174,   177,   178,   183,
     185,   189,   192,   193,   197,   200,   201,   205,   207,   209,
     211,   213,   214,   217,   218,   221,   222,   225,   226,   229,
     230,   235,   237,   239,   243,   247,   248,   252,   253,   257,
     259,   260,   265,   267,   271,   272,   276,   277,   281,   283,
     284,   289,   291,   293,   297,   301,   304,   305,   309,   311,
     313,   314,   317,   318,   321,   322,   327,   329,   333,   336,
     337,   341,   343,   345,   346,   349,   350,   355,   356,   361,
     363,   367,   368,   372,   373,   377,   379,   380,   385,   387,
     389,   391,   395,   399,   403,   404,   408,   409,   413,   415,
     416,   421,   423,   427,   428,   432,   433,   437,   439,   440,
     445,   447,   449,   453,   457,   458,   461,   462,   467,   469,
     473,   474,   479,   481,   485,   486,   490,   491,   495,   497,
     498,   503,   505,   509,   510,   514,   515,   519,   521,   522,
     527,   529,   533,   534,   538,   539,   543,   545,   546,   551,
     553,   555,   559,   561,   563,   565,   568,   572,   577
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
      43,     0,    -1,    -1,    47,    40,    44,    46,    -1,    -1,
      45,    46,    -1,    48,    -1,    49,    -1,    65,    -1,    77,
      -1,   121,    -1,   131,    -1,    92,    -1,   141,    -1,    57,
      -1,   109,    -1,   115,    -1,   147,    -1,   153,    -1,   167,
      -1,   173,    -1,   179,    -1,    33,    39,    35,    -1,    41,
      -1,     3,    51,    -1,    -1,    18,    50,    51,    -1,    52,
      -1,    54,    -1,    -1,    53,   189,    -1,    -1,    55,    56,
      -1,    36,   185,    37,    -1,    -1,     6,    58,    60,    -1,
      -1,    21,    59,    60,    -1,   189,    -1,    -1,    61,    36,
      62,    37,    -1,    63,    -1,    62,    38,    63,    -1,    54,
      -1,    -1,    64,   185,    -1,     4,    67,    -1,    -1,    19,
      66,    67,    -1,    68,    -1,    70,    -1,    -1,    69,   189,
      -1,    -1,    71,    74,    -1,    -1,    73,    74,    -1,    -1,
      75,    36,    76,    37,    -1,   185,    -1,    76,    38,   185,
      -1,    10,    81,    -1,    -1,    25,    78,    81,    -1,    10,
      82,    -1,    -1,    25,    80,    82,    -1,    83,    -1,    85,
      -1,    83,    -1,    87,    -1,    -1,    84,   189,    -1,    -1,
      86,    89,    -1,    -1,    88,    89,    -1,    -1,    90,    36,
      91,    37,    -1,   185,    -1,    91,    38,   185,    -1,    11,
      96,    -1,    -1,    26,    93,    96,    -1,    11,    97,    -1,
      -1,    26,    95,    97,    -1,    98,    -1,   102,    -1,   100,
      -1,   104,    -1,    -1,    99,   189,    -1,    -1,   101,   189,
      -1,    -1,   103,   106,    -1,    -1,   105,   106,    -1,    -1,
     107,    36,   108,    37,    -1,    70,    -1,    77,    -1,   108,
      38,    70,    -1,   108,    38,    77,    -1,    -1,     7,   110,
     112,    -1,    -1,    22,   111,   112,    -1,   189,    -1,    -1,
     113,    36,   114,    37,    -1,    70,    -1,   114,    38,    70,
      -1,    -1,    13,   116,   118,    -1,    -1,    28,   117,   118,
      -1,   189,    -1,    -1,   119,    36,   120,    37,    -1,    70,
      -1,    77,    -1,   120,    38,    70,    -1,   120,    38,    77,
      -1,     5,   123,    -1,    -1,    20,   122,   123,    -1,   124,
      -1,   126,    -1,    -1,   125,   189,    -1,    -1,   127,   128,
      -1,    -1,   129,    36,   130,    37,    -1,    74,    -1,   130,
      38,    74,    -1,    16,   133,    -1,    -1,    31,   132,   133,
      -1,   134,    -1,   136,    -1,    -1,   135,   189,    -1,    -1,
     137,    36,   138,    37,    -1,    -1,   139,    36,   140,    37,
      -1,   185,    -1,   140,    38,   185,    -1,    -1,    12,   142,
     144,    -1,    -1,    27,   143,   144,    -1,   189,    -1,    -1,
     145,    36,   146,    37,    -1,    72,    -1,    79,    -1,    94,
      -1,   146,    38,    72,    -1,   146,    38,    79,    -1,   146,
      38,    94,    -1,    -1,     8,   148,   150,    -1,    -1,    23,
     149,   150,    -1,   189,    -1,    -1,   151,    36,   152,    37,
      -1,   126,    -1,   152,    38,   126,    -1,    -1,    14,   154,
     156,    -1,    -1,    29,   155,   156,    -1,   189,    -1,    -1,
     157,    36,   158,    37,    -1,   126,    -1,   141,    -1,   158,
      38,   126,    -1,   158,    38,   141,    -1,    -1,   160,   161,
      -1,    -1,   162,    36,   163,    37,    -1,   164,    -1,   163,
      38,   164,    -1,    -1,   165,    36,   166,    37,    -1,   185,
      -1,   166,    38,   185,    -1,    -1,    15,   168,   170,    -1,
      -1,    30,   169,   170,    -1,   189,    -1,    -1,   171,    36,
     172,    37,    -1,   159,    -1,   172,    38,   159,    -1,    -1,
      17,   174,   176,    -1,    -1,    32,   175,   176,    -1,   189,
      -1,    -1,   177,    36,   178,    37,    -1,   136,    -1,   178,
      38,   136,    -1,    -1,     9,   180,   182,    -1,    -1,    24,
     181,   182,    -1,   189,    -1,    -1,   183,    36,   184,    37,
      -1,   189,    -1,    46,    -1,   184,    38,    46,    -1,   186,
      -1,   187,    -1,   188,    -1,    35,    35,    -1,    35,    35,
      35,    -1,    35,    35,    35,    35,    -1,    34,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,    38,    38,    38,    40,    40,    43,    45,    47,    49,
      51,    53,    55,    57,    59,    61,    63,    65,    67,    69,
      71,    73,    76,    79,    85,    87,    87,    90,    92,    95,
      95,    98,    98,   101,   106,   106,   108,   108,   111,   113,
     113,   116,   118,   121,   124,   124,   130,   132,   132,   135,
     137,   140,   140,   143,   143,   146,   146,   149,   149,   152,
     154,   159,   161,   161,   164,   166,   166,   169,   171,   174,
     176,   179,   179,   182,   182,   185,   185,   188,   188,   191,
     193,   198,   200,   200,   203,   205,   205,   208,   210,   213,
     215,   218,   218,   221,   221,   224,   224,   227,   227,   230,
     230,   233,   235,   237,   239,   244,   244,   247,   247,   251,
     253,   253,   256,   258,   263,   263,   266,   266,   270,   272,
     272,   275,   277,   279,   281,   286,   288,   288,   291,   293,
     296,   296,   299,   299,   302,   302,   306,   309,   314,   316,
     316,   319,   321,   324,   324,   327,   327,   330,   330,   333,
     335,   342,   342,   344,   344,   348,   350,   350,   353,   355,
     357,   359,   361,   363,   368,   368,   370,   370,   374,   376,
     376,   379,   381,   386,   386,   388,   388,   392,   394,   394,
     397,   399,   401,   403,   407,   407,   410,   410,   413,   415,
     418,   418,   421,   423,   426,   426,   428,   428,   431,   433,
     433,   436,   438,   442,   442,   444,   444,   447,   449,   449,
     452,   454,   460,   460,   463,   463,   467,   469,   469,   473,
     475,   477,   481,   483,   485,   488,   491,   494,   497
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
  "MULTISURFACE", "POLYHEDRALSURFACE", "TRIANGLE", "TIN", "POINTM",
  "LINESTRINGM", "POLYGONM", "MULTIPOINTM", "MULTILINESTRINGM",
  "MULTIPOLYGONM", "GEOMETRYCOLLECTIONM", "CIRCULARSTRINGM",
  "COMPOUNDCURVEM", "CURVEPOLYGONM", "MULTICURVEM", "MULTISURFACEM",
  "POLYHEDRALSURFACEM", "TRIANGLEM", "TINM", "SRID", "EMPTY", "VALUE",
  "LPAREN", "RPAREN", "COMMA", "EQUALS", "SEMICOLON", "WKB", "$accept",
  "geometry", "@1", "@2", "geometry_int", "srid", "geom_wkb", "geom_point",
  "@3", "point", "empty_point", "@4", "nonempty_point", "@5", "point_int",
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
  "polygon_1", "@37", "polygon_int", "geom_triangle", "@38", "triangle",
  "empty_triangle", "@39", "nonempty_triangle", "@40", "triangle_int",
  "@41", "triangle_1", "geom_curvepolygon", "@42", "@43", "curvepolygon",
  "@44", "curvepolygon_int", "geom_multipolygon", "@45", "@46",
  "multipolygon", "@47", "multipolygon_int", "geom_multisurface", "@48",
  "@49", "multisurface", "@50", "multisurface_int", "patch", "@51",
  "patch_1", "@52", "patch_rings", "patch_int", "@53", "patch_int1",
  "geom_polyhedralsurface", "@54", "@55", "polyhedralsurface", "@56",
  "polyhedralsurface_int", "geom_tin", "@57", "@58", "tin", "@59",
  "tin_int", "geom_geometrycollection", "@60", "@61", "geometrycollection",
  "@62", "geometrycollection_int", "a_point", "point_2d", "point_3d",
  "point_4d", "empty", 0
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
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    42,    44,    43,    45,    43,    46,    46,    46,    46,
      46,    46,    46,    46,    46,    46,    46,    46,    46,    46,
      46,    46,    47,    48,    49,    50,    49,    51,    51,    53,
      52,    55,    54,    56,    58,    57,    59,    57,    60,    61,
      60,    62,    62,    63,    64,    63,    65,    66,    65,    67,
      67,    69,    68,    71,    70,    73,    72,    75,    74,    76,
      76,    77,    78,    77,    79,    80,    79,    81,    81,    82,
      82,    84,    83,    86,    85,    88,    87,    90,    89,    91,
      91,    92,    93,    92,    94,    95,    94,    96,    96,    97,
      97,    99,    98,   101,   100,   103,   102,   105,   104,   107,
     106,   108,   108,   108,   108,   110,   109,   111,   109,   112,
     113,   112,   114,   114,   116,   115,   117,   115,   118,   119,
     118,   120,   120,   120,   120,   121,   122,   121,   123,   123,
     125,   124,   127,   126,   129,   128,   130,   130,   131,   132,
     131,   133,   133,   135,   134,   137,   136,   139,   138,   140,
     140,   142,   141,   143,   141,   144,   145,   144,   146,   146,
     146,   146,   146,   146,   148,   147,   149,   147,   150,   151,
     150,   152,   152,   154,   153,   155,   153,   156,   157,   156,
     158,   158,   158,   158,   160,   159,   162,   161,   163,   163,
     165,   164,   166,   166,   168,   167,   169,   167,   170,   171,
     170,   172,   172,   174,   173,   175,   173,   176,   177,   176,
     178,   178,   180,   179,   181,   179,   182,   183,   182,   184,
     184,   184,   185,   185,   185,   186,   187,   188,   189
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     4,     0,     2,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     3,     1,     2,     0,     3,     1,     1,     0,
       2,     0,     2,     3,     0,     3,     0,     3,     1,     0,
       4,     1,     3,     1,     0,     2,     2,     0,     3,     1,
       1,     0,     2,     0,     2,     0,     2,     0,     4,     1,
       3,     2,     0,     3,     2,     0,     3,     1,     1,     1,
       1,     0,     2,     0,     2,     0,     2,     0,     4,     1,
       3,     2,     0,     3,     2,     0,     3,     1,     1,     1,
       1,     0,     2,     0,     2,     0,     2,     0,     2,     0,
       4,     1,     1,     3,     3,     0,     3,     0,     3,     1,
       0,     4,     1,     3,     0,     3,     0,     3,     1,     0,
       4,     1,     1,     3,     3,     2,     0,     3,     1,     1,
       0,     2,     0,     2,     0,     4,     1,     3,     2,     0,
       3,     1,     1,     0,     2,     0,     4,     0,     4,     1,
       3,     0,     3,     0,     3,     1,     0,     4,     1,     1,
       1,     3,     3,     3,     0,     3,     0,     3,     1,     0,
       4,     1,     3,     0,     3,     0,     3,     1,     0,     4,
       1,     1,     3,     3,     0,     2,     0,     4,     1,     3,
       0,     4,     1,     3,     0,     3,     0,     3,     1,     0,
       4,     1,     3,     0,     3,     0,     3,     1,     0,     4,
       1,     3,     0,     3,     0,     3,     1,     0,     4,     1,
       1,     3,     1,     1,     1,     2,     3,     4,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       4,     0,     0,     0,     0,     0,     1,    29,    51,   130,
      34,   105,   164,   212,    71,    91,   151,   114,   173,   194,
     143,   203,    25,    47,   126,    36,   107,   166,   214,    62,
      82,   153,   116,   175,   196,   139,   205,    23,     5,     6,
       7,    14,     8,     9,    12,    15,    16,    10,    11,    13,
      17,    18,    19,    20,    21,     2,    22,    24,    27,     0,
      28,     0,    46,    49,     0,    50,    57,   125,   128,     0,
     129,   134,    39,   110,   169,   217,    61,    67,     0,    68,
      77,    81,    87,     0,    88,    99,   156,   119,   178,   199,
     138,   141,     0,   142,     0,   208,    29,    51,   130,    39,
     110,   169,   217,    71,    91,   156,   119,   178,   199,   143,
     208,     0,   228,    30,     0,    32,    52,    54,     0,   131,
     133,     0,    35,     0,    38,   106,     0,   109,   165,     0,
     168,   213,     0,   216,    72,    74,     0,    92,    96,     0,
     152,     0,   155,   115,     0,   118,   174,     0,   177,   195,
       0,   198,   144,   147,   204,     0,   207,    26,    48,   127,
      37,   108,   167,   215,    63,    83,   154,   117,   176,   197,
     140,   206,     3,     0,     0,   222,   223,   224,     0,    57,
      31,    53,   132,     0,     0,    53,    55,    53,   132,   184,
       0,     0,   145,   225,    33,     0,    59,   136,     0,    43,
       0,    41,     0,   112,     0,   171,     0,   220,     0,   219,
       0,    79,   101,   102,     0,    71,    93,    65,    85,   158,
      57,   159,   160,     0,   121,   122,     0,   180,   181,     0,
     201,   186,     0,   146,     0,   210,     0,   226,    58,     0,
     135,    57,    40,    31,    45,   111,    53,   170,   132,   218,
       0,    78,     0,   100,    53,    64,    69,    70,    77,    84,
      89,     0,    90,    99,    71,    93,    56,   157,    55,   120,
      53,   179,   132,   185,     0,   200,   184,     0,   149,   209,
     145,   227,    60,   137,    42,   113,   172,   221,    80,   103,
     104,    76,    94,    98,    66,    86,   161,   162,   163,   123,
     124,   182,   183,   190,   202,   148,     0,   211,     0,   188,
       0,   150,   187,   190,     0,   189,     0,   192,   191,     0,
     193
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,   111,     3,    38,     4,    39,    40,    96,    57,
      58,    59,    60,    61,   115,    41,    72,    99,   122,   123,
     200,   201,   202,    42,    97,    62,    63,    64,    65,    66,
     219,   220,   117,   118,   195,    43,   103,   221,   264,    76,
     255,    77,    78,    79,    80,   257,   258,   135,   136,   210,
      44,   104,   222,   265,    81,   259,    82,    83,   260,   261,
      84,    85,   262,   263,   138,   139,   214,    45,    73,   100,
     125,   126,   204,    46,    87,   106,   143,   144,   226,    47,
      98,    67,    68,    69,    70,    71,   120,   121,   198,    48,
     109,    90,    91,    92,    93,    94,   190,   191,   277,    49,
      86,   105,   140,   141,   223,    50,    74,   101,   128,   129,
     206,    51,    88,   107,   146,   147,   229,   230,   231,   273,
     274,   308,   309,   310,   316,    52,    89,   108,   149,   150,
     232,    53,    95,   110,   154,   155,   236,    54,    75,   102,
     131,   132,   208,   174,   175,   176,   177,   124
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -210
static const yytype_int16 yypact[] =
{
     -21,   -13,    34,   200,     5,    15,  -210,    18,    20,    22,
    -210,  -210,  -210,  -210,    45,    47,  -210,  -210,  -210,  -210,
      53,  -210,  -210,  -210,  -210,  -210,  -210,  -210,  -210,  -210,
    -210,  -210,  -210,  -210,  -210,  -210,  -210,  -210,  -210,  -210,
    -210,  -210,  -210,  -210,  -210,  -210,  -210,  -210,  -210,  -210,
    -210,  -210,  -210,  -210,  -210,  -210,  -210,  -210,  -210,    36,
    -210,    55,  -210,  -210,    36,  -210,  -210,  -210,  -210,    36,
    -210,  -210,    36,    36,    36,    36,  -210,  -210,    36,  -210,
    -210,  -210,  -210,    36,  -210,  -210,    36,    36,    36,    36,
    -210,  -210,    36,  -210,    65,    36,    18,    20,    22,    36,
      36,    36,    36,    45,    47,    36,    36,    36,    36,    53,
      36,   200,  -210,  -210,    29,  -210,  -210,  -210,    69,  -210,
    -210,    75,  -210,    76,  -210,  -210,    77,  -210,  -210,    78,
    -210,  -210,    79,  -210,  -210,  -210,    80,  -210,  -210,    81,
    -210,    82,  -210,  -210,    83,  -210,  -210,    84,  -210,  -210,
      85,  -210,  -210,  -210,  -210,    86,  -210,  -210,  -210,  -210,
    -210,  -210,  -210,  -210,  -210,  -210,  -210,  -210,  -210,  -210,
    -210,  -210,  -210,    88,    89,  -210,  -210,  -210,    29,  -210,
      90,  -210,  -210,   140,    29,    10,    27,    10,    32,  -210,
      91,    94,  -210,    92,  -210,    -6,  -210,  -210,     2,  -210,
      23,  -210,    29,  -210,    28,  -210,    30,  -210,    35,  -210,
      39,  -210,  -210,  -210,    41,    95,    96,  -210,  -210,  -210,
    -210,  -210,  -210,    48,  -210,  -210,    50,  -210,  -210,    57,
    -210,  -210,    60,  -210,    29,  -210,    62,    98,  -210,    29,
    -210,  -210,  -210,    90,  -210,  -210,  -210,  -210,  -210,  -210,
     200,  -210,    29,  -210,    10,  -210,  -210,  -210,  -210,  -210,
    -210,    36,  -210,  -210,    95,    96,  -210,  -210,    27,  -210,
      10,  -210,    32,  -210,    99,  -210,  -210,    66,  -210,  -210,
    -210,  -210,  -210,  -210,  -210,  -210,  -210,  -210,  -210,  -210,
    -210,  -210,  -210,  -210,  -210,  -210,  -210,  -210,  -210,  -210,
    -210,  -210,  -210,  -210,  -210,  -210,    29,  -210,    70,  -210,
     100,  -210,  -210,  -210,    29,  -210,    72,  -210,  -210,    29,
    -210
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -210,  -210,  -210,  -210,  -109,  -210,  -210,  -210,  -210,    38,
    -210,  -210,  -172,  -210,  -210,  -210,  -210,  -210,    40,  -210,
    -210,  -105,  -210,  -210,  -210,    43,  -210,  -210,  -164,  -210,
     -95,  -210,  -157,  -210,  -210,  -174,  -210,   -93,  -210,    73,
     -87,  -209,  -210,  -210,  -210,  -210,  -210,   -80,  -210,  -210,
    -210,  -210,   -89,  -210,    87,   -85,  -210,  -210,  -210,  -210,
    -210,  -210,  -210,  -210,   -81,  -210,  -210,  -210,  -210,  -210,
      93,  -210,  -210,  -210,  -210,  -210,   127,  -210,  -210,  -210,
    -210,    97,  -210,  -210,  -179,  -210,  -210,  -210,  -210,  -210,
    -210,    74,  -210,  -210,  -188,  -210,  -210,  -210,  -210,  -170,
    -210,  -210,   129,  -210,  -210,  -210,  -210,  -210,   134,  -210,
    -210,  -210,  -210,  -210,   130,  -210,  -210,   -92,  -210,  -210,
    -210,  -210,  -128,  -210,  -210,  -210,  -210,  -210,   128,  -210,
    -210,  -210,  -210,  -210,   132,  -210,  -210,  -210,  -210,  -210,
     136,  -210,  -210,  -177,  -210,  -210,  -210,   -59
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -146
static const yytype_int16 yytable[] =
{
     113,   196,   172,   205,   235,   116,   256,   211,   199,   227,
     119,   213,     1,   225,   127,   130,   133,   203,   228,   134,
      14,   212,   197,   224,   137,   244,     5,   142,   145,   148,
     151,   238,   239,   152,     6,    29,   156,   215,   216,   240,
     241,   127,   130,   133,    16,    55,   142,   145,   148,   151,
      56,   156,   217,   218,   -31,   256,   -53,   278,  -132,    31,
     242,   243,   282,   266,   173,   245,   246,   247,   248,   286,
     112,   199,   249,   250,   207,   288,   251,   252,   253,   254,
     290,   -73,   285,   -95,   283,   267,   268,   269,   270,  -145,
     289,   114,   307,   301,   271,   272,   300,   275,   276,   279,
     280,   153,   302,   305,   306,   178,   299,   312,   313,   318,
     319,   179,   180,   181,   182,   183,   184,   185,   186,   187,
     188,   189,   192,   193,   209,   -44,   194,   237,   233,   311,
     234,   -75,   -97,   281,   157,   303,   314,   317,   284,   160,
     158,   287,   320,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,   296,   112,   297,   164,   294,   291,   298,
     295,    37,   293,   170,   304,   315,     0,     0,     0,     0,
       0,   165,     0,   161,     0,   159,     0,     0,     0,     0,
       0,     0,   292,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,   167,   166,   162,   169,   168,   163,     0,
       0,    37,   171
};

static const yytype_int16 yycheck[] =
{
      59,   178,   111,   182,   192,    64,   215,   184,   180,   188,
      69,   185,    33,   187,    73,    74,    75,   181,   188,    78,
      10,   185,   179,   187,    83,   202,    39,    86,    87,    88,
      89,    37,    38,    92,     0,    25,    95,    10,    11,    37,
      38,   100,   101,   102,    12,    40,   105,   106,   107,   108,
      35,   110,    25,    26,    36,   264,    36,   234,    36,    27,
      37,    38,   239,   220,    35,    37,    38,    37,    38,   248,
      34,   243,    37,    38,   183,   252,    37,    38,    37,    38,
     254,    36,   246,    36,   241,    37,    38,    37,    38,    36,
     254,    36,   280,   272,    37,    38,   270,    37,    38,    37,
      38,    36,   272,    37,    38,    36,   270,    37,    38,    37,
      38,    36,    36,    36,    36,    36,    36,    36,    36,    36,
      36,    36,    36,    35,   183,    35,    37,    35,    37,   306,
      36,    36,    36,    35,    96,    36,    36,   314,   243,    99,
      97,   250,   319,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,   268,    34,   268,   103,   264,   258,   268,
     265,    41,   263,   109,   276,   313,    -1,    -1,    -1,    -1,
      -1,   104,    -1,   100,    -1,    98,    -1,    -1,    -1,    -1,
      -1,    -1,   261,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,   106,   105,   101,   108,   107,   102,    -1,
      -1,    41,   110
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    33,    43,    45,    47,    39,     0,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    41,    46,    48,
      49,    57,    65,    77,    92,   109,   115,   121,   131,   141,
     147,   153,   167,   173,   179,    40,    35,    51,    52,    53,
      54,    55,    67,    68,    69,    70,    71,   123,   124,   125,
     126,   127,    58,   110,   148,   180,    81,    83,    84,    85,
      86,    96,    98,    99,   102,   103,   142,   116,   154,   168,
     133,   134,   135,   136,   137,   174,    50,    66,   122,    59,
     111,   149,   181,    78,    93,   143,   117,   155,   169,   132,
     175,    44,    34,   189,    36,    56,   189,    74,    75,   189,
     128,   129,    60,    61,   189,   112,   113,   189,   150,   151,
     189,   182,   183,   189,   189,    89,    90,   189,   106,   107,
     144,   145,   189,   118,   119,   189,   156,   157,   189,   170,
     171,   189,   189,    36,   176,   177,   189,    51,    67,   123,
      60,   112,   150,   182,    81,    96,   144,   118,   156,   170,
     133,   176,    46,    35,   185,   186,   187,   188,    36,    36,
      36,    36,    36,    36,    36,    36,    36,    36,    36,    36,
     138,   139,    36,    35,    37,    76,   185,    74,   130,    54,
      62,    63,    64,    70,   114,   126,   152,    46,   184,   189,
      91,   185,    70,    77,   108,    10,    11,    25,    26,    72,
      73,    79,    94,   146,    70,    77,   120,   126,   141,   158,
     159,   160,   172,    37,    36,   136,   178,    35,    37,    38,
      37,    38,    37,    38,   185,    37,    38,    37,    38,    37,
      38,    37,    38,    37,    38,    82,    83,    87,    88,    97,
     100,   101,   104,   105,    80,    95,    74,    37,    38,    37,
      38,    37,    38,   161,   162,    37,    38,   140,   185,    37,
      38,    35,   185,    74,    63,    70,   126,    46,   185,    70,
      77,    89,   189,   106,    82,    97,    72,    79,    94,    70,
      77,   126,   141,    36,   159,    37,    38,   136,   163,   164,
     165,   185,    37,    38,    36,   164,   166,   185,    37,    38,
     185
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

  case 22:
#line 76 "wktparse.y"
    { set_srid((yyvsp[(3) - (3)].value)); }
    break;

  case 23:
#line 79 "wktparse.y"
    { alloc_wkb((yyvsp[(1) - (1)].wkb)); }
    break;

  case 25:
#line 87 "wktparse.y"
    { set_zm(0, 1); }
    break;

  case 29:
#line 95 "wktparse.y"
    { alloc_point(); }
    break;

  case 30:
#line 95 "wktparse.y"
    { pop(); }
    break;

  case 31:
#line 98 "wktparse.y"
    { alloc_point(); }
    break;

  case 32:
#line 98 "wktparse.y"
    { pop(); }
    break;

  case 34:
#line 106 "wktparse.y"
    { alloc_multipoint(); }
    break;

  case 35:
#line 106 "wktparse.y"
    { pop(); }
    break;

  case 36:
#line 108 "wktparse.y"
    { set_zm(0, 1); alloc_multipoint(); }
    break;

  case 37:
#line 108 "wktparse.y"
    {pop(); }
    break;

  case 39:
#line 113 "wktparse.y"
    { alloc_counter(); }
    break;

  case 40:
#line 113 "wktparse.y"
    { pop(); }
    break;

  case 44:
#line 124 "wktparse.y"
    { alloc_point(); }
    break;

  case 45:
#line 124 "wktparse.y"
    { pop(); }
    break;

  case 47:
#line 132 "wktparse.y"
    { set_zm(0, 1); }
    break;

  case 51:
#line 140 "wktparse.y"
    { alloc_linestring(); }
    break;

  case 52:
#line 140 "wktparse.y"
    { pop(); }
    break;

  case 53:
#line 143 "wktparse.y"
    { alloc_linestring(); }
    break;

  case 54:
#line 143 "wktparse.y"
    { check_linestring(); pop(); }
    break;

  case 55:
#line 146 "wktparse.y"
    { alloc_linestring_closed(); }
    break;

  case 56:
#line 146 "wktparse.y"
    { check_closed_linestring(); pop(); }
    break;

  case 57:
#line 149 "wktparse.y"
    { alloc_counter(); }
    break;

  case 58:
#line 149 "wktparse.y"
    { pop(); }
    break;

  case 62:
#line 161 "wktparse.y"
    {set_zm(0, 1); }
    break;

  case 65:
#line 166 "wktparse.y"
    {set_zm(0, 1); }
    break;

  case 71:
#line 179 "wktparse.y"
    { alloc_circularstring(); }
    break;

  case 72:
#line 179 "wktparse.y"
    { pop(); }
    break;

  case 73:
#line 182 "wktparse.y"
    { alloc_circularstring(); }
    break;

  case 74:
#line 182 "wktparse.y"
    { check_circularstring(); pop(); }
    break;

  case 75:
#line 185 "wktparse.y"
    { alloc_circularstring_closed(); }
    break;

  case 76:
#line 185 "wktparse.y"
    { check_closed_circularstring(); pop(); }
    break;

  case 77:
#line 188 "wktparse.y"
    { alloc_counter(); }
    break;

  case 78:
#line 188 "wktparse.y"
    { pop(); }
    break;

  case 82:
#line 200 "wktparse.y"
    { set_zm(0, 1); }
    break;

  case 85:
#line 205 "wktparse.y"
    { set_zm(0, 1); }
    break;

  case 91:
#line 218 "wktparse.y"
    { alloc_compoundcurve(); }
    break;

  case 92:
#line 218 "wktparse.y"
    { pop(); }
    break;

  case 93:
#line 221 "wktparse.y"
    { alloc_compoundcurve_closed(); }
    break;

  case 94:
#line 221 "wktparse.y"
    { pop(); }
    break;

  case 95:
#line 224 "wktparse.y"
    { alloc_compoundcurve(); }
    break;

  case 96:
#line 224 "wktparse.y"
    {  check_compoundcurve(); pop(); }
    break;

  case 97:
#line 227 "wktparse.y"
    { alloc_compoundcurve_closed(); }
    break;

  case 98:
#line 227 "wktparse.y"
    {  check_closed_compoundcurve(); pop(); }
    break;

  case 99:
#line 230 "wktparse.y"
    { alloc_counter(); }
    break;

  case 100:
#line 230 "wktparse.y"
    {pop();}
    break;

  case 105:
#line 244 "wktparse.y"
    { alloc_multilinestring(); }
    break;

  case 106:
#line 245 "wktparse.y"
    { pop(); }
    break;

  case 107:
#line 247 "wktparse.y"
    { set_zm(0, 1); alloc_multilinestring(); }
    break;

  case 108:
#line 248 "wktparse.y"
    { pop(); }
    break;

  case 110:
#line 253 "wktparse.y"
    { alloc_counter(); }
    break;

  case 111:
#line 253 "wktparse.y"
    { pop();}
    break;

  case 114:
#line 263 "wktparse.y"
    { alloc_multicurve(); }
    break;

  case 115:
#line 264 "wktparse.y"
    { pop(); }
    break;

  case 116:
#line 266 "wktparse.y"
    { set_zm(0, 1); alloc_multicurve(); }
    break;

  case 117:
#line 267 "wktparse.y"
    { pop(); }
    break;

  case 119:
#line 272 "wktparse.y"
    { alloc_counter(); }
    break;

  case 120:
#line 272 "wktparse.y"
    { pop(); }
    break;

  case 126:
#line 288 "wktparse.y"
    { set_zm(0, 1); }
    break;

  case 130:
#line 296 "wktparse.y"
    { alloc_polygon(); }
    break;

  case 131:
#line 296 "wktparse.y"
    { pop(); }
    break;

  case 132:
#line 299 "wktparse.y"
    { alloc_polygon(); }
    break;

  case 133:
#line 299 "wktparse.y"
    { check_polygon(); pop(); }
    break;

  case 134:
#line 302 "wktparse.y"
    { alloc_counter(); }
    break;

  case 135:
#line 302 "wktparse.y"
    { pop();}
    break;

  case 139:
#line 316 "wktparse.y"
    { set_zm(0, 1); }
    break;

  case 143:
#line 324 "wktparse.y"
    { alloc_triangle(); }
    break;

  case 144:
#line 324 "wktparse.y"
    { pop(); }
    break;

  case 145:
#line 327 "wktparse.y"
    { alloc_triangle(); }
    break;

  case 146:
#line 327 "wktparse.y"
    { check_triangle(); pop(); }
    break;

  case 147:
#line 330 "wktparse.y"
    { alloc_counter(); }
    break;

  case 148:
#line 330 "wktparse.y"
    { pop(); }
    break;

  case 151:
#line 342 "wktparse.y"
    { alloc_curvepolygon(); }
    break;

  case 152:
#line 342 "wktparse.y"
    { check_curvepolygon(); pop(); }
    break;

  case 153:
#line 344 "wktparse.y"
    { set_zm(0, 1); alloc_curvepolygon(); }
    break;

  case 154:
#line 345 "wktparse.y"
    { check_curvepolygon(); pop(); }
    break;

  case 156:
#line 350 "wktparse.y"
    { alloc_counter(); }
    break;

  case 157:
#line 350 "wktparse.y"
    { pop(); }
    break;

  case 164:
#line 368 "wktparse.y"
    { alloc_multipolygon(); }
    break;

  case 165:
#line 368 "wktparse.y"
    { pop(); }
    break;

  case 166:
#line 370 "wktparse.y"
    { set_zm(0, 1); alloc_multipolygon(); }
    break;

  case 167:
#line 371 "wktparse.y"
    { pop();}
    break;

  case 169:
#line 376 "wktparse.y"
    { alloc_counter(); }
    break;

  case 170:
#line 376 "wktparse.y"
    { pop(); }
    break;

  case 173:
#line 386 "wktparse.y"
    {alloc_multisurface(); }
    break;

  case 174:
#line 386 "wktparse.y"
    { pop(); }
    break;

  case 175:
#line 388 "wktparse.y"
    { set_zm(0, 1); alloc_multisurface(); }
    break;

  case 176:
#line 389 "wktparse.y"
    { pop(); }
    break;

  case 178:
#line 394 "wktparse.y"
    { alloc_counter(); }
    break;

  case 179:
#line 394 "wktparse.y"
    { pop(); }
    break;

  case 184:
#line 407 "wktparse.y"
    { alloc_polygon(); }
    break;

  case 185:
#line 407 "wktparse.y"
    { pop(); }
    break;

  case 186:
#line 410 "wktparse.y"
    { alloc_counter(); }
    break;

  case 187:
#line 410 "wktparse.y"
    { check_polyhedralsurface_patch(); pop(); }
    break;

  case 190:
#line 418 "wktparse.y"
    { alloc_counter(); }
    break;

  case 191:
#line 418 "wktparse.y"
    { pop(); }
    break;

  case 194:
#line 426 "wktparse.y"
    {alloc_polyhedralsurface(); }
    break;

  case 195:
#line 426 "wktparse.y"
    { pop(); }
    break;

  case 196:
#line 428 "wktparse.y"
    {set_zm(0, 1); alloc_polyhedralsurface(); }
    break;

  case 197:
#line 428 "wktparse.y"
    { pop(); }
    break;

  case 199:
#line 433 "wktparse.y"
    { alloc_counter(); }
    break;

  case 200:
#line 433 "wktparse.y"
    { pop(); }
    break;

  case 203:
#line 442 "wktparse.y"
    {alloc_tin(); }
    break;

  case 204:
#line 442 "wktparse.y"
    { pop(); }
    break;

  case 205:
#line 444 "wktparse.y"
    {set_zm(0, 1); alloc_tin(); }
    break;

  case 206:
#line 444 "wktparse.y"
    { pop(); }
    break;

  case 208:
#line 449 "wktparse.y"
    { alloc_counter(); }
    break;

  case 209:
#line 449 "wktparse.y"
    { pop(); }
    break;

  case 212:
#line 460 "wktparse.y"
    { alloc_geomertycollection(); }
    break;

  case 213:
#line 461 "wktparse.y"
    { pop(); }
    break;

  case 214:
#line 463 "wktparse.y"
    { set_zm(0, 1); alloc_geomertycollection(); }
    break;

  case 215:
#line 464 "wktparse.y"
    { pop();}
    break;

  case 217:
#line 469 "wktparse.y"
    { alloc_counter(); }
    break;

  case 218:
#line 469 "wktparse.y"
    { pop(); }
    break;

  case 225:
#line 488 "wktparse.y"
    {alloc_point_2d((yyvsp[(1) - (2)].value),(yyvsp[(2) - (2)].value)); }
    break;

  case 226:
#line 491 "wktparse.y"
    {alloc_point_3d((yyvsp[(1) - (3)].value),(yyvsp[(2) - (3)].value),(yyvsp[(3) - (3)].value)); }
    break;

  case 227:
#line 494 "wktparse.y"
    {alloc_point_4d((yyvsp[(1) - (4)].value),(yyvsp[(2) - (4)].value),(yyvsp[(3) - (4)].value),(yyvsp[(4) - (4)].value)); }
    break;

  case 228:
#line 497 "wktparse.y"
    { alloc_empty(); }
    break;


/* Line 1267 of yacc.c.  */
#line 2391 "y.tab.c"
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


#line 498 "wktparse.y"






