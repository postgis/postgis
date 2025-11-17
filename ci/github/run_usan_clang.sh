#!/usr/bin/env bash
set -e
CLANG_FULL_VER=`clang --version`
echo $CLANG_FULL_VER
echo `cat /etc/os-release`
export CLANG_VER=`clang --version | perl -lne 'print $1 if /version (\d+)/'`
echo "Clang major version is: $CLANG_VER"
# Enable undefined behaviour sanitizer using traps
CFLAGS_USAN="-g3 -O0 -mtune=generic -fno-omit-frame-pointer -fsanitize=undefined -fsanitize-undefined-trap-on-error -fno-sanitize-recover"
LDFLAGS_STD="-Wl,-Bsymbolic-functions -Wl,-z,relro"

# Sanitizer options to continue avoid stopping the runs on leaks (expected on postgres binaries)
export ASAN_OPTIONS=halt_on_error=false,leak_check_at_exit=false,exitcode=0
export MSAN_OPTIONS=halt_on_error=false,leak_check_at_exit=false,exitcode=0

#Run postgres preloading sanitizer libs
LD_PRELOAD=/usr/lib/clang/${CLANG_VER}/lib/linux/libclang_rt.asan-x86_64.so

/usr/local/pgsql/bin/pg_ctl -c -o '-F' -l /tmp/logfile start


# Build with Clang and usan flags
./autogen.sh
./configure CC=clang CFLAGS="${CFLAGS_USAN}" LDFLAGS="${LDFLAGS_STD}"
bash ./ci/github/logbt -- make -j $(nproc)
bash ./ci/github/logbt -- make check RUNTESTFLAGS=--verbose
