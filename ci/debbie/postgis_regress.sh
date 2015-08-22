#!/bin/bash
set -e
## begin variables passed in by jenkins

# export PG_VER=9.2
# export PGPORT=8442
# export OS_BUILD=64
# export POSTGIS_MAJOR_VERSION=2
# export POSTGIS_MINOR_VERSION=2
# export POSTGIS_MICRO_VERSION=0dev
# export JENKINS_HOME=/var/lib/jenkins/workspace
# export GEOS_VER=3.6.0dev
# export GDAL_VER=2.0
# export MAKE_GARDEN=1
# export MAKE_EXTENSION=0

## end variables passed in by jenkins

PROJECTS=${JENKINS_HOME}/workspace
PGPATH=${PROJECTS}/pg/rel/pg${PG_VER}w${OS_BUILD}

export PGUSER=postgres
export PATH="${PGPATH}/bin:$PATH"
export LD_LIBRARY_PATH="${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}/lib:${PROJECTS}/geos/rel-${GEOS_VER}w${OS_BUILD}/lib:${PGPATH}/lib"

rm -rf ${WORKSPACE}/tmp/${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}
rm -rf ${WORKSPACE}/tmp/${POSTGIS_MAJOR_VERSION}_${POSTGIS_MINOR_VERSION}_pg${PG_VER}w${OS_BUILD}
#mkdir ${WORKSPACE}/tmp/
export PGIS_REG_TMPDIR=${WORKSPACE}/tmp/${POSTGIS_MAJOR_VERSION}_${POSTGIS_MINOR_VERSION}_pg${PG_VER}w${OS_BUILD}

echo $PGPORT
echo ${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}

## doesn't work for some reason - just hard-code to branches for now
# if [[ $POSTGIS_MICRO_VERSION  == *SVN* || $POSTGIS_MICRO_VERSION  == *dev*  ]]  
# then
# 	export POSTGIS_SRC=${PROJECTS}/postgis/branches/${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}
# else
# 	#tagged version -- official release
# 	export POSTGIS_SRC=${PROJECTS}/postgis/tags/${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}.${POSTGIS_MICRO_VERSION}
# fi;

export POSTGIS_SRC=${PROJECTS}/postgis/branches/${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}

cd ${POSTGIS_SRC}

if [ -e ./GNUMakefile ]; then
 make distclean
fi

./autogen.sh

./configure \
    --with-pgconfig=${PROJECTS}/pg/rel/pg${PG_VER}w${OS_BUILD}/bin/pg_config \
    --with-geosconfig=${PROJECTS}/geos/rel-${GEOS_VER}w${OS_BUILD}/bin/geos-config \
    --with-gdalconfig=${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}/bin/gdal-config --without-interrupt-tests
make clean
## install so we can later test extension upgrade
make 

if [ "$?" != "0" ]; then
  exit $?
fi

make check RUNTESTFLAGS=-v

if [ "$MAKE_EXTENSION" = "1" ]; then
 echo "Running extension testing"
 make install
 make check RUNTESTFLAGS=--extension
 if [ "$?" != "0" ]; then
  exit $?
 fi
fi

if [ "$MAKE_GARDEN" = "1" ]; then
 echo "Running garden test"
 make garden
fi
