\set VERBOSITY terse
set client_min_messages to ERROR;

-- Import city_data
\i load_topology.sql

SELECT topology.ST_ModEdgeHeal('city_data', 1, null);
SELECT topology.ST_ModEdgeHeal('city_data', null, 1);
SELECT topology.ST_ModEdgeHeal(null, 1, 2);
SELECT topology.ST_ModEdgeHeal('', 1, 2);
SELECT topology.ST_ModEdgeHeal('  ', 1, 2);
SELECT topology.ST_ModEdgeHeal('public', 1, 2);

-- Not connected edges
SELECT topology.ST_ModEdgeHeal('city_data', 25, 3);

-- Other connected edges
SELECT topology.ST_ModEdgeHeal('city_data', 9, 10);

-- Closed edge
SELECT topology.ST_ModEdgeHeal('city_data', 2, 3);
SELECT topology.ST_ModEdgeHeal('city_data', 3, 2);

-- Heal to self 
SELECT topology.ST_ModEdgeHeal('city_data', 25, 25);

-- Good ones {

-- check state before 
SELECT 'E'||edge_id,
  ST_AsText(ST_StartPoint(geom)), ST_AsText(ST_EndPoint(geom)),
  next_left_edge, next_right_edge, start_node, end_node
  FROM city_data.edge_data ORDER BY edge_id;
SELECT 'N'||node_id FROM city_data.node;

-- No other edges involved, SQL/MM caseno 2, drops node 6
SELECT 'MH(4,5)', topology.ST_ModEdgeHeal('city_data', 4, 5);

-- Face and other edges involved, SQL/MM caseno 1, drops node 16
SELECT 'MH(21,6)', topology.ST_ModEdgeHeal('city_data', 21, 6);
-- Face and other edges involved, SQL/MM caseno 2, drops node 19
SELECT 'MH(8,15)', topology.ST_ModEdgeHeal('city_data', 8, 15);
-- Face and other edges involved, SQL/MM caseno 3, drops node 8
SELECT 'MH(12,22)', topology.ST_ModEdgeHeal('city_data', 12, 22);
-- Face and other edges involved, SQL/MM caseno 4, drops node 11
SELECT 'MH(16,14)', topology.ST_ModEdgeHeal('city_data', 16, 14);

-- check state after
SELECT 'E'||edge_id,
  ST_AsText(ST_StartPoint(geom)), ST_AsText(ST_EndPoint(geom)),
  next_left_edge, next_right_edge, start_node, end_node
  FROM city_data.edge_data ORDER BY edge_id;
SELECT 'N'||node_id FROM city_data.node;

-- }

-- clean up
SELECT topology.DropTopology('city_data');

-- TODO: add TopoGeometry tests !
