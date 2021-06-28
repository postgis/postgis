#!/bin/bash
# Reallie is a 64-bit Debian 11, residing in same host as debbie
## BRANCH is passed in via jenkins which is set via gitea web hook
#export BRANCH=618a67b1d6fc223dd5a4c0b02c824939f21dbd65
## label is set by jenkins
#export label=${label}

## TODO: Determine what will trigger reallie to move

export WORKSPACE=/home/jenkins/workspace

cd ${WORKSPACE}/PostGIS_Worker_Run/label/${label}/$BRANCH
export OS_BUILD=64
export PG_VER=14
export PGPATH=${WORKSPACE}/pg/label/${label}/rel/pg${PG_VER}w${OS_BUILD}

export PATH=${PGPATH}/bin:${PGPATH}/lib:${PATH}
export PGPORT=55432
export PGDATA=$PGPATH/data_${PGPORT}
export PGHOST=localhost

sh autogen.sh
./configure --with-pgconfig=${PGPATH}/bin/pg_config
#make clean
make
export err_status=0
make install
cd regress/real
make check
err_status=$?


if [ -d $PGDATA/postmaster.pid ] ; then
	$PGCTL stop -D $PGDATA -s -m fast
fi
exit $err_status
