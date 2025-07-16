export WORKSPACE=/home/jenkins/workspace
export GEOS_VER=3.14.0
export SFCGAL_VER=master

export OS_BUILD=64
export PG_VER=17
export GEOS_PATH=${WORKSPACE}/geos/rel-${GEOS_VER}
export SFCGAL_PATH=${WORKSPACE}/sfcgal/rel-sfcgal-${SFCGAL_VER}
#export GEOS_PATH=~/geos/rel-${GEOS_VER}
export PGPATH=${WORKSPACE}/pg/label/${label}/rel/pg${PG_VER}w${OS_BUILD}
export PATH=${PGPATH}/bin:${PGPATH}/lib:${GEOS_PATH}/bin:${GEOS_PATH}/lib:${SFCGAL_PATH}/bin:${SFCGAL_PATH}/lib:${PATH}

export PKG_CONFIG_PATH="${GEOS_PATH}/lib/pkgconfig:${SFCGAL_PATH}/lib/pkgconfig"
export PGPORT=55434
export PGDATA=$PGPATH/data_${PGPORT}
export PGHOST=localhost
export LD_LIBRARY_PATH="${GEOS_PATH}/lib:${PGPATH}/lib"
