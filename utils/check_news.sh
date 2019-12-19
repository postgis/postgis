#!/bin/sh

usage() {
  echo "Usage: $0 [-v] [--ticket-refs] [--ticket-refs-skip-commits=<file>] [<sourcedir>]"
  echo "Sourcedir defaults to one directory above this script"
}


### COMMAND LINE PARSING
VERBOSE=no
TICKET_REFS=no
RD= # Root source dir
TICKET_REFS_SKIP_COMMITS=/dev/null
while [ $# -gt 0 ]; do
  if [ "$1" = "--help" ]; then
    usage
    exit 0
  elif [ "$1" = "-v" ]; then
    VERBOSE=yes
  elif [ "$1" = "--ticket-refs" ]; then
    TICKET_REFS="yes"
  elif [ "$1" = "--ticket-refs-skip-commits" ]; then
    shift
    TICKET_REFS_SKIP_COMMITS=$1
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

# Extract dates from NEWS file and check they
# are sorted correctly
pdate=$(date '+%Y/%m/%d') # most recent timestamp
prel=Unreleased
grep -B1 '^[0-9]\{4\}/[0-9]\{2\}/[0-9]\{2\}' NEWS |
  while read rel; do
    read date
    read sep
    ts=$( date -d "${date}" '+%s' )
    pts=$( date -d "${pdate}" '+%s' )
    if test $ts -gt $pts; then
      echo "FAIL: ${rel} (${date}) appears after ${prel} (${pdate})" >&2
      exit 1
    fi
    if test "${VERBOSE}" = yes; then
      echo "INFO: ${rel} (${date}) before ${prel} (${pdate})"
    fi
    pts=${ts}
    prel=${rel}
  done
test $? = 0 || exit 1
echo "PASS: NEWS file entries are in good order"

if test "${TICKET_REFS}" = "yes"; then
  # If git is available, check that every ticket reference in
  # commit logs is also found in the NEWS file
  if which git > /dev/null && test -e .git; then
    git log --grep '#[0-9]\+' --pretty='format:%H%n%w(800,1,1)%B' |
      grep -v 'git-svn-id:' |
      awk 'NR > 1 && /^[^ ]/ { printf "\n" } 1 { printf "%s", $0 }' |
      grep -wvf $TICKET_REFS_SKIP_COMMITS |
      grep -i ' #[0-9]\+' |
      sed -En 's|#([0-9]+)|\a\1\n|;/\n/!b;s|.*\a||;P;D' |
      sort -nru |
    while read ref; do
      ref="#${ref}"
      if ! grep -qw "${ref}" NEWS; then
        echo "FAIL: Reference to commit-logged ticket ref ${ref} missing from NEWS" >&2
        exit 1
      else
        if test "${VERBOSE}" = yes; then
          echo "INFO: Reference to commit-logged ticket ref ${ref} found in NEWS"
        fi
      fi
    done
    test $? = 0 || exit 1
    echo "PASS: All ticket references in commits log found in NEWS"
  else
    echo "SKIP: GIT history cannot be checked (missing git or missing ${RD}/.git)"
  fi
fi

exit 0
