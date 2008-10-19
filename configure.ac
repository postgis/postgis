dnl **********************************************************************
dnl * $Id: configure.ac 2797 2008-05-31 09:56:44Z mcayland $
dnl *
dnl * PostGIS - Spatial Types for PostgreSQL
dnl * http://postgis.refractions.net
dnl * Copyright 2008 Mark Cave-Ayland
dnl *
dnl * This is free software; you can redistribute and/or modify it under
dnl * the terms of the GNU General Public Licence. See the COPYING file.
dnl *
dnl **********************************************************************

AC_INIT()
AC_CONFIG_HEADERS([postgis_config.h])

dnl Invoke libtool: we do this as it is the easiest way to find the PIC
dnl flags required to build liblwgeom
AC_PROG_LIBTOOL

dnl
dnl Compilers
dnl
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX

dnl
dnl Define PIC flags in PICFLAGS (note: this variable is set as part of libtool initialisation above)
dnl
PICFLAGS="$lt_prog_compiler_pic"
AC_SUBST([PICFLAGS])

dnl
dnl For GCC enable additional warning flags -Wall and -Wmissing-prototypes (using macro included with libtool)
dnl
WARNFLAGS=""
AC_LIBTOOL_COMPILER_OPTION([if $compiler supports -Wall], [dummy], [-Wall], [], [WARNFLAGS="$WARNFLAGS -Wall"], []) 
AC_LIBTOOL_COMPILER_OPTION([if $compiler supports -Wmissing-prototypes], [dummy], [-Wmissing-prototypes], [], [WARNFLAGS="$WARNFLAGS -Wmissing-prototypes"], []) 
AC_SUBST([WARNFLAGS])

dnl
dnl Define executable suffix for use with the loader Makefiles
dnl
EXESUFFIX="$ac_cv_exeext"
AC_SUBST([EXESUFFIX])

dnl
dnl Version Information imported from Version.config 
dnl
POSTGIS_MAJOR_VERSION=`cat Version.config | grep POSTGIS_MAJOR_VERSION | sed 's/[[^=]]*=\([[0-9]]\)/\1/g'`
POSTGIS_MINOR_VERSION=`cat Version.config | grep POSTGIS_MINOR_VERSION | sed 's/[[^=]]*=\([[0-9]]\)/\1/g'`
POSTGIS_MICRO_VERSION=`cat Version.config | grep POSTGIS_MICRO_VERSION | sed 's/[[^=]]*=\([[0-9]]\)/\1/g'`

AC_DEFINE_UNQUOTED([POSTGIS_MAJOR_VERSION], ["$POSTGIS_MAJOR_VERSION"], [PostGIS major version])
AC_DEFINE_UNQUOTED([POSTGIS_MINOR_VERSION], ["$POSTGIS_MINOR_VERSION"], [PostGIS minor version])
AC_DEFINE_UNQUOTED([POSTGIS_MICRO_VERSION], ["$POSTGIS_MICRO_VERSION"], [PostGIS micro version])

AC_SUBST([POSTGIS_MAJOR_VERSION])
AC_SUBST([POSTGIS_MINOR_VERSION])
AC_SUBST([POSTGIS_MICRO_VERSION])

dnl
dnl Search for flex/bison to build the parser
dnl

AC_PROG_LEX
AC_PROG_YACC
AC_SUBST([LEX])
AC_SUBST([YACC])


dnl
dnl Search for xsltproc which is required for building documentation
dnl

AC_PATH_PROG([XSLTPROC], [xsltproc], [])
if test "x$XSLTPROC" = "x"; then
	AC_MSG_WARN([xsltproc is not installed so documentation cannot be built])
fi


dnl
dnl Search for db2pdf which is required for building PDF documentation
dnl

AC_PATH_PROG([DB2PDF], [db2pdf], [])
if test "x$DB2PDF" = "x"; then
	AC_MSG_WARN([db2pdf is not installed so PDF documentation cannot be built])
fi


dnl
dnl Search for dblatex which is required for building PDF documentation
dnl

AC_PATH_PROG([DBLATEX], [dblatex], [])
if test "x$DBLATEX" = "x"; then
	AC_MSG_WARN([dblatex is not installed so PDF documentation cannot be built])
fi



dnl
dnl Allow the user to specify the location of the html/docbook.xsl stylesheet
dnl

