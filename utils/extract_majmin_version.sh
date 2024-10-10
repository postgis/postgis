#!/bin/sh

grep '^POSTGIS_.*OR' Version.config |
  cut -d= -f2 |
  tr '\n' '.' |
  sed 's/\.$//'
