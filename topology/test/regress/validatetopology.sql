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

-------------------------------------------------------------
-- Following tests will use the city_data topology as a base
-------------------------------------------------------------

\i :top_builddir/topology/test/load_topology.sql

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

-- Test face with multiple shells is caught
-- See https://trac.osgeo.org/postgis/ticket/4945
BEGIN;
UPDATE city_data.edge_data SET right_face = 3 WHERE edge_id IN (8, 17);
UPDATE city_data.edge_data SET left_face = 3 WHERE edge_id IN (11, 15);
-- To reduce the noise,
-- set face 3 mbr to include the mbr of face 5
-- and delete face 5
UPDATE city_data.face SET mbr = (
  SELECT ST_Envelope(ST_Collect(mbr))
  FROM city_data.face
  WHERE face_id IN ( 3, 5 )
) WHERE face_id = 3;
DELETE FROM city_data.face WHERE face_id = 5;
SELECT '#4945', * FROM ValidateTopology('city_data')
UNION
SELECT '#4945', '---', null, null
ORDER BY 1,2,3,4;
ROLLBACK;

-- Test outer face labeling
-- See https://trac.osgeo.org/postgis/ticket/4684

-- 1. Set wrong outer face for isolated inside edges
BEGIN;
UPDATE city_data.edge_data SET left_face = 2, right_face = 2
  WHERE edge_id = 25;
SELECT '#4830.1', (ValidateTopology('city_data')).* UNION
SELECT '#4830.1', '---', null, null ORDER BY 1,2,3,4;
ROLLBACK;
-- 2. Set wrong outer face for isolated outside edge
BEGIN;
UPDATE city_data.edge_data SET left_face = 2, right_face = 2
  WHERE edge_id IN (4, 5);
SELECT '#4830.2', (ValidateTopology('city_data')).* UNION
SELECT '#4830.2', '---', null, null ORDER BY 1,2,3,4;
ROLLBACK;
-- 3. Set wrong outer face for face hole
BEGIN;
UPDATE city_data.edge_data SET right_face = 9
  WHERE edge_id = 26;
SELECT '#4830.3', (ValidateTopology('city_data')).* UNION
SELECT '#4830.3', '---', null, null ORDER BY 1,2,3,4;
ROLLBACK;
-- 4. Set universal outer face for isolated edge inside a face
BEGIN;
UPDATE city_data.edge_data SET left_face = 0, right_face = 0
  WHERE edge_id = 25;
SELECT '#4830.4', (ValidateTopology('city_data')).* UNION
SELECT '#4830.4', '---', null, null ORDER BY 1,2,3,4;
ROLLBACK;
-- 5. Set universal outer face for face hole
BEGIN;
UPDATE city_data.edge_data SET right_face = 0
  WHERE edge_id = 26;
SELECT '#4830.5', (ValidateTopology('city_data')).* UNION
SELECT '#4830.5', '---', null, null ORDER BY 1,2,3,4;
ROLLBACK;

-- Test ability to call twice in a transaction
-- in presence of mixed face labeling
-- See https://trac.osgeo.org/postgis/ticket/5017
BEGIN;
SELECT '#5017.0', (ValidateTopology('city_data'));
SELECT '#5017.1', (ValidateTopology('city_data'));
update city_data.edge_data SET left_face = 8 WHERE edge_id = 10;
SELECT '#5017.2', (ValidateTopology('city_data')).error;
SELECT '#5017.3', (ValidateTopology('city_data')).error;
ROLLBACK;

-- Test dangling edgerings are never considered shells
-- See https://trac.osgeo.org/postgis/ticket/5105
BEGIN;
SELECT NULL FROM CreateTopology('t5105');
SELECT '#5105.0', TopoGeo_addLineString('t5105', '
LINESTRING(
  29.262792863298348 71.22115103790775,
  29.26598031986849  71.22202978558047,
  29.275379947735576 71.22044935739267,
  29.29461024331857  71.22741507590429,
  29.275379947735576 71.22044935739267,
  29.26598031986849  71.22202978558047,
  29.262792863298348 71.22115103790775
)');
SELECT '#5105.1', TopoGeo_addLineString(
  't5105',
  ST_Translate(geom, 0, -2)
)
FROM t5105.edge WHERE edge_id = 1;
SELECT '#5105.edges_count', count(*) FROM t5105.edge;
SELECT '#5105.faces_count', count(*) FROM t5105.face WHERE face_id > 0;
SELECT '#5105.unexpected_invalidities', * FROM ValidateTopology('t5105');
-- TODO: add some areas to the endpoints of the dangling edges above
--       to form O-O figures
ROLLBACK;

SELECT NULL FROM topology.DropTopology('city_data');

