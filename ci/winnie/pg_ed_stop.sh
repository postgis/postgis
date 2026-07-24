#!/usr/bin/env bash

# Reset the EDB PostgreSQL cluster used by Winnie before PostGIS regression
# jobs.  This script is intended to replace the Jenkins-managed PG_ED_STOP
# batch so the cleanup logic lives with the rest of the Winnie CI scripts.

set -e

# shellcheck source=ci/winnie/winnie_common.sh
. "$(dirname "$0")/winnie_common.sh"

if [ -z "${PGPORT:-}" ]; then
  echo "PGPORT is required"
  exit 1
fi

PGDATA="${PGPATHEDB}/data_${PGPORT}"
LOGFILE="${PGPATHEDB}/logfile_${PGPORT}"
INITDB_LOG="${PGPATHEDB}/initdb_${PGPORT}.log"

echo "Resetting Winnie PostgreSQL ${PG_VER} ${OS_BUILD}-bit cluster on port ${PGPORT}"
echo "PGPATHEDB=${PGPATHEDB}"
echo "PGDATA=${PGDATA}"

win_path() {
  cygpath -aw "$1" 2>/dev/null || printf "%s\n" "$1"
}

kill_pid() {
  pid="$1"
  if [ -z "${pid}" ] || [ "${pid}" = "0" ]; then
    return
  fi

  echo "Terminating stale PostgreSQL process ${pid}"
  taskkill.exe /PID "${pid}" /T /F || true
}

kill_postgres_by_port() {
  netstat.exe -ano -p TCP 2>/dev/null |
    awk -v port=":${PGPORT}" '
      $2 ~ port "$" && $NF ~ /^[0-9]+$/ {
        print $NF
      }
    ' |
    sort -u |
    while read -r pid; do
      kill_pid "${pid}"
    done
}

kill_postgres_by_command_line() {
  WINNIE_PGDATA_WIN="$(win_path "${PGDATA}")"
  WINNIE_PGPATH_WIN="$(win_path "${PGPATHEDB}")"
  export WINNIE_PGDATA_WIN WINNIE_PGPATH_WIN PGPORT

  # shellcheck disable=SC2016
  powershell.exe -NoProfile -ExecutionPolicy Bypass -Command '
    $pgdata = $env:WINNIE_PGDATA_WIN;
    $pgpath = $env:WINNIE_PGPATH_WIN;
    $pgport = $env:PGPORT;
    Get-CimInstance Win32_Process |
      Where-Object {
        ($_.Name -eq "postgres.exe") -and (
          ($_.CommandLine -like "*$pgdata*") -or
          ($_.CommandLine -like "*$pgpath*data_$pgport*") -or
          ($_.CommandLine -like "*-p $pgport*")
        )
      } |
      ForEach-Object { $_.ProcessId }
  ' 2>/dev/null |
    tr -d '\r' |
    awk 'NF { print $1 }' |
    sort -u |
    while read -r pid; do
      kill_pid "${pid}"
    done
}

clear_stale_postgres_processes() {
  echo "Checking for stale PostgreSQL processes bound to ${PGDATA} or port ${PGPORT}"
  kill_postgres_by_command_line
  kill_postgres_by_port
}

stop_cluster() {
  if [ -x "${PGPATHEDB}/bin/pg_ctl" ]; then
    "${PGPATHEDB}/bin/pg_ctl" -D "${PGDATA}" -l "${LOGFILE}" stop || true
  fi
}

initialize_cluster() {
  rm -f "${INITDB_LOG}"
  "${PGPATHEDB}/bin/initdb" -D "${PGDATA}" -E=UTF8 -U postgres -A trust > "${INITDB_LOG}" 2>&1
}

cd "${PGPATHEDB}"

stop_cluster
clear_stale_postgres_processes
rm -rf "${PGDATA}"

if ! initialize_cluster; then
  cat "${INITDB_LOG}"
  if grep -q "pre-existing shared memory block is still in use" "${INITDB_LOG}"; then
    echo "initdb found stale PostgreSQL shared memory; terminating matching processes and retrying once"
    clear_stale_postgres_processes
    rm -rf "${PGDATA}"
    initialize_cluster
  else
    exit 1
  fi
fi

"${PGPATHEDB}/bin/pg_ctl" -D "${PGDATA}" -l "${LOGFILE}" -o "-F -p ${PGPORT}" start
"${PGPATHEDB}/bin/pg_ctl" -D "${PGDATA}" -l "${LOGFILE}" stop
