#!/usr/bin/env bash
set -e

./autogen.sh
# Check that compilation works with all checks
# enabled
./configure --enable-debug --enable-assert
sed -i 's/POSTGIS_DEBUG_LEVEL [0-9]$/POSTGIS_DEBUG_LEVEL 4/' postgis_config.h
make -j
