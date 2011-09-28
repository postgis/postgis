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
#define yyparse wkt_yyparse
#define yylex   wkt_yylex
#define yyerror wkt_yyerror
#define yylval  wkt_yylval
#define yychar  wkt_yychar
#define yydebug wkt_yydebug
#define yynerrs wkt_yynerrs
#define yylloc wkt_yylloc

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     POINT_TOK = 258,
     LINESTRING_TOK = 259,
     POLYGON_TOK = 260,
     MPOINT_TOK = 261,
     MLINESTRING_TOK = 262,
     MPOLYGON_TOK = 263,
     MSURFACE_TOK = 264,
     MCURVE_TOK = 265,
     CURVEPOLYGON_TOK = 266,
     COMPOUNDCURVE_TOK = 267,
     CIRCULARSTRING_TOK = 268,
     COLLECTION_TOK = 269,
     RBRACKET_TOK = 270,
     LBRACKET_TOK = 271,
     COMMA_TOK = 272,
     EMPTY_TOK = 273,
     SEMICOLON_TOK = 274,
     TRIANGLE_TOK = 275,
     TIN_TOK = 276,
     POLYHEDRALSURFACE_TOK = 277,
     DOUBLE_TOK = 278,
     DIMENSIONALITY_TOK = 279,
     SRID_TOK = 280
   };
#endif
/* Tokens.  */
#define POINT_TOK 258
#define LINESTRING_TOK 259
#define POLYGON_TOK 260
#define MPOINT_TOK 261
#define MLINESTRING_TOK 262
#define MPOLYGON_TOK 263
#define MSURFACE_TOK 264
#define MCURVE_TOK 265
#define CURVEPOLYGON_TOK 266
#define COMPOUNDCURVE_TOK 267
#define CIRCULARSTRING_TOK 268
#define COLLECTION_TOK 269
#define RBRACKET_TOK 270
#define LBRACKET_TOK 271
#define COMMA_TOK 272
#define EMPTY_TOK 273
#define SEMICOLON_TOK 274
#define TRIANGLE_TOK 275
#define TIN_TOK 276
#define POLYHEDRALSURFACE_TOK 277
#define DOUBLE_TOK 278
#define DIMENSIONALITY_TOK 279
#define SRID_TOK 280




/* Copy the first part of user declarations.  */
#line 1 "lwin_wkt_parse.y"


/* WKT Parser */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lwin_wkt.h"
#include "lwin_wkt_parse.h"
#include "lwgeom_log.h"


/* Prototypes to quiet the compiler */
int wkt_yyparse(void);
void wkt_yyerror(const char *str);
int wkt_yylex(void);


/* Declare the global parser variable */
LWGEOM_PARSER_RESULT global_parser_result;

/* Turn on/off verbose parsing (turn off for production) */
int wkt_yydebug = 0;

/* 
* Error handler called by the bison parser. Mostly we will be 
* catching our own errors and filling out the message and errlocation
* from WKT_ERROR in the grammar, but we keep this one 
* around just in case.
*/
void wkt_yyerror(const char *str)
{
	/* If we haven't already set a message and location, let's set one now. */
	if ( ! global_parser_result.message ) 
	{
		global_parser_result.message = parser_error_messages[PARSER_ERROR_OTHER];
		global_parser_result.errcode = PARSER_ERROR_OTHER;
		global_parser_result.errlocation = wkt_yylloc.last_column;
	}
	LWDEBUGF(4,"%s", str);
}

/**
* Parse a WKT geometry string into an LWGEOM structure. Note that this
* process uses globals and is not re-entrant, so don't call it within itself
* (eg, from within other functions in lwin_wkt.c) or from a threaded program.
* Note that parser_result.wkinput picks up a reference to wktstr.
*/
int lwgeom_parse_wkt(LWGEOM_PARSER_RESULT *parser_result, char *wktstr, int parser_check_flags)
{
	int parse_rv = 0;

	/* Clean up our global parser result. */
	global_parser_result.geom = NULL;
	global_parser_result.message = NULL;
	global_parser_result.serialized_lwgeom = NULL;
	global_parser_result.errcode = 0;
	global_parser_result.errlocation = 0;
	global_parser_result.size = 0;

	/* Set the input text string, and parse checks. */
	global_parser_result.wkinput = wktstr;
	global_parser_result.parser_check_flags = parser_check_flags;
		
	wkt_lexer_init(wktstr); /* Lexer ready */
	parse_rv = wkt_yyparse(); /* Run the parse */
	LWDEBUGF(4,"wkt_yyparse returned %d", parse_rv);
	wkt_lexer_close(); /* Clean up lexer */
	
	/* A non-zero parser return is an error. */
	if ( parse_rv != 0 ) 
	{
		if( ! global_parser_result.errcode )
		{
			global_parser_result.errcode = PARSER_ERROR_OTHER;
			global_parser_result.message = parser_error_messages[PARSER_ERROR_OTHER];
			global_parser_result.errlocation = wkt_yylloc.last_column;
		}

		LWDEBUGF(5, "error returned by wkt_yyparse() @ %d: [%d] '%s'", 
		            global_parser_result.errlocation, 
		            global_parser_result.errcode, 
		            global_parser_result.message);
		
		/* Copy the global values into the return pointer */
		*parser_result = global_parser_result;
		return LW_FAILURE;
	}
	
	/* Copy the global value into the return pointer */
	*parser_result = global_parser_result;
	return LW_SUCCESS;
}

