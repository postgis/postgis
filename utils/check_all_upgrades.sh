#!/bin/sh

if test -z "$1"; then
  echo "Usage: $0 <to_version>" >&2
  exit 1
fi
to_version="$1"

# Return -1, 1 or 0 if the first version
# is respectively smaller, greater or equal
# to the second version
semver_compare()
{
  V1=`echo "$1" | tr '.' ' '`
  V2=$2
  # echo "V1: $V1" >&2
  # echo "V2: $V2" >&2
  for v1 in $V1; do
    v2=`echo "$V2" | cut -d. -f1`
    if [ -z "$v2" ]; then
      echo 1; return;
    fi
    V2=`echo "$V2" | cut -d. -sf2-`
    # echo "v: $v1 - $v2" >&2
    if expr "$v1" '<' "$v2" > /dev/null; then
      echo -1; return;
    fi
    if expr "$v1" '>' "$v2" > /dev/null; then
      echo 1; return;
    fi
  done
  if [ -n "$V2" ]; then
    echo -1; return;
  fi
  echo 0; return;
}


BUILDDIR=$PWD
EXTDIR=`pg_config --sharedir`/extension/

cd $EXTDIR
failures=0
files=`'ls' postgis--* | grep -v -- '--.*--' | sed 's/^postgis--\(.*\)\.sql/\1/'`
for fname in unpackaged $files; do
  from_version="$fname"
  # only consider versions older than ${to_version}
  cmp=`semver_compare "${from_version}" "${to_version}"`
  if test $cmp -ge 0; then
    continue
  fi
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
