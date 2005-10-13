--
-- From examples in chapter 1.12.1 of
-- "Spatial Topology and Network Data Models" (Oracle manual)
--
-- Modified to use postgis-based topology model.
-- Loads the whole topology represented in Figure 1-1 of the
-- manual, creates TopoGeometry objects and associations.
--

--ORA--------------------------------
--ORA---- Main steps for using the topology data model with a topology
--ORA---- built from edge, node, and face data
--ORA--------------------------------
--ORA---- 1. Create a topology.
--ORA---- 2. Load (normally bulk-load) topology data (node, edge, and face tables).
--ORA---- 3. Create feature tables.
--ORA---- 4. Associate feature tables with the topology.
--ORA---- 5. Initialize topology
--ORA---- 6. Load feature tables using the SDO_TOPO_GEOMETRY constructor.
--ORA---- 7. Query the data.
--ORA---- 8. Optionally, edit data using the PL/SQL or Java API.


DROP SCHEMA features CASCADE;

BEGIN;

-- 1. Create the topology. 
SELECT topology.DropTopology('city_data'); -- get rid of the old one
SELECT topology.CreateTopology('city_data');

-- 2. Load topology data (node, edge, and face tables).
-- Use INSERT statements here instead of a bulk-load utility.

-- 2A. Insert data into <topology_name>.FACE table.

INSERT INTO city_data.face(face_id) VALUES(1); -- F1
INSERT INTO city_data.face(face_id) VALUES(2); -- F2
INSERT INTO city_data.face(face_id) VALUES(9); -- F9
INSERT INTO city_data.face(face_id) VALUES(3); -- F3
INSERT INTO city_data.face(face_id) VALUES(4); -- F4
INSERT INTO city_data.face(face_id) VALUES(5); -- F5
INSERT INTO city_data.face(face_id) VALUES(6); -- F6
INSERT INTO city_data.face(face_id) VALUES(7); -- F7
INSERT INTO city_data.face(face_id) VALUES(8); -- F8

-- 2B. Insert data into <topology_name>.NODE table.
-- N1
INSERT INTO city_data.node(node_id, geom, containing_face) 
	VALUES(1, 'POINT(8 30)', NULL);
-- N2
INSERT INTO city_data.node(node_id, geom, containing_face) 
	VALUES(2, 'POINT(25 30)', NULL);
-- N3
INSERT INTO city_data.node(node_id, geom, containing_face) 
	VALUES(3, 'POINT(25 35)', NULL);
-- N4
INSERT INTO city_data.node(node_id, geom, containing_face) 
	VALUES(4, 'POINT(20 37)', 2);
-- N5
INSERT INTO city_data.node(node_id, geom, containing_face) 
	VALUES(5, 'POINT(36 38)', NULL);
-- N6
INSERT INTO city_data.node(node_id, geom, containing_face) 
	VALUES(6, 'POINT(57 33)', NULL);
-- N7
INSERT INTO city_data.node(node_id, geom, containing_face) 
	VALUES(7, 'POINT(41 40)', NULL);
-- N8
INSERT INTO city_data.node(node_id, geom, containing_face) 
	VALUES(8, 'POINT(9 6)', NULL);
-- N9
INSERT INTO city_data.node(node_id, geom, containing_face) 
	VALUES(9, 'POINT(21 6)', NULL);
-- N10
INSERT INTO city_data.node(node_id, geom, containing_face) 
	VALUES(10, 'POINT(35 6)', NULL);
-- N11
INSERT INTO city_data.node(node_id, geom, containing_face) 
	VALUES(11, 'POINT(47 6)', NULL);
-- N12
INSERT INTO city_data.node(node_id, geom, containing_face) 
	VALUES(12, 'POINT(47 14)', NULL);
-- N13
INSERT INTO city_data.node(node_id, geom, containing_face) 
	VALUES(13, 'POINT(35 14)', NULL);
-- N14
INSERT INTO city_data.node(node_id, geom, containing_face) 
	VALUES(14, 'POINT(21 14)', NULL);
-- N15
INSERT INTO city_data.node(node_id, geom, containing_face) 
	VALUES(15, 'POINT(9 14)', NULL);
-- N16
INSERT INTO city_data.node(node_id, geom, containing_face) 
	VALUES(16, 'POINT(9 22)', NULL);
-- N17
INSERT INTO city_data.node(node_id, geom, containing_face) 
	VALUES(17, 'POINT(21 22)', NULL);
-- N18
INSERT INTO city_data.node(node_id, geom, containing_face) 
	VALUES(18, 'POINT(35 22)', NULL);
