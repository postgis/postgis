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
--ORA---- ...
--ORA---- 3. Create feature tables.
--ORA---- 4. Associate feature tables with the topology.
--ORA---- 5. Initialize topology
--ORA---- 6. Load feature tables using the SDO_TOPO_GEOMETRY constructor.

BEGIN;

-- 3. Create feature tables

CREATE SCHEMA large_features;

CREATE TABLE large_features.land_parcels ( -- Land parcels (selected faces)
  feature_name VARCHAR PRIMARY KEY, fid bigserial);
CREATE TABLE large_features.city_streets ( -- City streets (selected edges)
  feature_name VARCHAR PRIMARY KEY, fid bigserial);
CREATE TABLE large_features.traffic_signs ( -- Traffic signs (selected nodes)
  feature_name VARCHAR PRIMARY KEY, fid bigserial);

-- 4. Associate feature tables with the topology.
--    Add the three topology geometry layers to the large_city_data topology.
--    Any order is OK.
SELECT NULL FROM topology.AddTopoGeometryColumn('large_city_data', 'large_features.land_parcels', 'feature', 1101, 'POLYGON');
SELECT NULL FROM topology.AddTopoGeometryColumn('large_city_data', 'large_features.traffic_signs','feature', 1102, 'POINT');
SELECT NULL FROM topology.AddTopoGeometryColumn('large_city_data', 'large_features.city_streets','feature', 1103, 'LINE');

--  As a result, Spatial generates a unique TG_LAYER_ID for each layer in
--  the topology metadata (USER/ALL_SDO_TOPO_METADATA).

--NOTYET---- 5. Initialize topology metadata.
--NOTYET--EXECUTE topology.INITIALIZE_METADATA('large_city_data');

-- 6. Load feature tables using the CreateTopoGeom constructor.
-- Each topology feature can consist of one or more objects (face, edge, node)
-- of an appropriate type. For example, a land parcel can consist of one face,
-- or two or more faces, as specified in the SDO_TOPO_OBJECT_ARRAY.
-- There are typically fewer large_features than there are faces, nodes, and edges.
-- In this example, the only large_features are these:
-- Area large_features (land parcels): P1, P2, P3, P4, P5
-- Point large_features (traffic signs): S1, S2, S3, S4
-- Linear large_features (roads/streets): R1, R2, R3, R4

-- 6A. Load LAND_PARCELS table.
-- P1
INSERT INTO large_features.land_parcels(fid, feature_name, feature) VALUES (1101000000001, 'P1', -- Feature name
  topology.CreateTopoGeom(
    'large_city_data', -- Topology name
    3, -- Topology geometry type (polygon/multipolygon)
    1101, -- TG_LAYER_ID for this topology (from topology.layer)
    '{{1000000000003,3},{1000000000006,3}}', -- face_id:3 face_id:6
    1101000000001) -- tg_id
    );

-- P2
INSERT INTO large_features.land_parcels(fid, feature_name, feature) VALUES (1101000000002, 'P2', -- Feature name
  topology.CreateTopoGeom(
    'large_city_data', -- Topology name
    3, -- Topology geometry type (polygon/multipolygon)
    1101, -- TG_LAYER_ID for this topology (from ALL_SDO_TOPO_METADATA)
    '{{1000000000004,3},{1000000000007,3}}',
    1101000000002));
-- P3
INSERT INTO large_features.land_parcels(fid, feature_name, feature) VALUES (1101000000003, 'P3', -- Feature name
  topology.CreateTopoGeom(
    'large_city_data', -- Topology name
    3, -- Topology geometry type (polygon/multipolygon)
    1101, -- TG_LAYER_ID for this topology (from topology.layer)
    '{{1000000000005,3},{1000000000008,3}}',
    1101000000003));
-- P4
INSERT INTO large_features.land_parcels(fid, feature_name, feature) VALUES (1101000000004, 'P4', -- Feature name
  topology.CreateTopoGeom(
    'large_city_data', -- Topology name
    3, -- Topology geometry type (polygon/multipolygon)
    1101, -- TG_LAYER_ID for this topology (from topology.layer)
    '{{1000000000002,3}}',
    1101000000004));
