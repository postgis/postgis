#!/bin/sh

EXIT_ON_FIRST_FAILURE=0
EXTDIR=`pg_config --sharedir`/extension/
CTBDIR=`pg_config --sharedir`/contrib/
TMPDIR=/tmp/check_all_upgrades-$$-tmp
PGVER=`pg_config --version | awk '{print $2}'`
PGVER_MAJOR=$(echo "${PGVER}" | sed 's/\.[^\.]*//' | sed 's/\(alpha\|beta\|rc\).*//' )
echo "INFO: PostgreSQL version: ${PGVER} [${PGVER_MAJOR}]"

BUILDDIR=$PWD # TODO: allow override ?

cd $(dirname $0)/..
SRCDIR=$PWD # TODO: allow override ?
cd -


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

mkdir -p ${TMPDIR}
cleanup()
{
  rm -rf ${TMPDIR}
}

trap 'cleanup' 0


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

failed()
{
  failures=$((failures+1))
  if test $EXIT_ON_FIRST_FAILURE != 0 -a $failures != 0; then
    exit $failures
  fi
}

minimum_postgis_version_for_postgresql_major_version()
{
  pgver=$1
  supportfile=${TMPDIR}/minimum_supported_version_for_postgresql

  if ! test -e ${supportfile}; then
    # Source: https://trac.osgeo.org/postgis/wiki/UsersWikiPostgreSQLPostGIS
    cat > ${supportfile} <<EOF
9.6:2.3
10:2.4
11:2.5
12:2.5
13:3.0
14:3.1
15:3.2
EOF
  fi

  # Drop patch-level number from PostgreSQL version
  minsupported=`grep ^${pgver}: ${supportfile} | cut -d: -f2`
  test -n "${minsupported}" || {
    echo "Cannot detemine minimum supported PostGIS version for PostgreSQL major version ${pgver}" >&2
    exit 1
  }
  echo "${minsupported}"
}

compatible_upgrade()
{
  label=$1
  from=$2
  to=$3

  #echo "XXXX compatible_upgrade: upgrade_path=${upgrade_path}"
  #echo "XXXX compatible_upgrade: from=${from}"
  #echo "XXXX compatible_upgrade: to=${to}"

  # only consider versions older than ${to}
  cmp=`semver_compare "${from}" "${to}"`
  #echo "INFO: semver_compare ${from} ${to} returned $cmp"
  if test $cmp -ge 0; then
    echo "SKIP: $label ($to is not newer than $from)"
    return 1
  fi
  cmp=`semver_compare "${PGIS_MIN_VERSION}" "${from}"`
  if test $cmp -gt 0; then
    echo "SKIP: $label ($from older than ${PGIS_MIN_VERSION}, which is required to run in PostgreSQL ${PGVER})"
    return 1
  fi

  return 0
}

to_version_param="$1"
to_version=$to_version_param
if expr $to_version : ':auto' >/dev/null; then
  export PGDATABASE=template1
  to_version=`psql -XAtc "select default_version from pg_available_extensions where name = 'postgis'"` || exit 1
elif expr $to_version : '.*!$' >/dev/null; then
  to_version=$(echo "${to_version}" | sed 's/\!$//')
fi


PGIS_MIN_VERSION=`minimum_postgis_version_for_postgresql_major_version "${PGVER_MAJOR}"`
echo "INFO: minimum PostGIS version supporting PostgreSQL ${PGVER_MAJOR}: ${PGIS_MIN_VERSION}"


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

  USERTESTFLAGS=${RUNTESTFLAGS}

  # Check extension->extension upgrades
  files=`'ls' ${EXT}--* | grep -v -- '--.*--' | sed "s/^${EXT}--\(.*\)\.sql/\1/"`
  for fname in $files; do
    from_version="$fname"
    UPGRADE_PATH="${from_version}--${to_version_param}"
    test_label="${EXT} extension upgrade ${UPGRADE_PATH}"
    if expr $to_version_param : ':auto' >/dev/null; then
      test_label="${test_label} ($to_version)"
    fi
    compatible_upgrade "${test_label}" ${from_version} ${to_version} || continue
    UPGRADE_FILE="${EXT}--${from_version}--${to_version}.sql"
    if ! test -e ${UPGRADE_FILE}; then
      echo "SKIP: ${test_label} ($UPGRADE_FILE is missing)"
      continue
    fi
    echo "Testing ${test_label}"
    RUNTESTFLAGS="-v --extension --upgrade-path=${UPGRADE_PATH} ${USERTESTFLAGS}" \
    make -C ${REGDIR} check && {
      echo "PASS: ${test_label}"
    } || {
      echo "FAIL: ${test_label}"
      failed
    }
  done

  # Check unpackaged->extension upgrades
  for majmin in `'ls' -d ${CTBDIR}/postgis-* | sed 's/.*postgis-//'`; do
    UPGRADE_PATH="unpackaged${majmin}--${to_version_param}"
    test_label="${EXT} extension upgrade ${UPGRADE_PATH}"
    if expr $to_version_param : ':auto' >/dev/null; then
      test_label="${test_label} ($to_version)"
    fi
    compatible_upgrade "${test_label}" ${majmin} ${to_version} || continue
    UPGRADE_FILE="${EXT}--unpackaged--${to_version}.sql"
    if ! test -e ${UPGRADE_FILE}; then
      echo "SKIP: ${test_label} ($UPGRADE_FILE is missing)"
      continue
    fi
    echo "Testing ${test_label}"
    RUNTESTFLAGS="-v --extension --upgrade-path=${UPGRADE_PATH} ${USERTESTFLAGS}" \
    make -C ${REGDIR} check && {
      echo "PASS: ${test_label}"
    } || {
      echo "FAIL: ${test_label}"
      failed
    }
  done

  # Check unpackaged->unpackaged upgrades
  CURRENTVERSION=`grep '^POSTGIS_' ${SRCDIR}/Version.config | cut -d= -f2 | paste -sd '.'`
  if test ${to_version} = "${CURRENTVERSION}"; then
    for majmin in `'ls' -d ${CTBDIR}/postgis-* | sed 's/.*postgis-//'`
    do #{
      UPGRADE_PATH="unpackaged${majmin}--:auto"
      test_label="${EXT} script-based upgrade ${UPGRADE_PATH}"
      if expr $to_version_param : ':auto' >/dev/null; then
        test_label="${test_label} ($to_version)"
      fi
      compatible_upgrade "${test_label}" ${majmin} ${to_version} || continue
      echo "Testing ${test_label}"
      RUNTESTFLAGS="-v --upgrade-path=${UPGRADE_PATH} ${USERTESTFLAGS}" \
      make -C ${REGDIR} check && {
        echo "PASS: ${test_label}"
      } || {
        echo "FAIL: ${test_label}"
        failed
      }
    done #}
  else #}{
    echo "SKIP: ${EXT} script-based upgrades (${to_version_param} [${to_version}] does not match built version ${CURRENTVERSION})"
  fi #}

done

exit $failures
