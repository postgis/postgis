#!/bin/sh

EXIT_ON_FIRST_FAILURE=0
EXTDIR=`pg_config --sharedir`/extension/
CTBDIR=`pg_config --sharedir`/contrib/
TMPDIR=$(mktemp -d "${TMPDIR:-/tmp}/check_all_upgrades.XXXXXX") || exit 1
PGVER=`pg_config --version | awk '{print $2}'`
PGVER_MAJOR=$(echo "${PGVER}" | sed 's/\(alpha\|beta\|rc\).*//' | awk -F. '{ if ($1 == "9") print $1 "." $2; else print $1 }')
SKIP_LABEL_REGEXP=
SELFTEST=0
echo "INFO: PostgreSQL version: ${PGVER} [${PGVER_MAJOR}]"
MAKE=$(which gmake make | head -1)
BUILDDIR=$PWD # TODO: allow override ?

cd $(dirname $0)/..
SRCDIR=$PWD # TODO: allow override ?
cd - > /dev/null

# This is useful to run database queries
export PGDATABASE=template1

usage() {
  echo "Usage: $0 [-s] <to_version>"
  echo "Options:"
  echo "\t-s  Stop on first failure"
  echo "\t--skip <regexp>  Do not run tests with label matching given extended regexp"
  echo "\t--self-test  Run check_all_upgrades.sh unit tests"
}

while test -n "$1"; do
  if test "$1" = "-s"; then
    EXIT_ON_FIRST_FAILURE=1
  elif test "$1" = "--self-test"; then
    SELFTEST=1
  elif test "$1" = "--skip"; then
    shift
    SKIP_LABEL_REGEXP=$1
  elif test -z "$to_version_param"; then
    to_version_param="$1"
  else
    usage >&2
    exit 1
  fi
  shift
done

if test "${SELFTEST}" != 1 && test -z "$to_version_param"; then
  usage >&2
  exit 1
fi

cleanup()
{
  echo "Cleaning up"
  rm -rf "${TMPDIR}"
}

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
    v1_numeric=`echo "$v1" | sed 's/[^0-9].*//'`
    v2_numeric=`echo "$v2" | sed 's/[^0-9].*//'`
    if test -n "$v1_numeric" && test -n "$v2_numeric"; then
      if [ "$v1_numeric" -lt "$v2_numeric" ]; then
        echo -1; return;
      fi
      if [ "$v1_numeric" -gt "$v2_numeric" ]; then
        echo 1; return;
      fi
    fi
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
  if test $EXIT_ON_FIRST_FAILURE != 0; then
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
16:3.3
17:3.3
EOF
  fi

  # Drop patch-level number from PostgreSQL version
  minsupported=`grep ^${pgver}: ${supportfile} | cut -d: -f2`
  test -n "${minsupported}" || {
    echo "Cannot determine minimum supported PostGIS version for PostgreSQL major version ${pgver}" >&2
    exit 1
  }
  echo "${minsupported}"
}

kept_label()
{
  label=$1

  if test -n "${SKIP_LABEL_REGEXP}"; then
    if echo "${label}" | egrep -q "${SKIP_LABEL_REGEXP}"; then
      echo "SKIP: $label (matches regexp '${SKIP_LABEL_REGEXP}')"
      return 1;
    fi
  fi

  return 0;
}

# Usage: compatible_from <label> <from>
compatible_from()
{
  label=$1
  from=$2
  cmp=`semver_compare "${PGIS_MIN_VERSION}" "${from}"`
  if test $cmp -gt 0; then
    echo "SKIP: $label ($from older than ${PGIS_MIN_VERSION}, which is required to run in PostgreSQL ${PGVER})"
    return 1
  fi

  return 0
}

# Usage: compatible_upgrade <label> <from> <to>
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

  if compatible_from "$label" "$from"; then
    return 0
  else
    return 1
  fi
}

check_downgrade_error()
{
  log_file=$1

  ERR=$( grep 'ERROR:.*Downgrade .* forbidden' ${log_file} )
  test -n "$ERR" && {
    echo "PASS: ${test_label} gave $ERR"
    return 0
  }

  CREATE_EXT_ERR=$( grep 'Error encountered creating EXTENSION POSTGIS' ${log_file} )
  LOAD_ERR=$( grep 'ERROR:.*could not load library' ${log_file} )
  test -n "$CREATE_EXT_ERR" && test -n "$LOAD_ERR" && {
    # A downgrade cannot be checked until the source extension version
    # can be created. Later loader failures are part of the downgrade
    # attempt and must remain visible as failures.
    echo "SKIP: ${test_label} could not load source extension library:"
    echo "$LOAD_ERR"
    return 0
  }

  echo "FAIL: ${test_label} gave some other error:"
  tail ${log_file}
  return 1
}

