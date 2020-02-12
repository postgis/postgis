#!/bin/sh

usage() {

  echo "Usage: $0 <command> [<args>]"
  echo "Commands:"
  echo "  help               print this message and exits"
  echo "  enable <database>  enable postgis in given database"
  echo "  upgrade <database>  enable postgis in given database"
  echo "  status <database>  prints postgis status in given database"

}

enable() {
  db="$1"; shift
  test -n "$db" || {
    echo "Please specify a database name" >&2
    return 1
  }
  echo "Enable is not implemented yet" >&2
  return 1
}

upgrade() {
  test -n "$1" || {
    echo "Please specify at least a database name" >&2
    return 1
  }
  for db in $@; do
    echo "upgrading db $db"
    LOG=`cat <<EOF | psql -XtA ${db}
BEGIN;
UPDATE pg_catalog.pg_extension SET extversion = 'ANY'
 WHERE extname IN (
			'postgis',
			'postgis_raster',
			'postgis_sfcgal',
			'postgis_topology',
			'postgis_tiger_geocoder'
  );
SELECT postgis_extensions_upgrade();
COMMIT;
EOF`
    #sh $0 status ${db}
  done
}

status() {
  test -n "$1" || {
    echo "Please specify at least a database name" >&2
    return 1
  }
  for db in $@; do
    SCHEMA=`cat <<EOF | psql -XtA ${db}
SELECT n.nspname
  FROM pg_namespace n, pg_proc p
  WHERE n.oid = p.pronamespace
    AND p.proname = 'postgis_full_version'
EOF
`
    if test -z "$SCHEMA"; then
      echo "db $db does not have postgis enabled"
      continue
    fi
    FULL_VERSION=`cat <<EOF | psql -XtA ${db}
SELECT ${SCHEMA}.postgis_full_version()
EOF
`
    #POSTGIS="3.1.0dev r3.1.0alpha1-3-gfc5392de7"
    VERSION=`echo "$FULL_VERSION" | sed 's/POSTGIS="\([^ ]*\).*/\1/'`
    EXTENSION=
    if expr "$FULL_VERSION" : '.*\[EXTENSION\]' > /dev/null; then
      EXTENSION=" as extension"
    fi
    NEED_UPGRADE=
    if expr "$FULL_VERSION" : '.*need upgrade' > /dev/null; then
      NEED_UPGRADE=" - NEEDS UPGRADE"
    fi

    echo "db $db has postgis ${VERSION}${EXTENSION} in schema ${SCHEMA}${NEED_UPGRADE}"
  done
}

test -n "$1" || {
  usage >&2
  exit 1
}

while test -n "$1"; do
  if test "$1" = "help"; then
    usage 0
  elif test "$1" = "enable"; then
    shift
    enable $@
    exit $?
  elif test "$1" = "upgrade"; then
    shift
    upgrade $@
    exit $?
  elif test "$1" = "status"; then
    shift
    status $@
    exit $?
  else
    echo "Unrecognized command: $1" >&2
    usage >&2
    exit 1
  fi
  shift
done
