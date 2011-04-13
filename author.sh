#!/bin/sh

if test x"$1" = "x"; then
  echo "Usage: $0 <nick>" >&2
  exit 1
fi

authors_file="`dirname $0`/authors.svn"

if test ! -e "${authors_file}"; then
  echo "Authors file ${authors_file} does not exist" >&2
  exit 1
fi

nick="$1"
grep  "^${nick}" "${authors_file}" | cut -d: -f2
