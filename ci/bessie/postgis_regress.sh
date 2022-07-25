#!/usr/bin/env bash

set -e

#BRANCH is passed in via jenkins which is set via webhook hook
export PATH=${PATH}:/usr/local:/usr/local/lib:/usr/local/bin
sh autogen.sh
./configure --with-projdir=/usr/local --with-libiconv=/usr/local --without-interrupt-tests --with-library-minor-version
#make distclean
make
export PGUSER=postgres
export PGIS_REG_TMPDIR=~/tmp/pgis_reg_${BRANCH}
psql -c "DROP DATABASE IF EXISTS postgis_reg;"
make check RUNTESTFLAGS="-v"
sudo make install
make check RUNTESTFLAGS="-v --extension"
