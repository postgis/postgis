dnl **********************************************************************
dnl *
dnl * PostGIS - Spatial Types for PostgreSQL
dnl * http://postgis.net
dnl * Copyright 2008 Mark Cave-Ayland
dnl *
dnl * This is free software; you can redistribute and/or modify it under
dnl * the terms of the GNU General Public Licence. See the COPYING file.
dnl *
dnl **********************************************************************

AC_INIT()
AC_CONFIG_HEADERS([postgis_config.h])
AC_CONFIG_MACRO_DIR([macros])
AC_CONFIG_AUX_DIR([build-aux])
AC_PROG_INSTALL


dnl Invoke libtool: we do this as it is the easiest way to find the PIC
dnl flags required to build liblwgeom
AC_PROG_LIBTOOL

dnl
dnl Compilers
dnl
AC_PROG_CC
AC_PROG_CPP
AC_PROG_GREP
AC_C_BIGENDIAN

dnl
dnl SQL Preprocessor
dnl
AC_PATH_PROG([CPPBIN], [cpp], [])
if test "x$CPPBIN" != "x"; then
  SQLPP="${CPPBIN} -traditional-cpp -w -P"
else
  AC_PATH_PROG([GPP], [gpp_], [])
  if test "x$GPP" != "x"; then
    SQLPP="${GPP} -C -s \'" dnl Use better string support
  else
    if test "x${CPP}" != "x"; then
      SQLPP="${CPP} -traditional-cpp"
    else
		  AC_MSG_ERROR([Required "cpp" command not found])
    fi
  fi
fi
AC_SUBST([SQLPP])

dnl
dnl Silence warning: ar: 'u' modifier ignored since 'D' is the default
dnl
AC_SUBST(AR_FLAGS, [cr])

dnl
dnl Define PIC flags in PICFLAGS (note: this variable is set as part of libtool initialisation above)
dnl
PICFLAGS="$lt_prog_compiler_pic"
AC_SUBST([PICFLAGS])

AC_LIBTOOL_COMPILER_OPTION([if $compiler supports -fno-math-errno], [_cv_nomatherrno], [-fno-math-errno], [], [CFLAGS="$CFLAGS -fno-math-errno"], [])
AC_LIBTOOL_COMPILER_OPTION([if $compiler supports -fno-signed-zeros], [_cv_nosignedzeros], [-fno-signed-zeros], [], [CFLAGS="$CFLAGS -fno-signed-zeros"], [])
AC_LIBTOOL_COMPILER_OPTION([if $compiler supports -std=gnu99], [_cv_std], -std=gnu99, [], [CFLAGS="-std=gnu99 $CFLAGS"], [])


dnl
dnl Add libm to LDFLAGS so we don't need to add it everywhere manually
dnl
LDFLAGS="$LDFLAGS -lm"

dnl
dnl Exporting used library symbols in the module is a source of issues,
dnl see https://trac.osgeo.org/postgis/ticket/3281
dnl
EXCLUDELIBS_LDFLAGS=""
AC_LIBTOOL_LINKER_OPTION([if $compiler supports --exclude-libs], [_cv_exclude_libs], [[-Wl,--exclude-libs,ALL]], [EXCLUDELIBS_LDFLAGS="-Wl,--exclude-libs,ALL"])
AC_SUBST([EXCLUDELIBS_LDFLAGS])

dnl
dnl Define executable suffix for use with the loader Makefiles
dnl
EXESUFFIX="$ac_cv_exeext"
AC_SUBST([EXESUFFIX])

dnl
dnl Version Information imported from Version.config
dnl
POSTGIS_MAJOR_VERSION=`cat Version.config | grep ^POSTGIS_MAJOR_VERSION | sed 's/[[^=]]*=\([[0-9]]\)/\1/g'`
POSTGIS_MINOR_VERSION=`cat Version.config | grep ^POSTGIS_MINOR_VERSION | sed 's/[[^=]]*=\([[0-9]]\)/\1/g'`
POSTGIS_MICRO_VERSION=`cat Version.config | grep ^POSTGIS_MICRO_VERSION | sed 's/[[^=]]*=\([[0-9]]\)/\1/g'`

AC_DEFINE_UNQUOTED([POSTGIS_MAJOR_VERSION], ["$POSTGIS_MAJOR_VERSION"], [PostGIS major version])
AC_DEFINE_UNQUOTED([POSTGIS_MINOR_VERSION], ["$POSTGIS_MINOR_VERSION"], [PostGIS minor version])
AC_DEFINE_UNQUOTED([POSTGIS_MICRO_VERSION], ["$POSTGIS_MICRO_VERSION"], [PostGIS micro version])

AC_SUBST([POSTGIS_MAJOR_VERSION])
AC_SUBST([POSTGIS_MINOR_VERSION])
AC_SUBST([POSTGIS_MICRO_VERSION])

dnl =====================================================
dnl Include minor version in postgis extension libraries
dnl =====================================================
LIBINCLUDEMINORVERSION="no"
AC_ARG_WITH([library-minor-version],
    [AS_HELP_STRING([--with-library-minor-version],
                    [Include minor version in the PostgreSQL PostGIS library files])],
    [LIBINCLUDEMINORVERSION="yes"], [])

if test "x$with-library-minor-version" = "xno"; then
    LIBINCLUDEMINORVERSION="no"
fi

AC_SUBST([LIBINCLUDEMINORVERSION])


dnl
dnl Liblwgeom version information imported from Version.config
dnl
LIBLWGEOM_CURRENT=`cat Version.config | grep ^LIBLWGEOM_IFACE_CUR | sed 's/[[^=]]*=\([[0-9]]\)/\1/g'`
LIBLWGEOM_AGE=`cat Version.config | grep ^LIBLWGEOM_IFACE_AGE | sed 's/[[^=]]*=\([[0-9]]\)/\1/g'`
LIBLWGEOM_REV=`cat Version.config | grep ^LIBLWGEOM_IFACE_REV | sed 's/[[^=]]*=\([[0-9]]\)/\1/g'`
AC_SUBST([LIBLWGEOM_CURRENT])
AC_SUBST([LIBLWGEOM_AGE])
AC_SUBST([LIBLWGEOM_REV])


dnl
dnl Search for flex/bison to build the parser
dnl

AC_PROG_LEX
AC_PATH_PROG(YACC, bison)
AC_SUBST([LEX])
AC_SUBST([YACC])

dnl
dnl Search for OS-specific headers
dnl
AC_CHECK_HEADER([ieeefp.h], [HAVE_IEEEFP_H=1], [HAVE_IEEEFP_H=0])
AC_DEFINE_UNQUOTED([HAVE_IEEEFP_H], [$HAVE_IEEEFP_H], [ieeefp.h header])
AC_CHECK_HEADER([termios.h], [HAVE_TERMIOS_H=1], [HAVE_TERMIOS_H=0])
AC_DEFINE_UNQUOTED([HAVE_TERMIOS_H], [$HAVE_TERMIOS_H], [termios.h header])


dnl
dnl Check for platform-specific functions
dnl
AC_CHECK_FUNC(vasprintf, AC_DEFINE([HAVE_VASPRINTF]))
AC_CHECK_FUNC(asprintf, AC_DEFINE([HAVE_ASPRINTF]))
AC_FUNC_FSEEKO()

dnl
dnl MingW requires use of pwd -W to give proper Windows (not MingW) paths
dnl for in-place regression tests
dnl
case $host_os in
         *mingw*)
                 MINGWBUILD=1
                 ;;
         *)
                 MINGWBUILD=0
                 ;;
esac
AC_SUBST([MINGWBUILD])

AC_PATH_PROG([PERL], [perl], [])
if test "x$PERL" = "x"; then
        AC_MSG_ERROR([Perl was not found. Building PostGIS requires Perl.])
fi
AC_SUBST([PERL])

dnl ===========================================================================
dnl Find components needed to build documentation
dnl ===========================================================================
dnl
dnl Search for xsltproc which is required for building documentation
dnl

CAN_BUILD_COMMENTS=yes

AC_PATH_PROG([XSLTPROC], [xsltproc], [])
if test "x$XSLTPROC" = "x"; then
	AC_MSG_WARN([xsltproc is not installed so documentation cannot be built])
  CAN_BUILD_COMMENTS=no
fi

CAN_BUILD_PDF=${CAN_BUILD_COMMENTS}

AC_PATH_PROG([IMAGEMAGICK], [convert], [])
if test "x$IMAGEMAGICK" = "x"; then
	AC_MSG_WARN([ImageMagick does not seem to be installed. Documentation cannot be built])
  CAN_BUILD_PDF=no