-- N19
INSERT INTO city_data.node(node_id, geom, containing_face) 
	VALUES(19, 'POINT(47 22)', NULL);
-- N20
INSERT INTO city_data.node(node_id, geom, containing_face) 
	VALUES(20, 'POINT(4 31)', NULL);
-- N21
INSERT INTO city_data.node(node_id, geom, containing_face) 
	VALUES(21, 'POINT(9 35)', NULL);
-- N22
INSERT INTO city_data.node(node_id, geom, containing_face) 
	VALUES(22, 'POINT(13 35)', NULL);

-- 2C. Insert data into <topology_name>.EDGE table.
-- E1
INSERT INTO city_data.edge VALUES(1, 1, 1, 1, -1, 1, 0,
  'LINESTRING(8 30, 16 30, 16 38, 3 38, 3 30, 8 30)');
-- E2
INSERT INTO city_data.edge VALUES(2, 2, 2, -3, -2, 2, 0,
  'LINESTRING(25 30, 31 30, 31 40, 17 40, 17 30, 25 30)');
-- E3
INSERT INTO city_data.edge VALUES(3, 2, 3, -3, 2, 2, 2,
  'LINESTRING(25 30, 25 35)');
-- E4
INSERT INTO city_data.edge VALUES(4, 5, 6, -5, 4, 0, 0,
    'LINESTRING(36 38, 38 35, 41 34, 42 33, 45 32, 47 28, 50 28, 52 32, 57 33)');
-- E5
INSERT INTO city_data.edge VALUES(5, 7, 6, -4, 5, 0, 0,
    'LINESTRING(41 40, 45 40, 47 42, 62 41, 61 38, 59 39, 57 36, 57 33)');
-- E6
INSERT INTO city_data.edge VALUES(6, 16, 17, 7, -21, 0, 3,
    'LINESTRING(9 22, 21 22)');
-- E7
INSERT INTO city_data.edge VALUES(7, 17, 18, 8, -19, 0, 4,
    'LINESTRING(21 22, 35 22)');
-- E8
INSERT INTO city_data.edge VALUES(8, 18, 19, -15, -17, 0, 5,
    'LINESTRING(35 22, 47 22)');
-- E9
INSERT INTO city_data.edge VALUES(9, 15, 14, 19, -22, 3, 6,
    'LINESTRING(9 14, 21 14)');
-- E10
INSERT INTO city_data.edge VALUES(10, 13, 14, -20, 17, 7, 4,
    'LINESTRING(35 14, 21 14)');
-- E11
INSERT INTO city_data.edge VALUES(11, 13, 12, 15, -18, 5, 8,
    'LINESTRING(35 14, 47 14)');
-- E12
INSERT INTO city_data.edge VALUES(12, 8, 9, 20, 22, 6, 0,
    'LINESTRING(9 6, 21 6)');
-- E13
INSERT INTO city_data.edge VALUES(13, 9, 10, 18, -12, 7, 0,
    'LINESTRING(21 6, 35 6)');
-- E14
INSERT INTO city_data.edge VALUES(14, 10, 11, 16, -13, 8, 0,
    'LINESTRING(35 6, 47 6)');
-- E15
INSERT INTO city_data.edge VALUES(15, 12, 19, -8, -16, 5, 0,
    'LINESTRING(47 14, 47 22)');
-- E16
INSERT INTO city_data.edge VALUES(16, 11, 12, -11, -14, 8, 0,
    'LINESTRING(47 6, 47 14)');
-- E17
INSERT INTO city_data.edge VALUES(17, 13, 18, -7, 11, 4, 5,
    'LINESTRING(35 14, 35 22)');
-- E18
INSERT INTO city_data.edge VALUES(18, 10, 13, 10, 14, 7, 8,
    'LINESTRING(35 6, 35 14)');
-- E19
INSERT INTO city_data.edge VALUES(19, 14, 17, -6, -10, 3, 4,
    'LINESTRING(21 14, 21 22)');
-- E20
INSERT INTO city_data.edge VALUES(20, 9, 14, -9, 13, 6, 7,
    'LINESTRING(21 6, 21 14)');
-- E21
INSERT INTO city_data.edge VALUES(21, 15, 16, 6, 9, 0, 3,
    'LINESTRING(9 14, 9 22)');
-- E22
INSERT INTO city_data.edge VALUES(22, 8, 15, 21, 12, 0, 6,
    'LINESTRING(9 6, 9 14)');
-- E25
INSERT INTO city_data.edge VALUES(25, 21, 22, -25, 25, 1, 1,
  'LINESTRING(9 35, 13 35)');
