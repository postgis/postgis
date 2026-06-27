set client_min_messages to WARNING;

DROP SCHEMA IF EXISTS t5863_feat CASCADE;
DROP SCHEMA IF EXISTS t5863 CASCADE;
DROP SCHEMA IF EXISTS t5863_unknown CASCADE;
DROP SCHEMA IF EXISTS t5863z CASCADE;
DROP ROLE IF EXISTS t5863_reader;
DROP ROLE IF EXISTS t5863_column_reader;
DELETE FROM topology.topology WHERE name = 't5863';
DELETE FROM topology.topology WHERE name = 't5863_unknown';
DELETE FROM topology.topology WHERE name = 't5863z';

CREATE ROLE t5863_reader;
CREATE ROLE t5863_column_reader;

SELECT 'create', topology.CreateTopology('t5863', 4326) > 0;
SELECT 'node1', topology.ST_AddIsoNode('t5863', 0, 'SRID=4326;POINT(0 0)'::geometry);
SELECT 'node2', topology.ST_AddIsoNode('t5863', 0, 'SRID=4326;POINT(1 1)'::geometry);
SELECT 'edge', topology.ST_AddIsoEdge('t5863', 1, 2, 'SRID=4326;LINESTRING(0 0, 1 1)'::geometry);
UPDATE t5863.face SET mbr = ST_MakeEnvelope(0, 0, 1, 1, 4326) WHERE face_id = 0;
GRANT SELECT ON t5863.edge TO t5863_reader;
GRANT SELECT (geom) ON t5863.edge TO t5863_column_reader;

CREATE SCHEMA t5863_feat;
CREATE TABLE t5863_feat.lines(id integer);
SELECT 'layer', topology.AddTopoGeometryColumn('t5863', 't5863_feat', 'lines', 'tg', 'LINE');
INSERT INTO t5863_feat.lines(id, tg)
VALUES (1, topology.CreateTopoGeom('t5863', 2, 1, '{{1,2}}'::topology.TopoElementArray));

SELECT 'before-topology',
  topology.GetTopologySRID('t5863'),
  (SELECT ST_Srid(tg) FROM t5863_feat.lines WHERE id = 1);
SELECT 'before-primitives',
  (SELECT DISTINCT ST_SRID(geom) FROM t5863.node),
  (SELECT DISTINCT ST_SRID(geom) FROM t5863.edge_data),
  (SELECT DISTINCT ST_SRID(mbr) FROM t5863.face WHERE mbr IS NOT NULL);
SELECT 'before-typmods',
  (SELECT format_type(atttypid, atttypmod) FROM pg_attribute WHERE attrelid = 't5863.node'::regclass AND attname = 'geom'),
  (SELECT format_type(atttypid, atttypmod) FROM pg_attribute WHERE attrelid = 't5863.edge_data'::regclass AND attname = 'geom'),
  (SELECT format_type(atttypid, atttypmod) FROM pg_attribute WHERE attrelid = 't5863.face'::regclass AND attname = 'mbr');

SELECT 'update', topology.UpdateTopologySRID('t5863', 3857);

SELECT 'after-topology',
  topology.GetTopologySRID('t5863'),
  (SELECT ST_Srid(tg) FROM t5863_feat.lines WHERE id = 1);
SELECT 'after-primitives',
  (SELECT DISTINCT ST_SRID(geom) FROM t5863.node),
  (SELECT DISTINCT ST_SRID(geom) FROM t5863.edge_data),
  (SELECT DISTINCT ST_SRID(mbr) FROM t5863.face WHERE mbr IS NOT NULL);
SELECT 'after-typmods',
  (SELECT format_type(atttypid, atttypmod) FROM pg_attribute WHERE attrelid = 't5863.node'::regclass AND attname = 'geom'),
  (SELECT format_type(atttypid, atttypmod) FROM pg_attribute WHERE attrelid = 't5863.edge_data'::regclass AND attname = 'geom'),
  (SELECT format_type(atttypid, atttypmod) FROM pg_attribute WHERE attrelid = 't5863.face'::regclass AND attname = 'mbr');
SELECT 'edge-grant',
  has_table_privilege('t5863_reader', 't5863.edge', 'SELECT');
SELECT 'edge-column-grant',
  has_table_privilege('t5863_column_reader', 't5863.edge', 'SELECT'),
  has_column_privilege('t5863_column_reader', 't5863.edge', 'geom', 'SELECT');

INSERT INTO t5863.edge(edge_id, start_node, end_node, next_left_edge, next_right_edge, left_face, right_face, geom)
VALUES (2, 2, 1, -2, 2, 0, 0, 'SRID=3857;LINESTRING(1 1, 0 0)'::geometry);
SELECT 'view-insert', edge_id, ST_SRID(geom), abs_next_left_edge, abs_next_right_edge
FROM t5863.edge_data
WHERE edge_id = 2;

SELECT 'create-z', topology.CreateTopology('t5863z', 4326, 0, true) > 0;
SELECT 'update-z', topology.UpdateTopologySRID('t5863z', 3857);
SELECT 'z-typmods',
  (SELECT format_type(atttypid, atttypmod) FROM pg_attribute WHERE attrelid = 't5863z.node'::regclass AND attname = 'geom'),
  (SELECT format_type(atttypid, atttypmod) FROM pg_attribute WHERE attrelid = 't5863z.edge_data'::regclass AND attname = 'geom'),
  (SELECT format_type(atttypid, atttypmod) FROM pg_attribute WHERE attrelid = 't5863z.face'::regclass AND attname = 'mbr');

SELECT 'create-unknown', topology.CreateTopology('t5863_unknown', 4326) > 0;
SELECT 'invalid-srid', topology.UpdateTopologySRID('t5863_unknown', 999000);
SELECT 'unknown-srid', topology.UpdateTopologySRID('t5863_unknown', 0);
SELECT 'after-unknown-srid', topology.GetTopologySRID('t5863_unknown');

SELECT topology.DropTopology('t5863_unknown');
SELECT topology.DropTopology('t5863z');
SELECT topology.DropTopology('t5863');
DROP SCHEMA t5863_feat CASCADE;
DROP ROLE t5863_column_reader;
DROP ROLE t5863_reader;
