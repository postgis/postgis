#Berrie is a 32-bit Rasberry Pi managed by Bruce Rindahl
#BRANCH is passed in via jenkins which is set via gitea web hook
#export BRANCH=618a67b1d6fc223dd5a4c0b02c824939f21dbd65
export WORKSPACE=/home/jenkins/workspace

cd ${WORKSPACE}/PostGIS_Worker_Run/label/berrie/$BRANCH
export OS_BUILD=32
export PG_VER=13
export PGPATH=${WORKSPACE}/pg/label/berrie/rel/pg${PG_VER}w${OS_BUILD}

export PATH=${PATH}:${PGPATH}/bin:${PGPATH}/lib
export PGPORT=55532
export PGDATA=$PGPATH/data_${PGPORT}
export PGHOST=localhost

sh autogen.sh
./configure --with-pgconfig=${PGPATH}/bin/pg_config
#make clean
#make

#make check RUNTESTFLAGS="-v"
make install
make check RUNTESTFLAGS="-v --extension"

if [ -d $PGDATA/postmaster.pid ] ; then
	$PGCTL stop -D $PGDATA -s -m fast
fi