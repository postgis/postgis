set client_min_messages to ERROR;

\i :top_builddir/topology/test/load_topology.sql

-- Test bogus calls
SELECT 'non-existent-topo', topology.GetRingEdges('non-existent', 1);
SELECT 'non-existent-edge', topology.GetRingEdges('city_data', 10000000);
SELECT 'non-existent-edge-negative', topology.GetRingEdges('city_data', -10000000);

SELECT 'R'||edge_id, (topology.GetRingEdges('city_data', edge_id)).*
	FROM city_data.edge;

SELECT 'R-'||edge_id, (topology.GetRingEdges('city_data', -edge_id)).*
	FROM city_data.edge;

SELECT topology.DropTopology('city_data');
