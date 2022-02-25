#!/bin/sh

# TODO: turn to perl ?

KEEPTOPO=0

if test "$1" = "--nodrop"; then
  KEEPTOPO=1
fi

export PGOPTIONS='-c client_min_messages=WARNING';
PSQL_OPTS="-tA"

cleanup() {
  echo 'Cleaning up'
  if test "$KEEPTOPO" = 0; then
    psql ${PSQL_OPTS} -c "SELECT NULL FROM DropTopology('mt_topo');" | grep -v '^$'
  fi
  rm -f pids.$$
  rm -f invalidities_found
}

trap cleanup EXIT

#psql ${PSQL_OPTS} -c "SELECT NULL FROM DropTopology('mt_topo');" | grep -v '^$'
psql ${PSQL_OPTS} -c "SELECT NULL FROM CreateTopology('mt_topo')" > /dev/null || exit 1

# $RANDOM: random integer between 0 and 32767
for x in `seq 0 10 40`; do
  for y in `seq 0 10 40`; do

      # Add polygon
      cat <<EOF | psql ${PSQL_OPTS} | grep -v '^$' &
SELECT NULL FROM
    topology.TopoGeo_addPolygon(
      'mt_topo',
      ST_Buffer(
        ST_MakePoint(
          ${x},
          ${y}
        ),
        8
      )
    );
EOF
      echo $! >> pids.$$

      # Add horizontal edge
      cat <<EOF | psql ${PSQL_OPTS} | grep -v '^$' &
SELECT NULL FROM
    topology.TopoGeo_addLineString(
      'mt_topo',
       ST_MakeLine(
         ST_MakePoint(
            ${x} - 6,
            ${y} + 1
         ),
         ST_MakePoint(
            ${x} + 6,
            ${y} + 1
         )
      )
    );
EOF
      echo $! >> pids.$$

      # Add isolated node in the middle of the polygon added before
      cat <<EOF | psql ${PSQL_OPTS} | grep -v '^$' &
SELECT NULL FROM
    topology.TopoGeo_addPoint(
      'mt_topo',
       ST_MakePoint(
          ${x},
          ${y}
       )
    );
EOF
      echo $! >> pids.$$
  done
done

# wait for all pids
for pid in `cat pids.$$`; do
    wait $pid
done

psql ${PSQL_OPTS} -c "SELECT TopologySummary('mt_topo')" | grep -v '^$'
echo " -- INVALIDITIES START --"
psql ${PSQL_OPTS} -c "SELECT 'INVALIDITY', * FROM ValidateTopology('mt_topo')" | grep -v '^$' | tee invalidities_found
echo " -- INVALIDITIES END --"
test $(cat invalidities_found | wc -l) = 0
