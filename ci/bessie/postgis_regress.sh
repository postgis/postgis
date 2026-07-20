#!/usr/bin/env bash

set -e

#BRANCH is passed in via jenkins which is set via webhook hook
export PATH=${PATH}:/usr/local:/usr/local/lib:/usr/local/bin

PGIS_TMP_ROOT=$(mktemp -d "${TMPDIR:-/tmp}/postgis-bessie.XXXXXX")
export PGDATA="${PGIS_TMP_ROOT}/data"
export PGHOST="${PGIS_TMP_ROOT}"
export PGUSER=postgres
export PGIS_REG_TMPDIR="${PGIS_TMP_ROOT}/regress"
PGLOG="${PGIS_TMP_ROOT}/postgresql.log"

cleanup() {
  status=$?
  trap - EXIT
  if test -s "${PGDATA}/postmaster.pid"; then
    pg_ctl -D "${PGDATA}" -m immediate stop >/dev/null 2>&1 || true
  fi
  if test "${status}" -ne 0 && test -s "${PGLOG}"; then
    echo "PostgreSQL log follows:" >&2
    tail -n 100 "${PGLOG}" >&2
  fi
  rm -rf -- "${PGIS_TMP_ROOT}"
  exit "${status}"
}

trap cleanup EXIT
trap 'exit 1' HUP INT TERM

sh autogen.sh
./configure --with-projdir=/usr/local --with-libiconv=/usr/local --without-interrupt-tests --with-library-minor-version --without-raster
#make distclean
make

initdb -N --no-data-checksums -U "${PGUSER}" -D "${PGDATA}"
printf '%s\n' \
  'fsync = off' \
  'full_page_writes = off' \
  'synchronous_commit = off' \
  >> "${PGDATA}/postgresql.conf"
pg_ctl -D "${PGDATA}" -l "${PGLOG}" \
  -o "-F -c listen_addresses='' -c unix_socket_directories='${PGHOST}'" start
mkdir -p "${PGIS_REG_TMPDIR}"

make check RUNTESTFLAGS="-v"
sudo make install
make check RUNTESTFLAGS="-v --extension"
