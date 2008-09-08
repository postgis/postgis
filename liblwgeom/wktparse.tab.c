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
} YYLTYPE;
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
#define YYLAST   180

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  36
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  107
/* YYNRULES -- Number of rules.  */
#define YYNRULES  169
/* YYNRULES -- Number of states.  */
#define YYNSTATES  237

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
     163,   164,   167,   168,   171,   172,   177,   179,   183,   184,
     188,   189,   193,   195,   196,   201,   203,   205,   209,   213,
     214,   218,   219,   223,   225,   226,   231,   233,   237,   238,
     242,   243,   247,   249,   250,   255,   257,   259,   263,   267,
     270,   271,   275,   277,   279,   280,   283,   284,   287,   288,
     293,   295,   299,   300,   304,   305,   309,   311,   312,   317,
     319,   321,   325,   329,   330,   334,   335,   339,   341,   342,
     347,   349,   353,   354,   358,   359,   363,   365,   366,   371,
     373,   375,   379,   383,   384,   388,   389,   393,   395,   396,
     401,   403,   405,   409,   411,   413,   415,   418,   422,   427
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
      37,     0,    -1,    -1,    41,    34,    38,    40,    -1,    -1,
      39,    40,    -1,    42,    -1,    43,    -1,    59,    -1,    71,
      -1,   104,    -1,    86,    -1,   114,    -1,    51,    -1,    92,
      -1,    98,    -1,   120,    -1,   126,    -1,   132,    -1,    27,
      33,    29,    -1,    35,    -1,     3,    45,    -1,    -1,    15,
      44,    45,    -1,    46,    -1,    48,    -1,    -1,    47,   142,
      -1,    -1,    49,    50,    -1,    30,   138,    31,    -1,    -1,
       6,    52,    54,    -1,    -1,    18,    53,    54,    -1,   142,
      -1,    -1,    55,    30,    56,    31,    -1,    57,    -1,    56,
      32,    57,    -1,    48,    -1,    -1,    58,   138,    -1,     4,
      61,    -1,    -1,    16,    60,    61,    -1,    62,    -1,    64,
      -1,    -1,    63,   142,    -1,    -1,    65,    68,    -1,    -1,
      67,    68,    -1,    -1,    69,    30,    70,    31,    -1,   138,
      -1,    70,    32,   138,    -1,    10,    75,    -1,    -1,    22,
      72,    75,    -1,    10,    76,    -1,    -1,    22,    74,    76,
      -1,    77,    -1,    79,    -1,    77,    -1,    81,    -1,    -1,
      78,   142,    -1,    -1,    80,    83,    -1,    -1,    82,    83,
      -1,    -1,    84,    30,    85,    31,    -1,   138,    -1,    85,
      32,   138,    -1,    -1,    11,    87,    89,    -1,    -1,    23,
      88,    89,    -1,   142,    -1,    -1,    90,    30,    91,    31,
      -1,    64,    -1,    71,    -1,    91,    32,    64,    -1,    91,
      32,    71,    -1,    -1,     7,    93,    95,    -1,    -1,    19,
      94,    95,    -1,   142,    -1,    -1,    96,    30,    97,    31,
      -1,    64,    -1,    97,    32,    64,    -1,    -1,    13,    99,
     101,    -1,    -1,    25,   100,   101,    -1,   142,    -1,    -1,
     102,    30,   103,    31,    -1,    64,    -1,    71,    -1,   103,
      32,    64,    -1,   103,    32,    71,    -1,     5,   106,    -1,
      -1,    17,   105,   106,    -1,   107,    -1,   109,    -1,    -1,
     108,   142,    -1,    -1,   110,   111,    -1,    -1,   112,    30,
     113,    31,    -1,    68,    -1,   113,    32,    68,    -1,    -1,
      12,   115,   117,    -1,    -1,    24,   116,   117,    -1,   142,
      -1,    -1,   118,    30,   119,    31,    -1,    66,    -1,    73,
      -1,   119,    32,    66,    -1,   119,    32,    73,    -1,    -1,
       8,   121,   123,    -1,    -1,    20,   122,   123,    -1,   142,
      -1,    -1,   124,    30,   125,    31,    -1,   109,    -1,   125,
      32,   109,    -1,    -1,    14,   127,   129,    -1,    -1,    26,
     128,   129,    -1,   142,    -1,    -1,   130,    30,   131,    31,
      -1,   109,    -1,   114,    -1,   131,    32,   109,    -1,   131,
      32,   114,    -1,    -1,     9,   133,   135,    -1,    -1,    21,
     134,   135,    -1,   142,    -1,    -1,   136,    30,   137,    31,
      -1,   142,    -1,    40,    -1,   137,    32,    40,    -1,   139,
      -1,   140,    -1,   141,    -1,    29,    29,    -1,    29,    29,
      29,    -1,    29,    29,    29,    29,    -1,    28,    -1
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
     176,   176,   179,   179,   182,   182,   185,   187,   192,   192,
     194,   194,   197,   199,   199,   202,   204,   206,   208,   213,
     213,   216,   216,   220,   222,   222,   225,   227,   232,   232,
     235,   235,   239,   241,   241,   244,   246,   248,   250,   255,
     257,   257,   260,   262,   265,   265,   268,   268,   271,   271,
     274,   276,   281,   281,   283,   283,   287,   289,   289,   292,
     294,   296,   298,   303,   303,   305,   305,   309,   311,   311,
     314,   316,   321,   321,   323,   323,   327,   329,   329,   332,
     334,   336,   338,   343,   343,   346,   346,   350,   352,   352,
     356,   358,   360,   364,   366,   368,   371,   374,   377,   380
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
  "geom_compoundcurve", "@21", "@22", "compoundcurve", "@23",
  "compoundcurve_int", "geom_multilinestring", "@24", "@25",
  "multilinestring", "@26", "multilinestring_int", "geom_multicurve",
  "@27", "@28", "multicurve", "@29", "multicurve_int", "geom_polygon",
  "@30", "polygon", "empty_polygon", "@31", "nonempty_polygon", "@32",
  "polygon_1", "@33", "polygon_int", "geom_curvepolygon", "@34", "@35",
  "curvepolygon", "@36", "curvepolygon_int", "geom_multipolygon", "@37",
  "@38", "multipolygon", "@39", "multipolygon_int", "geom_multisurface",
  "@40", "@41", "multisurface", "@42", "multisurface_int",
  "geom_geometrycollection", "@43", "@44", "geometrycollection", "@45",
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
      80,    79,    82,    81,    84,    83,    85,    85,    87,    86,
      88,    86,    89,    90,    89,    91,    91,    91,    91,    93,
      92,    94,    92,    95,    96,    95,    97,    97,    99,    98,
     100,    98,   101,   102,   101,   103,   103,   103,   103,   104,
     105,   104,   106,   106,   108,   107,   110,   109,   112,   111,
     113,   113,   115,   114,   116,   114,   117,   118,   117,   119,
     119,   119,   119,   121,   120,   122,   120,   123,   124,   123,
     125,   125,   127,   126,   128,   126,   129,   130,   129,   131,
     131,   131,   131,   133,   132,   134,   132,   135,   136,   135,
     137,   137,   137,   138,   138,   138,   139,   140,   141,   142
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
       0,     2,     0,     2,     0,     4,     1,     3,     0,     3,
       0,     3,     1,     0,     4,     1,     1,     3,     3,     0,
       3,     0,     3,     1,     0,     4,     1,     3,     0,     3,
       0,     3,     1,     0,     4,     1,     1,     3,     3,     2,
       0,     3,     1,     1,     0,     2,     0,     2,     0,     4,
       1,     3,     0,     3,     0,     3,     1,     0,     4,     1,
       1,     3,     3,     0,     3,     0,     3,     1,     0,     4,
       1,     3,     0,     3,     0,     3,     1,     0,     4,     1,
       1,     3,     3,     0,     3,     0,     3,     1,     0,     4,
       1,     1,     3,     1,     1,     1,     2,     3,     4,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       4,     0,     0,     0,     0,     0,     1,    26,    48,   114,
      31,    89,   133,   153,    68,    78,   122,    98,   142,    22,
      44,   110,    33,    91,   135,   155,    59,    80,   124,   100,
     144,    20,     5,     6,     7,    13,     8,     9,    11,    14,
      15,    10,    12,    16,    17,    18,     2,    19,    21,    24,
       0,    25,     0,    43,    46,     0,    47,    54,   109,   112,
       0,   113,   118,    36,    94,   138,   158,    58,    64,     0,
      65,    74,    83,   127,   103,   147,    26,    48,   114,    36,
      94,   138,   158,    68,    83,   127,   103,   147,     0,   169,
      27,     0,    29,    49,    51,     0,   115,   117,     0,    32,
       0,    35,    90,     0,    93,   134,     0,   137,   154,     0,
     157,    69,    71,     0,    79,     0,    82,   123,     0,   126,
      99,     0,   102,   143,     0,   146,    23,    45,   111,    34,
      92,   136,   156,    60,    81,   125,   101,   145,     3,     0,
       0,   163,   164,   165,     0,    54,    28,    50,   116,     0,
       0,    50,    52,    50,   116,   166,    30,     0,    56,   120,
       0,    40,     0,    38,     0,    96,     0,   140,     0,   161,
       0,   160,     0,    76,    85,    86,     0,    68,    62,   129,
      54,   130,     0,   105,   106,     0,   149,   150,     0,   167,
      55,     0,   119,    54,    37,    28,    42,    95,    50,   139,
     116,   159,     0,    75,     0,    84,    50,    61,    66,    67,
      74,    68,    53,   128,    52,   104,    50,   148,   116,   168,
      57,   121,    39,    97,   141,   162,    77,    87,    88,    73,
      63,   131,   132,   107,   108,   151,   152
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,    88,     3,    32,     4,    33,    34,    76,    48,
      49,    50,    51,    52,    92,    35,    63,    79,    99,   100,
     162,   163,   164,    36,    77,    53,    54,    55,    56,    57,
     179,   180,    94,    95,   157,    37,    83,   181,   211,    67,
     207,    68,    69,    70,    71,   209,   210,   112,   113,   172,
      38,    72,    84,   114,   115,   176,    39,    64,    80,   102,
     103,   166,    40,    74,    86,   120,   121,   185,    41,    78,
      58,    59,    60,    61,    62,    97,    98,   160,    42,    73,
      85,   117,   118,   182,    43,    65,    81,   105,   106,   168,
      44,    75,    87,   123,   124,   188,    45,    66,    82,   108,
     109,   170,   140,   141,   142,   143,   101
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -166
static const yytype_int16 yypact[] =
{
     -17,   -14,    21,   145,    -7,     5,  -166,    29,    30,    40,
    -166,  -166,  -166,  -166,    51,  -166,  -166,  -166,  -166,  -166,
    -166,  -166,  -166,  -166,  -166,  -166,  -166,  -166,  -166,  -166,
    -166,  -166,  -166,  -166,  -166,  -166,  -166,  -166,  -166,  -166,
    -166,  -166,  -166,  -166,  -166,  -166,  -166,  -166,  -166,  -166,
      23,  -166,    53,  -166,  -166,    23,  -166,  -166,  -166,  -166,
      23,  -166,  -166,    23,    23,    23,    23,  -166,  -166,    23,
    -166,  -166,    23,    23,    23,    23,    29,    30,    40,    23,
      23,    23,    23,    51,    23,    23,    23,    23,   145,  -166,
    -166,    55,  -166,  -166,  -166,    56,  -166,  -166,    57,  -166,
      58,  -166,  -166,    59,  -166,  -166,    61,  -166,  -166,    62,
    -166,  -166,  -166,    63,  -166,    64,  -166,  -166,    65,  -166,
    -166,    66,  -166,  -166,    67,  -166,  -166,  -166,  -166,  -166,
    -166,  -166,  -166,  -166,  -166,  -166,  -166,  -166,  -166,    70,
      54,  -166,  -166,  -166,    55,  -166,    72,  -166,  -166,   112,
      55,     8,    19,     8,    28,    73,  -166,   -18,  -166,  -166,
      16,  -166,    18,  -166,    55,  -166,    24,  -166,    31,  -166,
      33,  -166,    35,  -166,  -166,  -166,    42,    74,  -166,  -166,
    -166,  -166,    44,  -166,  -166,    46,  -166,  -166,    48,    76,
    -166,    55,  -166,  -166,  -166,    72,  -166,  -166,  -166,  -166,
    -166,  -166,   145,  -166,    55,  -166,     8,  -166,  -166,  -166,
    -166,    74,  -166,  -166,    19,  -166,     8,  -166,    28,  -166,
    -166,  -166,  -166,  -166,  -166,  -166,  -166,  -166,  -166,  -166,
    -166,  -166,  -166,  -166,  -166,  -166,  -166
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -166,  -166,  -166,  -166,   -88,  -166,  -166,  -166,  -166,    27,
    -166,  -166,  -142,  -166,  -166,  -166,  -166,  -166,    32,  -166,
    -166,   -89,  -166,  -166,  -166,    36,  -166,  -166,  -108,  -166,
    -107,  -166,  -136,  -166,  -166,  -148,  -166,  -105,  -166,    60,
    -101,  -165,  -166,  -166,  -166,  -166,  -166,   -98,  -166,  -166,
    -166,  -166,  -166,    88,  -166,  -166,  -166,  -166,  -166,    93,
    -166,  -166,  -166,  -166,  -166,    89,  -166,  -166,  -166,  -166,
      68,  -166,  -166,  -146,  -166,  -166,  -166,  -166,  -147,  -166,
    -166,    91,  -166,  -166,  -166,  -166,  -166,    96,  -166,  -166,
    -166,  -166,  -166,    52,  -166,  -166,  -166,  -166,  -166,    92,
    -166,  -166,  -122,  -166,  -166,  -166,   -49
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -117
static const yytype_int16 yytable[] =
{
     138,    90,   167,   175,   161,   184,    93,   187,   186,   159,
       1,    96,   208,   190,   191,   104,   107,   110,    14,     5,
     111,     6,   158,   116,   119,   122,   125,    46,   173,   177,
      26,   104,   107,   110,    47,   116,   119,   122,   125,   165,
      16,   178,   196,   174,   212,   183,   208,   192,   193,   194,
     195,    89,    28,   161,   224,   197,   198,   221,   228,   -28,
     -50,   169,   199,   200,   201,   202,   203,   204,   234,   220,
    -116,   236,   235,   205,   206,   213,   214,   215,   216,   217,
     218,   -70,   226,    91,   139,   156,   144,   145,   146,   147,
     223,   148,   149,   150,   151,   152,   153,   154,   227,   155,
     171,   -41,   189,   126,   -72,   219,   222,   231,   233,   232,
     230,   129,   229,   127,   225,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,   137,
      89,     0,     0,   133,     0,     0,   128,    31,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,   134,   130,   132,   136,   135,   131,     0,     0,
      31
};

static const yytype_int16 yycheck[] =
{
      88,    50,   148,   151,   146,   153,    55,   154,   154,   145,
      27,    60,   177,    31,    32,    64,    65,    66,    10,    33,
      69,     0,   144,    72,    73,    74,    75,    34,   150,    10,
      22,    80,    81,    82,    29,    84,    85,    86,    87,   147,
      12,    22,   164,   151,   180,   153,   211,    31,    32,    31,
      32,    28,    24,   195,   200,    31,    32,   193,   206,    30,
      30,   149,    31,    32,    31,    32,    31,    32,   216,   191,
      30,   218,   218,    31,    32,    31,    32,    31,    32,    31,
      32,    30,   204,    30,    29,    31,    30,    30,    30,    30,
     198,    30,    30,    30,    30,    30,    30,    30,   206,    29,
     149,    29,    29,    76,    30,    29,   195,   214,   216,   214,
     211,    79,   210,    77,   202,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    87,
      28,    -1,    -1,    83,    -1,    -1,    78,    35,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    84,    80,    82,    86,    85,    81,    -1,    -1,
      35
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    27,    37,    39,    41,    33,     0,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    35,    40,    42,    43,    51,    59,    71,    86,    92,
      98,   104,   114,   120,   126,   132,    34,    29,    45,    46,
      47,    48,    49,    61,    62,    63,    64,    65,   106,   107,
     108,   109,   110,    52,    93,   121,   133,    75,    77,    78,
      79,    80,    87,   115,    99,   127,    44,    60,   105,    53,
      94,   122,   134,    72,    88,   116,   100,   128,    38,    28,
     142,    30,    50,   142,    68,    69,   142,   111,   112,    54,
      55,   142,    95,    96,   142,   123,   124,   142,   135,   136,
     142,   142,    83,    84,    89,    90,   142,   117,   118,   142,
     101,   102,   142,   129,   130,   142,    45,    61,   106,    54,
      95,   123,   135,    75,    89,   117,   101,   129,    40,    29,
     138,   139,   140,   141,    30,    30,    30,    30,    30,    30,
      30,    30,    30,    30,    30,    29,    31,    70,   138,    68,
     113,    48,    56,    57,    58,    64,    97,   109,   125,    40,
     137,   142,    85,   138,    64,    71,    91,    10,    22,    66,
      67,    73,   119,    64,    71,   103,   109,   114,   131,    29,
      31,    32,    31,    32,    31,    32,   138,    31,    32,    31,
      32,    31,    32,    31,    32,    31,    32,    76,    77,    81,
      82,    74,    68,    31,    32,    31,    32,    31,    32,    29,
     138,    68,    57,    64,   109,    40,   138,    64,    71,    83,
      76,    66,    73,    64,    71,   109,   114
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
    { alloc_lwgeom(srid); }
    break;

  case 4:
#line 40 "wktparse.y"
    { alloc_lwgeom(-1); }
    break;

  case 19:
#line 70 "wktparse.y"
    { set_srid((yyvsp[(3) - (3)].value)); }
    break;

  case 20:
#line 73 "wktparse.y"
    { alloc_wkb((yyvsp[(1) - (1)].wkb)); }
    break;

  case 22:
#line 81 "wktparse.y"
    { set_zm(0, 1); }
    break;

  case 26:
#line 89 "wktparse.y"
    { alloc_point(); }
    break;

  case 27:
#line 89 "wktparse.y"
    { pop(); }
    break;

  case 28:
#line 92 "wktparse.y"
    { alloc_point(); }
    break;

  case 29:
#line 92 "wktparse.y"
    { pop(); }
    break;

  case 31:
#line 100 "wktparse.y"
    { alloc_multipoint(); }
    break;

  case 32:
#line 100 "wktparse.y"
    { pop(); }
    break;

  case 33:
#line 102 "wktparse.y"
    { set_zm(0, 1); alloc_multipoint(); }
    break;

  case 34:
#line 102 "wktparse.y"
    {pop(); }
    break;

  case 36:
#line 107 "wktparse.y"
    { alloc_counter(); }
    break;

  case 37:
#line 107 "wktparse.y"
    { pop(); }
    break;

  case 41:
#line 118 "wktparse.y"
    { alloc_point(); }
    break;

  case 42:
#line 118 "wktparse.y"
    { pop(); }
    break;

  case 44:
#line 126 "wktparse.y"
    { set_zm(0, 1); }
    break;

  case 48:
#line 134 "wktparse.y"
    { alloc_linestring(); }
    break;

  case 49:
#line 134 "wktparse.y"
    { pop(); }
    break;

  case 50:
#line 137 "wktparse.y"
    { alloc_linestring(); }
    break;

  case 51:
#line 137 "wktparse.y"
    { pop(); }
    break;

  case 52:
#line 140 "wktparse.y"
    { alloc_linestring_closed(); }
    break;

  case 53:
#line 140 "wktparse.y"
    { pop(); }
    break;

  case 54:
#line 143 "wktparse.y"
    { alloc_counter(); }
    break;

  case 55:
#line 143 "wktparse.y"
    { popc(); }
    break;

  case 59:
#line 155 "wktparse.y"
    {set_zm(0, 1); }
    break;

  case 62:
#line 160 "wktparse.y"
    {set_zm(0, 1); }
    break;

  case 68:
#line 173 "wktparse.y"
    { alloc_circularstring(); }
    break;

  case 69:
#line 173 "wktparse.y"
    { pop(); }
    break;

  case 70:
#line 176 "wktparse.y"
    { alloc_circularstring(); }
    break;

  case 71:
#line 176 "wktparse.y"
    { pop(); }
    break;

  case 72:
#line 179 "wktparse.y"
    { alloc_circularstring_closed(); }
    break;

  case 73:
#line 179 "wktparse.y"
    { pop(); }
    break;

  case 74:
#line 182 "wktparse.y"
    { alloc_counter(); }
    break;

  case 75:
#line 182 "wktparse.y"
    { popc(); }
    break;

  case 78:
#line 192 "wktparse.y"
    { alloc_compoundcurve(); }
    break;

  case 79:
#line 192 "wktparse.y"
    { pop(); }
    break;

  case 80:
#line 194 "wktparse.y"
    {set_zm(0, 1); alloc_compoundcurve(); }
    break;

  case 81:
#line 194 "wktparse.y"
    { pop(); }
    break;

  case 83:
#line 199 "wktparse.y"
    { alloc_counter(); }
    break;

  case 84:
#line 199 "wktparse.y"
    { pop(); }
    break;

  case 89:
#line 213 "wktparse.y"
    { alloc_multilinestring(); }
    break;

  case 90:
#line 214 "wktparse.y"
    { pop(); }
    break;

  case 91:
#line 216 "wktparse.y"
    { set_zm(0, 1); alloc_multilinestring(); }
    break;

  case 92:
#line 217 "wktparse.y"
    { pop(); }
    break;

  case 94:
#line 222 "wktparse.y"
    { alloc_counter(); }
    break;

  case 95:
#line 222 "wktparse.y"
    { pop();}
    break;

  case 98:
#line 232 "wktparse.y"
    { alloc_multicurve(); }
    break;

  case 99:
#line 233 "wktparse.y"
    { pop(); }
    break;

  case 100:
#line 235 "wktparse.y"
    { set_zm(0, 1); alloc_multicurve(); }
    break;

  case 101:
#line 236 "wktparse.y"
    { pop(); }
    break;

  case 103:
#line 241 "wktparse.y"
    { alloc_counter(); }
    break;

  case 104:
#line 241 "wktparse.y"
    { pop(); }
    break;

  case 110:
#line 257 "wktparse.y"
    { set_zm(0, 1); }
    break;

  case 114:
#line 265 "wktparse.y"
    { alloc_polygon(); }
    break;

  case 115:
#line 265 "wktparse.y"
    { pop(); }
    break;

  case 116:
#line 268 "wktparse.y"
    { alloc_polygon(); }
    break;

  case 117:
#line 268 "wktparse.y"
    { pop(); }
    break;

  case 118:
#line 271 "wktparse.y"
    { alloc_counter(); }
    break;

  case 119:
#line 271 "wktparse.y"
    { pop();}
    break;

  case 122:
#line 281 "wktparse.y"
    { alloc_curvepolygon(); }
    break;

  case 123:
#line 281 "wktparse.y"
    { pop(); }
    break;

  case 124:
#line 283 "wktparse.y"
    { set_zm(0, 1); alloc_curvepolygon(); }
    break;

  case 125:
#line 284 "wktparse.y"
    { pop(); }
    break;

  case 127:
#line 289 "wktparse.y"
    { alloc_counter(); }
    break;

  case 128:
#line 289 "wktparse.y"
    { pop(); }
    break;

  case 133:
#line 303 "wktparse.y"
    { alloc_multipolygon(); }
    break;

  case 134:
#line 303 "wktparse.y"
    { pop(); }
    break;

  case 135:
#line 305 "wktparse.y"
    { set_zm(0, 1); alloc_multipolygon(); }
    break;

  case 136:
#line 306 "wktparse.y"
    { pop();}
    break;

  case 138:
#line 311 "wktparse.y"
    { alloc_counter(); }
    break;

  case 139:
#line 311 "wktparse.y"
    { pop(); }
    break;

  case 142:
#line 321 "wktparse.y"
    {alloc_multisurface(); }
    break;

  case 143:
#line 321 "wktparse.y"
    { pop(); }
    break;

  case 144:
#line 323 "wktparse.y"
    { set_zm(0, 1); alloc_multisurface(); }
    break;

  case 145:
#line 324 "wktparse.y"
    { pop(); }
    break;

  case 147:
#line 329 "wktparse.y"
    { alloc_counter(); }
    break;

  case 148:
#line 329 "wktparse.y"
    { pop(); }
    break;

  case 153:
#line 343 "wktparse.y"
    { alloc_geomertycollection(); }
    break;

  case 154:
#line 344 "wktparse.y"
    { pop(); }
    break;

  case 155:
#line 346 "wktparse.y"
    { set_zm(0, 1); alloc_geomertycollection(); }
    break;

  case 156:
#line 347 "wktparse.y"
    { pop();}
    break;

  case 158:
#line 352 "wktparse.y"
    { alloc_counter(); }
    break;

  case 159:
#line 352 "wktparse.y"
    { pop(); }
    break;

  case 166:
#line 371 "wktparse.y"
    {alloc_point_2d((yyvsp[(1) - (2)].value),(yyvsp[(2) - (2)].value)); }
    break;

  case 167:
#line 374 "wktparse.y"
    {alloc_point_3d((yyvsp[(1) - (3)].value),(yyvsp[(2) - (3)].value),(yyvsp[(3) - (3)].value)); }
    break;

  case 168:
#line 377 "wktparse.y"
    {alloc_point_4d((yyvsp[(1) - (4)].value),(yyvsp[(2) - (4)].value),(yyvsp[(3) - (4)].value),(yyvsp[(4) - (4)].value)); }
    break;

  case 169:
#line 380 "wktparse.y"
    { alloc_empty(); }
    break;


/* Line 1267 of yacc.c.  */
#line 2128 "y.tab.c"
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


#line 381 "wktparse.y"






