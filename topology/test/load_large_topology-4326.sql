--
-- From examples in chapter 1.12.1 of
-- "Spatial Topology and Network Data Models" (Oracle manual)
--
-- Modified to use postgis-based topology model.
-- Loads the whole topology represented in Figure 1-1 of the
-- manual
--

--ORA---------------------------------------------------------------------
--ORA---- Main steps for using the topology data model with a topology
--ORA---- built from edge, node, and face data
--ORA---------------------------------------------------------------------
--ORA---- 1. Create a topology.
--ORA---- 2. Load (normally bulk-load) topology data
--ORA----    (node, edge, and face tables).

BEGIN;

-- 1. Create the topology.
--
-- NOTE:
--  Returns topology id... which depend on how many
--  topologies where created in the regress database
--  so we just check it is a number greater than 0
--
SELECT NULL FROM topology.CreateTopology('large_city_data', 4326, 0, false, 0, true);

-- 2. Load topology data (node, edge, and face tables).
-- Use INSERT statements here instead of a bulk-load utility.

-- 2A. Insert data into <topology_name>.FACE table.

INSERT INTO large_city_data.face(face_id) VALUES(1000000000001); -- F1
INSERT INTO large_city_data.face(face_id) VALUES(1000000000002); -- F2
INSERT INTO large_city_data.face(face_id) VALUES(1000000000003); -- F3
INSERT INTO large_city_data.face(face_id) VALUES(1000000000004); -- F4
INSERT INTO large_city_data.face(face_id) VALUES(1000000000005); -- F5
INSERT INTO large_city_data.face(face_id) VALUES(1000000000006); -- F6
INSERT INTO large_city_data.face(face_id) VALUES(1000000000007); -- F7
INSERT INTO large_city_data.face(face_id) VALUES(1000000000008); -- F8
INSERT INTO large_city_data.face(face_id) VALUES(1000000000009); -- F9

-- UPDATE Face id sequence
SELECT NULL FROM setval('large_city_data.face_face_id_seq', 1000000000009);

-- 2B. Insert data into <topology_name>.NODE table.
-- N1
INSERT INTO large_city_data.node(node_id, geom, containing_face)
	VALUES(1000000000001, 'SRID=4326;POINT(8 30)', NULL);
-- N2
INSERT INTO large_city_data.node(node_id, geom, containing_face)
	VALUES(1000000000002, 'SRID=4326;POINT(25 30)', NULL);
-- N3
INSERT INTO large_city_data.node(node_id, geom, containing_face)
	VALUES(1000000000003, 'SRID=4326;POINT(25 35)', NULL);
-- N4
INSERT INTO large_city_data.node(node_id, geom, containing_face)
	VALUES(1000000000004, 'SRID=4326;POINT(20 37)', 1000000000002);
-- N5
INSERT INTO large_city_data.node(node_id, geom, containing_face)
	VALUES(1000000000005, 'SRID=4326;POINT(36 38)', NULL);
-- N6
INSERT INTO large_city_data.node(node_id, geom, containing_face)
	VALUES(1000000000006, 'SRID=4326;POINT(57 33)', NULL);
-- N7
INSERT INTO large_city_data.node(node_id, geom, containing_face)
	VALUES(1000000000007, 'SRID=4326;POINT(41 40)', NULL);
-- N8
INSERT INTO large_city_data.node(node_id, geom, containing_face)
	VALUES(1000000000008, 'SRID=4326;POINT(9 6)', NULL);
-- N9
INSERT INTO large_city_data.node(node_id, geom, containing_face)
	VALUES(1000000000009, 'SRID=4326;POINT(21 6)', NULL);
-- N10
INSERT INTO large_city_data.node(node_id, geom, containing_face)
	VALUES(1000000000010, 'SRID=4326;POINT(35 6)', NULL);
-- N11
INSERT INTO large_city_data.node(node_id, geom, containing_face)
	VALUES(1000000000011, 'SRID=4326;POINT(47 6)', NULL);
-- N12
INSERT INTO large_city_data.node(node_id, geom, containing_face)
	VALUES(1000000000012, 'SRID=4326;POINT(47 14)', NULL);
-- N13
INSERT INTO large_city_data.node(node_id, geom, containing_face)
	VALUES(1000000000013, 'SRID=4326;POINT(35 14)', NULL);