run_check()
{
  if command -v unbuffer >/dev/null 2>&1; then
    unbuffer ${MAKE} -C "${REGDIR}" check "TESTS=${SRCDIR}/regress/core/regress.sql" ${MAKE_ARGS}
  else
    ${MAKE} -C "${REGDIR}" check "TESTS=${SRCDIR}/regress/core/regress.sql" ${MAKE_ARGS}
  fi
}

check_downgrade()
{
  RUNTESTFLAGS="-v --extension --upgrade-path=${UPGRADE_PATH} ${USERTESTFLAGS}" \
  run_check > "${TMPDIR}/log" 2>&1
  if test $? = 0; then
    echo "FAIL: ${test_label} did not error out:"
    tail "${TMPDIR}/log"
    failed
  else
    check_downgrade_error ${TMPDIR}/log || failed
  fi
}

report_missing_versions()
{
  if test -n "${MISSING_EXT_UPGRADES}"; then
    echo "INFO: missing upgrade scripts: ${MISSING_EXT_UPGRADES}"
    echo "HINT: use 'postgis install-extension-upgrades' to install them"
  fi
  cleanup
}

check_all_upgrades_selftest()
{
  test_label="self-test downgrade"
  test_tmp=${TMPDIR}-selftest
  mkdir -p ${test_tmp} || exit 1
  trap "rm -rf ${test_tmp}" EXIT

  # Keep self-test assertions explicit: POSIX sh does not make an ignored
  # pipeline status fail the surrounding function.
  check_selftest_output()
  {
    log_file=$1
    expected_pattern=$2
    output_file=$3

    if ! check_downgrade_error ${log_file} > ${output_file} 2>&1; then
      echo "FAIL: expected self-test classification to pass for ${log_file}" >&2
      cat ${output_file} >&2
      exit 1
    fi

    if ! grep -q "${expected_pattern}" ${output_file}; then
      echo "FAIL: expected self-test output matching ${expected_pattern}" >&2
      cat ${output_file} >&2
      exit 1
    fi
  }

  echo 'ERROR:  Downgrade 3.5.1dev to 3.4.7 forbidden' > ${test_tmp}/downgrade.log
  check_selftest_output \
    ${test_tmp}/downgrade.log \
    '^PASS: self-test downgrade gave ERROR:  Downgrade' \
    ${test_tmp}/downgrade.out

  {
    echo "Error encountered creating EXTENSION POSTGIS"
    echo 'ERROR:  could not load library "/tmp/postgis-3.5.so": undefined symbol: GEOSPreparedRelatePattern'
  } > ${test_tmp}/loader.log
  check_selftest_output \
    ${test_tmp}/loader.log \
    '^SKIP: self-test downgrade could not load source extension library:' \
    ${test_tmp}/loader.out

  {
    echo "Error encountered updating EXTENSION POSTGIS"
    echo 'ERROR:  could not load library "/tmp/postgis-3.5.so": undefined symbol: GEOSPreparedRelatePattern'
  } > ${test_tmp}/update-loader.log
  if check_downgrade_error ${test_tmp}/update-loader.log > ${test_tmp}/update-loader.out 2>&1; then
    echo "FAIL: loader errors after source creation should fail self-test" >&2
    exit 1
  fi
  grep -q '^FAIL: self-test downgrade gave some other error:' ${test_tmp}/update-loader.out

  echo 'ERROR:  unrelated failure' > ${test_tmp}/other.log
  if check_downgrade_error ${test_tmp}/other.log > ${test_tmp}/other.out 2>&1; then
    echo "FAIL: unrelated downgrade errors should fail self-test" >&2
    exit 1
  fi
  grep -q '^FAIL: self-test downgrade gave some other error:' ${test_tmp}/other.out

  if test "`semver_compare 3.2.7 3.2.11dev`" != "-1"; then
    echo "FAIL: expected 3.2.7 to compare older than 3.2.11dev" >&2
    exit 1
  fi

  if test "`semver_compare 3.2.11dev 3.2.7`" != "1"; then
    echo "FAIL: expected 3.2.11dev to compare newer than 3.2.7" >&2
    exit 1
  fi

  if test "`semver_compare 3.2.11dev 3.2.11dev`" != "0"; then
    echo "FAIL: expected identical dev versions to compare equal" >&2
    exit 1
  fi
}

