#!/usr/bin/env bash
set -e

./autogen.sh
# Check that compilation works at nonzero POSTGIS_DEBUG_LEVEL
./configure --enable-debug # sets PARANOIA_LEVEL
sed -i 's/POSTGIS_DEBUG_LEVEL [0-9]$/POSTGIS_DEBUG_LEVEL 4/' postgis_config.h
make -j
