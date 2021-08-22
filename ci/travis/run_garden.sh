#!/usr/bin/env bash
set -e

# Debug-friendly flags
CFLAGS_STD="-g -O0 -mtune=generic -fno-omit-frame-pointer --coverage"
LDFLAGS_STD="--coverage"

/usr/local/pgsql/bin/pg_ctl -c -l /tmp/logfile -o '-F' start
./autogen.sh
./configure CFLAGS="${CFLAGS_STD}" LDFLAGS="${LDFLAGS_STD}" --enable-debug
make -j
bash ./ci/travis/logbt -- make garden
(curl -S -f https://codecov.io/bash -o .github/codecov.bash && bash .github/codecov.bash) || echo "Coverage report failed"
