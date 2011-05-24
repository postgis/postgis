set client_min_messages to ERROR;

\i load_topology.sql


-- good one
SELECT topology.ST_ChangeEdgeGeom('city_data', 25,
 'LINESTRING(9 35, 11 33, 13 35)');

-- start/end points mismatch
SELECT topology.ST_ChangeEdgeGeom('city_data', 25,
 'LINESTRING(10 35, 13 35)');
SELECT topology.ST_ChangeEdgeGeom('city_data', 25,
 'LINESTRING(9 35, 13 36)');

-- Node crossing 
SELECT topology.ST_ChangeEdgeGeom('city_data', 3,
  'LINESTRING(25 30, 20 36, 20 38, 25 35)');

-- Non-existent edge (#979)
SELECT topology.ST_ChangeEdgeGeom('city_data', 666,
  'LINESTRING(25 30, 20 36, 20 38, 25 35)');

-- Test edge crossing
SELECT topology.ST_ChangeEdgeGeom('city_data', 25,
 'LINESTRING(9 35, 11 40, 13 35)');

-- TODO: test changing closed edge
-- TODO: test reverse direction of closed edge

SELECT topology.DropTopology('city_data');
