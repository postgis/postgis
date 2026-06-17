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

if [ "$CC" == "" ]; then
    echo "CC env var not defined"
    exit 1
fi

POSTGIS_SOURCE_DIR="${POSTGIS_SOURCE_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
POSTGIS_BUILD_DIR="${POSTGIS_BUILD_DIR:-$POSTGIS_SOURCE_DIR}"
FUZZERS_DIR="$POSTGIS_SOURCE_DIR/fuzzers"
JSON_C_LIBS=$(pkg-config --libs json-c)
GEOS_LIBS=$(geos-config --clibs)
PROJ_XML2_LIBS=$(pkg-config --libs proj libxml-2.0)
GDAL_CFLAGS=$(gdal-config --cflags)
GDAL_LIBS=$(gdal-config --libs)
POSTGIS_FUZZER_LIBS="$JSON_C_LIBS $GEOS_LIBS $PROJ_XML2_LIBS"
POSTGIS_PACKAGE_RUNTIME_LIBS="${POSTGIS_PACKAGE_RUNTIME_LIBS:-1}"

target_cflags()
{
    case "$1" in
        raster_deserialize_fuzzer)
            echo "-I$POSTGIS_SOURCE_DIR/raster/rt_core -I$POSTGIS_BUILD_DIR/raster/rt_core -I$POSTGIS_SOURCE_DIR/raster -I$POSTGIS_BUILD_DIR/raster -I$POSTGIS_SOURCE_DIR -I$POSTGIS_BUILD_DIR $GDAL_CFLAGS"
            ;;
    esac
}

target_libs()
{
    case "$1" in
        raster_deserialize_fuzzer)
            echo "$POSTGIS_BUILD_DIR/raster/rt_core/librtcore.a $GDAL_LIBS"
            ;;
    esac
}

build_fuzzer()
{
    fuzzerName=$1
    sourceFilename=$2
    extension=${sourceFilename##*.}
    objectFile=""

    echo "Building fuzzer $fuzzerName"

    if [ "$extension" = "c" ]; then
        objectFile=$(mktemp)
        "$CC" $CFLAGS -I"$POSTGIS_SOURCE_DIR/liblwgeom" -I"$POSTGIS_BUILD_DIR/liblwgeom" $(target_cflags "$fuzzerName") \
            -c "$sourceFilename" -o "$objectFile"
        sourceFilename=$objectFile
    fi

    "$CXX" $CXXFLAGS -std=c++11 -I"$POSTGIS_SOURCE_DIR/liblwgeom" -I"$POSTGIS_BUILD_DIR/liblwgeom" \
        "$sourceFilename" -o "$OUT/$fuzzerName" \
        $LDFLAGS \
        -lFuzzingEngine -lstdc++ $(target_libs "$fuzzerName") \
        "$POSTGIS_BUILD_DIR/liblwgeom/.libs/liblwgeom.a" $POSTGIS_FUZZER_LIBS

    if [ "$objectFile" != "" ]; then
        rm -f "$objectFile"
    fi
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

for F in "$FUZZERS_DIR"/*.cpp; do
    [ -e "$F" ] || continue
    fuzzerName=$(basename "$F" .cpp)
    build_fuzzer "$fuzzerName" "$F"
done

for F in "$FUZZERS_DIR"/*_fuzzer.c; do
    [ -e "$F" ] || continue
    fuzzerName=$(basename "$F" .c)
    build_fuzzer "$fuzzerName" "$F"
done

for artifact in "$FUZZERS_DIR"/*.dict "$FUZZERS_DIR"/*.options "$FUZZERS_DIR"/*.zip; do
    [ -e "$artifact" ] || continue
    cp "$artifact" "$OUT/"
done

if [ "$POSTGIS_PACKAGE_RUNTIME_LIBS" != "0" ]; then
    package_runtime_libs
fi