#define WKT_ERROR() { if ( global_parser_result.errcode != 0 ) { YYERROR; } }




/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 102 "lwin_wkt_parse.y"
{
	int integervalue;
	double doublevalue;
	char *stringvalue;
	LWGEOM *geometryvalue;
	POINT coordinatevalue;
	POINTARRAY *ptarrayvalue;
}
/* Line 193 of yacc.c.  */
#line 260 "lwin_wkt_parse.c"
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
#line 285 "lwin_wkt_parse.c"

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
#define YYFINAL  80
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   288

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  26
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  40
/* YYNRULES -- Number of rules.  */
#define YYNRULES  133
/* YYNRULES -- Number of states.  */
#define YYNSTATES  261

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   280

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
      25
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     9,    11,    13,    15,    17,    19,
      21,    23,    25,    27,    29,    31,    33,    35,    37,    39,
      44,    50,    54,    57,    61,    63,    68,    74,    78,    81,
      85,    89,    93,    95,    97,    99,   104,   110,   114,   117,
     122,   128,   132,   135,   140,   146,   150,   153,   157,   159,
     163,   165,   170,   176,   180,   183,   187,   191,   196,   202,
     206,   209,   213,   215,   217,   219,   221,   223,   227,   229,
     233,   235,   239,   243,   248,   254,   258,   261,   265,   269,
     273,   275,   277,   279,   284,   290,   294,   297,   301,   305,
     309,   313,   315,   317,   319,   321,   326,   332,   336,   339,
     343,   345,   350,   356,   360,   363,   368,   374,   378,   381,
     385,   389,   391,   398,   406,   410,   413,   419,   424,   430,
     434,   437,   441,   443,   445,   449,   454,   460,   464,   467,
     471,   473,   476,   480
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      27,     0,    -1,    28,    -1,    25,    19,    28,    -1,    63,
      -1,    55,    -1,    54,    -1,    48,    -1,    38,    -1,    41,
      -1,    60,    -1,    52,    -1,    35,    -1,    31,    -1,    50,
      -1,    33,    -1,    34,    -1,    58,    -1,    29,    -1,    14,
      16,    30,    15,    -1,    14,    24,    16,    30,    15,    -1,
      14,    24,    18,    -1,    14,    18,    -1,    30,    17,    28,
      -1,    28,    -1,     9,    16,    32,    15,    -1,     9,    24,
      16,    32,    15,    -1,     9,    24,    18,    -1,     9,    18,
      -1,    32,    17,    38,    -1,    32,    17,    41,    -1,    32,
      17,    39,    -1,    38,    -1,    41,    -1,    39,    -1,    21,
      16,    57,    15,    -1,    21,    24,    16,    57,    15,    -1,
      21,    24,    18,    -1,    21,    18,    -1,    22,    16,    37,
      15,    -1,    22,    24,    16,    37,    15,    -1,    22,    24,
      18,    -1,    22,    18,    -1,     8,    16,    36,    15,    -1,
       8,    24,    16,    36,    15,    -1,     8,    24,    18,    -1,
       8,    18,    -1,    36,    17,    39,    -1,    39,    -1,    37,
      17,    40,    -1,    40,    -1,     5,    16,    45,    15,    -1,
       5,    24,    16,    45,    15,    -1,     5,    24,    18,    -1,
       5,    18,    -1,    16,    45,    15,    -1,    16,    44,    15,
      -1,    11,    16,    42,    15,    -1,    11,    24,    16,    42,
      15,    -1,    11,    24,    18,    -1,    11,    18,    -1,    42,
      17,    43,    -1,    43,    -1,    56,    -1,    55,    -1,    48,
      -1,    54,    -1,    44,    17,    46,    -1,    46,    -1,    45,
      17,    47,    -1,    47,    -1,    16,    64,    15,    -1,    16,
      64,    15,    -1,    12,    16,    49,    15,    -1,    12,    24,
      16,    49,    15,    -1,    12,    24,    18,    -1,    12,    18,
      -1,    49,    17,    54,    -1,    49,    17,    55,    -1,    49,
      17,    56,    -1,    54,    -1,    55,    -1,    56,    -1,    10,
      16,    51,    15,    -1,    10,    24,    16,    51,    15,    -1,
      10,    24,    18,    -1,    10,    18,    -1,    51,    17,    54,
      -1,    51,    17,    48,    -1,    51,    17,    55,    -1,    51,
      17,    56,    -1,    54,    -1,    48,    -1,    55,    -1,    56,
      -1,     7,    16,    53,    15,    -1,     7,    24,    16,    53,
      15,    -1,     7,    24,    18,    -1,     7,    18,    -1,    53,
      17,    56,    -1,    56,    -1,    13,    16,    64,    15,    -1,
      13,    24,    16,    64,    15,    -1,    13,    24,    18,    -1,
      13,    18,    -1,     4,    16,    64,    15,    -1,     4,    24,
      16,    64,    15,    -1,     4,    24,    18,    -1,     4,    18,
      -1,    16,    64,    15,    -1,    57,    17,    59,    -1,    59,
      -1,    20,    16,    16,    64,    15,    15,    -1,    20,    24,
      16,    16,    64,    15,    15,    -1,    20,    24,    18,    -1,
      20,    18,    -1,    16,    16,    64,    15,    15,    -1,     6,
      16,    61,    15,    -1,     6,    24,    16,    61,    15,    -1,
       6,    24,    18,    -1,     6,    18,    -1,    61,    17,    62,
      -1,    62,    -1,    65,    -1,    16,    65,    15,    -1,     3,
      16,    64,    15,    -1,     3,    24,    16,    64,    15,    -1,
       3,    24,    18,    -1,     3,    18,    -1,    64,    17,    65,
      -1,    65,    -1,    23,    23,    -1,    23,    23,    23,    -1,
      23,    23,    23,    23,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   206,   206,   208,   212,   213,   214,   215,   216,   217,
     218,   219,   220,   221,   222,   223,   224,   225,   226,   229,
     231,   233,   235,   239,   241,   245,   247,   249,   251,   255,
     257,   259,   261,   263,   265,   269,   271,   273,   275,   279,
     281,   283,   285,   289,   291,   293,   295,   299,   301,   305,
     307,   311,   313,   315,   317,   321,   324,   327,   329,   331,
     333,   337,   339,   343,   344,   345,   346,   349,   351,   355,
     357,   361,   364,   367,   369,   371,   373,   377,   379,   381,
     383,   385,   387,   391,   393,   395,   397,   401,   403,   405,
     407,   409,   411,   413,   415,   419,   421,   423,   425,   429,
     431,   435,   437,   439,   441,   445,   447,   449,   451,   455,
     459,   461,   465,   467,   469,   471,   475,   479,   481,   483,
     485,   489,   491,   495,   497,   501,   503,   505,   507,   511,
     513,   517,   519,   521
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "POINT_TOK", "LINESTRING_TOK",
  "POLYGON_TOK", "MPOINT_TOK", "MLINESTRING_TOK", "MPOLYGON_TOK",
  "MSURFACE_TOK", "MCURVE_TOK", "CURVEPOLYGON_TOK", "COMPOUNDCURVE_TOK",
  "CIRCULARSTRING_TOK", "COLLECTION_TOK", "RBRACKET_TOK", "LBRACKET_TOK",
  "COMMA_TOK", "EMPTY_TOK", "SEMICOLON_TOK", "TRIANGLE_TOK", "TIN_TOK",
  "POLYHEDRALSURFACE_TOK", "DOUBLE_TOK", "DIMENSIONALITY_TOK", "SRID_TOK",
  "$accept", "geometry", "geometry_no_srid", "geometrycollection",
  "geometry_list", "multisurface", "surface_list", "tin",
  "polyhedralsurface", "multipolygon", "polygon_list", "patch_list",
  "polygon", "polygon_untagged", "patch", "curvepolygon", "curvering_list",
  "curvering", "patchring_list", "ring_list", "patchring", "ring",
  "compoundcurve", "compound_list", "multicurve", "curve_list",
  "multilinestring", "linestring_list", "circularstring", "linestring",
  "linestring_untagged", "triangle_list", "triangle", "triangle_untagged",
  "multipoint", "point_list", "point_untagged", "point", "ptarray",
  "coordinate", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    26,    27,    27,    28,    28,    28,    28,    28,    28,
      28,    28,    28,    28,    28,    28,    28,    28,    28,    29,
      29,    29,    29,    30,    30,    31,    31,    31,    31,    32,
      32,    32,    32,    32,    32,    33,    33,    33,    33,    34,
      34,    34,    34,    35,    35,    35,    35,    36,    36,    37,
      37,    38,    38,    38,    38,    39,    40,    41,    41,    41,
      41,    42,    42,    43,    43,    43,    43,    44,    44,    45,
      45,    46,    47,    48,    48,    48,    48,    49,    49,    49,
      49,    49,    49,    50,    50,    50,    50,    51,    51,    51,
      51,    51,    51,    51,    51,    52,    52,    52,    52,    53,
      53,    54,    54,    54,    54,    55,    55,    55,    55,    56,
      57,    57,    58,    58,    58,    58,    59,    60,    60,    60,
      60,    61,    61,    62,    62,    63,    63,    63,    63,    64,
      64,    65,    65,    65
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     3,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     4,
       5,     3,     2,     3,     1,     4,     5,     3,     2,     3,
       3,     3,     1,     1,     1,     4,     5,     3,     2,     4,
       5,     3,     2,     4,     5,     3,     2,     3,     1,     3,
       1,     4,     5,     3,     2,     3,     3,     4,     5,     3,
       2,     3,     1,     1,     1,     1,     1,     3,     1,     3,
       1,     3,     3,     4,     5,     3,     2,     3,     3,     3,
       1,     1,     1,     4,     5,     3,     2,     3,     3,     3,
       3,     1,     1,     1,     1,     4,     5,     3,     2,     3,
       1,     4,     5,     3,     2,     4,     5,     3,     2,     3,
       3,     1,     6,     7,     3,     2,     5,     4,     5,     3,
       2,     3,     1,     1,     3,     4,     5,     3,     2,     3,
       1,     2,     3,     4
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     2,    18,
      13,    15,    16,    12,     8,     9,     7,    14,    11,     6,
       5,    17,    10,     4,     0,   128,     0,     0,   108,     0,
       0,    54,     0,     0,   120,     0,     0,    98,     0,     0,
      46,     0,     0,    28,     0,     0,    86,     0,     0,    60,
       0,     0,    76,     0,     0,   104,     0,     0,    22,     0,
       0,   115,     0,     0,    38,     0,     0,    42,     0,     0,
       1,     0,     0,   130,     0,   127,     0,     0,   107,     0,
       0,    70,     0,    53,     0,     0,   122,   123,     0,   119,
       0,     0,   100,     0,    97,     0,     0,    48,     0,    45,
       0,    32,    34,    33,     0,    27,    92,     0,    91,    93,
      94,     0,    85,     0,    62,    65,    66,    64,    63,     0,
      59,     0,    80,    81,    82,     0,    75,     0,     0,   103,
      24,     0,     0,    21,     0,     0,   114,     0,     0,   111,
       0,    37,     0,     0,    50,     0,    41,     3,   131,   125,
       0,     0,   105,     0,     0,    51,     0,     0,     0,   117,
       0,     0,     0,    95,     0,     0,     0,    43,     0,     0,
      25,     0,     0,    83,     0,     0,    57,     0,     0,    73,
       0,     0,   101,     0,    19,     0,     0,     0,     0,     0,
      35,     0,     0,     0,     0,    68,    39,     0,     0,   132,
     129,   126,   106,    72,    69,    52,   124,   121,   118,   109,
      99,    96,    55,    47,    44,    29,    31,    30,    26,    88,
      87,    89,    90,    84,    61,    58,    77,    78,    79,    74,
     102,    23,    20,     0,     0,     0,   110,    36,     0,    56,
       0,    49,    40,   133,   112,     0,     0,    71,    67,   113,
     116
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    17,   140,    19,   141,    20,   110,    21,    22,    23,
     106,   153,    24,   107,   154,    25,   123,   124,   204,    90,
     205,    91,    26,   131,    27,   117,    28,   101,    29,    30,
     128,   148,    31,   149,    32,    95,    96,    33,    82,    83
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -90
static const yytype_int16 yypact[] =
{
     107,    20,    21,    31,    34,    38,    45,    50,    72,    76,
      77,    81,   132,   136,   137,   164,     1,     8,   -90,   -90,
     -90,   -90,   -90,   -90,   -90,   -90,   -90,   -90,   -90,   -90,
     -90,   -90,   -90,   -90,     4,   -90,     7,     4,   -90,    24,
      27,   -90,    55,     3,   -90,    88,    41,   -90,    91,    44,
     -90,   106,    30,   -90,   148,    17,   -90,   173,    17,   -90,
     174,    18,   -90,   178,     4,   -90,   181,   165,   -90,   182,
      49,   -90,   185,    66,   -90,   186,    69,   -90,   189,   165,
     -90,    75,   108,   -90,     4,   -90,   166,     4,   -90,     4,
     191,   -90,    27,   -90,     4,   194,   -90,   -90,     3,   -90,
       4,   195,   -90,    41,   -90,    27,   198,   -90,    44,   -90,
     199,   -90,   -90,   -90,    30,   -90,   -90,   202,   -90,   -90,
     -90,    17,   -90,   203,   -90,   -90,   -90,   -90,   -90,    17,
     -90,   206,   -90,   -90,   -90,    18,   -90,   207,     4,   -90,
     -90,   210,   165,   -90,     4,    87,   -90,   110,   211,   -90,
      66,   -90,   121,   214,   -90,    69,   -90,   -90,   122,   -90,
       4,   215,   -90,   218,   219,   -90,    27,   222,     9,   -90,
       3,   223,   226,   -90,    41,   227,   230,   -90,    44,   231,
     -90,    30,   234,   -90,    17,   235,   -90,    17,   238,   -90,
      18,   239,   -90,   242,   -90,   165,   243,   246,     4,     4,
     -90,    66,   247,     4,   250,   -90,   -90,    69,   251,   128,
     -90,   -90,   -90,   -90,   -90,   -90,   -90,   -90,   -90,   -90,
     -90,   -90,   -90,   -90,   -90,   -90,   -90,   -90,   -90,   -90,
     -90,   -90,   -90,   -90,   -90,   -90,   -90,   -90,   -90,   -90,
     -90,   -90,   -90,   142,   254,   255,   -90,   -90,   258,   -90,
     121,   -90,   -90,   -90,   -90,   143,   150,   -90,   -90,   -90,
     -90
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -90,   -90,     0,   -90,    42,   -90,    79,   -90,   -90,   -90,
     168,   119,   -39,   -47,    70,   -38,   149,    92,   -90,   -89,
      32,   114,   -40,   146,   -90,   162,   -90,   183,   -51,   -49,
     -44,   134,   -90,    84,   -90,   190,   117,   -90,   -36,   -11
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint16 yytable[] =
{
      18,    86,   102,   167,   118,   112,   119,   126,    80,   127,
     132,   120,   133,   111,   113,   116,   176,   134,   125,    94,
      79,     2,     2,    84,   216,    85,    81,    81,   137,    10,
      11,    11,    97,   100,   100,     3,    34,    37,    35,    38,
      87,     9,    88,    89,    36,    39,   105,    40,   161,    41,
      43,   163,    44,   164,    46,    42,    47,   100,    45,   102,
     105,    49,    48,    50,   172,   144,    52,   112,    53,    51,
     118,    92,   119,    93,    54,   111,   113,   120,   126,   157,
     127,   116,   147,   168,   132,   152,   133,    97,    55,   125,
      56,   134,    58,    61,    59,    62,    57,    64,   158,    65,
      60,    63,   193,   198,    98,    66,    99,   103,   197,   104,
       1,     2,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,   108,   159,   109,   160,   199,    13,    14,    15,
     220,   223,    16,   230,   226,   231,   126,   203,   127,   236,
     232,   237,   225,   227,   229,   209,   238,   125,    67,   210,
      68,   253,    70,    73,    71,    74,    69,   254,   259,    97,
      72,    75,   244,   245,   114,   260,   115,   248,     1,     2,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      76,   162,    77,   160,   196,    13,    14,    15,    78,   121,
     129,   122,   130,   182,   135,   241,   136,   138,   142,   139,
     143,   145,   150,   146,   151,   155,   165,   156,   166,   169,
     173,   170,   174,   177,   180,   178,   181,   183,   186,   184,
     187,   189,   192,   190,   160,   194,   200,   195,   201,   206,
     211,   207,   160,   212,   213,   160,   160,   215,   218,   166,
     170,   219,   221,   160,   174,   222,   224,   166,   178,   228,
     233,   181,   184,   235,   239,   187,   190,   240,   242,   160,
     195,   243,   247,   160,   201,   249,   252,   250,   207,   255,
     256,   160,   160,   257,   208,   160,   179,   251,   188,   234,
     214,   191,   258,   185,   202,   246,   175,   217,   171
};

static const yytype_uint8 yycheck[] =
{
       0,    37,    46,    92,    55,    52,    55,    58,     0,    58,
      61,    55,    61,    52,    52,    55,   105,    61,    58,    16,
      19,     4,     4,    16,    15,    18,    23,    23,    64,    12,
      13,    13,    43,    16,    16,     5,    16,    16,    18,    18,
      16,    11,    18,    16,    24,    24,    16,    16,    84,    18,
      16,    87,    18,    89,    16,    24,    18,    16,    24,   103,
      16,    16,    24,    18,   100,    16,    16,   114,    18,    24,
     121,    16,   121,    18,    24,   114,   114,   121,   129,    79,
     129,   121,    16,    94,   135,    16,   135,    98,    16,   129,
      18,   135,    16,    16,    18,    18,    24,    16,    23,    18,
      24,    24,   138,    16,    16,    24,    18,    16,   144,    18,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    16,    15,    18,    17,    16,    20,    21,    22,
     174,   178,    25,   184,   181,   184,   187,    16,   187,   190,
     184,   190,   181,   181,   184,    23,   190,   187,    16,   160,
      18,    23,    16,    16,    18,    18,    24,    15,    15,   170,
      24,    24,   198,   199,    16,    15,    18,   203,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      16,    15,    18,    17,   142,    20,    21,    22,    24,    16,
      16,    18,    18,   114,    16,   195,    18,    16,    16,    18,
      18,    16,    16,    18,    18,    16,    15,    18,    17,    15,
      15,    17,    17,    15,    15,    17,    17,    15,    15,    17,
      17,    15,    15,    17,    17,    15,    15,    17,    17,    15,
      15,    17,    17,    15,    15,    17,    17,    15,    15,    17,
      17,    15,    15,    17,    17,    15,    15,    17,    17,    15,
      15,    17,    17,    15,    15,    17,    17,    15,    15,    17,
      17,    15,    15,    17,    17,    15,    15,    17,    17,    15,
      15,    17,    17,    15,   155,    17,   108,   207,   129,   187,
     166,   135,   250,   121,   150,   201,   103,   170,    98
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    20,    21,    22,    25,    27,    28,    29,
      31,    33,    34,    35,    38,    41,    48,    50,    52,    54,
      55,    58,    60,    63,    16,    18,    24,    16,    18,    24,
      16,    18,    24,    16,    18,    24,    16,    18,    24,    16,
      18,    24,    16,    18,    24,    16,    18,    24,    16,    18,
      24,    16,    18,    24,    16,    18,    24,    16,    18,    24,
      16,    18,    24,    16,    18,    24,    16,    18,    24,    19,
       0,    23,    64,    65,    16,    18,    64,    16,    18,    16,
      45,    47,    16,    18,    16,    61,    62,    65,    16,    18,
      16,    53,    56,    16,    18,    16,    36,    39,    16,    18,
      32,    38,    39,    41,    16,    18,    48,    51,    54,    55,
      56,    16,    18,    42,    43,    48,    54,    55,    56,    16,
      18,    49,    54,    55,    56,    16,    18,    64,    16,    18,
      28,    30,    16,    18,    16,    16,    18,    16,    57,    59,
      16,    18,    16,    37,    40,    16,    18,    28,    23,    15,
      17,    64,    15,    64,    64,    15,    17,    45,    65,    15,
      17,    61,    64,    15,    17,    53,    45,    15,    17,    36,
      15,    17,    32,    15,    17,    51,    15,    17,    42,    15,
      17,    49,    15,    64,    15,    17,    30,    64,    16,    16,
      15,    17,    57,    16,    44,    46,    15,    17,    37,    23,
      65,    15,    15,    15,    47,    15,    15,    62,    15,    15,
      56,    15,    15,    39,    15,    38,    39,    41,    15,    48,
      54,    55,    56,    15,    43,    15,    54,    55,    56,    15,
      15,    28,    15,    15,    64,    64,    59,    15,    64,    15,
      17,    40,    15,    23,    15,    15,    15,    15,    46,    15,
      15
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
      case 28: /* "geometry_no_srid" */
#line 184 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1428 "lwin_wkt_parse.c"
	break;
      case 29: /* "geometrycollection" */
#line 185 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1433 "lwin_wkt_parse.c"
	break;
      case 31: /* "multisurface" */
#line 192 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1438 "lwin_wkt_parse.c"
	break;
      case 32: /* "surface_list" */
#line 171 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1443 "lwin_wkt_parse.c"
	break;
      case 33: /* "tin" */
#line 199 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1448 "lwin_wkt_parse.c"
	break;
      case 34: /* "polyhedralsurface" */
#line 198 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1453 "lwin_wkt_parse.c"
	break;
      case 35: /* "multipolygon" */
#line 191 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1458 "lwin_wkt_parse.c"
	break;
      case 36: /* "polygon_list" */
#line 172 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1463 "lwin_wkt_parse.c"
	break;
      case 37: /* "patch_list" */
#line 173 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1468 "lwin_wkt_parse.c"
	break;
      case 38: /* "polygon" */
#line 195 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1473 "lwin_wkt_parse.c"
	break;
      case 39: /* "polygon_untagged" */
#line 197 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1478 "lwin_wkt_parse.c"
	break;
      case 40: /* "patch" */
#line 196 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1483 "lwin_wkt_parse.c"
	break;
      case 41: /* "curvepolygon" */
#line 182 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1488 "lwin_wkt_parse.c"
	break;
      case 42: /* "curvering_list" */
#line 169 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1493 "lwin_wkt_parse.c"
	break;
      case 43: /* "curvering" */
#line 183 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1498 "lwin_wkt_parse.c"
	break;
      case 44: /* "patchring_list" */
#line 179 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1503 "lwin_wkt_parse.c"
	break;
      case 45: /* "ring_list" */
#line 178 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1508 "lwin_wkt_parse.c"
	break;
      case 46: /* "patchring" */
#line 168 "lwin_wkt_parse.y"
	{ ptarray_free((yyvaluep->ptarrayvalue)); };
#line 1513 "lwin_wkt_parse.c"
	break;
      case 47: /* "ring" */
#line 167 "lwin_wkt_parse.y"
	{ ptarray_free((yyvaluep->ptarrayvalue)); };
#line 1518 "lwin_wkt_parse.c"
	break;
      case 48: /* "compoundcurve" */
#line 181 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1523 "lwin_wkt_parse.c"
	break;
      case 49: /* "compound_list" */
#line 177 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1528 "lwin_wkt_parse.c"
	break;
      case 50: /* "multicurve" */
#line 188 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1533 "lwin_wkt_parse.c"
	break;
      case 51: /* "curve_list" */
#line 176 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1538 "lwin_wkt_parse.c"
	break;
      case 52: /* "multilinestring" */
#line 189 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1543 "lwin_wkt_parse.c"
	break;
      case 53: /* "linestring_list" */
#line 175 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1548 "lwin_wkt_parse.c"
	break;
      case 54: /* "circularstring" */
#line 180 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1553 "lwin_wkt_parse.c"
	break;
      case 55: /* "linestring" */
#line 186 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1558 "lwin_wkt_parse.c"
	break;
      case 56: /* "linestring_untagged" */
#line 187 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1563 "lwin_wkt_parse.c"
	break;
      case 57: /* "triangle_list" */
#line 170 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1568 "lwin_wkt_parse.c"
	break;
      case 58: /* "triangle" */
#line 200 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1573 "lwin_wkt_parse.c"
	break;
      case 59: /* "triangle_untagged" */
#line 201 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1578 "lwin_wkt_parse.c"
	break;
      case 60: /* "multipoint" */
#line 190 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1583 "lwin_wkt_parse.c"
	break;
      case 61: /* "point_list" */
#line 174 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1588 "lwin_wkt_parse.c"
	break;
      case 62: /* "point_untagged" */
#line 194 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1593 "lwin_wkt_parse.c"
	break;
      case 63: /* "point" */
#line 193 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1598 "lwin_wkt_parse.c"
	break;
      case 64: /* "ptarray" */
#line 166 "lwin_wkt_parse.y"
	{ ptarray_free((yyvaluep->ptarrayvalue)); };
#line 1603 "lwin_wkt_parse.c"
	break;

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
#line 207 "lwin_wkt_parse.y"
    { wkt_parser_geometry_new((yyvsp[(1) - (1)].geometryvalue), SRID_UNKNOWN); WKT_ERROR(); }
    break;

  case 3:
#line 209 "lwin_wkt_parse.y"
    { wkt_parser_geometry_new((yyvsp[(3) - (3)].geometryvalue), (yyvsp[(1) - (3)].integervalue)); WKT_ERROR(); }
    break;

  case 4:
#line 212 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 5:
#line 213 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 6:
#line 214 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 7:
#line 215 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 8:
#line 216 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 9:
#line 217 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 10:
#line 218 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 11:
#line 219 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 12:
#line 220 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 13:
#line 221 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 14:
#line 222 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 15:
#line 223 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 16:
#line 224 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 17:
#line 225 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 18:
#line 226 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 19:
#line 230 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(COLLECTIONTYPE, (yyvsp[(3) - (4)].geometryvalue), NULL); WKT_ERROR(); }
    break;

  case 20:
#line 232 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(COLLECTIONTYPE, (yyvsp[(4) - (5)].geometryvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 21:
#line 234 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(COLLECTIONTYPE, NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 22:
#line 236 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(COLLECTIONTYPE, NULL, NULL); WKT_ERROR(); }
    break;

  case 23:
#line 240 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 24:
#line 242 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 25:
#line 246 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTISURFACETYPE, (yyvsp[(3) - (4)].geometryvalue), NULL); WKT_ERROR(); }
    break;

  case 26:
#line 248 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTISURFACETYPE, (yyvsp[(4) - (5)].geometryvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 27:
#line 250 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTISURFACETYPE, NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 28:
#line 252 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTISURFACETYPE, NULL, NULL); WKT_ERROR(); }
    break;

  case 29:
#line 256 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 30:
#line 258 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 31:
#line 260 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 32:
#line 262 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 33:
#line 264 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 34:
#line 266 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 35:
#line 270 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(TINTYPE, (yyvsp[(3) - (4)].geometryvalue), NULL); WKT_ERROR(); }
    break;

  case 36:
#line 272 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(TINTYPE, (yyvsp[(4) - (5)].geometryvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 37:
#line 274 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(TINTYPE, NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 38:
#line 276 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(TINTYPE, NULL, NULL); WKT_ERROR(); }
    break;

  case 39:
#line 280 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(POLYHEDRALSURFACETYPE, (yyvsp[(3) - (4)].geometryvalue), NULL); WKT_ERROR(); }
    break;

  case 40:
#line 282 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(POLYHEDRALSURFACETYPE, (yyvsp[(4) - (5)].geometryvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 41:
#line 284 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(POLYHEDRALSURFACETYPE, NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 42:
#line 286 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(POLYHEDRALSURFACETYPE, NULL, NULL); WKT_ERROR(); }
    break;

  case 43:
#line 290 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTIPOLYGONTYPE, (yyvsp[(3) - (4)].geometryvalue), NULL); WKT_ERROR(); }
    break;

  case 44:
#line 292 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTIPOLYGONTYPE, (yyvsp[(4) - (5)].geometryvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 45:
#line 294 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTIPOLYGONTYPE, NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 46:
#line 296 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTIPOLYGONTYPE, NULL, NULL); WKT_ERROR(); }
    break;

  case 47:
#line 300 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 48:
#line 302 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 49:
#line 306 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 50:
#line 308 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 51:
#line 312 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_polygon_finalize((yyvsp[(3) - (4)].geometryvalue), NULL); WKT_ERROR(); }
    break;

  case 52:
#line 314 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_polygon_finalize((yyvsp[(4) - (5)].geometryvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 53:
#line 316 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_polygon_finalize(NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 54:
#line 318 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_polygon_finalize(NULL, NULL); WKT_ERROR(); }
    break;

  case 55:
#line 321 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(2) - (3)].geometryvalue); }
    break;

  case 56:
#line 324 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(2) - (3)].geometryvalue); }
    break;

  case 57:
#line 328 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_curvepolygon_finalize((yyvsp[(3) - (4)].geometryvalue), NULL); WKT_ERROR(); }
    break;

  case 58:
#line 330 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_curvepolygon_finalize((yyvsp[(4) - (5)].geometryvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 59:
#line 332 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_curvepolygon_finalize(NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 60:
#line 334 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_curvepolygon_finalize(NULL, NULL); WKT_ERROR(); }
    break;

  case 61:
#line 338 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_curvepolygon_add_ring((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 62:
#line 340 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_curvepolygon_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 63:
#line 343 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 64:
#line 344 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 65:
#line 345 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 66:
#line 346 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 67:
#line 350 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_polygon_add_ring((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].ptarrayvalue),'Z'); WKT_ERROR(); }
    break;

  case 68:
#line 352 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_polygon_new((yyvsp[(1) - (1)].ptarrayvalue),'Z'); WKT_ERROR(); }
    break;

  case 69:
#line 356 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_polygon_add_ring((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].ptarrayvalue),'2'); WKT_ERROR(); }
    break;

  case 70:
#line 358 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_polygon_new((yyvsp[(1) - (1)].ptarrayvalue),'2'); WKT_ERROR(); }
    break;

  case 71:
#line 361 "lwin_wkt_parse.y"
    { (yyval.ptarrayvalue) = (yyvsp[(2) - (3)].ptarrayvalue); }
    break;

  case 72:
#line 364 "lwin_wkt_parse.y"
    { (yyval.ptarrayvalue) = (yyvsp[(2) - (3)].ptarrayvalue); }
    break;

  case 73:
#line 368 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(COMPOUNDTYPE, (yyvsp[(3) - (4)].geometryvalue), NULL); WKT_ERROR(); }
    break;

  case 74:
#line 370 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(COMPOUNDTYPE, (yyvsp[(4) - (5)].geometryvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 75:
#line 372 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(COMPOUNDTYPE, NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 76:
#line 374 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(COMPOUNDTYPE, NULL, NULL); WKT_ERROR(); }
    break;

  case 77:
#line 378 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_compound_add_geom((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 78:
#line 380 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_compound_add_geom((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 79:
#line 382 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_compound_add_geom((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 80:
#line 384 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 81:
#line 386 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 82:
#line 388 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 83:
#line 392 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTICURVETYPE, (yyvsp[(3) - (4)].geometryvalue), NULL); WKT_ERROR(); }
    break;

  case 84:
#line 394 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTICURVETYPE, (yyvsp[(4) - (5)].geometryvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 85:
#line 396 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTICURVETYPE, NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 86:
#line 398 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTICURVETYPE, NULL, NULL); WKT_ERROR(); }
    break;

  case 87:
#line 402 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 88:
#line 404 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 89:
#line 406 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 90:
#line 408 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 91:
#line 410 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 92:
#line 412 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 93:
#line 414 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 94:
#line 416 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 95:
#line 420 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTILINETYPE, (yyvsp[(3) - (4)].geometryvalue), NULL); WKT_ERROR(); }
    break;

  case 96:
#line 422 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTILINETYPE, (yyvsp[(4) - (5)].geometryvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 97:
#line 424 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTILINETYPE, NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 98:
#line 426 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTILINETYPE, NULL, NULL); WKT_ERROR(); }
    break;

  case 99:
#line 430 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 100:
#line 432 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 101:
#line 436 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_circularstring_new((yyvsp[(3) - (4)].ptarrayvalue), NULL); WKT_ERROR(); }
    break;

  case 102:
#line 438 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_circularstring_new((yyvsp[(4) - (5)].ptarrayvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 103:
#line 440 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_circularstring_new(NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 104:
#line 442 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_circularstring_new(NULL, NULL); WKT_ERROR(); }
    break;

  case 105:
#line 446 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_linestring_new((yyvsp[(3) - (4)].ptarrayvalue), NULL); WKT_ERROR(); }
    break;

  case 106:
#line 448 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_linestring_new((yyvsp[(4) - (5)].ptarrayvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 107:
#line 450 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_linestring_new(NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 108:
#line 452 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_linestring_new(NULL, NULL); WKT_ERROR(); }
    break;

  case 109:
#line 456 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_linestring_new((yyvsp[(2) - (3)].ptarrayvalue), NULL); WKT_ERROR(); }
    break;

  case 110:
#line 460 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 111:
#line 462 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 112:
#line 466 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_triangle_new((yyvsp[(4) - (6)].ptarrayvalue), NULL); WKT_ERROR(); }
    break;

  case 113:
#line 468 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_triangle_new((yyvsp[(5) - (7)].ptarrayvalue), (yyvsp[(2) - (7)].stringvalue)); WKT_ERROR(); }
    break;

  case 114:
#line 470 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_triangle_new(NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 115:
#line 472 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_triangle_new(NULL, NULL); WKT_ERROR(); }
    break;

  case 116:
#line 476 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_triangle_new((yyvsp[(3) - (5)].ptarrayvalue), NULL); WKT_ERROR(); }
    break;

  case 117:
#line 480 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTIPOINTTYPE, (yyvsp[(3) - (4)].geometryvalue), NULL); WKT_ERROR(); }
    break;

  case 118:
#line 482 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTIPOINTTYPE, (yyvsp[(4) - (5)].geometryvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 119:
#line 484 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTIPOINTTYPE, NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 120:
#line 486 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTIPOINTTYPE, NULL, NULL); WKT_ERROR(); }
    break;

  case 121:
#line 490 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 122:
#line 492 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 123:
#line 496 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_point_new(wkt_parser_ptarray_new((yyvsp[(1) - (1)].coordinatevalue)),NULL); WKT_ERROR(); }
    break;

  case 124:
#line 498 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_point_new(wkt_parser_ptarray_new((yyvsp[(2) - (3)].coordinatevalue)),NULL); WKT_ERROR(); }
    break;

  case 125:
#line 502 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_point_new((yyvsp[(3) - (4)].ptarrayvalue), NULL); WKT_ERROR(); }
    break;

  case 126:
#line 504 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_point_new((yyvsp[(4) - (5)].ptarrayvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 127:
#line 506 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_point_new(NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 128:
#line 508 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_point_new(NULL,NULL); WKT_ERROR(); }
    break;

  case 129:
#line 512 "lwin_wkt_parse.y"
    { (yyval.ptarrayvalue) = wkt_parser_ptarray_add_coord((yyvsp[(1) - (3)].ptarrayvalue), (yyvsp[(3) - (3)].coordinatevalue)); WKT_ERROR(); }
    break;

  case 130:
#line 514 "lwin_wkt_parse.y"
    { (yyval.ptarrayvalue) = wkt_parser_ptarray_new((yyvsp[(1) - (1)].coordinatevalue)); WKT_ERROR(); }
    break;

  case 131:
#line 518 "lwin_wkt_parse.y"
    { (yyval.coordinatevalue) = wkt_parser_coord_2((yyvsp[(1) - (2)].doublevalue), (yyvsp[(2) - (2)].doublevalue)); WKT_ERROR(); }
    break;

  case 132:
#line 520 "lwin_wkt_parse.y"
    { (yyval.coordinatevalue) = wkt_parser_coord_3((yyvsp[(1) - (3)].doublevalue), (yyvsp[(2) - (3)].doublevalue), (yyvsp[(3) - (3)].doublevalue)); WKT_ERROR(); }
    break;

  case 133:
#line 522 "lwin_wkt_parse.y"
    { (yyval.coordinatevalue) = wkt_parser_coord_4((yyvsp[(1) - (4)].doublevalue), (yyvsp[(2) - (4)].doublevalue), (yyvsp[(3) - (4)].doublevalue), (yyvsp[(4) - (4)].doublevalue)); WKT_ERROR(); }
    break;


/* Line 1267 of yacc.c.  */
#line 2587 "lwin_wkt_parse.c"
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


#line 524 "lwin_wkt_parse.y"