fi

AC_PATH_PROG([DBLATEX], [dblatex], [])
if test "x$DBLATEX" = "x"; then
	AC_MSG_WARN([dblatex is not installed so PDF documentation cannot be built])
  CAN_BUILD_PDF=no
fi

AC_SUBST(CAN_BUILD_COMMENTS)
AC_SUBST(CAN_BUILD_PDF)

AC_PATH_PROG([XMLLINT], [xmllint], [])
if test "x$XMLLINT" = "x"; then
	AC_MSG_WARN([xmllint is not installed so documentation cannot be checked])
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
		/usr/share/xml/docbook/stylesheet/docbook-xsl
		/usr/share/xml/docbook/stylesheet/nwalsh
		/usr/share/sgml/docbook/stylesheet/xsl/nwalsh
		/opt/local/share/xsl/docbook-xsl
		/usr/local/opt/docbook-xsl/docbook-xsl
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

dnl
dnl Let's see if we can find mathml2.dtd
dnl
AC_ARG_WITH([mathmldtd],
	[AS_HELP_STRING([--with-mathmldtd=PATH], [specify the dtd path for mathml2.dtd])],
	[MATHML2_DTD="$withval"], [MATHML2_DTD=""])
dnl TODO: get more paths to add here...
if test "x$MATHML2_DTD" = "x"; then
	MATHML2_DTD="http://www.w3.org/Math/DTD/mathml2/mathml2.dtd"
	SEARCHPATH="
			/usr/share/xml/schema/w3c/mathml/dtd
			"
	for p in ${SEARCHPATH}; do
		if test -r "${p}"/mathml2.dtd; then
			MATHML2_DTD="${p}/mathml2.dtd"
			break
		fi
	done
fi
AC_SUBST([MATHML2_DTD])

dnl ===========================================================================
dnl Detect if pkg-config installed
dnl ===========================================================================
# check for pkg-config
PKG_PROG_PKG_CONFIG
if test -z "$PKG_CONFIG"; then
   AC_MSG_WARN([Cannot find pkg-config, make sure it is installed in your PATH])
fi


dnl ===========================================================================
dnl Detect CUnit if it is installed (used for unit testing)
dnl
dnl Note that we pass any specified CPPFLAGS and LDFLAGS into the Makefile
dnl as CUnit is the only compile-time dependency that cannot obtain any
dnl specialised flags using a --with-X parameter, and so we allow this
dnl information to be passed in if required.
dnl ===========================================================================

CUNIT_CPPFLAGS=""
CUNIT_LDFLAGS=""
if test ! -z "$PKG_CONFIG"; then
	PKG_CHECK_MODULES([CUNIT], [cunit],
		[
			CUNIT_CPPFLAGS="$CPPFLAGS $CUNIT_CFLAGS"
			CUNIT_LDFLAGS="$LDFLAGS $CUNIT_LIBS"
		],
		[AC_MSG_WARN([could not locate CUnit required for unit tests])])
else
	AC_CHECK_HEADER([CUnit/CUnit.h],
		[
			CUNIT_CPPFLAGS="$CPPFLAGS"
			AC_CHECK_LIB([cunit], [CU_initialize_registry], [CUNIT_LDFLAGS="$LDFLAGS -lcunit"], [AC_MSG_WARN([could not locate CUnit required for unit tests])])
		],
		[AC_MSG_WARN([could not locate CUnit required for unit tests])])
fi

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

LIBLWGEOM_ONLY="no"
AC_SUBST([LIBLWGEOM_ONLY])

dnl ===========================================================================
dnl Detect the version of PostgreSQL installed on the system, if needed
dnl ===========================================================================

AC_ARG_VAR(PG_CONFIG, [PostgreSQL configure command to determine Postgres version to build against.])
AC_ARG_WITH([pgconfig],
	[AS_HELP_STRING([--with-pgconfig=FILE], [specify an alternative pg_config file or disable postgresql (--without-pgconfig)])],
	[PG_CONFIG="$withval"], [])

if test "x$PG_CONFIG" = "xno"; then
	LIBLWGEOM_ONLY="yes"
	AC_MSG_RESULT([PostgreSQL support disabled])

elif test "x$PG_CONFIG" = "x"; then
	dnl PG_CONFIG was not specified, so search within the current path
	AC_PATH_PROG([PG_CONFIG], [pg_config])

	dnl If we couldn't find pg_config, display an error
	if test "x$PG_CONFIG" = "x"; then
		AC_MSG_ERROR([could not find pg_config within the current path. You may need to re-run configure with a --with-pgconfig parameter.])
	fi
else
	dnl PG_CONFIG was specified; display a message to the user
	if test "x$PG_CONFIG" = "xyes"; then
		AC_MSG_ERROR([you must specify a parameter to --with-pgconfig, e.g. --with-pgconfig=/path/to/pg_config])
	else
		if test -f "$PG_CONFIG"; then
			AC_MSG_RESULT([Using user-specified pg_config file: $PG_CONFIG])
		else
			AC_MSG_ERROR([the user-specified pg_config file $PG_CONFIG does not exist])
		fi
	fi
fi

if test "x$LIBLWGEOM_ONLY" = "xno"; then

  dnl ===========================================================================
  dnl Ensure that $PG_CONFIG --pgxs points to a valid file. This is because some
  dnl distributions such as Debian also include pg_config as part of libpq-dev
  dnl packages, but don't install the Makefile it points to unless
  dnl the postgresql-server-dev packages are installed :)
  dnl ===========================================================================

  PGXS=`"$PG_CONFIG" --pgxs`
  if test "x$PGXS" = "x" -o ! -f "$PGXS"; then
    AC_MSG_ERROR([the PGXS Makefile $PGXS cannot be found. Please install the PostgreSQL server development packages and re-run configure.])
  fi

  AC_SUBST([PGXS])

  dnl Extract the version information from pg_config
  dnl Note: we extract the major & minor separately, ensure they are numeric, and then combine to give
  dnl the final version. This is to guard against user error...
  PGSQL_FULL_VERSION=`"$PG_CONFIG" --version`

  PGSQL_MAJOR_VERSION=`echo $PGSQL_FULL_VERSION | sed 's/[[^0-9]]*\([[0-9]]*\).*/\1/'`
  PGSQL_MINOR_VERSION=`$PG_CONFIG --version | sed 's/[[^0-9]]*\([[0-9]]\)\.\([[0-9]]\).*/\2/'`

  if test $PGSQL_MAJOR_VERSION -gt 9; then
  	dnl ==================================================================
  	dnl Starting with PostgreSQL 10, major is the new minor
  	dnl This is to prevent things like 10.31 ranking higher than 11.0
  	dnl===================================================================
  	PGSQL_MINOR_VERSION=0
  fi

  POSTGIS_PGSQL_VERSION="$PGSQL_MAJOR_VERSION$PGSQL_MINOR_VERSION"
  POSTGIS_PGSQL_HR_VERSION="$PGSQL_MAJOR_VERSION.$PGSQL_MINOR_VERSION"

  PGSQL_PKGLIBDIR=`"$PG_CONFIG" --pkglibdir`
  PGSQL_LIBDIR=`"$PG_CONFIG" --libdir`
  PGSQL_SHAREDIR=`"$PG_CONFIG" --sharedir`

  AC_MSG_RESULT([checking PostgreSQL version... $PGSQL_FULL_VERSION])
  dnl Ensure that we are using PostgreSQL >= 9.6
  if test $POSTGIS_PGSQL_VERSION -lt 96; then
    AC_MSG_ERROR([PostGIS requires PostgreSQL >= 9.6])
  fi

  HAVE_SPGIST=no
  if test $POSTGIS_PGSQL_VERSION -gt 100; then
    HAVE_SPGIST=yes
  fi

  dnl Note: We don't need the server-side LDFLAGS or CPPFLAGS because we get these from PGXS

  dnl Extract the linker and include flags for the frontend (for programs that use libpq)
  PGSQL_FE_LDFLAGS=-L`"$PG_CONFIG" --libdir`" -lpq"
  PGSQL_FE_CPPFLAGS=-I`"$PG_CONFIG" --includedir`

  AC_SUBST([PGSQL_FE_LDFLAGS])
  AC_SUBST([PGSQL_FE_CPPFLAGS])

  dnl Extract the include flags for the backend (libpgcommon)
  PGSRV_INC=`"$PG_CONFIG" --includedir-server`
  PGSQL_BE_CPPFLAGS="-I${PGSRV_INC}"
  dnl Add $PGSRV_INC/port/win32 to MinGW build to pick up netdb.h
  case $host in
    *mingw32*)
      PGSQL_BE_CPPFLAGS="${PGSQL_BE_CPPFLAGS} -I${PGSRV_INC}/port/win32"
      ;;
  esac

  AC_SUBST([PGSQL_BE_CPPFLAGS])

  dnl Extract the documentation and man page directories
  PGSQL_DOCDIR=`"$PG_CONFIG" --docdir`
  PGSQL_MANDIR=`"$PG_CONFIG" --mandir`

  AC_SUBST([PGSQL_DOCDIR])
  AC_SUBST([PGSQL_MANDIR])

  dnl Extract the locale directory
  PGSQL_LOCALEDIR=`"$PG_CONFIG" --localedir`
  AC_DEFINE_UNQUOTED([PGSQL_LOCALEDIR], ["$PGSQL_LOCALEDIR"], [Location of PostgreSQL locale directory])

  dnl Extract the executable directory
  PGSQL_BINDIR=`"$PG_CONFIG" --bindir`

  AC_SUBST([PGSQL_BINDIR])

  dnl Extract the share directory
  PGSQL_SHAREDIR=`"$PG_CONFIG" --sharedir`

  AC_SUBST([PGSQL_SHAREDIR])

  dnl Ensure that we can parse libpq-fe.h
  CPPFLAGS_SAVE="$CPPFLAGS"
  CPPFLAGS="$PGSQL_FE_CPPFLAGS"
  AC_CHECK_HEADER([libpq-fe.h], [], [AC_MSG_ERROR([could not find libpq-fe.h])])
  CPPFLAGS="$CPPFLAGS_SAVE"

  dnl Ensure we can link against libpq
  LIBS_SAVE="$LIBS"
  LIBS="$PGSQL_FE_LDFLAGS"

  CC_SAVE="$CC"
  CC=`"$PG_CONFIG" --cc`

  CFLAGS_SAVE="$CFLAGS"
  CFLAGS=`"$PG_CONFIG" --cflags`

  AC_CHECK_LIB([pq], [PQserverVersion],
    [],
    [AC_MSG_ERROR([could not find libpq])],
    [])
  LIBS="$LIBS_SAVE"
  CC="$CC_SAVE"
  CFLAGS="$CFLAGS_SAVE"

  AC_DEFINE_UNQUOTED([POSTGIS_PGSQL_VERSION], [$POSTGIS_PGSQL_VERSION], [PostgreSQL server version])
  AC_SUBST([POSTGIS_PGSQL_VERSION])
  AC_DEFINE_UNQUOTED([POSTGIS_PGSQL_HR_VERSION], [$POSTGIS_PGSQL_HR_VERSION], [PostgreSQL human readable server version])
  AC_SUBST([POSTGIS_PGSQL_HR_VERSION])
  AC_SUBST([HAVE_SPGIST])

