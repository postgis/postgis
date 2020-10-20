#### $Id: package_postgis210.sh 10291 2012-09-15 05:37:23Z robe $
### this script is used to zip up the compiled binaries
## PostgreSQL, OS_BUILD denote the last build to be packaged
## and are passed in by the jenkins job process
## the scp to postgis website is done using Jenkins scp plugin as a final step
###
#export OS_BUILD=64
#export PGPORT=8442
#POSTGIS_MAJOR_VERSION=2
#POSTGIS_MINOR_VERSION=1
#POSTGIS_MICRO_VERSION=0dev
#export OS_BUILD=32

#export GCC_TYPE=
#if no override is set - use these values
#otherwise use the ones jenkins passes thru
#!/usr/bin/env bash
if  [[ "${OVERRIDE}" == '' ]] ; then
	export GEOS_VER=3.8.1
	export GDAL_VER=3.0.4
	export PROJ_VER=6.3.2
	export SFCGAL_VER=1.3.8
	export CGAL_VER=5.0
	export ICON_VER=1.16
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
  export ICON_VER=1.15
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

export PROJECTS=/projects
export PROJECTS=/projects
export PATHOLD=$PATH


export PGHOST=localhost

export PGUSER=postgres

export PATHOLD=$PATH
WEB=/home/www/postgis/htdocs
DWN=${WEB}/download

export PATHOLD="/mingw/bin:/mingw/include:/c/Windows/system32:/c/Windows:.:/bin:/include:/usr/local/bin"
#export PG_VER=9.2beta2

echo PATH BEFORE: $PATH

export PGPATH=${PROJECTS}/postgresql/rel/pg${PG_VER}w${OS_BUILD}${GCC_TYPE}
export PGPATHEDB=${PGPATH}edb
#export PROJSO=libproj-13.dll
export POSTGIS_MINOR_VER=${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}
export POSTGIS_MICRO_VER=${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}.${POSTGIS_MICRO_VERSION}

if [ -n "$SOURCE_FOLDER" ]; then
  export POSTGIS_SRC=${PROJECTS}/postgis/$SOURCE_FOLDER
fi

#export POSTGIS_SRC=${PROJECTS}/postgis/trunk
#POSTGIS_SVN_REVISION=will_be_passed_in_by_bot
export GDAL_DATA="${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}${GCC_TYPE}/share/gdal"


