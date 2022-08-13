#!/usr/bin/env bash
set -e
## begin variables passed in by jenkins

export PG_VER=15
export PGPATH=/usr/lib/postgresql/${PG_VER}
export PATH=${PGPATH}/bin:${PATH}
export MAKE_GARDEN=0
export MAKE_EXTENSION=1
export DUMP_RESTORE=0
export MAKE_LOGBT=0
export NO_SFCGAL=0
export MAKE_UPGRADE=1


make check

## install so we can test upgrades/dump-restores etc.
sudo make install

if [ "$MAKE_EXTENSION" = "1" ]; then
 echo "Running extension testing"
 make check RUNTESTFLAGS="$RUNTESTFLAGS --extension"
 if [ "$?" != "0" ]; then
  exit $?
 fi
fi



if [ "$DUMP_RESTORE" = "1" ]; then
 echo "Dum restore test"
 make check RUNTESTFLAGS="$RUNTESTFLAGS --dumprestore"
 if [ "$?" != "0" ]; then
  exit $?
 fi
fi

if [ "$MAKE_GARDEN" = "1" ]; then
 echo "Running garden test"
 make garden
 if [ "$?" != "0" ]; then
  exit $?
 fi
fi

# Test all available upgrades
# TODO: protect via some variable ?
if [ "$MAKE_UPGRADE" = "1" ]; then
 utils/check_all_upgrades.sh \
        `grep '^POSTGIS_' Version.config | cut -d= -f2 | paste -sd '.'`
 if [ "$?" != "0" ]; then
  exit $?
 fi
fi
