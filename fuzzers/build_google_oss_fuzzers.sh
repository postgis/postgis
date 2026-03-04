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
JSON_C_LIBS=$(pkg-config --libs json-c)
GEOS_LIBS=$(geos-config --clibs)
PROJ_XML2_LIBS=$(pkg-config --libs proj libxml-2.0)
POSTGIS_FUZZER_LIBS="$JSON_C_LIBS $GEOS_LIBS $PROJ_XML2_LIBS"
POSTGIS_PACKAGE_RUNTIME_LIBS="${POSTGIS_PACKAGE_RUNTIME_LIBS:-1}"

build_fuzzer()
{
    fuzzerName=$1
    sourceFilename=$2
    shift
    shift
    echo "Building fuzzer $fuzzerName"
    $CXX $CXXFLAGS -std=c++11 -I$SRC_DIR/liblwgeom \
        $sourceFilename $* -o $OUT/$fuzzerName \
        -lFuzzingEngine -lstdc++ $SRC_DIR/liblwgeom/.libs/liblwgeom.a $POSTGIS_FUZZER_LIBS
}

package_runtime_libs()
{
    mkdir -p "$OUT/lib"

    for fuzzer in "$OUT"/*_fuzzer; do
        [ -f "$fuzzer" ] || continue

        if command -v patchelf >/dev/null 2>&1; then
            patchelf --set-rpath '$ORIGIN/lib' "$fuzzer"
        fi

        while read -r lib; do
            [ -f "$lib" ] || continue

            base="$(basename "$lib")"
            case "$base" in
                libc.so.*|libm.so.*|libdl.so.*|libpthread.so.*|librt.so.*|libgcc_s.so.*|ld-linux*.so.*|linux-vdso.so.*|libresolv.so.*)
                    continue
                    ;;
            esac

            cp -n "$lib" "$OUT/lib/" || true
        done < <(ldd "$fuzzer" | awk '/=> \// {print $3}')
    done

    if command -v patchelf >/dev/null 2>&1; then
        patchelf --set-rpath '$ORIGIN' "$OUT"/lib/*.so* 2>/dev/null || true
    fi
}

fuzzerFiles=$(dirname $0)/*.cpp
for F in $fuzzerFiles; do
    fuzzerName=$(basename $F .cpp)
    build_fuzzer $fuzzerName $F
done

cp $(dirname $0)/*.dict $(dirname $0)/*.options $(dirname $0)/*.zip $OUT/

if [ "$POSTGIS_PACKAGE_RUNTIME_LIBS" != "0" ]; then
    package_runtime_libs
fi
