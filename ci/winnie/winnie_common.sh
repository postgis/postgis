# Common code for all winnie scripts
#
# TODO: add more shared code, I guess
#

set -e

export PROJECTS=/projects
# Don't convert paths
# See https://trac.osgeo.org/postgis/ticket/5436#comment:5
export MSYS2_ARG_CONV_EXCL=/config/tags
export XML_CATALOG_FILES="/projects/docbook/docbook-5.0.1/catalog.xml"

if  [[ "${OVERRIDE}" == '' ]] ; then
	export GEOS_VER=3.13.1
	export GDAL_VER=3.9.2
	export PROJ_VER=8.2.1
	export SFCGAL_VER=2.1.0
	export CGAL_VER=6.0.1
	export ICON_VER=1.17
	export ZLIB_VER=1.2.13
	export PROTOBUF_VER=3.2.0
	export PROTOBUFC_VER=1.2.1
	export JSON_VER=0.12
	export PROJSO=libproj_8_2.dll
	export LZ4_VER=1.9.3
fi;

if [[ ${PCRE_VER} == '' ]]; then
	PCRE_VER=10.40
fi;

export PROTOBUF_VER=3.2.0
export PROTOBUFC_VER=1.2.1
export JSON_VER=0.12




if [ -d "${PROJECTS}/pcre2/rel-pcre2-${PCRE_VER}w${OS_BUILD}${GCC_TYPE}" ]; then
export PCRE_PATH=${PROJECTS}/pcre2/rel-pcre2-${PCRE_VER}w${OS_BUILD}${GCC_TYPE}
else
export PCRE_PATH=${PROJECTS}/pcre/rel-${PCRE_VER}w${OS_BUILD}${GCC_TYPE}
fi


echo "PCRE_PATH is $PCRE_PATH"
#export OS_BUILD=64
#export PGPORT=8442

#export OS_BUILD=32

#export GCC_TYPE=
#if no override is set - use these values
#otherwise use the ones jenkins passes thru
if  [[ "${ICON_VER}" == '' ]] ; then
  export ICON_VER=1.17
fi;

echo "ICON_VER ${ICON_VER}"

if  [[ "${LZ4_VER}" == '' ]] ; then
  export LZ4_VER=1.9.3
fi;

echo "LZ4_VER ${LZ4_VER}"


#set to something even if override is on but not set
if  [[ "${ZLIB_VER}" == '' ]] ; then
  export ZLIB_VER=1.2.13
fi;

#set to something even if override is on but not set
if  [[ "${LIBXML_VER}" == '' ]] ; then
  export LIBXML_VER=2.12.5
fi;

#set to something even if override is on but not set
if  [[ "${CGAL_VER}" == '' ]] ; then
  export CGAL_VER=6.0.1
fi;

##hard code versions of cgal etc. for now

BOOST_VER=1.84.0
export BOOST_VER_WU=1_84_0


export LZ4_PATH=${PROJECTS}/lz4/rel-lz4-${LZ4_VER}w${OS_BUILD}${GCC_TYPE}
echo ${PATH}

PATH="/mingw64/bin:/usr/local/bin:/usr/bin:/bin:/c/Windows/System32:/usr/bin/site_perl:/usr/bin/vendor_perl:/usr/bin/core_perl"
export PATH="${PROJECTS}/CGAL/rel-cgal-${CGAL_VER}w${OS_BUILD}${GCC_TYPE}/bin:${PROJECTS}/CGAL/rel-sfcgal-${SFCGAL_VER}w${OS_BUILD}${GCC_TYPE}/bin:${PROJECTS}/boost/rel-${BOOST_VER_WU}w${OS_BUILD}${GCC_TYPE}/lib:${PATH}"
#export GDAL_VER=2.4.0
#export PROJ_VER=5.2.0

echo "ZLIB_VER $ZLIB_VER"
echo "PROJ_VER $PROJ_VER"
echo "LIBXML_VER $LIBXML_VER"
echo "CGAL_VER $CGAL_VER"
export PROJECTS=/projects
export MINGPROJECTS=/projects


if [ "$OS_BUILD" == "64" ] ; then
	export MINGHOST=x86_64-w64-mingw32
else
	export MINGHOST=i686-w64-mingw32
fi;

#needed for proj.db to be found during cunit - for some reason on winnie it doesn't set
if [ -d "${PROJECTS}/proj/rel-${PROJ_VER}w${OS_BUILD}${GCC_TYPE}/share/proj" ]; then
export PROJ_LIB=${PROJECTS}/proj/rel-${PROJ_VER}w${OS_BUILD}${GCC_TYPE}/share/proj
export PROJ_PATH=${PROJECTS}/proj/rel-${PROJ_VER}w${OS_BUILD}${GCC_TYPE}
else
export PROJ_LIB=${PROJECTS}/proj/rel-proj-${PROJ_VER}w${OS_BUILD}${GCC_TYPE}/share/proj
export PROJ_PATH=${PROJECTS}/proj/rel-proj-${PROJ_VER}w${OS_BUILD}${GCC_TYPE}
fi

