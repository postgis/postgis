set client_min_messages to ERROR;

\i load_topology.sql


-- Endpoint / node mismatch
SELECT topology.ST_AddEdgeNewFaces('city_data', 7, 6,
 'LINESTRING(36 38,57 33)');
SELECT topology.ST_AddEdgeNewFaces('city_data', 5, 7,
 'LINESTRING(36 38,57 33)');

-- Crosses a node
SELECT topology.ST_AddEdgeNewFaces('city_data', 5, 6,
 'LINESTRING(36 38, 41 40, 57 33)');

-- Non-existent node 
SELECT topology.ST_AddEdgeNewFaces('city_data', 5, 60000,
 'LINESTRING(36 38,57 33)');
SELECT topology.ST_AddEdgeNewFaces('city_data', 60000, 6,
 'LINESTRING(36 38,57 33)');

-- Non-simple curve
SELECT topology.ST_AddEdgeNewFaces('city_data', 5, 5,
 'LINESTRING(36 38, 40 50, 36 38)');

-- Coincident edge
SELECT topology.ST_AddEdgeNewFaces('city_data', 18, 19,
 'LINESTRING(35 22,47 22)');

-- Crosses an edge
SELECT topology.ST_AddEdgeNewFaces('city_data', 5, 6,
 'LINESTRING(36 38, 40 50, 57 33)');

-- Touches an existing edge
SELECT 'O', topology.ST_AddEdgeNewFaces('city_data', 5, 6,
 'LINESTRING(36 38,45 32,57 33)');

-- Shares a portion of an existing edge
SELECT 'O', topology.ST_AddEdgeNewFaces('city_data', 5, 6,
 'LINESTRING(36 38,38 35,57 33)');

-- TODO: check succeeding ones...

SELECT topology.DropTopology('city_data');
