#Berrie is a 32-bit Rasberry Pi managed by Bruce Rindahl
#This is script to launch custom compiled PostgreSQL
#export label=berrie #this is passed in via Jenkins
export WORKSPACE=/home/jenkins/workspace



export OS_BUILD=32
export PG_VER=12
export PGPATH=${WORKSPACE}/pg/label/${label}/rel/pg${PG_VER}w${OS_BUILD}
export PATH=${PATH}:${PGPATH}/bin:${PGPATH}/lib
export PGPORT=55432
export PGDATA=$PGPATH/data_${PGPORT}
export PGLOG="$PGDATA/pgsql.log"
# What to use to start up the postmaster
DAEMON="$PGPATH/bin/pg_ctl -D $PGDATA -o '-F' -l logfile start"

# What to use to shut down the postmaster
PGCTL="$PGPATH/bin/pg_ctl"

# remove cluster if exists
if [ -d $PGDATA ] ; then
    if [ -d $PGDATA/postmaster.pid ] ; then
    	$PGCTL stop -D $PGDATA -s -m fast
    fi;

    rm -rf $PGDATA
fi;

#initialize cluster
$PGPATH/bin/initdb

echo -n "Starting PostgreSQL: "
$DAEMON &
#sleep a bit because sometimes postgres takes a bit to start up
sleep 20
echo "ok"
exit 0
