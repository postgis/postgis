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
AC_CONFIG_MACRO_DIR([macros])


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
AC_LIBTOOL_COMPILER_OPTION([if $compiler supports -Wall], [_cv_wall], [-Wall], [], [WARNFLAGS="$WARNFLAGS -Wall"], []) 
AC_LIBTOOL_COMPILER_OPTION([if $compiler supports -Wmissing-prototypes], [_cv_misprot], [-Wmissing-prototypes], [], [WARNFLAGS="$WARNFLAGS -Wmissing-prototypes"], []) 
AC_SUBST([WARNFLAGS])

dnl
dnl For some GCC versions and platforms, floating point issues need to be
dnl ironed out. 
NUMERICFLAGS=""
AC_LIBTOOL_COMPILER_OPTION([if $compiler supports -ffloat-store], [dummy_cv_ffloat_store], [-ffloat-store], [], [NUMERICFLAGS="$NUMERICFLAGS -ffloat-store"], [])
AC_SUBST([NUMERICFLAGS])

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
dnl Search for OS-specific headers
dnl
AC_CHECK_HEADER([ieeefp.h], [HAVE_IEEEFP_H=1], [HAVE_IEEEFP_H=0])
AC_DEFINE_UNQUOTED([HAVE_IEEEFP_H], [$HAVE_IEEEFP_H], [ieeefp.h header])

dnl
dnl Check for platform-specific printing functions
dnl
AC_CHECK_FUNC(vasprintf, [HAVE_VASPRINTF=1], [HAVE_VASPRINTF=0])
AC_CHECK_FUNC(asprintf, [HAVE_ASPRINTF=1], [HAVE_ASPRINTF=0])
AC_SUBST([HAVE_VASPRINTF])
AC_SUBST([HAVE_ASPRINTF])

dnl 
dnl MingW requires use of pwd -W to give proper Windows (not MingW) paths 
dnl for in-place regression tests 
dnl 
case $host_os in
         *mingw*)
                 PWDREGRESS="pwd -W"
                 ;;
         *)
                 PWDREGRESS="pwd"
                 ;;
esac
AC_DEFINE_UNQUOTED([PWDREGRESS])
AC_SUBST([PWDREGRESS])


dnl ===========================================================================
dnl Find components needed to build documentation
dnl ===========================================================================
dnl
dnl Search for xsltproc which is required for building documentation
dnl

AC_PATH_PROG([IMAGEMAGICK], [convert], [])
if test "x$IMAGEMAGICK" = "x"; then
	AC_MSG_WARN([ImageMagick does not seem to be installed. Documentation cannot be built])
fi

AC_PATH_PROG([XSLTPROC], [xsltproc], [])
if test "x$XSLTPROC" = "x"; then
	AC_MSG_WARN([xsltproc is not installed so documentation cannot be built])
fi

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
	dnl If the user did not specify a directory for the docbook 
	dnl stylesheet, choose the first directory
	dnl that matches from the following list
	SEARCHPATH="
		/usr/share/sgml/docbook/xsl-stylesheets 
		/usr/share/xml/docbook/stylesheet/nwalsh
		/usr/share/sgml/docbook/stylesheet/xsl/nwalsh
		/opt/local/share/xsl/docbook-xsl
		/usr/local/share/xsl/docbook-xsl
		/usr/share/xsl/docbook-xsl
		"
	for p in ${SEARCHPATH}; do
		if test -r "${p}"/html/docbook.xsl; then
			XSLBASE_AUTO="${p}"
			break
		fi
	done

	dnl Check to see if the automatically searched paths above located a 
	dnl valid Docbook stylesheet
	if test "x$XSLBASE_AUTO" = "x"; then
		AC_MSG_WARN([could not locate Docbook stylesheets required to build the documentation])
	fi
else
	dnl The user specified an alternate directory so make sure everything 
	dnl looks sensible
	if test ! -d "$XSLBASE"; then
		AC_MSG_ERROR([the docbook stylesheet directory specified using --with-xsldir does not exist])
	fi 

	if test ! -f "$XSLBASE/html/docbook.xsl"; then
		AC_MSG_ERROR([the docbook stylesheet directory specified using --with-xsldir does not contain the html/docbook.xsl file])
	fi
fi

dnl
dnl If XSLBASE has been set then at this point we know it must be 
dnl valid and so we can just use it. If XSLBASE_AUTO has been set, and XSLBASE
dnl is empty then a valid stylesheet was found in XSLBASE_AUTO so we 
dnl should use that. Otherwise just continue silently with a blank XSLBASE
dnl variable which will trigger the error message in the documentation Makefile
dnl

if test "x$XSLBASE" = "x"; then
	if test ! "x$XSLBASE_AUTO" = "x"; then
		XSLBASE="$XSLBASE_AUTO"
	fi
fi

AC_SUBST([XSLBASE])


