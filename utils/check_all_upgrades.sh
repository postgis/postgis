#!/bin/sh

if test -z "$1"; then
  echo "Usage: $0 <to_version>" >&2
  exit 1
fi
to_version="$1"

BUILDDIR=$PWD
EXTDIR=`pg_config --sharedir`/extension/

cd $EXTDIR
failures=0
files=`'ls' postgis--* | grep -v -- '--.*--' | sed 's/^postgis--\(.*\)\.sql/\1/'`
for fname in unpackaged $files; do
  from_version="$fname"
  UPGRADE_PATH="${from_version}--${to_version}"
  if test -e postgis--${UPGRADE_PATH}.sql; then
    echo "Testing upgrade $UPGRADE_PATH"
    export RUNTESTFLAGS="-v --extension --upgrade-path=${UPGRADE_PATH}"
    make -C ${BUILDDIR}/regress check && {
      echo "PASS: upgrade $UPGRADE_PATH"
    } || {
      echo "FAIL: upgrade $UPGRADE_PATH"
      failures=$((failures+1))
    }
  else
    echo "SKIP: upgrade $UPGRADE_PATH is missing"
  fi
done

exit $failures
