#!/usr/bin/env bash

# Exit on first error
set -e

service postgresql start $PGVER
export PGPORT=`grep ^port /etc/postgresql/$PGVER/main/postgresql.conf | awk '{print $3}'`
export PATH=/usr/lib/postgresql/$PGVER/bin:$PATH
psql --version

#-----------------------------------------------
# Out of tree build for given PostgreSQL version
#-----------------------------------------------

SRCDIR=$PWD
BUILDDIR=/tmp/postgis-build/${PGVER}
mkdir -p "${BUILDDIR}"
cd "${BUILDDIR}"
${SRCDIR}/configure \
  CFLAGS="-O2 -Wall -fno-omit-frame-pointer -Werror" \
  --enable-lto
make -j

# we should maybe wait for postgresql service to startup here...
psql -c "select version()" template1

#-----------------------------------------------
# Pre-install tests
#-----------------------------------------------

RUNTESTFLAGS=-v make check
RUNTESTFLAGS=-v make check-double-upgrade

RUNTESTFLAGS=-v make -C regress check-locked-upgrade
RUNTESTFLAGS=-v make -C topology/test check-locked-upgrade

# This is blocked by https://trac.osgeo.org/postgis/ticket/5516
#RUNTESTFLAGS=-v make -C raster/test/regress check-locked-upgrade

#-----------------------------------------------
# Install
#-----------------------------------------------

make install

#-----------------------------------------------
# Post-install tests
#-----------------------------------------------

RUNTESTFLAGS=-v make installcheck

#-----------------------------------------------
# Upgrade tests
#-----------------------------------------------

CURRENTVERSION=`grep '^POSTGIS_' ${SRCDIR}/Version.config | cut -d= -f2 | paste -sd '.'`
mkfifo check.fifo
tee check.log < check.fifo &
${SRCDIR}/utils/check_all_upgrades.sh -s ${CURRENTVERSION}! > check.fifo
wait # for tee process to flush its buffers
echo "-- Summary of upgrade tests --"
egrep '(PASS|FAIL|SKIP|INFO)' check.log