dnl ===========================================================================
dnl Detect CUnit if it is installed (used for unit testing)
dnl
dnl Note that we pass any specified CPPFLAGS and LDFLAGS into the Makefile 
dnl as CUnit is the only compile-time dependency that cannot obtain any 
dnl specialised flags using a --with-X parameter, and so we allow this 
dnl information to be passed in if required.
dnl ===========================================================================

CUNIT_LDFLAGS=""
AC_CHECK_HEADER([CUnit/CUnit.h], [
	CUNIT_CPPFLAGS="$CPPFLAGS"
	AC_CHECK_LIB([cunit], [CU_initialize_registry], [CUNIT_LDFLAGS="$LDFLAGS -lcunit"], [AC_MSG_WARN([could not locate CUnit required for liblwgeom unit tests])])
	],
	[
	AC_MSG_WARN([could not locate CUnit required for liblwgeom unit tests])
	])
	
AC_SUBST([CUNIT_CPPFLAGS])
AC_SUBST([CUNIT_LDFLAGS])


dnl ===========================================================================
dnl Detect iconv if it is installed (used for shp2pgsql encoding conversion 
dnl if available)
dnl ===========================================================================

ICONV_CFLAGS=""
ICONV_LDFLAGS=""

AC_ARG_WITH([libiconv], 
	[AS_HELP_STRING([--with-libiconv=PATH], [specify a path to non-default libiconv installation])], 
	[LIBICONV_PATH="$withval"], [LIBICONV_PATH=""])

LDFLAGS_SAVE="$LDFLAGS"
CFLAGS_SAVE="$CFLAGS"

if test "x$LIBICONV_PATH" != "x"; then
	AC_MSG_RESULT([checking user-specified libiconv location: $LIBICONV_PATH])
	ICONV_CFLAGS="-I$LIBICONV_PATH/include"
	ICONV_LDFLAGS="-L$LIBICONV_PATH/lib"
	LDFLAGS="$ICONV_LDFLAGS $LDFLAGS"
	CFLAGS="$ICONV_CFLAGS $CFLAGS"
fi

HAVE_ICONV_H=0
AC_CHECK_HEADER([iconv.h], [HAVE_ICONV_H=1], [])

dnl If we find the header file, try and link against the library
if test "x$HAVE_ICONV_H" = "x1"; then
	dnl libconv defines iconv_open to libiconv_open, so we'll check that directly
	AC_CHECK_LIB([iconv], [libiconv_open], [ICONV_LDFLAGS="$ICONV_LDFLAGS -liconv" HAVE_ICONV=1], [])
	if test "x$HAVE_ICONV" = "x"; then
	        dnl Check for iconv included as part of libc, using iconv_open
	        AC_CHECK_LIB([c], [iconv_open], [ICONV_LDFLAGS="$ICONV_LDFLAGS -lc" HAVE_ICONV=1], [])
		if test "x$HAVE_ICONV" = "x"; then
			dnl But it's possible this implementation of libiconv doesn't have a libiconv_* define
			AC_CHECK_LIB([iconv], [iconv_open], [ICONV_LDFLAGS="$ICONV_LDFLAGS -liconv" HAVE_ICONV=1], [])
			if test "x$HAVE_ICONV" = "x"; then
				dnl No iconv library was found; issue a warning to the console
				AC_MSG_ERROR([Could not find libiconv. Please install libiconv and libiconv-devel.])
			fi
		fi	
	fi
else
	dnl No iconv header was found; issue a warning to the console
	AC_MSG_ERROR([Could not find iconv.h header. Please install libiconv and libiconv-devel.])
fi

AC_CHECK_FUNCS([iconvctl libiconvctl],[],[])

LDFLAGS="$LDFLAGS_SAVE"
CFLAGS="$CFLAGS_SAVE"

dnl Only define HAVE_ICONV in postgis_config.h if we detect iconv sucessfully
if test "x$HAVE_ICONV" != "x"; then
	AC_DEFINE_UNQUOTED([HAVE_ICONV], [$HAVE_ICONV], [Defined if libiconv headers and library are present])
fi

AC_SUBST([ICONV_LDFLAGS])
AC_SUBST([ICONV_CFLAGS])


dnl ===========================================================================
dnl Detect the version of PostgreSQL installed on the system
dnl ===========================================================================

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


dnl ===========================================================================
dnl Ensure that $PG_CONFIG --pgxs points to a valid file. This is because some 
dnl distributions such as Debian also include pg_config as part of libpq-dev 
dnl packages, but don't install the Makefile it points to unless
dnl the postgresql-server-dev packages are installed :)
dnl ===========================================================================

PGXS=`$PGCONFIG --pgxs`
if test ! -f $PGXS; then
	AC_MSG_ERROR([the PGXS Makefile $PGXS cannot be found. Please install the PostgreSQL server development packages and re-run configure.])
fi

AC_SUBST([PGXS])