-- P5 (Includes F1, but not F9.)
INSERT INTO large_features.land_parcels(fid, feature_name, feature) VALUES (1101000000005, 'P5', -- Feature name
  topology.CreateTopoGeom(
    'large_city_data', -- Topology name
    3, -- Topology geometry type (polygon/multipolygon)
    1101, -- TG_LAYER_ID for this topology (from topology.layer)
    '{{1000000000001,3}}',
    1101000000005));

-- 6B. Load TRAFFIC_SIGNS table.
-- S1
INSERT INTO large_features.traffic_signs(fid, feature_name, feature) VALUES (1102000000002, 'S1', -- Feature name
  topology.CreateTopoGeom(
    'large_city_data', -- Topology name
    1, -- Topology geometry type (point)
    1102, -- TG_LAYER_ID for this topology (from topology.layer)
    '{{1000000000014,1}}',
    1102000000002));
-- S2
INSERT INTO large_features.traffic_signs(fid, feature_name, feature) VALUES (1102000000003, 'S2', -- Feature name
  topology.CreateTopoGeom(
    'large_city_data', -- Topology name
    1, -- Topology geometry type (point)
    1102, -- TG_LAYER_ID for this topology (from topology.layer)
    '{{1000000000013,1}}',
    1102000000003));
-- S3
INSERT INTO large_features.traffic_signs(fid, feature_name, feature) VALUES (1102000000004, 'S3', -- Feature name
  topology.CreateTopoGeom(
    'large_city_data', -- Topology name
    1, -- Topology geometry type (point)
    1102, -- TG_LAYER_ID for this topology (from topology.layer)
    '{{1000000000006,1}}',
    1102000000004));
-- S4
INSERT INTO large_features.traffic_signs(fid, feature_name, feature) VALUES (1102000000005, 'S4', -- Feature name
  topology.CreateTopoGeom(
    'large_city_data', -- Topology name
    1, -- Topology geometry type (point)
    1102, -- TG_LAYER_ID for this topology (from topology.layer)
    '{{1000000000004,1}}',
    1102000000005));

-- 6C. Load CITY_STREETS table.
-- (Note: "R" in feature names is for "Road", because "S" is used for signs.)
-- R1
INSERT INTO large_features.city_streets(fid, feature_name, feature) VALUES (1103000000001, 'R1', -- Feature name
  topology.CreateTopoGeom(
    'large_city_data', -- Topology name
    2, -- Topology geometry type (line string)
    1103, -- TG_LAYER_ID for this topology (from topology.layer)
    '{{1000000000009,2},{-1000000000010,2}}',
    1103000000001)); -- E9, E10
-- R2
INSERT INTO large_features.city_streets(fid, feature_name, feature) VALUES (1103000000002, 'R2', -- Feature name
  topology.CreateTopoGeom(
    'large_city_data', -- Topology name
    2, -- Topology geometry type (line string)
    1103, -- TG_LAYER_ID for this topology (from topology.layer)
    '{{1000000000004,2},{-1000000000005,2}}',
    1103000000002)); -- E4, E5
-- R3
INSERT INTO large_features.city_streets(fid, feature_name, feature) VALUES (1103000000003, 'R3', -- Feature name
  topology.CreateTopoGeom(
    'large_city_data', -- Topology name
    2, -- Topology geometry type (line string)
    1103, -- TG_LAYER_ID for this topology (from topology.layer)
    '{{1000000000025,2}}',
    1103000000003));
-- R4
INSERT INTO large_features.city_streets(fid, feature_name, feature) VALUES (1103000000004, 'R4', -- Feature name
  topology.CreateTopoGeom(
    'large_city_data', -- Topology name
    2, -- Topology geometry type (line string)
    1103, -- TG_LAYER_ID for this topology (from topology.layer)
    '{{1000000000003,2}}',
    1103000000004));

END;

