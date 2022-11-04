export WORKSPACE=/home/jenkins/workspace
export GEOS_VER=3.11.1

export OS_BUILD=32
export PG_VER=15
export GEOS_PATH=${WORKSPACE}/geos/rel-${GEOS_VER}
#export GEOS_PATH=~/geos/rel-${GEOS_VER}
export PGPATH=${WORKSPACE}/pg/label/${label}/rel/pg${PG_VER}w${OS_BUILD}
export PATH=${PGPATH}/bin:${PGPATH}/lib:${GEOS_PATH}/bin:${GEOS_PATH}/lib:${PATH}
export PKG_CONFIG_PATH="${GEOS_PATH}/lib/pkgconfig"
export PGPORT=55433
export PGDATA=$PGPATH/data_${PGPORT}
export PGHOST=localhost
export LD_LIBRARY_PATH="${GEOS_PATH}/lib:${PGPATH}/lib"
