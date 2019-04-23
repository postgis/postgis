#!/usr/bin/env bash
set -e

# Enable undefined behaviour sanitizer using traps
CFLAGS_USAN="-g3 -O0 -mtune=generic -fno-omit-frame-pointer -fsanitize=undefined,implicit-conversion -fsanitize-undefined-trap-on-error -fno-sanitize-recover=implicit-conversion"
LDFLAGS_STD="-Wl,-Bsymbolic-functions -Wl,-z,relro"

/usr/local/pgsql/bin/pg_ctl -c -l /tmp/logfile start
./autogen.sh

# Build with Clang and usan flags
./configure CC=clang CFLAGS="${CFLAGS_USAN}" LDFLAGS="${LDFLAGS_STD}"
bash ./ci/travis/logbt -- make -j check RUNTESTFLAGS=--verbose