AC_ARG_WITH([xsldir], 
	[AS_HELP_STRING([--with-xsldir=PATH], [specify the directory containing the docbook.xsl stylesheet])], 
	[XSLBASE="$withval"], [XSLBASE=""])

XSLBASE_AUTO=""
if test "x$XSLBASE" = "x"; then
	dnl If the user did not specify a directory for the docbook stylesheet, choose the first directory
	dnl that matches from the following list
	SEARCHPATH="
		/usr/share/sgml/docbook/xsl-stylesheets 
		/usr/share/xml/docbook/stylesheet/nwalsh
		/usr/share/sgml/docbook/stylesheet/xsl/nwalsh
		"
	for p in ${SEARCHPATH}; do
		if test -r "${p}"/html/docbook.xsl; then
			XSLBASE_AUTO="${p}"
			break
		fi
	done

	dnl Check to see if the automatically searched paths above located a valid Docbook stylesheet
	if test "x$XSLBASE_AUTO" = "x"; then
		AC_MSG_WARN([could not locate Docbook stylesheets required to build the documentation])
	fi
else
	dnl The user specified an alternate directory so make sure everything looks sensible
	if test ! -d "$XSLBASE"; then
		AC_MSG_ERROR([the docbook stylesheet directory specified using --with-xsldir does not exist])
	fi 

	if test ! -f "$XSLBASE/html/docbook.xsl"; then
		AC_MSG_ERROR([the docbook stylesheet directory specified using --with-xsldir does not contain the html/docbook.xsl file])
	fi
fi

dnl
dnl If XSLBASE has been set then at this point we know it must be valid and so we can just use it. If XSLBASE_AUTO has been set, and XSLBASE
dnl is empty then a valid stylesheet was found in XSLBASE_AUTO so we should use that. Otherwise just continue silently with a blank XSLBASE
dnl variable which will trigger the error message in the documentation Makefile
dnl

if test "x$XSLBASE" = "x"; then
	if test ! "x$XSLBASE_AUTO" = "x"; then
		XSLBASE="$XSLBASE_AUTO"
	fi
fi

AC_SUBST([XSLBASE])


dnl
dnl Detect iconv if it is installed (used for shp2pgsql encoding conversion if available)
dnl

HAVE_ICONV_H=0
AC_CHECK_HEADER([iconv.h], [HAVE_ICONV_H=1], [])

dnl If we find the header file, try and link against the library
if test "x$HAVE_ICONV_H" != "x0"; then
	dnl Check for iconv includes as part of libc
	AC_CHECK_LIB([c], [iconv_open], [ICONV_LDFLAGS=-lc HAVE_ICONV=1], [])
	if test "x$HAVE_ICONV" = "x"; then
		dnl If not found, check for iconv included as part of libiconv
		AC_CHECK_LIB([iconv], [iconv_open], [ICONV_LDFLAGS=-liconv HAVE_ICONV=1], [])
		if test "x$HAVE_ICONV" = "x"; then
			dnl If not found, check for Win32 iconv (some of them use a lib prefix for functions within the iconv DLLs)
			AC_CHECK_LIB([iconv], [libiconv_open], [ICONV_LDFLAGS=-liconv HAVE_ICONV=1], [])
			if test "x$HAVE_ICONV" = "x"; then
				dnl No iconv library was found; issue a warning to the console
				AC_MSG_WARN([could not find iconv library: no support for encoding conversion will be included])
			fi
		fi	
	fi
else
	dnl No iconv header was found; issue a warning to the console
	AC_MSG_WARN([could not find iconv.h header: no support for encoding conversion will be included])
fi


dnl Only define HAVE_ICONV in postgis_config.h if we detect iconv sucessfully
if test "x$HAVE_ICONV" != "x"; then
	AC_DEFINE_UNQUOTED([HAVE_ICONV], [$HAVE_ICONV], [Defined if libiconv headers and library are present])
fi

AC_SUBST([ICONV_LDFLAGS])


dnl
dnl Detect the version of PostgreSQL installed on the system
dnl

AC_ARG_WITH([pgconfig], 
	[AS_HELP_STRING([--with-pgconfig=FILE], [specify an alternative pg_config file])], 
	[PGCONFIG="$withval"], [PGCONFIG=""])

