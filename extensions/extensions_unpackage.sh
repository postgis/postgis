#!/bin/sh

test -n "$1" || {
  echo "Usage: $0 <dbname>"
  exit 1
}

db="$1"
psql -XtA -f $(dirname $0)/extensions_unpackage.sql ${db}
