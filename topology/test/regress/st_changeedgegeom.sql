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

-- Collisions on edge motion path is forbidden:
-- get to include a whole isolated edge
SELECT topology.ST_ChangeEdgeGeom('city_data', 26,
 'LINESTRING(4 31, 7 31, 15 34, 12 37.5, 4 34, 4 31)');

-- This movement doesn't collide:
SELECT 'T5', topology.ST_ChangeEdgeGeom('city_data', 3,
 'LINESTRING(25 30, 18 35, 18 39, 23 39, 23 36, 20 38, 19 37, 20 35, 25 35)');

-- This movement doesn't collide either:
SELECT 'T6', topology.ST_ChangeEdgeGeom('city_data', 3,
 'LINESTRING(25 30, 22 38, 25 35)');

-- This movement gets to include an isolated node:
SELECT topology.ST_ChangeEdgeGeom('city_data', 3,
 'LINESTRING(25 30, 18 35, 18 39, 23 39, 23 36, 20 35, 25 35)');

-- This movement is legit
SELECT 'T7', topology.ST_ChangeEdgeGeom('city_data', 2,
 'LINESTRING(25 30, 28 39, 16 39, 25 30)');

-- This movement gets to exclude an isolated node:
SELECT topology.ST_ChangeEdgeGeom('city_data', 2,
 'LINESTRING(25 30, 28 39, 20 39, 25 30)');

-- Test changing winding direction of closed edge
SELECT topology.ST_ChangeEdgeGeom('city_data', 26,
 ST_Reverse('LINESTRING(4 31, 7 31, 4 34, 4 31)'));

-- Maintain winding of closed edge 
SELECT 'T8', topology.ST_ChangeEdgeGeom('city_data', 26,
 'LINESTRING(4 31, 4 30.4, 5 30.4, 4 31)');

-- TODO: test moving closed edge into another face
-- TODO: test changing winding of non-closed edge ring
-- TODO: test face mbr update

SELECT topology.DropTopology('city_data');
