/* postgis_config.h.  Generated from postgis_config.h.in by configure.  */
/* postgis_config.h.in.  Generated from configure.ac by autoheader.  */

#include "postgis_svn_revision.h"

/* Define to 1 if translation of program messages to the user's native
   language is requested. */
#define ENABLE_NLS 1

/* Define to 1 if GDALFPolygonize function is available */
#define GDALFPOLYGONIZE 1

/* Define to 1 if you have the MacOS X function CFLocaleCopyCurrent in the
   CoreFoundation framework. */
/* #undef HAVE_CFLOCALECOPYCURRENT */

/* Define to 1 if you have the MacOS X function CFPreferencesCopyAppValue in
   the CoreFoundation framework. */
/* #undef HAVE_CFPREFERENCESCOPYAPPVALUE */

/* Define if the GNU dcgettext() function is already present or preinstalled.
   */
#define HAVE_DCGETTEXT 1

/* Define for some functions we are interested in */
#define HAVE_VASPRINTF 1
#define HAVE_ASPRINTF 1
#define HAVE_FSEEKO 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define if the GNU gettext() function is already present or preinstalled. */
#define HAVE_GETTEXT 1

/* Define if you have the iconv() function and it works. */
#define HAVE_ICONV 1

/* Define to 1 if you have the `iconvctl' function. */
/* #undef HAVE_ICONVCTL */

/* ieeefp.h header */
#define HAVE_IEEEFP_H 0

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `geos_c' library (-lgeos_c). */
#define HAVE_LIBGEOS_C 1

/* Define to 1 if you have the `libiconvctl' function. */
/* #undef HAVE_LIBICONVCTL */

/* Define to 1 if libjson is present */
#define HAVE_LIBJSON 1

/* Define to 1 if you have the `pq' library (-lpq). */
#define HAVE_LIBPQ 1

/* Define to 1 if you have the `proj' library (-lproj). */
#define HAVE_LIBPROJ 1

/* Define to 1 if you have the `xml2' library (-lxml2). */
#define HAVE_LIBXML2 1

/* Define to 1 if you have the <libxml/parser.h> header file. */
#define HAVE_LIBXML_PARSER_H 1

/* Define to 1 if you have the <libxml/tree.h> header file. */
#define HAVE_LIBXML_TREE_H 1

/* Define to 1 if you have the <libxml/xpathInternals.h> header file. */
#define HAVE_LIBXML_XPATHINTERNALS_H 1

/* Define to 1 if you have the <libxml/xpath.h> header file. */
#define HAVE_LIBXML_XPATH_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Location of PostgreSQL locale directory */
#define PGSQL_LOCALEDIR "/usr/share/locale"

/* Enable caching of bounding box within geometries */
#define POSTGIS_AUTOCACHE_BBOX 1

/* PostGIS build date */
#define POSTGIS_BUILD_DATE "2013-10-30 17:44:27"

/* PostGIS library debug level (0=disabled) */
#define POSTGIS_DEBUG_LEVEL 0

/* GDAL library version */
#define POSTGIS_GDAL_VERSION 19

/* GEOS library version */
#define POSTGIS_GEOS_VERSION 33

/* PostGIS libxml2 version */
#define POSTGIS_LIBXML2_VERSION "2.9.1"

/* PostGIS library version */
#define POSTGIS_LIB_VERSION "2.1.0"

/* PostGIS major version */
#define POSTGIS_MAJOR_VERSION "2"

/* PostGIS minor version */
#define POSTGIS_MINOR_VERSION "1"

/* PostGIS micro version */
#define POSTGIS_MICRO_VERSION "0"

/* PostgreSQL server version */
#define POSTGIS_PGSQL_VERSION 91

/* Enable GEOS profiling (0=disabled) */
#define POSTGIS_PROFILE 0

/* PROJ library version */
#define POSTGIS_PROJ_VERSION 48

/* PostGIS Raster build date */
#define POSTGIS_RASTER_BUILD_DATE "2013-10-30 17:44:27"

/* PostGIS Raster library version */
#define POSTGIS_RASTER_LIB_VERSION "0.1.6d"

/* PostGIS Raster major version */
#define POSTGIS_RASTER_MAJOR_VERSION "0"

/* PostGIS Raster micro version */
#define POSTGIS_RASTER_MICRO_VERSION "6d"

/* PostGIS Raster minor version */
#define POSTGIS_RASTER_MINOR_VERSION "1"

/* PostGIS Raster scripts version */
#define POSTGIS_RASTER_SCRIPTS_VERSION "0.1.6d"

/* PostGIS Raster version */
#define POSTGIS_RASTER_VERSION "0.1"

/* Define to 1 if a warning is outputted every time a double is truncated */
#define POSTGIS_RASTER_WARN_ON_TRUNCATION 0

/* PostGIS scripts version */
#define POSTGIS_SCRIPTS_VERSION "2.1.0"

/* Enable use of ANALYZE statistics */
#define POSTGIS_USE_STATS 1

/* PostGIS version */
#define POSTGIS_VERSION "2.1 USE_GEOS=1 USE_PROJ=1 USE_STATS=1"

/* Define command to determine the current directory during regression */
/* #undef PWDREGRESS */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to 1 if `lex' declares `yytext' as a `char *' by default, not a
   `char[]'. */
/* #undef YYTEXT_POINTER */
