#!/usr/bin/env bash
set -e

# Enable undefined behaviour sanitizer using traps
CFLAGS_USAN="-g3 -O0 -mtune=generic -fno-omit-frame-pointer -fsanitize=undefined -fsanitize-undefined-trap-on-error"
LDFLAGS_STD="-Wl,-Bsymbolic-functions -Wl,-z,relro"

/usr/local/pgsql/bin/pg_ctl -c -o '-F' -l /tmp/logfile start


# Build with Clang and usan flags
./autogen.sh
./configure CC=clang CFLAGS="${CFLAGS_USAN}" LDFLAGS="${LDFLAGS_STD}"
bash ./ci/travis/logbt -- make -j
bash ./ci/travis/logbt -- make check RUNTESTFLAGS=--verbose
