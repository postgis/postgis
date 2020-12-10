#!/usr/bin/env bash
set -e
if  [[ "${OVERRIDE}" == '' ]] ; then
	export GEOS_VER=3.8.1
	export GDAL_VER=3.0.4
	export PROJ_VER=6.3.2
	export SFCGAL_VER=1.3.8
	export CGAL_VER=5.0.2
	export ICON_VER=1.15
	export ZLIB_VER=1.2.11
  export PROTOBUF_VER=3.2.0
	export PROTOBUFC_VER=1.2.1
	export JSON_VER=0.12
	export PROJSO=libproj-13.dll
fi;

export PROTOBUF_VER=3.2.0
export PROTOBUFC_VER=1.2.1
export JSON_VER=0.12
export PCRE_VER=8.33

if  [[ "${ICON_VER}" == '' ]] ; then
  export ICON_VER=1.16
fi;

echo "ICON_VER ${ICON_VER}"

#set to something even if override is on but not set
if  [[ "${ZLIB_VER}" == '' ]] ; then
  export ZLIB_VER=1.2.11
fi;


#set to something even if override is on but not set
if  [[ "${LIBXML_VER}" == '' ]] ; then
  export LIBXML_VER=2.9.9
fi;

#set to something even if override is on but not set
if  [[ "${CGAL_VER}" == '' ]] ; then
  export CGAL_VER=5.0
fi;

echo "ZLIB_VER $ZLIB_VER"
echo "PROJ_VER $PROJ_VER"
echo "LIBXML_VER $LIBXML_VER"
echo "CGAL_VER $CGAL_VER"
echo "ZLIB_VER $ZLIB_VER"
echo "PROJ_VER $PROJ_VER"
echo "LIBXML_VER $LIBXML_VER"
export PROJECTS=/projects
export MINGPROJECTS=/projects
export PATHOLD=$PATH


if [ "$OS_BUILD" == "64" ] ; then
	export MINGHOST=x86_64-w64-mingw32
else
	export MINGHOST=i686-w64-mingw32
fi;

echo PATH AFTER: $PATH
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

export POSTGIS_VER=${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}
export POSTGIS_MICRO_VER=${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}.${POSTGIS_MICRO_VERSION}

if [ -n "$SOURCE_FOLDER" ]; then
  export POSTGIS_SRC=${PROJECTS}/postgis/$SOURCE_FOLDER
else
  if [[ "$POSTGIS_MICRO_VERSION"  == *SVN* || "$POSTGIS_MICRO_VERSION"  == *dev* ]] ; then
    export POSTGIS_SRC=${PROJECTS}/postgis/branches/${POSTGIS_VER}
  else
    #tagged version -- official release
    export POSTGIS_SRC=${PROJECTS}/postgis/tags/${POSTGIS_VER}.${POSTGIS_MICRO_VERSION}
  fi;
fi;

#export POSTGIS_SRC=${PROJECTS}/postgis/trunk
export GDAL_DATA="${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}${GCC_TYPE}/share/gdal"

export RELVERDIR=postgis-pg${REL_PGVER}-binaries-${POSTGIS_MICRO_VER}w${OS_BUILD}

export PATH="${PATHOLD}:${PGPATH}/bin:${PGPATH}/lib"
export PATH="${PROJECTS}/xsltproc:${PROJECTS}/gtkw${OS_BUILD}${GCC_TYPE}/bin:${PROJECTS}/geos/rel-${GEOS_VER}w${OS_BUILD}${GCC_TYPE}/bin:${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}${GCC_TYPE}/bin:${PROJECTS}/rel-libiconv-${ICON_VER}w${OS_BUILD}${GCC_TYPE}/lib:${PROJECTS}/rel-libiconv-${ICON_VER}w${OS_BUILD}${GCC_TYPE}/include:${PROJECTS}/rel-libiconv-${ICON_VER}w${OS_BUILD}${GCC_TYPE}/bin:${MINGPROJECTS}/proj/rel-${PROJ_VER}w${OS_BUILD}${GCC_TYPE}/bin:${PROJECTS}/libxml/rel-libxml2-${LIBXML_VER}w${OS_BUILD}${GCC_TYPE}/bin:${PROJECTS}/zlib/rel-zlib-${ZLIB_VER}w${OS_BUILD}${GCC_TYPE}/bin:${PROJECTS}/zlib/rel-zlib-${ZLIB_VER}w${OS_BUILD}${GCC_TYPE}/lib:${PATH}"
export PKG_CONFIG_PATH="${PROJECTS}/sqlite/rel-sqlite3w${OS_BUILD}${GCC_TYPE}/lib/pkgconfig:${PROJECTS}/zlib/rel-zlib-${ZLIB_VER}w${OS_BUILD}${GCC_TYPE}/lib:${PROJECTS}/protobuf/rel-${PROTOBUF_VER}w${OS_BUILD}${GCC_TYPE}/lib/pkgconfig:${PROJECTS}/json-c/rel-${JSON_VER}w${OS_BUILD}${GCC_TYPE}/lib/pkgconfig:${PROJECTS}/proj/rel-${PROJ_VER}w${OS_BUILD}${GCC_TYPE}/lib/pkgconfig:${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}${GCC_TYPE}/lib/pkgconfig:${PROJECTS}/pcre/rel-${PCRE_VER}w${OS_BUILD}${GCC_TYPE}/lib/pkgconfig:${PROJECTS}/zlib/rel-zlib-${ZLIB_VER}w${OS_BUILD}${GCC_TYPE}/lib/pkgconfig:${PROJECTS}/gtkw${OS_BUILD}${GCC_TYPE}/lib/pkgconfig:/mingw/${MINGHOST}/lib/pkgconfig"
export SHLIB_LINK="-static-libstdc++ -lstdc++ -Wl,-Bdynamic -lm"
CPPFLAGS="-I${PGPATH}/include -I${PROJECTS}/rel-libiconv-${ICON_VER}w${OS_BUILD}${GCC_TYPE}/include"

