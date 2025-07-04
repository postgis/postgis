SET client_min_messages TO NOTICE;
SELECT NULL FROM topology.CreateTopology('topo');
SELECT topology.MakeTopologyPrecise('topo');
UPDATE topology.topology SET precision = 5 WHERE name = 'topo';
SELECT topology.MakeTopologyPrecise('topo');
SELECT NULL FROM topology.TopoGeo_addLineString('topo', 'LINESTRING(123456789 0.1, 6 -1002003004)');
SELECT topology.MakeTopologyPrecise('topo', gridSize => 1e-10);
SELECT 'start', 'e', edge_id, ST_AsEWKT(geom) FROM topo.edge ORDER BY edge_id;
SELECT topology.MakeTopologyPrecise('topo');
SELECT 'prec5', 'e', edge_id, ST_AsEWKT(geom) FROM topo.edge ORDER BY edge_id;

SET client_min_messages TO WARNING;

-- TODO: test bbox limited made-precise topology

-- Cleanup

SELECT NULL FROM topology.DropTopology('topo');
