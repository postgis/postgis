#!/usr/bin/env bash
set -e

WARNINGS="-Werror -Wall -Wextra -Wformat -Werror=format-security"
WARNINGS_DISABLED="-Wno-unused-parameter -Wno-implicit-fallthrough -Wno-cast-function-type"

CFLAGS_STD="-g -O2 -mtune=generic -fno-omit-frame-pointer ${WARNINGS} ${WARNINGS_DISABLED}"
LDFLAGS_STD="-Wl,-Bsymbolic-functions -Wl,-z,relro"

/usr/local/pgsql/bin/pg_ctl -c -l /tmp/logfile -o '-F' start
./autogen.sh
./configure CFLAGS="${CFLAGS_STD}" LDFLAGS="${LDFLAGS_STD}"
make -j
make check RUNTESTFLAGS="--verbose --after-create-db-script ${PWD}/regress/hooks/standard-conforming-strings-off.sql"