export REL_PGVER=${PG_VER//./} #strip the period


export RELDIR=${PROJECTS}/postgis/builds/${POSTGIS_MINOR_VER}
export RELVERDIR=postgis-pg${REL_PGVER}-binaries-${POSTGIS_MICRO_VER}w${OS_BUILD}${GCC_TYPE}
export PATH="${PATHOLD}:${PGPATH}/bin:${PGPATH}/lib"
export PCRE_VER=8.33
#PATH="${PGPATH}/bin:${PGPATH}/lib:${PROJECTS}/xsltproc:${PROJECTS}/gtkw${OS_BUILD}${GCC_TYPE}/bin:${PROJECTS}/gtkw${OS_BUILD}/bin:${PROJECTS}/geos/rel-${GEOS_VER}w${OS_BUILD}${GCC_TYPE}/bin:${PROJECTS}/rel-libiconv-${ICON_VER}w${OS_BUILD}${GCC_TYPE}/include:${PROJECTS}/rel-libiconv-1.13.1w${OS_BUILD}${GCC_TYPE}/bin:${PATH}"
#echo PATH AFTER: $PATH
outdir="${RELDIR}/${RELVERDIR}"
package="${RELDIR}/${RELVERDIR}.zip"
verfile="${RELDIR}/${RELVERDIR}/version.txt"
rm -rf $outdir
rm $package
mkdir -p $outdir
mkdir -p $outdir/share/contrib/postgis-${POSTGIS_MINOR_VER}
mkdir -p $outdir/share/contrib/postgis-${POSTGIS_MINOR_VER}/proj
mkdir -p $outdir/share/extension
mkdir $outdir/bin
mkdir $outdir/lib
mkdir $outdir/bin/postgisgui
mkdir $outdir/bin/postgisgui/share
mkdir $outdir/bin/postgisgui/lib
mkdir $outdir/utils
cp ${PROJECTS}/rel-libiconv-${ICON_VER}w${OS_BUILD}${GCC_TYPE}/bin/*.dll  $outdir/bin/postgisgui
# it seems 9.2 and 9.3 doesn't come with its own libiconv good grief
# and trying to use their libiconv2.dll makes shp2pgsql crash
if [[ "$PG_VER" == *9.2* || "$PG_VER" == *9.3* ]]; then
	cp ${PROJECTS}/rel-libiconv-1.13.1w${OS_BUILD}${GCC_TYPE}/bin/*.dll  $outdir/bin
fi;
cp ${PGPATHEDB}/bin/libpq.dll  $outdir/bin/postgisgui
#cp ${PGPATHEDB}/bin/libiconv2.dll  $outdir/bin/postgisgui
cp ${PROJECTS}/rel-libiconv-${ICON_VER}w${OS_BUILD}${GCC_TYPE}/bin/libicon*.dll $outdir/bin/postgisgui
cp ${PGPATHEDB}/bin/libintl*.dll $outdir/bin/postgisgui

cp ${PGPATHEDB}/bin/ssleay32.dll $outdir/bin/postgisgui
cp ${PGPATHEDB}/bin/libeay32.dll $outdir/bin/postgisgui
cp ${PGPATHEDB}/bin/libcrypto-1_1-x64.dll $outdir/bin/postgisgui
cp ${PGPATHEDB}/bin/libssl-1_1-x64.dll $outdir/bin/postgisgui

cp /c/ming${OS_BUILD}${GCC_TYPE}/mingw${OS_BUILD}/bin/libstdc++-6.dll $outdir/bin
cp /c/ming${OS_BUILD}${GCC_TYPE}/mingw${OS_BUILD}/bin/libgcc*.dll $outdir/bin
cp -r ${PROJECTS}/gtkw${OS_BUILD}${GCC_TYPE}/bin/*.dll $outdir/bin/postgisgui
cp /c/ming${OS_BUILD}${GCC_TYPE}/mingw${OS_BUILD}/bin/libstdc++-6.dll $outdir/bin/postgisgui
cp /c/ming${OS_BUILD}${GCC_TYPE}/mingw${OS_BUILD}/bin/libgcc*.dll $outdir/bin/postgisgui
cp -r ${PROJECTS}/gtkw${OS_BUILD}${GCC_TYPE}/etc $outdir/bin/postgisgui
cp -r ${PROJECTS}/gtkw${OS_BUILD}${GCC_TYPE}/share/themes $outdir/bin/postgisgui/share
cp -r ${PROJECTS}/gtkw${OS_BUILD}${GCC_TYPE}/lib/gtk-2.0 $outdir/bin/postgisgui/lib
cp -r ${PROJECTS}/gtkw${OS_BUILD}${GCC_TYPE}/lib/*.dll $outdir/bin/postgisgui/lib
cp -r ${PROJECTS}/gtkw${OS_BUILD}${GCC_TYPE}/lib/gdk-pixbuf-2.0 $outdir/bin/postgisgui/lib


cp ${PROJECTS}/proj/rel-${PROJ_VER}w${OS_BUILD}${GCC_TYPE}/share/proj/* $outdir/share/contrib/postgis-${POSTGIS_MINOR_VER}/proj
cp ${PROJECTS}/proj/rel-${PROJ_VER}w${OS_BUILD}${GCC_TYPE}/bin/*.dll $outdir/bin
cp -p ${PROJECTS}/geos/rel-${GEOS_VER}w${OS_BUILD}${GCC_TYPE}/bin/*.dll $outdir/bin

cp ${PROJECTS}/proj/rel-${PROJ_VER}w${OS_BUILD}${GCC_TYPE}/bin/*.dll $outdir/bin/postgisgui
cp -p ${PROJECTS}/geos/rel-${GEOS_VER}w${OS_BUILD}${GCC_TYPE}/bin/*.dll $outdir/bin/postgisgui

#for protobuf
cp ${PROJECTS}/protobuf/rel-${PROTOBUF_VER}w${OS_BUILD}${GCC_TYPE}/bin/libprotobuf-c-*.dll $outdir/bin

echo "POSTGIS: ${POSTGIS_MINOR_VER} r${POSTGIS_SVN_REVISION} http://postgis.net/source" > $verfile

if [ "$POSTGIS_MAJOR_VERSION" > "1" ] ; then
  ## only copy gdal components if 2+.  1.5 doesn't have raster support
  cp -p ${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}${GCC_TYPE}/bin/*.dll $outdir/bin
  cp -rp  ${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}${GCC_TYPE}/share/gdal $outdir/gdal-data

  if [ "$POSTGIS_MINOR_VERSION" > "0" ] ; then
    ## only copy pagc standardizer components for 2.1+
    cp -p ${PROJECTS}/pcre/rel-${PCRE_VER}w${OS_BUILD}${GCC_TYPE}/bin/libpcre-1*.dll $outdir/bin
    # cp -p ${PGPATH}/share/extension/address*.* $outdir/share/extension
    # cp -p ${PGPATH}/share/extension/us-*.sql $outdir/share/extension
  fi;
fi;

if [ -n "$SFCGAL_VER"  ]; then
	## only copy cgal and sfcgal stuff if sfcgal is packaged
	export BOOST_VER=1.59.0
	export BOOST_VER_WU=1_59_0
	export GMP_VER=5.1.2
	export MPFR_VER=3.1.2
	echo "CGAL VERSION: ${CGAL_VER} http://www.cgal.org" >> $verfile
	echo "Boost VERSION: ${BOOST_VER} http://www.boost.org" >> $verfile
	echo "GMP VERSION: ${GMP_VER} https://gmplib.org" >> $verfile
	echo "MPFR VERSION: ${MPFR_VER} http://www.mpfr.org" >> $verfile

	cp -p ${PROJECTS}/CGAL/rel-cgal-${CGAL_VER}w${OS_BUILD}${GCC_TYPE}/bin/*.dll $outdir/bin
	cp -p ${PROJECTS}/CGAL/rel-sfcgal-${SFCGAL_VER}w${OS_BUILD}${GCC_TYPE}/lib/*.dll $outdir/bin
	# cp -p ${PROJECTS}/CGAL/rel-cgal-${CGAL_VER}w${OS_BUILD}${GCC_TYPE}/bin/*.dll $outdir/bin/postgisgui
	# cp -p ${PROJECTS}/CGAL/rel-sfcgal-${SFCGAL_VER}w${OS_BUILD}${GCC_TYPE}/lib/*.dll $outdir/bin/postgisgui
fi;

echo "PROTOBUF VERSION: ${PROTOBUF_VER} https://github.com/google/protobuf" >> $verfile
echo "PROTOBUF-C VERSION: ${PROTOBUFC_VER} https://github.com/protobuf-c/protobuf-c"  >> $verfile
cp ${PROJECTS}/libxml/rel-libxml2-${LIBXML_VER}w${OS_BUILD}${GCC_TYPE}/bin/*.dll  $outdir/bin/
#cp ${PGPATHEDB}/bin/libxml2-2.dll   $outdir/bin/

cd ${POSTGIS_SRC}
strip postgis/*.dll
strip sfcgal/*.dll
strip raster/rt_pg/*.dll
strip liblwgeom/.libs/*.dll

cp postgis/*.dll ${RELDIR}/${RELVERDIR}/lib
cp sfcgal/*.dll ${RELDIR}/${RELVERDIR}/lib
cp topology/*.dll ${RELDIR}/${RELVERDIR}/lib
cp raster/rt_pg/*.dll ${RELDIR}/${RELVERDIR}/lib
cp doc/*_comments.sql ${RELDIR}/${RELVERDIR}/share/contrib/postgis-${POSTGIS_MINOR_VER}
cp postgis/*.sql ${RELDIR}/${RELVERDIR}/share/contrib/postgis-${POSTGIS_MINOR_VER}
cp raster/rt_pg/*.sql ${RELDIR}/${RELVERDIR}/share/contrib/postgis-${POSTGIS_MINOR_VER}
cp utils/*.pl ${RELDIR}/${RELVERDIR}/utils

cp raster/loader/.libs/raster2pgsql.exe ${RELDIR}/${RELVERDIR}/bin
cp raster/loader/raster2pgsql.exe ${RELDIR}/${RELVERDIR}/bin
cp liblwgeom/.libs/*.dll ${RELDIR}/${RELVERDIR}/bin
cp loader/shp2pgsql.exe ${RELDIR}/${RELVERDIR}/bin
cp loader/.libs/shp2pgsql.exe ${RELDIR}/${RELVERDIR}/bin
cp loader/pgsql2shp.exe ${RELDIR}/${RELVERDIR}/bin
cp loader/.libs/pgsql2shp.exe ${RELDIR}/${RELVERDIR}/bin
cp loader/shp2pgsql-gui.exe ${RELDIR}/${RELVERDIR}/bin/postgisgui
cp loader/.libs/shp2pgsql-gui.exe ${RELDIR}/${RELVERDIR}/bin/postgisgui

#cp liblwgeom/.libs/*.dll ${RELDIR}/${RELVERDIR}/bin/postgisgui

#shp2pgsql-gui now has dependency on geos (though in theory it shouldn't)
cp -p ${PROJECTS}/geos/rel-${GEOS_VER}w${OS_BUILD}${GCC_TYPE}/bin/*.dll ${RELDIR}/${RELVERDIR}/bin/postgisgui
cp spatial_ref_sys.sql ${RELDIR}/${RELVERDIR}/share/contrib/postgis-${POSTGIS_MINOR_VER}
cp topology/topology.sql ${RELDIR}/${RELVERDIR}/share/contrib/postgis-${POSTGIS_MINOR_VER}
cp topology/topology_upgrade_*.sql ${RELDIR}/${RELVERDIR}/share/contrib/postgis-${POSTGIS_MINOR_VER}
#cp topology/README* ${RELDIR}/${RELVERDIR}/share/contrib/postgis-${POSTGIS_MINOR_VER}
#cp utils/* ${RELDIR}/${RELVERDIR}/utils
#cp extras/* ${RELDIR}/${RELVERDIR}/share/contrib/postgis-${POSTGIS_MINOR_VER}/extras
cp ${PGPATH}/share/extension/postgis*${POSTGIS_MICRO_VER}.sql ${RELDIR}/${RELVERDIR}/share/extension
cp ${PGPATH}/share/extension/postgis*${POSTGIS_MICRO_VER}next.sql ${RELDIR}/${RELVERDIR}/share/extension
cp ${PGPATH}/share/extension/address_standardizer*${POSTGIS_MICRO_VER}.sql ${RELDIR}/${RELVERDIR}/share/extension
cp -r extensions/*/*.control ${RELDIR}/${RELVERDIR}/share/extension
cp -r extensions/*/*.dll ${RELDIR}/${RELVERDIR}/lib #only address_standardizer in theory has this
#cp extensions/postgis_topology/sql/* ${RELDIR}/${RELVERDIR}/share/extension
#cp extensions/postgis_topology/*.control ${RELDIR}/${RELVERDIR}/share/extension
cp -r ${RELDIR}/packaging_notes/* ${RELDIR}/${RELVERDIR}/


echo "GEOS VERSION: ${GEOS_VER} https://trac.osgeo.org/geos" >> $verfile
echo "GDAL VERSION: ${GDAL_VER} https://gdal.org/download.html#current-releases" >> $verfile
echo "PROJ VERSION: ${PROJ_VER} https://proj.org/download.html#current-release" >> $verfile

echo "LIBICONV VERSION: ${ICON_VER} http://ftp.gnu.org/gnu/libiconv/libiconv-${ICONV_VER}.tar.gz" >> $verfile

if [ -n "$SFCGAL_VER"  ]; then
    echo "CGAL VERSION: ${CGAL_VER} http://www.cgal.org" >> $verfile
    echo "BOOST VERSION: ${BOOST_VER} http://www.boost.org" >> $verfile
    echo "SFCGAL VERSION: ${SFCGAL_VER} http://www.sfcgal.org https://github.com/Oslandia/SFCGAL" >> $verfile
fi;

if [ -f ${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}${GCC_TYPE}/gdal_depends.txt ]; then
	cat $verfile ${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}${GCC_TYPE}/gdal_depends.txt > $verfile
fi
#echo "PAGC ADDRESS STANDARDIZER: http://sourceforge.net/p/pagc/code/HEAD/tree/branches/sew-refactor/postgresql " >> $verfile
cd ${RELDIR}
zip -r $package ${RELVERDIR}
#scp $package robe@www.refractions.net:${DWN}/${REL_PGVER}/buildbot/${RELVERDIR}.zip
cp $package ${PROJECTS}/postgis/win_web/download/windows/pg${REL_PGVER}/buildbot
cd ${POSTGIS_SRC}