dnl Extract the version information from pg_config
dnl Note: we extract the major & minor separately, ensure they are numeric, and then combine to give
dnl the final version. This is to guard against user error... 
PGSQL_MAJOR_VERSION=`$PGCONFIG --version | sed 's/[[A-Za-z ]]*//' | cut -d. -f1 | sed 's/[[^0-9]]//g'`	
PGSQL_MINOR_VERSION=`$PGCONFIG --version | sed 's/[[A-Za-z ]]*//' | cut -d. -f2 | sed 's/[[^0-9]]//g'`	
PGSQL_FULL_VERSION=`$PGCONFIG --version`
POSTGIS_PGSQL_VERSION="$PGSQL_MAJOR_VERSION$PGSQL_MINOR_VERSION"

AC_MSG_RESULT([checking PostgreSQL version... $PGSQL_FULL_VERSION])
dnl Ensure that we are using PostgreSQL >= 8.3
if test ! "$PGSQL_MAJOR_VERSION" -ge 8; then
	AC_MSG_ERROR([PostGIS requires PostgreSQL >= 8.3])
else
	if test "$PGSQL_MAJOR_VERSION" -eq 8; then
		if test ! "$PGSQL_MINOR_VERSION" -ge 3; then
			AC_MSG_ERROR([PostGIS requires PostgreSQL >= 8.3])
		fi	
	fi
fi	

dnl Temporary hack until minimum PostgreSQL version is 9.0:
dnl If PostgreSQL < 9.0 is detected, trigger the inclusion of the new versioned PGXS targets
PGXSOVERRIDE=0
if test ! "$PGSQL_MAJOR_VERSION" -ge 9; then
	PGXSOVERRIDE=1
fi

AC_SUBST([PGXSOVERRIDE])

dnl Note: We don't need the server-side LDFLAGS or CPPFLAGS because we get these from PGXS 

dnl Extract the linker and include flags for the frontend (for programs that use libpq)
PGSQL_FE_LDFLAGS=-L`$PGCONFIG --libdir`" -lpq"
PGSQL_FE_CPPFLAGS=-I`$PGCONFIG --includedir`

AC_SUBST([PGSQL_FE_LDFLAGS])
AC_SUBST([PGSQL_FE_CPPFLAGS])

dnl Extract the include flags for the backend (libpgcommon)
PGSRV_INC=`$PGCONFIG --includedir-server`
PGSQL_BE_CPPFLAGS="-I${PGSRV_INC}"
dnl Add $PGSRV_INC/port/win32 to MinGW build to pick up netdb.h
case $host in
	*mingw32*)
		PGSQL_BE_CPPFLAGS="${PGSQL_BE_CPPFLAGS} -I${PGSRV_INC}/port/win32"
		;;
esac

AC_SUBST([PGSQL_BE_CPPFLAGS])

dnl Extract the documentation and man page directories
PGSQL_DOCDIR=`$PGCONFIG --docdir`
PGSQL_MANDIR=`$PGCONFIG --mandir`

AC_SUBST([PGSQL_DOCDIR])
AC_SUBST([PGSQL_MANDIR])

dnl Extract the executable directory
PGSQL_BINDIR=`$PGCONFIG --bindir`

AC_SUBST([PGSQL_BINDIR])

dnl Extract the share directory
PGSQL_SHAREDIR=`$PGCONFIG --sharedir`

AC_SUBST([PGSQL_SHAREDIR])

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


dnl ===========================================================================
dnl Detect libxml2 if it is installed
dnl (needed to GeomFromGML and GeomFromKML functions)
dnl ===========================================================================

AC_ARG_WITH([xml2config], 
	[AS_HELP_STRING([--with-xml2config=FILE], [specify an alternative xml2-config file])], 
	[XML2CONFIG="$withval"], [XML2CONFIG=""])

if test "x$XML2CONFIG" = "x"; then
	dnl XML2CONFIG was not specified, so search within the current path
	AC_PATH_PROG([XML2CONFIG], [xml2-config])

	dnl If we couldn't find xml2-config, display a warning
	if test "x$XML2CONFIG" = "x"; then
		AC_MSG_ERROR([could not find xml2-config from libxml2 within the current path. You may need to try re-running configure with a --with-xml2config parameter.])
	fi
else
	dnl XML2CONFIG was specified; display a message to the user
	if test "x$XML2CONFIG" = "xyes"; then
		AC_MSG_ERROR([you must specify a parameter to --with-xml2config, e.g. --with-xml2config=/path/to/xml2-config])
	else
		if test -f $XML2CONFIG; then
			AC_MSG_RESULT([Using user-specified xml2-config file: $XML2CONFIG])
		else
			AC_MSG_ERROR([the user-specified xml2-config file $XML2CONFIG does not exist])
		fi	
	fi
fi


dnl Extract the linker and include flags 
XML2_LDFLAGS=`$XML2CONFIG --libs`
XML2_CPPFLAGS=`$XML2CONFIG --cflags`

dnl Extract the version
POSTGIS_LIBXML2_VERSION=`$XML2CONFIG --version`

dnl Check headers file
CPPFLAGS_SAVE="$CPPFLAGS"
CPPFLAGS="$XML2_CPPFLAGS"
AC_CHECK_HEADERS([libxml/tree.h libxml/parser.h libxml/xpath.h libxml/xpathInternals.h],
		 [], [AC_MSG_ERROR([could not find headers include related to libxml2])])

