#!/bin/sh

if test -z "$1"; then
  echo "Usage: $0 <to_version>" >&2
  exit 1
fi
to_version="$1"

BUILDDIR=$PWD
EXTDIR=`pg_config --sharedir`/extension/
RET=0

cd $EXTDIR
'ls' postgis--* | grep -v -- '--.*--' |
sed 's/^postgis--\(.*\)\.sql/\1/' | while read fname; do
  from_version="$fname"
  UPGRADE_PATH="${from_version}--${to_version}"
  if test -e postgis--${UPGRADE_PATH}.sql; then
    echo "Testing upgrade $UPGRADE_PATH"
    export RUNTESTFLAGS="-v --extension --upgrade-path=${UPGRADE_PATH}"
    make -C ${BUILDDIR}/regress check
    ret=$?
    echo "UPGRADE TEST EXIT CODE: $ret"
    if test $ret -ne 0; then
      cat /tmp/pgis_reg/regress_log
    fi
    RET=$((RET+$ret))
    dropdb postgis_reg
  else
    echo "Missing script for $UPGRADE_PATH upgrade" >&2
  fi
done

exit $RET
