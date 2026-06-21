\set VERBOSITY terse
set client_min_messages to WARNING;


--------------------------------------------------------
-- See https://trac.osgeo.org/postgis/ticket/4851
--------------------------------------------------------

select NULL FROM createtopology('tt');

-- Create simple puntal layer (will be layer 1)
CREATE TABLE tt.f_puntal(id serial);
SELECT 'simple_puntual_layer', AddTopoGeometryColumn('tt', 'tt', 'f_puntal','g','POINT');

-- Create a lineal layer (will be layer 2)
CREATE TABLE tt.f_lineal(id serial);
SELECT 'simple_lineal_layer', AddTopoGeometryColumn('tt', 'tt', 'f_lineal','g','LINE');

-- Create an areal layer (will be layer 3)
CREATE TABLE tt.f_areal(id serial);
SELECT 'simple_areal_layer', AddTopoGeometryColumn('tt', 'tt', 'f_areal','g','POLYGON');

-- Create a collection layer (will be layer 4)
CREATE TABLE tt.f_coll(id serial);
SELECT 'simple_collection_layer', AddTopoGeometryColumn('tt', 'tt', 'f_coll','g','COLLECTION');

-- Create a hierarchical puntal layer (will be layer 5)
CREATE TABLE tt.f_hier_puntal(id serial);
SELECT 'hierarchical_puntual_layer', AddTopoGeometryColumn('tt', 'tt', 'f_hier_puntal','g','POINT', 1);

-- Create another simple puntal layer so the child layer id differs from
-- the primitive POINT element type (will be layer 6)
CREATE TABLE tt.f_puntal_child(id serial);
SELECT 'simple_puntual_child_layer', AddTopoGeometryColumn('tt', 'tt', 'f_puntal_child','g','POINT');

-- Create a hierarchical puntal layer using layer 6 as children (will be layer 7)
CREATE TABLE tt.f_hier_puntal_child(id serial);
SELECT 'hierarchical_puntual_child_layer', AddTopoGeometryColumn('tt', 'tt', 'f_hier_puntal_child','g','POINT', 6);

-- Create a hierarchical collection layer using layer 6 as children (will be layer 8)
CREATE TABLE tt.f_hier_puntal_child_coll(id serial);
SELECT 'hierarchical_puntual_child_collection_layer', AddTopoGeometryColumn('tt', 'tt', 'f_hier_puntal_child_coll','g','COLLECTION', 6);


-- point to point
SELECT 'tg2tg.poi2poi', ST_AsText(TopoGeom_addTopoGeom(
    toTopoGeom('POINT(1 1)', 'tt', 1),
    toTopoGeom('POINT(0 0)', 'tt', 1)
));

-- point to mixed
SELECT 'tg2tg.poi2mix', ST_AsText(TopoGeom_addTopoGeom(
    toTopoGeom('LINESTRING(1 1, 0 0)', 'tt', 4),
    toTopoGeom('POINT(0 0)', 'tt', 1)
));

-- line to line
SELECT 'tg2tg.lin2lin', ST_AsText(TopoGeom_addTopoGeom(
    toTopoGeom('LINESTRING(0 1, 2 1)', 'tt', 2),
    toTopoGeom('LINESTRING(0 0, 2 0)', 'tt', 2)
));

-- line to mixed
SELECT 'tg2tg.lin2mix', ST_AsText(TopoGeom_addTopoGeom(
    toTopoGeom('POINT(0 0)', 'tt', 4),
    toTopoGeom('LINESTRING(0 0, 2 0)', 'tt', 2)
));

CREATE TEMP TABLE topogeom_addtopogeom_lineal AS
WITH edge AS (
  SELECT topology.AddEdge('tt', 'LINESTRING(10 0, 11 0)'::geometry) AS edge_id
),
target AS (
  SELECT edge_id, topology.CreateTopoGeom('tt', 2, 2, ARRAY[ARRAY[edge_id, 2]]) AS g
  FROM edge
),
source AS (
  SELECT edge_id, topology.CreateTopoGeom('tt', 2, 2, ARRAY[ARRAY[-edge_id, 2]]) AS g
  FROM edge
)
SELECT edge_id, topology.TopoGeom_addTopoGeom(target.g, source.g) AS g
FROM target
JOIN source USING (edge_id);

SELECT 'tg2tg.lin2lin.reversed', ST_AsText(g::geometry)
FROM topogeom_addtopogeom_lineal;

SELECT 'tg2tg.lin2lin.reversed.components', count(*), bool_and(r.element_id = t.edge_id)
FROM topogeom_addtopogeom_lineal t
JOIN tt.relation r
ON r.topogeo_id = id(t.g)
AND r.layer_id = layer_id(t.g)
WHERE r.element_type = 2;

CREATE TEMP TABLE topogeom_addtopogeom_lineal_source_dupe AS
WITH edge AS (
  SELECT topology.AddEdge('tt', 'LINESTRING(12 0, 13 0)'::geometry) AS edge_id
),
target AS (
  SELECT edge_id, topology.CreateTopoGeom('tt', 2, 2) AS g
  FROM edge
),
source AS (
  SELECT
    edge_id,
    topology.CreateTopoGeom('tt', 2, 2, ARRAY[ARRAY[edge_id, 2], ARRAY[-edge_id, 2]]) AS g
  FROM edge
)
SELECT edge_id, topology.TopoGeom_addTopoGeom(target.g, source.g) AS g
FROM target
JOIN source USING (edge_id);

