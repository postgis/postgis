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


/* 
* TODO
*
* When an error occurs and we stop the parse process, 
* what happens to the partially build structres, the point
* arrays and whatever else is hanging around at different
* parts of the process?
*/

/* WKT Parser */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lwin_wkt.h"
#include "lwin_wkt_parse.h"


/* Prototypes to quiet the compiler */
int wkt_yyparse(void);
void wkt_yyerror(const char *str);
int wkt_yywrap(void);
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
		global_parser_result.message = str;
		global_parser_result.errcode = PARSER_ERROR_OTHER;
		global_parser_result.errlocation = wkt_yylloc.last_column;
	}
	LWDEBUGF(4,"%s", str);
}

int wkt_yywrap(void)
{
	return 1;
}

/**
* Parse a WKT geometry string into an LWGEOM structure. Note that this
* process uses globals and is not re-entrant, so don't call it within itself
* (eg, from within other functions in lwin_wkt.c) or from a threaded program.
* Note that parser_result.wkinput picks up a reference to wktstr.
*/
int lwgeom_from_wkt(LWGEOM_PARSER_RESULT *parser_result, char *wktstr, int parser_check_flags)
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
	
	/* Clean up the return value by copying onto it */
	
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
			global_parser_result.message = parser_error_messages[global_parser_result.errcode];
			global_parser_result.errlocation = wkt_yylloc.last_column;
		}

		LWDEBUGF(5, "error returned by wkt_yyparse() @ %d: [%d] '%s'", 
		            global_parser_result.errlocation, 
		            global_parser_result.errcode, 
		            global_parser_result.message);
		
		/* Copy the global values into the return pointer */
		*parser_result = global_parser_result;
		return LW_FALSE;
	}
	
	/* Copy the global value into the return pointer */
	*parser_result = global_parser_result;
	return LW_TRUE;
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
#line 119 "lwin_wkt_parse.y"
{
	int integervalue;
	double doublevalue;
	char *stringvalue;
	LWGEOM *geometryvalue;
	POINT coordinatevalue;
	POINTARRAY *ptarrayvalue;
}
/* Line 193 of yacc.c.  */
#line 277 "lwin_wkt_parse.c"
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
#line 302 "lwin_wkt_parse.c"

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
#define YYLAST   274

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  26
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  35
/* YYNRULES -- Number of rules.  */
#define YYNRULES  119
/* YYNRULES -- Number of states.  */
#define YYNSTATES  240

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
     164,   170,   174,   177,   181,   186,   192,   196,   199,   203,
     205,   207,   209,   211,   213,   217,   219,   223,   228,   234,
     238,   241,   246,   252,   256,   259,   263,   267,   271,   273,
     275,   277,   282,   288,   292,   295,   299,   301,   306,   312,
     316,   319,   324,   330,   334,   337,   341,   345,   347,   354,
     362,   366,   369,   375,   380,   386,   390,   393,   397,   399,
     401,   405,   410,   416,   420,   423,   427,   429,   432,   436
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      27,     0,    -1,    28,    -1,    25,    19,    28,    -1,    58,
      -1,    50,    -1,    49,    -1,    44,    -1,    37,    -1,    39,
      -1,    55,    -1,    47,    -1,    35,    -1,    31,    -1,    45,
      -1,    33,    -1,    34,    -1,    53,    -1,    29,    -1,    14,
      16,    30,    15,    -1,    14,    24,    16,    30,    15,    -1,
      14,    24,    18,    -1,    14,    18,    -1,    30,    17,    28,
      -1,    28,    -1,     9,    16,    32,    15,    -1,     9,    24,
      16,    32,    15,    -1,     9,    24,    18,    -1,     9,    18,
      -1,    32,    17,    37,    -1,    32,    17,    39,    -1,    32,
      17,    38,    -1,    37,    -1,    39,    -1,    38,    -1,    21,
      16,    52,    15,    -1,    21,    24,    16,    52,    15,    -1,
      21,    24,    18,    -1,    21,    18,    -1,    22,    16,    36,
      15,    -1,    22,    24,    16,    36,    15,    -1,    22,    24,
      18,    -1,    22,    18,    -1,     8,    16,    36,    15,    -1,
       8,    24,    16,    36,    15,    -1,     8,    24,    18,    -1,
       8,    18,    -1,    36,    17,    38,    -1,    38,    -1,     5,
      16,    42,    15,    -1,     5,    24,    16,    42,    15,    -1,
       5,    24,    18,    -1,     5,    18,    -1,    16,    42,    15,
      -1,    11,    16,    40,    15,    -1,    11,    24,    16,    40,
      15,    -1,    11,    24,    18,    -1,    11,    18,    -1,    40,
      17,    41,    -1,    41,    -1,    51,    -1,    50,    -1,    44,
      -1,    49,    -1,    42,    17,    43,    -1,    43,    -1,    16,
      59,    15,    -1,    12,    16,    46,    15,    -1,    12,    24,
      16,    46,    15,    -1,    12,    24,    18,    -1,    12,    18,
      -1,    10,    16,    46,    15,    -1,    10,    24,    16,    46,
      15,    -1,    10,    24,    18,    -1,    10,    18,    -1,    46,
      17,    49,    -1,    46,    17,    50,    -1,    46,    17,    51,
      -1,    49,    -1,    50,    -1,    51,    -1,     7,    16,    48,
      15,    -1,     7,    24,    16,    48,    15,    -1,     7,    24,
      18,    -1,     7,    18,    -1,    48,    17,    51,    -1,    51,
      -1,    13,    16,    59,    15,    -1,    13,    24,    16,    59,
      15,    -1,    13,    24,    18,    -1,    13,    18,    -1,     4,
      16,    59,    15,    -1,     4,    24,    16,    59,    15,    -1,
       4,    24,    18,    -1,     4,    18,    -1,    16,    59,    15,
      -1,    52,    17,    54,    -1,    54,    -1,    20,    16,    16,
      59,    15,    15,    -1,    20,    24,    16,    16,    59,    15,
      15,    -1,    20,    24,    18,    -1,    20,    18,    -1,    16,
      16,    59,    15,    15,    -1,     6,    16,    56,    15,    -1,
       6,    24,    16,    56,    15,    -1,     6,    24,    18,    -1,
       6,    18,    -1,    56,    17,    57,    -1,    57,    -1,    60,
      -1,    16,    60,    15,    -1,     3,    16,    59,    15,    -1,
       3,    24,    16,    59,    15,    -1,     3,    24,    18,    -1,
       3,    18,    -1,    59,    17,    60,    -1,    60,    -1,    23,
      23,    -1,    23,    23,    23,    -1,    23,    23,    23,    23,
      -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   204,   204,   206,   210,   211,   212,   213,   214,   215,
     216,   217,   218,   219,   220,   221,   222,   223,   224,   227,
     229,   231,   233,   237,   239,   243,   245,   247,   249,   253,
     255,   257,   259,   261,   263,   267,   269,   271,   273,   277,
     279,   281,   283,   287,   289,   291,   293,   297,   299,   303,
     305,   307,   309,   313,   316,   318,   320,   322,   326,   328,
     332,   333,   334,   335,   338,   340,   344,   347,   349,   351,
     353,   357,   359,   361,   363,   367,   369,   371,   373,   375,
     377,   381,   383,   385,   387,   391,   393,   397,   399,   401,
     403,   407,   409,   411,   413,   417,   421,   423,   427,   429,
     431,   433,   437,   441,   443,   445,   447,   451,   453,   457,
     459,   463,   465,   467,   469,   473,   475,   479,   481,   483
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
  "polyhedralsurface", "multipolygon", "polygon_list", "polygon",
  "polygon_untagged", "curvepolygon", "curvering_list", "curvering",
  "ring_list", "ring", "compoundcurve", "multicurve", "curve_list",
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
      37,    37,    37,    38,    39,    39,    39,    39,    40,    40,
      41,    41,    41,    41,    42,    42,    43,    44,    44,    44,
      44,    45,    45,    45,    45,    46,    46,    46,    46,    46,
      46,    47,    47,    47,    47,    48,    48,    49,    49,    49,
      49,    50,    50,    50,    50,    51,    52,    52,    53,    53,
      53,    53,    54,    55,    55,    55,    55,    56,    56,    57,
      57,    58,    58,    58,    58,    59,    59,    60,    60,    60
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     3,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     4,
       5,     3,     2,     3,     1,     4,     5,     3,     2,     3,
       3,     3,     1,     1,     1,     4,     5,     3,     2,     4,
       5,     3,     2,     4,     5,     3,     2,     3,     1,     4,
       5,     3,     2,     3,     4,     5,     3,     2,     3,     1,
       1,     1,     1,     1,     3,     1,     3,     4,     5,     3,
       2,     4,     5,     3,     2,     3,     3,     3,     1,     1,
       1,     4,     5,     3,     2,     3,     1,     4,     5,     3,
       2,     4,     5,     3,     2,     3,     3,     1,     6,     7,
       3,     2,     5,     4,     5,     3,     2,     3,     1,     1,
       3,     4,     5,     3,     2,     3,     1,     2,     3,     4
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     2,    18,
      13,    15,    16,    12,     8,     9,     7,    14,    11,     6,
       5,    17,    10,     4,     0,   114,     0,     0,    94,     0,
       0,    52,     0,     0,   106,     0,     0,    84,     0,     0,
      46,     0,     0,    28,     0,     0,    74,     0,     0,    57,
       0,     0,    70,     0,     0,    90,     0,     0,    22,     0,
       0,   101,     0,     0,    38,     0,     0,    42,     0,     0,
       1,     0,     0,   116,     0,   113,     0,     0,    93,     0,
       0,    65,     0,    51,     0,     0,   108,   109,     0,   105,
       0,     0,    86,     0,    83,     0,     0,    48,     0,    45,
       0,    32,    34,    33,     0,    27,     0,    78,    79,    80,
       0,    73,     0,    59,    62,    63,    61,    60,     0,    56,
       0,     0,    69,     0,     0,    89,    24,     0,     0,    21,
       0,     0,   100,     0,     0,    97,     0,    37,     0,     0,
      41,     3,   117,   111,     0,     0,    91,     0,     0,    49,
       0,     0,     0,   103,     0,     0,     0,    81,     0,     0,
       0,    43,     0,     0,    25,     0,     0,    71,     0,     0,
      54,     0,     0,    67,     0,    87,     0,    19,     0,     0,
       0,     0,     0,    35,     0,     0,    39,     0,   118,   115,
     112,    92,    66,    64,    50,   110,   107,   104,    95,    85,
      82,    53,    47,    44,    29,    31,    30,    26,    75,    76,
      77,    72,    58,    55,    68,    88,    23,    20,     0,     0,
       0,    96,    36,    40,   119,    98,     0,     0,    99,   102
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    17,   136,    19,   137,    20,   110,    21,    22,    23,
     106,    24,   107,    25,   122,   123,    90,    91,    26,    27,
     116,    28,   101,    29,    30,   119,   144,    31,   145,    32,
      95,    96,    33,    82,    83
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -66
static const yytype_int16 yypact[] =
{
      12,    38,    42,    71,    76,    85,    89,    90,    94,   101,
     117,   122,   127,   153,   162,   163,    17,    49,   -66,   -66,
     -66,   -66,   -66,   -66,   -66,   -66,   -66,   -66,   -66,   -66,
     -66,   -66,   -66,   -66,    44,   -66,    29,    44,   -66,    45,
      60,   -66,    56,    34,   -66,    57,    74,   -66,    75,    80,
     -66,   104,    30,   -66,   131,    39,   -66,   132,    26,   -66,
     166,    39,   -66,   167,    44,   -66,   173,   154,   -66,   174,
      81,   -66,   177,    86,   -66,   178,    80,   -66,   181,   154,
     -66,    88,   155,   -66,    44,   -66,   183,    44,   -66,    44,
     186,   -66,    60,   -66,    44,   187,   -66,   -66,    34,   -66,
      44,   190,   -66,    74,   -66,    60,   191,   -66,    80,   -66,
     194,   -66,   -66,   -66,    30,   -66,   195,   -66,   -66,   -66,
      39,   -66,   198,   -66,   -66,   -66,   -66,   -66,    26,   -66,
     199,    39,   -66,   202,    44,   -66,   -66,   203,   154,   -66,
      44,    99,   -66,   100,   206,   -66,    86,   -66,   207,    80,
     -66,   -66,    98,   -66,    44,   210,   -66,   211,   214,   -66,
      60,   215,   124,   -66,    34,   218,   219,   -66,    74,   222,
     223,   -66,    80,   226,   -66,    30,   227,   -66,    39,   230,
     -66,    26,   231,   -66,   234,   -66,   235,   -66,   154,   238,
     239,    44,    44,   -66,    86,   242,   -66,   243,   121,   -66,
     -66,   -66,   -66,   -66,   -66,   -66,   -66,   -66,   -66,   -66,
     -66,   -66,   -66,   -66,   -66,   -66,   -66,   -66,   -66,   -66,
     -66,   -66,   -66,   -66,   -66,   -66,   -66,   -66,   138,   246,
     247,   -66,   -66,   -66,   -66,   -66,   139,   158,   -66,   -66
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -66,   -66,     0,   -66,   128,   -66,   151,   -66,   -66,   -66,
     -64,   -46,   -49,   -43,   140,    91,   -65,   107,   -45,   -66,
     -32,   -66,   168,   -51,   -50,   -44,   123,   -66,    79,   -66,
     172,   110,   -66,   -36,   -12
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint8 yytable[] =
{
      18,    86,   102,   112,   117,   118,   111,   125,   126,   113,
     117,   118,   148,   124,   127,     1,     2,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,   161,   133,   130,
       2,    97,    13,    14,    15,     3,    79,    16,    10,    11,
     170,     9,   100,     2,   173,    84,   105,    85,   155,    80,
      94,   157,    11,   158,    34,   100,    35,    81,    37,   102,
      38,    87,    36,    88,   166,   112,    39,    81,   111,   117,
     118,   113,    92,    98,    93,    99,    89,   125,   126,   151,
     117,   118,   162,   124,   127,   197,    97,    40,   179,    41,
     100,   103,    43,   104,    44,    42,   105,   140,   186,   184,
      45,    46,   143,    47,   190,    49,    52,    50,    53,    48,
      55,   152,    56,    51,    54,   191,   192,    58,    57,    59,
     108,   198,   109,   212,   209,    60,   215,   218,   219,   214,
     125,   126,   216,    61,   220,    62,   124,   127,    64,   205,
      65,    63,   199,    67,   234,    68,    66,   114,   120,   115,
     121,    69,    97,   235,   238,   229,   230,     1,     2,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    70,
     153,    71,   154,   239,    13,    14,    15,    72,    73,    76,
      74,    77,   128,   131,   129,   132,    75,    78,   226,   134,
     138,   135,   139,   141,   146,   142,   147,   149,   156,   150,
     154,   159,   163,   160,   164,   167,   171,   168,   172,   174,
     177,   175,   178,   180,   183,   181,   178,   185,   187,   154,
     188,   193,   196,   194,   172,   200,   201,   154,   154,   202,
     204,   154,   160,   207,   208,   164,   154,   210,   211,   168,
     160,   213,   217,   172,   175,   221,   223,   178,   181,   224,
     225,   178,   154,   227,   228,   188,   154,   232,   233,   194,
     172,   236,   237,   154,   154,   176,   189,   203,   182,   195,
     165,   169,   222,   231,   206
};

static const yytype_uint8 yycheck[] =
{
       0,    37,    46,    52,    55,    55,    52,    58,    58,    52,
      61,    61,    76,    58,    58,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    92,    64,    61,
       4,    43,    20,    21,    22,     5,    19,    25,    12,    13,
     105,    11,    16,     4,   108,    16,    16,    18,    84,     0,
      16,    87,    13,    89,    16,    16,    18,    23,    16,   103,
      18,    16,    24,    18,   100,   114,    24,    23,   114,   120,
     120,   114,    16,    16,    18,    18,    16,   128,   128,    79,
     131,   131,    94,   128,   128,   149,    98,    16,   120,    18,
      16,    16,    16,    18,    18,    24,    16,    16,   134,   131,
      24,    16,    16,    18,   140,    16,    16,    18,    18,    24,
      16,    23,    18,    24,    24,    16,    16,    16,    24,    18,
      16,    23,    18,   172,   168,    24,   175,   178,   178,   175,
     181,   181,   175,    16,   178,    18,   181,   181,    16,    15,
      18,    24,   154,    16,    23,    18,    24,    16,    16,    18,
      18,    24,   164,    15,    15,   191,   192,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    16,
      15,    18,    17,    15,    20,    21,    22,    24,    16,    16,
      18,    18,    16,    16,    18,    18,    24,    24,   188,    16,
      16,    18,    18,    16,    16,    18,    18,    16,    15,    18,
      17,    15,    15,    17,    17,    15,    15,    17,    17,    15,
      15,    17,    17,    15,    15,    17,    17,    15,    15,    17,
      17,    15,    15,    17,    17,    15,    15,    17,    17,    15,
      15,    17,    17,    15,    15,    17,    17,    15,    15,    17,
      17,    15,    15,    17,    17,    15,    15,    17,    17,    15,
      15,    17,    17,    15,    15,    17,    17,    15,    15,    17,
      17,    15,    15,    17,    17,   114,   138,   160,   128,   146,
      98,   103,   181,   194,   164
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    20,    21,    22,    25,    27,    28,    29,
      31,    33,    34,    35,    37,    39,    44,    45,    47,    49,
      50,    53,    55,    58,    16,    18,    24,    16,    18,    24,
      16,    18,    24,    16,    18,    24,    16,    18,    24,    16,
      18,    24,    16,    18,    24,    16,    18,    24,    16,    18,
      24,    16,    18,    24,    16,    18,    24,    16,    18,    24,
      16,    18,    24,    16,    18,    24,    16,    18,    24,    19,
       0,    23,    59,    60,    16,    18,    59,    16,    18,    16,
      42,    43,    16,    18,    16,    56,    57,    60,    16,    18,
      16,    48,    51,    16,    18,    16,    36,    38,    16,    18,
      32,    37,    38,    39,    16,    18,    46,    49,    50,    51,
      16,    18,    40,    41,    44,    49,    50,    51,    16,    18,
      46,    16,    18,    59,    16,    18,    28,    30,    16,    18,
      16,    16,    18,    16,    52,    54,    16,    18,    36,    16,
      18,    28,    23,    15,    17,    59,    15,    59,    59,    15,
      17,    42,    60,    15,    17,    56,    59,    15,    17,    48,
      42,    15,    17,    36,    15,    17,    32,    15,    17,    46,
      15,    17,    40,    15,    46,    15,    59,    15,    17,    30,
      59,    16,    16,    15,    17,    52,    15,    36,    23,    60,
      15,    15,    15,    43,    15,    15,    57,    15,    15,    51,
      15,    15,    38,    15,    37,    38,    39,    15,    49,    50,
      51,    15,    41,    15,    15,    15,    28,    15,    15,    59,
      59,    54,    15,    15,    23,    15,    15,    15,    15,    15
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
#line 183 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1421 "lwin_wkt_parse.c"
	break;
      case 29: /* "geometrycollection" */
#line 184 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1426 "lwin_wkt_parse.c"
	break;
      case 31: /* "multisurface" */
#line 191 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1431 "lwin_wkt_parse.c"
	break;
      case 33: /* "tin" */
#line 197 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1436 "lwin_wkt_parse.c"
	break;
      case 34: /* "polyhedralsurface" */
#line 196 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1441 "lwin_wkt_parse.c"
	break;
      case 35: /* "multipolygon" */
#line 190 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1446 "lwin_wkt_parse.c"
	break;
      case 37: /* "polygon" */
#line 194 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1451 "lwin_wkt_parse.c"
	break;
      case 38: /* "polygon_untagged" */
#line 195 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1456 "lwin_wkt_parse.c"
	break;
      case 39: /* "curvepolygon" */
#line 181 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1461 "lwin_wkt_parse.c"
	break;
      case 41: /* "curvering" */
#line 182 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1466 "lwin_wkt_parse.c"
	break;
      case 43: /* "ring" */
#line 178 "lwin_wkt_parse.y"
	{ ptarray_freeall((yyvaluep->ptarrayvalue)); };
#line 1471 "lwin_wkt_parse.c"
	break;
      case 44: /* "compoundcurve" */
#line 180 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1476 "lwin_wkt_parse.c"
	break;
      case 45: /* "multicurve" */
#line 187 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1481 "lwin_wkt_parse.c"
	break;
      case 47: /* "multilinestring" */
#line 188 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1486 "lwin_wkt_parse.c"
	break;
      case 49: /* "circularstring" */
#line 179 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1491 "lwin_wkt_parse.c"
	break;
      case 50: /* "linestring" */
#line 185 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1496 "lwin_wkt_parse.c"
	break;
      case 51: /* "linestring_untagged" */
#line 186 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1501 "lwin_wkt_parse.c"
	break;
      case 53: /* "triangle" */
#line 198 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1506 "lwin_wkt_parse.c"
	break;
      case 54: /* "triangle_untagged" */
#line 199 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1511 "lwin_wkt_parse.c"
	break;
      case 55: /* "multipoint" */
#line 189 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1516 "lwin_wkt_parse.c"
	break;
      case 57: /* "point_untagged" */
#line 193 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1521 "lwin_wkt_parse.c"
	break;
      case 58: /* "point" */
#line 192 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };
#line 1526 "lwin_wkt_parse.c"
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
#line 205 "lwin_wkt_parse.y"
    { wkt_parser_geometry_new((yyvsp[(1) - (1)].geometryvalue), SRID_UNKNOWN); WKT_ERROR(); }
    break;

  case 3:
#line 207 "lwin_wkt_parse.y"
    { wkt_parser_geometry_new((yyvsp[(3) - (3)].geometryvalue), (yyvsp[(1) - (3)].integervalue)); WKT_ERROR(); }
    break;

  case 4:
#line 210 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 5:
#line 211 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 6:
#line 212 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 7:
#line 213 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 8:
#line 214 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 9:
#line 215 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 10:
#line 216 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 11:
#line 217 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 12:
#line 218 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 13:
#line 219 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 14:
#line 220 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 15:
#line 221 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 16:
#line 222 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 17:
#line 223 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 18:
#line 224 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 19:
#line 228 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(COLLECTIONTYPE, (yyvsp[(3) - (4)].geometryvalue), NULL); WKT_ERROR(); }
    break;

  case 20:
#line 230 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(COLLECTIONTYPE, (yyvsp[(4) - (5)].geometryvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 21:
#line 232 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(COLLECTIONTYPE, NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 22:
#line 234 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(COLLECTIONTYPE, NULL, NULL); WKT_ERROR(); }
    break;

  case 23:
#line 238 "lwin_wkt_parse.y"
    { wkt_parser_collection_add_geom((yyval.geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 24:
#line 240 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 25:
#line 244 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTISURFACETYPE, (yyvsp[(3) - (4)].geometryvalue), NULL); WKT_ERROR(); }
    break;

  case 26:
#line 246 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTISURFACETYPE, (yyvsp[(4) - (5)].geometryvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 27:
#line 248 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTISURFACETYPE, NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 28:
#line 250 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTISURFACETYPE, NULL, NULL); WKT_ERROR(); }
    break;

  case 29:
#line 254 "lwin_wkt_parse.y"
    { wkt_parser_collection_add_geom((yyval.geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 30:
#line 256 "lwin_wkt_parse.y"
    { wkt_parser_collection_add_geom((yyval.geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 31:
#line 258 "lwin_wkt_parse.y"
    { wkt_parser_collection_add_geom((yyval.geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 32:
#line 260 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 33:
#line 262 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 34:
#line 264 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 35:
#line 268 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(TINTYPE, (yyvsp[(3) - (4)].geometryvalue), NULL); WKT_ERROR(); }
    break;

  case 36:
#line 270 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(TINTYPE, (yyvsp[(4) - (5)].geometryvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 37:
#line 272 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(TINTYPE, NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 38:
#line 274 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(TINTYPE, NULL, NULL); WKT_ERROR(); }
    break;

  case 39:
#line 278 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(POLYHEDRALSURFACETYPE, (yyvsp[(3) - (4)].geometryvalue), NULL); WKT_ERROR(); }
    break;

  case 40:
#line 280 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(POLYHEDRALSURFACETYPE, (yyvsp[(4) - (5)].geometryvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 41:
#line 282 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(POLYHEDRALSURFACETYPE, NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 42:
#line 284 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(POLYHEDRALSURFACETYPE, NULL, NULL); WKT_ERROR(); }
    break;

  case 43:
#line 288 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTIPOLYGONTYPE, (yyvsp[(3) - (4)].geometryvalue), NULL); WKT_ERROR(); }
    break;

  case 44:
#line 290 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTIPOLYGONTYPE, (yyvsp[(4) - (5)].geometryvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 45:
#line 292 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTIPOLYGONTYPE, NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 46:
#line 294 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTIPOLYGONTYPE, NULL, NULL); WKT_ERROR(); }
    break;

  case 47:
#line 298 "lwin_wkt_parse.y"
    { wkt_parser_collection_add_geom((yyval.geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 48:
#line 300 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 49:
#line 304 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_polygon_finalize((yyvsp[(3) - (4)].geometryvalue), NULL); WKT_ERROR(); }
    break;

  case 50:
#line 306 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_polygon_finalize((yyvsp[(4) - (5)].geometryvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 51:
#line 308 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_polygon_finalize(NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 52:
#line 310 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_polygon_finalize(NULL, NULL); WKT_ERROR(); }
    break;

  case 53:
#line 313 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(2) - (3)].geometryvalue); }
    break;

  case 54:
#line 317 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_curvepolygon_finalize((yyvsp[(3) - (4)].geometryvalue), NULL); WKT_ERROR(); }
    break;

  case 55:
#line 319 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_curvepolygon_finalize((yyvsp[(4) - (5)].geometryvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 56:
#line 321 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_curvepolygon_finalize(NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 57:
#line 323 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_curvepolygon_finalize(NULL, NULL); WKT_ERROR(); }
    break;

  case 58:
#line 327 "lwin_wkt_parse.y"
    { wkt_parser_curvepolygon_add_ring((yyval.geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 59:
#line 329 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_curvepolygon_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 60:
#line 332 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 61:
#line 333 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 62:
#line 334 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 63:
#line 335 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 64:
#line 339 "lwin_wkt_parse.y"
    { wkt_parser_polygon_add_ring((yyval.geometryvalue),(yyvsp[(3) - (3)].ptarrayvalue)); WKT_ERROR(); }
    break;

  case 65:
#line 341 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_polygon_new((yyvsp[(1) - (1)].ptarrayvalue)); WKT_ERROR(); }
    break;

  case 66:
#line 344 "lwin_wkt_parse.y"
    { (yyval.ptarrayvalue) = (yyvsp[(2) - (3)].ptarrayvalue); }
    break;

  case 67:
#line 348 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(COMPOUNDTYPE, (yyvsp[(3) - (4)].geometryvalue), NULL); WKT_ERROR(); }
    break;

  case 68:
#line 350 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(COMPOUNDTYPE, (yyvsp[(4) - (5)].geometryvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 69:
#line 352 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(COMPOUNDTYPE, NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 70:
#line 354 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(COMPOUNDTYPE, NULL, NULL); WKT_ERROR(); }
    break;

  case 71:
#line 358 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTICURVETYPE, (yyvsp[(3) - (4)].geometryvalue), NULL); WKT_ERROR(); }
    break;

  case 72:
#line 360 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTICURVETYPE, (yyvsp[(4) - (5)].geometryvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 73:
#line 362 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTICURVETYPE, NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 74:
#line 364 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTICURVETYPE, NULL, NULL); WKT_ERROR(); }
    break;

  case 75:
#line 368 "lwin_wkt_parse.y"
    { wkt_parser_collection_add_geom((yyval.geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 76:
#line 370 "lwin_wkt_parse.y"
    { wkt_parser_collection_add_geom((yyval.geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 77:
#line 372 "lwin_wkt_parse.y"
    { wkt_parser_collection_add_geom((yyval.geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 78:
#line 374 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 79:
#line 376 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 80:
#line 378 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 81:
#line 382 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTILINETYPE, (yyvsp[(3) - (4)].geometryvalue), NULL); WKT_ERROR(); }
    break;

  case 82:
#line 384 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTILINETYPE, (yyvsp[(4) - (5)].geometryvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 83:
#line 386 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTILINETYPE, NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 84:
#line 388 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTILINETYPE, NULL, NULL); WKT_ERROR(); }
    break;

  case 85:
#line 392 "lwin_wkt_parse.y"
    { wkt_parser_collection_add_geom((yyval.geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 86:
#line 394 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 87:
#line 398 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_circularstring_new((yyvsp[(3) - (4)].ptarrayvalue), NULL); WKT_ERROR(); }
    break;

  case 88:
#line 400 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_circularstring_new((yyvsp[(4) - (5)].ptarrayvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 89:
#line 402 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_circularstring_new(NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 90:
#line 404 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_circularstring_new(NULL, NULL); WKT_ERROR(); }
    break;

  case 91:
#line 408 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_linestring_new((yyvsp[(3) - (4)].ptarrayvalue), NULL); WKT_ERROR(); }
    break;

  case 92:
#line 410 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_linestring_new((yyvsp[(4) - (5)].ptarrayvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 93:
#line 412 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_linestring_new(NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 94:
#line 414 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_linestring_new(NULL, NULL); WKT_ERROR(); }
    break;

  case 95:
#line 418 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_linestring_new((yyvsp[(2) - (3)].ptarrayvalue), NULL); WKT_ERROR(); }
    break;

  case 96:
#line 422 "lwin_wkt_parse.y"
    { wkt_parser_collection_add_geom((yyval.geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 97:
#line 424 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 98:
#line 428 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_triangle_new((yyvsp[(4) - (6)].ptarrayvalue), NULL); WKT_ERROR(); }
    break;

  case 99:
#line 430 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_triangle_new((yyvsp[(5) - (7)].ptarrayvalue), (yyvsp[(2) - (7)].stringvalue)); WKT_ERROR(); }
    break;

  case 100:
#line 432 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_triangle_new(NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 101:
#line 434 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_triangle_new(NULL, NULL); WKT_ERROR(); }
    break;

  case 102:
#line 438 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_triangle_new((yyvsp[(3) - (5)].ptarrayvalue), NULL); WKT_ERROR(); }
    break;

  case 103:
#line 442 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTIPOINTTYPE, (yyvsp[(3) - (4)].geometryvalue), NULL); WKT_ERROR(); }
    break;

  case 104:
#line 444 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTIPOINTTYPE, (yyvsp[(4) - (5)].geometryvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 105:
#line 446 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTIPOINTTYPE, NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 106:
#line 448 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTIPOINTTYPE, NULL, NULL); WKT_ERROR(); }
    break;

  case 107:
#line 452 "lwin_wkt_parse.y"
    { wkt_parser_collection_add_geom((yyval.geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 108:
#line 454 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 109:
#line 458 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_point_new(wkt_parser_ptarray_new((yyvsp[(1) - (1)].coordinatevalue)),NULL); WKT_ERROR(); }
    break;

  case 110:
#line 460 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_point_new(wkt_parser_ptarray_new((yyvsp[(2) - (3)].coordinatevalue)),NULL); WKT_ERROR(); }
    break;

  case 111:
#line 464 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_point_new((yyvsp[(3) - (4)].ptarrayvalue), NULL); WKT_ERROR(); }
    break;

  case 112:
#line 466 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_point_new((yyvsp[(4) - (5)].ptarrayvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 113:
#line 468 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_point_new(NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 114:
#line 470 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_point_new(NULL,NULL); WKT_ERROR(); }
    break;

  case 115:
#line 474 "lwin_wkt_parse.y"
    { wkt_parser_ptarray_add_coord((yyval.ptarrayvalue), (yyvsp[(3) - (3)].coordinatevalue)); WKT_ERROR(); }
    break;

  case 116:
#line 476 "lwin_wkt_parse.y"
    { (yyval.ptarrayvalue) = wkt_parser_ptarray_new((yyvsp[(1) - (1)].coordinatevalue)); WKT_ERROR(); }
    break;

  case 117:
#line 480 "lwin_wkt_parse.y"
    { (yyval.coordinatevalue) = wkt_parser_coord_2((yyvsp[(1) - (2)].doublevalue), (yyvsp[(2) - (2)].doublevalue)); WKT_ERROR(); }
    break;

  case 118:
#line 482 "lwin_wkt_parse.y"
    { (yyval.coordinatevalue) = wkt_parser_coord_3((yyvsp[(1) - (3)].doublevalue), (yyvsp[(2) - (3)].doublevalue), (yyvsp[(3) - (3)].doublevalue)); WKT_ERROR(); }
    break;

  case 119:
#line 484 "lwin_wkt_parse.y"
    { (yyval.coordinatevalue) = wkt_parser_coord_4((yyvsp[(1) - (4)].doublevalue), (yyvsp[(2) - (4)].doublevalue), (yyvsp[(3) - (4)].doublevalue), (yyvsp[(4) - (4)].doublevalue)); WKT_ERROR(); }
    break;


/* Line 1267 of yacc.c.  */
#line 2440 "lwin_wkt_parse.c"
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


#line 486 "lwin_wkt_parse.y"



