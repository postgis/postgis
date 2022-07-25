#!/usr/bin/env bash

set -e

#export PATH=${PATH}:/usr/local:/usr/local/lib:/usr/local/bin
export CC=gcc11
export CXX=g++11
export CXXFLAGS="-O2 -pipe  -fstack-protector-strong -Wl,-rpath=/usr/local/lib/gcc11  -nostdinc++ -isystem /usr/include/c++/v1 -Wl,-rpath=/usr/local/lib/gcc11"
export CFLAGS="-Wall -Wmissing-prototypes -Wpointer-arith -Wendif-labels -Wmissing-format-attribute -Wformat-security -fno-strict-aliasing -fwrapv -fexcess-precision=standard -Wno-format-trunc"
export PATH=${PATH}:/usr/local:/usr/local/lib:/usr/local/bin
sh autogen.sh
./configure PKG_CONFIG=/usr/local/bin/pkgconf LDFLAGS="-L/usr/local/lib" --with-libiconv-prefix=/usr/local --without-gui --without-interrupt-tests --with-topology --with-raster --with-sfcgal=/usr/local/bin/sfcgal-config --with-address-standardizer --with-protobuf --with-wagyu

#make distclean
make
export PGUSER=postgres
export PGIS_REG_TMPDIR=~/tmp/pgis_reg_${BRANCH}
psql -c "DROP DATABASE IF EXISTS postgis_reg;"
make check RUNTESTFLAGS="-v"
sudo make install
make check RUNTESTFLAGS="-v --extension"
