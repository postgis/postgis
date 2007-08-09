#!/bin/sh

FIRST=""
for file in /data/tiger/www2.census.gov/geo/tiger/tiger2006se/*/*.ZIP; do
  ./tigerimport.sh append mydb $file $FIRST
  if [ -z "$FIRST" ]; then
    FIRST="-a"
  fi
done
