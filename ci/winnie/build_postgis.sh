#!/usr/bin/env bash

. $(dirname $0)/winnie_common.sh

if [ -n "$SOURCE_FOLDER" ]; then
  export POSTGIS_SRC=${PROJECTS}/postgis/$SOURCE_FOLDER
  cd ${POSTGIS_SRC}
else
  if [[ "$POSTGIS_MICRO_VERSION"  == *SVN* || "$POSTGIS_MICRO_VERSION"  == *dev* ]] ; then
    export POSTGIS_SRC=${PROJECTS}/postgis/branches/${POSTGIS_VER}
  else
    #tagged version -- official release
    export POSTGIS_SRC=${PROJECTS}/postgis/tags/${POSTGIS_VER}.${POSTGIS_MICRO_VERSION}
    INCLUDE_MINOR_LIB=0
  fi;
fi;

export POSTGIS_VER=${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}
export POSTGIS_MICRO_VER=${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}.${POSTGIS_MICRO_VERSION}


#export POSTGIS_SRC=${PROJECTS}/postgis/trunk
export RELVERDIR=postgis-pg${REL_PGVER}-binaries-${POSTGIS_MICRO_VER}w${OS_BUILD}


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
# if [ -e ./GNUMakefile ]; then
# 	make distclean
# fi

git clean -dxf # ensure we start fresh
git describe --long --all --tags # Report where we are at

sh autogen.sh

if [ -n "$PCRE_VER" ]; then
    export PATH="${PROJECTS}/pcre/rel-${PCRE_VER}w${OS_BUILD}${GCC_TYPE}/include:${PROJECTS}/pcre/rel-${PCRE_VER}w${OS_BUILD}${GCC_TYPE}/lib:${PATH}"
fi

if [$INCLUDE_MINOR_LIB == "1"]; then

#CPPFLAGS="-I${PGPATH}/include -I${PROJECTS}/rel-libiconv-${ICON_VER}w${OS_BUILD}${GCC_TYPE}/include" \
#CFLAGS="-Wall -fno-omit-frame-pointer"

#LDFLAGS="-Wl,--enable-auto-import -L${PGPATH}/lib -L${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}${GCC_TYPE}/lib -L${PROJECTS}/rel-libiconv-${ICON_VER}w${OS_BUILD}${GCC_TYPE}/lib" \

LDFLAGS="-Wl,--enable-auto-import -L${PGPATH}/lib -L${LZ4_PATH}/bin -L${PROJECTS}/rel-libiconv-${ICON_VER}w${OS_BUILD}${GCC_TYPE}/lib -L${PROJECTS}/zlib/rel-zlib-${ZLIB_VER}w${OS_BUILD}${GCC_TYPE}/lib" \
./configure \
  --host=${MINGHOST} --with-xml2config=${PROJECTS}/libxml/rel-libxml2-${LIBXML_VER}w${OS_BUILD}${GCC_TYPE}/bin/xml2-config  \
  --with-pgconfig=${PGPATH}/bin/pg_config \
  --with-geosconfig=${PROJECTS}/geos/rel-${GEOS_VER}w${OS_BUILD}${GCC_TYPE}/bin/geos-config \
  --with-libiconv=${PROJECTS}/rel-libiconv-${ICON_VER}w${OS_BUILD}${GCC_TYPE} \
  --with-xsldir=${PROJECTS}/docbook/docbook-xsl-1.76.1 \
  --with-gui --with-gettext=no \
  --with-sfcgal=${PROJECTS}/CGAL/rel-sfcgal-${SFCGAL_VER}w${OS_BUILD}${GCC_TYPE}/bin/sfcgal-config \
  --prefix=${PROJECTS}/postgis/liblwgeom-${POSTGIS_VER}w${OS_BUILD}${GCC_TYPE} --with-library-minor-version
  #exit
else
LDFLAGS="-Wl,--enable-auto-import -L${PGPATH}/lib -L${LZ4_PATH}/bin -L${PROJECTS}/rel-libiconv-${ICON_VER}w${OS_BUILD}${GCC_TYPE}/lib -L${PROJECTS}/zlib/rel-zlib-${ZLIB_VER}w${OS_BUILD}${GCC_TYPE}/lib" \
./configure \
  --host=${MINGHOST} --with-xml2config=${PROJECTS}/libxml/rel-libxml2-${LIBXML_VER}w${OS_BUILD}${GCC_TYPE}/bin/xml2-config  \
  --with-pgconfig=${PGPATH}/bin/pg_config \
  --with-geosconfig=${PROJECTS}/geos/rel-${GEOS_VER}w${OS_BUILD}${GCC_TYPE}/bin/geos-config \
  --with-libiconv=${PROJECTS}/rel-libiconv-${ICON_VER}w${OS_BUILD}${GCC_TYPE} \
  --with-xsldir=${PROJECTS}/docbook/docbook-xsl-1.76.1 \
  --with-gui --with-gettext=no \
  --with-sfcgal=${PROJECTS}/CGAL/rel-sfcgal-${SFCGAL_VER}w${OS_BUILD}${GCC_TYPE}/bin/sfcgal-config \
  --prefix=${PROJECTS}/postgis/liblwgeom-${POSTGIS_VER}w${OS_BUILD}${GCC_TYPE}
fi;

make clean

#patch liblwgeom generated make to get rid of dynamic linking
#sed -i 's/LDFLAGS += -no-undefined//g' liblwgeom/Makefile

make
make install