fi dnl LIBLWGEOM_ONLY != no


dnl ===========================================================================
dnl Explain our prefix policy if necessary.
dnl ===========================================================================

if test "$prefix" != "NONE"; then
  AC_MSG_RESULT([------------------------------------------------------------------------])
  AC_MSG_RESULT([  WARNING: You have set the --prefix to '$prefix'. But we mostly    ])
  AC_MSG_RESULT([  ignore the --prefix. For your info, using the values determined from ])
  AC_MSG_RESULT([  $PG_CONFIG we will be installing:   ])
  AC_MSG_RESULT([    * postgis shared library in $PGSQL_LIBDIR ])
  AC_MSG_RESULT([    * postgis SQL files in $PGSQL_SHAREDIR/contrib/postgis-$POSTGIS_MAJOR_VERSION.$POSTGIS_MINOR_VERSION ])
  AC_MSG_RESULT([    * postgis executables in $PGSQL_BINDIR ])
  AC_MSG_RESULT([------------------------------------------------------------------------])
fi

dnl ===========================================================================
dnl Detect libxml2 if it is installed
dnl (needed to GeomFromGML and GeomFromKML functions)
dnl ===========================================================================

AC_ARG_WITH([xml2config],
	[AS_HELP_STRING([--with-xml2config=FILE], [specify an alternative xml2-config file])],
	[XML2CONFIG="$withval"], [XML2CONFIG=""])

XML2_LDFLAGS=""
XML2_CPPFLAGS=""

if test "x$XML2CONFIG" = "x"; then
	dnl XML2CONFIG was not specified, so search within the current path
	AC_PATH_PROG([XML2CONFIG], [xml2-config])

	dnl If we couldn't find xml2-config, display a warning if pkg-config fails too
	if test "x$XML2CONFIG" = "x"; then
		if test ! -z "$PKG_CONFIG"; then
			PKG_CHECK_MODULES([LIBXML2], [libxml-2.0], [
				XML2_CPPFLAGS="$LIBXML2_CFLAGS"
				XML2_LDFLAGS="$LIBXML2_LIBS"
				POSTGIS_LIBXML2_VERSION=`$PKG_CONFIG libxml-2.0 --modversion`
			], [])
		else
			AC_MSG_ERROR([could not find xml2-config from libxml2 within the current path. You may need to try re-running configure with a --with-xml2config parameter.])
		fi
	else
		dnl Extract the linker and include flags
		XML2_LDFLAGS=`$XML2CONFIG --libs`
		XML2_CPPFLAGS=`$XML2CONFIG --cflags`

		dnl Extract the version
		POSTGIS_LIBXML2_VERSION=`$XML2CONFIG --version`
	fi
else
	dnl XML2CONFIG was specified; display a message to the user
	if test "x$XML2CONFIG" = "xyes"; then
		AC_MSG_ERROR([you must specify a parameter to --with-xml2config, e.g. --with-xml2config=/path/to/xml2-config])
	else
		if test -f $XML2CONFIG; then
			AC_MSG_RESULT([Using user-specified xml2-config file: $XML2CONFIG])

			dnl Extract the linker and include flags
			XML2_LDFLAGS=`$XML2CONFIG --libs`
			XML2_CPPFLAGS=`$XML2CONFIG --cflags`

			dnl Extract the version
			POSTGIS_LIBXML2_VERSION=`$XML2CONFIG --version`
		else
			AC_MSG_ERROR([the user-specified xml2-config file $XML2CONFIG does not exist])
		fi
	fi
fi


dnl
dnl XCode in 10.12 supplies bad flags in xml2config resulting
dnl in compile errors. For that one version, we force the prefix
dnl to match where we know the libraries reside
dnl
case $host_os in
    darwin*)
        AC_PATH_PROG([XCRUN], [xcrun])
        if test "x$XCRUN" = "x"; then
        	AC_MSG_RESULT([using OSX XCode... no])
        else
            XCODE_VER=`$XCRUN --show-sdk-version`
            if test $XCODE_VER = "10.12"; then
                AC_MSG_RESULT([applying OSX XCode 10.12 libxml special case... yes])
                XML2_LDFLAGS=`$XML2CONFIG --exec-prefix=/usr --libs`
            else
                AC_MSG_RESULT([using OSX XCode $XCODE_VER... yes])
            fi
        fi
        ;;
esac

dnl Check headers file
CPPFLAGS_SAVE="$CPPFLAGS"
CPPFLAGS="$XML2_CPPFLAGS"
AC_CHECK_HEADERS([libxml/tree.h libxml/parser.h libxml/xpath.h libxml/xpathInternals.h],
		 [], [AC_MSG_ERROR([could not find headers include related to libxml2])])
CPPFLAGS="$CPPFLAGS_SAVE"

dnl Ensure we can link against libxml2
LIBS_SAVE="$LIBS"
LIBS="$XML2_LDFLAGS"
AC_CHECK_LIB([xml2], [xmlInitParser], [], [AC_MSG_ERROR([could not find libxml2])], [])
LIBS="$LIBS_SAVE"

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
if test "x$GEOS_PATCH_VERSION" = "x"; then
	GEOS_PATCH_VERSION="0";
fi
GEOS_FULL_VERSION=`$GEOSCONFIG --version`
POSTGIS_GEOS_VERSION="$GEOS_MAJOR_VERSION$GEOS_MINOR_VERSION"
GEOS_NUMERIC_PATCH_VERSION=`printf "%02d" $GEOS_PATCH_VERSION`
GEOS_NUMERIC_MINOR_VERSION=`printf "%02d" $GEOS_MINOR_VERSION`
GEOS_NUMERIC_VERSION="$GEOS_MAJOR_VERSION$GEOS_NUMERIC_MINOR_VERSION$GEOS_NUMERIC_PATCH_VERSION"