-- N14
INSERT INTO large_city_data.node(node_id, geom, containing_face)
	VALUES(1000000000014, 'SRID=4326;POINT(21 14)', NULL);
-- N15
INSERT INTO large_city_data.node(node_id, geom, containing_face)
	VALUES(1000000000015, 'SRID=4326;POINT(9 14)', NULL);
-- N16
INSERT INTO large_city_data.node(node_id, geom, containing_face)
	VALUES(1000000000016, 'SRID=4326;POINT(9 22)', NULL);
-- N17
INSERT INTO large_city_data.node(node_id, geom, containing_face)
	VALUES(1000000000017, 'SRID=4326;POINT(21 22)', NULL);
-- N18
INSERT INTO large_city_data.node(node_id, geom, containing_face)
	VALUES(1000000000018, 'SRID=4326;POINT(35 22)', NULL);
-- N19
INSERT INTO large_city_data.node(node_id, geom, containing_face)
	VALUES(1000000000019, 'SRID=4326;POINT(47 22)', NULL);
-- N20
INSERT INTO large_city_data.node(node_id, geom, containing_face)
	VALUES(1000000000020, 'SRID=4326;POINT(4 31)', NULL);
-- N21
INSERT INTO large_city_data.node(node_id, geom, containing_face)
	VALUES(1000000000021, 'SRID=4326;POINT(9 35)', NULL);
-- N22
INSERT INTO large_city_data.node(node_id, geom, containing_face)
	VALUES(1000000000022, 'SRID=4326;POINT(13 35)', NULL);

-- UPDATE Node id sequence
SELECT NULL FROM setval('large_city_data.node_node_id_seq', 1000000000022);

-- 2C. Insert data into <topology_name>.EDGE table.
-- E1
INSERT INTO large_city_data.edge VALUES(1000000000001, 1000000000001, 1000000000001, 1000000000001, -1000000000001, 1000000000001, 0,
  'SRID=4326;LINESTRING(8 30, 16 30, 16 38, 3 38, 3 30, 8 30)');
-- E2
INSERT INTO large_city_data.edge VALUES(1000000000002, 1000000000002, 1000000000002, 1000000000003, -1000000000002, 1000000000002, 0,
  'SRID=4326;LINESTRING(25 30, 31 30, 31 40, 17 40, 17 30, 25 30)');
-- E3
INSERT INTO large_city_data.edge VALUES(1000000000003, 1000000000002, 1000000000003, -1000000000003, 1000000000002, 1000000000002, 1000000000002,
  'SRID=4326;LINESTRING(25 30, 25 35)');
-- E4
INSERT INTO large_city_data.edge VALUES(1000000000004, 1000000000005, 1000000000006, -1000000000005, 1000000000004, 0, 0,
    'SRID=4326;LINESTRING(36 38, 38 35, 41 34, 42 33, 45 32, 47 28, 50 28, 52 32, 57 33)');
-- E5
INSERT INTO large_city_data.edge VALUES(1000000000005, 1000000000007, 1000000000006, -1000000000004, 1000000000005, 0, 0,
    'SRID=4326;LINESTRING(41 40, 45 40, 47 42, 62 41, 61 38, 59 39, 57 36, 57 33)');
-- E6
INSERT INTO large_city_data.edge VALUES(1000000000006, 1000000000016, 1000000000017, 1000000000007, -1000000000021, 0, 1000000000003,
    'SRID=4326;LINESTRING(9 22, 21 22)');
-- E7
INSERT INTO large_city_data.edge VALUES(1000000000007, 1000000000017, 1000000000018, 1000000000008, -1000000000019, 0, 1000000000004,
    'SRID=4326;LINESTRING(21 22, 35 22)');
-- E8
INSERT INTO large_city_data.edge VALUES(1000000000008, 1000000000018, 1000000000019, -1000000000015, -1000000000017, 0, 1000000000005,
    'SRID=4326;LINESTRING(35 22, 47 22)');
-- E9
INSERT INTO large_city_data.edge VALUES(1000000000009, 1000000000015, 1000000000014, 1000000000019, -1000000000022, 1000000000003, 1000000000006,
    'SRID=4326;LINESTRING(9 14, 21 14)');
