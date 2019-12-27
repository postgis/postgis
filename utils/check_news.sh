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
    TICKET_REFS_SKIP_COMMITS=$( cd $( dirname $1 ) && pwd )
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
    ts=$( echo "${date}" | tr -d '/' )
    pts=$( echo "${pdate}" | tr -d '/' )
    if test $ts -gt $pts; then
      echo "FAIL: ${rel} (${date}) appears after ${prel} (${pdate})"
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

  last_news_release=$( grep \
    -B1 '^[0-9]\{4\}/[0-9]\{2\}/[0-9]\{2\}' NEWS |
    head -1 | awk '{print $2}' )
  echo "INFO: Checking ticket refs in commits since tag '$last_news_release'"

  # If git is available, check that every ticket reference in
  # commit logs is also found in the NEWS file
  if which git > /dev/null && test -e .git; then
    git rev-list HEAD --not $last_news_release |
      grep -vwFf $TICKET_REFS_SKIP_COMMITS |
      xargs git log --no-walk --pretty='format:%B' |
      sed -En 's|#([0-9]+)|\a\1\n|;/\n/!b;s|.*\a||;P;D' |
      sort -nru |
    { failures=0; while read ref; do
      ref="#${ref}"
      if ! grep -qw "${ref}" NEWS; then
        # See if it is found in any stable branch older than this one
        # and hint if found
        foundin=
        for b in $(
            git branch --list stable-* |
            sort -n | sed '/^\*/q' | tac |
            grep -v '\*'
            )
        do
          if git show $b:NEWS |
              grep -qw "${ref}"
          then
            foundin=$b
            break
          fi
        done
        if test -n "$foundin"; then
          # Check if the match is in a published version
          if git show $foundin:NEWS |
              sed '/^[0-9]\{4\}\/[0-9]\{2\}\/[0-9]\{2\}/q' |
              grep -qw "${ref}"
          then
            # The match is in a still unpublished section, this should
            # not be considered an error in development, but should be
            # fixed before closing the releasing.
            echo "WARN: Ticket ${ref} missing from NEWS (but found in dev section of branch $foundin)"
          else
            echo "FAIL: Ticket ${ref} missing from NEWS (and found in release from branch $foundin!)"
          fi
        else
          echo "FAIL: Ticket ${ref} MISSING from NEWS (here and in older branches)"
        fi
        failures=$((failures+1))
        #exit 1
      else
        if test "${VERBOSE}" = yes; then
          echo "INFO: Ticket ${ref} found in NEWS"
        fi
      fi
    done; exit $failures; }
    test $? = 0 || exit $?
    echo "PASS: All ticket references in commits log found in NEWS"
  else
    echo "SKIP: GIT history cannot be checked (missing git or missing ${RD}/.git)"
  fi
fi

exit 0
