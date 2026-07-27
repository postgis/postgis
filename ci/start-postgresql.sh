#!/bin/sh

set -eu

: "${PGVER:?PGVER must name the PostgreSQL major version}"

PGBINDIR=${PGBINDIR:-/usr/lib/postgresql/${PGVER}/bin}
PGCTL=${PGCTL:-${PGBINDIR}/pg_ctl}
INITDB=${INITDB:-${PGBINDIR}/initdb}
PGSTART_USER=${PGSTART_USER:-postgres}
PGSTART_TIMEOUT=${PGSTART_TIMEOUT:-300}

pgstart_number()
{
  case "$1" in
    ''|*[!0-9]*) echo 0 ;;
    *) echo "$1" ;;
  esac
}

pgstart_default_port()
{
  pipeline=$(pgstart_number "${CI_PIPELINE_NUMBER:-0}")
  workflow=$(pgstart_number "${CI_WORKFLOW_NUMBER:-${CI_STEP_NUMBER:-0}}")

  # TCP is disabled below. This only chooses the socket filename inside
  # the private PGHOST directory.
  echo $((20000 + (pipeline % 1000) * 32 + (workflow % 32)))
}

pgstart_print_log()
{
  if test -s "${PGLOG}"; then
    echo "PostgreSQL startup log (${PGLOG}) follows:" >&2
    tail -n 200 "${PGLOG}" >&2
  else
    echo "PostgreSQL startup log (${PGLOG}) is empty or missing" >&2
  fi
}

pgstart_stop()
{
  if test -n "${PGDATA:-}" && test -s "${PGDATA}/postmaster.pid"; then
    runuser -u "${PGSTART_USER}" -- "${PGCTL}" -D "${PGDATA}" -m fast -w stop || true
  fi
  if test -n "${PGROOT:-}"; then
    rm -rf "${PGROOT}"
  fi
}

PGROOT=${PGROOT:-$(mktemp -d "${TMPDIR:-/tmp}/postgis-pg${PGVER}.XXXXXX")}
PGDATA=${PGDATA:-${PGROOT}/data}
PGHOST=${PGHOST:-${PGROOT}/socket}
PGLOG=${PGLOG:-${PGROOT}/postgresql.log}
PGPORT=${PGPORT:-$(pgstart_default_port)}
PGUSER=${PGUSER:-postgres}

export PGROOT PGDATA PGHOST PGLOG PGPORT PGUSER

mkdir -p "${PGROOT}"
chown "${PGSTART_USER}:${PGSTART_USER}" "${PGROOT}"
runuser -u "${PGSTART_USER}" -- mkdir -p "${PGDATA}" "${PGHOST}"

INITDB_OPTS=
if "${INITDB}" --help 2>&1 | grep -q -- '--no-data-checksums'; then
  INITDB_OPTS="--no-data-checksums"
fi

if test ! -s "${PGDATA}/PG_VERSION"; then
  runuser -u "${PGSTART_USER}" -- "${INITDB}" -D "${PGDATA}" -A trust --no-sync ${INITDB_OPTS}
fi

trap pgstart_stop EXIT

PGSTART_OPTS=${PGSTART_OPTS:-"-F -h '' -p ${PGPORT} -k ${PGHOST} -c fsync=off -c full_page_writes=off -c synchronous_commit=off"}

if ! runuser -u "${PGSTART_USER}" -- "${PGCTL}" -D "${PGDATA}" -l "${PGLOG}" \
  -o "${PGSTART_OPTS}" -t "${PGSTART_TIMEOUT}" start; then
  pgstart_print_log
  echo "ci/start-postgresql.sh: PostgreSQL ${PGVER} failed to start" >&2
  exit 1
fi
