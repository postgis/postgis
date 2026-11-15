#!/usr/bin/env bash
set -euo pipefail

test "$(uname -m)" = aarch64
export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y --no-install-recommends \
  autoconf automake libtool build-essential ca-certificates pkg-config \
  bison flex \
  postgresql postgresql-server-dev-all \
  libcunit1-dev libgeos-dev libproj-dev libgdal-dev libjson-c-dev \
  libprotobuf-c-dev protobuf-c-compiler libxml2-dev gettext zlib1g-dev
rm -rf /var/lib/apt/lists/*

export PORTABILITY_TIER=arm64-berrie64-qemu
export SRCDIR="${CI_WORKSPACE:-$PWD}"
export BUILDDIR="${SRCDIR}/build/portability-${PORTABILITY_TIER}"
export PG_MAJOR="$(pg_config --version | awk '{print $2}' | cut -d. -f1)"
export PATH="/usr/lib/postgresql/${PG_MAJOR}/bin:${PATH}"
export PGROOT="$(mktemp -d)"
export PGDATA="${PGROOT}/data"
export PGHOST="${PGROOT}/socket"
export PGUSER=postgres
export PGDATABASE=postgres
export RUNTESTFLAGS="-v --extension --raster"

uname -m
dpkg-architecture -qDEB_HOST_GNU_TYPE
pg_config --version

cleanup() {
  runuser -u postgres -- pg_ctl -D "${PGDATA}" -m fast -w stop || true
  rm -rf "${PGROOT}"
}
trap cleanup EXIT

chown postgres:postgres "${PGROOT}"
runuser -u postgres -- mkdir "${PGDATA}" "${PGHOST}"
runuser -u postgres -- initdb -D "${PGDATA}" -A trust --no-sync
runuser -u postgres -- pg_ctl -D "${PGDATA}" \
  -l "${PGROOT}/postgresql.log" \
  -o "-F -k ${PGHOST} -c synchronous_commit=off -c full_page_writes=off" \
  -w start
psql -c 'select version()' template1

./autogen.sh
mkdir -p "${BUILDDIR}"
cd "${BUILDDIR}"
"${SRCDIR}/configure" \
  --with-raster \
  --without-topology \
  --without-sfcgal \
  --without-interrupt-tests \
  CC=cc \
  CFLAGS="-O2 -Wall -Werror -fno-omit-frame-pointer"
make -j1
make -C liblwgeom check-unit
make install
make -C regress check RUNTESTFLAGS="${RUNTESTFLAGS}" TESTS=" \
  ${SRCDIR}/regress/core/wkb \
  ${SRCDIR}/regress/core/twkb \
  ${SRCDIR}/regress/core/sql-mm-serialize \
  ${SRCDIR}/regress/core/curvetoline \
  ${SRCDIR}/regress/core/geography_centroid \
  ${SRCDIR}/regress/core/in_geojson \
  ${SRCDIR}/regress/core/out_geojson \
  ${SRCDIR}/regress/core/in_gml \
  ${SRCDIR}/regress/core/out_gml \
  ${SRCDIR}/regress/core/in_kml \
  ${SRCDIR}/regress/core/in_geohash \
  ${SRCDIR}/regress/core/in_flatgeobuf \
  ${SRCDIR}/regress/core/out_flatgeobuf \
  ${SRCDIR}/regress/core/flatgeobuf \
  ${SRCDIR}/regress/core/setpoint \
  ${SRCDIR}/regress/core/tickets \
  ${SRCDIR}/regress/core/nurbs_wkt \
  ${SRCDIR}/regress/core/mvt \
  ${SRCDIR}/regress/loader/LongOptions \
  ${SRCDIR}/regress/loader/TestANALYZE \
  ${SRCDIR}/raster/test/regress/check_gdal \
  ${SRCDIR}/raster/test/regress/loader/load_outdb \
  ${SRCDIR}/raster/test/regress/rt_bytea \
  ${SRCDIR}/raster/test/regress/rt_wkb \
  ${SRCDIR}/raster/test/regress/rt_fromgdalraster \
  ${SRCDIR}/raster/test/regress/rt_asgdalraster \
  ${SRCDIR}/raster/test/regress/rt_band \
  ${SRCDIR}/raster/test/regress/clean"