dnl Ensure that we are using GEOS >= 3.6.0
AC_MSG_RESULT([checking GEOS version... $GEOS_FULL_VERSION])
if test ! "$GEOS_NUMERIC_VERSION" -ge 30600; then
	AC_MSG_ERROR([PostGIS requires GEOS >= 3.6.0])
fi

dnl Extract the linker and include flags
GEOS_LDFLAGS=`$GEOSCONFIG --clibs`
GEOS_CPPFLAGS=`$GEOSCONFIG --cflags`
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
AC_SUBST([GEOS_NUMERIC_VERSION])


dnl ===========================================================================
dnl SFCGAL library support
dnl ===========================================================================

AC_ARG_WITH([sfcgal],
	[AS_HELP_STRING([--with-sfcgal=PATH], [Add SFCGAL support. ARG allows to specify an alternate PATH to sfcgal-config])],
	[SFCGAL_CONFIG="$withval"],
	[with_sfcgal=auto])


if test "x$with_sfcgal" != "xno"; then
  if test "x$with_sfcgal" = "xyes" -o "x$with_sfcgal" = "xauto"; then
	  AC_PATH_PROG([SFCGAL_CONFIG], [sfcgal-config], [])
  fi
	if test -x "$SFCGAL_CONFIG"; then
		SFCGAL_VERSION=`$SFCGAL_CONFIG --version` || AC_MSG_ERROR([cannot determine sfcgal version (tried with $SFCGAL_CONFIG --version)])
		SFCGAL_LDFLAGS=`$SFCGAL_CONFIG --libs`
		SFCGAL_CPPFLAGS=`$SFCGAL_CONFIG --cflags`

		SFCGAL_MAJOR_VERSION=`echo $SFCGAL_VERSION | cut -d. -f1 | sed 's/[[^0-9]]//g'`
		SFCGAL_MINOR_VERSION=`echo $SFCGAL_VERSION | cut -d. -f2 | sed 's/[[^0-9]]//g'`
		SFCGAL_PATCH_VERSION=`echo $SFCGAL_VERSION | cut -d. -f3 | sed 's/[[^0-9]]//g'`
		SFCGAL_NUMERIC_PATCH_VERSION=`printf "%02d" $SFCGAL_PATCH_VERSION`
		SFCGAL_NUMERIC_MINOR_VERSION=`printf "%02d" $SFCGAL_MINOR_VERSION`
		POSTGIS_SFCGAL_VERSION="$SFCGAL_MAJOR_VERSION$SFCGAL_NUMERIC_MINOR_VERSION$SFCGAL_NUMERIC_PATCH_VERSION"
		AC_DEFINE_UNQUOTED([POSTGIS_SFCGAL_VERSION], [$POSTGIS_SFCGAL_VERSION], [SFCGAL library version at build time])
		AC_SUBST([POSTGIS_SFCGAL_VERSION])

		SFCGAL_STATIC=`$SFCGAL_CONFIG --static`
		if test "x$SFCGAL_STATIC" = "xON"; then
			AC_MSG_WARN([The SFCGAL version found is not installed as a dynamic library.])
		else
        		SFCGAL="sfcgal"
			HAVE_SFCGAL="yes"
                        AC_DEFINE([HAVE_SFCGAL], [1], [Define to 1 if sfcgal is being built])
		fi
		if test ! "$POSTGIS_SFCGAL_VERSION" -ge 10301; then
			AC_MSG_ERROR([PostGIS requires SFCGAL >= 1.3.1 (found $SFCGAL_VERSION)])
		fi
	else
		if test "x$with_sfcgal" != "xauto"; then
			AC_MSG_ERROR([sfcgal-config cannot be found. Please install sfcgal])
		fi
	fi
fi

AC_SUBST([SFCGAL_VERSION])
AC_SUBST([SFCGAL_CPPFLAGS])
AC_SUBST([SFCGAL_LDFLAGS])
AC_SUBST([SFCGAL_OBJS])
AC_SUBST([SFCGAL])
AC_SUBST([HAVE_SFCGAL])


dnl ===========================================================================
dnl Detect gettext
dnl ===========================================================================

GETTEXT_CFLAGS=""
GETTEXT_LDFLAGS=""

AC_ARG_WITH([gettext],
	[AS_HELP_STRING([--with-gettext=PATH], [specify a path to non-default gettext installation])],
	[GETTEXT_PATH="$withval"], [GETTEXT_PATH="yes"])

LDFLAGS_SAVE="$LDFLAGS"
CFLAGS_SAVE="$CFLAGS"

if test "x$GETTEXT_PATH" != "xno"; then
	dnl If user has specified a custom gettext installation path, use it.
	if test "x$GETTEXT_PATH" != "xyes"; then
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
		AC_MSG_ERROR([you must specify a parameter to --with-projdir, e.g. --with-projdir=/path/to])
	else
		if test -d "$PROJDIR"; then
			AC_MSG_RESULT([Using user-specified proj directory: $PROJDIR])

			dnl Add the include directory to PROJ_CPPFLAGS
			PROJ_CPPFLAGS="-I$PROJDIR/include"
			PROJ_LDFLAGS="-L$PROJDIR/lib -lproj"
		else
			AC_MSG_ERROR([the --with-projdir directory "$PROJDIR" cannot be found])
		fi
	fi
elif test ! -z "$PKG_CONFIG"; then
	dnl To keep compatibility with PROJ pre 4.8, default to lproj if not found
	PKG_CHECK_MODULES([PROJ], [proj],
		[
			PROJ_CPPFLAGS="$PROJ_CFLAGS"
			PROJ_LDFLAGS="$PROJ_LIBS"
		],
		[
			PROJ_LDFLAGS="-lproj"
		])
else
	dnl To keep compatibility with PROJ pre 4.8, default to lproj
	PROJ_LDFLAGS="-lproj"
fi


dnl Check that we can find the proj_api.h header file
CPPFLAGS_SAVE="$CPPFLAGS"
CPPFLAGS="$PROJ_CPPFLAGS"
AC_CHECK_HEADER([proj_api.h],
	[],
	[AC_CHECK_HEADER([proj.h],
		[],
		[AC_MSG_ERROR([could not find proj.h or proj_api.h - you may need to specify the directory of a PROJ installation using --with-projdir])]
		)]
	)

dnl Return the PROJ.4 version number
AC_PROJ_VERSION([POSTGIS_PROJ_VERSION])
AC_DEFINE_UNQUOTED([POSTGIS_PROJ_VERSION], [$POSTGIS_PROJ_VERSION], [PROJ library version])
AC_SUBST([POSTGIS_PROJ_VERSION])
CPPFLAGS="$CPPFLAGS_SAVE"

dnl Ensure that we are using PROJ >= 4.6.0 (requires pj_set_searchpath)
if test ! "$POSTGIS_PROJ_VERSION" -ge 46; then
	AC_MSG_ERROR([PostGIS requires PROJ >= 4.6.0])
fi

AC_SUBST([PROJ_CPPFLAGS])
AC_SUBST([PROJ_LDFLAGS])

dnl Ensure we can link against libproj
LIBS_SAVE="$LIBS"
LIBS="$PROJ_LDFLAGS"
AC_CHECK_LIB([proj], [pj_get_release], [],
	[AC_CHECK_LIB([proj], [proj_info], [],
		[AC_MSG_ERROR([could not find libproj - you may need to specify the directory of a PROJ installation using --with-projdir])],
		[]
		)],
	[])
LIBS="$LIBS_SAVE"


dnl ===========================================================================
dnl Detect if json-c installed
dnl ===========================================================================

CHECK_JSON=yes
HAVE_JSON=no

AC_ARG_WITH([json],
	[AS_HELP_STRING([--without-json], [build without json-c support])],
	[CHECK_JSON="$withval"], [])

