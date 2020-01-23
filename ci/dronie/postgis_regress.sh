#!/usr/bin/env bash

# Exit on first error
set -e

service postgresql start $PGVER
export PGPORT=`grep ^port /etc/postgresql/$PGVER/main/postgresql.conf | awk '{print $3}'`
export PATH=/usr/lib/postgresql/$PGVER/bin:$PATH
psql --version
./autogen.sh
./configure CFLAGS="-O2 -Wall -fno-omit-frame-pointer -Werror" --without-interrupt-tests
make clean
make -j
# we should maybe wait for postgresql service to startup here...
psql -c "select version()" template1
RUNTESTFLAGS=-v make check
make install
RUNTESTFLAGS=-v make installcheck
utils/check_all_upgrades.sh -s \
  `grep '^POSTGIS_' Version.config | cut -d= -f2 | paste -sd '.'`
