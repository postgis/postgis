#Berrie is a 32-bit Rasberry Pi managed by Bruce Rindahl
#BRANCH is passed in via jenkins which is set via gitea web hook
sh autogen.sh
./configure
make clean
make
export PGUSER=postgres
export PGIS_REG_TMPDIR=~/tmp/pgis_reg_${BRANCH}
psql -c "DROP DATABASE IF EXISTS postgis_reg;"
make check RUNTESTFLAGS="-v"
#make install
#make check RUNTESTFLAGS="-v --extension"