if test "$CHECK_JSON" != "no"; then

    AC_ARG_WITH([jsondir],
            [AS_HELP_STRING([--with-jsondir=PATH], [specify the json-c installation directory])],
            [JSONDIR="$withval"], [JSONDIR=])

    if test ! "x$JSONDIR" = "x"; then
            dnl Make sure that the directory exists
            if test "x$JSONDIR" = "xyes"; then
                    AC_MSG_ERROR([you must specify a parameter to --with-jsondir, e.g. --with-jsondir=/path/to])
            else
                    AC_MSG_RESULT([Using user-specified json-c directory: $JSONDIR])

                    JSON_LDFLAGS="-L$JSONDIR/lib"
                    LIBS_SAVE="$LIBS"
                    LIBS="$JSON_LDFLAGS"
                    AC_CHECK_FILE("$JSONDIR/include/json-c/json.h",
                                [
                                    JSON_CPPFLAGS="-I$JSONDIR/include/json-c"
                                    AC_CHECK_LIB([json-c], [json_object_get], [HAVE_JSON=yes; JSON_LDFLAGS="${JSON_LDFLAGS} -ljson-c"])
                                ],
                                [
                                    AC_CHECK_FILE("$JSONDIR/include/json/json.h",
                                    [
                                        JSON_CPPFLAGS="-I$JSONDIR/include/json"
                                        AC_CHECK_LIB([json], [json_object_get], [HAVE_JSON=yes; JSON_LDFLAGS="${JSON_LDFLAGS} -ljson"])
                                    ],
                                    [AC_MSG_ERROR([Could not find header: json.h])])
                                ])
                    LIBS="$LIBS_SAVE"
            fi
    elif test ! -z "$PKG_CONFIG"; then
	PKG_CHECK_MODULES([JSONC], [json-c], [
                HAVE_JSON=yes
                JSON_CPPFLAGS="$JSONC_CFLAGS"
                JSON_LDFLAGS="$JSONC_LIBS"
            ], [AC_MSG_WARN("Could not find json-c")])
    fi

    if test "$HAVE_JSON" = "yes"; then
            AC_DEFINE([HAVE_LIBJSON], 1, [Define to 1 if libjson is present])
    fi

    AC_SUBST([JSON_CPPFLAGS])
    AC_SUBST([JSON_LDFLAGS])
    AC_SUBST([HAVE_JSON])

fi


dnl ===========================================================================
dnl Detect if protobuf-c installed
dnl ===========================================================================

CHECK_PROTOBUF=yes
HAVE_PROTOBUF=no

AC_ARG_WITH([protobuf],
	[AS_HELP_STRING([--without-protobuf], [build without protobuf-c support])],
	[CHECK_PROTOBUF="$withval"], [])

dnl User didn't turn OFF protobuf support so...
if test "$CHECK_PROTOBUF" != "no"; then

	HAVE_PROTOBUF=yes
	dnl Need to find libdir, incdir and protoc-c compiler

	CPPFLAGS_SAVE="$CPPFLAGS"
	LDFLAGS_SAVE="$LDFLAGS"
        LIBS_SAVE="$LIBS"

	dnl Try pkgconfig first
	if test -n "$PKG_CONFIG"; then
		dnl Ensure libprotobuf-c is of minimum required version
		PKG_CHECK_MODULES([PROTOBUFC], [libprotobuf-c >= 1.1.0], [
				PROTOBUF_CPPFLAGS="$PROTOBUFC_CFLAGS";
				PROTOBUF_LDFLAGS="$PROTOBUFC_LIBS";
			], [
				AC_MSG_RESULT([libprotobuf-c not found in pkg-config])
			])
	fi

	AC_ARG_WITH([protobufdir],
		[AS_HELP_STRING([--with-protobufdir=PATH], [specify the protobuf-c installation directory])],[
			if test "x$withval" = "xyes"; then
				AC_MSG_ERROR([you must specify a parameter to --with-protobufdir, e.g. --with-protobufdir=/usr/local])
			else
				PROTOBUF_LDFLAGS="-L$withval/lib -lprotobuf-c";
				PROTOBUF_CPPFLAGS="-I$withval/include"
			fi
		], [])

	AC_ARG_WITH([protobuf-inc],
		[AS_HELP_STRING([--with-protobuf-inc=PATH], [google/protobuf-c/protobuf-c.h header installation directory])], [
			PROTOBUF_CPPFLAGS="-I$withval"
		],[])

	AC_ARG_WITH([protobuf-lib],
		[AS_HELP_STRING([--with-protobuf-lib=PATH], [libprotobuf-c.so/dll/dylib library installation directory])], [
			PROTOBUF_LDFLAGS="-L$withval -lprotobuf-c"
		],[])

	if test -n "$PROTOBUF_CPPFLAGS"; then
		CPPFLAGS="$PROTOBUF_CPPFLAGS"
	fi

	if test -n "$PROTOBUF_LDFLAGS"; then
		LDFLAGS="$PROTOBUF_LDFLAGS"
	fi

	dnl confirm that discovered/configured include path works
	AC_CHECK_HEADER([protobuf-c/protobuf-c.h], [],
		AC_MSG_ERROR([unable to find protobuf-c/protobuf-c.h using CPPFLAGS. You can disable MVT and Geobuf support using --without-protobuf])
		HAVE_PROTOBUF=no
	)

	dnl confirm that discovered/configured library path works
	AC_CHECK_LIB([protobuf-c], [protobuf_c_message_init], [],
		AC_MSG_ERROR([unable to link protobuf-c using $LDFLAGS. You can disable MVT and Geobuf support using --without-protobuf])
		HAVE_PROTOBUF=no
	)

	AC_CHECK_LIB([protobuf-c], [protobuf_c_version],
		AC_DEFINE([HAVE_PROTOBUF_C_VERSION], [1], [Define to 1 if protobuf_c_version() is present]),
		[])

	AC_MSG_CHECKING([protobuf-c version])
	AC_PROTOBUFC_VERSION([PROTOC_VERSION])
        if test ! "$PROTOC_VERSION" -ge 1001000; then
                AC_MSG_ERROR("Old protobuf-c release found but 1.1.0 is required. You can disable MVT and Geobuf support using --without-protobuf")
        fi
	AC_MSG_RESULT([$PROTOC_VERSION])

	CPPFLAGS="$CPPFLAGS_SAVE"
	LDFLAGS="$LDFLAGS_SAVE"
        LIBS="$LIBS_SAVE"

	dnl confirm that protobuf compiler is available
	AC_PATH_PROG(PROTOCC, protoc-c)
	if test -z "$PROTOCC"; then
		AC_MSG_ERROR([Cannot find protoc-c protobuf compiler on the PATH: $PATH. You can disable MVT and Geobuf support using --without-protobuf])
		HAVE_PROTOBUF=no
	fi

	dnl all tests passed! turn on compile-time defines
        if test "$HAVE_PROTOBUF" = "yes"; then
		AC_DEFINE([HAVE_LIBPROTOBUF], [1], [Define to 1 if libprotobuf-c is present])
		AC_DEFINE_UNQUOTED([LIBPROTOBUF_VERSION], [$PROTOC_VERSION], [Numeric version number for libprotobuf-c])
	else
                AC_MSG_ERROR("Missing protobuf-c support. You can disable MVT and Geobuf support using --without-protobuf")
        fi

        AC_SUBST([HAVE_PROTOBUF])
        AC_SUBST([PROTOC_VERSION])
        AC_SUBST([PROTOBUF_CPPFLAGS])
        AC_SUBST([PROTOBUF_LDFLAGS])

fi


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
dnl Allow the developer to turn on expensive checks and debugging flags
dnl with --enable-debug
dnl

AC_ARG_ENABLE([debug], AC_HELP_STRING([--enable-debug], [Enable debugging code and flags]),
	[ENABLE_DEBUG=1], [ENABLE_DEBUG=0])

if test $ENABLE_DEBUG -eq 1; then
    AC_DEFINE([PARANOIA_LEVEL], [10], [Enable use of memory checks])
    CFLAGS="$CFLAGS -g"
else
    AC_DEFINE([PARANOIA_LEVEL], [0], [Disable use of memory checks])
    CPPFLAGS="-DNDEBUG $CPPFLAGS"
fi

dnl ===========================================================================
dnl Allow the developer to disable the automatic updates of postgis_revision.h
dnl with --without-phony-revision
dnl
AC_ARG_WITH([phony-revision], AC_HELP_STRING([--without-phony-revision], [Disable automatic updates of postgis_revision.h]), [], [])
if test "x$with_phony_revision" != "xno"; then
    PHONY_REVISION="yes"
else
    PHONY_REVISION="no"
fi
AC_MSG_RESULT([phony-revision: $with_phony_revision])
AC_SUBST([PHONY_REVISION])

dnl ===========================================================================
dnl Define version macros
dnl