if test "${SELFTEST}" = 1; then
  check_all_upgrades_selftest
  exit
fi

if test -z "$to_version_param"; then
  usage >&2
  exit 1
fi

mkdir -p ${TMPDIR}
trap 'cleanup' EXIT


to_version=$to_version_param
if expr $to_version : ':auto' >/dev/null; then
  to_version=`psql -XAtc "select default_version from pg_available_extensions where name = 'postgis'"` || exit 1
  MISSING_EXT_UPGRADES=
  trap report_missing_versions EXIT
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
if test -f postgis_sfcgal--${to_version}.sql; then
  INSTALLED_EXTENSIONS="$INSTALLED_EXTENSIONS postgis_sfcgal"
fi

echo "INFO: installed extensions: $INSTALLED_EXTENSIONS"

USERTESTFLAGS=${RUNTESTFLAGS}

# Make use of all public functions defined by source version when the
# branch carries that hook, and use double-upgrade.
if test -f "${SRCDIR}/regress/hooks/use-all-functions.sql"; then
  USERTESTFLAGS="\
    ${USERTESTFLAGS} \
    --before-upgrade-script ${SRCDIR}/regress/hooks/use-all-functions.sql \
  "
fi
USERTESTFLAGS="\
  ${USERTESTFLAGS} \
  --after-upgrade-script ${SRCDIR}/regress/hooks/hook-after-upgrade.sql \
"

