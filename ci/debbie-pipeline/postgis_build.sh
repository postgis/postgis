export PG_VER=15
export PGPATH=/usr/lib/postgresql/${PG_VER}
export PATH=${PGPATH}/bin:${PATH}
sh autogen.sh
./configure
make