#needed for proj.db to be found during cunit - for some reason on winnie it doesn't set
export PROJ_LIB=${PROJECTS}/proj/rel-${PROJ_VER}w${OS_BUILD}${GCC_TYPE}/share/proj

#add protobuf
export PATH="${PROJECTS}/protobuf/rel-${PROTOBUF_VER}w${OS_BUILD}${GCC_TYPE}/bin:${PROJECTS}/protobuf/rel-${PROTOBUF_VER}w${OS_BUILD}${GCC_TYPE}/lib:${PATH}"

echo PATH AFTER: $PATH

echo WORKSPACE IS $WORKSPACE
#mkdir ${PROJECTS}/postgis/tmp
export PGIS_REG_TMPDIR=${PROJECTS}/postgis/tmp/${POSTGIS_MICRO_VER}_pg${PG_VER}_geos${GEOS_VER}_gdal${GDAL_VER}w${OS_BUILD}
rm -rf ${PGIS_REG_TMPDIR}
mkdir ${PGIS_REG_TMPDIR}
export TMPDIR=${PGIS_REG_TMPDIR}

#rm -rf ${PGIS_REG_TMPDIR}
#TMPDIR=${PROJECTS}/postgis/tmp/${POSTGIS_VER}_${PG_VER}_${GEOS_VERSION}_${PROJ_VER}
echo PORT IS $PGPORT
echo PGIS_REG_TMPDIR IS $PGIS_REG_TMPDIR
export XSLTPROCFLAGS=
cd ${POSTGIS_SRC}
if [ -e ./GNUMakefile ]; then
	make distclean
fi

sh autogen.sh

if [ -n "$PCRE_VER" ]; then
    export PATH="${PROJECTS}/pcre/rel-${PCRE_VER}w${OS_BUILD}${GCC_TYPE}/include:${PROJECTS}/pcre/rel-${PCRE_VER}w${OS_BUILD}${GCC_TYPE}/lib:${PATH}"
fi



if [ -n "$SFCGAL_VER" ]; then
	##hard code versions of cgal etc. for now
	#export CGAL_VER=4.11
	BOOST_VER=1.59.0
	#BOOST_VER_WU=1_49_0
	export BOOST_VER_WU=1_59_0
	export PATH="${PROJECTS}/CGAL/rel-cgal-${CGAL_VER}w${OS_BUILD}${GCC_TYPE}/bin:${PROJECTS}/CGAL/rel-sfcgal-${SFCGAL_VER}w${OS_BUILD}${GCC_TYPE}/bin:${PROJECTS}/boost/rel-${BOOST_VER_WU}w${OS_BUILD}${GCC_TYPE}/lib:${PATH}"


#CPPFLAGS="-I${PGPATH}/include -I${PROJECTS}/rel-libiconv-${ICON_VER}w${OS_BUILD}${GCC_TYPE}/include" \
#CFLAGS="-Wall -fno-omit-frame-pointer"

#LDFLAGS="-Wl,--enable-auto-import -L${PGPATH}/lib -L${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}${GCC_TYPE}/lib -L${PROJECTS}/rel-libiconv-${ICON_VER}w${OS_BUILD}${GCC_TYPE}/lib" \