-- E26
INSERT INTO city_data.edge VALUES(26, 20, 20, 26, -26, 9, 1,
  'LINESTRING(4 31, 7 31, 7 34, 4 34, 4 31)');

-- 3. Create feature tables  

CREATE SCHEMA features;

CREATE TABLE features.land_parcels ( -- Land parcels (selected faces)
  feature_name VARCHAR PRIMARY KEY);
CREATE TABLE features.city_streets ( -- City streets (selected edges)
  feature_name VARCHAR PRIMARY KEY);
CREATE TABLE features.traffic_signs ( -- Traffic signs (selected nodes)
  feature_name VARCHAR PRIMARY KEY);



-- 4. Associate feature tables with the topology.
--    Add the three topology geometry layers to the CITY_DATA topology.
--    Any order is OK.
SELECT topology.AddTopoGeometryColumn('city_data', 'features', 'land_parcels', 'feature', 'POLYGON');
SELECT topology.AddTopoGeometryColumn('city_data', 'features', 'traffic_signs','feature', 'POINT');
SELECT topology.AddTopoGeometryColumn('city_data', 'features', 'city_streets','feature', 'LINE');

--  As a result, Spatial generates a unique TG_LAYER_ID for each layer in
--  the topology metadata (USER/ALL_SDO_TOPO_METADATA).

--NOTYET---- 5. Initialize topology metadata.
--NOTYET--EXECUTE topology.INITIALIZE_METADATA('CITY_DATA');



-- 6. Load feature tables using the CreateTopoGeom constructor.
-- Each topology feature can consist of one or more objects (face, edge, node)
-- of an appropriate type. For example, a land parcel can consist of one face,
-- or two or more faces, as specified in the SDO_TOPO_OBJECT_ARRAY.
-- There are typically fewer features than there are faces, nodes, and edges.
-- In this example, the only features are these:
-- Area features (land parcels): P1, P2, P3, P4, P5
-- Point features (traffic signs): S1, S2, S3, S4
-- Linear features (roads/streets): R1, R2, R3, R4

-- 6A. Load LAND_PARCELS table.
-- P1
INSERT INTO features.land_parcels VALUES ('P1', -- Feature name
  topology.CreateTopoGeom(
    'city_data', -- Topology name
    3, -- Topology geometry type (polygon/multipolygon)
    1, -- TG_LAYER_ID for this topology (from topology.layer)
    '{{3,3},{6,3}}') -- face_id:3 face_id:6
    );

-- P2
INSERT INTO features.land_parcels VALUES ('P2', -- Feature name
  topology.CreateTopoGeom(
    'city_data', -- Topology name
    3, -- Topology geometry type (polygon/multipolygon)
    1, -- TG_LAYER_ID for this topology (from ALL_SDO_TOPO_METADATA)
    '{{4,3},{7,3}}'));
-- P3
INSERT INTO features.land_parcels VALUES ('P3', -- Feature name
  topology.CreateTopoGeom(
    'city_data', -- Topology name
    3, -- Topology geometry type (polygon/multipolygon)
    1, -- TG_LAYER_ID for this topology (from topology.layer)
    '{{5,3},{8,3}}'));
-- P4
INSERT INTO features.land_parcels VALUES ('P4', -- Feature name
  topology.CreateTopoGeom(
    'city_data', -- Topology name
    3, -- Topology geometry type (polygon/multipolygon)
    1, -- TG_LAYER_ID for this topology (from topology.layer)
    '{{2,3}}'));
-- P5 (Includes F1, but not F9.)
INSERT INTO features.land_parcels VALUES ('P5', -- Feature name
  topology.CreateTopoGeom(
    'city_data', -- Topology name
    3, -- Topology geometry type (polygon/multipolygon)
    1, -- TG_LAYER_ID for this topology (from topology.layer)
    '{{1,3}}'));

-- 6B. Load TRAFFIC_SIGNS table.
-- S1
INSERT INTO features.traffic_signs VALUES ('S1', -- Feature name
  topology.CreateTopoGeom(
    'city_data', -- Topology name
    1, -- Topology geometry type (point)
    2, -- TG_LAYER_ID for this topology (from topology.layer)
    '{{14,1}}'));
-- S2
INSERT INTO features.traffic_signs VALUES ('S2', -- Feature name
  topology.CreateTopoGeom(
    'city_data', -- Topology name
    1, -- Topology geometry type (point)
    2, -- TG_LAYER_ID for this topology (from topology.layer)
    '{{13,1}}'));
