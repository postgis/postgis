#!/usr/bin/env bash
set -e

# Enable undefined behaviour sanitizer using traps
CFLAGS_STD="-g -O2 -mtune=generic -fno-omit-frame-pointer -fsanitize=undefined -fsanitize-trap=undefined"
LDFLAGS_STD="-Wl,-Bsymbolic-functions -Wl,-z,relro"

/usr/local/pgsql/bin/pg_ctl -c -l /tmp/logfile start
./autogen.sh

# Build with Clang and usan flags
./configure CC=clang CFLAGS="${CFLAGS_STD}" LDFLAGS="${LDFLAGS_STD}"
bash ./ci/travis/logbt -- make -j check RUNTESTFLAGS=--verbose