dnl Ensure we can link against libxml2
LIBS_SAVE="$LIBS"
LIBS="$XML2_LDFLAGS"
AC_CHECK_LIB([xml2], [xmlInitParser], [], [AC_MSG_ERROR([could not find libxml2])], [])

AC_DEFINE_UNQUOTED([POSTGIS_LIBXML2_VERSION], ["$POSTGIS_LIBXML2_VERSION"], [PostGIS libxml2 version])
AC_SUBST([POSTGIS_LIBXML2_VERSION])



dnl ===========================================================================
dnl Detect the version of GEOS installed on the system
dnl ===========================================================================

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

dnl Extract the version information from geos_config
dnl Note: we extract the major & minor separately, ensure they are numeric, 
dnl and then combine to give the final version. 
dnl This is to guard against user error... 
GEOS_MAJOR_VERSION=`$GEOSCONFIG --version | cut -d. -f1 | sed 's/[[^0-9]]//g'`
GEOS_MINOR_VERSION=`$GEOSCONFIG --version | cut -d. -f2 | sed 's/[[^0-9]]//g'`
GEOS_PATCH_VERSION=`$GEOSCONFIG --version | cut -d. -f3 | sed 's/[[^0-9]]//g'`
GEOS_FULL_VERSION=`$GEOSCONFIG --version`
POSTGIS_GEOS_VERSION="$GEOS_MAJOR_VERSION$GEOS_MINOR_VERSION"
GEOS_NUMERIC_VERSION="$GEOS_MAJOR_VERSION$GEOS_MINOR_VERSION$GEOS_PATCH_VERSION"

dnl Ensure that we are using GEOS >= 3.1.0
AC_MSG_RESULT([checking GEOS version... $GEOS_FULL_VERSION])
if test ! "$GEOS_NUMERIC_VERSION" -ge 311; then
	AC_MSG_ERROR([PostGIS requires GEOS >= 3.1.1])
fi

dnl Extract the linker and include flags 
GEOS_LDFLAGS=`$GEOSCONFIG --ldflags`
GEOS_CPPFLAGS=-I`$GEOSCONFIG --includes`
AC_SUBST([GEOS_LDFLAGS])
AC_SUBST([GEOS_CPPFLAGS])

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


dnl ===========================================================================
dnl Detect gettext
dnl ===========================================================================

GETTEXT_CFLAGS=""
GETTEXT_LDFLAGS=""

AC_ARG_WITH([gettext], 
	[AS_HELP_STRING([--with-gettext=PATH], [specify a path to non-default gettext installation])], 
	[GETTEXT_PATH="$withval"], [GETTEXT_PATH=""])

LDFLAGS_SAVE="$LDFLAGS"
CFLAGS_SAVE="$CFLAGS"

if test "x$GETTEXT_PATH" != "x"; then
	AC_MSG_RESULT([checking user-specified gettext location: $GETTEXT_PATH])
	GETTEXT_CFLAGS="-I$GETTEXT_PATH/include"
	GETTEXT_LDFLAGS="-L$GETTEXT_PATH/lib"
	LDFLAGS="$GETTEXT_LDFLAGS $LDFLAGS"
	CFLAGS="$GETTEXT_CFLAGS $CFLAGS"
fi

AM_GNU_GETTEXT([external], [], [])

if test "x$LIBINTL" = "x"; then
	USE_NLS=no
fi

AC_SUBST([GETTEXT_CFLAGS])
AC_SUBST([GETTEXT_LDFLAGS])

LDFLAGS="$LDFLAGS_SAVE"
CFLAGS="$CFLAGS_SAVE"

dnl ===========================================================================
dnl Detect the version of PROJ.4 installed
dnl ===========================================================================

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

AC_SUBST([PROJ_CPPFLAGS])
AC_SUBST([PROJ_LDFLAGS])

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

dnl ===========================================================================
dnl Detect GTK+2.0 for GUI
dnl ===========================================================================

AC_ARG_WITH([gui], 
	[AS_HELP_STRING([--with-gui], [compile the data import GUI (requires GTK+2.0)])], 
	[GUI="yes"], [GUI="no"])

if test "x$GUI" = "xyes"; then
	AC_MSG_RESULT([GUI: Build requested, checking for dependencies (GKT+2.0)])
	dnl Try to find the GTK libs with pkgconfig 
	AM_PATH_GTK_2_0([2.8.0], [GTK_BUILD="gui"], [GTK_BUILD=""])

	dnl Add -mwindows to MinGW GUI build
	case $host in
		*mingw32*)
			GTK_WIN32_FLAGS=-mwindows
			GTK_WIN32_RES=shp2pgsql-gui.res
                        GTK_WIN32_BUILD=gui-win32
			;;
	esac
fi



AC_SUBST([GTK_CFLAGS])
AC_SUBST([GTK_LIBS])
AC_SUBST([GTK_WIN32_FLAGS])
AC_SUBST([GTK_WIN32_RES])
AC_SUBST([GTK_BUILD])
AC_SUBST([IGE_MAC_CFLAGS])
AC_SUBST([IGE_MAC_LIBS])