LDFLAGS="-Wl,--enable-auto-import -L${PGPATH}/lib -L${PROJECTS}/rel-libiconv-${ICON_VER}w${OS_BUILD}${GCC_TYPE}/lib -L${PROJECTS}/zlib/rel-zlib-${ZLIB_VER}w${OS_BUILD}${GCC_TYPE}/lib" \
./configure \
  --host=${MINGHOST} --with-xml2config=${PROJECTS}/libxml/rel-libxml2-${LIBXML_VER}w${OS_BUILD}${GCC_TYPE}/bin/xml2-config  \
  --with-pgconfig=${PGPATH}/bin/pg_config \
  --with-geosconfig=${PROJECTS}/geos/rel-${GEOS_VER}w${OS_BUILD}${GCC_TYPE}/bin/geos-config \
  --with-libiconv=${PROJECTS}/rel-libiconv-${ICON_VER}w${OS_BUILD}${GCC_TYPE} \
  --with-xsldir=${PROJECTS}/docbook/docbook-xsl-1.76.1 \
  --with-gui --with-gettext=no \
  --with-sfcgal=${PROJECTS}/CGAL/rel-sfcgal-${SFCGAL_VER}w${OS_BUILD}${GCC_TYPE}/bin/sfcgal-config \
  --without-interrupt-tests \
  --prefix=${PROJECTS}/postgis/liblwgeom-${POSTGIS_VER}w${OS_BUILD}${GCC_TYPE}
  #exit
else
CPPFLAGS="-I${PGPATH}/include -I${PROJECTS}/rel-libiconv-${ICON_VER}w${OS_BUILD}${GCC_TYPE}/include" \
CFLAGS="-Wall -fno-omit-frame-pointer" \
LDFLAGS="-L${PGPATH}/lib -L${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}${GCC_TYPE}/lib -L${PROJECTS}/rel-libiconv-${ICON_VER}w${OS_BUILD}${GCC_TYPE}/lib" ./configure \
  --host=${MINGHOST} --with-xml2config=${PROJECTS}/libxml/rel-libxml2-${LIBXML_VER}w${OS_BUILD}${GCC_TYPE}/bin/xml2-config \
  --with-pgconfig=${PGPATH}/bin/pg_config \
  --with-geosconfig=${PROJECTS}/geos/rel-${GEOS_VER}w${OS_BUILD}${GCC_TYPE}/bin/geos-config \
  --with-gui --with-gettext=no \
  --with-libiconv=${PROJECTS}/rel-libiconv-${ICON_VER}w${OS_BUILD}${GCC_TYPE} \
  --with-xsldir=${PROJECTS}/docbook/docbook-xsl-1.76.1 \
  --without-interrupt-tests \
  --prefix=${PROJECTS}/postgis/liblwgeom-${POSTGIS_VER}w${OS_BUILD}${GCC_TYPE}
fi;


make clean

#patch liblwgeom generated make to get rid of dynamic linking
#sed -i 's/LDFLAGS += -no-undefined//g' liblwgeom/Makefile

make
make install
make check RUNTESTFLAGS=-v

if [ "$MAKE_EXTENSION" == "1" ]; then
 export PGUSER=postgres
 #need to copy install files to EDB install (since not done by make install
 cd ${POSTGIS_SRC}
 echo "Postgis src dir is ${POSTGIS_SRC}"
 strip postgis/postgis-*.dll
 strip raster/rt_pg/postgis_raster-*.dll
 cp topology/*.dll ${PGPATHEDB}/lib
 cp postgis/postgis*.dll ${PGPATHEDB}/lib
 cp sfcgal/*.dll ${PGPATHEDB}/lib
 cp raster/rt_pg/postgis_raster-*.dll ${PGPATHEDB}/lib
 cp -r ${PGPATH}/share/extension/postgis*${POSTGIS_MICRO_VER}.sql ${PGPATHEDB}/share/extension
 cp -r ${PGPATH}/share/extension/postgis*${POSTGIS_MICRO_VER}next.sql ${PGPATHEDB}/share/extension
 cp -r ${PGPATH}/share/extension/address_standardizer*${POSTGIS_MICRO_VER}.sql ${PGPATHEDB}/share/extension
 cp -r extensions/*/*.control ${PGPATHEDB}/share/extension
 cp -r extensions/*/*.dll ${PGPATHEDB}/lib

 make check RUNTESTFLAGS="--extension -v"

 ##test address standardizer
 cd ${POSTGIS_SRC}
 cd extensions/address_standardizer
 make installcheck

 ##test tiger geocoder
 cd ${POSTGIS_SRC}
 cd extensions/postgis_tiger_geocoder
 make installcheck
 if [ "$?" != "0" ]; then
  exit $?
 fi
fi

if [ "$DUMP_RESTORE" = "1" ]; then
 echo "Dum restore test"
 make install
 make check RUNTESTFLAGS="-v --dumprestore"
 if [ "$?" != "0" ]; then
  exit $?
 fi
fi

if [ "$MAKE_GARDEN" == "1" ]; then
 export PGUSER=postgres
 make garden
fi
