#BRANCH is passed in via jenkins which is set via svn hook
export PATH=${PATH}:/usr/local:/usr/local/lib:/usr/local/bin

pg_config_major()
{
  "$1" --version 2>/dev/null | sed -n 's/^PostgreSQL \([0-9][0-9]*\).*/\1/p'
}

find_supported_pg_config()
{
  for candidate in \
    "${PG_CONFIG:-}" \
    /usr/local/bin/pg_config15 \
    /usr/local/pgsql15/bin/pg_config \
    /usr/local/pgsql-15/bin/pg_config \
    /usr/local/postgresql15/bin/pg_config \
    "$(command -v pg_config || true)"
  do
    test -n "${candidate}" || continue
    test -x "${candidate}" || continue
    major=$(pg_config_major "${candidate}")
    test -n "${major}" || continue
    if test "${major}" -le 15; then
      printf '%s\n' "${candidate}"
      return 0
    fi
  done
  return 1
}

PG_CONFIG_PATH=$(find_supported_pg_config || true)
if test -z "${PG_CONFIG_PATH}"; then
  pg_config_version=$(pg_config --version 2>/dev/null || true)
  echo "Skipping Bessie: PostGIS 3.2 supports PostgreSQL <= 15, but no supported pg_config was found. Default pg_config: ${pg_config_version:-missing}."
  exit 0
fi

PG_BINDIR=$(dirname "${PG_CONFIG_PATH}")
export PATH="${PG_BINDIR}:${PATH}"

PGIS_TMP_ROOT=$(mktemp -d "${TMPDIR:-/tmp}/postgis-bessie.XXXXXX")
export PGDATA="${PGIS_TMP_ROOT}/data"
export PGHOST="${PGIS_TMP_ROOT}"
export PGUSER=postgres
export PGIS_REG_TMPDIR="${PGIS_TMP_ROOT}/regress"
PGLOG="${PGIS_TMP_ROOT}/postgresql.log"

cleanup() {
  status=$?
  trap - EXIT
  if test -s "${PGDATA}/postmaster.pid"; then
    pg_ctl -D "${PGDATA}" -m immediate stop >/dev/null 2>&1 || true
  fi
  if test "${status}" -ne 0 && test -s "${PGLOG}"; then
    echo "PostgreSQL log follows:" >&2
    tail -n 100 "${PGLOG}" >&2
  fi
  rm -rf -- "${PGIS_TMP_ROOT}"
  exit "${status}"
}

trap cleanup EXIT
trap 'exit 1' HUP INT TERM

sh autogen.sh
./configure \
  --with-pgconfig="${PG_CONFIG_PATH}" \
  --with-projdir=/usr/local \
  --with-libiconv=/usr/local \
  --without-interrupt-tests \
  --with-library-minor-version
make clean
make

initdb -N --no-data-checksums -U "${PGUSER}" -D "${PGDATA}"
printf '%s\n' \
  'fsync = off' \
  'full_page_writes = off' \
  'synchronous_commit = off' \
  >> "${PGDATA}/postgresql.conf"
pg_ctl -D "${PGDATA}" -l "${PGLOG}" \
  -o "-F -c listen_addresses='' -c unix_socket_directories='${PGHOST}'" start
mkdir -p "${PGIS_REG_TMPDIR}"

make check RUNTESTFLAGS="-v"
sudo make install
make check RUNTESTFLAGS="-v --extension"