POSTGIS_VERSION="$POSTGIS_MAJOR_VERSION.$POSTGIS_MINOR_VERSION USE_GEOS=1 USE_PROJ=1 USE_STATS=1"
if test "$HAVE_LIBXML2" = "1"; then
	POSTGIS_VERSION="$POSTGIS_VERSION USE_LIBXML2=1"
fi

POSTGIS_LIB_VERSION="$POSTGIS_MAJOR_VERSION.$POSTGIS_MINOR_VERSION.$POSTGIS_MICRO_VERSION"
POSTGIS_BUILD_DATE=`date ${SOURCE_DATE_EPOCH:+-d @$SOURCE_DATE_EPOCH} -u "+%Y-%m-%d %H:%M:%S"`
POSTGIS_SCRIPTS_VERSION="$POSTGIS_LIB_VERSION"

AC_DEFINE_UNQUOTED([POSTGIS_VERSION], ["$POSTGIS_VERSION"], [PostGIS version])
AC_DEFINE_UNQUOTED([POSTGIS_LIB_VERSION], ["$POSTGIS_LIB_VERSION"], [PostGIS library version])
AC_DEFINE_UNQUOTED([POSTGIS_BUILD_DATE], ["$POSTGIS_BUILD_DATE"], [PostGIS build date])
AC_DEFINE_UNQUOTED([POSTGIS_SCRIPTS_VERSION], ["$POSTGIS_SCRIPTS_VERSION"], [PostGIS scripts version])

AC_SUBST([POSTGIS_VERSION])
AC_SUBST([POSTGIS_LIB_VERSION])
AC_SUBST([POSTGIS_BUILD_DATE])
AC_SUBST([POSTGIS_SCRIPTS_VERSION])


dnl ====================================
dnl address standardizer stuff
dnl ====================================
AC_ARG_WITH([address-standardizer],
    [AS_HELP_STRING([--without-address-standardizer],
                    [Disable the address_standardizer extension])],
    [], [])

HAVE_PCRE=no
ADDRESS_STANDARDIZER=""

if test "x$with_address_standardizer" != "xno"; then
    dnl ===========================================================================
    dnl Detect the version of PCRE installed
    dnl ===========================================================================


    AC_ARG_WITH([pcredir],
        [AS_HELP_STRING([--with-pcredir=PATH], [specify the PCRE installation directory])],
        [PCREDIR="$withval"], [PCREDIR=""])

    if test ! "x$PCREDIR" = "x"; then
        dnl Make sure that the directory exists
        if test "x$PCREDIR" = "xyes"; then
            AC_MSG_ERROR([you must specify a parameter to --with-pcredir, e.g. --with-pcredir=/path/to])
        else
            if test -d "$PCREDIR"; then
                AC_MSG_RESULT([Using user-specified PCRE directory: $PCREDIR])

                AC_CHECK_FILE("$PCREDIR/include/pcre.h",
                        [
                            PCRE_CPPFLAGS="-I$PCREDIR/include"
                            PCRE_LDFLAGS="-L$PCREDIR/lib -lpcre"
                            HAVE_PCRE=yes
                            ADDRESS_STANDARDIZER="address_standardizer"
                        ],
                        [AC_MSG_ERROR([Could not find header: pcre.h])])
            else
                AC_MSG_ERROR([the --with-pcredir directory "$PCREDIR" cannot be found])
            fi
        fi
    elif test ! -z "$PKG_CONFIG"; then
	PKG_CHECK_MODULES([PCRE], [libpcre], [
                PCRE_CPPFLAGS="$PCRE_CFLAGS"
                PCRE_LDFLAGS="$PCRE_LIBS"
                ADDRESS_STANDARDIZER="address_standardizer"
                HAVE_PCRE=yes
            ],
            [
                ADDRESS_STANDARDIZER=""
                HAVE_PCRE=no
            ])
    fi

    AC_SUBST([PCRE_CPPFLAGS])
    AC_SUBST([PCRE_LDFLAGS])

else
    ADDRESS_STANDARDIZER=""
    AC_MSG_RESULT([ADDRESS_STANDARDIZER support: disabled])
fi

AC_SUBST([HAVE_PCRE])
AC_SUBST([ADDRESS_STANDARDIZER])

CPPFLAGS="$PGSQL_CPPFLAGS $GEOS_CPPFLAGS $PROJ_CPPFLAGS $PROTOBUF_CPPFLAGS $XML2_CPPFLAGS $SFCGAL_CPPFLAGS $JSON_CPPFLAGS $PCRE_CPPFLAGS $CPPFLAGS"
dnl AC_MSG_RESULT([CPPFLAGS: $CPPFLAGS])

SHLIB_LINK="$PGSQL_LDFLAGS $GEOS_LDFLAGS $PROJ_LDFLAGS $JSON_LDFLAGS $PROTOBUF_LDFLAGS $XML2_LDFLAGS $SFCGAL_LDFLAGS $EXCLUDELIBS_LDFLAGS $LDFLAGS"
AC_SUBST([SHLIB_LINK])
dnl AC_MSG_RESULT([SHLIB_LINK: $SHLIB_LINK])

dnl ====================================
dnl topology stuff
dnl ====================================
AC_ARG_WITH([topology],
    [AS_HELP_STRING([--without-topology],
                    [Disable the topology extension])],
    [], [])

if test "x$with_topology" != "xno"; then

    TOPOLOGY="topology"
    AC_MSG_RESULT([TOPOLOGY: Topology support requested])
else
    AC_MSG_RESULT([TOPOLOGY: Topology support disabled])
fi

AC_SUBST([TOPOLOGY])

dnl ====================================
dnl interrupt tests
dnl ====================================
INTERRUPTTESTS="no"
AC_ARG_WITH([interrupt-tests],
    [AS_HELP_STRING([--with-interrupt-tests],
                    [Enable the interrupt tests])],
    [INTERRUPTTESTS="yes"], [])

if test "x$with_interrupt_tests" = "xno"; then
    INTERRUPTTESTS="no"
fi

AC_SUBST([INTERRUPTTESTS])



dnl ===========================================================================
dnl SRID stuff
dnl ===========================================================================

SRID_MAX=999999
SRID_USR_MAX=998999
AC_SUBST([SRID_MAX])
AC_SUBST([SRID_USR_MAX])


dnl ====================================
dnl raster stuff
dnl ====================================
AC_ARG_WITH(
	[raster],
	AC_HELP_STRING([--without-raster], [Disable the raster extension]),
	[], [])

