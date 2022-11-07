#!/usr/bin/env bash
### this script is used to zip up the compiled binaries
## PostgreSQL, OS_BUILD denote the last build to be packaged
## and are passed in by the jenkins job process
###
#export OS_BUILD=64
#export PGPORT=8442

#export OS_BUILD=32

#export GCC_TYPE=
#if no override is set - use these values
#otherwise use the ones jenkins passes thru
#!/usr/bin/env bash
if  [[ "${OVERRIDE}" == '' ]] ; then
	export GEOS_VER=3.10.3
	export GDAL_VER=3.4.2
	export PROJ_VER=7.2.1
	export SFCGAL_VER=1.4.1
	export CGAL_VER=5.3
	export ICON_VER=1.16
	export ZLIB_VER=1.2.11
  export PROTOBUF_VER=3.2.0
	export PROTOBUFC_VER=1.2.1
	export JSON_VER=0.12
	export PROJSO=libproj-19.dll
	export CURL_VER=7.73
fi;

export PROTOBUF_VER=3.2.0
export PROTOBUFC_VER=1.2.1
export JSON_VER=0.12
export PCRE_VER=8.33

if  [[ "${ICON_VER}" == '' ]] ; then
  export ICON_VER=1.15
fi;

echo "ICON_VER ${ICON_VER}"

if  [[ "${CURL_VER}" == '' ]] ; then
  export CURL_VER=7.73
fi;

echo "CURL_VER ${CURL_VER}"

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
  export CGAL_VER=5.3
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

if [ -n "$SOURCE_FOLDER" ]; then
  export POSTGIS_SRC=${PROJECTS}/postgis/$SOURCE_FOLDER
	cd $POSTGIS_SRC
fi

export POSTGIS_MAJOR_VERSION=`grep ^POSTGIS_MAJOR_VERSION Version.config | cut -d= -f2`
export POSTGIS_MINOR_VERSION=`grep ^POSTGIS_MINOR_VERSION Version.config | cut -d= -f2`
export POSTGIS_MICRO_VERSION=`grep ^POSTGIS_MICRO_VERSION Version.config | cut -d= -f2`

export POSTGIS_MINOR_VER=${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}
export POSTGIS_MICRO_VER=${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}.${POSTGIS_MICRO_VERSION}

echo "Version: $POSTGIS_MICRO_VER"

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

cp ${PGPATHEDB}/bin/libpq.dll  $outdir/bin/postgisgui
#cp ${PGPATHEDB}/bin/libiconv2.dll  $outdir/bin/postgisgui
cp ${PROJECTS}/rel-libiconv-${ICON_VER}w${OS_BUILD}${GCC_TYPE}/bin/libicon*.dll $outdir/bin/postgisgui
cp ${PGPATHEDB}/bin/libintl*.dll $outdir/bin/postgisgui

cp ${PGPATHEDB}/bin/ssleay32.dll $outdir/bin/postgisgui
cp ${PGPATHEDB}/bin/libeay32.dll $outdir/bin/postgisgui
cp ${PGPATHEDB}/bin/libcrypto-1_1-x64.dll $outdir/bin/postgisgui
cp ${PGPATHEDB}/bin/libssl-1_1-x64.dll $outdir/bin/postgisgui

#pg 15 is shipping with newer ssl
cp ${PGPATHEDB}/bin/libcrypto-3-x64.dll $outdir/bin/postgisgui
cp ${PGPATHEDB}/bin/libssl-3-x64.dll $outdir/bin/postgisgui


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

echo "POSTGIS: ${POSTGIS_MICRO_VER} http://postgis.net/source" > $verfile

