#!/usr/bin/env bash

set -Eeuo pipefail

SRCDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "${SRCDIR}"

if python3 ci/ci_need_suite.py github-ci; then
  :
else
  gate_status=$?
  if [[ "${gate_status}" -eq 78 ]]; then
    exit 0
  fi
  exit "${gate_status}"
fi

if [[ "$(id -u)" -eq 0 ]]; then
  echo "PostGIS macOS CI must run as an unprivileged user" >&2
  exit 1
fi

if [[ "$(uname -s)" != Darwin || "$(uname -m)" != arm64 ]]; then
  echo "PostGIS macOS CI requires native Darwin arm64" >&2
  exit 1
fi

readonly HOMEBREW_PREFIX="${HOMEBREW_PREFIX:-/opt/homebrew}"
readonly PG_HOME="${HOMEBREW_PREFIX}/opt/postgresql@18"
readonly PG_CONFIG="${PG_HOME}/bin/pg_config"
readonly INITDB="${PG_HOME}/bin/initdb"
readonly POSTGRES="${PG_HOME}/bin/postgres"
readonly PG_ISREADY="${PG_HOME}/bin/pg_isready"
readonly BUILD_JOBS="${POSTGIS_BUILD_JOBS:-4}"

case "${BUILD_JOBS}" in
  ''|*[!0-9]*|0)
    echo "POSTGIS_BUILD_JOBS must be a positive integer" >&2
    exit 2
    ;;
esac

for command_path in "${PG_CONFIG}" "${INITDB}" "${POSTGRES}" "${PG_ISREADY}"; do
  if [[ ! -x "${command_path}" ]]; then
    echo "Required PostgreSQL 18 command is missing: ${command_path}" >&2
    exit 1
  fi
done

PGROOT="$(mktemp -d "${TMPDIR:-/tmp}/postgis-macos-ci.XXXXXXXX")"
PGDATA="${PGROOT}/data"
PGHOST="${PGROOT}/socket"
PGIS_REG_TMPDIR="${PGROOT}/regress"
BUILDDIR="${PGROOT}/build"
PGPORT=5432
PG_PID=
INSTALLROOT="${PGROOT}/install"
EXTENSION_SHARE="${INSTALLROOT}$(${PG_CONFIG} --sharedir)"
EXTENSION_LIB="${INSTALLROOT}$(${PG_CONFIG} --pkglibdir)"

cleanup() {
  local status=$?
  trap - EXIT
  if [[ -n "${PG_PID}" ]]; then
    if kill -0 "${PG_PID}" 2>/dev/null; then
      kill -TERM "${PG_PID}" || status=$?
    fi
    wait "${PG_PID}" || true
  fi
  rm -rf "${PGROOT}"
  exit "${status}"
}
trap cleanup EXIT

export PGHOST PGPORT
export PGUSER="$(id -un)"
export PGDATABASE=postgres
export PGIS_REG_TMPDIR
export PGIS_REG_SHAREDIR="${EXTENSION_SHARE}"

export PATH="${PG_HOME}/bin:${HOMEBREW_PREFIX}/bin:${HOMEBREW_PREFIX}/sbin:${HOMEBREW_PREFIX}/opt/ccache/libexec:${PATH}"
export PGCONFIG="${PG_CONFIG}"
export CFLAGS="-I${HOMEBREW_PREFIX}/opt/gettext/include -I${PG_HOME}/include -I${HOMEBREW_PREFIX}/include -Wno-nullability-completeness"
export LDFLAGS="-L${HOMEBREW_PREFIX}/opt/gettext/lib -L${PG_HOME}/lib"
export CXXFLAGS="-std=c++17"
export CC="ccache clang"
export CXX="ccache clang++"
export CCACHE_DIR="${PGROOT}/ccache"
export CCACHE_MAXSIZE=1G

ccache --max-size="${CCACHE_MAXSIZE}"
./autogen.sh

mkdir -p "${BUILDDIR}"
cd "${BUILDDIR}"
"${SRCDIR}/configure" \
  --without-gui \
  --without-interrupt-tests \
  --with-topology \
  --without-raster \
  --with-sfcgal \
  --with-protobuf \
  --with-pgconfig="${PGCONFIG}"

# Each workflow uses its own Unix socket directory and data directory. TCP is
# disabled, so concurrent local-backend workflows can safely reuse port 5432.
mkdir -p "${PGHOST}"
"${INITDB}" -D "${PGDATA}" -A trust --no-sync
"${POSTGRES}" -D "${PGDATA}" -F -h '' -p "${PGPORT}" -k "${PGHOST}" \
  -c fsync=off -c full_page_writes=off -c synchronous_commit=off \
  -c "extension_control_path=${EXTENSION_SHARE}:\$system" \
  -c "dynamic_library_path=${EXTENSION_LIB}:\$libdir" \
  > "${PGROOT}/postgresql.log" 2>&1 &
PG_PID=$!

postgres_ready=0
for ((attempt = 0; attempt < 300; attempt++)); do
  if "${PG_ISREADY}" -q -h "${PGHOST}" -p "${PGPORT}" -d template1 -U "${PGUSER}"; then
    postgres_ready=1
    break
  fi
  if ! kill -0 "${PG_PID}" 2>/dev/null; then
    break
  fi
  sleep 1
done
if [[ "${postgres_ready}" -ne 1 ]]; then
  echo "PostgreSQL 18 did not become ready" >&2
  cat "${PGROOT}/postgresql.log" >&2
  exit 1
fi
psql -XAt -d template1 -c 'SELECT version()'

# Install only into the workflow directory. PostgreSQL 18 can discover the
# private extension controls; their module paths must also point at this build.
make -j"${BUILD_JOBS}"
make install DESTDIR="${INSTALLROOT}"
python3 - "${EXTENSION_SHARE}/extension" "${EXTENSION_LIB}" <<'PYCONTROLS'
import pathlib
import sys
for control in pathlib.Path(sys.argv[1]).glob("postgis*.control"):
    control.write_text(control.read_text().replace("$libdir", sys.argv[2]))
PYCONTROLS
if make check RUNTESTFLAGS="-v --extension --dumprestore"; then
  :
else
  status=$?
  show_regression_diffs() {
    local found=0
    local diff_file
    for diff_file in "${PGIS_REG_TMPDIR}"/*_diff; do
      [[ -f "${diff_file}" ]] || continue
      if [[ "${found}" -eq 0 ]]; then
        echo "---- BEGIN REGRESSION DIFFS (${PGIS_REG_TMPDIR}) ----"
      fi
      found=1
      echo "---- ${diff_file} ----"
      cat "${diff_file}"
    done
    if [[ "${found}" -eq 0 ]]; then
      echo "No regression diff files found in ${PGIS_REG_TMPDIR}"
    else
      echo "---- END REGRESSION DIFFS ----"
    fi
  }
  show_regression_diffs
  exit "${status}"
fi
