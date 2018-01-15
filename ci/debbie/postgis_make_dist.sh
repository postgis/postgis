#!/bin/bash
set -e
## begin variables passed in by jenkins
# export reference=$reference

export PG_VER=9.6
export OS_BUILD=64
export PROJECTS=/var/lib/jenkins/workspace
export GEOS_VER=3.6
export GDAL_VER=2.2
export WEB_DIR=/var/www/postgis_stuff
export PATH="${PGPATH}/bin:$PATH"
export LD_LIBRARY_PATH="${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}/lib:${PROJECTS}/geos/rel-${GEOS_VER}w${OS_BUILD}/lib:${PGPATH}/lib"

echo "Reference Branch: $reference"
POSTGIS_MAJOR_VERSION=`grep ^POSTGIS_MAJOR_VERSION Version.config | cut -d= -f2`
POSTGIS_MINOR_VERSION=`grep ^POSTGIS_MINOR_VERSION Version.config | cut -d= -f2`
POSTGIS_MICRO_VERSION=`grep ^POSTGIS_MICRO_VERSION Version.config | cut -d= -f2`

export CONFIGURE_ARGS="--with-pgconfig=${PROJECTS}/pg/rel/pg${PG_VER}w${OS_BUILD}/bin/pg_config --with-geosconfig=${PROJECTS}/geos/rel-${GEOS_VER}w${OS_BUILD}/bin/geos-config --with-gdalconfig=${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}/bin/gdal-config --with-sfcgal=/usr/bin/sfcgal-config --prefix=${PROJECTS}/pg/rel/pg${PG_VER}w${OS_BUILD}"

#override the checkout folder used for building tar ball
export newoutdir="postgis-${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}.${POSTGIS_MICRO_VERSION}"

#fake production build
#export newoutdir="postgis-${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}.0"

sh make_dist.sh
export package=${newoutdir}.tar.gz
echo "The package name is $package"

cp $package $WEB_DIR
sh ci/debbie/postgis_release_docs.sh
