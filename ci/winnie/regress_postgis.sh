#!/bin/bash
set -e
#winnie passed in variables
#export OS_BUILD=64
#export GCC_TYPE=gcc48
#export PG_VER=9.3
#export PROJ_VER=4.8.0
#export PROJSO=libproj-0.dll
#export POSTGIS_MAJOR_VERSION=2
#export POSTGIS_MINOR_VERSION=2
#export POSTGIS_MICRO_VERSION=0dev


export PROJECTS=/projects
export MINGPROJECTS=/projects
export PATHOLD=$PATH


if [ "$OS_BUILD" == "64" ] ; then
	export MINGHOST=x86_64-w64-mingw32
else
	export MINGHOST=i686-w64-mingw32
fi;

export PATHOLD="/mingw/bin:/mingw/include:/mingw/lib:/c/Windows/system32:/c/Windows:.:/bin:/include:/usr/local/bin:/c/ming${OS_BUILD}/svn"
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
if [[ "$POSTGIS_MICRO_VERSION"  == *SVN* || "$POSTGIS_MICRO_VERSION"  == *dev* ]] ; then
	export POSTGIS_SRC=${PROJECTS}/postgis/branches/${POSTGIS_VER}
else
	#tagged version -- official release
	export POSTGIS_SRC=${PROJECTS}/postgis/tags/${POSTGIS_VER}.${POSTGIS_MICRO_VERSION}
fi;
export LIBXML_VER=2.7.8
#export POSTGIS_SRC=${PROJECTS}/postgis/trunk
export GDAL_DATA="${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}${GCC_TYPE}/share/gdal"

export RELVERDIR=postgis-pg${REL_PGVER}-binaries-${POSTGIS_MICRO_VER}w${OS_BUILD}

export PATH="${PATHOLD}:${PGPATH}/bin:${PGPATH}/lib"
#PATH="${MINGPROJECTS}/gettext/rel-gettext-0.18.1/bin:${MINGPROJECTS}/xsltproc:${MINGPROJECTS}/gtk/bin:${PROJECTS}/geos/rel-${GEOS_VER}w${OS_BUILD}/bin:${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}/bin:${MINGPROJECTS}/rel-libiconv-1.13.1w${OS_BUILD}/include:${MINGPROJECTS}/rel-libiconv-1.13.1w${OS_BUILD}/bin:${MINGPROJECTS}/proj/rel-${PROJ_VER}w${OS_BUILD}/bin:${MINGPROJECTS}/libxml/rel-libxml2-${LIBXML_VER}w${OS_BUILD}/bin:${PATH}"
PATH="${MINGPROJECTS}/xsltproc:${MINGPROJECTS}/gtkw${OS_BUILD}${GCC_TYPE}/bin:${PROJECTS}/geos/rel-${GEOS_VER}w${OS_BUILD}${GCC_TYPE}/bin:${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}${GCC_TYPE}/bin:${MINGPROJECTS}/rel-libiconv-1.13.1w${OS_BUILD}${GCC_TYPE}/lib:${MINGPROJECTS}/rel-libiconv-1.13.1w${OS_BUILD}${GCC_TYPE}/include:${MINGPROJECTS}/rel-libiconv-1.13.1w${OS_BUILD}${GCC_TYPE}/bin:${MINGPROJECTS}/proj/rel-${PROJ_VER}w${OS_BUILD}${GCC_TYPE}/bin:${MINGPROJECTS}/libxml/rel-libxml2-${LIBXML_VER}w${OS_BUILD}${GCC_TYPE}/bin:${PATH}"
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
	export CGAL_VER=4.6.1
	BOOST_VER=1.59.0
	
	export BOOST_VER_WU=1_59_0
	export PATH="${PROJECTS}/CGAL/rel-cgal-${CGAL_VER}w${OS_BUILD}${GCC_TYPE}/bin:${PROJECTS}/boost/rel-${BOOST_VER_WU}w${OS_BUILD}${GCC_TYPE}/lib:${PATH}"
	
	#this is a major cludge but only building with pcre support if SFCGAL is built - can't figure out how to do this without passing PCRE
	#TODO: turn back on topology regression testing once topology issues resolved
	CPPFLAGS="-I${PGPATH}/include -I${MINGPROJECTS}/rel-libiconv-1.13.1w${OS_BUILD}${GCC_TYPE}/include" \
LDFLAGS="-L${PGPATH}/lib -L${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}${GCC_TYPE}/lib -L${MINGPROJECTS}/rel-libiconv-1.13.1w${OS_BUILD}${GCC_TYPE}/lib" ./configure \
  --host=${MINGHOST} --with-xml2config=${PROJECTS}/libxml/rel-libxml2-${LIBXML_VER}w${OS_BUILD}${GCC_TYPE}/bin/xml2-config \
  --with-pgconfig=${PGPATH}/bin/pg_config \
  --with-geosconfig=${PROJECTS}/geos/rel-${GEOS_VER}w${OS_BUILD}${GCC_TYPE}/bin/geos-config \
  --with-projdir=${MINGPROJECTS}/proj/rel-${PROJ_VER}w${OS_BUILD}${GCC_TYPE} \
  --with-gdalconfig=${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}${GCC_TYPE}/bin/gdal-config \
  --with-jsondir=${MINGPROJECTS}/json-c/rel-${JSON_VER}w${OS_BUILD}${GCC_TYPE} \
  --with-libiconv=${PROJECTS}/rel-libiconv-1.13.1w${OS_BUILD}${GCC_TYPE} \
  --with-xsldir=${PROJECTS}/docbook/docbook-xsl-1.76.1 \
  --with-gui --with-gettext=no --with-sfcgal=${PROJECTS}/CGAL/rel-sfcgal-${SFCGAL_VER}w${OS_BUILD}${GCC_TYPE}/bin/sfcgal-config \
  --with-pcredir=${PROJECTS}/pcre/rel-${PCRE_VER}w${OS_BUILD}${GCC_TYPE}  --without-interrupt-tests
