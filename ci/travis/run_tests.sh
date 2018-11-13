#!/usr/bin/env bash
set -e

WARNINGS="-Werror -Wall -Wextra -Wformat -Werror=format-security"
WARNINGS_DISABLED="-Wno-unused-parameter -Wno-implicit-fallthrough -Wno-unknown-warning-option -Wno-cast-function-type"

# Standard flags, as we might build PostGIS for production
CFLAGS_STD="-g -O2 -mtune=generic -fno-omit-frame-pointer ${WARNINGS} ${WARNINGS_DISABLED}"
LDFLAGS_STD="-Wl,-Bsymbolic-functions -Wl,-z,relro"

export CUNIT_WITH_VALGRIND=YES
export CUNIT_VALGRIND_FLAGS="--leak-check=full --error-exitcode=1"

/usr/local/pgsql/bin/pg_ctl -c -l /tmp/logfile start
./autogen.sh

# Standard build
./configure CFLAGS="${CFLAGS_STD}" LDFLAGS="${LDFLAGS_STD}"
bash ./ci/travis/logbt -- make -j check RUNTESTFLAGS=--verbose

# Check that compilation works at nonzero POSTGIS_DEBUG_LEVEL
./configure --enable-debug # sets PARANOIA_LEVEL
sed -i 's/POSTGIS_DEBUG_LEVEL [0-9]$/POSTGIS_DEBUG_LEVEL 4/' postgis_config.h
make -j