dnl ===========================================================================
dnl Allow the user to enable debugging with --enable-debug
dnl
dnl Currently we default to debug level 4. See DEBUG for more information. 
dnl

AC_ARG_ENABLE([debug], AC_HELP_STRING([--enable-debug], [Enable verbose debugging messages]), 
	[POSTGIS_DEBUG_LEVEL=4], [POSTGIS_DEBUG_LEVEL=0]) 

AC_DEFINE_UNQUOTED([POSTGIS_DEBUG_LEVEL], [$POSTGIS_DEBUG_LEVEL], [PostGIS library debug level (0=disabled)])

dnl ===========================================================================
dnl Allow the user to enable GEOS profiling with --enable-profile
dnl

AC_ARG_ENABLE([profile], AC_HELP_STRING([--enable-profile], [Enable GEOS profiling messages]), 
	[POSTGIS_PROFILE=1], [POSTGIS_PROFILE=0]) 

AC_DEFINE_UNQUOTED([POSTGIS_PROFILE], [$POSTGIS_PROFILE], [Enable GEOS profiling (0=disabled)])

dnl ===========================================================================
dnl Define version macros
dnl

POSTGIS_VERSION="$POSTGIS_MAJOR_VERSION.$POSTGIS_MINOR_VERSION USE_GEOS=1 USE_PROJ=1 USE_STATS=1"
if test "$HAVE_LIBXML2" = "1"; then
	POSTGIS_VERSION="$POSTGIS_VERSION USE_LIBXML2=1"
fi

POSTGIS_LIB_VERSION="$POSTGIS_MAJOR_VERSION.$POSTGIS_MINOR_VERSION.$POSTGIS_MICRO_VERSION"
POSTGIS_BUILD_DATE=`date -u "+%Y-%m-%d %H:%M:%S"`
POSTGIS_SCRIPTS_SVNREV=`perl utils/read_scripts_version.pl`
POSTGIS_SCRIPTS_VERSION="$POSTGIS_MAJOR_VERSION.$POSTGIS_MINOR_VERSION r$POSTGIS_SCRIPTS_SVNREV"

AC_DEFINE_UNQUOTED([POSTGIS_VERSION], ["$POSTGIS_VERSION"], [PostGIS version])
AC_DEFINE_UNQUOTED([POSTGIS_LIB_VERSION], ["$POSTGIS_LIB_VERSION"], [PostGIS library version])
AC_DEFINE_UNQUOTED([POSTGIS_BUILD_DATE], ["$POSTGIS_BUILD_DATE"], [PostGIS build date])
AC_DEFINE_UNQUOTED([POSTGIS_SCRIPTS_VERSION], ["$POSTGIS_SCRIPTS_VERSION"], [PostGIS scripts version])

AC_SUBST([POSTGIS_VERSION])
AC_SUBST([POSTGIS_LIB_VERSION])
AC_SUBST([POSTGIS_BUILD_DATE])
AC_SUBST([POSTGIS_SCRIPTS_VERSION])


dnl ===========================================================================
dnl Other parameters 
dnl

dnl Always enable BBOX caching by default
AC_DEFINE_UNQUOTED([POSTGIS_AUTOCACHE_BBOX], [1], [Enable caching of bounding box within geometries])

dnl Always enable use of ANALYZE statistics by default
AC_DEFINE_UNQUOTED([POSTGIS_USE_STATS], [1], [Enable use of ANALYZE statistics])


CPPFLAGS="$PGSQL_CPPFLAGS $GEOS_CPPFLAGS $PROJ_CPPFLAGS $XML2_CPPFLAGS"
dnl AC_MSG_RESULT([CPPFLAGS: $CPPFLAGS])

SHLIB_LINK="$PGSQL_LDFLAGS $GEOS_LDFLAGS $PROJ_LDFLAGS -lgeos_c -lproj $XML2_LDFLAGS"
AC_SUBST([SHLIB_LINK])
dnl AC_MSG_RESULT([SHLIB_LINK: $SHLIB_LINK])

dnl ====================================
dnl topology stuff
dnl ====================================
AC_ARG_WITH([topology],
    [AS_HELP_STRING([--with-topology],
                    [compile the topology extension @<:@default=no@:>@])],
    [TOPOLOGY="topology"], [TOPOLOGY=""])

if test "x$TOPOLOGY" = "xtopology"; then
    AC_MSG_RESULT([TOPOLOGY: Topology support requested])
fi

dnl
dnl Topology Version Information imported from topology/Version.config 
dnl
POSTGIS_TOPOLOGY_MAJOR_VERSION=`cat topology/Version.config | grep POSTGIS_TOPOLOGY_MAJOR_VERSION | sed 's/[[^=]]*=\([[0-9]]\)/\1/g'`
POSTGIS_TOPOLOGY_MINOR_VERSION=`cat topology/Version.config | grep POSTGIS_TOPOLOGY_MINOR_VERSION | sed 's/[[^=]]*=\([[0-9]]\)/\1/g'`
POSTGIS_TOPOLOGY_MICRO_VERSION=`cat topology/Version.config | grep POSTGIS_TOPOLOGY_MICRO_VERSION | sed 's/[[^=]]*=\([[0-9]]\)/\1/g'`

