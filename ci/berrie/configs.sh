export WORKSPACE=/home/jenkins/workspace
export GEOS_VER=3.9.3

export OS_BUILD=32
export PG_VER=12
#export GEOS_PATH=${WORKSPACE}/geos/rel-${GEOS_VER}
#export GEOS_PATH=~/geos/rel-${GEOS_VER}
export PGPATH=${WORKSPACE}/pg/label/${label}/rel/pg${PG_VER}w${OS_BUILD}
#export PATH=${PGPATH}/bin:${PGPATH}/lib:${GEOS_PATH}/bin:${GEOS_PATH}/lib:${PATH}
export PATH=${PGPATH}/bin:${PGPATH}/lib:${PATH}
export PKG_CONFIG_PATH="${GEOS_PATH}/lib/pkgconfig"
export PGPORT=55432
export PGDATA=$PGPATH/data_${PGPORT}
export PGHOST=localhost
#export LD_LIBRARY_PATH="${GEOS_PATH}/lib:${PGPATH}/lib"
export LD_LIBRARY_PATH="${PGPATH}/lib"
