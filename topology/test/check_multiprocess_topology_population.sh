#!/bin/sh

# TODO: turn to perl ?

export PGOPTIONS='-c client_min_messages=WARNING';
PSQL_OPTS="-tA"

cleanup() {
  #echo 'Cleaning up'
  psql ${PSQL_OPTS} -c "SELECT NULL FROM DropTopology('mt_topo');" | grep -v '^$'
}

trap cleanup EXIT

psql ${PSQL_OPTS} -c "SELECT NULL FROM CreateTopology('mt_topo')" > /dev/null

# $RANDOM: random integer between 0 and 32767
for x in `seq 1 4`; do
  for y in `seq 1 4`; do
    cat <<EOF | psql ${PSQL_OPTS} | grep -v '^$' &
SELECT NULL FROM
  topology.TopoGeo_addPolygon(
    'mt_topo',
    ST_Buffer(
      ST_MakePoint(
        ${x} ,
        ${y}
      ),
      10000
    )
  )
EOF
    echo $! >> pids.$$
  done
done

# wait for all pids
for pid in `cat pids.$$`; do
    wait $pid
done
rm -f pids.$$

psql ${PSQL_OPTS} -c "SELECT TopologySummary('mt_topo')" | grep -v '^$'
psql ${PSQL_OPTS} -c "SELECT 'INVALIDITY', * FROM ValidateTopology('mt_topo')" | grep -v '^$'