#export PG_VER=9.2beta2
export PGWINVER=${PG_VER}edb
export WORKSPACE=`pwd`

echo PATH BEFORE: $PATH

#export PGHOST=localhost
#export PGPORT=8442
export PGUSER=postgres
#export GEOS_VER=3.4.0dev
#export GDAL_VER=1.9.1
export PGPATH=${PROJECTS}/postgresql/rel/pg${PG_VER}w${OS_BUILD}${GCC_TYPE}
export PGPATHEDB=${PROJECTS}/postgresql/rel/pg${PG_VER}w${OS_BUILD}${GCC_TYPE}edb

export GDAL_DATA="${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}${GCC_TYPE}/share/gdal"

export PATH="${PGPATH}/bin:${PGPATH}/lib:${PATH}"
export PATH="${PROJECTS}/xsltproc:${PROJECTS}/gtkw${OS_BUILD}${GCC_TYPE}/bin:${PROJECTS}/geos/rel-${GEOS_VER}w${OS_BUILD}${GCC_TYPE}/bin:${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}${GCC_TYPE}/bin:${PROJECTS}/rel-libiconv-${ICON_VER}w${OS_BUILD}${GCC_TYPE}/lib:${PROJECTS}/rel-libiconv-${ICON_VER}w${OS_BUILD}${GCC_TYPE}/include:${PROJECTS}/rel-libiconv-${ICON_VER}w${OS_BUILD}${GCC_TYPE}/bin:${PROJ_PATH}/bin:${PROJECTS}/libxml/rel-libxml2-${LIBXML_VER}w${OS_BUILD}${GCC_TYPE}/bin:${PROJECTS}/zlib/rel-zlib-${ZLIB_VER}w${OS_BUILD}${GCC_TYPE}/bin:${PROJECTS}/zlib/rel-zlib-${ZLIB_VER}w${OS_BUILD}${GCC_TYPE}/lib:${PATH}"
export PKG_CONFIG_PATH="${PROJECTS}/sqlite/rel-sqlite3w${OS_BUILD}${GCC_TYPE}/lib/pkgconfig:${PROJECTS}/protobuf/rel-${PROTOBUF_VER}w${OS_BUILD}${GCC_TYPE}/lib/pkgconfig:${PROJECTS}/json-c/rel-${JSON_VER}w${OS_BUILD}${GCC_TYPE}/lib/pkgconfig:${PROJ_PATH}/lib/pkgconfig:${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}${GCC_TYPE}/lib/pkgconfig:${PCRE_PATH}/lib/pkgconfig:${PROJECTS}/zlib/rel-zlib-${ZLIB_VER}w${OS_BUILD}${GCC_TYPE}/lib/pkgconfig:${PROJECTS}/gtkw${OS_BUILD}${GCC_TYPE}/lib/pkgconfig:/mingw/${MINGHOST}/lib/pkgconfig"
export SHLIB_LINK="-static-libstdc++ -lstdc++ -Wl,-Bdynamic -lm"
CPPFLAGS="-I${PGPATH}/include -I${PROJECTS}/rel-libiconv-${ICON_VER}w${OS_BUILD}${GCC_TYPE}/include"



#add protobuf
export PATH="${PROJECTS}/protobuf/rel-${PROTOBUF_VER}w${OS_BUILD}${GCC_TYPE}/bin:${PROJECTS}/protobuf/rel-${PROTOBUF_VER}w${OS_BUILD}${GCC_TYPE}/lib:${PATH}"

export PATH="${PATH}:${PGPATH}/bin:${PGPATH}/lib"

#add lz4
export PATH="${LZ4_PATH}/bin:${LZ4_PATH}/lib:${PATH}"

export SHLIB_LINK="-static-libstdc++ -lstdc++ -Wl,-Bdynamic -lm"

CPPFLAGS="-I${PGPATH}/include -I${PROJECTS}/rel-libiconv-${ICON_VER}w${OS_BUILD}${GCC_TYPE}/include"

#add protobuf
export PATH="${PROJECTS}/protobuf/rel-${PROTOBUF_VER}w${OS_BUILD}${GCC_TYPE}/bin:${PROJECTS}/protobuf/rel-${PROTOBUF_VER}w${OS_BUILD}${GCC_TYPE}/lib:${PATH}"

# add pcre for address_standardizer
if [ -n "$PCRE_PATH" ]; then
    export PATH="${PCRE_PATH}/include:${PCRE_PATH}/lib:${PATH}"
fi

echo "PATH AFTER: $PATH"

if  [[ "${INCLUDE_MINOR_LIB}" == '' ]] ; then
	export INCLUDE_MINOR_LIB=1
fi;