for EXT in ${INSTALLED_EXTENSIONS}; do #{
  if test "${EXT}" = "postgis"; then
    REGDIR=${BUILDDIR}/regress
  elif test "${EXT}" = "postgis_topology"; then
    REGDIR=${BUILDDIR}/topology/test
  elif test "${EXT}" = "postgis_raster"; then
    REGDIR=${BUILDDIR}/raster/test/regress
  elif test "${EXT}" = "postgis_sfcgal"; then
    REGDIR=${BUILDDIR}/sfcgal/regress
  else
    echo "SKIP: don't know where to find regress tests for extension ${EXT}"
  fi

  # Check extension->extension upgrades
  files=`'ls' ${EXT}--* | grep -v -- '--.*--' | sed "s/^${EXT}--\(.*\)\.sql/\1/"`
  for fname in $files; do
    from_version="$fname"
    if test "$from_version" = "unpackaged"; then
      # We deal with unpackaged tests separately
      continue;
    fi
    UPGRADE_PATH="${from_version}--${to_version_param}"

    cmp=`semver_compare "${from_version}" "${to_version}"`
    #echo "INFO: semver_compare ${from} ${to} returned $cmp"
    if test $cmp -eq 0; then
      echo "SKIP: ${from_version} -> ${to_version} (we won't test same-version upgrade/downgrade here)"
      continue;
    fi

    if test $cmp -lt 0; then
      test_label="${EXT} extension upgrade ${UPGRADE_PATH}"
    else
      test_label="${EXT} extension downgrade ${UPGRADE_PATH}"
    fi

    if expr $to_version_param : ':auto' >/dev/null; then
      test_label="${test_label} ($to_version)"
    fi

    kept_label "${test_label}" || continue

    if test $cmp -gt 0; then
      echo "SKIP: ${test_label} (${to_version} is not newer than ${from_version})"
      continue
    fi

    path=$( psql -XAtc "
        SELECT path
        FROM pg_catalog.pg_extension_update_paths('${EXT}')
        WHERE source = '${from_version}'
        AND target = '${to_version}'
    " ) || exit 1
    if test -z "${path}"; then
      echo "SKIP: ${test_label} (no upgrade path from ${from_version} to ${to_version} known by postgresql)"
      MISSING_EXT_UPGRADES="${from_version} ${MISSING_EXT_UPGRADES}"
      continue
    fi

    compatible_from "${test_label}" ${from_version} || continue

    echo "Testing ${test_label}"

    if expr "${test_label}" : '^.*upgrade' > /dev/null; then
      RUNTESTFLAGS="-v --extension --upgrade-path=${UPGRADE_PATH} ${USERTESTFLAGS}" \
      ${MAKE} -C ${REGDIR} check ${MAKE_ARGS} && {
        echo "PASS: ${test_label}"
      } || {
        echo "FAIL: ${test_label}"
        failed
      }
    else
      check_downgrade

      test_label="${test_label} with standard-conforming-strings off"
      echo "Testing ${test_label}"
      USERTESTFLAGS="\
        ${USERTESTFLAGS} \
        --before-upgrade-script ${SRCDIR}/regress/hooks/standard-conforming-strings-off.sql \
      " check_downgrade
    fi

  done

  if ! kept_label "unpackaged"; then
    echo "SKIP: ${EXT} script-based upgrades (disabled by commandline)"
    continue;
  fi

  # Check unpackaged->extension upgrades
  for majmin in `'ls' -d ${CTBDIR}/postgis-* | sed 's/.*postgis-//'`; do
    UPGRADE_PATH="unpackaged${majmin}--${to_version_param}"
    test_label="${EXT} extension upgrade ${UPGRADE_PATH}"
    if expr $to_version_param : ':auto' >/dev/null; then
      test_label="${test_label} ($to_version)"
    fi
    kept_label "${test_label}" || continue
    compatible_upgrade "${test_label}" ${majmin} ${to_version} || continue
    path=$( psql -XAtc "
        SELECT path
        FROM pg_catalog.pg_extension_update_paths('${EXT}')
        WHERE source = 'unpackaged'
        AND target = '${to_version}'
    " ) || exit 1
    if test -z "${path}"; then
      echo "SKIP: ${test_label} (no upgrade path from unpackaged to ${to_version} known by postgresql)"
      continue
    fi
    echo "Testing ${test_label}"
    RUNTESTFLAGS="-v --extension --upgrade-path=${UPGRADE_PATH} ${USERTESTFLAGS}" \
    ${MAKE} -C ${REGDIR} check ${MAKE_ARGS} && {
      echo "PASS: ${test_label}"
    } || {
      echo "FAIL: ${test_label}"
      failed
    }
  done

  # Check unpackaged->unpackaged upgrades (if target version == current version)
#  CURRENTVERSION=`grep '^POSTGIS_' ${SRCDIR}/Version.config | cut -d= -f2 | paste -sd '.'`
  CURRENTVERSION=$(grep '^POSTGIS_' ${SRCDIR}/Version.config | cut -d= -f2 | tr '\n' '.' | sed 's/\.$//')

  if test "${to_version}" != "${CURRENTVERSION}"; then #{
    echo "SKIP: ${EXT} script-based upgrades (${to_version_param} [${to_version}] does not match built version ${CURRENTVERSION})"
    continue
  fi #}

  for majmin in `'ls' -d ${CTBDIR}/postgis-* | sed 's/.*postgis-//'`
  do #{
    UPGRADE_PATH="unpackaged${majmin}--:auto"
    test_label="${EXT} script soft upgrade ${UPGRADE_PATH}"
    if expr $to_version_param : ':auto' >/dev/null; then
      test_label="${test_label} ($to_version)"
    fi

    compatible_upgrade "${test_label}" ${majmin} ${to_version} || continue

    if kept_label "${test_label}"; then #{
      echo "Testing ${test_label}"
      RUNTESTFLAGS="-v --upgrade-path=${UPGRADE_PATH} ${USERTESTFLAGS}" \
      ${MAKE} -C ${REGDIR} check ${MAKE_ARGS} && {
        echo "PASS: ${test_label}"
      } || {
        echo "FAIL: ${test_label}"
        failed
      }
    fi #}

    test_label="${EXT} script hard upgrade ${UPGRADE_PATH}"
    if kept_label "${test_label}"; then #{
      echo "Testing ${test_label}"
      RUNTESTFLAGS="-v --dumprestore --upgrade-path=${UPGRADE_PATH} ${USERTESTFLAGS}" \
      ${MAKE} -C ${REGDIR} check ${MAKE_ARGS} && {
        echo "PASS: ${test_label}"
      } || {
        echo "FAIL: ${test_label}"
        failed
      }
    fi #}

  done #}

done #}

exit $failures
