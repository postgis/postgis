#!/bin/sh

usage() {
  echo "Usage: $0 [<sourcedir>]"
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
test ${RD}/doc/po || {
  echo "ERROR: Source dir ${RD} does not contain a doc/po directory" >&2
  exit 1
}

ENABLED_TRANSLATIONS=$(grep ^translations ${RD}/doc/Makefile.in | cut -d= -f2)
fail=0

CORRECTLY_ENABLED=

# Available languages are those for which we have at least a .po file
for LANG in $(
  find ${RD}/doc/po/ -name '*.po' | xargs dirname | sort -u | xargs -n 1 basename
)
do

  # Check that the language dir has a Makefile.in
  test -f ${RD}/doc/po/${LANG}/Makefile.in || {
    echo "FAIL: ${RD}/doc/po/${LANG} is missing a Makefile.in"
    fail=$((fail+1))
    continue
  }

  # Check that the language dir is enabled in doc/Makefile.in
  echo "${ENABLED_TRANSLATIONS}" | grep -qw "${LANG}" || {
    echo "FAIL: ${LANG} is not enabled in ${RD}/doc/Makefile.in (translations)"
    fail=$((fail+1))
    continue
  }

  # Check that the language check is enabled for woodie
  grep -qw "${LANG}" ${RD}/.woodpecker/docs.yml || {
    echo "FAIL: ${LANG} is not enabled in ${RD}/.woodpecker/docs.yml"
    fail=$((fail+1))
    continue
  }

  echo "PASS: ${LANG} is correctly enabled"

done

exit $fail