if test "x$with_raster" != "xno"; then
	RASTER="raster"
	AC_MSG_RESULT([RASTER: Raster support requested])

	AC_CONFIG_HEADERS([raster/raster_config.h])

	dnl ===========================================================================
	dnl Allow output of double truncation warnings with --with-raster-dblwarning
	dnl ===========================================================================
	AC_ARG_WITH(
		[raster-dblwarning],
		AC_HELP_STRING([--with-raster-dblwarning], [output double truncation warnings.  Only used with --with-raster]),
		[POSTGIS_RASTER_WARN_ON_TRUNCATION=1],
		[POSTGIS_RASTER_WARN_ON_TRUNCATION=0])

	AC_DEFINE_UNQUOTED(
		[POSTGIS_RASTER_WARN_ON_TRUNCATION],
		[$POSTGIS_RASTER_WARN_ON_TRUNCATION],
		[Define to 1 if a warning is outputted every time a double is truncated])

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
	LIBGDAL_DEPLIBS_LDFLAGS=""

	dnl GDAL version constants, update here
	GDAL_MIN_VERSION=1.8.0
	GDAL_MIN_VERSION_NUMBER=180 dnl TODO: Use GDAL version calculation MAJOR*1000+MINOR*100+REV*10+BUILD

	AC_ARG_WITH(
		[gdalconfig],
		AC_HELP_STRING([--with-gdalconfig=@<:@ARG@:>@],[specify location of gdal-config (ARG=path).  Only used with --with-raster]),
		[GDAL_CONFIG="$withval"],
		[AC_PATH_PROG([GDAL_CONFIG], [gdal-config], [])]
  )

	AC_MSG_CHECKING([GDAL version])

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

		AC_MSG_RESULT([$GDAL_FULL_VERSION])

		dnl Ensure we are using minimum required version of GDAL
		if test ! "$GDAL_VERSION_NUMBER" -ge "$GDAL_MIN_VERSION_NUMBER" ; then
			AC_MSG_ERROR([PostGIS raster requires GDAL >= $GDAL_MIN_VERSION. Use --without-raster to build without raster support.])
		fi

		AC_DEFINE_UNQUOTED([POSTGIS_GDAL_VERSION], [$POSTGIS_GDAL_VERSION], [GDAL library version])
		AC_SUBST([POSTGIS_GDAL_VERSION])

		dnl Check that OGR is enabled
		AC_MSG_CHECKING([for OGR enabled])
		OGR_ENABLED=`$GDAL_CONFIG --ogr-enabled`
		if test "x$OGR_ENABLED" != "xyes"; then
			AC_MSG_RESULT([$OGR_ENABLED])
			AC_MSG_ERROR([PostGIS raster requires OGR to be enabled in GDAL. Use --without-raster to build without raster support.])
		fi
		AC_MSG_RESULT([$OGR_ENABLED])

		dnl Extract the linker and include flags
		LIBGDAL_LDFLAGS=`$GDAL_CONFIG --libs`
		dnl LIBGDAL_DEPLIBS_LDFLAGS=`$GDAL_CONFIG --dep-libs`
		LIBGDAL_CFLAGS=`$GDAL_CONFIG --cflags`

		CPPFLAGS_SAVE="$CPPFLAGS"
		CPPFLAGS="$LIBGDAL_CFLAGS"
		CFLAGS_SAVE="$CFLAGS"
		CFLAGS=`"$PG_CONFIG" --cflags`
		CC_SAVE="$CC"
		CC=`"$PG_CONFIG" --cc`

		LIBS_SAVE="$LIBS"
		LIBS="$LIBGDAL_LDFLAGS"

		dnl Check headers file
		AC_CHECK_HEADERS(
			[gdal.h ogr_api.h cpl_conv.h],
			[],
			[AC_MSG_ERROR([could not find GDAL headers])])

		dnl Ensure we can link against gdal
		AC_SEARCH_LIBS([GDALAllRegister], [gdal], [], [AC_MSG_ERROR([could not find GDAL])], [])

		LIBS="$LIBGDAL_LDFLAGS"

		dnl Ensure we can link against ogr
		AC_SEARCH_LIBS([OGRRegisterAll], [gdal], [], [AC_MSG_ERROR([could not find OGR])], [])

		LIBS="$LIBGDAL_LDFLAGS"
		CPPFLAGS="$CPPFLAGS_SAVE"
		CFLAGS="$CFLAGS_SAVE"
		CC="$CC_SAVE"
		LIBS="$LIBS_SAVE"

		AC_SUBST([LIBGDAL_CFLAGS])
		AC_SUBST([LIBGDAL_LDFLAGS])
		AC_SUBST([LIBGDAL_DEPLIBS_LDFLAGS])

	else
		AC_MSG_RESULT([not found])
		AC_MSG_ERROR([gdal-config not found. Use --without-raster or try --with-gdalconfig=<path to gdal-config>])
	fi

	dnl Define raster objects, for makefiles
	RT_CORE_LIB=corelib
	RT_PG_LIB=pglib
	RT_LOADER=rtloader
	RT_POSTGIS_SQL=rtpostgis.sql

	AC_SUBST([RASTER])
	AC_SUBST([RT_CORE_LIB])
	AC_SUBST([RT_PG_LIB])
	AC_SUBST([RT_LOADER])
	AC_SUBST([RT_POSTGIS_SQL])

	RT_MAKEFILE_LIST="
		raster/Makefile \
		raster/rt_core/Makefile \
		raster/rt_pg/Makefile \
		raster/loader/Makefile \
		raster/test/Makefile \
		raster/test/cunit/Makefile \
		raster/test/regress/Makefile \
		raster/scripts/Makefile \
		raster/scripts/python/Makefile"

else
	AC_MSG_RESULT([RASTER: Raster support disabled])

	dnl We still generate raster/rt_pg makefile to
	dnl use for upgrading from pre-split-raster versions
	RT_MAKEFILE_LIST="raster/Makefile raster/rt_pg/Makefile"
fi


dnl ===========================================================================
dnl Deps folder
dnl ===========================================================================

DEPS_MAKEFILE_LIST="
    deps/Makefile"


dnl ===========================================================================
dnl Wagyu
dnl ===========================================================================

dnl Wagyu will only be necessary if protobuf is present to build MVTs

