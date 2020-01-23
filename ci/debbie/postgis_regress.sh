#!/usr/bin/env bash
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
# export SFCGAL_VER=1.3
export MAKE_GARDEN=0
export MAKE_EXTENSION=1
export DUMP_RESTORE=0
export MAKE_LOGBT=0
export NO_SFCGAL=0

## end variables passed in by jenkins

PROJECTS=${JENKINS_HOME}/workspace
PGPATH=${PROJECTS}/pg/rel/pg${PG_VER}w${OS_BUILD}

export PGUSER=postgres
export PATH="${PGPATH}/bin:${PROJECTS}/sfcgal/rel-${SFCGAL_VER}w${OS_BUILD}/bin:$PATH"
export LD_LIBRARY_PATH="${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}/lib:${PROJECTS}/geos/rel-${GEOS_VER}w${OS_BUILD}/lib:${PROJECTS}/sfcgal/rel-${SFCGAL_VER}w${OS_BUILD}/lib:${PGPATH}/lib"

rm -rf ${WORKSPACE}/tmp/${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}
rm -rf ${WORKSPACE}/tmp/${POSTGIS_MAJOR_VERSION}_${POSTGIS_MINOR_VERSION}_pg${PG_VER}w${OS_BUILD}
#mkdir ${WORKSPACE}/tmp/
export PGIS_REG_TMPDIR=${WORKSPACE}/tmp/${POSTGIS_MAJOR_VERSION}_${POSTGIS_MINOR_VERSION}_pg${PG_VER}w${OS_BUILD}

#adding this sleep so postgres instance has some grace period for starting
#otherwise the attempt to drop the database, sometimes happens when pg is in middle of start
for i in {0..60}; do psql -c 'select;' && break; sleep 0.5; done

export POSTGIS_REGRESS_DB="postgis_reg" # FIXME: tweak to avoid clashes
psql -c "DROP DATABASE IF EXISTS $POSTGIS_REGRESS_DB"

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
#--with-sfcgal=${PROJECTS}/sfcgal/rel-sfcgal-${SFCGAL_VER}w${OS_BUILD}/bin/sfcgal-config
./configure \
    --with-pgconfig=${PROJECTS}/pg/rel/pg${PG_VER}w${OS_BUILD}/bin/pg_config \
    --with-geosconfig=${PROJECTS}/geos/rel-${GEOS_VER}w${OS_BUILD}/bin/geos-config \
    --with-gdalconfig=${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}/bin/gdal-config \
    --without-interrupt-tests \
    --prefix=${PROJECTS}/pg/rel/pg${PG_VER}w${OS_BUILD}
make clean
make

if [ "$?" != "0" ]; then
  exit $?
fi

if [ "$MAKE_LOGBT" = "1" ]; then
 echo "Running logbt testing"
 bash ./ci/debbie/logbt -- make -j check RUNTESTFLAGS=--verbose
 if [ "$?" != "0" ]; then
  exit $?
 fi
else
  export RUNTESTFLAGS=-v
fi


make check

## install so we can test upgrades/dump-restores etc.
make install

if [ "$MAKE_EXTENSION" = "1" ]; then
 echo "Running extension testing"
 make check RUNTESTFLAGS="$RUNTESTFLAGS --extension"
 if [ "$?" != "0" ]; then
  exit $?
 fi
fi



if [ "$DUMP_RESTORE" = "1" ]; then
 echo "Dum restore test"
 make check RUNTESTFLAGS="$RUNTESTFLAGS --dumprestore"
 if [ "$?" != "0" ]; then
  exit $?
 fi
fi

if [ "$MAKE_GARDEN" = "1" ]; then
 echo "Running garden test"
 make garden
 if [ "$?" != "0" ]; then
  exit $?
 fi
fi

# Test all available upgrades
# TODO: protect via some variable ?
utils/check_all_upgrades.sh \
        `grep '^POSTGIS_' Version.config | cut -d= -f2 | paste -sd '.'`
if [ "$?" != "0" ]; then
  exit $?
fi
