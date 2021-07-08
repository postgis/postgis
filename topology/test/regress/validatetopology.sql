\set VERBOSITY terse
set client_min_messages to ERROR;

-- TODO: merge legacy_validate.sql here

-- See ticket #1789
select null from ( select topology.CreateTopology('t') > 0 ) as ct;
COPY t.node (node_id, containing_face, geom) FROM stdin;
1	\N	01010000000000000000E065C002000000008056C0
2	\N	01010000000000000000E065C000000000008056C0
3	\N	010100000000000000009865C04FE5D4AD958655C0
\.
COPY t.edge_data (edge_id, start_node, end_node, next_left_edge, abs_next_left_edge, next_right_edge, abs_next_right_edge, left_face, right_face, geom) FROM stdin;
1	1	3	2	2	1	1	0	0	0102000000020000000000000000E065C002000000008056C000000000009865C04FE5D4AD958655C0
2	3	2	-2	2	-1	1	0	0	01020000000200000000000000009865C04FE5D4AD958655C00000000000E065C000000000008056C0
\.
SELECT '#1789', * FROM ValidateTopology('t') UNION
SELECT '#1789', '---', null, null ORDER BY 1,2,3,4;

SELECT '#1797', (ValidateTopology('t')).* UNION
SELECT '#1797', '---', null, null ORDER BY 1,2,3,4;

select null from ( select topology.DropTopology('t') ) as dt;

-- Test correctness of node.containing_face
-- See https://trac.osgeo.org/postgis/ticket/3233

SELECT null from ( select topology.CreateTopology('t') ) as ct;
SELECT null from ( select TopoGeo_addPolygon('t', 'POLYGON((0 0,10 0,10 10,0 10,0 0))') ) af;
SELECT null from ( select TopoGeo_addPoint('t', 'POINT(5 5)') ) ap;
SELECT '#3233.0', (ValidateTopology('t')).* UNION
SELECT '#3233.0', '---', null, null ORDER BY 1,2,3,4;
-- 1. Set wrong containing_face for isolated node
UPDATE t.node SET containing_face = 0 WHERE ST_Equals(geom, 'POINT(5 5)');
SELECT '#3233.1', (ValidateTopology('t')).* UNION
SELECT '#3233.1', '---', null, null ORDER BY 1,2,3,4;
-- 2. Set null containing_face for isolated node
UPDATE t.node SET containing_face = NULL WHERE ST_Equals(geom, 'POINT(5 5)');
SELECT '#3233.2', (ValidateTopology('t')).* UNION
SELECT '#3233.2', '---', null, null ORDER BY 1,2,3,4;
-- 3. Set containing_face for non-isolated node
UPDATE t.node SET containing_face = 1 WHERE ST_Equals(geom, 'POINT(5 5)');
UPDATE t.node SET containing_face = 0 WHERE NOT ST_Equals(geom, 'POINT(5 5)');
SELECT '#3233.3', (ValidateTopology('t')).* UNION
SELECT '#3233.3', '---', null, null ORDER BY 1,2,3,4;
SELECT null from ( select topology.DropTopology('t') ) as dt;

-- Test edges containment in their side faces
-- See https://trac.osgeo.org/postgis/ticket/4684

SELECT null from ( select topology.CreateTopology('t') ) as ct;
SELECT null from ( select TopoGeo_addPolygon('t',
  'POLYGON((0 0,10 0,10 10,0 10,0 0))' -- face 1
) ) af;
SELECT null from ( select TopoGeo_addPolygon('t',
  'POLYGON((20 0,30 0,30 10,20 10,20 0))' -- face 2
) ) af;
SELECT null from ( select TopoGeo_addLineString('t',
  'LINESTRING(5 5, 10 5)') -- edge inside faces, touching on endpoint
) al;
SELECT null from ( select TopoGeo_addLineString('t',
  'LINESTRING(10 6, 5 6)') -- edge inside faces, touching on startpoint
) al;
SELECT null from ( select TopoGeo_addLineString('t',
  'LINESTRING(10 5, 15 5)') -- edge outside faces, touching on startpoint
) al;
SELECT null from ( select TopoGeo_addLineString('t',
  'LINESTRING(15 6, 10 6)') -- edge outside faces, touching on endpoint
) al;
SELECT null from ( select TopoGeo_addLineString('t',
  'LINESTRING(5 8, 6 8)') -- edge inside face1, isolated
) al;
SELECT '#4830.0', (ValidateTopology('t')).* UNION
SELECT '#4830.0', '---', null, null ORDER BY 1,2,3,4;
-- 1. Set wrong left_face for dangling inside edges
BEGIN;
UPDATE t.edge_data SET left_face = 2
  WHERE
    ST_Equals(geom, 'LINESTRING(5 5, 10 5)') OR
    ST_Equals(geom, 'LINESTRING(10 6, 5 6)');
