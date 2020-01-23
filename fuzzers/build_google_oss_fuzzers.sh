#!/usr/bin/env bash

set -e

if [ "$SRC" == "" ]; then
    echo "SRC env var not defined"
    exit 1
fi

if [ "$OUT" == "" ]; then
    echo "OUT env var not defined"
    exit 1
fi

if [ "$CXX" == "" ]; then
    echo "CXX env var not defined"
    exit 1
fi

SRC_DIR=$(dirname $0)/..

build_fuzzer()
{
    fuzzerName=$1
    sourceFilename=$2
    shift
    shift
    echo "Building fuzzer $fuzzerName"
    $CXX $CXXFLAGS -std=c++11 -I$SRC_DIR/liblwgeom \
        $sourceFilename $* -o $OUT/$fuzzerName \
        -lFuzzingEngine -lstdc++ $SRC_DIR/liblwgeom/.libs/liblwgeom.a /usr/lib/x86_64-linux-gnu/libjson-c.a
}

fuzzerFiles=$(dirname $0)/*.cpp
for F in $fuzzerFiles; do
    fuzzerName=$(basename $F .cpp)
    build_fuzzer $fuzzerName $F
done

cp $(dirname $0)/*.dict $(dirname $0)/*.options $(dirname $0)/*.zip $OUT/