POSTGIS_TOPOLOGY_VERSION="$POSTGIS_TOPOLOGY_MAJOR_VERSION.$POSTGIS_TOPOLOGY_MINOR_VERSION.$POSTGIS_TOPOLOGY_MICRO_VERSION"

AC_SUBST([POSTGIS_TOPOLOGY_VERSION])
AC_SUBST([TOPOLOGY])


dnl ====================================
dnl raster stuff
dnl ====================================
AC_ARG_WITH([raster],
    [AS_HELP_STRING([--with-raster],
                    [compile the raster extension (requires GDAL >= 1.6.0) @<:@default=no@:>@])],
    [RASTER="raster"], [RASTER=""])

if test "x$RASTER" = "xraster"; then dnl # {
    AC_MSG_RESULT([RASTER: Raster support requested])

    AC_CONFIG_HEADERS([raster/raster_config.h])

    dnl
    dnl Raster Version Information imported from raster/Version.config 
    dnl
    POSTGIS_RASTER_MAJOR_VERSION=`cat raster/Version.config | grep POSTGIS_RASTER_MAJOR_VERSION | sed 's/[[^=]]*=\([[0-9]]\)/\1/g'`
    POSTGIS_RASTER_MINOR_VERSION=`cat raster/Version.config | grep POSTGIS_RASTER_MINOR_VERSION | sed 's/[[^=]]*=\([[0-9]]\)/\1/g'`
    POSTGIS_RASTER_MICRO_VERSION=`cat raster/Version.config | grep POSTGIS_RASTER_MICRO_VERSION | sed 's/[[^=]]*=\([[0-9]]\)/\1/g'`

    AC_DEFINE_UNQUOTED([POSTGIS_RASTER_MAJOR_VERSION], ["$POSTGIS_RASTER_MAJOR_VERSION"], [PostGIS Raster major version])
    AC_DEFINE_UNQUOTED([POSTGIS_RASTER_MINOR_VERSION], ["$POSTGIS_RASTER_MINOR_VERSION"], [PostGIS Raster minor version])
    AC_DEFINE_UNQUOTED([POSTGIS_RASTER_MICRO_VERSION], ["$POSTGIS_RASTER_MICRO_VERSION"], [PostGIS Raster micro version])

    AC_SUBST([POSTGIS_RASTER_MAJOR_VERSION])
    AC_SUBST([POSTGIS_RASTER_MINOR_VERSION])
    AC_SUBST([POSTGIS_RASTER_MICRO_VERSION])

    dnl ==============================
    dnl Define raster version macros
    dnl ==============================
    POSTGIS_RASTER_VERSION="$POSTGIS_RASTER_MAJOR_VERSION.$POSTGIS_RASTER_MINOR_VERSION"
    POSTGIS_RASTER_LIB_VERSION="$POSTGIS_RASTER_MAJOR_VERSION.$POSTGIS_RASTER_MINOR_VERSION.$POSTGIS_RASTER_MICRO_VERSION"
    POSTGIS_RASTER_BUILD_DATE=`date -u "+%Y-%m-%d %H:%M:%S"`
    POSTGIS_RASTER_SCRIPTS_VERSION="$POSTGIS_RASTER_LIB_VERSION"

    AC_DEFINE_UNQUOTED([POSTGIS_RASTER_VERSION], ["$POSTGIS_RASTER_VERSION"], [WKTRaster version])
    AC_DEFINE_UNQUOTED([POSTGIS_RASTER_LIB_VERSION], ["$POSTGIS_RASTER_LIB_VERSION"], [WKTRaster library version])
    AC_DEFINE_UNQUOTED([POSTGIS_RASTER_BUILD_DATE], ["$POSTGIS_RASTER_BUILD_DATE"], [WKTRaster build date])
    AC_DEFINE_UNQUOTED([POSTGIS_RASTER_SCRIPTS_VERSION], ["$POSTGIS_RASTER_SCRIPTS_VERSION"], [WKTRaster scripts version])

    AC_SUBST([POSTGIS_RASTER_VERSION])
    AC_SUBST([POSTGIS_RASTER_LIB_VERSION])
    AC_SUBST([POSTGIS_RASTER_BUILD_DATE])
    AC_SUBST([POSTGIS_RASTER_SCRIPTS_VERSION])

    dnl ================================================
    dnl Vars needed for raster extension
    dnl ================================================
    POSTGIS_SRCDIR=`$PWDREGRESS`
    LIBLWGEOM_CFLAGS="-I${POSTGIS_SRCDIR}/liblwgeom"
    LIBLWGEOM_LDFLAGS="${POSTGIS_SRCDIR}/liblwgeom/.libs/liblwgeom.a"
    LIBPGCOMMON_CFLAGS="-I${POSTGIS_SRCDIR}/libpgcommon"
    LIBPGCOMMON_LDFLAGS="${POSTGIS_SRCDIR}/libpgcommon/libpgcommon.a"

    AC_SUBST([POSTGIS_SRCDIR])
    AC_SUBST([LIBLWGEOM_CFLAGS])
    AC_SUBST([LIBLWGEOM_LDFLAGS])
    AC_SUBST([LIBPGCOMMON_CFLAGS])
    AC_SUBST([LIBPGCOMMON_LDFLAGS])

    dnl ========================================================================
    dnl Determine GDAL Support
    dnl
    dnl TODO: Now, --with-gdalconfig can have only 1 value: path to gdal-config. It
    dnl could be useful to allow path to GDAL tree, because the cflags and the
    dnl libs can be obtained from GDAL tree too, apart from gdal-config
    dnl How to get cflags and libs from GDAL tree?
    dnl
    dnl LIBGDAL_CFLAGS="-I$with_gdal/port -I$with_gdal/ogr -I$with_gdal/alg -I$with_gdal/gcore -I$with_gdal/frmts "
    dnl LIBGDAL_LDFLAGS="-L${with_gdal}/.libs -lgdal -L${with_gdal}/ -lgdal"
    dnl ========================================================================
    dnl not used right now
    USE_GDAL_SOURCE_TREE="no"
    LIBGDAL_CFLAGS=""
    LIBGDAL_LDFLAGS=""

	dnl GDAL version constants, update here
	GDAL_MIN_VERSION=1.6.0
	GDAL_MIN_VERSION_NUMBER=160 dnl TODO: Use GDAL version calculation MAJOR*1000+MINOR*100+REV*10+BUILD

    AC_ARG_WITH([gdal],
		AC_HELP_STRING([--with-gdalconfig=@<:@ARG@:>@],[specify location of gdal-config (ARG=path)]),
        	[GDAL_CONFIG="$withval"], [GDAL_CONFIG=""])

    AC_MSG_CHECKING([for GDAL >= $GDAL_MIN_VERSION])
    if test "x$GDAL_CONFIG" = "x"; then
	   	dnl If GDAL_CONFIG path not provided, try to find it
        GDAL_CONFIG_TMP=`which gdal-config`
        if test "`basename xx/${GDAL_CONFIG_TMP}`" = "gdal-config" ; then
            GDAL_CONFIG=${GDAL_CONFIG_TMP}
        fi
    fi

    if test -x "$GDAL_CONFIG"; then
       dnl Extract the version information from gdal-config
       dnl Note: we extract the major & minor separately, ensure they are numeric,
       dnl and then combine to give the final version.
       dnl This is to guard against user error...
       GDAL_MAJOR_VERSION=`$GDAL_CONFIG --version | cut -d. -f1 | sed 's/[[^0-9]]//g'`
       GDAL_MINOR_VERSION=`$GDAL_CONFIG --version | cut -d. -f2 | sed 's/[[^0-9]]//g'`
       GDAL_PATCH_VERSION=`$GDAL_CONFIG --version | cut -d. -f3 | sed 's/[[^0-9]]//g'`
       GDAL_FULL_VERSION=`$GDAL_CONFIG --version`
       POSTGIS_GDAL_VERSION="$GDAL_MAJOR_VERSION$GDAL_MINOR_VERSION"
       GDAL_VERSION_NUMBER="$GDAL_MAJOR_VERSION$GDAL_MINOR_VERSION$GDAL_PATCH_VERSION"

       dnl Ensure we are using minimum required version of GDAL
       if test ! "$GDAL_VERSION_NUMBER" -ge "$GDAL_MIN_VERSION_NUMBER" ; then
	   	  AC_MSG_RESULT([not found])
	   	  AC_MSG_ERROR([PostGIS requires GDAL >= $GDAL_MIN_VERSION])
	   fi

	else
		AC_MSG_RESULT([not found])
		AC_MSG_ERROR([gdal-config not found. Try --with-gdalconfig=<path to gdal-config>])
	fi
	AC_MSG_RESULT([found])

	dnl Check if the new polygonize function is present
	AC_CHECK_LIB([gdal], [GDALFPolygonize], [AC_DEFINE_UNQUOTED([GDALFPOLYGONIZE], [1])], [AC_DEFINE_UNQUOTED([GDALFPOLYGONIZE], [0])], []) 

	dnl Determine if GDAL Python bindings are available
	AC_MSG_CHECKING([for GDAL Python bindings])
	GDAL_PYTHON_TEST='try:
  from osgeo import gdal
  print 1