-- E10
INSERT INTO large_city_data.edge VALUES(1000000000010, 1000000000013, 1000000000014, -1000000000020, 1000000000017, 1000000000007, 1000000000004,
    'SRID=4326;LINESTRING(35 14, 21 14)');
-- E11
INSERT INTO large_city_data.edge VALUES(1000000000011, 1000000000013, 1000000000012, 1000000000015, -1000000000018, 1000000000005, 1000000000008,
    'SRID=4326;LINESTRING(35 14, 47 14)');
-- E12
INSERT INTO large_city_data.edge VALUES(1000000000012, 1000000000008, 1000000000009, 1000000000020, 1000000000022, 1000000000006, 0,
    'SRID=4326;LINESTRING(9 6, 21 6)');
-- E13
INSERT INTO large_city_data.edge VALUES(1000000000013, 1000000000009, 1000000000010, 1000000000018, -1000000000012, 1000000000007, 0,
    'SRID=4326;LINESTRING(21 6, 35 6)');
-- E14
INSERT INTO large_city_data.edge VALUES(1000000000014, 1000000000010, 1000000000011, 1000000000016, -1000000000013, 1000000000008, 0,
    'SRID=4326;LINESTRING(35 6, 47 6)');
-- E15
INSERT INTO large_city_data.edge VALUES(1000000000015, 1000000000012, 1000000000019, -1000000000008, -1000000000016, 1000000000005, 0,
    'SRID=4326;LINESTRING(47 14, 47 22)');
-- E16
INSERT INTO large_city_data.edge VALUES(1000000000016, 1000000000011, 1000000000012, -1000000000011, -1000000000014, 1000000000008, 0,
    'SRID=4326;LINESTRING(47 6, 47 14)');
-- E17
INSERT INTO large_city_data.edge VALUES(1000000000017, 1000000000013, 1000000000018, -1000000000007, 1000000000011, 1000000000004, 1000000000005,
    'SRID=4326;LINESTRING(35 14, 35 22)');
-- E18
INSERT INTO large_city_data.edge VALUES(1000000000018, 1000000000010, 1000000000013, 1000000000010, 1000000000014, 1000000000007, 1000000000008,
    'SRID=4326;LINESTRING(35 6, 35 14)');
-- E19
INSERT INTO large_city_data.edge VALUES(1000000000019, 1000000000014, 1000000000017, -1000000000006, -1000000000010, 1000000000003, 1000000000004,
    'SRID=4326;LINESTRING(21 14, 21 22)');
-- E20
INSERT INTO large_city_data.edge VALUES(1000000000020, 1000000000009, 1000000000014, -1000000000009, 1000000000013, 1000000000006, 1000000000007,
    'SRID=4326;LINESTRING(21 6, 21 14)');
-- E21
INSERT INTO large_city_data.edge VALUES(1000000000021, 1000000000015, 1000000000016, 1000000000006, 1000000000009, 0, 1000000000003,
    'SRID=4326;LINESTRING(9 14, 9 22)');
-- E22
INSERT INTO large_city_data.edge VALUES(1000000000022, 1000000000008, 1000000000015, 1000000000021, 1000000000012, 0, 1000000000006,
    'SRID=4326;LINESTRING(9 6, 9 14)');
-- E25
INSERT INTO large_city_data.edge VALUES(1000000000025, 1000000000021, 1000000000022, -1000000000025, 1000000000025, 1000000000001, 1000000000001,
  'SRID=4326;LINESTRING(9 35, 13 35)');
-- E26
INSERT INTO large_city_data.edge VALUES(1000000000026, 1000000000020, 1000000000020, 1000000000026, -1000000000026, 1000000000009, 1000000000001,
  'SRID=4326;LINESTRING(4 31, 7 31, 7 34, 4 34, 4 31)');

-- UPDATE Edge id sequence
SELECT NULL FROM setval('large_city_data.edge_data_edge_id_seq', 1000000000026);

-- Set face minimum bounding rectangle
UPDATE large_city_data.face set mbr = ST_SetSRID( ( select st_extent(geom) from large_city_data.edge where left_face = face_id or right_face = face_id ), 4326 ) where face_id != 0;

-- Validate the loaded topology
SELECT 'load_validation', *
FROM ValidateTopology('large_city_data')
WHERE error IS NOT NULL;

END;

