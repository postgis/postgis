#!/usr/bin/env bash

set -e

if [ "$CC" == "" ]; then
    echo "CC env var not defined"
    exit 1
fi

if [ "$CXX" == "" ]; then
    echo "CXX env var not defined"
    exit 1
fi

if [ "$OUT" == "" ]; then
    echo "OUT env var not defined"
    exit 1
fi

FUZZERS_DIR=$(cd "$(dirname "$0")" && pwd)
SRC_DIR=$(dirname "$FUZZERS_DIR")
mkdir -p "$OUT"

if [ "$SRC" == "" ]; then
    export SRC="$SRC_DIR"
fi

export CXXFLAGS="$CXXFLAGS -std=c++11"
POSTGIS_OSS_FUZZ_CONFIGURE_FLAGS=(
    --enable-static
    --without-raster
    --without-protobuf
    --enable-debug
)

cd "$SRC_DIR"
./autogen.sh
./configure CC="$CC" CXX="$CXX" "${POSTGIS_OSS_FUZZ_CONFIGURE_FLAGS[@]}"
cd liblwgeom
make clean -s
make -j"$(nproc)" -s
cd ..

bash ./fuzzers/build_google_oss_fuzzers.sh
bash ./fuzzers/build_seed_corpus.sh