if test "x$PGCONFIG" = "x"; then
	dnl PGCONFIG was not specified, so search within the current path
	AC_PATH_PROG([PGCONFIG], [pg_config])

	dnl If we couldn't find pg_config, display an error
	if test "x$PGCONFIG" = "x"; then
		AC_MSG_ERROR([could not find pg_config within the current path. You may need to try re-running configure with a --with-pgconfig parameter.])
	fi
else
	dnl PGCONFIG was specified; display a message to the user
	if test "x$PGCONFIG" = "xyes"; then
		AC_MSG_ERROR([you must specify a parameter to --with-pgconfig, e.g. --with-pgconfig=/path/to/pg_config])
	else
		if test -f $PGCONFIG; then
			AC_MSG_RESULT([Using user-specified pg_config file: $PGCONFIG])
		else
			AC_MSG_ERROR([the user-specified pg_config file $PGCONFIG does not exist])
		fi
	fi
fi

dnl Extract the version information from pg_config
dnl Note: we extract the major & minor separately, ensure they are numeric, and then combine to give
dnl the final version. This is to guard against user error... 
PGSQL_MAJOR_VERSION=`$PGCONFIG --version | sed 's/[[A-Za-z ]]*//' | cut -d. -f1 | sed 's/[[^0-9]]//g'`	
PGSQL_MINOR_VERSION=`$PGCONFIG --version | sed 's/[[A-Za-z ]]*//' | cut -d. -f2 | sed 's/[[^0-9]]//g'`	
POSTGIS_PGSQL_VERSION="$PGSQL_MAJOR_VERSION$PGSQL_MINOR_VERSION"

dnl Ensure that we are using PostgreSQL >= 8.1
if test ! "$PGSQL_MAJOR_VERSION" -ge 8; then
	AC_MSG_ERROR([PostGIS requires PostgreSQL >= 8.1])
else
	if test "$PGSQL_MAJOR_VERSION" -eq 8; then
		if test ! "$PGSQL_MINOR_VERSION" -ge 1; then
			AC_MSG_ERROR([PostGIS requires PostgreSQL >= 8.1])
		fi	
	fi
fi	

dnl Note: We don't need the server-side LDFLAGS or CPPFLAGS because we get these from PGXS 

dnl Extract the linker and include flags for the frontend (for programs that use libpq)
PGSQL_FE_LDFLAGS="-L`$PGCONFIG --libdir` -lpq"
PGSQL_FE_CPPFLAGS="-I`$PGCONFIG --includedir`"

AC_SUBST([PGSQL_FE_LDFLAGS])
AC_SUBST([PGSQL_FE_CPPFLAGS])

dnl Extract the documentation and man page directories
PGSQL_DOCDIR=`$PGCONFIG --docdir`
PGSQL_MANDIR=`$PGCONFIG --mandir`

AC_SUBST([PGSQL_DOCDIR])
AC_SUBST([PGSQL_MANDIR])

dnl Extract the executable directory
PGSQL_BINDIR=`$PGCONFIG --bindir`

AC_SUBST([PGSQL_BINDIR])


dnl Ensure that we can parse libpq-fe.h
CPPFLAGS_SAVE="$CPPFLAGS"
CPPFLAGS="$PGSQL_FE_CPPFLAGS"
AC_CHECK_HEADER([libpq-fe.h], [], [AC_MSG_ERROR([could not find libpq-fe.h])])
CPPFLAGS="$CPPFLAGS_SAVE"

dnl Ensure we can link against libpq
LIBS_SAVE="$LIBS"
LIBS="$PGSQL_FE_LDFLAGS"
AC_CHECK_LIB([pq], [PQserverVersion],
	[],
	[AC_MSG_ERROR([could not find libpq])],
	[])
LIBS="$LIBS_SAVE"

AC_DEFINE_UNQUOTED([POSTGIS_PGSQL_VERSION], [$POSTGIS_PGSQL_VERSION], [PostgreSQL server version])	
AC_SUBST([POSTGIS_PGSQL_VERSION])


dnl
dnl Detect the version of GEOS installed on the system
dnl

AC_ARG_WITH([geosconfig], 
	[AS_HELP_STRING([--with-geosconfig=FILE], [specify an alternative geos-config file])], 
	[GEOSCONFIG="$withval"], [GEOSCONFIG=""])

if test "x$GEOSCONFIG" = "x"; then
	dnl GEOSCONFIG was not specified, so search within the current path
	AC_PATH_PROG([GEOSCONFIG], [geos-config])

	dnl If we couldn't find geos-config, display an error
	if test "x$GEOSCONFIG" = "x"; then
		AC_MSG_ERROR([could not find geos-config within the current path. You may need to try re-running configure with a --with-geosconfig parameter.])
	fi
