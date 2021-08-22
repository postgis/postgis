\set VERBOSITY terse
set client_min_messages to WARNING;

select NULL FROM createtopology('tt', 4326);

-- layer 1 is PUNTUAL
CREATE TABLE tt.f_point(lbl text primary key);
SELECT NULL FROM AddTopoGeometryColumn('tt', 'tt', 'f_point', 'g', 'POINT');

-- layer 2 is LINEAL
CREATE TABLE tt.f_line(lbl text primary key );
SELECT NULL FROM AddTopoGeometryColumn('tt', 'tt', 'f_line', 'g', 'LINE');

-- layer 3 is AREAL
CREATE TABLE tt.f_area(lbl text primary key);
SELECT NULL FROM AddTopoGeometryColumn('tt', 'tt', 'f_area', 'g', 'POLYGON');

-- layer 4 is MIXED
CREATE TABLE tt.f_coll(lbl text primary key);
SELECT NULL FROM AddTopoGeometryColumn('tt', 'tt', 'f_coll', 'g', 'COLLECTION');

-- layer 5 is HIERARCHICAL PUNTUAL
CREATE TABLE tt.f_hier_point(lbl text primary key);
SELECT NULL FROM AddTopoGeometryColumn('tt', 'tt', 'f_hier_point', 'g', 'POINT', 1);

-- layer 6 is HIERARCHICAL LINEAL
CREATE TABLE tt.f_hier_line(lbl text primary key);
SELECT NULL FROM AddTopoGeometryColumn('tt', 'tt', 'f_hier_line', 'g', 'LINE', 2);

-- layer 7 is HIERARCHICAL AREAL
CREATE TABLE tt.f_hier_area(lbl text primary key);
SELECT NULL FROM AddTopoGeometryColumn('tt', 'tt', 'f_hier_area', 'g', 'POLYGON', 3);

-- layer 8 is HIERARCHICAL MIXED
CREATE TABLE tt.f_hier_coll(lbl text primary key);
SELECT NULL FROM AddTopoGeometryColumn('tt', 'tt', 'f_hier_coll', 'g', 'COLLECTION', 4);


-- Cast empties
SELECT 'empty', 'puntal', ST_AsEWKT(CreateTopoGeom('tt', 1, 1)::geometry);
SELECT 'empty', 'lineal', ST_AsEWKT(CreateTopoGeom('tt', 2, 2)::geometry);
SELECT 'empty', 'areal', ST_AsEWKT(CreateTopoGeom('tt', 3, 3)::geometry);
SELECT 'empty', 'mixed', ST_AsEWKT(CreateTopoGeom('tt', 4, 4)::geometry);

SELECT 'empty', 'hier', 'puntal', ST_AsEWKT(CreateTopoGeom('tt', 1, 5)::geometry);
SELECT 'empty', 'hier', 'lineal', ST_AsEWKT(CreateTopoGeom('tt', 2, 6)::geometry);
SELECT 'empty', 'hier', 'areal', ST_AsEWKT(CreateTopoGeom('tt', 3, 7)::geometry);
SELECT 'empty', 'hier', 'mixed', ST_AsEWKT(CreateTopoGeom('tt', 4, 8)::geometry);

-- Cast directed lineal
-- See https://trac.osgeo.org/postgis/ticket/4881

-- Insert 3 horizontal edge going in mixed directions
-- Odd ids: left-to-right
-- Even ids: right-to-left
INSERT INTO tt.node(node_id, containing_face, geom) VALUES
( 1, NULL, 'SRID=4326;POINT(1 0)' ),
( 2, NULL, 'SRID=4326;POINT(2 0)' ),
( 3, NULL, 'SRID=4326;POINT(3 0)' ),
( 4, NULL, 'SRID=4326;POINT(4 0)' )
;
SELECT NULL FROM setval('tt.node_node_id_seq', 3);
INSERT INTO tt.edge(
  edge_id, start_node, end_node,
  next_left_edge, next_right_edge,
  left_face, right_face,
  geom
)
VALUES (
  1, 1, 2,
  -2, 1,
  0, 0,
  'SRID=4326;LINESTRING(0 0, 1 0)'
),(
  2, 3, 2,
  -1, 2,
  0, 0,
  'SRID=4326;LINESTRING(2 0, 1 0)'
),(
  3, 3, 4,
  -3, 2,
  0, 0,
  'SRID=4326;LINESTRING(2 0, 3 0)'
)
;
SELECT NULL FROM setval('tt.edge_data_edge_id_seq', 2);

-- Insert two lines using the horizontal edge
-- in opposite directions
INSERT INTO tt.f_line(lbl, g) VALUES (
  'dir.2r', CreateTopoGeom('tt', 2, 2, '{{1,2},{-2,2},{3,2}}')
), (
  'dir.2l', CreateTopoGeom('tt', 2, 2, '{{-1,2},{-3,2},{2,2}}')
), (
  'dir.multi.2r', CreateTopoGeom('tt', 2, 2, '{{1,2},{3,2}}')
), (
  'dir.multi.2l', CreateTopoGeom('tt', 2, 2, '{{-1,2},{-3,2}}')
), (
  'dir.multi.converge', CreateTopoGeom('tt', 2, 2, '{{1,2},{-3,2}}')
), (
  'dir.multi.diverge', CreateTopoGeom('tt', 2, 2, '{{-1,2},{3,2}}')
);

-- Check out lines are rendered with direction
-- based on their signed element_id
SELECT
  '#4881',
  'E',
  e.edge_id,
  ST_AsText(e.geom)
FROM tt.edge e
ORDER BY e.edge_id;

SELECT
  '#4881',
  l.lbl,
  ST_AsEWKT(ST_CollectionHomogenize(l.g::geometry)),
  array_agg(r.element_id ORDER BY abs(r.element_id))
FROM tt.f_line l, tt.relation r
WHERE lbl like 'dir.%'
AND r.topogeo_id = id(l.g)
GROUP BY 1,2,3
ORDER BY lbl;

-- Cleanup
SELECT NULL FROM DropTopology('tt');