except ImportError:
  pass'

	GDAL_PYTHON_FOUND=`python -c "$GDAL_PYTHON_TEST"`
	if test "$GDAL_PYTHON_FOUND" = "1"; then
	    AC_MSG_RESULT([found])
	else	
		AC_MSG_RESULT([not found])
	    AC_MSG_ERROR([GDAL Python bindings required by raster2pgsql.py loader])
  	fi

	dnl Determine if GDAL Python bindings come with NumPy support
	AC_MSG_CHECKING([for NumPy array support in GDAL Python bindings])
	GDAL_NUMPY_TEST='try:
  from osgeo import gdal_array
  print 1
except ImportError:
  pass'

	GDAL_NUMPY_FOUND=`python -c "$GDAL_NUMPY_TEST"`
	if test "$GDAL_NUMPY_FOUND" = "1"; then
	    AC_MSG_RESULT([found])
	else	
		AC_MSG_RESULT([not found])
	    AC_MSG_ERROR([GDAL Python bindings with NumPy array support required by raster2pgsql.py loader])
  	fi

    dnl Extract the linker and include flags 
    LIBGDAL_LDFLAGS=`$GDAL_CONFIG --libs`
    LIBGDAL_CFLAGS=`$GDAL_CONFIG --cflags`

    AC_SUBST([LIBGDAL_CFLAGS])
    AC_SUBST([LIBGDAL_LDFLAGS])
	AC_SUBST([GDALFPOLYGONIZE])

    dnl Define raster objects, for makefiles
    RT_CORE_LIB=corelib
    RT_PG_LIB=pglib
    RT_POSTGIS_SQL=rtpostgis.sql

    AC_SUBST([RASTER])
    AC_SUBST([RT_CORE_LIB])
    AC_SUBST([RT_PG_LIB])
    AC_SUBST([RT_POSTGIS_SQL])

    RT_MAKEFILE_LIST="
                    raster/Makefile \
                    raster/rt_core/Makefile \ 
                    raster/rt_pg/Makefile \ 
                    raster/test/Makefile \
                    raster/test/core/Makefile \ 
                    raster/test/regress/Makefile \ 
                    raster/scripts/python/Makefile"

