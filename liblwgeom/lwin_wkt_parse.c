/* A Bison parser, made by GNU Bison 2.5.  */

/* Bison implementation for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2011 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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
#define YYBISON_VERSION "2.5"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 1

/* Substitute the variable and function names.  */
#define yyparse         wkt_yyparse
#define yylex           wkt_yylex
#define yyerror         wkt_yyerror
#define yylval          wkt_yylval
#define yychar          wkt_yychar
#define yydebug         wkt_yydebug
#define yynerrs         wkt_yynerrs
#define yylloc          wkt_yylloc

/* Copy the first part of user declarations.  */

/* Line 268 of yacc.c  */
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




/* Line 268 of yacc.c  */
#line 178 "y.tab.c"

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




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 293 of yacc.c  */
#line 103 "lwin_wkt_parse.y"

	int integervalue;
	double doublevalue;
	char *stringvalue;
	LWGEOM *geometryvalue;
	POINT coordinatevalue;
	POINTARRAY *ptarrayvalue;



/* Line 293 of yacc.c  */
#line 275 "y.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
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


/* Line 343 of yacc.c  */
#line 300 "y.tab.c"

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
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
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
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
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
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
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
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  80
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   294

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  26
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  40
/* YYNRULES -- Number of rules.  */
#define YYNRULES  136
/* YYNRULES -- Number of states.  */
#define YYNSTATES  264

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
     163,   165,   170,   176,   180,   183,   187,   189,   193,   198,
     204,   208,   211,   215,   217,   219,   221,   223,   225,   229,
     231,   235,   237,   241,   245,   250,   256,   260,   263,   267,
     271,   275,   277,   279,   281,   286,   292,   296,   299,   303,
     307,   311,   315,   317,   319,   321,   323,   328,   334,   338,
     341,   345,   347,   352,   358,   362,   365,   370,   376,   380,
     383,   387,   389,   393,   395,   402,   410,   414,   417,   423,
     428,   434,   438,   441,   445,   447,   449,   453,   455,   460,
     466,   470,   473,   477,   479,   482,   486
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
       5,    18,    -1,    16,    45,    15,    -1,    18,    -1,    16,
      44,    15,    -1,    11,    16,    42,    15,    -1,    11,    24,
      16,    42,    15,    -1,    11,    24,    18,    -1,    11,    18,
      -1,    42,    17,    43,    -1,    43,    -1,    56,    -1,    55,
      -1,    48,    -1,    54,    -1,    44,    17,    46,    -1,    46,
      -1,    45,    17,    47,    -1,    47,    -1,    16,    64,    15,
      -1,    16,    64,    15,    -1,    12,    16,    49,    15,    -1,
      12,    24,    16,    49,    15,    -1,    12,    24,    18,    -1,
      12,    18,    -1,    49,    17,    54,    -1,    49,    17,    55,
      -1,    49,    17,    56,    -1,    54,    -1,    55,    -1,    56,
      -1,    10,    16,    51,    15,    -1,    10,    24,    16,    51,
      15,    -1,    10,    24,    18,    -1,    10,    18,    -1,    51,
      17,    54,    -1,    51,    17,    48,    -1,    51,    17,    55,
      -1,    51,    17,    56,    -1,    54,    -1,    48,    -1,    55,
      -1,    56,    -1,     7,    16,    53,    15,    -1,     7,    24,
      16,    53,    15,    -1,     7,    24,    18,    -1,     7,    18,
      -1,    53,    17,    56,    -1,    56,    -1,    13,    16,    64,
      15,    -1,    13,    24,    16,    64,    15,    -1,    13,    24,
      18,    -1,    13,    18,    -1,     4,    16,    64,    15,    -1,
       4,    24,    16,    64,    15,    -1,     4,    24,    18,    -1,
       4,    18,    -1,    16,    64,    15,    -1,    18,    -1,    57,
      17,    59,    -1,    59,    -1,    20,    16,    16,    64,    15,
      15,    -1,    20,    24,    16,    16,    64,    15,    15,    -1,
      20,    24,    18,    -1,    20,    18,    -1,    16,    16,    64,
      15,    15,    -1,     6,    16,    61,    15,    -1,     6,    24,
      16,    61,    15,    -1,     6,    24,    18,    -1,     6,    18,
      -1,    61,    17,    62,    -1,    62,    -1,    65,    -1,    16,
      65,    15,    -1,    18,    -1,     3,    16,    64,    15,    -1,
       3,    24,    16,    64,    15,    -1,     3,    24,    18,    -1,
       3,    18,    -1,    64,    17,    65,    -1,    65,    -1,    23,
      23,    -1,    23,    23,    23,    -1,    23,    23,    23,    23,
      -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   207,   207,   209,   213,   214,   215,   216,   217,   218,
     219,   220,   221,   222,   223,   224,   225,   226,   227,   230,
     232,   234,   236,   240,   242,   246,   248,   250,   252,   256,
     258,   260,   262,   264,   266,   270,   272,   274,   276,   280,
     282,   284,   286,   290,   292,   294,   296,   300,   302,   306,
     308,   312,   314,   316,   318,   322,   324,   328,   331,   333,
     335,   337,   341,   343,   347,   348,   349,   350,   353,   355,
     359,   361,   365,   368,   371,   373,   375,   377,   381,   383,
     385,   387,   389,   391,   395,   397,   399,   401,   405,   407,
     409,   411,   413,   415,   417,   419,   423,   425,   427,   429,
     433,   435,   439,   441,   443,   445,   449,   451,   453,   455,
     459,   461,   465,   467,   471,   473,   475,   477,   481,   485,
     487,   489,   491,   495,   497,   501,   503,   505,   509,   511,
     513,   515,   519,   521,   525,   527,   529
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
      37,    38,    38,    38,    38,    39,    39,    40,    41,    41,
      41,    41,    42,    42,    43,    43,    43,    43,    44,    44,
      45,    45,    46,    47,    48,    48,    48,    48,    49,    49,
      49,    49,    49,    49,    50,    50,    50,    50,    51,    51,
      51,    51,    51,    51,    51,    51,    52,    52,    52,    52,
      53,    53,    54,    54,    54,    54,    55,    55,    55,    55,
      56,    56,    57,    57,    58,    58,    58,    58,    59,    60,
      60,    60,    60,    61,    61,    62,    62,    62,    63,    63,
      63,    63,    64,    64,    65,    65,    65
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     3,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     4,
       5,     3,     2,     3,     1,     4,     5,     3,     2,     3,
       3,     3,     1,     1,     1,     4,     5,     3,     2,     4,
       5,     3,     2,     4,     5,     3,     2,     3,     1,     3,
       1,     4,     5,     3,     2,     3,     1,     3,     4,     5,
       3,     2,     3,     1,     1,     1,     1,     1,     3,     1,
       3,     1,     3,     3,     4,     5,     3,     2,     3,     3,
       3,     1,     1,     1,     4,     5,     3,     2,     3,     3,
       3,     3,     1,     1,     1,     1,     4,     5,     3,     2,
       3,     1,     4,     5,     3,     2,     4,     5,     3,     2,
       3,     1,     3,     1,     6,     7,     3,     2,     5,     4,
       5,     3,     2,     3,     1,     1,     3,     1,     4,     5,
       3,     2,     3,     1,     2,     3,     4
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     2,    18,
      13,    15,    16,    12,     8,     9,     7,    14,    11,     6,
       5,    17,    10,     4,     0,   131,     0,     0,   109,     0,
       0,    54,     0,     0,   122,     0,     0,    99,     0,     0,
      46,     0,     0,    28,     0,     0,    87,     0,     0,    61,
       0,     0,    77,     0,     0,   105,     0,     0,    22,     0,
       0,   117,     0,     0,    38,     0,     0,    42,     0,     0,
       1,     0,     0,   133,     0,   130,     0,     0,   108,     0,
       0,    71,     0,    53,     0,   127,     0,   124,   125,     0,
     121,     0,   111,     0,   101,     0,    98,     0,    56,     0,
      48,     0,    45,     0,    32,    34,    33,     0,    27,    93,
       0,    92,    94,    95,     0,    86,     0,    63,    66,    67,
      65,    64,     0,    60,     0,    81,    82,    83,     0,    76,
       0,     0,   104,    24,     0,     0,    21,     0,     0,   116,
       0,     0,   113,     0,    37,     0,     0,    50,     0,    41,
       3,   134,   128,     0,     0,   106,     0,     0,    51,     0,
       0,     0,   119,     0,     0,     0,    96,     0,     0,     0,
      43,     0,     0,    25,     0,     0,    84,     0,     0,    58,
       0,     0,    74,     0,     0,   102,     0,    19,     0,     0,
       0,     0,     0,    35,     0,     0,     0,     0,    69,    39,
       0,     0,   135,   132,   129,   107,    73,    70,    52,   126,
     123,   120,   110,   100,    97,    55,    47,    44,    29,    31,
      30,    26,    89,    88,    90,    91,    85,    62,    59,    78,
      79,    80,    75,   103,    23,    20,     0,     0,     0,   112,
      36,     0,    57,     0,    49,    40,   136,   114,     0,     0,
      72,    68,   115,   118
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    17,   143,    19,   144,    20,   113,    21,    22,    23,
     109,   156,    24,   110,   157,    25,   126,   127,   207,    90,
     208,    91,    26,   134,    27,   120,    28,   103,    29,    30,
     131,   151,    31,   152,    32,    96,    97,    33,    82,    83
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -90
static const yytype_int16 yypact[] =
{
     109,    -2,    16,    23,    26,    36,    39,    40,    52,    53,
      74,    79,    83,    84,   108,   137,     7,    46,   -90,   -90,
     -90,   -90,   -90,   -90,   -90,   -90,   -90,   -90,   -90,   -90,
     -90,   -90,   -90,   -90,    43,   -90,    27,    43,   -90,    88,
      33,   -90,   144,   140,   -90,   167,   175,   -90,   176,   179,
     -90,   183,    20,   -90,   184,    11,   -90,   187,    11,   -90,
     188,    17,   -90,   191,    43,   -90,   192,   168,   -90,   195,
      51,   -90,   196,    56,   -90,   199,    70,   -90,   200,   168,
     -90,    68,   110,   -90,    43,   -90,   169,    43,   -90,    43,
     204,   -90,    33,   -90,    43,   -90,   205,   -90,   -90,   140,
     -90,    43,   -90,   208,   -90,   175,   -90,    33,   -90,   209,
     -90,   179,   -90,   212,   -90,   -90,   -90,    20,   -90,   -90,
     213,   -90,   -90,   -90,    11,   -90,   216,   -90,   -90,   -90,
     -90,   -90,    11,   -90,   217,   -90,   -90,   -90,    17,   -90,
     220,    43,   -90,   -90,   221,   168,   -90,    43,    80,   -90,
      93,   224,   -90,    56,   -90,    94,   225,   -90,    70,   -90,
     -90,   105,   -90,    43,   228,   -90,   229,   232,   -90,    33,
     233,    44,   -90,   140,   236,   237,   -90,   175,   240,   241,
     -90,   179,   244,   -90,    20,   245,   -90,    11,   248,   -90,
      11,   249,   -90,    17,   252,   -90,   253,   -90,   168,   256,
     257,    43,    43,   -90,    56,   260,    43,   261,   -90,   -90,
      70,   264,   112,   -90,   -90,   -90,   -90,   -90,   -90,   -90,
     -90,   -90,   -90,   -90,   -90,   -90,   -90,   -90,   -90,   -90,
     -90,   -90,   -90,   -90,   -90,   -90,   -90,   -90,   -90,   -90,
     -90,   -90,   -90,   -90,   -90,   -90,    47,   265,   268,   -90,
     -90,   269,   -90,    94,   -90,   -90,   -90,   -90,   131,   132,
     -90,   -90,   -90,   -90
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -90,   -90,     0,   -90,     5,   -90,    37,   -90,   -90,   -90,
      48,     6,   -39,   -33,   -42,   -32,    55,   -21,   -90,   -89,
     -57,   118,   -50,   150,   -90,   165,   -90,   185,   -51,   -49,
     -44,   138,   -90,    89,   -90,   193,   121,   -90,   -36,    -6
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint16 yytable[] =
{
      18,    86,   104,   170,   121,   119,   122,   129,   128,   130,
     135,   123,   136,   114,    34,     2,    35,   137,   179,   115,
     116,     2,    36,    10,    11,     3,    79,   101,   140,   102,
      11,     9,    37,   101,    38,   102,   107,    98,   108,    40,
      39,    41,    43,    84,    44,    85,    80,    42,   164,    89,
      45,   166,    46,   167,    47,    49,    52,    50,    53,   219,
      48,   104,   257,    51,    54,   175,    81,   147,    55,    58,
      56,    59,   150,   121,   119,   122,    57,    60,   114,   160,
     123,   129,   128,   130,   115,   116,   155,   135,   171,   136,
      61,   161,    62,    98,   137,    64,   201,    65,    63,    67,
      70,    68,    71,    66,    87,   196,    88,    69,    72,   202,
     206,   200,     1,     2,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    73,   162,    74,   163,   212,    13,
      14,    15,    75,   223,    16,   256,   233,   232,   234,   129,
     128,   130,   239,   235,   240,   228,   262,   263,   226,   241,
     199,   229,   230,    76,   185,    77,    94,   213,    95,   182,
      92,    78,    93,    81,   211,   247,   248,    98,   254,   237,
     251,     1,     2,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    99,   165,   100,   163,   191,    13,    14,
      15,   101,   105,   102,   106,   107,   261,   108,   244,   111,
     117,   112,   118,   124,   132,   125,   133,   138,   141,   139,
     142,   145,   148,   146,   149,   153,   158,   154,   159,   168,
     172,   169,   173,   176,   180,   177,   181,   183,   186,   184,
     187,   189,   192,   190,   193,   195,   197,   163,   198,   203,
     209,   204,   210,   214,   215,   163,   163,   216,   218,   163,
     169,   221,   222,   173,   163,   224,   225,   177,   169,   227,
     231,   181,   184,   236,   238,   187,   190,   242,   243,   193,
     163,   245,   246,   198,   163,   250,   252,   204,   253,   255,
     258,   210,   163,   259,   260,   163,   163,   217,   194,   188,
     178,   205,   174,   249,   220
};

#define yypact_value_is_default(yystate) \
  ((yystate) == (-90))

#define yytable_value_is_error(yytable_value) \
  YYID (0)

static const yytype_uint8 yycheck[] =
{
       0,    37,    46,    92,    55,    55,    55,    58,    58,    58,
      61,    55,    61,    52,    16,     4,    18,    61,   107,    52,
      52,     4,    24,    12,    13,     5,    19,    16,    64,    18,
      13,    11,    16,    16,    18,    18,    16,    43,    18,    16,
      24,    18,    16,    16,    18,    18,     0,    24,    84,    16,
      24,    87,    16,    89,    18,    16,    16,    18,    18,    15,
      24,   105,    15,    24,    24,   101,    23,    16,    16,    16,
      18,    18,    16,   124,   124,   124,    24,    24,   117,    79,
     124,   132,   132,   132,   117,   117,    16,   138,    94,   138,
      16,    23,    18,    99,   138,    16,    16,    18,    24,    16,
      16,    18,    18,    24,    16,   141,    18,    24,    24,    16,
      16,   147,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    16,    15,    18,    17,    23,    20,
      21,    22,    24,   177,    25,    23,   187,   187,   187,   190,
     190,   190,   193,   187,   193,   184,    15,    15,   181,   193,
     145,   184,   184,    16,   117,    18,    16,   163,    18,   111,
      16,    24,    18,    23,   158,   201,   202,   173,   210,   190,
     206,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    16,    15,    18,    17,   132,    20,    21,
      22,    16,    16,    18,    18,    16,   253,    18,   198,    16,
      16,    18,    18,    16,    16,    18,    18,    16,    16,    18,
      18,    16,    16,    18,    18,    16,    16,    18,    18,    15,
      15,    17,    17,    15,    15,    17,    17,    15,    15,    17,
      17,    15,    15,    17,    17,    15,    15,    17,    17,    15,
      15,    17,    17,    15,    15,    17,    17,    15,    15,    17,
      17,    15,    15,    17,    17,    15,    15,    17,    17,    15,
      15,    17,    17,    15,    15,    17,    17,    15,    15,    17,
      17,    15,    15,    17,    17,    15,    15,    17,    17,    15,
      15,    17,    17,    15,    15,    17,    17,   169,   138,   124,
     105,   153,    99,   204,   173
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
      45,    47,    16,    18,    16,    18,    61,    62,    65,    16,
      18,    16,    18,    53,    56,    16,    18,    16,    18,    36,
      39,    16,    18,    32,    38,    39,    41,    16,    18,    48,
      51,    54,    55,    56,    16,    18,    42,    43,    48,    54,
      55,    56,    16,    18,    49,    54,    55,    56,    16,    18,
      64,    16,    18,    28,    30,    16,    18,    16,    16,    18,
      16,    57,    59,    16,    18,    16,    37,    40,    16,    18,
      28,    23,    15,    17,    64,    15,    64,    64,    15,    17,
      45,    65,    15,    17,    61,    64,    15,    17,    53,    45,
      15,    17,    36,    15,    17,    32,    15,    17,    51,    15,
      17,    42,    15,    17,    49,    15,    64,    15,    17,    30,
      64,    16,    16,    15,    17,    57,    16,    44,    46,    15,
      17,    37,    23,    65,    15,    15,    15,    47,    15,    15,
      62,    15,    15,    56,    15,    15,    39,    15,    38,    39,
      41,    15,    48,    54,    55,    56,    15,    43,    15,    54,
      55,    56,    15,    15,    28,    15,    15,    64,    64,    59,
      15,    64,    15,    17,    40,    15,    23,    15,    15,    15,
      15,    46,    15,    15
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
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
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
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
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
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       , &(yylsp[(yyi + 1) - (yynrhs)])		       );
      YYFPRINTF (stderr, "\n");
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

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (0, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  YYSIZE_T yysize1;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = 0;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - Assume YYFAIL is not used.  It's too flawed to consider.  See
       <http://lists.gnu.org/archive/html/bison-patches/2009-12/msg00024.html>
       for details.  YYERROR is fine as it does not invoke this
       function.
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                yysize1 = yysize + yytnamerr (0, yytname[yyx]);
                if (! (yysize <= yysize1
                       && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                  return 2;
                yysize = yysize1;
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  yysize1 = yysize + yystrlen (yyformat);
  if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
    return 2;
  yysize = yysize1;

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
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

/* Line 1391 of yacc.c  */
#line 185 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };

/* Line 1391 of yacc.c  */
#line 1496 "y.tab.c"
	break;
      case 29: /* "geometrycollection" */

/* Line 1391 of yacc.c  */
#line 186 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };

/* Line 1391 of yacc.c  */
#line 1505 "y.tab.c"
	break;
      case 31: /* "multisurface" */

/* Line 1391 of yacc.c  */
#line 193 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };

/* Line 1391 of yacc.c  */
#line 1514 "y.tab.c"
	break;
      case 32: /* "surface_list" */

/* Line 1391 of yacc.c  */
#line 172 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };

/* Line 1391 of yacc.c  */
#line 1523 "y.tab.c"
	break;
      case 33: /* "tin" */

/* Line 1391 of yacc.c  */
#line 200 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };

/* Line 1391 of yacc.c  */
#line 1532 "y.tab.c"
	break;
      case 34: /* "polyhedralsurface" */

/* Line 1391 of yacc.c  */
#line 199 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };

/* Line 1391 of yacc.c  */
#line 1541 "y.tab.c"
	break;
      case 35: /* "multipolygon" */

/* Line 1391 of yacc.c  */
#line 192 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };

/* Line 1391 of yacc.c  */
#line 1550 "y.tab.c"
	break;
      case 36: /* "polygon_list" */

/* Line 1391 of yacc.c  */
#line 173 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };

/* Line 1391 of yacc.c  */
#line 1559 "y.tab.c"
	break;
      case 37: /* "patch_list" */

/* Line 1391 of yacc.c  */
#line 174 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };

/* Line 1391 of yacc.c  */
#line 1568 "y.tab.c"
	break;
      case 38: /* "polygon" */

/* Line 1391 of yacc.c  */
#line 196 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };

/* Line 1391 of yacc.c  */
#line 1577 "y.tab.c"
	break;
      case 39: /* "polygon_untagged" */

/* Line 1391 of yacc.c  */
#line 198 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };

/* Line 1391 of yacc.c  */
#line 1586 "y.tab.c"
	break;
      case 40: /* "patch" */

/* Line 1391 of yacc.c  */
#line 197 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };

/* Line 1391 of yacc.c  */
#line 1595 "y.tab.c"
	break;
      case 41: /* "curvepolygon" */

/* Line 1391 of yacc.c  */
#line 183 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };

/* Line 1391 of yacc.c  */
#line 1604 "y.tab.c"
	break;
      case 42: /* "curvering_list" */

/* Line 1391 of yacc.c  */
#line 170 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };

/* Line 1391 of yacc.c  */
#line 1613 "y.tab.c"
	break;
      case 43: /* "curvering" */

/* Line 1391 of yacc.c  */
#line 184 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };

/* Line 1391 of yacc.c  */
#line 1622 "y.tab.c"
	break;
      case 44: /* "patchring_list" */

/* Line 1391 of yacc.c  */
#line 180 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };

/* Line 1391 of yacc.c  */
#line 1631 "y.tab.c"
	break;
      case 45: /* "ring_list" */

/* Line 1391 of yacc.c  */
#line 179 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };

/* Line 1391 of yacc.c  */
#line 1640 "y.tab.c"
	break;
      case 46: /* "patchring" */

/* Line 1391 of yacc.c  */
#line 169 "lwin_wkt_parse.y"
	{ ptarray_free((yyvaluep->ptarrayvalue)); };

/* Line 1391 of yacc.c  */
#line 1649 "y.tab.c"
	break;
      case 47: /* "ring" */

/* Line 1391 of yacc.c  */
#line 168 "lwin_wkt_parse.y"
	{ ptarray_free((yyvaluep->ptarrayvalue)); };

/* Line 1391 of yacc.c  */
#line 1658 "y.tab.c"
	break;
      case 48: /* "compoundcurve" */

/* Line 1391 of yacc.c  */
#line 182 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };

/* Line 1391 of yacc.c  */
#line 1667 "y.tab.c"
	break;
      case 49: /* "compound_list" */

/* Line 1391 of yacc.c  */
#line 178 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };

/* Line 1391 of yacc.c  */
#line 1676 "y.tab.c"
	break;
      case 50: /* "multicurve" */

/* Line 1391 of yacc.c  */
#line 189 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };

/* Line 1391 of yacc.c  */
#line 1685 "y.tab.c"
	break;
      case 51: /* "curve_list" */

/* Line 1391 of yacc.c  */
#line 177 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };

/* Line 1391 of yacc.c  */
#line 1694 "y.tab.c"
	break;
      case 52: /* "multilinestring" */

/* Line 1391 of yacc.c  */
#line 190 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };

/* Line 1391 of yacc.c  */
#line 1703 "y.tab.c"
	break;
      case 53: /* "linestring_list" */

/* Line 1391 of yacc.c  */
#line 176 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };

/* Line 1391 of yacc.c  */
#line 1712 "y.tab.c"
	break;
      case 54: /* "circularstring" */

/* Line 1391 of yacc.c  */
#line 181 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };

/* Line 1391 of yacc.c  */
#line 1721 "y.tab.c"
	break;
      case 55: /* "linestring" */

/* Line 1391 of yacc.c  */
#line 187 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };

/* Line 1391 of yacc.c  */
#line 1730 "y.tab.c"
	break;
      case 56: /* "linestring_untagged" */

/* Line 1391 of yacc.c  */
#line 188 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };

/* Line 1391 of yacc.c  */
#line 1739 "y.tab.c"
	break;
      case 57: /* "triangle_list" */

/* Line 1391 of yacc.c  */
#line 171 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };

/* Line 1391 of yacc.c  */
#line 1748 "y.tab.c"
	break;
      case 58: /* "triangle" */

/* Line 1391 of yacc.c  */
#line 201 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };

/* Line 1391 of yacc.c  */
#line 1757 "y.tab.c"
	break;
      case 59: /* "triangle_untagged" */

/* Line 1391 of yacc.c  */
#line 202 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };

/* Line 1391 of yacc.c  */
#line 1766 "y.tab.c"
	break;
      case 60: /* "multipoint" */

/* Line 1391 of yacc.c  */
#line 191 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };

/* Line 1391 of yacc.c  */
#line 1775 "y.tab.c"
	break;
      case 61: /* "point_list" */

/* Line 1391 of yacc.c  */
#line 175 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };

/* Line 1391 of yacc.c  */
#line 1784 "y.tab.c"
	break;
      case 62: /* "point_untagged" */

/* Line 1391 of yacc.c  */
#line 195 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };

/* Line 1391 of yacc.c  */
#line 1793 "y.tab.c"
	break;
      case 63: /* "point" */

/* Line 1391 of yacc.c  */
#line 194 "lwin_wkt_parse.y"
	{ lwgeom_free((yyvaluep->geometryvalue)); };

/* Line 1391 of yacc.c  */
#line 1802 "y.tab.c"
	break;
      case 64: /* "ptarray" */

/* Line 1391 of yacc.c  */
#line 167 "lwin_wkt_parse.y"
	{ ptarray_free((yyvaluep->ptarrayvalue)); };

/* Line 1391 of yacc.c  */
#line 1811 "y.tab.c"
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


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Location data for the lookahead symbol.  */
YYLTYPE yylloc;

/* Number of syntax errors so far.  */
int yynerrs;


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
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.
       `yyls': related to locations.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    /* The location stack.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls;
    YYLTYPE *yylsp;

    /* The locations where the error started and ended.  */
    YYLTYPE yyerror_range[3];

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yyls = yylsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

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
  yylloc.first_column = yylloc.last_column = 1;
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
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
	YYSTACK_RELOCATE (yyls_alloc, yyls);
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

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
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
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
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

/* Line 1806 of yacc.c  */
#line 208 "lwin_wkt_parse.y"
    { wkt_parser_geometry_new((yyvsp[(1) - (1)].geometryvalue), SRID_UNKNOWN); WKT_ERROR(); }
    break;

  case 3:

/* Line 1806 of yacc.c  */
#line 210 "lwin_wkt_parse.y"
    { wkt_parser_geometry_new((yyvsp[(3) - (3)].geometryvalue), (yyvsp[(1) - (3)].integervalue)); WKT_ERROR(); }
    break;

  case 4:

/* Line 1806 of yacc.c  */
#line 213 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 5:

/* Line 1806 of yacc.c  */
#line 214 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 6:

/* Line 1806 of yacc.c  */
#line 215 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 7:

/* Line 1806 of yacc.c  */
#line 216 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 8:

/* Line 1806 of yacc.c  */
#line 217 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 9:

/* Line 1806 of yacc.c  */
#line 218 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 10:

/* Line 1806 of yacc.c  */
#line 219 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 11:

/* Line 1806 of yacc.c  */
#line 220 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 12:

/* Line 1806 of yacc.c  */
#line 221 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 13:

/* Line 1806 of yacc.c  */
#line 222 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 14:

/* Line 1806 of yacc.c  */
#line 223 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 15:

/* Line 1806 of yacc.c  */
#line 224 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 16:

/* Line 1806 of yacc.c  */
#line 225 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 17:

/* Line 1806 of yacc.c  */
#line 226 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 18:

/* Line 1806 of yacc.c  */
#line 227 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 19:

/* Line 1806 of yacc.c  */
#line 231 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(COLLECTIONTYPE, (yyvsp[(3) - (4)].geometryvalue), NULL); WKT_ERROR(); }
    break;

  case 20:

/* Line 1806 of yacc.c  */
#line 233 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(COLLECTIONTYPE, (yyvsp[(4) - (5)].geometryvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 21:

/* Line 1806 of yacc.c  */
#line 235 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(COLLECTIONTYPE, NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 22:

/* Line 1806 of yacc.c  */
#line 237 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(COLLECTIONTYPE, NULL, NULL); WKT_ERROR(); }
    break;

  case 23:

/* Line 1806 of yacc.c  */
#line 241 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 24:

/* Line 1806 of yacc.c  */
#line 243 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 25:

/* Line 1806 of yacc.c  */
#line 247 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTISURFACETYPE, (yyvsp[(3) - (4)].geometryvalue), NULL); WKT_ERROR(); }
    break;

  case 26:

/* Line 1806 of yacc.c  */
#line 249 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTISURFACETYPE, (yyvsp[(4) - (5)].geometryvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 27:

/* Line 1806 of yacc.c  */
#line 251 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTISURFACETYPE, NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 28:

/* Line 1806 of yacc.c  */
#line 253 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTISURFACETYPE, NULL, NULL); WKT_ERROR(); }
    break;

  case 29:

/* Line 1806 of yacc.c  */
#line 257 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 30:

/* Line 1806 of yacc.c  */
#line 259 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 31:

/* Line 1806 of yacc.c  */
#line 261 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 32:

/* Line 1806 of yacc.c  */
#line 263 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 33:

/* Line 1806 of yacc.c  */
#line 265 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 34:

/* Line 1806 of yacc.c  */
#line 267 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 35:

/* Line 1806 of yacc.c  */
#line 271 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(TINTYPE, (yyvsp[(3) - (4)].geometryvalue), NULL); WKT_ERROR(); }
    break;

  case 36:

/* Line 1806 of yacc.c  */
#line 273 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(TINTYPE, (yyvsp[(4) - (5)].geometryvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 37:

/* Line 1806 of yacc.c  */
#line 275 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(TINTYPE, NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 38:

/* Line 1806 of yacc.c  */
#line 277 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(TINTYPE, NULL, NULL); WKT_ERROR(); }
    break;

  case 39:

/* Line 1806 of yacc.c  */
#line 281 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(POLYHEDRALSURFACETYPE, (yyvsp[(3) - (4)].geometryvalue), NULL); WKT_ERROR(); }
    break;

  case 40:

/* Line 1806 of yacc.c  */
#line 283 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(POLYHEDRALSURFACETYPE, (yyvsp[(4) - (5)].geometryvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 41:

/* Line 1806 of yacc.c  */
#line 285 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(POLYHEDRALSURFACETYPE, NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 42:

/* Line 1806 of yacc.c  */
#line 287 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(POLYHEDRALSURFACETYPE, NULL, NULL); WKT_ERROR(); }
    break;

  case 43:

/* Line 1806 of yacc.c  */
#line 291 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTIPOLYGONTYPE, (yyvsp[(3) - (4)].geometryvalue), NULL); WKT_ERROR(); }
    break;

  case 44:

/* Line 1806 of yacc.c  */
#line 293 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTIPOLYGONTYPE, (yyvsp[(4) - (5)].geometryvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 45:

/* Line 1806 of yacc.c  */
#line 295 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTIPOLYGONTYPE, NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 46:

/* Line 1806 of yacc.c  */
#line 297 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTIPOLYGONTYPE, NULL, NULL); WKT_ERROR(); }
    break;

  case 47:

/* Line 1806 of yacc.c  */
#line 301 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 48:

/* Line 1806 of yacc.c  */
#line 303 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 49:

/* Line 1806 of yacc.c  */
#line 307 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 50:

/* Line 1806 of yacc.c  */
#line 309 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 51:

/* Line 1806 of yacc.c  */
#line 313 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_polygon_finalize((yyvsp[(3) - (4)].geometryvalue), NULL); WKT_ERROR(); }
    break;

  case 52:

/* Line 1806 of yacc.c  */
#line 315 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_polygon_finalize((yyvsp[(4) - (5)].geometryvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 53:

/* Line 1806 of yacc.c  */
#line 317 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_polygon_finalize(NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 54:

/* Line 1806 of yacc.c  */
#line 319 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_polygon_finalize(NULL, NULL); WKT_ERROR(); }
    break;

  case 55:

/* Line 1806 of yacc.c  */
#line 323 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(2) - (3)].geometryvalue); }
    break;

  case 56:

/* Line 1806 of yacc.c  */
#line 325 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_polygon_finalize(NULL, NULL); WKT_ERROR(); }
    break;

  case 57:

/* Line 1806 of yacc.c  */
#line 328 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(2) - (3)].geometryvalue); }
    break;

  case 58:

/* Line 1806 of yacc.c  */
#line 332 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_curvepolygon_finalize((yyvsp[(3) - (4)].geometryvalue), NULL); WKT_ERROR(); }
    break;

  case 59:

/* Line 1806 of yacc.c  */
#line 334 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_curvepolygon_finalize((yyvsp[(4) - (5)].geometryvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 60:

/* Line 1806 of yacc.c  */
#line 336 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_curvepolygon_finalize(NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 61:

/* Line 1806 of yacc.c  */
#line 338 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_curvepolygon_finalize(NULL, NULL); WKT_ERROR(); }
    break;

  case 62:

/* Line 1806 of yacc.c  */
#line 342 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_curvepolygon_add_ring((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 63:

/* Line 1806 of yacc.c  */
#line 344 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_curvepolygon_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 64:

/* Line 1806 of yacc.c  */
#line 347 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 65:

/* Line 1806 of yacc.c  */
#line 348 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 66:

/* Line 1806 of yacc.c  */
#line 349 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 67:

/* Line 1806 of yacc.c  */
#line 350 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = (yyvsp[(1) - (1)].geometryvalue); }
    break;

  case 68:

/* Line 1806 of yacc.c  */
#line 354 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_polygon_add_ring((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].ptarrayvalue),'Z'); WKT_ERROR(); }
    break;

  case 69:

/* Line 1806 of yacc.c  */
#line 356 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_polygon_new((yyvsp[(1) - (1)].ptarrayvalue),'Z'); WKT_ERROR(); }
    break;

  case 70:

/* Line 1806 of yacc.c  */
#line 360 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_polygon_add_ring((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].ptarrayvalue),'2'); WKT_ERROR(); }
    break;

  case 71:

/* Line 1806 of yacc.c  */
#line 362 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_polygon_new((yyvsp[(1) - (1)].ptarrayvalue),'2'); WKT_ERROR(); }
    break;

  case 72:

/* Line 1806 of yacc.c  */
#line 365 "lwin_wkt_parse.y"
    { (yyval.ptarrayvalue) = (yyvsp[(2) - (3)].ptarrayvalue); }
    break;

  case 73:

/* Line 1806 of yacc.c  */
#line 368 "lwin_wkt_parse.y"
    { (yyval.ptarrayvalue) = (yyvsp[(2) - (3)].ptarrayvalue); }
    break;

  case 74:

/* Line 1806 of yacc.c  */
#line 372 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(COMPOUNDTYPE, (yyvsp[(3) - (4)].geometryvalue), NULL); WKT_ERROR(); }
    break;

  case 75:

/* Line 1806 of yacc.c  */
#line 374 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(COMPOUNDTYPE, (yyvsp[(4) - (5)].geometryvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 76:

/* Line 1806 of yacc.c  */
#line 376 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(COMPOUNDTYPE, NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 77:

/* Line 1806 of yacc.c  */
#line 378 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(COMPOUNDTYPE, NULL, NULL); WKT_ERROR(); }
    break;

  case 78:

/* Line 1806 of yacc.c  */
#line 382 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_compound_add_geom((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 79:

/* Line 1806 of yacc.c  */
#line 384 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_compound_add_geom((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 80:

/* Line 1806 of yacc.c  */
#line 386 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_compound_add_geom((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 81:

/* Line 1806 of yacc.c  */
#line 388 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 82:

/* Line 1806 of yacc.c  */
#line 390 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 83:

/* Line 1806 of yacc.c  */
#line 392 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 84:

/* Line 1806 of yacc.c  */
#line 396 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTICURVETYPE, (yyvsp[(3) - (4)].geometryvalue), NULL); WKT_ERROR(); }
    break;

  case 85:

/* Line 1806 of yacc.c  */
#line 398 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTICURVETYPE, (yyvsp[(4) - (5)].geometryvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 86:

/* Line 1806 of yacc.c  */
#line 400 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTICURVETYPE, NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 87:

/* Line 1806 of yacc.c  */
#line 402 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTICURVETYPE, NULL, NULL); WKT_ERROR(); }
    break;

  case 88:

/* Line 1806 of yacc.c  */
#line 406 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 89:

/* Line 1806 of yacc.c  */
#line 408 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 90:

/* Line 1806 of yacc.c  */
#line 410 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 91:

/* Line 1806 of yacc.c  */
#line 412 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 92:

/* Line 1806 of yacc.c  */
#line 414 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 93:

/* Line 1806 of yacc.c  */
#line 416 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 94:

/* Line 1806 of yacc.c  */
#line 418 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 95:

/* Line 1806 of yacc.c  */
#line 420 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 96:

/* Line 1806 of yacc.c  */
#line 424 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTILINETYPE, (yyvsp[(3) - (4)].geometryvalue), NULL); WKT_ERROR(); }
    break;

  case 97:

/* Line 1806 of yacc.c  */
#line 426 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTILINETYPE, (yyvsp[(4) - (5)].geometryvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 98:

/* Line 1806 of yacc.c  */
#line 428 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTILINETYPE, NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 99:

/* Line 1806 of yacc.c  */
#line 430 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTILINETYPE, NULL, NULL); WKT_ERROR(); }
    break;

  case 100:

/* Line 1806 of yacc.c  */
#line 434 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 101:

/* Line 1806 of yacc.c  */
#line 436 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 102:

/* Line 1806 of yacc.c  */
#line 440 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_circularstring_new((yyvsp[(3) - (4)].ptarrayvalue), NULL); WKT_ERROR(); }
    break;

  case 103:

/* Line 1806 of yacc.c  */
#line 442 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_circularstring_new((yyvsp[(4) - (5)].ptarrayvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 104:

/* Line 1806 of yacc.c  */
#line 444 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_circularstring_new(NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 105:

/* Line 1806 of yacc.c  */
#line 446 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_circularstring_new(NULL, NULL); WKT_ERROR(); }
    break;

  case 106:

/* Line 1806 of yacc.c  */
#line 450 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_linestring_new((yyvsp[(3) - (4)].ptarrayvalue), NULL); WKT_ERROR(); }
    break;

  case 107:

/* Line 1806 of yacc.c  */
#line 452 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_linestring_new((yyvsp[(4) - (5)].ptarrayvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 108:

/* Line 1806 of yacc.c  */
#line 454 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_linestring_new(NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 109:

/* Line 1806 of yacc.c  */
#line 456 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_linestring_new(NULL, NULL); WKT_ERROR(); }
    break;

  case 110:

/* Line 1806 of yacc.c  */
#line 460 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_linestring_new((yyvsp[(2) - (3)].ptarrayvalue), NULL); WKT_ERROR(); }
    break;

  case 111:

/* Line 1806 of yacc.c  */
#line 462 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_linestring_new(NULL, NULL); WKT_ERROR(); }
    break;

  case 112:

/* Line 1806 of yacc.c  */
#line 466 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 113:

/* Line 1806 of yacc.c  */
#line 468 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 114:

/* Line 1806 of yacc.c  */
#line 472 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_triangle_new((yyvsp[(4) - (6)].ptarrayvalue), NULL); WKT_ERROR(); }
    break;

  case 115:

/* Line 1806 of yacc.c  */
#line 474 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_triangle_new((yyvsp[(5) - (7)].ptarrayvalue), (yyvsp[(2) - (7)].stringvalue)); WKT_ERROR(); }
    break;

  case 116:

/* Line 1806 of yacc.c  */
#line 476 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_triangle_new(NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 117:

/* Line 1806 of yacc.c  */
#line 478 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_triangle_new(NULL, NULL); WKT_ERROR(); }
    break;

  case 118:

/* Line 1806 of yacc.c  */
#line 482 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_triangle_new((yyvsp[(3) - (5)].ptarrayvalue), NULL); WKT_ERROR(); }
    break;

  case 119:

/* Line 1806 of yacc.c  */
#line 486 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTIPOINTTYPE, (yyvsp[(3) - (4)].geometryvalue), NULL); WKT_ERROR(); }
    break;

  case 120:

/* Line 1806 of yacc.c  */
#line 488 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTIPOINTTYPE, (yyvsp[(4) - (5)].geometryvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 121:

/* Line 1806 of yacc.c  */
#line 490 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTIPOINTTYPE, NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 122:

/* Line 1806 of yacc.c  */
#line 492 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTIPOINTTYPE, NULL, NULL); WKT_ERROR(); }
    break;

  case 123:

/* Line 1806 of yacc.c  */
#line 496 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[(1) - (3)].geometryvalue),(yyvsp[(3) - (3)].geometryvalue)); WKT_ERROR(); }
    break;

  case 124:

/* Line 1806 of yacc.c  */
#line 498 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[(1) - (1)].geometryvalue)); WKT_ERROR(); }
    break;

  case 125:

/* Line 1806 of yacc.c  */
#line 502 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_point_new(wkt_parser_ptarray_new((yyvsp[(1) - (1)].coordinatevalue)),NULL); WKT_ERROR(); }
    break;

  case 126:

/* Line 1806 of yacc.c  */
#line 504 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_point_new(wkt_parser_ptarray_new((yyvsp[(2) - (3)].coordinatevalue)),NULL); WKT_ERROR(); }
    break;

  case 127:

/* Line 1806 of yacc.c  */
#line 506 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_point_new(NULL, NULL); WKT_ERROR(); }
    break;

  case 128:

/* Line 1806 of yacc.c  */
#line 510 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_point_new((yyvsp[(3) - (4)].ptarrayvalue), NULL); WKT_ERROR(); }
    break;

  case 129:

/* Line 1806 of yacc.c  */
#line 512 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_point_new((yyvsp[(4) - (5)].ptarrayvalue), (yyvsp[(2) - (5)].stringvalue)); WKT_ERROR(); }
    break;

  case 130:

/* Line 1806 of yacc.c  */
#line 514 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_point_new(NULL, (yyvsp[(2) - (3)].stringvalue)); WKT_ERROR(); }
    break;

  case 131:

/* Line 1806 of yacc.c  */
#line 516 "lwin_wkt_parse.y"
    { (yyval.geometryvalue) = wkt_parser_point_new(NULL,NULL); WKT_ERROR(); }
    break;

  case 132:

/* Line 1806 of yacc.c  */
#line 520 "lwin_wkt_parse.y"
    { (yyval.ptarrayvalue) = wkt_parser_ptarray_add_coord((yyvsp[(1) - (3)].ptarrayvalue), (yyvsp[(3) - (3)].coordinatevalue)); WKT_ERROR(); }
    break;

  case 133:

/* Line 1806 of yacc.c  */
#line 522 "lwin_wkt_parse.y"
    { (yyval.ptarrayvalue) = wkt_parser_ptarray_new((yyvsp[(1) - (1)].coordinatevalue)); WKT_ERROR(); }
    break;

  case 134:

/* Line 1806 of yacc.c  */
#line 526 "lwin_wkt_parse.y"
    { (yyval.coordinatevalue) = wkt_parser_coord_2((yyvsp[(1) - (2)].doublevalue), (yyvsp[(2) - (2)].doublevalue)); WKT_ERROR(); }
    break;

  case 135:

/* Line 1806 of yacc.c  */
#line 528 "lwin_wkt_parse.y"
    { (yyval.coordinatevalue) = wkt_parser_coord_3((yyvsp[(1) - (3)].doublevalue), (yyvsp[(2) - (3)].doublevalue), (yyvsp[(3) - (3)].doublevalue)); WKT_ERROR(); }
    break;

  case 136:

/* Line 1806 of yacc.c  */
#line 530 "lwin_wkt_parse.y"
    { (yyval.coordinatevalue) = wkt_parser_coord_4((yyvsp[(1) - (4)].doublevalue), (yyvsp[(2) - (4)].doublevalue), (yyvsp[(3) - (4)].doublevalue), (yyvsp[(4) - (4)].doublevalue)); WKT_ERROR(); }
    break;



/* Line 1806 of yacc.c  */
#line 3086 "y.tab.c"
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
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
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }

  yyerror_range[1] = yylloc;

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
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

  /* Else will try to reuse lookahead token after shifting the error
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

  yyerror_range[1] = yylsp[1-yylen];
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
      if (!yypact_value_is_default (yyn))
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

      yyerror_range[1] = *yylsp;
      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, yylsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;

  yyerror_range[2] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the lookahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, yyerror_range, 2);
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

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, &yylloc);
    }
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



/* Line 2067 of yacc.c  */
#line 532 "lwin_wkt_parse.y"