else
	dnl GEOSCONFIG was specified; display a message to the user
	if test "x$GEOSCONFIG" = "xyes"; then
		AC_MSG_ERROR([you must specify a parameter to --with-geosconfig, e.g. --with-geosconfig=/path/to/geos-config])
	else
		if test -f $GEOSCONFIG; then
			AC_MSG_RESULT([Using user-specified geos-config file: $GEOSCONFIG])
		else
			AC_MSG_ERROR([the user-specified geos-config file $GEOSCONFIG does not exist])
		fi	
	fi
fi

dnl Extract the version information from pg_config
dnl Note: we extract the major & minor separately, ensure they are numeric, and then combine to give
dnl the final version. This is to guard against user error... 
GEOS_MAJOR_VERSION=`$GEOSCONFIG --version | cut -d. -f1 | sed 's/[[^0-9]]//g'`
GEOS_MINOR_VERSION=`$GEOSCONFIG --version | cut -d. -f2 | sed 's/[[^0-9]]//g'`
POSTGIS_GEOS_VERSION="$GEOS_MAJOR_VERSION$GEOS_MINOR_VERSION"

dnl Ensure that we are using GEOS >= 2.2.0 (requires CAPI) 
if test ! "$GEOS_MAJOR_VERSION" -ge 2; then
	AC_MSG_ERROR([PostGIS requires GEOS >= 2.2])
else
	if test "$GEOS_MAJOR_VERSION" -eq 2; then
		if test ! "$GEOS_MINOR_VERSION" -ge 2; then
			AC_MSG_ERROR([PostGIS requires GEOS >= 2.2])
		fi
	fi
fi	

dnl Extract the linker and include flags 
GEOS_LDFLAGS=`$GEOSCONFIG --ldflags`
GEOS_CPPFLAGS=-I`$GEOSCONFIG --includes`

dnl Ensure that we can parse geos_c.h
CPPFLAGS_SAVE="$CPPFLAGS"
CPPFLAGS="$GEOS_CPPFLAGS"
AC_CHECK_HEADER([geos_c.h], [], [AC_MSG_ERROR([could not find geos_c.h - you may need to specify the directory of a geos-config file using --with-geosconfig])])
CPPFLAGS="$CPPFLAGS_SAVE"

dnl Ensure we can link against libgeos_c
LIBS_SAVE="$LIBS"
LIBS="$GEOS_LDFLAGS"
AC_CHECK_LIB([geos_c], [initGEOS],
	[],
	[AC_MSG_ERROR([could not find libgeos_c - you may need to specify the directory of a geos-config file using --with-geosconfig])],
	[])
LIBS="$LIBS_SAVE"

AC_DEFINE_UNQUOTED([POSTGIS_GEOS_VERSION], [$POSTGIS_GEOS_VERSION], [GEOS library version])	
AC_SUBST([POSTGIS_GEOS_VERSION])


dnl
dnl Detect the version of PROJ.4 installed
dnl

AC_ARG_WITH([projdir], 
	[AS_HELP_STRING([--with-projdir=PATH], [specify the PROJ.4 installation directory])], 
	[PROJDIR="$withval"], [PROJDIR=""])

if test ! "x$PROJDIR" = "x"; then
	dnl Make sure that the directory exists 
	if test "x$PROJDIR" = "xyes"; then
		AC_MSG_ERROR([you must specifiy a parameter to --with-projdir, e.g. --with-projdir=/path/to])
	else
		if test -d "$PROJDIR"; then
			AC_MSG_RESULT([Using user-specified proj directory: $PROJDIR])

			dnl Add the include directory to PROJ_CPPFLAGS	
			PROJ_CPPFLAGS="-I$PROJDIR/include"	
			PROJ_LDFLAGS="-L$PROJDIR/lib"
		else
			AC_MSG_ERROR([the --with-projdir directory "$PROJDIR" cannot be found]) 
		fi
	fi
fi

dnl Check that we can find the proj_api.h header file
CPPFLAGS_SAVE="$CPPFLAGS"
CPPFLAGS="$PROJ_CPPFLAGS"
AC_CHECK_HEADER([proj_api.h], [], [AC_MSG_ERROR([could not find proj_api.h - you may need to specify the directory of a PROJ.4 installation using --with-projdir])])	

