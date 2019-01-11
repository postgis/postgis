#!/usr/bin/env bash
set -e

# Flags for coverage build
CFLAGS_COV="-g -O0 --coverage"
LDFLAGS_COV="--coverage"

/usr/local/pgsql/bin/pg_ctl -c -l /tmp/logfile start
./autogen.sh
./configure CFLAGS="${CFLAGS_COV}" LDFLAGS="${LDFLAGS_COV}" --enable-debug
make -j check
bash .github/codecov.bash
