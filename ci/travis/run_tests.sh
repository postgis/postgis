#!/usr/bin/env bash
set -e

WARNINGS="-Werror -Wall -Wextra -Wformat -Werror=format-security"
WARNINGS_DISABLED="-Wno-unused-parameter -Wno-implicit-fallthrough -Wno-unknown-warning-option -Wno-cast-function-type"

CFLAGS_STD="-g -O2 -mtune=generic -fno-omit-frame-pointer ${WARNINGS} ${WARNINGS_DISABLED}"
LDFLAGS_STD="-Wl,-Bsymbolic-functions -Wl,-z,relro"

CFLAGS_COV="-g -O0 --coverage"
LDFLAGS_COV="--coverage"

/usr/local/pgsql/bin/pg_ctl -l /tmp/logfile start
./autogen.sh

CFLAGS="${CFLAGS_STD}" LDFLAGS="${LDFLAGS_COV}" ./configure
make -j
RUNTESTFLAGS="-v" make check

CFLAGS="${CFLAGS_COV}" LDFLAGS="${LDFLAGS_COV}" ./configure
make -j
make check
