\set VERBOSITY terse
set client_min_messages to ERROR;

BEGIN;

SELECT NULL FROM topology.CreateTopology('t0');
SELECT NULL FROM topology.CreateTopology('t1');

SELECT 't0', topology.TotalTopologySize('t0') = topology.TotalTopologySize('t1');

SELECT NULL FROM topology.TopoGeo_addPoint('t0', 'POINT(0 0)');

SELECT 't1', topology.TotalTopologySize('t0') > topology.TotalTopologySize('t1');

SELECT NULL FROM topology.TopoGeo_addLineString('t1', 'LINESTRING(0 0, 5 6)');

SELECT 't2', topology.TotalTopologySize('t0') < topology.TotalTopologySize('t1');

ROLLBACK;
