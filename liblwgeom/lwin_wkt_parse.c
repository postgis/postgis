/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 1 "liblwgeom/lwin_wkt_parse.y"


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
void wkt_yyerror(__attribute__((__unused__)) const char *str)
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
int lwgeom_parse_wkt(LWGEOM_PARSER_RESULT *parser_result, const char *wktstr, int parser_check_flags)
{
	int parse_rv = 0;

	/* Clean up our global parser result. */
	lwgeom_parser_result_init(&global_parser_result);
	/* Work-around possible bug in GNU Bison 3.0.2 resulting in wkt_yylloc
	 * members not being initialized on yyparse() as documented here:
	 * https://www.gnu.org/software/bison/manual/html_node/Location-Type.html
	 * See discussion here:
	 * http://lists.osgeo.org/pipermail/postgis-devel/2014-September/024506.html
	 */
	wkt_yylloc.last_column = wkt_yylloc.last_line = \
		wkt_yylloc.first_column = wkt_yylloc.first_line = 1;

	/* Set the input text string, and parse checks. */
	global_parser_result.wkinput = wktstr;
	global_parser_result.parser_check_flags = parser_check_flags;

	wkt_lexer_init(wktstr); /* Lexer ready */
	parse_rv = wkt_yyparse(); /* Run the parse */
	LWDEBUGF(4,"wkt_yyparse returned %d", parse_rv);
	wkt_lexer_close(); /* Clean up lexer */

	/* A non-zero parser return is an error. */
	if ( parse_rv || global_parser_result.errcode )
	{
		if( ! global_parser_result.errcode )
		{
			global_parser_result.errcode = PARSER_ERROR_OTHER;
			global_parser_result.message = parser_error_messages[PARSER_ERROR_OTHER];
			global_parser_result.errlocation = wkt_yylloc.last_column;
		}
		else if (global_parser_result.geom)
		{
			lwgeom_free(global_parser_result.geom);
			global_parser_result.geom = NULL;
		}

		LWDEBUGF(5, "error returned by wkt_yyparse() @ %d: [%d] '%s'",
		            global_parser_result.errlocation,
		            global_parser_result.errcode,
		            global_parser_result.message);

		/* Copy the global values into the return pointer */
		*parser_result = global_parser_result;
		wkt_yylex_destroy();
		return LW_FAILURE;
	}

	/* Copy the global value into the return pointer */
	*parser_result = global_parser_result;
	wkt_yylex_destroy();
	return LW_SUCCESS;
}

#define SET_PARSER_ERROR(parser_errcode) \
    do { \
        global_parser_result.errcode = (parser_errcode); \
        global_parser_result.message = parser_error_messages[(parser_errcode)]; \
        global_parser_result.errlocation = wkt_yylloc.last_column; \
    } while(0)

#define WKT_ERROR() { if ( global_parser_result.errcode != 0 ) { YYERROR; } }

struct WKT_NURBS_CONTROLPOINTS
{
	POINTARRAY *points;
	POINTARRAY *weights;
};

static WKT_NURBS_CONTROLPOINTS *
wkt_parser_nurbs_controlpoints_new(POINT point, double weight)
{
	WKT_NURBS_CONTROLPOINTS *controlpoints;

	if (!isfinite(weight) || weight <= 0.0)
	{
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;
	}

	controlpoints = lwalloc(sizeof(WKT_NURBS_CONTROLPOINTS));
	controlpoints->points = wkt_parser_ptarray_new(point);
	controlpoints->weights = wkt_parser_ptarray_new(wkt_parser_coord_2(weight, 0));
	return controlpoints;
}

static void
wkt_parser_nurbs_controlpoints_free(WKT_NURBS_CONTROLPOINTS *controlpoints)
{
	if (!controlpoints)
		return;
	if (controlpoints->points)
		ptarray_free(controlpoints->points);
	if (controlpoints->weights)
		ptarray_free(controlpoints->weights);
	lwfree(controlpoints);
}

static WKT_NURBS_CONTROLPOINTS *
wkt_parser_nurbs_controlpoints_add(WKT_NURBS_CONTROLPOINTS *controlpoints, WKT_NURBS_CONTROLPOINTS *next)
{
	POINT4D point;
	POINT4D weight;

	if (!controlpoints || !next)
	{
		wkt_parser_nurbs_controlpoints_free(controlpoints);
		wkt_parser_nurbs_controlpoints_free(next);
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;
	}

	if (FLAGS_GET_ZM(controlpoints->points->flags) != FLAGS_GET_ZM(next->points->flags))
	{
		wkt_parser_nurbs_controlpoints_free(controlpoints);
		wkt_parser_nurbs_controlpoints_free(next);
		SET_PARSER_ERROR(PARSER_ERROR_MIXDIMS);
		return NULL;
	}

	getPoint4d_p(next->points, 0, &point);
	getPoint4d_p(next->weights, 0, &weight);
	ptarray_append_point(controlpoints->points, &point, LW_TRUE);
	ptarray_append_point(controlpoints->weights, &weight, LW_TRUE);
	wkt_parser_nurbs_controlpoints_free(next);
	return controlpoints;
}

static POINTARRAY *
wkt_parser_knot_list_add_repeated(POINTARRAY *knots, double value, double multiplicity)
{
	uint32_t repeat_count;

	if (!isfinite(value) || !isfinite(multiplicity) || multiplicity < 1.0 ||
	    fabs(multiplicity - round(multiplicity)) >= 1e-10)
	{
		if (knots)
			ptarray_free(knots);
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;
	}

	repeat_count = (uint32_t)round(multiplicity);
	for (uint32_t i = 0; i < repeat_count; i++)
	{
		POINT knot = wkt_parser_coord_2(value, 0);
		knots = knots ? wkt_parser_ptarray_add_coord(knots, knot) : wkt_parser_ptarray_new(knot);
		if (!knots)
			return NULL;
	}
	return knots;
}


