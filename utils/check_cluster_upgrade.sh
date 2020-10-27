#!/bin/sh

TMPDIR=/tmp/pgis_upgrade_test-$$/
DATADIR=${TMPDIR}/cluster
LOGFILE=${TMPDIR}/log
INIT_SCRIPT=
export PGPORT=15432
export PGHOST=${TMPDIR}
export PGDATABASE=pgis
PG_CONFIG_OLD=
PG_CONFIG_NEW=

usage() {
  echo "Usage: $0 [-i <init_script>] <pg_config_old> <pg_config_new>"
}

cleanup() {
  if test -f ${DATADIR}/cluster/postmaster.pid; then
    ${BIN_OLD}/pg_ctl -D ${DATADIR} stop
  fi
  rm -rf ${TMPDIR}
}

trap 'cleanup' EXIT

mkdir -p ${TMPDIR}

while test -n "$1"; do
  if test "$1" = "-i"; then
    shift
    INIT_SCRIPT=$1
    if test -f ${INIT_SCRIPT}; then
      :
    else
      echo "${INIT_SCRIPT} is not a file" >&2
      exit 1
    fi
    shift
  elif test -z "$PG_CONFIG_OLD"; then
    PG_CONFIG_OLD="$1"
    shift
  elif test -z "$PG_CONFIG_NEW"; then
    PG_CONFIG_NEW="$1"
    shift
  else
    echo "Unrecognized argument: $1" >&2
    usage >&2
    exit 1
  fi
done

if test -z "$PG_CONFIG_NEW"; then
  echo "Missing new pg_config path" >&2
  usage >&2
  exit 1
fi

if test "$PG_CONFIG_OLD" = "$PG_CONFIG_NEW"; then
  echo "Old and new pg_config paths need be different" >&2
  exit 1
fi

BIN_OLD=$(${PG_CONFIG_OLD} --bindir)
BIN_NEW=$(${PG_CONFIG_NEW} --bindir)

echo "Testing cluster upgrade"
echo "FROM: $(${PG_CONFIG_OLD} --version)"
echo "  TO: $(${PG_CONFIG_NEW} --version)"

echo "Creating FROM cluster"
${BIN_OLD}/initdb ${DATADIR} > ${LOGFILE} 2>&1 || {
  cat ${LOGFILE} && exit 1
}

echo "Starting FROM cluster"
${BIN_OLD}/pg_ctl -D ${DATADIR} -o "-F -p ${PGPORT} -k ${TMPDIR}" -l ${LOGFILE} start || {
  cat ${LOGFILE} && exit 1
}

echo "Creating FROM db"
${BIN_OLD}/createdb pgis || exit 1

#TODO: run an custom script
if test -n "$INIT_SCRIPT"; then
  ${BIN_OLD}/psql -X --set ON_ERROR_STOP=1 -f "$INIT_SCRIPT" || exit 1
else
  ${BIN_OLD}/psql -Xc --set ON_ERROR_STOP=1 "CREATE EXTENSION postgis" || exit 1
fi

echo "---- OLD cluster info -------------------------"
${BIN_OLD}/psql -XAt --set ON_ERROR_STOP=1 <<EOF || exit 1
SELECT version();
SELECT postgis_full_version();
EOF
echo "-----------------------------------------------"

echo "Stopping FROM cluster"
${BIN_OLD}/pg_ctl -D ${DATADIR} stop || exit 1

export PGDATAOLD=${DATADIR}.old
mv ${DATADIR} ${PGDATAOLD}

export PGDATANEW=${DATADIR}

echo "Creating TO cluster"
${BIN_NEW}/initdb ${PGDATANEW} > ${LOGFILE} 2>&1 || {
  cat ${LOGFILE} && exit 1
}

echo "Upgrading cluster"
cd ${TMPDIR}
${BIN_NEW}/pg_upgrade --link -b ${BIN_OLD} -B ${BIN_NEW} > ${LOGFILE} 2>&1 ||  {
  cat ${LOGFILE}
  DUMPLOG=$(grep 'pg_upgrade_dump.*log' ${LOGFILE} | cut -d'"' -f2)
  if test -n "${DUMPLOG}"; then
    echo "${DUMPLOG} follows:"
    tail ${TMPDIR}/${DUMPLOG}
  fi
  exit 1
}

echo "Starting TO cluster"
${BIN_NEW}/pg_ctl -D ${DATADIR} -o "-F -p ${PGPORT} -k ${TMPDIR}" -l ${LOGFILE} start || {
  cat ${LOGFILE} && exit 1
}

echo "---- NEW cluster info -------------------------"
${BIN_NEW}/psql -XAx <<EOF || exit 1
SELECT version();
SELECT postgis_full_version();
SELECT postgis_extensions_upgrade();
SELECT postgis_full_version();
EOF
echo "-----------------------------------------------"
