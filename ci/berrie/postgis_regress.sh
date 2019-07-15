#Berrie is a 32-bit Rasberry Pi managed by Bruce Rindahl
#BRANCH is passed in via jenkins which is set via gitea web hook
export WORKSPACE=~/workspace
export OS_BUILD=32
export PG_VER=12
export PGPATH=${WORKSPACE}/postgresql/rel/pg${PG_VER}w${OS_BUILD}
export PATH=${PATH}:${PGPATH}/bin:${PGPATH}/lib
export PGPORT=55432
export PGDATA=${WORKSPACE}/postgresql/rel/pg${PG_VER}w${OS_BUILD}/data_${PGPORT}
# What to use to start up the postmaster (we do NOT use pg_ctl for this,
# as it adds no value and can cause the postmaster to misrecognize a stale
# lock file)
DAEMON="$PGPATH/bin/postmaster"

# What to use to shut down the postmaster
PGCTL="$PDPATH/bin/pg_ctl"

# remove cluster if exists
if [ -d $PGDATA ] ; then
    rm $PGDATA
fi;

#initialize cluster
$PGPATH/bin/initdb

echo -n "Starting PostgreSQL: "
$DAEMON
sleep 5
echo "ok"

sh autogen.sh
./configure --with-pgconfig=${PGPATH}/bin/pg_config
make clean
make
export PGUSER=postgres
#export PGIS_REG_TMPDIR=tmp
psql -c "DROP DATABASE IF EXISTS postgis_reg;"
make check RUNTESTFLAGS="-v"
make install
make check RUNTESTFLAGS="-v --extension"

echo -n "Stopping PostgreSQL: "
$PGCTL stop -D $PGDATA -s -m fast
echo "ok"
;;