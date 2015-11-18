#!/bin/bash
set -e
#### $Id:build_postgis210.sh 10208 2012-08-30 05:38:22Z robe $
#all these get passed in by jenkins
#export OS_BUILD=64 
#export PG_VER=9.2beta2
#export PGHOST=localhost
#export PGPORT=8442
#export PGUSER=postgres
#export GEOS_VER=3.4.0dev
#export GDAL_VER=1.9.1
#POSTGIS_SVN_REVISION=passed_in_by_buildbot
#POSTGIS_MAJOR_VERSION=2
#POSTGIS_MINOR_VERSION=1
#POSTGIS_MICRO_VERSION=0SVN
#export GCC_TYPE=gcc48  #for pre-4.8.0 compiles this is blank
#export JSON_VER=0.9
#export SFCGAL_VER=1.0.4
#export PROJ_VER=4.8.0


export LIBXML_VER=2.7.8

if [[ "${GCC_TYPE}" == *gcc48* ]] ; then
	export PROJECTS=/projects
	export MINGPROJECTS=/projects
	export PATHOLD=$PATH
else
	export PROJECTS=/projects
	export MINGPROJECTS=/projects
	export PATHOLD=$PATH
	#export JSON_VER=0.9
fi;
export PATHOLD=$PATH


if [ "$OS_BUILD" == "64" ] ; then
	export MINGHOST=x86_64-w64-mingw32
else
	export MINGHOST=i686-w64-mingw32
fi;

export PATHOLD="/mingw/bin:/mingw/include:/c/Windows/system32:/c/Windows:.:/bin:/include:/usr/local/bin:/c/ming${OS_BUILD}/svn"

export PGWINVER=${PG_VER}edb

echo PATH BEFORE: $PATH

export PGPATH=${PROJECTS}/postgresql/rel/pg${PG_VER}w${OS_BUILD}${GCC_TYPE}
#export PROJSO=libproj-0.dll

export POSTGIS_VER=${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}

export POSTGIS_MICRO_VER=${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}.${POSTGIS_MICRO_VERSION}
if [[ "$POSTGIS_MICRO_VERSION"  == *SVN* || "$POSTGIS_MICRO_VERSION"  == *dev* ]] ; then
	export POSTGIS_SRC=${PROJECTS}/postgis/branches/${POSTGIS_VER}
else
	#tagged version -- official release
	export POSTGIS_SRC=${PROJECTS}/postgis/tags/${POSTGIS_VER}.${POSTGIS_MICRO_VERSION}
fi;

#export POSTGIS_SRC=${PROJECTS}/postgis/trunk
export GDAL_DATA="${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}${GCC_TYPE}/share/gdal"
# export REL_PGVER=$(echo $PG_VER | tr '.' '') 
# echo $REL_PGVER
# export RELDIR=${MINGPROJECTS}/postgis/builds/postgis-21
# export RELVERDIR=postgis-pg${REL_PGVER}-binaries-${POSTGIS_MICRO_VER}w64

#export PATH="${PATHOLD}:${PGPATH}/bin:${PGPATH}/lib"
export PATH="${PGPATH}/bin:${PGPATH}/lib:${MINGPROJECTS}/rel-libiconv-1.13.1w${OS_BUILD}${GCC_TYPE}/include:${MINGPROJECTS}/rel-libiconv-1.13.1w${OS_BUILD}${GCC_TYPE}/bin:${MINGPROJECTS}/gtkw${OS_BUILD}${GCC_TYPE}/bin:${MINGPROJECTS}/gtk/bin:${PROJECTS}/geos/rel-${GEOS_VER}w${OS_BUILD}${GCC_TYPE}/bin:${MINGPROJECTS}/xsltproc:${PATH}"
echo PATH AFTER: $PATH

cd ${POSTGIS_SRC}
if [ -e ./GNUMakefile ]; then
	make distclean
fi
echo ${POSTGIS_SRC}
sh autogen.sh

#hack to get around boolean incompatibility
#-D__USE_MINGW_ANSI_STDIO=1
if [ "$JSON_VER" == "0.9" ] ; then
	cp ${MINGPROJECTS}/json-c/rel-${JSON_VER}w${OS_BUILD}${GCC_TYPE}/include/json/json_object.h.for_configure ${MINGPROJECTS}/json-c/rel-${JSON_VER}w${OS_BUILD}${GCC_TYPE}/include/json/json_object.h
fi
export XSLTPROCFLAGS=

#add  PCRE for address standardizer
if [ -n "$PCRE_VER" ]; then
    export PATH="${PROJECTS}/pcre/rel-${PCRE_VER}w${OS_BUILD}${GCC_TYPE}/include:${PROJECTS}/pcre/rel-${PCRE_VER}w${OS_BUILD}${GCC_TYPE}/lib:${PATH}"
fi

if [ -n "$SFCGAL_VER" ]; then
	##hard code versions of cgal etc. for now
	export CGAL_VER=4.2
	BOOST_VER=1.53.0
	#BOOST_VER_WU=1_49_0
	export BOOST_VER_WU=1_53_0
	export PATH="${PROJECTS}/CGAL/rel-cgal-${CGAL_VER}w${OS_BUILD}${GCC_TYPE}/bin:${PROJECTS}/boost/rel-${BOOST_VER_WU}w${OS_BUILD}${GCC_TYPE}/lib:${PATH}"
	
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
  --with-pcredir=${PROJECTS}/pcre/rel-${PCRE_VER}w${OS_BUILD}${GCC_TYPE}
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
#make clean
##hack to get around boolean incompatibility now only needed for 2.0 (no longer for 2.1)
if [ "$POSTGIS_MINOR_VERSION" == "0" ] ; then
cp ${MINGPROJECTS}/json-c/rel-${JSON_VER}w${OS_BUILD}${GCC_TYPE}/include/json/json_object.h.for_compile ${MINGPROJECTS}/json-c/rel-${JSON_VER}w${OS_BUILD}${GCC_TYPE}/include/json/json_object.h
fi;


#patch liblwgeom generated make to get rid of dynamic linking
sed -i 's/LDFLAGS += -no-undefined//g' liblwgeom/Makefile

make && make install