SELECT '#4830.1', (ValidateTopology('t')).* UNION
SELECT '#4830.1', '---', null, null ORDER BY 1,2,3,4;
ROLLBACK;
-- 2. Set wrong right_face for dangling inside edges
BEGIN;
UPDATE t.edge_data SET right_face = 2
  WHERE
    ST_Equals(geom, 'LINESTRING(5 5, 10 5)') OR
    ST_Equals(geom, 'LINESTRING(10 6, 5 6)');
SELECT '#4830.2', (ValidateTopology('t')).* UNION
SELECT '#4830.2', '---', null, null ORDER BY 1,2,3,4;
ROLLBACK;
-- 3. Set wrong left_face for dangling outside edges
BEGIN;
UPDATE t.edge_data SET left_face = 2
  WHERE
    ST_Equals(geom, 'LINESTRING(10 5, 15 5)') OR
    ST_Equals(geom, 'LINESTRING(15 6, 10 6)');
SELECT '#4830.3', (ValidateTopology('t')).* UNION
SELECT '#4830.3', '---', null, null ORDER BY 1,2,3,4;
ROLLBACK;
-- 4. Set wrong right_face for dangling outside edges
BEGIN;
UPDATE t.edge_data SET right_face = 2
  WHERE
    ST_Equals(geom, 'LINESTRING(10 5, 15 5)') OR
    ST_Equals(geom, 'LINESTRING(15 6, 10 6)');
SELECT '#4830.4', (ValidateTopology('t')).* UNION
SELECT '#4830.4', '---', null, null ORDER BY 1,2,3,4;
ROLLBACK;
-- 5. Set universal face on both sides of internal edge
BEGIN;
UPDATE t.edge_data SET left_face = 0, right_face = 0
  WHERE
    ST_Equals(geom, 'LINESTRING(5 8, 6 8)');
SELECT '#4830.5', (ValidateTopology('t')).* UNION
SELECT '#4830.5', '---', null, null ORDER BY 1,2,3,4;
ROLLBACK;
SELECT null from ( select topology.DropTopology('t') ) as dt;

-------------------------------------------------------------
-- Following tests will use the city_data topology as a base
-------------------------------------------------------------

\i ../load_topology.sql
SELECT 'unexpected_city_data_invalidities', * FROM ValidateTopology('city_data');

-- Test correctness of edge linking
-- See https://trac.osgeo.org/postgis/ticket/3042
--set client_min_messages to NOTICE;
BEGIN;
-- Break edge linking for all edges around node 14
UPDATE city_data.edge_data SET next_left_edge = -next_left_edge where edge_id in (9,10,20);
UPDATE city_data.edge_data SET next_right_edge = -next_right_edge where edge_id = 19;
-- Break edge linking of dangling edges, including one around last node (3)
UPDATE city_data.edge_data
SET
  next_left_edge = -next_left_edge,
  next_right_edge = -next_right_edge
where edge_id in (3,25);
set client_min_messages to WARNING;
SELECT '#3042', * FROM ValidateTopology('city_data')
UNION
SELECT '#3042', '---', null, null
ORDER BY 1,2,3,4;
ROLLBACK;

-- Test correctness of side-labeling
-- See https://trac.osgeo.org/postgis/ticket/4944
BEGIN;
-- Swap side-label of face-binding edge
UPDATE city_data.edge_data
  SET left_face = right_face, right_face = left_face
  WHERE edge_id = 19;
--set client_min_messages to DEBUG;
SELECT '#4944', * FROM ValidateTopology('city_data')
UNION
SELECT '#4944', '---', null, null
ORDER BY 1,2,3,4;
ROLLBACK;

SELECT NULL FROM topology.DropTopology('city_data');

