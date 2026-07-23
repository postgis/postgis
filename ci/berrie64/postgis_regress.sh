#!/usr/bin/env bash
set -e
# Berrie64 is a 64-bit Raspberry Pi Arm managed by Bruce Rindahl
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

pg_stop()
{
	if [ -n "${PGDATA:-}" ] && test -s "${PGDATA}/postmaster.pid"; then
		"${PGPATH}/bin/pg_ctl" stop -D "${PGDATA}" -s -m fast || true
	fi
}
trap pg_stop EXIT

sh autogen.sh
./configure --with-pgconfig=${PGPATH}/bin/pg_config --with-geosconfig=${GEOS_PATH}/bin/geos-config --with-library-minor-version \
    --without-interrupt-tests --prefix=${PGPATH}
#make clean
make
export err_status=0
make check RUNTESTFLAGS="-v"
make install

make check RUNTESTFLAGS="-v --extension"
err_status=$?

# Optional PROJ grids are not installed on this worker. Grid-dependent manual
# examples are marked requires-external-state, so run the remaining examples
# without the global optional-grid preflight.
make garden EXAMPLETEST_CHECK_ENVIRONMENT=no
err_status=$?

utils/check_all_upgrades.sh \
    `grep '^POSTGIS_' Version.config | cut -d= -f2 | paste -sd '.'`

err_status=$?

exit $err_status
