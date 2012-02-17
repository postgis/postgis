set client_min_messages to ERROR;

\i load_topology.sql


-- good one
SELECT 'T1', topology.ST_ChangeEdgeGeom('city_data', 25,
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

-- Test change in presence of edges sharing node (#1428)
SELECT 'T2', topology.ST_ChangeEdgeGeom('city_data', 5,
 'LINESTRING(41 40, 57 33)');

-- Change to edge crossing old self 
SELECT 'T3', topology.ST_ChangeEdgeGeom('city_data', 5,
 'LINESTRING(41 40, 49 40, 49 34, 57 33)');

-- Change a closed edge
SELECT 'T4', topology.ST_ChangeEdgeGeom('city_data', 26,
 'LINESTRING(4 31, 7 31, 4 34, 4 31)');

-- TODO: test reverse direction of closed edge

SELECT topology.DropTopology('city_data');