HAVE_WAGYU=no
if test "x$HAVE_PROTOBUF" = "xyes"; then
    AC_ARG_WITH([wagyu], AC_HELP_STRING([--without-wagyu], [Don't use the wagyu library]), [], [])

    if test "x$with_wagyu" != xno; then
            AC_MSG_RESULT([WAGYU: Wagyu usage requested])

            DEPS_SUBDIR="deps"
            AC_SUBST([DEPS_SUBDIR])

            WAGYU_LIB=libwagyu.la
            AC_SUBST([WAGYU_LIB])

            dnl ============================================================
            dnl We force  to use the same compiler as Postgresql
            dnl ============================================================
            CXX_SAVE="$CXX"
            CC_SAVE="$CC"
            CFLAGS_SAVE="$CFLAGS"
            CXXFLAGS_SAVE="$CXXFLAGS"
            CPPFLAGS_SAVE="$CPPFLAGS"
            LDFLAGS_SAVE="$LDFLAGS"
            LIBS_SAVE="$LIBS"

            WAGYU_CXX=`"$PG_CONFIG" --cc`
            CPPFLAGS="-x c++"
            CFLAGS=""
            LDFLAGS=""
            LIBS=""
            CXX="$WAGYU_CXX"
            AC_PROG_CXX
            AX_CXX_COMPILE_STDCXX(11, noext, mandatory)
            WAGYU_CXX="$CXX -x c++"

            dnl ============================================================
            dnl Check if we can declare the c++ stdlib
            dnl ============================================================
            CC="$WAGYU_CXX"

            AC_CHECK_LIB(c++, main, [HAVE_CPP=yes], [HAVE_CPP=no])
            AC_CHECK_LIB(stdc++, main, [HAVE_STDCPP=yes], [HAVE_STDCPP=no])

            if test "x$HAVE_CPP" = "xyes"; then
                WAGYU_LDFLAGS="-lc++"
            elif test "x$HAVE_STDCPP" = "xyes"; then
                WAGYU_LDFLAGS="-lstdc++"
            else
                AC_MSG_WARN("Could not find a C++ standard library")
                WAGYU_LDFLAGS=""
            fi

            CXX="$CXX_SAVE"
            CC="$CC_SAVE"
            CFLAGS="$CFLAGS_SAVE"
            CXXFLAGS="$CXXFLAGS_SAVE"
            CPPFLAGS="$CPPFLAGS_SAVE"
            LDFLAGS="$LDFLAGS_SAVE"
            LIBS="$LIBS_SAVE"

            HAVE_WAGYU=yes
            AC_DEFINE([HAVE_WAGYU], [1], [Define to 1 if wagyu is being built])
            AC_SUBST([HAVE_WAGYU])
            AC_SUBST([WAGYU_CXX])
            AC_SUBST([WAGYU_LDFLAGS])

            DEPS_MAKEFILE_LIST="$DEPS_MAKEFILE_LIST
                    deps/wagyu/Makefile"
    fi
fi

dnl ===========================================================================
dnl Ryu
dnl ===========================================================================
RYU_LIB=libryu.la
AC_SUBST([RYU_LIB])

DEPS_MAKEFILE_LIST="$DEPS_MAKEFILE_LIST
        deps/ryu/Makefile"


dnl ===========================================================================
dnl See if we have the requirements for building the extensions, namely
dnl the xlstproc tool for generating the comments SQL file.
dnl ===========================================================================

EXTENSIONS=""
if test \
        "x$XSLTPROC" != "x" -o \
        -e doc/postgis_comments.sql -a \
        -e doc/raster_comments.sql;
then
    AC_MSG_RESULT([enabling PostgreSQL extension support...])
    EXTENSIONS=extensions
    AC_SUBST([EXTENSIONS])
fi


dnl ===========================================================================
dnl Output the relevant files
dnl ===========================================================================

AC_OUTPUT([GNUmakefile
   extensions/Makefile
   extensions/postgis/Makefile
   extensions/postgis_raster/Makefile
   extensions/postgis_sfcgal/Makefile
   extensions/postgis_topology/Makefile
   extensions/postgis_tiger_geocoder/Makefile
   extensions/address_standardizer/Makefile
   liblwgeom/Makefile
   liblwgeom/cunit/Makefile
   liblwgeom/liblwgeom.h
   libpgcommon/Makefile
   libpgcommon/cunit/Makefile
   postgis/Makefile
   postgis/sqldefines.h
	 sfcgal/Makefile
   sfcgal/regress/Makefile
   loader/Makefile
   loader/cunit/Makefile
   topology/Makefile
   topology/test/Makefile
   regress/Makefile
   regress/core/Makefile
   regress/core/tests.mk
   regress/dumper/Makefile
   regress/loader/Makefile
   doc/Makefile
   doc/Makefile.comments
   doc/html/image_src/Makefile
   utils/Makefile
   $RT_MAKEFILE_LIST
   $DEPS_MAKEFILE_LIST])

dnl ===========================================================================
dnl Display the configuration status information
dnl ===========================================================================

AC_MSG_RESULT()
AC_MSG_RESULT([  PostGIS is now configured for ${host}])
AC_MSG_RESULT()
AC_MSG_RESULT([ -------------- Compiler Info ------------- ])
AC_MSG_RESULT([  C compiler:           ${CC} ${CFLAGS}])
if test "x$HAVE_WAGYU" = "xyes"; then
    AC_MSG_RESULT([  C++ compiler (Wagyu): ${WAGYU_CXX} ${CXXFLAGS}])
fi
AC_MSG_RESULT([  CPPFLAGS:             $CPPFLAGS])
AC_MSG_RESULT([  LDFLAGS:              $LDFLAGS])
AC_MSG_RESULT([  SQL preprocessor:     ${SQLPP}])
AC_MSG_RESULT()
AC_MSG_RESULT([ -------------- Additional Info ------------- ])
if test "x$LIBINCLUDEMINORVERSION" = "xyes"; then
  AC_MSG_RESULT([  POSTGIS lib file includes minor:   ENABLED])
fi

if test "x$INTERRUPTTESTS" = "xyes"; then
  AC_MSG_RESULT([  Interrupt Tests:   ENABLED])
else
  AC_MSG_RESULT([  Interrupt Tests:   DISABLED use: --with-interrupt-tests to enable])
fi

AC_MSG_RESULT()
AC_MSG_RESULT([ -------------- Dependencies -------------- ])
AC_MSG_RESULT([  GEOS config:          ${GEOSCONFIG}])
AC_MSG_RESULT([  GEOS version:         ${GEOS_FULL_VERSION}])
if test "x$RASTER" = "xraster"; then
    AC_MSG_RESULT([  GDAL config:          ${GDAL_CONFIG}])
    AC_MSG_RESULT([  GDAL version:         ${GDAL_FULL_VERSION}])
fi
if test "x$SFCGAL" = "xsfcgal"; then
    AC_MSG_RESULT([  SFCGAL config:        ${SFCGAL_CONFIG}])
    AC_MSG_RESULT([  SFCGAL version:       ${SFCGAL_VERSION}])
fi
if test "x$LIBLWGEOM_ONLY" = "xno"; then
  AC_MSG_RESULT([  PostgreSQL config:    ${PG_CONFIG}])
  AC_MSG_RESULT([  PostgreSQL version:   ${PGSQL_FULL_VERSION}])
else
  AC_MSG_RESULT([  PostgreSQL support:   DISABLED])
fi
AC_MSG_RESULT([  PROJ4 version:        ${POSTGIS_PROJ_VERSION}])
AC_MSG_RESULT([  Libxml2 config:       ${XML2CONFIG}])
AC_MSG_RESULT([  Libxml2 version:      ${POSTGIS_LIBXML2_VERSION}])
AC_MSG_RESULT([  JSON-C support:       ${HAVE_JSON}])

AC_MSG_RESULT([  protobuf support:     ${HAVE_PROTOBUF}])
if test "x$HAVE_PROTOBUF" = "xyes"; then
  AC_MSG_RESULT([  protobuf-c version:   ${PROTOC_VERSION}])
fi

AC_MSG_RESULT([  PCRE support:         ${HAVE_PCRE}])
AC_MSG_RESULT([  Perl:                 ${PERL}])
AC_MSG_RESULT([  Wagyu:                ${HAVE_WAGYU}])

AC_MSG_RESULT()
AC_MSG_RESULT([ --------------- Extensions --------------- ])
if test "x$RASTER" = "xraster"; then
    AC_MSG_RESULT([  PostGIS Raster:                     enabled])
else
    AC_MSG_RESULT([  PostGIS Raster:                     disabled])

fi
if test "x$TOPOLOGY" = "xtopology"; then
    AC_MSG_RESULT([  PostGIS Topology:                   enabled])
else
    AC_MSG_RESULT([  PostGIS Topology:                   disabled])
fi
if test "x$SFCGAL" = "xsfcgal"; then
    AC_MSG_RESULT([  SFCGAL support:                     enabled])
else
    AC_MSG_RESULT([  SFCGAL support:                     disabled])
fi

if test "x$ADDRESS_STANDARDIZER" = "xaddress_standardizer"; then
    AC_MSG_RESULT([  Address Standardizer support:       enabled])
else
    AC_MSG_RESULT([  Address Standardizer support:       disabled])
fi

AC_MSG_RESULT()
AC_MSG_RESULT([ -------- Documentation Generation -------- ])
AC_MSG_RESULT([  xsltproc:             ${XSLTPROC}])
AC_MSG_RESULT([  xsl style sheets:     ${XSLBASE}])
AC_MSG_RESULT([  dblatex:              ${DBLATEX}])
AC_MSG_RESULT([  convert:              ${IMAGEMAGICK}])
AC_MSG_RESULT([  mathml2.dtd:          ${MATHML2_DTD}])
AC_MSG_RESULT()

if test "$GEOS_NUMERIC_VERSION" -lt 30900; then
AC_MSG_WARN([ --------- GEOS VERSION WARNING ------------ ])
AC_MSG_WARN([  You are building against GEOS ${GEOS_FULL_VERSION}.])
AC_MSG_WARN([])
AC_MSG_WARN([  To take advantage of _all_ the features of ])
AC_MSG_WARN([  PostGIS, GEOS 3.9.0 or higher is required.])
AC_MSG_WARN([])
AC_MSG_WARN([  For _many_ features, GEOS 3.6.0 is enough.])
AC_MSG_WARN([])
AC_MSG_WARN([  We recommend GEOS 3.8.0 or higher])
AC_MSG_WARN([])
AC_MSG_WARN([  You can download the latest versions from ])
AC_MSG_WARN([  http://geos.osgeo.org/ ])
AC_MSG_WARN()
fi

if test ! -z "$PKG_CONFIG"; then
    if test ! "x$PROJDIR" = "x"; then
        AC_MSG_WARN()
        AC_MSG_WARN([ | You are building using --with-projdir. This option isn't standard and    |])
        AC_MSG_WARN([ | might be incompatible with future releases of PROJ.                      |])
        AC_MSG_WARN([ | You can instead adjust the PKG_CONFIG_PATH environment variable if you   |])
        AC_MSG_WARN([ | installed software in a non-standard prefix.                             |])
        AC_MSG_WARN([ | Alternatively, you may set the environment variables PROJ_CFLAGS and     |])
        AC_MSG_WARN([ | PROJ_LIBS to avoid the need to call pkg-config.                          |])
    fi

    if test ! "x$JSONDIR" = "x"; then
        AC_MSG_WARN()
        AC_MSG_WARN([ | You are building using --with-jsondir. This option isn't standard and    |])
        AC_MSG_WARN([ | might be incompatible with future releases of json-c.                    |])
        AC_MSG_WARN([ | You can instead adjust the PKG_CONFIG_PATH environment variable if you   |])
        AC_MSG_WARN([ | installed software in a non-standard prefix.                             |])
        AC_MSG_WARN([ | Alternatively, you may set the environment variables JSONC_CFLAGS and    |])
        AC_MSG_WARN([ | JSONC_LIBS to avoid the need to call pkg-config.                         |])
    fi

    if test ! "x$PCREDIR" = "x"; then
        AC_MSG_WARN()
        AC_MSG_WARN([ | You are building using --with-pcredir. This option isn't standard and    |])
        AC_MSG_WARN([ | might be incompatible with future releases of libpcre.                   |])
        AC_MSG_WARN([ | You can instead adjust the PKG_CONFIG_PATH environment variable if you   |])
        AC_MSG_WARN([ | installed software in a non-standard prefix.                             |])
        AC_MSG_WARN([ | Alternatively, you may set the environment variables PCRE_CFLAGS and     |])
        AC_MSG_WARN([ | PCRE_LIBS to avoid the need to call pkg-config.                          |])
    fi
fi
