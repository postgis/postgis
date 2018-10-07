#!/usr/bin/env bash
set -e

WARNINGS="-Werror -Wall -Wextra -Wformat -Werror=format-security"
WARNINGS_DISABLED="-Wno-unused-parameter -Wno-implicit-fallthrough -Wno-unknown-warning-option -Wno-cast-function-type"

CFLAGS_STD="-g -O2 -mtune=generic -fno-omit-frame-pointer ${WARNINGS} ${WARNINGS_DISABLED}"
LDFLAGS_STD="-Wl,-Bsymbolic-functions -Wl,-z,relro"

CFLAGS_COV="-g -O0 --coverage"
LDFLAGS_COV="--coverage"

/usr/local/pgsql/bin/pg_ctl -c -l /tmp/logfile start
./autogen.sh

./configure CFLAGS="${CFLAGS_STD}" LDFLAGS="${LDFLAGS_STD}"
bash ./ci/travis/logbt -- make -j check RUNTESTFLAGS=--verbose

./configure CFLAGS="${CFLAGS_COV}" LDFLAGS="${LDFLAGS_COV}"
make -j check