if [ "$POSTGIS_MAJOR_VERSION" > "1" ] ; then
  ## only copy gdal components if 2+.  1.5 doesn't have raster support
  cp -p ${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}${GCC_TYPE}/bin/*.dll $outdir/bin
  cp -rp  ${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}${GCC_TYPE}/share/gdal $outdir/gdal-data

	# needed for address standardizer
  cp -p ${PROJECTS}/pcre/rel-${PCRE_VER}w${OS_BUILD}${GCC_TYPE}/bin/libpcre-1*.dll $outdir/bin

fi;

if [ -n "$SFCGAL_VER"  ]; then
	## only copy cgal and sfcgal stuff if sfcgal is packaged
	export BOOST_VER=1.78.0
	export BOOST_VER_WU=1_78_0
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
echo "CURL VERSION: ${CURL_VER} https://github.com/protobuf-c/protobuf-c"  >> $verfile
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
cp topology/loader/* ${RELDIR}/${RELVERDIR}/bin

#cp liblwgeom/.libs/*.dll ${RELDIR}/${RELVERDIR}/bin/postgisgui

#shp2pgsql-gui now has dependency on geos (though in theory it shouldn't)
cp -p ${PROJECTS}/geos/rel-${GEOS_VER}w${OS_BUILD}${GCC_TYPE}/bin/*.dll ${RELDIR}/${RELVERDIR}/bin/postgisgui
cp spatial_ref_sys.sql ${RELDIR}/${RELVERDIR}/share/contrib/postgis-${POSTGIS_MINOR_VER}
cp topology/topology.sql ${RELDIR}/${RELVERDIR}/share/contrib/postgis-${POSTGIS_MINOR_VER}
cp topology/topology_upgrade_*.sql ${RELDIR}/${RELVERDIR}/share/contrib/postgis-${POSTGIS_MINOR_VER}
#cp topology/README* ${RELDIR}/${RELVERDIR}/share/contrib/postgis-${POSTGIS_MINOR_VER}
#cp utils/* ${RELDIR}/${RELVERDIR}/utils
#cp extras/* ${RELDIR}/${RELVERDIR}/share/contrib/postgis-${POSTGIS_MINOR_VER}/extras
# in case we ever do MAX ship all the max scripts
export POSTGIS_MINOR_MAX_VER=${POSTGIS_MINOR_VER}.UPGRADEME

#export UPGRADE_VER_FILE="extensions/upgradeable_versions.mk"
value=$(<extensions/upgradeable_versions.mk)
export value=${value//\\/}
value=${value//UPGRADEABLE_VERSIONS = /}
#echo $value
export UPGRADEABLE_VERSIONS=$value
#echo "Versions are:  $UPGRADEABLE_VERSIONS"
export WIN_RELEASED_VERSIONS="2.0.0 2.0.1 2.0.3 2.0.4 2.0.6 2.1.4 2.1.7 2.1.8 2.2.0 2.2.3 2.3.0 2.3.7 2.4.0 2.4.4"
for EXTNAME in postgis postgis_raster postgis_topology postgis_sfcgal postgis_tiger_geocoder address_standardizer; do
	cp extensions/$EXTNAME/sql/$EXTNAME--${POSTGIS_MICRO_VER}.sql ${RELDIR}/${RELVERDIR}/share/extension
	cp extensions/$EXTNAME/sql/*unpackaged*.sql ${RELDIR}/${RELVERDIR}/share/extension
	cp extensions/$EXTNAME/sql/$EXTNAME--ANY--${POSTGIS_MICRO_VER}.sql ${RELDIR}/${RELVERDIR}/share/extension
	cp extensions/$EXTNAME/sql/$EXTNAME--ANY--${POSTGIS_MICRO_VER}.sql ${RELDIR}/${RELVERDIR}/share/extension/$EXTNAME--${POSTGIS_MINOR_MAX_VER}--${POSTGIS_MICRO_VER}.sql
	cp extensions/$EXTNAME/sql/$EXTNAME--ANY--${POSTGIS_MICRO_VER}.sql ${RELDIR}/${RELVERDIR}/share/extension/$EXTNAME--${POSTGIS_MICRO_VER}next--${POSTGIS_MICRO_VER}.sql
	cp extensions/$EXTNAME/sql/$EXTNAME--ANY--${POSTGIS_MICRO_VER}.sql ${RELDIR}/${RELVERDIR}/share/extension/$EXTNAME--${POSTGIS_MICRO_VER}--${POSTGIS_MICRO_VER}next.sql

	# special cases of ANY and next
	echo "--placeholder" > ${RELDIR}/${RELVERDIR}/share/extension/$EXTNAME--ANY--${POSTGIS_MINOR_MAX_VER}.sql

	if test "$EXTNAME" = "address_standardizer"; then #repeat for address_standardizer_data_us
		cp extensions/$EXTNAME/sql/${EXTNAME}_data_us--${POSTGIS_MICRO_VER}.sql ${RELDIR}/${RELVERDIR}/share/extension
		cp extensions/postgis/sql/postgis--ANY--${POSTGIS_MICRO_VER}.sql ${RELDIR}/${RELVERDIR}/share/extension
		cp extensions/$EXTNAME/sql/${EXTNAME}_data_us--ANY--${POSTGIS_MICRO_VER}.sql ${RELDIR}/${RELVERDIR}/share/extension/${EXTNAME}_data_us--${POSTGIS_MINOR_MAX_VER}--${POSTGIS_MICRO_VER}.sql
		cp extensions/$EXTNAME/sql/${EXTNAME}_data_us--ANY--${POSTGIS_MICRO_VER}.sql ${RELDIR}/${RELVERDIR}/share/extension/${EXTNAME}_data_us--${POSTGIS_MICRO_VER}next--${POSTGIS_MICRO_VER}.sql
		echo "--placeholder" > ${RELDIR}/${RELVERDIR}/share/extension//${EXTNAME}_data_us--ANY--${POSTGIS_MINOR_MAX_VER}.sql
		echo "--placeholder" > ${RELDIR}/${RELVERDIR}/share/extension//${EXTNAME}_data_us--${POSTGIS_MICRO_VER}--${POSTGIS_MINOR_MAX_VER}.sql
		echo "--placeholder" > ${RELDIR}/${RELVERDIR}/share/extension//${EXTNAME}_data_us--${POSTGIS_MICRO_VER}next--${POSTGIS_MINOR_MAX_VER}.sql
	fi

	for OLD_VERSION in $UPGRADEABLE_VERSIONS; do \
		if [ "$OLD_VERSION" > "2.5" ] || [ "$OLD_VERSION" in $WIN_RELEASED_VERSIONS ]; then
			echo "--placeholder" > ${RELDIR}/${RELVERDIR}/share/extension/$EXTNAME--$OLD_VERSION--${POSTGIS_MINOR_MAX_VER}.sql; \
			if test "$EXTNAME" = "address_standardizer"; then
				echo "--placeholder" > ${RELDIR}/${RELVERDIR}/share/extension/${EXTNAME}_data_us--$OLD_VERSION--${POSTGIS_MINOR_MAX_VER}.sql; \
			fi
		fi
	done
done
#cp ${PGPATH}/share/extension/postgis*--2.5*${POSTGIS_MICRO_VER}.sql ${RELDIR}/${RELVERDIR}/share/extension
#cp ${PGPATH}/share/extension/postgis*--3*${POSTGIS_MICRO_VER}.sql ${RELDIR}/${RELVERDIR}/share/extension
#cp ${PGPATH}/share/extension/postgis*${POSTGIS_MICRO_VER}next.sql ${RELDIR}/${RELVERDIR}/share/extension

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
md5sum $package > ${package}.md5
#scp $package robe@www.refractions.net:${DWN}/${REL_PGVER}/buildbot/${RELVERDIR}.zip
cp $package ${PROJECTS}/postgis/win_web/download/windows/pg${REL_PGVER}/buildbot
cp ${package}.md5 ${PROJECTS}/postgis/win_web/download/windows/pg${REL_PGVER}/buildbot
cd ${POSTGIS_SRC}
