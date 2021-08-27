#!/usr/bin/env bash
set -e

# Enable undefined behaviour sanitizer using traps to
CFLAGS_STD="-g3 -O0 -mtune=generic -fno-omit-frame-pointer -fsanitize=undefined -fsanitize-undefined-trap-on-error"
LDFLAGS_STD="-Wl,-Bsymbolic-functions -Wl,-z,relro"

/usr/local/pgsql/bin/pg_ctl -c -l /tmp/logfile -o '-F' start
./autogen.sh

# Build with GCC and usan flags
./configure CC=gcc CFLAGS="${CFLAGS_STD}" LDFLAGS="${LDFLAGS_STD}"
bash ./ci/travis/logbt -- make -j
bash ./ci/travis/logbt -- make check RUNTESTFLAGS=--verbose
