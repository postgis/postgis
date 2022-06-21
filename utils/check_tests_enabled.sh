
#!/bin/sh

usage() {
  echo "Usage: $0 [<sourcedir>]"
}

TMPDIR=/tmp/postgis_check_tests_enabled_$$
mkdir -p ${TMPDIR}

cleanup() {
  #echo "Things are in ${TMPDIR}"
  rm -rf ${TMPDIR}
}

trap 'cleanup' EXIT

# Usage: check_enabled <path-to-test.mk> [<subdir-containing-tests>]
check_enabled() {
  mk=$1
  suffix=$2
  bd=`dirname ${mk}`/${suffix}

  #cho "MK file: ${mk}"
  #echo "Suffix: ${suffix}"
  #echo "Basedir: ${bd}"

  grep 'top_srcdir)' ${mk} |
      sed 's|.*top_srcdir)/||;s/ .*$//' |
      sed 's|\\||' |
      sed 's/\.sql$//' > ${TMPDIR}/enabled_tests

  #cat ${TMPDIR}/enabled_tests

  find ${bd} -name '*.sql' |
    grep -v '\-post.sql' | grep -v '\-pre.sql' |
    sed 's|\.select.sql$||' |
    sed 's|//|/|' |
    sed 's|\.sql$||' > ${TMPDIR}/available_tests

  #cat ${TMPDIR}/available_tests

  MISSING=`grep -vf ${TMPDIR}/enabled_tests ${TMPDIR}/available_tests`
  if test -n "${MISSING}"; then
    (
    echo "The following tests are available but not enabled in:"
    echo "- ${mk}:"
    echo "${MISSING}" | sed 's/^/  /'
    ) >&2
    return 1
  else
    echo "All tests enabled in ${mk}"
    return 0
  fi
}

### COMMAND LINE PARSING
RD= # Root source dir
while [ $# -gt 0 ]; do
  if [ "$1" = "--help" ]; then
    usage
    exit 0
  elif [ -z "${RD}" ]; then
    RD=$1
  else
    echo "ERROR: unrecognized extra argument $1" >&2
    usage >&2
    exit 1
  fi
  shift
done


if [ -z "${RD}" ]; then
  RD=`dirname $0`/..
fi

cd ${RD}
check_enabled topology/test/tests.mk regress &&
check_enabled regress/loader/tests.mk &&
check_enabled regress/dumper/tests.mk &&
check_enabled sfcgal/regress/tests.mk.in &&
check_enabled regress/core/tests.mk.in &&
check_enabled raster/test/regress/tests.mk
