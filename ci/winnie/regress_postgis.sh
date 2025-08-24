#!/usr/bin/env bash

. $(dirname $0)/winnie_common.sh

echo "MSYS2_ARG_CONV_EXCL=$MSYS2_ARG_CONV_EXCL"

echo PATH AFTER: $PATH
#export PG_VER=9.2beta2
export PGWINVER=${PG_VER}edb
export WORKSPACE=`pwd`

export POSTGIS_VER=${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}
export POSTGIS_MICRO_VER=${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}.${POSTGIS_MICRO_VERSION}

if [ -n "$SOURCE_FOLDER" ]; then
  export POSTGIS_SRC=${PROJECTS}/postgis/$SOURCE_FOLDER
	cd $POSTGIS_SRC
else
  if [[ "$POSTGIS_MICRO_VERSION"  == *SVN* || "$POSTGIS_MICRO_VERSION"  == *dev* ]] ; then
    export POSTGIS_SRC=${PROJECTS}/postgis/branches/${POSTGIS_VER}
  else
    #tagged version -- official release
    export POSTGIS_SRC=${PROJECTS}/postgis/tags/${POSTGIS_VER}.${POSTGIS_MICRO_VERSION}
  fi;
fi;

export POSTGIS_MAJOR_VERSION=`grep ^POSTGIS_MAJOR_VERSION Version.config | cut -d= -f2`
export POSTGIS_MINOR_VERSION=`grep ^POSTGIS_MINOR_VERSION Version.config | cut -d= -f2`
export POSTGIS_MICRO_VERSION=`grep ^POSTGIS_MICRO_VERSION Version.config | cut -d= -f2`
echo "Version ${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}.${POSTGIS_MICRO_VERSION}"

#export POSTGIS_SRC=${PROJECTS}/postgis/trunk
export GDAL_DATA="${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}${GCC_TYPE}/share/gdal"

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

if [ -e ./GNUMakefile ]; then
	make distclean
fi

sh autogen.sh

# excluding topology cause it's erroring out on some tests
#EXTRA_CONFIGURE_ARGS="${EXTRA_CONFIGURE_ARGS} --without-topology"

if [ -n "$PCRE_VER" ]; then
    export PATH="${PROJECTS}/pcre/rel-${PCRE_VER}w${OS_BUILD}${GCC_TYPE}/include:${PROJECTS}/pcre/rel-${PCRE_VER}w${OS_BUILD}${GCC_TYPE}/lib:${PATH}"
fi

if [ $INCLUDE_MINOR_LIB == "1" ]; then
  EXTRA_CONFIGURE_ARGS="${EXTRA_CONFIGURE_ARGS} --with-library-minor-version"
fi

if [ $REGRESS_WITHOUT_TOPOLOGY == "1" ]; then 
   EXTRA_CONFIGURE_ARGS="${EXTRA_CONFIGURE_ARGS} --without-topology"
fi

if [ $REGRESS_WITHOUT_RASTER == "1" ]; then 
   EXTRA_CONFIGURE_ARGS="${EXTRA_CONFIGURE_ARGS} --without-raster"
fi

#CPPFLAGS="-I${PGPATH}/include -I${PROJECTS}/rel-libiconv-${ICON_VER}w${OS_BUILD}${GCC_TYPE}/include" \
#CFLAGS="-Wall -fno-omit-frame-pointer"

#LDFLAGS="-Wl,--enable-auto-import -L${PGPATH}/lib -L${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}${GCC_TYPE}/lib -L${PROJECTS}/rel-libiconv-${ICON_VER}w${OS_BUILD}${GCC_TYPE}/lib" \

CPPFLAGS="-I${PGPATH}/include -I${PROJECTS}/rel-libiconv-${ICON_VER}w${OS_BUILD}${GCC_TYPE}/include" \
LDFLAGS="-Wl,--enable-auto-import -L${PGPATH}/lib -L${LZ4_PATH}/lib -L${PROJECTS}/rel-libiconv-${ICON_VER}w${OS_BUILD}${GCC_TYPE}/lib -L${PROJECTS}/zlib/rel-zlib-${ZLIB_VER}w${OS_BUILD}${GCC_TYPE}/lib" \
./configure \
  --host=${MINGHOST} --with-xml2config=${PROJECTS}/libxml/rel-libxml2-${LIBXML_VER}w${OS_BUILD}${GCC_TYPE}/bin/xml2-config  \
  --with-pgconfig=${PGPATH}/bin/pg_config \
  --with-geosconfig=${PROJECTS}/geos/rel-${GEOS_VER}w${OS_BUILD}${GCC_TYPE}/bin/geos-config \
  --with-libiconv=${PROJECTS}/rel-libiconv-${ICON_VER}w${OS_BUILD}${GCC_TYPE} \
  --with-gui --with-gettext=no \
  --with-sfcgal=${PROJECTS}/CGAL/rel-sfcgal-${SFCGAL_VER}w${OS_BUILD}${GCC_TYPE}/bin/sfcgal-config \
  --prefix=${PROJECTS}/postgis/liblwgeom-${POSTGIS_VER}w${OS_BUILD}${GCC_TYPE} --with-library-minor-version \
  ${EXTRA_CONFIGURE_ARGS}


#make distclean

#patch liblwgeom generated make to get rid of dynamic linking
#sed -i 's/LDFLAGS += -no-undefined//g' liblwgeom/Makefile

make -j 4
make install

# don't run tests twice. Only run regular if extension test is not asked for
if [ "$MAKE_EXTENSION" == "" ]; then
  make check RUNTESTFLAGS=-v