SELECT 'tg2tg.lin2lin.source-reversed', ST_AsText(g::geometry)
FROM topogeom_addtopogeom_lineal_source_dupe;

SELECT 'tg2tg.lin2lin.source-reversed.components', count(*), bool_and(r.element_id = t.edge_id)
FROM topogeom_addtopogeom_lineal_source_dupe t
JOIN tt.relation r
ON r.topogeo_id = id(t.g)
AND r.layer_id = layer_id(t.g)
WHERE r.element_type = 2;

-- poly to poly
SELECT 'tg2tg.pol2pol', ST_AsText(ST_Simplify(ST_Normalize(TopoGeom_addTopoGeom(
    toTopoGeom(ST_MakeEnvelope(0,10,20,10), 'tt', 3),
    toTopoGeom(ST_MakeEnvelope(0,0,10,10), 'tt', 3)
)::geometry), 0));

-- poly to mixed
SELECT 'tg2tg.pol2pol', ST_AsText(ST_Simplify(ST_Normalize(TopoGeom_addTopoGeom(
    toTopoGeom(ST_MakePoint(0,0), 'tt', 4),
    toTopoGeom(ST_MakeEnvelope(0,0,10,10), 'tt', 4)
)::geometry), 0));

CREATE TEMP TABLE topogeom_addtopogeom_hier_puntal AS
WITH inserted AS (
  INSERT INTO tt.f_puntal_child(g)
  SELECT toTopoGeom(geom, 'tt', 6)
  FROM (VALUES
    ('POINT(20 20)'::geometry),
    ('POINT(21 21)'::geometry)
  ) AS v(geom)
  RETURNING id, g
),
hier AS (
  SELECT ord, topology.CreateTopoGeom('tt', 1, 7, ARRAY[ARRAY[id(g), layer_id(g)]]) AS g
  FROM (
    SELECT row_number() OVER (ORDER BY id) AS ord, g
    FROM inserted
  ) ordered
)
SELECT topology.TopoGeom_addTopoGeom(target.g, source.g) AS g
FROM hier target
JOIN hier source ON target.ord = 1 AND source.ord = 2;

SELECT 'tg2tg.hier.poi2poi.child-layer-type', type(g), ST_AsText(g::geometry)
FROM topogeom_addtopogeom_hier_puntal;

WITH child AS (
  SELECT ord, toTopoGeom(geom, 'tt', 6) AS g
  FROM (VALUES
    (1, 'POINT(30 30)'::geometry),
    (2, 'POINT(31 31)'::geometry)
  ) AS v(ord, geom)
),
hier_point AS (
  SELECT topology.CreateTopoGeom('tt', 1, 7, ARRAY[ARRAY[id(g), layer_id(g)]]) AS g
  FROM child
  WHERE ord = 1
),
hier_collection AS (
  SELECT topology.CreateTopoGeom('tt', 4, 8, ARRAY[ARRAY[id(g), layer_id(g)]]) AS g
  FROM child
  WHERE ord = 2
)
SELECT 'tg2tg.hier.poi2coll.child-layer-type', type(topology.TopoGeom_addTopoGeom(hier_point.g, hier_collection.g))
FROM hier_point, hier_collection;

WITH child AS (
  SELECT ord, toTopoGeom(geom, 'tt', 6) AS g
  FROM (VALUES
    (1, 'POINT(40 40)'::geometry),
    (2, 'POINT(41 41)'::geometry)
  ) AS v(ord, geom)
),
hier_point AS (
  SELECT topology.CreateTopoGeom('tt', 1, 7, ARRAY[ARRAY[id(g), layer_id(g)]]) AS g
  FROM child
  WHERE ord = 1
),
hier_collection AS (
  SELECT topology.CreateTopoGeom('tt', 4, 8, ARRAY[ARRAY[id(g), layer_id(g)]]) AS g
  FROM child
  WHERE ord = 2
)
SELECT 'tg2tg.hier.coll2poi.child-layer-type', type(topology.TopoGeom_addTopoGeom(hier_collection.g, hier_point.g))
FROM hier_point, hier_collection;

-- TODO: point to point (hierarchical)
-- TODO: point to mixed (hierarchical)
-- TODO: line to line (hierarchical)
-- TODO: line to mixed (hierarchical)
-- TODO: poly to poly (hierarchical)
-- TODO: poly to mixed (hierarchical)
-- TODO: BOGUS calls (incompatible merges)

DROP TABLE tt.f_coll;
DROP TABLE tt.f_areal;
DROP TABLE tt.f_lineal;
DROP TABLE tt.f_hier_puntal_child_coll;
DROP TABLE tt.f_hier_puntal_child;
DROP TABLE tt.f_puntal_child;
DROP TABLE tt.f_hier_puntal;
DROP TABLE tt.f_puntal;
DROP TABLE topogeom_addtopogeom_hier_puntal;
DROP TABLE topogeom_addtopogeom_lineal;
DROP TABLE topogeom_addtopogeom_lineal_source_dupe;
select NULL from droptopology('tt');
