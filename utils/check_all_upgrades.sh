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

INSTALLED_EXTENSIONS=postgis
if test -f postgis_topology--${to_version}.sql; then
  INSTALLED_EXTENSIONS="$INSTALLED_EXTENSIONS postgis_topology"
fi
if test -f postgis_raster--${to_version}.sql; then
  INSTALLED_EXTENSIONS="$INSTALLED_EXTENSIONS postgis_raster"
fi

echo "INFO: installed extensions: $INSTALLED_EXTENSIONS"

for EXT in ${INSTALLED_EXTENSIONS}; do
  if test "${EXT}" = "postgis"; then
    REGDIR=${BUILDDIR}/regress
  elif test "${EXT}" = "postgis_topology"; then
    REGDIR=${BUILDDIR}/topology/test
  elif test "${EXT}" = "postgis_raster"; then
    REGDIR=${BUILDDIR}/raster/test/regress
  else
    echo "SKIP: don't know where to find regress tests for extension ${EXT}"
  fi
  files=`'ls' ${EXT}--* | grep -v -- '--.*--' | sed "s/^${EXT}--\(.*\)\.sql/\1/"`
  for fname in unpackaged $files; do
    from_version="$fname"
    # only consider versions older than ${to_version}
    cmp=`semver_compare "${from_version}" "${to_version}"`
    if test $cmp -ge 0; then
      continue
    fi
    UPGRADE_PATH="${from_version}--${to_version}"
    if test -e ${EXT}--${UPGRADE_PATH}.sql; then
      echo "Testing ${EXT} upgrade $UPGRADE_PATH"
      export RUNTESTFLAGS="-v --extension --upgrade-path=${UPGRADE_PATH}"
      make -C ${REGDIR} check && {
        echo "PASS: upgrade $UPGRADE_PATH"
      } || {
        echo "FAIL: upgrade $UPGRADE_PATH"
        failures=$((failures+1))
      }
    else
      echo "SKIP: ${EXT} upgrade $UPGRADE_PATH is missing"
    fi
  done
done

exit $failures