else dnl # }{
    RT_MAKEFILE_LIST="raster/Makefile"
fi dnl # }


dnl Output the relevant files
AC_OUTPUT([GNUmakefile 
        liblwgeom/Makefile liblwgeom/cunit/Makefile liblwgeom/liblwgeom.h
        libpgcommon/Makefile libpgcommon/cunit/Makefile postgis/Makefile
        postgis/sqldefines.h loader/Makefile loader/cunit/Makefile
        topology/Makefile regress/Makefile doc/Makefile doc/Makefile.comments
        doc/html/image_src/Makefile $RT_MAKEFILE_LIST])
    
dnl ===========================================================================
dnl Display the configuration status information
dnl ===========================================================================

AC_MSG_RESULT()
AC_MSG_RESULT([  PostGIS is now configured for ${host}])
AC_MSG_RESULT()
AC_MSG_RESULT([ -------------- Compiler Info ------------- ])
AC_MSG_RESULT([  C compiler:           ${CC} ${CFLAGS}])
AC_MSG_RESULT([  C++ compiler:         ${CXX} ${CXXFLAGS}])
AC_MSG_RESULT()
AC_MSG_RESULT([ -------------- Dependencies -------------- ])
AC_MSG_RESULT([  GEOS config:          ${GEOSCONFIG}])
AC_MSG_RESULT([  GEOS version:         ${GEOS_FULL_VERSION}])
if test "x$RASTER" = "xraster"; then
    AC_MSG_RESULT([  GDAL config:          ${GDAL_CONFIG}])
    AC_MSG_RESULT([  GDAL version:         ${GDAL_FULL_VERSION}])
fi
AC_MSG_RESULT([  PostgreSQL config:    ${PGCONFIG}])
AC_MSG_RESULT([  PostgreSQL version:   ${PGSQL_FULL_VERSION}])
AC_MSG_RESULT([  PROJ4 version:        ${POSTGIS_PROJ_VERSION}])
AC_MSG_RESULT([  Libxml2 config:       ${XML2CONFIG}])
AC_MSG_RESULT([  Libxml2 version:      ${POSTGIS_LIBXML2_VERSION}])
AC_MSG_RESULT([  PostGIS debug level:  ${POSTGIS_DEBUG_LEVEL}])
if test "x$RASTER" = "xraster" -o "x$TOPOLOGY" = "xtopology"; then
    AC_MSG_RESULT([ -------------- Extensions -------------- ])
if test "x$RASTER" = "xraster"; then
    AC_MSG_RESULT([  PostGIS Raster:       ${POSTGIS_RASTER_LIB_VERSION}])
fi
if test "x$TOPOLOGY" = "xtopology"; then
    AC_MSG_RESULT([  PostGIS Topology:     ${POSTGIS_TOPOLOGY_VERSION}])
fi
fi
AC_MSG_RESULT()
AC_MSG_RESULT([ -------- Documentation Generation -------- ])
AC_MSG_RESULT([  xsltproc:             ${XSLTPROC}])
AC_MSG_RESULT([  xsl style sheets:     ${XSLBASE}])
AC_MSG_RESULT([  dblatex:              ${DBLATEX}])
AC_MSG_RESULT([  convert:              ${IMAGEMAGICK}])
AC_MSG_RESULT()
