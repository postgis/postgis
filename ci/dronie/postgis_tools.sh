 #!/usr/bin/env bash

# Exit on first error
set -e

sh autogen.sh
./configure --without-pgconfig --prefix=/tmp/pgx
make
make check
make install
/tmp/pgx/bin/postgis help
/tmp/pgx/bin/shp2pgsql
/tmp/pgx/bin/raster2pgsql
