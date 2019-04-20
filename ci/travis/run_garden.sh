#!/usr/bin/env bash
set -e

# Debug-friendly flags
CFLAGS_STD="-g -O0 -mtune=generic -fno-omit-frame-pointer"
LDFLAGS_STD=""

/usr/local/pgsql/bin/pg_ctl -c -l /tmp/logfile start
./autogen.sh
./configure CFLAGS="${CFLAGS_STD}" LDFLAGS="${LDFLAGS_STD}"
make -j
bash ./ci/travis/logbt -- make garden
