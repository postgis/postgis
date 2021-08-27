#!/usr/bin/env bash
#bessie is a 32-bit Rasberry Pi managed by Bruce Rindahl
## BRANCH is passed in via jenkins which is set via gitea web hook
#export BRANCH=618a67b1d6fc223dd5a4c0b02c824939f21dbd65
## label is set by jenkins
#export label=${label}

export WORKSPACE=/home/jenkins/workspace

cd ${WORKSPACE}/PostGIS_Worker_Run/label/${label}/$BRANCH
export OS_BUILD=32
export PG_VER=12
export PGPATH=${WORKSPACE}/pg/label/${label}/rel/pg${PG_VER}w${OS_BUILD}

export PATH=${PGPATH}/bin:${PGPATH}/lib:${PATH}
export PGPORT=55432
export PGDATA=$PGPATH/data_${PGPORT}
export PGHOST=localhost

sh autogen.sh
./configure --with-pgconfig=${PGPATH}/bin/pg_config --without-protobuf
#make clean
make
export err_status=0
make check RUNTESTFLAGS="-v"
make install
make check RUNTESTFLAGS="-v --extension"
err_status=$?

if [ -d $PGDATA/postmaster.pid ] ; then
	$PGCTL stop -D $PGDATA -s -m fast
fi
exit $err_status
