#!/usr/bin/env bash
set -e

WARNINGS="-Werror -Wall -Wextra -Wformat -Werror=format-security"
WARNINGS_DISABLED="-Wno-unused-parameter -Wno-implicit-fallthrough -Wno-cast-function-type"

# Build with coverage
CFLAGS="-g -O0 --coverage -mtune=generic -fno-omit-frame-pointer ${WARNINGS} ${WARNINGS_DISABLED}"
LDFLAGS="--coverage"

/usr/local/pgsql/bin/pg_ctl -c -l /tmp/logfile -o '-F' start
./autogen.sh

# Standard build
./configure --without-wagyu CFLAGS="${CFLAGS}" CXXFLAGS="${CFLAGS}" LDFLAGS="${LDFLAGS}"
bash ./ci/travis/logbt -- make -j
bash ./ci/travis/logbt -- make check RUNTESTFLAGS=--verbose
(curl -S -f https://codecov.io/bash -o .github/codecov.bash && bash .github/codecov.bash) || echo "Coverage report failed"
