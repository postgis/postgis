#!/usr/bin/env bash

set -e

if [ "$OUT" == "" ]; then
    echo "OUT env var not defined"
    exit 1
fi

if [ "$CXX" == "" ]; then
    echo "CXX env var not defined"
    exit 1
fi

if ! command -v python3 >/dev/null 2>&1; then
    echo "python3 not found; skipping seed corpus archive generation"
    exit 0
fi

POSTGIS_SOURCE_DIR="${POSTGIS_SOURCE_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
POSTGIS_BUILD_DIR="${POSTGIS_BUILD_DIR:-$POSTGIS_SOURCE_DIR}"
FUZZERS_DIR="$POSTGIS_SOURCE_DIR/fuzzers"
JSON_C_LIBS=$(pkg-config --libs json-c)
GEOS_LIBS=$(geos-config --clibs)
PROJ_LIBS=$(pkg-config --libs proj)
if command -v xml2-config >/dev/null 2>&1; then
    XML2_LIBS=$(xml2-config --libs)
else
    XML2_LIBS=$(pkg-config --libs libxml-2.0)
fi

seed_builder="$OUT/build_gserialized_seed_corpus"
mkdir -p "$OUT"

"$CXX" $CXXFLAGS -std=c++11 \
    -I"$POSTGIS_SOURCE_DIR/liblwgeom" -I"$POSTGIS_BUILD_DIR/liblwgeom" $CPPFLAGS \
    "$FUZZERS_DIR/build_gserialized_seed_corpus.cpp" -o "$seed_builder" \
    $LDFLAGS "$POSTGIS_BUILD_DIR/liblwgeom/.libs/liblwgeom.a" \
    $JSON_C_LIBS $GEOS_LIBS $PROJ_LIBS $XML2_LIBS

"$seed_builder" "$OUT" "$FUZZERS_DIR/gserialized_seed_corpus.in"

for corpus_dir in \
    "$OUT/gserialized_from_bytea_fuzzer_seed_corpus" \
    "$OUT/gserialized_from_lwgeom_fuzzer_seed_corpus"; do
    corpus_zip="$OUT/$(basename "$corpus_dir").zip"
    rm -f "$corpus_zip"
    python3 -m zipfile -c "$corpus_zip" "$corpus_dir"/*
done