dnl Return the PROJ.4 version number
AC_PROJ_VERSION([POSTGIS_PROJ_VERSION])
AC_DEFINE_UNQUOTED([POSTGIS_PROJ_VERSION], [$POSTGIS_PROJ_VERSION], [PROJ library version])
AC_SUBST([POSTGIS_PROJ_VERSION])
CPPFLAGS="$CPPFLAGS_SAVE"

dnl Ensure that we are using PROJ >= 4.5.0 (requires pj_set_searchpath) 
if test ! "$POSTGIS_PROJ_VERSION" -ge 45; then
	AC_MSG_ERROR([PostGIS requires PROJ >= 4.5.0])
fi

dnl Ensure we can link against libproj
LIBS_SAVE="$LIBS"
LIBS="$PROJ_LDFLAGS"
AC_CHECK_LIB([proj], [pj_get_release],
	[],
	[AC_MSG_ERROR([could not find libproj - you may need to specify the directory of a PROJ.4 installation using --with-projdir])],
	[])
LIBS="$LIBS_SAVE"


dnl
dnl Allow the user to enable debugging with --enable-debug
dnl
dnl Currently we default to debug level 4. See DEBUG for more information. 
dnl

AC_ARG_ENABLE([debug], AC_HELP_STRING([--enable-debug], [Enable verbose debugging messages]), 
	[POSTGIS_DEBUG_LEVEL=4], [POSTGIS_DEBUG_LEVEL=0]) 

AC_DEFINE_UNQUOTED([POSTGIS_DEBUG_LEVEL], [$POSTGIS_DEBUG_LEVEL], [PostGIS library debug level (0=disabled)])


dnl
dnl Define version macros
dnl

POSTGIS_VERSION="$POSTGIS_MAJOR_VERSION.$POSTGIS_MINOR_VERSION USE_GEOS=1 USE_PROJ=1 USE_STATS=1"
POSTGIS_LIB_VERSION="$POSTGIS_MAJOR_VERSION.$POSTGIS_MINOR_VERSION.$POSTGIS_MICRO_VERSION"
POSTGIS_BUILD_DATE=`date -u "+%Y-%m-%d %H:%M:%S"`
POSTGIS_SCRIPTS_VERSION="$POSTGIS_LIB_VERSION"

AC_DEFINE_UNQUOTED([POSTGIS_VERSION], ["$POSTGIS_VERSION"], [PostGIS version])
AC_DEFINE_UNQUOTED([POSTGIS_LIB_VERSION], ["$POSTGIS_LIB_VERSION"], [PostGIS library version])
AC_DEFINE_UNQUOTED([POSTGIS_BUILD_DATE], ["$POSTGIS_BUILD_DATE"], [PostGIS build date])
AC_DEFINE_UNQUOTED([POSTGIS_SCRIPTS_VERSION], ["$POSTGIS_SCRIPTS_VERSION"], [PostGIS scripts version])

AC_SUBST([POSTGIS_VERSION])
AC_SUBST([POSTGIS_LIB_VERSION])
AC_SUBST([POSTGIS_BUILD_DATE])
AC_SUBST([POSTGIS_SCRIPTS_VERSION])


dnl
dnl Other parameters 
dnl

dnl Always enable BBOX caching by default
AC_DEFINE_UNQUOTED([POSTGIS_AUTOCACHE_BBOX], [1], [Enable caching of bounding box within geometries])

dnl Always enable use of ANALYZE statistics by default
AC_DEFINE_UNQUOTED([POSTGIS_USE_STATS], [1], [Enable use of ANALYZE statistics])


CPPFLAGS="$PGSQL_CPPFLAGS $GEOS_CPPFLAGS $PROJ_CPPFLAGS"
dnl AC_MSG_RESULT([CPPFLAGS: $CPPFLAGS])

SHLIB_LINK="$PGSQL_LDFLAGS $GEOS_LDFLAGS $PROJ_LDFLAGS -lgeos_c -lproj"
AC_SUBST([SHLIB_LINK])
dnl AC_MSG_RESULT([SHLIB_LINK: $SHLIB_LINK])

dnl Output the relevant files
AC_OUTPUT([liblwgeom/Makefile lwgeom/Makefile lwgeom/sqldefines.h loader/Makefile topology/Makefile regress/Makefile doc/Makefile])