elif [ "$POSTGIS_MAJOR_VERSION" == "2" ] ; then
CPPFLAGS="-I${PGPATH}/include -I${MINGPROJECTS}/rel-libiconv-1.13.1w${OS_BUILD}${GCC_TYPE}/include" \
LDFLAGS="-L${PGPATH}/lib -L${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}${GCC_TYPE}/lib -L${MINGPROJECTS}/rel-libiconv-1.13.1w${OS_BUILD}${GCC_TYPE}/lib" ./configure \
  --host=${MINGHOST} --with-xml2config=${PROJECTS}/libxml/rel-libxml2-${LIBXML_VER}w${OS_BUILD}${GCC_TYPE}/bin/xml2-config \
  --with-pgconfig=${PGPATH}/bin/pg_config \
  --with-geosconfig=${PROJECTS}/geos/rel-${GEOS_VER}w${OS_BUILD}${GCC_TYPE}/bin/geos-config \
  --with-projdir=${MINGPROJECTS}/proj/rel-${PROJ_VER}w${OS_BUILD}${GCC_TYPE} \
  --with-gdalconfig=${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}${GCC_TYPE}/bin/gdal-config \
  --with-jsondir=${MINGPROJECTS}/json-c/rel-${JSON_VER}w${OS_BUILD}${GCC_TYPE} \
  --with-libiconv=${PROJECTS}/rel-libiconv-1.13.1w${OS_BUILD}${GCC_TYPE} \
  --with-xsldir=${PROJECTS}/docbook/docbook-xsl-1.76.1 \
  --with-gui --with-gettext=no
else
CPPFLAGS="-I${PGPATH}/include  -I${MINGPROJECTS}/rel-libiconv-1.13.1w${OS_BUILD}${GCC_TYPE}/include" \
    LDFLAGS="-L${PGPATH}/lib -L${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}${GCC_TYPE}/lib -L${MINGPROJECTS}/rel-libiconv-1.13.1w${OS_BUILD}${GCC_TYPE}/lib" ./configure \
  --host=${MINGHOST} --with-xml2config=${PROJECTS}/libxml/rel-libxml2-${LIBXML_VER}w${OS_BUILD}${GCC_TYPE}/bin/xml2-config \
  --with-pgconfig=${PGPATH}/bin/pg_config \
  --with-geosconfig=${PROJECTS}/geos/rel-${GEOS_VER}w${OS_BUILD}${GCC_TYPE}/bin/geos-config \
  --with-projdir=${MINGPROJECTS}/proj/rel-${PROJ_VER}w${OS_BUILD}${GCC_TYPE} \
  --with-gdalconfig=${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}${GCC_TYPE}/bin/gdal-config \
  --with-jsondir=${MINGPROJECTS}/json-c/rel-${JSON_VER}w${OS_BUILD}${GCC_TYPE} \
  --with-libiconv=${PROJECTS}/rel-libiconv-1.13.1w${OS_BUILD}${GCC_TYPE} \
  --with-xsldir=${PROJECTS}/docbook/docbook-xsl-1.76.1 \
  --with-gettext=no
fi;

make clean

##hack to get around boolean incompatibility
##hack to get around boolean incompatibility now only needed for 2.0 (no longer for 2.1)
if [ "$POSTGIS_MINOR_VERSION" == "0" ] ; then
cp ${MINGPROJECTS}/json-c/rel-${JSON_VER}w${OS_BUILD}${GCC_TYPE}/include/json/json_object.h.for_compile ${MINGPROJECTS}/json-c/rel-${JSON_VER}w${OS_BUILD}${GCC_TYPE}/include/json/json_object.h
fi;

#patch liblwgeom generated make to get rid of dynamic linking
sed -i 's/LDFLAGS += -no-undefined//g' liblwgeom/Makefile

make
make install
make check RUNTESTFLAGS=-v

if [ "$MAKE_EXTENSION" == "1" ]; then
 export PGUSER=postgres
 #need to copy install files to EDB install (since not done by make install
 cd ${POSTGIS_SRC}
 echo "Postgis src dir is ${POSTGIS_SRC}"
 strip postgis/postgis-*.dll
 strip raster/rt_pg/rtpostgis-*.dll
 cp topology/*.dll ${PGPATHEDB}/lib
 cp postgis/postgis*.dll ${PGPATHEDB}/lib
 cp raster/rt_pg/rtpostgis-*.dll ${PGPATHEDB}/lib
 cp -r extensions/*/sql/* ${PGPATHEDB}/share/extension
 cp -r extensions/*/*.control ${PGPATHEDB}/share/extension
 cp -r extensions/*/*.dll ${PGPATHEDB}/lib
 
 make check RUNTESTFLAGS=--extension
 
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

if [ "$MAKE_GARDEN" == "1" ]; then
 export PGUSER=postgres
 make garden
fi