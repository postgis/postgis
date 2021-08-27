#!/bin/bash

# Exit on first error
set -e

su - postgres -c "export PGDATA=/var/lib/postgresql/data && pg_ctl -o '-F' -l /var/lib/postgresql/data/pg.log start"
#export PGPORT=`grep ^port /var/lib/postgresql/postgresql.conf | awk '{print $3}'`
export PGPORT=5432
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