fi 


if [ "$MAKE_EXTENSION" == "1" ]; then
 export PGUSER=postgres
 #need to copy install files to EDB install (since not done by make install
 cd ${POSTGIS_SRC}
 echo "Postgis src dir is ${POSTGIS_SRC}"
 #strip postgis/postgis-*.dll
 #strip raster/rt_pg/postgis_raster-*.dll
 #strip sfcgal/*.dll

 if [ $REGRESS_WITHOUT_TOPOLOGY == "" ]; then 
    cp -r topology/*.dll ${PGPATHEDB}/lib
 fi
 cp postgis/postgis*.dll ${PGPATHEDB}/lib
 cp sfcgal/*.dll ${PGPATHEDB}/lib

 if [ $REGRESS_WITHOUT_RASTER == "" ]; then 
    cp raster/rt_pg/postgis_raster-*.dll ${PGPATHEDB}/lib
 fi


export POSTGIS_MINOR_VER=${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}
export POSTGIS_MINOR_MAX_VER="ANY"
export POSTGIS_MICRO_VER=${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}.${POSTGIS_MICRO_VERSION}

#export UPGRADE_VER_FILE="extensions/upgradeable_versions.mk"
value=$(<extensions/upgradeable_versions.mk)
export value=${value//\\/}
value=${value//UPGRADEABLE_VERSIONS = /}
#echo $value
export UPGRADEABLE_VERSIONS=$value
export WIN_RELEASED_VERSIONS="2.0.0 2.0.1 2.0.3 2.0.4 2.0.6 2.1.4 2.1.7 2.1.8 2.2.0 2.2.3 2.3.0 2.3.7 2.4.0 2.4.4"
export extensions_to_install="postgis postgis_sfcgal postgis_tiger_geocoder address_standardizer"

if [ $REGRESS_WITHOUT_TOPOLOGY == "" ]; then 
  extensions_to_install="${extensions_to_install} postgis_topology"
fi

if [ $REGRESS_WITHOUT_RASTER == "" ]; then 
  extensions_to_install="${extensions_to_install} postgis_raster"
fi


#echo "Versions are:  $UPGRADEABLE_VERSIONS"
for EXTNAME in $extensions_to_install; do
  
	cp extensions/$EXTNAME/sql/*  ${PGPATHEDB}/share/extension
	cp extensions/$EXTNAME/sql/$EXTNAME--TEMPLATED--TO--ANY.sql  ${PGPATHEDB}/share/extension/$EXTNAME--$POSTGIS_MICRO_VER--${POSTGIS_MINOR_MAX_VER}.sql;

	if test "$EXTNAME" = "address_standardizer"; then #repeat for address_standardizer_data_us
		cp extensions/$EXTNAME/sql/${EXTNAME}_data_us--${POSTGIS_MICRO_VER}.sql  ${PGPATHEDB}/share/extension
		cp extensions//$EXTNAME/sql/${EXTNAME}_data_us--ANY--${POSTGIS_MICRO_VER}.sql  ${PGPATHEDB}/share/extension
		cp extensions/$EXTNAME/sql/${EXTNAME}--TEMPLATED--TO--ANY.sql ${PGPATHEDB}/share/extension/${EXTNAME}_data_us--$POSTGIS_MICRO_VER--ANY.sql;
	fi

	for OLD_VERSION in $UPGRADEABLE_VERSIONS; do \
		if [ "$OLD_VERSION" > "2.5" ] || [ "$OLD_VERSION" in $WIN_RELEASED_VERSIONS ]; then
			cp extensions/$EXTNAME/sql/$EXTNAME--TEMPLATED--TO--ANY.sql  ${PGPATHEDB}/share/extension/$EXTNAME--$OLD_VERSION--${POSTGIS_MINOR_MAX_VER}.sql; \
			if test "$EXTNAME" = "address_standardizer"; then
				cp extensions/$EXTNAME/sql/$EXTNAME--TEMPLATED--TO--ANY.sql  ${PGPATHEDB}/share/extension/${EXTNAME}_data_us--$OLD_VERSION--${POSTGIS_MINOR_MAX_VER}.sql; \
			fi
		fi
	done
done
 #cp -r ${PGPATH}/share/extension/postgis*${POSTGIS_MICRO_VER}.sql ${PGPATHEDB}/share/extension
 #cp -r ${PGPATH}/share/extension/postgis*${POSTGIS_MICRO_VER}next.sql ${PGPATHEDB}/share/extension
 #cp -r ${PGPATH}/share/extension/address_standardizer*${POSTGIS_MICRO_VER}.sql ${PGPATHEDB}/share/extension
cp -r extensions/*/*.control ${PGPATHEDB}/share/extension
cp -r extensions/*/*.dll ${PGPATHEDB}/lib

make check RUNTESTFLAGS="--extension -v"

if [ "$UPGRADE_TEST" == "1" ]; then
  export CURRENTVERSION=${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}.${POSTGIS_MICRO_VERSION}
  RUNTESTFLAGS='--extension' ${POSTGIS_SRC}/utils/check_all_upgrades.sh -s "${CURRENTVERSION}" --skip "unpackaged"
fi

 #test address standardizer
 cd ${POSTGIS_SRC}
 cd extensions/address_standardizer
 make installcheck

#test tiger geocoder
#  cd ${POSTGIS_SRC}
#  cd extensions/postgis_tiger_geocoder
#  make installcheck
#  if [ "$?" != "0" ]; then
#   exit $?
#  fi
#end extension
fi

if [ "$DUMP_RESTORE" == "1" ]; then
 echo "Dump restore test"
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