#line 275 "liblwgeom/lwin_wkt_parse.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "lwin_wkt_parse.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_POINT_TOK = 3,                  /* POINT_TOK  */
  YYSYMBOL_LINESTRING_TOK = 4,             /* LINESTRING_TOK  */
  YYSYMBOL_POLYGON_TOK = 5,                /* POLYGON_TOK  */
  YYSYMBOL_MPOINT_TOK = 6,                 /* MPOINT_TOK  */
  YYSYMBOL_MLINESTRING_TOK = 7,            /* MLINESTRING_TOK  */
  YYSYMBOL_MPOLYGON_TOK = 8,               /* MPOLYGON_TOK  */
  YYSYMBOL_MSURFACE_TOK = 9,               /* MSURFACE_TOK  */
  YYSYMBOL_MCURVE_TOK = 10,                /* MCURVE_TOK  */
  YYSYMBOL_CURVEPOLYGON_TOK = 11,          /* CURVEPOLYGON_TOK  */
  YYSYMBOL_COMPOUNDCURVE_TOK = 12,         /* COMPOUNDCURVE_TOK  */
  YYSYMBOL_CIRCULARSTRING_TOK = 13,        /* CIRCULARSTRING_TOK  */
  YYSYMBOL_COLLECTION_TOK = 14,            /* COLLECTION_TOK  */
  YYSYMBOL_RBRACKET_TOK = 15,              /* RBRACKET_TOK  */
  YYSYMBOL_LBRACKET_TOK = 16,              /* LBRACKET_TOK  */
  YYSYMBOL_COMMA_TOK = 17,                 /* COMMA_TOK  */
  YYSYMBOL_EMPTY_TOK = 18,                 /* EMPTY_TOK  */
  YYSYMBOL_SEMICOLON_TOK = 19,             /* SEMICOLON_TOK  */
  YYSYMBOL_TRIANGLE_TOK = 20,              /* TRIANGLE_TOK  */
  YYSYMBOL_TIN_TOK = 21,                   /* TIN_TOK  */
  YYSYMBOL_POLYHEDRALSURFACE_TOK = 22,     /* POLYHEDRALSURFACE_TOK  */
  YYSYMBOL_NURBSCURVE_TOK = 23,            /* NURBSCURVE_TOK  */
  YYSYMBOL_DEGREE_TOK = 24,                /* DEGREE_TOK  */
  YYSYMBOL_CONTROLPOINTS_TOK = 25,         /* CONTROLPOINTS_TOK  */
  YYSYMBOL_KNOTS_TOK = 26,                 /* KNOTS_TOK  */
  YYSYMBOL_NURBSPOINT_TOK = 27,            /* NURBSPOINT_TOK  */
  YYSYMBOL_WEIGHTEDPOINT_TOK = 28,         /* WEIGHTEDPOINT_TOK  */
  YYSYMBOL_WEIGHT_TOK = 29,                /* WEIGHT_TOK  */
  YYSYMBOL_KNOT_TOK = 30,                  /* KNOT_TOK  */
  YYSYMBOL_DOUBLE_TOK = 31,                /* DOUBLE_TOK  */
  YYSYMBOL_DIMENSIONALITY_TOK = 32,        /* DIMENSIONALITY_TOK  */
  YYSYMBOL_SRID_TOK = 33,                  /* SRID_TOK  */
  YYSYMBOL_YYACCEPT = 34,                  /* $accept  */
  YYSYMBOL_geometry = 35,                  /* geometry  */
  YYSYMBOL_geometry_no_srid = 36,          /* geometry_no_srid  */
  YYSYMBOL_geometrycollection = 37,        /* geometrycollection  */
  YYSYMBOL_geometry_list = 38,             /* geometry_list  */
  YYSYMBOL_multisurface = 39,              /* multisurface  */
  YYSYMBOL_surface_list = 40,              /* surface_list  */
  YYSYMBOL_tin = 41,                       /* tin  */
  YYSYMBOL_polyhedralsurface = 42,         /* polyhedralsurface  */
  YYSYMBOL_multipolygon = 43,              /* multipolygon  */
  YYSYMBOL_polygon_list = 44,              /* polygon_list  */
  YYSYMBOL_patch_list = 45,                /* patch_list  */
  YYSYMBOL_polygon = 46,                   /* polygon  */
  YYSYMBOL_polygon_untagged = 47,          /* polygon_untagged  */
  YYSYMBOL_patch = 48,                     /* patch  */
  YYSYMBOL_curvepolygon = 49,              /* curvepolygon  */
  YYSYMBOL_curvering_list = 50,            /* curvering_list  */
  YYSYMBOL_curvering = 51,                 /* curvering  */
  YYSYMBOL_patchring_list = 52,            /* patchring_list  */
  YYSYMBOL_ring_list = 53,                 /* ring_list  */
  YYSYMBOL_patchring = 54,                 /* patchring  */
  YYSYMBOL_ring = 55,                      /* ring  */
  YYSYMBOL_compoundcurve = 56,             /* compoundcurve  */
  YYSYMBOL_compound_list = 57,             /* compound_list  */
  YYSYMBOL_multicurve = 58,                /* multicurve  */
  YYSYMBOL_curve_list = 59,                /* curve_list  */
  YYSYMBOL_multilinestring = 60,           /* multilinestring  */
  YYSYMBOL_linestring_list = 61,           /* linestring_list  */
  YYSYMBOL_circularstring = 62,            /* circularstring  */
  YYSYMBOL_linestring = 63,                /* linestring  */
  YYSYMBOL_linestring_untagged = 64,       /* linestring_untagged  */
  YYSYMBOL_triangle_list = 65,             /* triangle_list  */
  YYSYMBOL_triangle = 66,                  /* triangle  */
  YYSYMBOL_triangle_untagged = 67,         /* triangle_untagged  */
  YYSYMBOL_multipoint = 68,                /* multipoint  */
  YYSYMBOL_point_list = 69,                /* point_list  */
  YYSYMBOL_point_untagged = 70,            /* point_untagged  */
  YYSYMBOL_point = 71,                     /* point  */
  YYSYMBOL_ptarray = 72,                   /* ptarray  */
  YYSYMBOL_coordinate = 73,                /* coordinate  */
  YYSYMBOL_opt_dimensionality = 74,        /* opt_dimensionality  */
  YYSYMBOL_nurbscurve = 75,                /* nurbscurve  */
  YYSYMBOL_iso_controlpoint = 76,          /* iso_controlpoint  */
  YYSYMBOL_iso_controlpoint_list = 77,     /* iso_controlpoint_list  */
  YYSYMBOL_iso_knot_list = 78,             /* iso_knot_list  */
  YYSYMBOL_weight_list = 79,               /* weight_list  */
  YYSYMBOL_knot_list = 80                  /* knot_list  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int16 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
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
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
             && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE) \
             + YYSIZEOF (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  85
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   404

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  34
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  47
/* YYNRULES -- Number of rules.  */
#define YYNRULES  160
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  367

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   288


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
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
      25,    26,    27,    28,    29,    30,    31,    32,    33
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   332,   332,   334,   338,   339,   340,   341,   342,   343,
     344,   345,   346,   347,   348,   349,   350,   351,   352,   353,
     356,   358,   360,   362,   366,   368,   372,   374,   376,   378,
     382,   384,   386,   388,   390,   392,   396,   398,   400,   402,
     406,   408,   410,   412,   416,   418,   420,   422,   426,   428,
     432,   434,   438,   440,   442,   444,   448,   450,   454,   457,
     459,   461,   463,   467,   469,   473,   474,   475,   476,   479,
     481,   485,   487,   491,   494,   497,   499,   501,   503,   507,
     509,   511,   513,   515,   517,   521,   523,   525,   527,   531,
     533,   535,   537,   539,   541,   543,   545,   547,   549,   553,
     555,   557,   559,   563,   565,   569,   571,   573,   575,   579,
     581,   583,   585,   589,   591,   595,   597,   601,   603,   605,
     607,   611,   615,   617,   619,   621,   625,   627,   631,   633,
     635,   639,   641,   643,   645,   649,   651,   655,   657,   659,
     663,   665,   669,   677,   686,   688,   691,   693,   696,   698,
     702,   704,   709,   714,   716,   721,   723,   728,   733,   741,
     746
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "POINT_TOK",
  "LINESTRING_TOK", "POLYGON_TOK", "MPOINT_TOK", "MLINESTRING_TOK",
  "MPOLYGON_TOK", "MSURFACE_TOK", "MCURVE_TOK", "CURVEPOLYGON_TOK",
  "COMPOUNDCURVE_TOK", "CIRCULARSTRING_TOK", "COLLECTION_TOK",
  "RBRACKET_TOK", "LBRACKET_TOK", "COMMA_TOK", "EMPTY_TOK",
  "SEMICOLON_TOK", "TRIANGLE_TOK", "TIN_TOK", "POLYHEDRALSURFACE_TOK",
  "NURBSCURVE_TOK", "DEGREE_TOK", "CONTROLPOINTS_TOK", "KNOTS_TOK",
  "NURBSPOINT_TOK", "WEIGHTEDPOINT_TOK", "WEIGHT_TOK", "KNOT_TOK",
  "DOUBLE_TOK", "DIMENSIONALITY_TOK", "SRID_TOK", "$accept", "geometry",
  "geometry_no_srid", "geometrycollection", "geometry_list",
  "multisurface", "surface_list", "tin", "polyhedralsurface",
  "multipolygon", "polygon_list", "patch_list", "polygon",
  "polygon_untagged", "patch", "curvepolygon", "curvering_list",
  "curvering", "patchring_list", "ring_list", "patchring", "ring",
  "compoundcurve", "compound_list", "multicurve", "curve_list",
  "multilinestring", "linestring_list", "circularstring", "linestring",
  "linestring_untagged", "triangle_list", "triangle", "triangle_untagged",
  "multipoint", "point_list", "point_untagged", "point", "ptarray",
  "coordinate", "opt_dimensionality", "nurbscurve", "iso_controlpoint",
  "iso_controlpoint_list", "iso_knot_list", "weight_list", "knot_list", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-288)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      27,    10,    58,    76,    80,    81,    85,    86,    89,    93,
     104,   108,   126,   146,   176,   185,   186,    -2,    21,  -288,
    -288,  -288,  -288,  -288,  -288,  -288,  -288,  -288,  -288,  -288,
    -288,  -288,  -288,  -288,  -288,  -288,    -9,  -288,    28,    -9,
    -288,   123,    29,  -288,   149,    40,  -288,   152,   189,  -288,
     194,   195,  -288,   198,    59,  -288,   203,    49,  -288,   204,
     119,  -288,   207,   153,  -288,   208,    -9,  -288,   211,   177,
    -288,   212,    35,  -288,   215,    47,  -288,   216,    52,  -288,
     220,    -6,  -288,   221,   177,  -288,    12,   178,  -288,    -9,
    -288,   225,    -9,  -288,    -9,   226,  -288,    29,  -288,    -9,
    -288,   229,  -288,  -288,    40,  -288,    -9,  -288,   233,  -288,
     189,  -288,    29,  -288,   234,  -288,   195,  -288,   237,  -288,
    -288,  -288,    59,  -288,  -288,   238,  -288,  -288,  -288,  -288,
      49,  -288,   241,  -288,  -288,  -288,  -288,  -288,   119,  -288,
     242,  -288,  -288,  -288,   153,  -288,   245,    -9,  -288,  -288,
     246,   177,  -288,    -9,    57,  -288,    66,   249,  -288,    47,
    -288,    90,   250,  -288,    52,  -288,    84,   102,    -4,  -288,
    -288,    96,  -288,    -9,   253,  -288,   254,   257,  -288,    29,
     258,   115,  -288,    40,   261,   262,  -288,   189,   266,   267,
    -288,   195,   270,  -288,    59,   271,  -288,    49,   274,  -288,
     119,   275,  -288,   153,   278,  -288,   279,  -288,   177,   282,
     283,    -9,    -9,  -288,    47,   286,    -9,   287,  -288,  -288,
      52,   290,   116,   118,   130,   155,   142,  -288,  -288,  -288,
    -288,  -288,  -288,  -288,  -288,  -288,  -288,  -288,  -288,  -288,
    -288,  -288,  -288,  -288,  -288,  -288,  -288,  -288,  -288,  -288,
    -288,  -288,  -288,  -288,  -288,  -288,  -288,  -288,  -288,  -288,
    -288,   161,   291,   294,  -288,  -288,   295,  -288,    90,  -288,
    -288,   171,    -9,   160,   190,  -288,  -288,   200,   230,  -288,
    -288,   309,   298,   317,    -9,  -288,  -288,  -288,   327,   299,
     309,   302,   320,  -288,   328,   329,   303,   330,  -288,   306,
     321,   320,  -288,   332,   325,   333,   320,  -288,   307,   310,
     321,   309,   331,  -288,   311,   324,   334,   314,   338,   340,
    -288,   342,  -288,   335,   315,    -9,   336,   337,   343,  -288,
     344,   347,   348,   318,  -288,   319,   336,   337,   346,   339,
     350,   341,   352,   345,   322,   323,   349,   355,  -288,   353,
    -288,  -288,   358,   359,   351,   354,   356,  -288,  -288,   360,
     362,   363,  -288,  -288,   357,   364,  -288
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     2,
      19,    13,    15,    16,    12,     8,     9,     7,    14,    11,
       6,     5,    17,    10,     4,    18,     0,   134,     0,     0,
     112,     0,     0,    55,     0,     0,   125,     0,     0,   102,
       0,     0,    47,     0,     0,    29,     0,     0,    88,     0,
       0,    62,     0,     0,    78,     0,     0,   108,     0,     0,
      23,     0,     0,   120,     0,     0,    39,     0,     0,    43,
       0,     0,   151,     0,     0,     1,     0,     0,   136,     0,
     133,     0,     0,   111,     0,     0,    72,     0,    54,     0,
     130,     0,   127,   128,     0,   124,     0,   114,     0,   104,
       0,   101,     0,    57,     0,    49,     0,    46,     0,    33,
      35,    34,     0,    28,    95,     0,    94,    97,    98,    96,
       0,    87,     0,    64,    67,    68,    66,    65,     0,    61,
       0,    82,    83,    84,     0,    77,     0,     0,   107,    25,
       0,     0,    22,     0,     0,   119,     0,     0,   116,     0,
      38,     0,     0,    51,     0,    42,     0,     0,     0,   150,
       3,   137,   131,     0,     0,   109,     0,     0,    52,     0,
       0,     0,   122,     0,     0,     0,    99,     0,     0,     0,
      44,     0,     0,    26,     0,     0,    85,     0,     0,    59,
       0,     0,    75,     0,     0,   105,     0,    20,     0,     0,
       0,     0,     0,    36,     0,     0,     0,     0,    70,    40,
       0,     0,     0,     0,     0,     0,   138,   135,   132,   110,
      74,    71,    53,   129,   126,   123,   113,   103,   100,    56,
      48,    45,    30,    32,    31,    27,    90,    89,    92,    93,
      91,    86,    63,    60,    79,    80,    81,    76,   106,    24,
      21,     0,     0,     0,   115,    37,     0,    58,     0,    50,
      41,     0,     0,     0,     0,   139,   117,     0,     0,    73,
      69,   141,     0,     0,     0,   118,   121,   140,     0,     0,
     141,     0,     0,   148,     0,     0,     0,     0,   154,     0,
       0,     0,   149,     0,     0,     0,     0,   158,     0,     0,
       0,   141,     0,   153,     0,     0,     0,     0,     0,     0,
     146,     0,   157,     0,     0,     0,     0,     0,     0,   147,
       0,     0,     0,     0,   160,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   142,     0,
     144,   159,     0,     0,     0,     0,     0,   143,   145,     0,
       0,     0,   152,   156,     0,     0,   155
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -288,  -288,     1,  -288,   232,  -288,   227,  -288,  -288,  -288,
     265,   222,   -39,   -31,   164,   -38,   251,   191,  -288,    17,
     122,   213,   -50,   252,  -288,   263,  -288,   284,   -51,   -49,
     -44,   236,  -288,   183,  -288,   296,   218,  -288,   -37,   -45,
    -287,   -52,    92,    98,    67,    94,    65
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,    18,   149,    20,   150,    21,   118,    22,    23,    24,
     114,   162,    25,   115,   163,    26,   132,   133,   217,    95,
     218,    96,    27,   140,    28,   125,    29,   108,    30,    31,
     137,   157,    32,   158,    33,   101,   102,    34,    87,    88,
     288,    35,   298,   299,   333,   308,   335
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
     103,    19,    91,   295,   109,   129,   126,   124,   127,   135,
     134,   136,   141,   128,   142,   119,   121,    84,   166,   143,
     224,    85,    86,   120,   318,   167,    36,   225,    37,   146,
       1,     2,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    38,   171,    89,    94,    90,    13,    14,    15,
      16,   153,   174,     2,   181,   176,    99,   177,   100,   103,
      17,    10,    11,   156,     3,   106,   109,   107,   161,   185,
       9,    86,    16,   211,    39,   112,    40,   113,   129,   126,
     124,   127,   212,   119,   121,   170,   128,   135,   134,   136,
      41,   120,    42,   141,    43,   142,    45,    48,    46,    49,
     143,    51,    54,    52,    55,    57,   216,    58,    44,    60,
     206,    61,    47,    50,   180,   222,   210,    53,    56,   223,
      63,    59,    64,     2,    66,    62,    67,   226,   227,   189,
     233,    10,    11,   271,   272,   106,    65,   107,   103,    92,
      68,    93,    69,   237,    70,   250,   247,   246,   248,   135,
     134,   136,   254,   249,   255,   242,   244,     2,    71,   256,
     240,   273,    72,   243,    73,    97,    11,    98,   104,   106,
     105,   107,   274,   275,   262,   263,   276,   283,    74,   266,
       1,     2,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    75,   172,    76,   173,   281,    13,    14,    15,
      16,    78,    81,    79,    82,   106,   284,   107,    77,   259,
     110,   112,   111,   113,   116,   285,   117,    80,    83,   122,
     130,   123,   131,   138,   144,   139,   145,   147,   151,   148,
     152,   154,   159,   155,   160,   282,   164,   168,   165,   169,
     175,   178,   173,   179,   182,   286,   183,   291,   186,   190,
     187,   191,   193,   196,   194,   197,   199,   202,   200,   203,
     205,   207,   173,   208,   213,   219,   214,   220,   228,   229,
     173,   173,   230,   232,   173,   179,   235,   236,   183,   173,
     331,   238,   239,   187,   179,   241,   245,   191,   194,   251,
     253,   197,   200,   257,   258,   203,   173,   260,   261,   208,
     173,   265,   267,   214,   268,   270,   277,   220,   173,   278,
     279,   173,   173,   289,   293,   173,   294,   296,   302,   173,
     303,   305,   314,   306,   315,   316,   320,   306,   321,   324,
     329,   315,   330,   340,   342,   341,   343,   352,   353,   341,
     343,   287,   290,   292,   300,   301,   304,   297,   310,   195,
     312,   323,   307,   311,   325,   322,   326,   319,   327,   336,
     337,   328,   338,   346,   339,   348,   332,   350,   334,   356,
     347,   349,   355,   357,   358,   362,   351,   363,   354,   366,
     364,   192,   359,   209,   269,   360,   221,   361,   365,   201,
     280,   252,   231,   198,   188,   215,   204,   264,   313,   309,
     184,   234,   345,   344,   317
};

