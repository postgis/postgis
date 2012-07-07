
/* cmakedefine to 1 if you have the <dlfcn.h> header file. */
#cmakedefine HAVE_DLFCN_H

/* cmakedefined if libiconv headers and library are present */
#cmakedefine HAVE_ICONV

/* cmakedefine to 1 if you have the <inttypes.h> header file. */
#cmakedefine HAVE_INTTYPES_H

/* cmakedefine to 1 if you have the `geos_c' library (-lgeos_c). */
#cmakedefine HAVE_LIBGEOS_C

/* cmakedefine to 1 if you have the `pq' library (-lpq). */
#cmakedefine HAVE_LIBPQ

/* cmakedefine to 1 if you have the `proj' library (-lproj). */
#cmakedefine HAVE_LIBPROJ

/* cmakedefine to 1 if you have the <memory.h> header file. */
#cmakedefine HAVE_MEMORY_H

/* cmakedefine to 1 if you have the <stdint.h> header file. */
#cmakedefine HAVE_STDINT_H

/* cmakedefine to 1 if you have the <stdlib.h> header file. */
#cmakedefine HAVE_STDLIB_H

/* cmakedefine to 1 if you have the <strings.h> header file. */
#cmakedefine HAVE_STRINGS_H

/* cmakedefine to 1 if you have the <string.h> header file. */
#cmakedefine HAVE_STRING_H

/* cmakedefine to 1 if you have the <sys/stat.h> header file. */
#cmakedefine HAVE_SYS_STAT_H

/* cmakedefine to 1 if you have the <sys/types.h> header file. */
#cmakedefine HAVE_SYS_TYPES_H

/* cmakedefine to 1 if you have the <unistd.h> header file. */
#cmakedefine HAVE_UNISTD_H

/* cmakedefine to 1 if you don't have the <unistd.h> header file. */
#cmakedefine YY_NO_UNISTD_H 1

/* PostGIS major version */
#cmakedefine POSTGIS_MAJOR_VERSION @POSTGIS_MAJOR_VERSION@

/* PostGIS micro version */
#cmakedefine POSTGIS_MICRO_VERSION @POSTGIS_MICRO_VERSION@

/* PostGIS minor version */
#cmakedefine POSTGIS_MINOR_VERSION @POSTGIS_MINOR_VERSION@

/* PostgreSQL server version */
#cmakedefine POSTGIS_PGSQL_VERSION @POSTGIS_PGSQL_VERSION@

/* Enable caching of bounding box within geometries */
#cmakedefine POSTGIS_AUTOCACHE_BBOX @POSTGIS_AUTOCACHE_BBOX@

/* PostGIS build date */
#cmakedefine POSTGIS_BUILD_DATE "@POSTGIS_BUILD_DATE@"

/* PostGIS library debug level (0=disabled) */
#cmakedefine POSTGIS_DEBUG_LEVEL @POSTGIS_DEBUG_LEVEL@

/* GEOS library version */
#cmakedefine POSTGIS_GEOS_VERSION @POSTGIS_GEOS_VERSION@

/* PostGIS library version */
#cmakedefine POSTGIS_LIB_VERSION @POSTGIS_LIB_VERSION@

/* Enable GEOS profiling (0=disabled) */
#cmakedefine POSTGIS_PROFILE @POSTGIS_PROFILE@

/* PROJ library version */
#cmakedefine POSTGIS_PROJ_VERSION @POSTGIS_PROJ_VERSION@

/* PostGIS scripts version */
#cmakedefine POSTGIS_SCRIPTS_VERSION @POSTGIS_SCRIPTS_VERSION@

/* Enable use of ANALYZE statistics */
#cmakedefine POSTGIS_USE_STATS @POSTGIS_USE_STATS@

/* PostGIS version */
#cmakedefine POSTGIS_VERSION "@POSTGIS_VERSION@"

/* cmakedefine to 1 if you have the ANSI C header files. */
#cmakedefine STDC_HEADERS

/* cmakedefine to 1 if `lex' declares `yytext' as a `char *' by default, not a
   `char[]'. */
/* #cmakedefine YYTEXT_POINTER */

/* cmakedefine to 1 if you have the C99 function finite */
#cmakedefine HAVE_FINITE
#define finite _finite

#cmakedefine __func__ @POSTGIS_MSVC_FUNC@