#!/bin/sh

EXIT_ON_FIRST_FAILURE=0

if test "$1" = "-s"; then
  EXIT_ON_FIRST_FAILURE=1
  shift
fi

if test -z "$1"; then
  echo "Usage: $0 [-s] <to_version>" >&2
  echo "Options:" >&2
  echo "\t-s  Stop on first failure" >&2
  exit 1
fi

to_version_param="$1"
to_version=$to_version_param
if expr $to_version : ':auto' >/dev/null; then
  export PGDATABASE=template1
  to_version=`psql -XAtc "select default_version from pg_available_extensions where name = 'postgis'"` || exit 1
elif expr $to_version : '.*!$' >/dev/null; then
  to_version=$(echo "${to_version}" | sed 's/\!$//')
fi


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
CTBDIR=`pg_config --sharedir`/contrib/
PGVER=`pg_config --version | awk '{print $2}'`

echo "PostgreSQL version: ${PGVER}"

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

failed()
{
  failures=$((failures+1))
  if test $EXIT_ON_FIRST_FAILURE != 0 -a $failures != 0; then
    exit $failures
  fi
}

compatible_upgrade()
{
  upgrade_path=$1
  from=$2
  to=$3

  #echo "XXXX compatible_upgrade: upgrade_path=${upgrade_path}"
  #echo "XXXX compatible_upgrade: from=${from}"
  #echo "XXXX compatible_upgrade: to=${to}"

  # only consider versions older than ${to_version_param}
  cmp=`semver_compare "${from}" "${to}"`
  if test $cmp -ge 0; then
    echo "SKIP: upgrade $upgrade_path ($to is not newer than $from)"
    return 1
  fi
  cmp=`semver_compare "${PGVER}" "11"`
  if test $cmp -ge 0; then
    cmp=`semver_compare "3.0" "${from}"`
    if test $cmp -ge 0; then
      echo "SKIP: upgrade $UPGRADE_PATH ($from < 3.0 which is required to run in PostgreSQL ${PGVER})"
      return 1
    fi
  fi

  return 0
}

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

  USERTESTFLAGS=${RUNTESTFLAGS}

  # Check extension->extension upgrades
  files=`'ls' ${EXT}--* | grep -v -- '--.*--' | sed "s/^${EXT}--\(.*\)\.sql/\1/"`
  for fname in $files; do
    from_version="$fname"
    UPGRADE_PATH="${from_version}--${to_version_param}"
    UPGRADE_FILE="${EXT}--${from_version}--${to_version}.sql"
    compatible_upgrade ${UPGRADE_PATH} ${from_version} ${to_version} || continue
    if test -e ${UPGRADE_FILE}; then
      if expr $to_version_param : ':auto' >/dev/null; then
        echo "Testing ${EXT} upgrade $UPGRADE_PATH ($to_version)"
      else
        echo "Testing ${EXT} upgrade $UPGRADE_PATH"
      fi
      RUNTESTFLAGS="-v --extension --upgrade-path=${UPGRADE_PATH} ${USERTESTFLAGS}" \
      make -C ${REGDIR} check && {
        echo "PASS: ${EXT} upgrade $UPGRADE_PATH"
      } || {
        echo "FAIL: ${EXT} upgrade $UPGRADE_PATH"
        failed
      }
    else
      echo "SKIP: ${EXT} upgrade $UPGRADE_FILE is missing"
    fi
  done

  # Check unpackaged->extension upgrades
  for majmin in `'ls' -d ${CTBDIR}/postgis-* | sed 's/.*postgis-//'`; do
    UPGRADE_PATH="unpackaged${majmin}--${to_version_param}"
    UPGRADE_FILE="${EXT}--unpackaged--${to_version}.sql"
    if ! test -e ${UPGRADE_FILE}; then
      echo "SKIP: ${EXT} upgrade $UPGRADE_FILE is missing"
      continue
    fi
    compatible_upgrade ${UPGRADE_PATH} ${majmin} ${to_version} || continue
    if expr $to_version_param : ':auto' >/dev/null; then
      echo "Testing ${EXT} upgrade $UPGRADE_PATH ($to_version)"
    else
      echo "Testing ${EXT} upgrade $UPGRADE_PATH"
    fi
    RUNTESTFLAGS="-v --extension --upgrade-path=${UPGRADE_PATH} ${USERTESTFLAGS}" \
    make -C ${REGDIR} check && {
      echo "PASS: ${EXT} upgrade $UPGRADE_PATH"
    } || {
      echo "FAIL: ${EXT} upgrade $UPGRADE_PATH"
      failed
    }
  done

done

exit $failures