static const yytype_int16 yycheck[] =
{
      45,     0,    39,   290,    48,    57,    57,    57,    57,    60,
      60,    60,    63,    57,    63,    54,    54,    19,    24,    63,
      24,     0,    31,    54,   311,    31,    16,    31,    18,    66,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    32,    31,    16,    16,    18,    20,    21,    22,
      23,    16,    89,     4,    99,    92,    16,    94,    18,   104,
      33,    12,    13,    16,     5,    16,   110,    18,    16,   106,
      11,    31,    23,    16,    16,    16,    18,    18,   130,   130,
     130,   130,    16,   122,   122,    84,   130,   138,   138,   138,
      32,   122,    16,   144,    18,   144,    16,    16,    18,    18,
     144,    16,    16,    18,    18,    16,    16,    18,    32,    16,
     147,    18,    32,    32,    97,    31,   153,    32,    32,    17,
      16,    32,    18,     4,    16,    32,    18,    31,   173,   112,
      15,    12,    13,    17,    16,    16,    32,    18,   183,    16,
      32,    18,    16,   187,    18,   197,   197,   197,   197,   200,
     200,   200,   203,   197,   203,   194,   194,     4,    32,   203,
     191,    31,    16,   194,    18,    16,    13,    18,    16,    16,
      18,    18,    17,    31,   211,   212,    15,    17,    32,   216,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    16,    15,    18,    17,    25,    20,    21,    22,
      23,    16,    16,    18,    18,    16,    16,    18,    32,   208,
      16,    16,    18,    18,    16,    15,    18,    32,    32,    16,
      16,    18,    18,    16,    16,    18,    18,    16,    16,    18,
      18,    16,    16,    18,    18,   272,    16,    16,    18,    18,
      15,    15,    17,    17,    15,    15,    17,   284,    15,    15,
      17,    17,    15,    15,    17,    17,    15,    15,    17,    17,
      15,    15,    17,    17,    15,    15,    17,    17,    15,    15,
      17,    17,    15,    15,    17,    17,    15,    15,    17,    17,
     325,    15,    15,    17,    17,    15,    15,    17,    17,    15,
      15,    17,    17,    15,    15,    17,    17,    15,    15,    17,
      17,    15,    15,    17,    17,    15,    15,    17,    17,    15,
      15,    17,    17,    15,    15,    17,    17,    15,    15,    17,
      17,    15,    15,    17,    17,    15,    15,    17,    17,    15,
      15,    17,    17,    15,    15,    17,    17,    15,    15,    17,
      17,    32,    25,    16,    16,    16,    16,    27,    16,   122,
      17,    17,    31,    28,    16,    31,    16,    26,    16,    16,
      16,    26,    15,    17,    16,    15,    30,    15,    31,    16,
      31,    30,    17,    15,    15,    15,    31,    15,    29,    15,
      17,   116,    31,   151,   220,    31,   164,    31,    31,   138,
     268,   200,   179,   130,   110,   159,   144,   214,   306,   301,
     104,   183,   337,   336,   310
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    20,    21,    22,    23,    33,    35,    36,
      37,    39,    41,    42,    43,    46,    49,    56,    58,    60,
      62,    63,    66,    68,    71,    75,    16,    18,    32,    16,
      18,    32,    16,    18,    32,    16,    18,    32,    16,    18,
      32,    16,    18,    32,    16,    18,    32,    16,    18,    32,
      16,    18,    32,    16,    18,    32,    16,    18,    32,    16,
      18,    32,    16,    18,    32,    16,    18,    32,    16,    18,
      32,    16,    18,    32,    19,     0,    31,    72,    73,    16,
      18,    72,    16,    18,    16,    53,    55,    16,    18,    16,
      18,    69,    70,    73,    16,    18,    16,    18,    61,    64,
      16,    18,    16,    18,    44,    47,    16,    18,    40,    46,
      47,    49,    16,    18,    56,    59,    62,    63,    64,    75,
      16,    18,    50,    51,    56,    62,    63,    64,    16,    18,
      57,    62,    63,    64,    16,    18,    72,    16,    18,    36,
      38,    16,    18,    16,    16,    18,    16,    65,    67,    16,
      18,    16,    45,    48,    16,    18,    24,    31,    16,    18,
      36,    31,    15,    17,    72,    15,    72,    72,    15,    17,
      53,    73,    15,    17,    69,    72,    15,    17,    61,    53,
      15,    17,    44,    15,    17,    40,    15,    17,    59,    15,
      17,    50,    15,    17,    57,    15,    72,    15,    17,    38,
      72,    16,    16,    15,    17,    65,    16,    52,    54,    15,
      17,    45,    31,    17,    24,    31,    31,    73,    15,    15,
      15,    55,    15,    15,    70,    15,    15,    64,    15,    15,
      47,    15,    46,    47,    49,    15,    56,    62,    63,    64,
      75,    15,    51,    15,    62,    63,    64,    15,    15,    36,
      15,    15,    72,    72,    67,    15,    72,    15,    17,    48,
      15,    17,    16,    31,    17,    31,    15,    15,    15,    15,
      54,    25,    72,    17,    16,    15,    15,    32,    74,    15,
      25,    72,    16,    15,    17,    74,    15,    27,    76,    77,
      16,    16,    15,    17,    16,    15,    17,    31,    79,    77,
      16,    28,    17,    76,    15,    17,    15,    79,    74,    26,
      15,    17,    31,    17,    15,    16,    16,    16,    26,    15,
      17,    73,    30,    78,    31,    80,    16,    16,    15,    16,
      15,    17,    15,    17,    78,    80,    17,    31,    15,    30,
      15,    31,    15,    15,    29,    17,    16,    15,    15,    31,
      31,    31,    15,    15,    17,    31,    15
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    34,    35,    35,    36,    36,    36,    36,    36,    36,
      36,    36,    36,    36,    36,    36,    36,    36,    36,    36,
      37,    37,    37,    37,    38,    38,    39,    39,    39,    39,
      40,    40,    40,    40,    40,    40,    41,    41,    41,    41,
      42,    42,    42,    42,    43,    43,    43,    43,    44,    44,
      45,    45,    46,    46,    46,    46,    47,    47,    48,    49,
      49,    49,    49,    50,    50,    51,    51,    51,    51,    52,
      52,    53,    53,    54,    55,    56,    56,    56,    56,    57,
      57,    57,    57,    57,    57,    58,    58,    58,    58,    59,
      59,    59,    59,    59,    59,    59,    59,    59,    59,    60,
      60,    60,    60,    61,    61,    62,    62,    62,    62,    63,
      63,    63,    63,    64,    64,    65,    65,    66,    66,    66,
      66,    67,    68,    68,    68,    68,    69,    69,    70,    70,
      70,    71,    71,    71,    71,    72,    72,    73,    73,    73,
      74,    74,    75,    75,    75,    75,    75,    75,    75,    75,
      75,    75,    76,    77,    77,    78,    78,    79,    79,    80,
      80
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     3,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       4,     5,     3,     2,     3,     1,     4,     5,     3,     2,
       3,     3,     3,     1,     1,     1,     4,     5,     3,     2,
       4,     5,     3,     2,     4,     5,     3,     2,     3,     1,
       3,     1,     4,     5,     3,     2,     3,     1,     3,     4,
       5,     3,     2,     3,     1,     1,     1,     1,     1,     3,
       1,     3,     1,     3,     3,     4,     5,     3,     2,     3,
       3,     3,     1,     1,     1,     4,     5,     3,     2,     3,
       3,     3,     3,     3,     1,     1,     1,     1,     1,     4,
       5,     3,     2,     3,     1,     4,     5,     3,     2,     4,
       5,     3,     2,     3,     1,     3,     1,     6,     7,     3,
       2,     5,     4,     5,     3,     2,     3,     1,     1,     3,
       1,     4,     5,     3,     2,     3,     1,     2,     3,     4,
       1,     0,    16,    17,    16,    17,    12,    13,     8,     9,
       3,     2,    11,     3,     1,     8,     6,     3,     1,     3,
       1
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF

/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (0)
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K])


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)


/* YYLOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

# ifndef YYLOCATION_PRINT

#  if defined YY_LOCATION_PRINT

   /* Temporary convenience wrapper in case some people defined the
      undocumented and private YY_LOCATION_PRINT macros.  */
#   define YYLOCATION_PRINT(File, Loc)  YY_LOCATION_PRINT(File, *(Loc))

#  elif defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL

/* Print *YYLOCP on YYO.  Private, do not rely on its existence. */

YY_ATTRIBUTE_UNUSED
static int
yy_location_print_ (FILE *yyo, YYLTYPE const * const yylocp)
{
  int res = 0;
  int end_col = 0 != yylocp->last_column ? yylocp->last_column - 1 : 0;
  if (0 <= yylocp->first_line)
    {
      res += YYFPRINTF (yyo, "%d", yylocp->first_line);
      if (0 <= yylocp->first_column)
        res += YYFPRINTF (yyo, ".%d", yylocp->first_column);
    }
  if (0 <= yylocp->last_line)
    {
      if (yylocp->first_line < yylocp->last_line)
        {
          res += YYFPRINTF (yyo, "-%d", yylocp->last_line);
          if (0 <= end_col)
            res += YYFPRINTF (yyo, ".%d", end_col);
        }
      else if (0 <= end_col && yylocp->first_column < end_col)
        res += YYFPRINTF (yyo, "-%d", end_col);
    }
  return res;
}

#   define YYLOCATION_PRINT  yy_location_print_

    /* Temporary convenience wrapper in case some people defined the
       undocumented and private YY_LOCATION_PRINT macros.  */
#   define YY_LOCATION_PRINT(File, Loc)  YYLOCATION_PRINT(File, &(Loc))

#  else

#   define YYLOCATION_PRINT(File, Loc) ((void) 0)
    /* Temporary convenience wrapper in case some people defined the
       undocumented and private YY_LOCATION_PRINT macros.  */
#   define YY_LOCATION_PRINT  YYLOCATION_PRINT

#  endif
# endif /* !defined YYLOCATION_PRINT */


# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value, Location); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (yylocationp);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  YYLOCATION_PRINT (yyo, yylocationp);
  YYFPRINTF (yyo, ": ");
  yy_symbol_value_print (yyo, yykind, yyvaluep, yylocationp);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp, YYLTYPE *yylsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)],
                       &(yylsp[(yyi + 1) - (yynrhs)]));
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, yylsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
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






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, YYLTYPE *yylocationp)
{
  YY_USE (yyvaluep);
  YY_USE (yylocationp);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  switch (yykind)
    {
    case YYSYMBOL_geometry_no_srid: /* geometry_no_srid  */
#line 308 "liblwgeom/lwin_wkt_parse.y"
            { lwgeom_free(((*yyvaluep).geometryvalue)); }
#line 1424 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_geometrycollection: /* geometrycollection  */
#line 309 "liblwgeom/lwin_wkt_parse.y"
            { lwgeom_free(((*yyvaluep).geometryvalue)); }
#line 1430 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_geometry_list: /* geometry_list  */
#line 310 "liblwgeom/lwin_wkt_parse.y"
            { lwgeom_free(((*yyvaluep).geometryvalue)); }
#line 1436 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_multisurface: /* multisurface  */
#line 317 "liblwgeom/lwin_wkt_parse.y"
            { lwgeom_free(((*yyvaluep).geometryvalue)); }
#line 1442 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_surface_list: /* surface_list  */
#line 295 "liblwgeom/lwin_wkt_parse.y"
            { lwgeom_free(((*yyvaluep).geometryvalue)); }
#line 1448 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_tin: /* tin  */
#line 325 "liblwgeom/lwin_wkt_parse.y"
            { lwgeom_free(((*yyvaluep).geometryvalue)); }
#line 1454 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_polyhedralsurface: /* polyhedralsurface  */
#line 323 "liblwgeom/lwin_wkt_parse.y"
            { lwgeom_free(((*yyvaluep).geometryvalue)); }
#line 1460 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_multipolygon: /* multipolygon  */
#line 316 "liblwgeom/lwin_wkt_parse.y"
            { lwgeom_free(((*yyvaluep).geometryvalue)); }
#line 1466 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_polygon_list: /* polygon_list  */
#line 296 "liblwgeom/lwin_wkt_parse.y"
            { lwgeom_free(((*yyvaluep).geometryvalue)); }
#line 1472 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_patch_list: /* patch_list  */
#line 297 "liblwgeom/lwin_wkt_parse.y"
            { lwgeom_free(((*yyvaluep).geometryvalue)); }
#line 1478 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_polygon: /* polygon  */
#line 320 "liblwgeom/lwin_wkt_parse.y"
            { lwgeom_free(((*yyvaluep).geometryvalue)); }
#line 1484 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_polygon_untagged: /* polygon_untagged  */
#line 322 "liblwgeom/lwin_wkt_parse.y"
            { lwgeom_free(((*yyvaluep).geometryvalue)); }
#line 1490 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_patch: /* patch  */
#line 321 "liblwgeom/lwin_wkt_parse.y"
            { lwgeom_free(((*yyvaluep).geometryvalue)); }
#line 1496 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_curvepolygon: /* curvepolygon  */
#line 306 "liblwgeom/lwin_wkt_parse.y"
            { lwgeom_free(((*yyvaluep).geometryvalue)); }
#line 1502 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_curvering_list: /* curvering_list  */
#line 293 "liblwgeom/lwin_wkt_parse.y"
            { lwgeom_free(((*yyvaluep).geometryvalue)); }
#line 1508 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_curvering: /* curvering  */
#line 307 "liblwgeom/lwin_wkt_parse.y"
            { lwgeom_free(((*yyvaluep).geometryvalue)); }
#line 1514 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_patchring_list: /* patchring_list  */
#line 303 "liblwgeom/lwin_wkt_parse.y"
            { lwgeom_free(((*yyvaluep).geometryvalue)); }
#line 1520 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_ring_list: /* ring_list  */
#line 302 "liblwgeom/lwin_wkt_parse.y"
            { lwgeom_free(((*yyvaluep).geometryvalue)); }
#line 1526 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_patchring: /* patchring  */
#line 287 "liblwgeom/lwin_wkt_parse.y"
            { ptarray_free(((*yyvaluep).ptarrayvalue)); }
#line 1532 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_ring: /* ring  */
#line 286 "liblwgeom/lwin_wkt_parse.y"
            { ptarray_free(((*yyvaluep).ptarrayvalue)); }
#line 1538 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_compoundcurve: /* compoundcurve  */
#line 305 "liblwgeom/lwin_wkt_parse.y"
            { lwgeom_free(((*yyvaluep).geometryvalue)); }
#line 1544 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_compound_list: /* compound_list  */
#line 301 "liblwgeom/lwin_wkt_parse.y"
            { lwgeom_free(((*yyvaluep).geometryvalue)); }
#line 1550 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_multicurve: /* multicurve  */
#line 313 "liblwgeom/lwin_wkt_parse.y"
            { lwgeom_free(((*yyvaluep).geometryvalue)); }
#line 1556 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_curve_list: /* curve_list  */
#line 300 "liblwgeom/lwin_wkt_parse.y"
            { lwgeom_free(((*yyvaluep).geometryvalue)); }
#line 1562 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_multilinestring: /* multilinestring  */
#line 314 "liblwgeom/lwin_wkt_parse.y"
            { lwgeom_free(((*yyvaluep).geometryvalue)); }
#line 1568 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_linestring_list: /* linestring_list  */
#line 299 "liblwgeom/lwin_wkt_parse.y"
            { lwgeom_free(((*yyvaluep).geometryvalue)); }
#line 1574 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_circularstring: /* circularstring  */
#line 304 "liblwgeom/lwin_wkt_parse.y"
            { lwgeom_free(((*yyvaluep).geometryvalue)); }
#line 1580 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_linestring: /* linestring  */
#line 311 "liblwgeom/lwin_wkt_parse.y"
            { lwgeom_free(((*yyvaluep).geometryvalue)); }
#line 1586 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_linestring_untagged: /* linestring_untagged  */
#line 312 "liblwgeom/lwin_wkt_parse.y"
            { lwgeom_free(((*yyvaluep).geometryvalue)); }
#line 1592 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_triangle_list: /* triangle_list  */
#line 294 "liblwgeom/lwin_wkt_parse.y"
            { lwgeom_free(((*yyvaluep).geometryvalue)); }
#line 1598 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_triangle: /* triangle  */
#line 326 "liblwgeom/lwin_wkt_parse.y"
            { lwgeom_free(((*yyvaluep).geometryvalue)); }
#line 1604 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_triangle_untagged: /* triangle_untagged  */
#line 327 "liblwgeom/lwin_wkt_parse.y"
            { lwgeom_free(((*yyvaluep).geometryvalue)); }
#line 1610 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_multipoint: /* multipoint  */
#line 315 "liblwgeom/lwin_wkt_parse.y"
            { lwgeom_free(((*yyvaluep).geometryvalue)); }
#line 1616 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_point_list: /* point_list  */
#line 298 "liblwgeom/lwin_wkt_parse.y"
            { lwgeom_free(((*yyvaluep).geometryvalue)); }
#line 1622 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_point_untagged: /* point_untagged  */
#line 319 "liblwgeom/lwin_wkt_parse.y"
            { lwgeom_free(((*yyvaluep).geometryvalue)); }
#line 1628 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_point: /* point  */
#line 318 "liblwgeom/lwin_wkt_parse.y"
            { lwgeom_free(((*yyvaluep).geometryvalue)); }
#line 1634 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_ptarray: /* ptarray  */
#line 285 "liblwgeom/lwin_wkt_parse.y"
            { ptarray_free(((*yyvaluep).ptarrayvalue)); }
#line 1640 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_nurbscurve: /* nurbscurve  */
#line 324 "liblwgeom/lwin_wkt_parse.y"
            { lwgeom_free(((*yyvaluep).geometryvalue)); }
#line 1646 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_iso_controlpoint: /* iso_controlpoint  */
#line 291 "liblwgeom/lwin_wkt_parse.y"
            { wkt_parser_nurbs_controlpoints_free(((*yyvaluep).nurbscontrolpointsvalue)); }
#line 1652 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_iso_controlpoint_list: /* iso_controlpoint_list  */
#line 292 "liblwgeom/lwin_wkt_parse.y"
            { wkt_parser_nurbs_controlpoints_free(((*yyvaluep).nurbscontrolpointsvalue)); }
#line 1658 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_iso_knot_list: /* iso_knot_list  */
#line 290 "liblwgeom/lwin_wkt_parse.y"
            { ptarray_free(((*yyvaluep).ptarrayvalue)); }
#line 1664 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_weight_list: /* weight_list  */
#line 288 "liblwgeom/lwin_wkt_parse.y"
            { ptarray_free(((*yyvaluep).ptarrayvalue)); }
#line 1670 "liblwgeom/lwin_wkt_parse.c"
        break;

    case YYSYMBOL_knot_list: /* knot_list  */
#line 289 "liblwgeom/lwin_wkt_parse.y"
            { ptarray_free(((*yyvaluep).ptarrayvalue)); }
#line 1676 "liblwgeom/lwin_wkt_parse.c"
        break;

      default:
        break;
    }
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Location data for the lookahead symbol.  */
YYLTYPE yylloc
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  = { 1, 1, 1, 1 }
# endif
;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

    /* The location stack: array, bottom, top.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls = yylsa;
    YYLTYPE *yylsp = yyls;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

  /* The locations where the error started and ended.  */
  YYLTYPE yyerror_range[3];



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  yylsp[0] = yylloc;
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;
        YYLTYPE *yyls1 = yyls;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yyls1, yysize * YYSIZEOF (*yylsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
        yyls = yyls1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
        YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


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

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      yyerror_range[1] = yylloc;
      goto yyerrlab1;
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
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
  *++yylsp = yylloc;

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
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
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location. */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  yyerror_range[1] = yyloc;
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* geometry: geometry_no_srid  */
#line 333 "liblwgeom/lwin_wkt_parse.y"
                { wkt_parser_geometry_new((yyvsp[0].geometryvalue), SRID_UNKNOWN); WKT_ERROR(); }
#line 1971 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 3: /* geometry: SRID_TOK SEMICOLON_TOK geometry_no_srid  */
#line 335 "liblwgeom/lwin_wkt_parse.y"
                { wkt_parser_geometry_new((yyvsp[0].geometryvalue), (yyvsp[-2].integervalue)); WKT_ERROR(); }
#line 1977 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 4: /* geometry_no_srid: point  */
#line 338 "liblwgeom/lwin_wkt_parse.y"
              { (yyval.geometryvalue) = (yyvsp[0].geometryvalue); }
#line 1983 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 5: /* geometry_no_srid: linestring  */
#line 339 "liblwgeom/lwin_wkt_parse.y"
                   { (yyval.geometryvalue) = (yyvsp[0].geometryvalue); }
#line 1989 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 6: /* geometry_no_srid: circularstring  */
#line 340 "liblwgeom/lwin_wkt_parse.y"
                       { (yyval.geometryvalue) = (yyvsp[0].geometryvalue); }
#line 1995 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 7: /* geometry_no_srid: compoundcurve  */
#line 341 "liblwgeom/lwin_wkt_parse.y"
                      { (yyval.geometryvalue) = (yyvsp[0].geometryvalue); }
#line 2001 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 8: /* geometry_no_srid: polygon  */
#line 342 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = (yyvsp[0].geometryvalue); }
#line 2007 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 9: /* geometry_no_srid: curvepolygon  */
#line 343 "liblwgeom/lwin_wkt_parse.y"
                     { (yyval.geometryvalue) = (yyvsp[0].geometryvalue); }
#line 2013 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 10: /* geometry_no_srid: multipoint  */
#line 344 "liblwgeom/lwin_wkt_parse.y"
                   { (yyval.geometryvalue) = (yyvsp[0].geometryvalue); }
#line 2019 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 11: /* geometry_no_srid: multilinestring  */
#line 345 "liblwgeom/lwin_wkt_parse.y"
                        { (yyval.geometryvalue) = (yyvsp[0].geometryvalue); }
#line 2025 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 12: /* geometry_no_srid: multipolygon  */
#line 346 "liblwgeom/lwin_wkt_parse.y"
                     { (yyval.geometryvalue) = (yyvsp[0].geometryvalue); }
#line 2031 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 13: /* geometry_no_srid: multisurface  */
#line 347 "liblwgeom/lwin_wkt_parse.y"
                     { (yyval.geometryvalue) = (yyvsp[0].geometryvalue); }
#line 2037 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 14: /* geometry_no_srid: multicurve  */
#line 348 "liblwgeom/lwin_wkt_parse.y"
                   { (yyval.geometryvalue) = (yyvsp[0].geometryvalue); }
#line 2043 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 15: /* geometry_no_srid: tin  */
#line 349 "liblwgeom/lwin_wkt_parse.y"
            { (yyval.geometryvalue) = (yyvsp[0].geometryvalue); }
#line 2049 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 16: /* geometry_no_srid: polyhedralsurface  */
#line 350 "liblwgeom/lwin_wkt_parse.y"
                          { (yyval.geometryvalue) = (yyvsp[0].geometryvalue); }
#line 2055 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 17: /* geometry_no_srid: triangle  */
#line 351 "liblwgeom/lwin_wkt_parse.y"
                 { (yyval.geometryvalue) = (yyvsp[0].geometryvalue); }
#line 2061 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 18: /* geometry_no_srid: nurbscurve  */
#line 352 "liblwgeom/lwin_wkt_parse.y"
                   { (yyval.geometryvalue) = (yyvsp[0].geometryvalue); }
#line 2067 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 19: /* geometry_no_srid: geometrycollection  */
#line 353 "liblwgeom/lwin_wkt_parse.y"
                           { (yyval.geometryvalue) = (yyvsp[0].geometryvalue); }
#line 2073 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 20: /* geometrycollection: COLLECTION_TOK LBRACKET_TOK geometry_list RBRACKET_TOK  */
#line 357 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_finalize(COLLECTIONTYPE, (yyvsp[-1].geometryvalue), NULL); WKT_ERROR(); }
#line 2079 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 21: /* geometrycollection: COLLECTION_TOK DIMENSIONALITY_TOK LBRACKET_TOK geometry_list RBRACKET_TOK  */
#line 359 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_finalize(COLLECTIONTYPE, (yyvsp[-1].geometryvalue), (yyvsp[-3].stringvalue)); WKT_ERROR(); }
#line 2085 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 22: /* geometrycollection: COLLECTION_TOK DIMENSIONALITY_TOK EMPTY_TOK  */
#line 361 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_finalize(COLLECTIONTYPE, NULL, (yyvsp[-1].stringvalue)); WKT_ERROR(); }
#line 2091 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 23: /* geometrycollection: COLLECTION_TOK EMPTY_TOK  */
#line 363 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_finalize(COLLECTIONTYPE, NULL, NULL); WKT_ERROR(); }
#line 2097 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 24: /* geometry_list: geometry_list COMMA_TOK geometry_no_srid  */
#line 367 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[-2].geometryvalue),(yyvsp[0].geometryvalue)); WKT_ERROR(); }
#line 2103 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 25: /* geometry_list: geometry_no_srid  */
#line 369 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[0].geometryvalue)); WKT_ERROR(); }
#line 2109 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 26: /* multisurface: MSURFACE_TOK LBRACKET_TOK surface_list RBRACKET_TOK  */
#line 373 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTISURFACETYPE, (yyvsp[-1].geometryvalue), NULL); WKT_ERROR(); }
#line 2115 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 27: /* multisurface: MSURFACE_TOK DIMENSIONALITY_TOK LBRACKET_TOK surface_list RBRACKET_TOK  */
#line 375 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTISURFACETYPE, (yyvsp[-1].geometryvalue), (yyvsp[-3].stringvalue)); WKT_ERROR(); }
#line 2121 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 28: /* multisurface: MSURFACE_TOK DIMENSIONALITY_TOK EMPTY_TOK  */
#line 377 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTISURFACETYPE, NULL, (yyvsp[-1].stringvalue)); WKT_ERROR(); }
#line 2127 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 29: /* multisurface: MSURFACE_TOK EMPTY_TOK  */
#line 379 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTISURFACETYPE, NULL, NULL); WKT_ERROR(); }
#line 2133 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 30: /* surface_list: surface_list COMMA_TOK polygon  */
#line 383 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[-2].geometryvalue),(yyvsp[0].geometryvalue)); WKT_ERROR(); }
#line 2139 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 31: /* surface_list: surface_list COMMA_TOK curvepolygon  */
#line 385 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[-2].geometryvalue),(yyvsp[0].geometryvalue)); WKT_ERROR(); }
#line 2145 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 32: /* surface_list: surface_list COMMA_TOK polygon_untagged  */
#line 387 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[-2].geometryvalue),(yyvsp[0].geometryvalue)); WKT_ERROR(); }
#line 2151 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 33: /* surface_list: polygon  */
#line 389 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[0].geometryvalue)); WKT_ERROR(); }
#line 2157 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 34: /* surface_list: curvepolygon  */
#line 391 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[0].geometryvalue)); WKT_ERROR(); }
#line 2163 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 35: /* surface_list: polygon_untagged  */
#line 393 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[0].geometryvalue)); WKT_ERROR(); }
#line 2169 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 36: /* tin: TIN_TOK LBRACKET_TOK triangle_list RBRACKET_TOK  */
#line 397 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_finalize(TINTYPE, (yyvsp[-1].geometryvalue), NULL); WKT_ERROR(); }
#line 2175 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 37: /* tin: TIN_TOK DIMENSIONALITY_TOK LBRACKET_TOK triangle_list RBRACKET_TOK  */
#line 399 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_finalize(TINTYPE, (yyvsp[-1].geometryvalue), (yyvsp[-3].stringvalue)); WKT_ERROR(); }
#line 2181 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 38: /* tin: TIN_TOK DIMENSIONALITY_TOK EMPTY_TOK  */
#line 401 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_finalize(TINTYPE, NULL, (yyvsp[-1].stringvalue)); WKT_ERROR(); }
#line 2187 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 39: /* tin: TIN_TOK EMPTY_TOK  */
#line 403 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_finalize(TINTYPE, NULL, NULL); WKT_ERROR(); }
#line 2193 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 40: /* polyhedralsurface: POLYHEDRALSURFACE_TOK LBRACKET_TOK patch_list RBRACKET_TOK  */
#line 407 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_finalize(POLYHEDRALSURFACETYPE, (yyvsp[-1].geometryvalue), NULL); WKT_ERROR(); }
#line 2199 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 41: /* polyhedralsurface: POLYHEDRALSURFACE_TOK DIMENSIONALITY_TOK LBRACKET_TOK patch_list RBRACKET_TOK  */
#line 409 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_finalize(POLYHEDRALSURFACETYPE, (yyvsp[-1].geometryvalue), (yyvsp[-3].stringvalue)); WKT_ERROR(); }
#line 2205 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 42: /* polyhedralsurface: POLYHEDRALSURFACE_TOK DIMENSIONALITY_TOK EMPTY_TOK  */
#line 411 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_finalize(POLYHEDRALSURFACETYPE, NULL, (yyvsp[-1].stringvalue)); WKT_ERROR(); }
#line 2211 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 43: /* polyhedralsurface: POLYHEDRALSURFACE_TOK EMPTY_TOK  */
#line 413 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_finalize(POLYHEDRALSURFACETYPE, NULL, NULL); WKT_ERROR(); }
#line 2217 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 44: /* multipolygon: MPOLYGON_TOK LBRACKET_TOK polygon_list RBRACKET_TOK  */
#line 417 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTIPOLYGONTYPE, (yyvsp[-1].geometryvalue), NULL); WKT_ERROR(); }
#line 2223 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 45: /* multipolygon: MPOLYGON_TOK DIMENSIONALITY_TOK LBRACKET_TOK polygon_list RBRACKET_TOK  */
#line 419 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTIPOLYGONTYPE, (yyvsp[-1].geometryvalue), (yyvsp[-3].stringvalue)); WKT_ERROR(); }
#line 2229 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 46: /* multipolygon: MPOLYGON_TOK DIMENSIONALITY_TOK EMPTY_TOK  */
#line 421 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTIPOLYGONTYPE, NULL, (yyvsp[-1].stringvalue)); WKT_ERROR(); }
#line 2235 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 47: /* multipolygon: MPOLYGON_TOK EMPTY_TOK  */
#line 423 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTIPOLYGONTYPE, NULL, NULL); WKT_ERROR(); }
#line 2241 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 48: /* polygon_list: polygon_list COMMA_TOK polygon_untagged  */
#line 427 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[-2].geometryvalue),(yyvsp[0].geometryvalue)); WKT_ERROR(); }
#line 2247 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 49: /* polygon_list: polygon_untagged  */
#line 429 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[0].geometryvalue)); WKT_ERROR(); }
#line 2253 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 50: /* patch_list: patch_list COMMA_TOK patch  */
#line 433 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[-2].geometryvalue),(yyvsp[0].geometryvalue)); WKT_ERROR(); }
#line 2259 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 51: /* patch_list: patch  */
#line 435 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[0].geometryvalue)); WKT_ERROR(); }
#line 2265 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 52: /* polygon: POLYGON_TOK LBRACKET_TOK ring_list RBRACKET_TOK  */
#line 439 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_polygon_finalize((yyvsp[-1].geometryvalue), NULL); WKT_ERROR(); }
#line 2271 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 53: /* polygon: POLYGON_TOK DIMENSIONALITY_TOK LBRACKET_TOK ring_list RBRACKET_TOK  */
#line 441 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_polygon_finalize((yyvsp[-1].geometryvalue), (yyvsp[-3].stringvalue)); WKT_ERROR(); }
#line 2277 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 54: /* polygon: POLYGON_TOK DIMENSIONALITY_TOK EMPTY_TOK  */
#line 443 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_polygon_finalize(NULL, (yyvsp[-1].stringvalue)); WKT_ERROR(); }
#line 2283 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 55: /* polygon: POLYGON_TOK EMPTY_TOK  */
#line 445 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_polygon_finalize(NULL, NULL); WKT_ERROR(); }
#line 2289 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 56: /* polygon_untagged: LBRACKET_TOK ring_list RBRACKET_TOK  */
#line 449 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = (yyvsp[-1].geometryvalue); }
#line 2295 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 57: /* polygon_untagged: EMPTY_TOK  */
#line 451 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_polygon_finalize(NULL, NULL); WKT_ERROR(); }
#line 2301 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 58: /* patch: LBRACKET_TOK patchring_list RBRACKET_TOK  */
#line 454 "liblwgeom/lwin_wkt_parse.y"
                                                 { (yyval.geometryvalue) = (yyvsp[-1].geometryvalue); }
#line 2307 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 59: /* curvepolygon: CURVEPOLYGON_TOK LBRACKET_TOK curvering_list RBRACKET_TOK  */
#line 458 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_curvepolygon_finalize((yyvsp[-1].geometryvalue), NULL); WKT_ERROR(); }
#line 2313 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 60: /* curvepolygon: CURVEPOLYGON_TOK DIMENSIONALITY_TOK LBRACKET_TOK curvering_list RBRACKET_TOK  */
#line 460 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_curvepolygon_finalize((yyvsp[-1].geometryvalue), (yyvsp[-3].stringvalue)); WKT_ERROR(); }
#line 2319 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 61: /* curvepolygon: CURVEPOLYGON_TOK DIMENSIONALITY_TOK EMPTY_TOK  */
#line 462 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_curvepolygon_finalize(NULL, (yyvsp[-1].stringvalue)); WKT_ERROR(); }
#line 2325 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 62: /* curvepolygon: CURVEPOLYGON_TOK EMPTY_TOK  */
#line 464 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_curvepolygon_finalize(NULL, NULL); WKT_ERROR(); }
#line 2331 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 63: /* curvering_list: curvering_list COMMA_TOK curvering  */
#line 468 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_curvepolygon_add_ring((yyvsp[-2].geometryvalue),(yyvsp[0].geometryvalue)); WKT_ERROR(); }
#line 2337 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 64: /* curvering_list: curvering  */
#line 470 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_curvepolygon_new((yyvsp[0].geometryvalue)); WKT_ERROR(); }
#line 2343 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 65: /* curvering: linestring_untagged  */
#line 473 "liblwgeom/lwin_wkt_parse.y"
                            { (yyval.geometryvalue) = (yyvsp[0].geometryvalue); }
#line 2349 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 66: /* curvering: linestring  */
#line 474 "liblwgeom/lwin_wkt_parse.y"
                   { (yyval.geometryvalue) = (yyvsp[0].geometryvalue); }
#line 2355 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 67: /* curvering: compoundcurve  */
#line 475 "liblwgeom/lwin_wkt_parse.y"
                      { (yyval.geometryvalue) = (yyvsp[0].geometryvalue); }
#line 2361 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 68: /* curvering: circularstring  */
#line 476 "liblwgeom/lwin_wkt_parse.y"
                       { (yyval.geometryvalue) = (yyvsp[0].geometryvalue); }
#line 2367 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 69: /* patchring_list: patchring_list COMMA_TOK patchring  */
#line 480 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_polygon_add_ring((yyvsp[-2].geometryvalue),(yyvsp[0].ptarrayvalue),'Z'); WKT_ERROR(); }
#line 2373 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 70: /* patchring_list: patchring  */
#line 482 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_polygon_new((yyvsp[0].ptarrayvalue),'Z'); WKT_ERROR(); }
#line 2379 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 71: /* ring_list: ring_list COMMA_TOK ring  */
#line 486 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_polygon_add_ring((yyvsp[-2].geometryvalue),(yyvsp[0].ptarrayvalue),'2'); WKT_ERROR(); }
#line 2385 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 72: /* ring_list: ring  */
#line 488 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_polygon_new((yyvsp[0].ptarrayvalue),'2'); WKT_ERROR(); }
#line 2391 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 73: /* patchring: LBRACKET_TOK ptarray RBRACKET_TOK  */
#line 491 "liblwgeom/lwin_wkt_parse.y"
                                          { (yyval.ptarrayvalue) = (yyvsp[-1].ptarrayvalue); }
#line 2397 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 74: /* ring: LBRACKET_TOK ptarray RBRACKET_TOK  */
#line 494 "liblwgeom/lwin_wkt_parse.y"
                                          { (yyval.ptarrayvalue) = (yyvsp[-1].ptarrayvalue); }
#line 2403 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 75: /* compoundcurve: COMPOUNDCURVE_TOK LBRACKET_TOK compound_list RBRACKET_TOK  */
#line 498 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_compound_finalize((yyvsp[-1].geometryvalue), NULL); WKT_ERROR(); }
#line 2409 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 76: /* compoundcurve: COMPOUNDCURVE_TOK DIMENSIONALITY_TOK LBRACKET_TOK compound_list RBRACKET_TOK  */
#line 500 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_compound_finalize((yyvsp[-1].geometryvalue), (yyvsp[-3].stringvalue)); WKT_ERROR(); }
#line 2415 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 77: /* compoundcurve: COMPOUNDCURVE_TOK DIMENSIONALITY_TOK EMPTY_TOK  */
#line 502 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_compound_finalize(NULL, (yyvsp[-1].stringvalue)); WKT_ERROR(); }
#line 2421 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 78: /* compoundcurve: COMPOUNDCURVE_TOK EMPTY_TOK  */
#line 504 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_compound_finalize(NULL, NULL); WKT_ERROR(); }
#line 2427 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 79: /* compound_list: compound_list COMMA_TOK circularstring  */
#line 508 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_compound_add_geom((yyvsp[-2].geometryvalue),(yyvsp[0].geometryvalue)); WKT_ERROR(); }
#line 2433 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 80: /* compound_list: compound_list COMMA_TOK linestring  */
#line 510 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_compound_add_geom((yyvsp[-2].geometryvalue),(yyvsp[0].geometryvalue)); WKT_ERROR(); }
#line 2439 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 81: /* compound_list: compound_list COMMA_TOK linestring_untagged  */
#line 512 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_compound_add_geom((yyvsp[-2].geometryvalue),(yyvsp[0].geometryvalue)); WKT_ERROR(); }
#line 2445 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 82: /* compound_list: circularstring  */
#line 514 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_compound_new((yyvsp[0].geometryvalue)); WKT_ERROR(); }
#line 2451 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 83: /* compound_list: linestring  */
#line 516 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_compound_new((yyvsp[0].geometryvalue)); WKT_ERROR(); }
#line 2457 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 84: /* compound_list: linestring_untagged  */
#line 518 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_compound_new((yyvsp[0].geometryvalue)); WKT_ERROR(); }
#line 2463 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 85: /* multicurve: MCURVE_TOK LBRACKET_TOK curve_list RBRACKET_TOK  */
#line 522 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTICURVETYPE, (yyvsp[-1].geometryvalue), NULL); WKT_ERROR(); }
#line 2469 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 86: /* multicurve: MCURVE_TOK DIMENSIONALITY_TOK LBRACKET_TOK curve_list RBRACKET_TOK  */
#line 524 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTICURVETYPE, (yyvsp[-1].geometryvalue), (yyvsp[-3].stringvalue)); WKT_ERROR(); }
#line 2475 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 87: /* multicurve: MCURVE_TOK DIMENSIONALITY_TOK EMPTY_TOK  */
#line 526 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTICURVETYPE, NULL, (yyvsp[-1].stringvalue)); WKT_ERROR(); }
#line 2481 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 88: /* multicurve: MCURVE_TOK EMPTY_TOK  */
#line 528 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTICURVETYPE, NULL, NULL); WKT_ERROR(); }
#line 2487 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 89: /* curve_list: curve_list COMMA_TOK circularstring  */
#line 532 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[-2].geometryvalue),(yyvsp[0].geometryvalue)); WKT_ERROR(); }
#line 2493 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 90: /* curve_list: curve_list COMMA_TOK compoundcurve  */
#line 534 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[-2].geometryvalue),(yyvsp[0].geometryvalue)); WKT_ERROR(); }
#line 2499 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 91: /* curve_list: curve_list COMMA_TOK nurbscurve  */
#line 536 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[-2].geometryvalue),(yyvsp[0].geometryvalue)); WKT_ERROR(); }
#line 2505 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 92: /* curve_list: curve_list COMMA_TOK linestring  */
#line 538 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[-2].geometryvalue),(yyvsp[0].geometryvalue)); WKT_ERROR(); }
#line 2511 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 93: /* curve_list: curve_list COMMA_TOK linestring_untagged  */
#line 540 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[-2].geometryvalue),(yyvsp[0].geometryvalue)); WKT_ERROR(); }
#line 2517 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 94: /* curve_list: circularstring  */
#line 542 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[0].geometryvalue)); WKT_ERROR(); }
#line 2523 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 95: /* curve_list: compoundcurve  */
#line 544 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[0].geometryvalue)); WKT_ERROR(); }
#line 2529 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 96: /* curve_list: nurbscurve  */
#line 546 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[0].geometryvalue)); WKT_ERROR(); }
#line 2535 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 97: /* curve_list: linestring  */
#line 548 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[0].geometryvalue)); WKT_ERROR(); }
#line 2541 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 98: /* curve_list: linestring_untagged  */
#line 550 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[0].geometryvalue)); WKT_ERROR(); }
#line 2547 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 99: /* multilinestring: MLINESTRING_TOK LBRACKET_TOK linestring_list RBRACKET_TOK  */
#line 554 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTILINETYPE, (yyvsp[-1].geometryvalue), NULL); WKT_ERROR(); }
#line 2553 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 100: /* multilinestring: MLINESTRING_TOK DIMENSIONALITY_TOK LBRACKET_TOK linestring_list RBRACKET_TOK  */
#line 556 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTILINETYPE, (yyvsp[-1].geometryvalue), (yyvsp[-3].stringvalue)); WKT_ERROR(); }
#line 2559 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 101: /* multilinestring: MLINESTRING_TOK DIMENSIONALITY_TOK EMPTY_TOK  */
#line 558 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTILINETYPE, NULL, (yyvsp[-1].stringvalue)); WKT_ERROR(); }
#line 2565 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 102: /* multilinestring: MLINESTRING_TOK EMPTY_TOK  */
#line 560 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTILINETYPE, NULL, NULL); WKT_ERROR(); }
#line 2571 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 103: /* linestring_list: linestring_list COMMA_TOK linestring_untagged  */
#line 564 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[-2].geometryvalue),(yyvsp[0].geometryvalue)); WKT_ERROR(); }
#line 2577 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 104: /* linestring_list: linestring_untagged  */
#line 566 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[0].geometryvalue)); WKT_ERROR(); }
#line 2583 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 105: /* circularstring: CIRCULARSTRING_TOK LBRACKET_TOK ptarray RBRACKET_TOK  */
#line 570 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_circularstring_new((yyvsp[-1].ptarrayvalue), NULL); WKT_ERROR(); }
#line 2589 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 106: /* circularstring: CIRCULARSTRING_TOK DIMENSIONALITY_TOK LBRACKET_TOK ptarray RBRACKET_TOK  */
#line 572 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_circularstring_new((yyvsp[-1].ptarrayvalue), (yyvsp[-3].stringvalue)); WKT_ERROR(); }
#line 2595 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 107: /* circularstring: CIRCULARSTRING_TOK DIMENSIONALITY_TOK EMPTY_TOK  */
#line 574 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_circularstring_new(NULL, (yyvsp[-1].stringvalue)); WKT_ERROR(); }
#line 2601 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 108: /* circularstring: CIRCULARSTRING_TOK EMPTY_TOK  */
#line 576 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_circularstring_new(NULL, NULL); WKT_ERROR(); }
#line 2607 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 109: /* linestring: LINESTRING_TOK LBRACKET_TOK ptarray RBRACKET_TOK  */
#line 580 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_linestring_new((yyvsp[-1].ptarrayvalue), NULL); WKT_ERROR(); }
#line 2613 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 110: /* linestring: LINESTRING_TOK DIMENSIONALITY_TOK LBRACKET_TOK ptarray RBRACKET_TOK  */
#line 582 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_linestring_new((yyvsp[-1].ptarrayvalue), (yyvsp[-3].stringvalue)); WKT_ERROR(); }
#line 2619 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 111: /* linestring: LINESTRING_TOK DIMENSIONALITY_TOK EMPTY_TOK  */
#line 584 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_linestring_new(NULL, (yyvsp[-1].stringvalue)); WKT_ERROR(); }
#line 2625 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 112: /* linestring: LINESTRING_TOK EMPTY_TOK  */
#line 586 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_linestring_new(NULL, NULL); WKT_ERROR(); }
#line 2631 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 113: /* linestring_untagged: LBRACKET_TOK ptarray RBRACKET_TOK  */
#line 590 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_linestring_new((yyvsp[-1].ptarrayvalue), NULL); WKT_ERROR(); }
#line 2637 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 114: /* linestring_untagged: EMPTY_TOK  */
#line 592 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_linestring_new(NULL, NULL); WKT_ERROR(); }
#line 2643 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 115: /* triangle_list: triangle_list COMMA_TOK triangle_untagged  */
#line 596 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[-2].geometryvalue),(yyvsp[0].geometryvalue)); WKT_ERROR(); }
#line 2649 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 116: /* triangle_list: triangle_untagged  */
#line 598 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[0].geometryvalue)); WKT_ERROR(); }
#line 2655 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 117: /* triangle: TRIANGLE_TOK LBRACKET_TOK LBRACKET_TOK ptarray RBRACKET_TOK RBRACKET_TOK  */
#line 602 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_triangle_new((yyvsp[-2].ptarrayvalue), NULL); WKT_ERROR(); }
#line 2661 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 118: /* triangle: TRIANGLE_TOK DIMENSIONALITY_TOK LBRACKET_TOK LBRACKET_TOK ptarray RBRACKET_TOK RBRACKET_TOK  */
#line 604 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_triangle_new((yyvsp[-2].ptarrayvalue), (yyvsp[-5].stringvalue)); WKT_ERROR(); }
#line 2667 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 119: /* triangle: TRIANGLE_TOK DIMENSIONALITY_TOK EMPTY_TOK  */
#line 606 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_triangle_new(NULL, (yyvsp[-1].stringvalue)); WKT_ERROR(); }
#line 2673 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 120: /* triangle: TRIANGLE_TOK EMPTY_TOK  */
#line 608 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_triangle_new(NULL, NULL); WKT_ERROR(); }
#line 2679 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 121: /* triangle_untagged: LBRACKET_TOK LBRACKET_TOK ptarray RBRACKET_TOK RBRACKET_TOK  */
#line 612 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_triangle_new((yyvsp[-2].ptarrayvalue), NULL); WKT_ERROR(); }
#line 2685 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 122: /* multipoint: MPOINT_TOK LBRACKET_TOK point_list RBRACKET_TOK  */
#line 616 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTIPOINTTYPE, (yyvsp[-1].geometryvalue), NULL); WKT_ERROR(); }
#line 2691 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 123: /* multipoint: MPOINT_TOK DIMENSIONALITY_TOK LBRACKET_TOK point_list RBRACKET_TOK  */
#line 618 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTIPOINTTYPE, (yyvsp[-1].geometryvalue), (yyvsp[-3].stringvalue)); WKT_ERROR(); }
#line 2697 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 124: /* multipoint: MPOINT_TOK DIMENSIONALITY_TOK EMPTY_TOK  */
#line 620 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTIPOINTTYPE, NULL, (yyvsp[-1].stringvalue)); WKT_ERROR(); }
#line 2703 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 125: /* multipoint: MPOINT_TOK EMPTY_TOK  */
#line 622 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_finalize(MULTIPOINTTYPE, NULL, NULL); WKT_ERROR(); }
#line 2709 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 126: /* point_list: point_list COMMA_TOK point_untagged  */
#line 626 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_add_geom((yyvsp[-2].geometryvalue),(yyvsp[0].geometryvalue)); WKT_ERROR(); }
#line 2715 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 127: /* point_list: point_untagged  */
#line 628 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_collection_new((yyvsp[0].geometryvalue)); WKT_ERROR(); }
#line 2721 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 128: /* point_untagged: coordinate  */
#line 632 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_point_new(wkt_parser_ptarray_new((yyvsp[0].coordinatevalue)),NULL); WKT_ERROR(); }
#line 2727 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 129: /* point_untagged: LBRACKET_TOK coordinate RBRACKET_TOK  */
#line 634 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_point_new(wkt_parser_ptarray_new((yyvsp[-1].coordinatevalue)),NULL); WKT_ERROR(); }
#line 2733 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 130: /* point_untagged: EMPTY_TOK  */
#line 636 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_point_new(NULL, NULL); WKT_ERROR(); }
#line 2739 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 131: /* point: POINT_TOK LBRACKET_TOK ptarray RBRACKET_TOK  */
#line 640 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_point_new((yyvsp[-1].ptarrayvalue), NULL); WKT_ERROR(); }
#line 2745 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 132: /* point: POINT_TOK DIMENSIONALITY_TOK LBRACKET_TOK ptarray RBRACKET_TOK  */
#line 642 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_point_new((yyvsp[-1].ptarrayvalue), (yyvsp[-3].stringvalue)); WKT_ERROR(); }
#line 2751 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 133: /* point: POINT_TOK DIMENSIONALITY_TOK EMPTY_TOK  */
#line 644 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_point_new(NULL, (yyvsp[-1].stringvalue)); WKT_ERROR(); }
#line 2757 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 134: /* point: POINT_TOK EMPTY_TOK  */
#line 646 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_point_new(NULL,NULL); WKT_ERROR(); }
#line 2763 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 135: /* ptarray: ptarray COMMA_TOK coordinate  */
#line 650 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.ptarrayvalue) = wkt_parser_ptarray_add_coord((yyvsp[-2].ptarrayvalue), (yyvsp[0].coordinatevalue)); WKT_ERROR(); }
#line 2769 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 136: /* ptarray: coordinate  */
#line 652 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.ptarrayvalue) = wkt_parser_ptarray_new((yyvsp[0].coordinatevalue)); WKT_ERROR(); }
#line 2775 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 137: /* coordinate: DOUBLE_TOK DOUBLE_TOK  */
#line 656 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.coordinatevalue) = wkt_parser_coord_2((yyvsp[-1].doublevalue), (yyvsp[0].doublevalue)); WKT_ERROR(); }
#line 2781 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 138: /* coordinate: DOUBLE_TOK DOUBLE_TOK DOUBLE_TOK  */
#line 658 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.coordinatevalue) = wkt_parser_coord_3((yyvsp[-2].doublevalue), (yyvsp[-1].doublevalue), (yyvsp[0].doublevalue)); WKT_ERROR(); }
#line 2787 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 139: /* coordinate: DOUBLE_TOK DOUBLE_TOK DOUBLE_TOK DOUBLE_TOK  */
#line 660 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.coordinatevalue) = wkt_parser_coord_4((yyvsp[-3].doublevalue), (yyvsp[-2].doublevalue), (yyvsp[-1].doublevalue), (yyvsp[0].doublevalue)); WKT_ERROR(); }
#line 2793 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 140: /* opt_dimensionality: DIMENSIONALITY_TOK  */
#line 664 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.stringvalue) = (yyvsp[0].stringvalue); }
#line 2799 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 141: /* opt_dimensionality: %empty  */
#line 665 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.stringvalue) = NULL; }
#line 2805 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 142: /* nurbscurve: NURBSCURVE_TOK LBRACKET_TOK DEGREE_TOK DOUBLE_TOK COMMA_TOK CONTROLPOINTS_TOK opt_dimensionality LBRACKET_TOK iso_controlpoint_list RBRACKET_TOK COMMA_TOK KNOTS_TOK LBRACKET_TOK iso_knot_list RBRACKET_TOK RBRACKET_TOK  */
#line 670 "liblwgeom/lwin_wkt_parse.y"
                {
			(yyval.geometryvalue) = wkt_parser_nurbscurve_new((yyvsp[-12].doublevalue), (yyvsp[-7].nurbscontrolpointsvalue)->points, (yyvsp[-7].nurbscontrolpointsvalue)->weights, (yyvsp[-2].ptarrayvalue), NULL);
			(yyvsp[-7].nurbscontrolpointsvalue)->points = NULL;
			(yyvsp[-7].nurbscontrolpointsvalue)->weights = NULL;
			wkt_parser_nurbs_controlpoints_free((yyvsp[-7].nurbscontrolpointsvalue));
			WKT_ERROR();
		}
#line 2817 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 143: /* nurbscurve: NURBSCURVE_TOK DIMENSIONALITY_TOK LBRACKET_TOK DEGREE_TOK DOUBLE_TOK COMMA_TOK CONTROLPOINTS_TOK opt_dimensionality LBRACKET_TOK iso_controlpoint_list RBRACKET_TOK COMMA_TOK KNOTS_TOK LBRACKET_TOK iso_knot_list RBRACKET_TOK RBRACKET_TOK  */
#line 678 "liblwgeom/lwin_wkt_parse.y"
                {
			(yyval.geometryvalue) = wkt_parser_nurbscurve_new((yyvsp[-12].doublevalue), (yyvsp[-7].nurbscontrolpointsvalue)->points, (yyvsp[-7].nurbscontrolpointsvalue)->weights, (yyvsp[-2].ptarrayvalue), (yyvsp[-15].stringvalue));
			(yyvsp[-7].nurbscontrolpointsvalue)->points = NULL;
			(yyvsp[-7].nurbscontrolpointsvalue)->weights = NULL;
			wkt_parser_nurbs_controlpoints_free((yyvsp[-7].nurbscontrolpointsvalue));
			WKT_ERROR();
		}
#line 2829 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 144: /* nurbscurve: NURBSCURVE_TOK LBRACKET_TOK DOUBLE_TOK COMMA_TOK LBRACKET_TOK ptarray RBRACKET_TOK COMMA_TOK LBRACKET_TOK weight_list RBRACKET_TOK COMMA_TOK LBRACKET_TOK knot_list RBRACKET_TOK RBRACKET_TOK  */
#line 687 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_nurbscurve_new((yyvsp[-13].doublevalue), (yyvsp[-10].ptarrayvalue), (yyvsp[-6].ptarrayvalue), (yyvsp[-2].ptarrayvalue), NULL); WKT_ERROR(); }
#line 2835 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 145: /* nurbscurve: NURBSCURVE_TOK DIMENSIONALITY_TOK LBRACKET_TOK DOUBLE_TOK COMMA_TOK LBRACKET_TOK ptarray RBRACKET_TOK COMMA_TOK LBRACKET_TOK weight_list RBRACKET_TOK COMMA_TOK LBRACKET_TOK knot_list RBRACKET_TOK RBRACKET_TOK  */
#line 689 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_nurbscurve_new((yyvsp[-13].doublevalue), (yyvsp[-10].ptarrayvalue), (yyvsp[-6].ptarrayvalue), (yyvsp[-2].ptarrayvalue), (yyvsp[-15].stringvalue)); WKT_ERROR(); }
#line 2841 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 146: /* nurbscurve: NURBSCURVE_TOK LBRACKET_TOK DOUBLE_TOK COMMA_TOK LBRACKET_TOK ptarray RBRACKET_TOK COMMA_TOK LBRACKET_TOK weight_list RBRACKET_TOK RBRACKET_TOK  */
#line 692 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_nurbscurve_new((yyvsp[-9].doublevalue), (yyvsp[-6].ptarrayvalue), (yyvsp[-2].ptarrayvalue), NULL, NULL); WKT_ERROR(); }
#line 2847 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 147: /* nurbscurve: NURBSCURVE_TOK DIMENSIONALITY_TOK LBRACKET_TOK DOUBLE_TOK COMMA_TOK LBRACKET_TOK ptarray RBRACKET_TOK COMMA_TOK LBRACKET_TOK weight_list RBRACKET_TOK RBRACKET_TOK  */
#line 694 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_nurbscurve_new((yyvsp[-9].doublevalue), (yyvsp[-6].ptarrayvalue), (yyvsp[-2].ptarrayvalue), NULL, (yyvsp[-11].stringvalue)); WKT_ERROR(); }
#line 2853 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 148: /* nurbscurve: NURBSCURVE_TOK LBRACKET_TOK DOUBLE_TOK COMMA_TOK LBRACKET_TOK ptarray RBRACKET_TOK RBRACKET_TOK  */
#line 697 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_nurbscurve_new((yyvsp[-5].doublevalue), (yyvsp[-2].ptarrayvalue), NULL, NULL, NULL); WKT_ERROR(); }
#line 2859 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 149: /* nurbscurve: NURBSCURVE_TOK DIMENSIONALITY_TOK LBRACKET_TOK DOUBLE_TOK COMMA_TOK LBRACKET_TOK ptarray RBRACKET_TOK RBRACKET_TOK  */
#line 699 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_nurbscurve_new((yyvsp[-5].doublevalue), (yyvsp[-2].ptarrayvalue), NULL, NULL, (yyvsp[-7].stringvalue)); WKT_ERROR(); }
#line 2865 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 150: /* nurbscurve: NURBSCURVE_TOK DIMENSIONALITY_TOK EMPTY_TOK  */
#line 703 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_nurbscurve_empty((yyvsp[-1].stringvalue)); WKT_ERROR(); }
#line 2871 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 151: /* nurbscurve: NURBSCURVE_TOK EMPTY_TOK  */
#line 705 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.geometryvalue) = wkt_parser_nurbscurve_empty(NULL); WKT_ERROR(); }
#line 2877 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 152: /* iso_controlpoint: NURBSPOINT_TOK LBRACKET_TOK WEIGHTEDPOINT_TOK opt_dimensionality LBRACKET_TOK coordinate RBRACKET_TOK COMMA_TOK WEIGHT_TOK DOUBLE_TOK RBRACKET_TOK  */
#line 710 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.nurbscontrolpointsvalue) = wkt_parser_nurbs_controlpoints_new((yyvsp[-5].coordinatevalue), (yyvsp[-1].doublevalue)); WKT_ERROR(); }
#line 2883 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 153: /* iso_controlpoint_list: iso_controlpoint_list COMMA_TOK iso_controlpoint  */
#line 715 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.nurbscontrolpointsvalue) = wkt_parser_nurbs_controlpoints_add((yyvsp[-2].nurbscontrolpointsvalue), (yyvsp[0].nurbscontrolpointsvalue)); WKT_ERROR(); }
#line 2889 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 154: /* iso_controlpoint_list: iso_controlpoint  */
#line 717 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.nurbscontrolpointsvalue) = (yyvsp[0].nurbscontrolpointsvalue); }
#line 2895 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 155: /* iso_knot_list: iso_knot_list COMMA_TOK KNOT_TOK LBRACKET_TOK DOUBLE_TOK COMMA_TOK DOUBLE_TOK RBRACKET_TOK  */
#line 722 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.ptarrayvalue) = wkt_parser_knot_list_add_repeated((yyvsp[-7].ptarrayvalue), (yyvsp[-3].doublevalue), (yyvsp[-1].doublevalue)); WKT_ERROR(); }
#line 2901 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 156: /* iso_knot_list: KNOT_TOK LBRACKET_TOK DOUBLE_TOK COMMA_TOK DOUBLE_TOK RBRACKET_TOK  */
#line 724 "liblwgeom/lwin_wkt_parse.y"
                { (yyval.ptarrayvalue) = wkt_parser_knot_list_add_repeated(NULL, (yyvsp[-3].doublevalue), (yyvsp[-1].doublevalue)); WKT_ERROR(); }
#line 2907 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 157: /* weight_list: weight_list COMMA_TOK DOUBLE_TOK  */
#line 729 "liblwgeom/lwin_wkt_parse.y"
                {
			(yyval.ptarrayvalue) = wkt_parser_ptarray_add_coord((yyvsp[-2].ptarrayvalue), wkt_parser_coord_2((yyvsp[0].doublevalue), 0));
			WKT_ERROR();
		}
#line 2916 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 158: /* weight_list: DOUBLE_TOK  */
#line 734 "liblwgeom/lwin_wkt_parse.y"
                {
			(yyval.ptarrayvalue) = wkt_parser_ptarray_new(wkt_parser_coord_2((yyvsp[0].doublevalue), 0));
			WKT_ERROR();
		}
#line 2925 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 159: /* knot_list: knot_list COMMA_TOK DOUBLE_TOK  */
#line 742 "liblwgeom/lwin_wkt_parse.y"
                {
			(yyval.ptarrayvalue) = wkt_parser_ptarray_add_coord((yyvsp[-2].ptarrayvalue), wkt_parser_coord_2((yyvsp[0].doublevalue), 0));
			WKT_ERROR();
		}
#line 2934 "liblwgeom/lwin_wkt_parse.c"
    break;

  case 160: /* knot_list: DOUBLE_TOK  */
#line 747 "liblwgeom/lwin_wkt_parse.y"
                {
			(yyval.ptarrayvalue) = wkt_parser_ptarray_new(wkt_parser_coord_2((yyvsp[0].doublevalue), 0));
			WKT_ERROR();
		}
#line 2943 "liblwgeom/lwin_wkt_parse.c"
    break;


#line 2947 "liblwgeom/lwin_wkt_parse.c"

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
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
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
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
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
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
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
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, yylsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  yyerror_range[2] = yylloc;
  ++yylsp;
  YYLLOC_DEFAULT (*yylsp, yyerror_range, 2);

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, &yylloc);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, yylsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

