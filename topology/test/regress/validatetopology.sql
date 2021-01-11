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
