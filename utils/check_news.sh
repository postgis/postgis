#!/bin/sh

RD=`dirname $0`/.. # root source dir

VERBOSE=no

if test "$1" = "-v"; then
  VERBOSE=yes
fi

# Extract dates from NEWS file and check they
# are sorted correctly
pdate=$(date '+%Y/%m/%d') # most recent timestamp
prel=Unreleased
grep -B1 '^[0-9]\{4\}/[0-9]\{2\}/[0-9]\{2\}' ${RD}/NEWS |
  while read rel; do
    read date
    read sep
    ts=$( date -d "${date}" '+%s' )
    pts=$( date -d "${pdate}" '+%s' )
    if test $ts -gt $pts; then
      echo "${rel} (${date}) appears after earlier published release ${prel}"
      exit 1
    fi
    if test "${VERBOSE}" = yes; then
      echo "${rel} (${date}) before ${prel} (${pdate})"
    fi
    pts=${ts}
    prel=${rel}
  done

echo "NEWS file entries are in good order"

exit 0
