#!/usr/bin/env bash
set -e

/usr/local/pgsql/bin/pg_ctl -l /tmp/logfile start
./autogen.sh
./configure
make
make check