-- S3
INSERT INTO features.traffic_signs VALUES ('S3', -- Feature name
  topology.CreateTopoGeom(
    'city_data', -- Topology name
    1, -- Topology geometry type (point)
    2, -- TG_LAYER_ID for this topology (from topology.layer)
    '{{6,1}}'));
-- S4
INSERT INTO features.traffic_signs VALUES ('S4', -- Feature name
  topology.CreateTopoGeom(
    'city_data', -- Topology name
    1, -- Topology geometry type (point)
    2, -- TG_LAYER_ID for this topology (from topology.layer)
    '{{4,1}}'));

-- 6C. Load CITY_STREETS table.
-- (Note: "R" in feature names is for "Road", because "S" is used for signs.)
-- R1
INSERT INTO features.city_streets VALUES ('R1', -- Feature name
  topology.CreateTopoGeom(
    'city_data', -- Topology name
    2, -- Topology geometry type (line string)
    3, -- TG_LAYER_ID for this topology (from topology.layer)
    '{{9,2},{-10,2}}')); -- E9, E10
-- R2
INSERT INTO features.city_streets VALUES ('R2', -- Feature name
  topology.CreateTopoGeom(
    'city_data', -- Topology name
    2, -- Topology geometry type (line string)
    3, -- TG_LAYER_ID for this topology (from topology.layer)
    '{{4,2},{-5,2}}')); -- E4, E5
-- R3
INSERT INTO features.city_streets VALUES ('R3', -- Feature name
  topology.CreateTopoGeom(
    'city_data', -- Topology name
    2, -- Topology geometry type (line string)
    3, -- TG_LAYER_ID for this topology (from topology.layer)
    '{{25,2}}'));
-- R4
INSERT INTO features.city_streets VALUES ('R4', -- Feature name
  topology.CreateTopoGeom(
    'city_data', -- Topology name
    2, -- Topology geometry type (line string)
    3, -- TG_LAYER_ID for this topology (from topology.layer)
    '{{3,2}}'));



-- 7. Query the data.
SELECT a.feature_name, id(a.feature) as tg_id,
	astext(topology.Geometry(a.feature)) as geom
FROM features.land_parcels a;

-- Query not in original example --strk;
SELECT a.feature_name, id(a.feature) as tg_id,
	astext(topology.Geometry(a.feature)) as geom
FROM features.traffic_signs a;

-- Query not in original example --strk;
SELECT a.feature_name, id(a.feature) as tg_id,
	astext(topology.Geometry(a.feature)) as geom
FROM features.city_streets a;

--NOTYET--
--NOTYET--/* Window is city_streets */
--NOTYET--SELECT a.feature_name, b.feature_name
--NOTYET--  FROM city_streets b,
--NOTYET--     land_parcels a
--NOTYET--  WHERE b.feature_name like 'R%' AND
--NOTYET--     sdo_anyinteract(a.feature, b.feature) = 'TRUE'
--NOTYET--  ORDER BY b.feature_name, a.feature_name;
--NOTYET--
--NOTYET---- Find all streets that have any interaction with land parcel P3.
--NOTYET---- (Should return only R1.)
--NOTYET--SELECT c.feature_name FROM city_streets c, land_parcels l
--NOTYET--  WHERE l.feature_name = 'P3' AND
--NOTYET--   SDO_ANYINTERACT (c.feature, l.feature) = 'TRUE';
--NOTYET--
--NOTYET---- Find all land parcels that have any interaction with traffic sign S1.
--NOTYET---- (Should return P1 and P2.)
--NOTYET--SELECT l.feature_name FROM land_parcels l, traffic_signs t
--NOTYET--  WHERE t.feature_name = 'S1' AND
--NOTYET--   SDO_ANYINTERACT (l.feature, t.feature) = 'TRUE';
--NOTYET--
--NOTYET---- Get the geometry for land parcel P1.
--NOTYET--SELECT l.feature_name, l.feature.get_geometry()
--NOTYET--  FROM land_parcels l WHERE l.feature_name = 'P1';
--NOTYET--
--NOTYET---- Get the boundary of face with face_id 3.
--NOTYET--SELECT topology.GET_FACE_BOUNDARY('CITY_DATA', 3) FROM DUAL;
--NOTYET--
--NOTYET---- Get the topological elements for land parcel P2.
--NOTYET---- CITY_DATA layer, land parcels (tg_ layer_id = 1), parcel P2 (tg_id = 2)
--NOTYET--SELECT topology.GET_TOPO_OBJECTS('CITY_DATA', 1, 2) FROM DUAL;
--NOTYET--
--NOTYET--

END;

