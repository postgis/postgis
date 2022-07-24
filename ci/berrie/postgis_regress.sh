#!/usr/bin/env bash
set -e
#bessie is a 32-bit Rasberry Pi managed by Bruce Rindahl
## BRANCH is passed in via jenkins which is set via gitea web hook
#export BRANCH=618a67b1d6fc223dd5a4c0b02c824939f21dbd65
## label is set by jenkins
#export label=${label}
SCRIPT=$(readlink -f "$0")
export CUR_DIR=$(dirname "$SCRIPT")
echo $CUR_DIR
export CONFIG_FILE="$CUR_DIR/configs.sh"
. $CONFIG_FILE
echo $PATH
echo $WORKSPACE

sh autogen.sh
./configure --with-pgconfig=${PGPATH}/bin/pg_config \
  --with-geosconfig=${GEOS_PATH}/bin/geos-config \
	--without-protobuf --with-library-minor-version \
	--enable-lto
#make clean
make
export err_status=0
make check RUNTESTFLAGS="-v"
make install
make check RUNTESTFLAGS="-v --extension"
err_status=$?

make garden
err_status=$?

if [ -d $PGDATA/postmaster.pid ] ; then
	$PGCTL stop -D $PGDATA -s -m fast
fi
exit $err_status
