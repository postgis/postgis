\set VERBOSITY terse
set client_min_messages to WARNING;

select NULL FROM createtopology('tt_union');

CREATE TABLE tt_union.f_puntal(id serial);
SELECT 'simple_puntual_layer', AddTopoGeometryColumn('tt_union', 'tt_union', 'f_puntal', 'g', 'POINT');

SELECT 'simple_collection_layer', AddTopoGeometryColumn('tt_union', 'tt_union', 'f_puntal', 'g_coll', 'COLLECTION');

CREATE TABLE tt_union.f_hier_puntal(id serial, child_id integer);
SELECT 'hierarchical_puntual_layer', AddTopoGeometryColumn('tt_union', 'tt_union', 'f_hier_puntal', 'g', 'POINT', 1);

CREATE TABLE tt_union.f_hier_puntal_coll(id serial, child_id integer);
SELECT 'hierarchical_puntual_collection_layer', AddTopoGeometryColumn('tt_union', 'tt_union', 'f_hier_puntal_coll', 'g', 'COLLECTION', 1);

CREATE TABLE tt_union.f_lineal(id serial);
SELECT 'simple_lineal_layer', AddTopoGeometryColumn('tt_union', 'tt_union', 'f_lineal', 'g', 'LINE');

CREATE TABLE tt_union.f_puntal_child(id serial);
SELECT 'simple_puntual_child_layer', AddTopoGeometryColumn('tt_union', 'tt_union', 'f_puntal_child', 'g', 'POINT');

CREATE TABLE tt_union.f_hier_puntal_child(id serial, child_id integer);
SELECT 'hierarchical_puntual_child_layer', AddTopoGeometryColumn('tt_union', 'tt_union', 'f_hier_puntal_child', 'g', 'POINT', 6);

INSERT INTO tt_union.f_puntal(g) VALUES
  (toTopoGeom('POINT(0 0)', 'tt_union', 1)),
  (NULL),
  (toTopoGeom('POINT(1 1)', 'tt_union', 1));

SELECT 'union.null-only', topology.ST_Union(NULL::topology.topogeometry) IS NULL;

SELECT 'union.points', ST_AsText(topology.ST_Union(g ORDER BY id))
FROM tt_union.f_puntal;

SELECT 'source.point', id, ST_AsText(g::geometry)
FROM tt_union.f_puntal
WHERE g IS NOT NULL
ORDER BY id;

WITH src(ord, g) AS (
  VALUES
    (1, toTopoGeom('POINT(2 2)', 'tt_union', 2)),
    (2, toTopoGeom('LINESTRING(2 2, 3 3)', 'tt_union', 2))
)
SELECT 'union.collection', ST_AsText(topology.ST_Union(g ORDER BY ord))
FROM src;

INSERT INTO tt_union.f_hier_puntal(child_id, g)
SELECT id(g), topology.CreateTopoGeom('tt_union', 1, 3, ARRAY[ARRAY[id(g), layer_id(g)]])
FROM tt_union.f_puntal
WHERE g IS NOT NULL
ORDER BY id;

SELECT 'union.hierarchical.points', ST_AsText(topology.ST_Union(g ORDER BY id))
FROM tt_union.f_hier_puntal;

INSERT INTO tt_union.f_hier_puntal_coll(child_id, g)
SELECT child_id, topology.CreateTopoGeom('tt_union', 4, 4, ARRAY[ARRAY[child_id, 1]])
FROM tt_union.f_hier_puntal
ORDER BY id
LIMIT 1;

WITH src(ord, g) AS (
  SELECT 1, g FROM tt_union.f_hier_puntal
  UNION ALL
  SELECT 2, g FROM tt_union.f_hier_puntal_coll
)
SELECT 'union.hierarchical.point-collection', type(topology.ST_Union(g ORDER BY ord))
FROM src;

UPDATE tt_union.f_hier_puntal AS tgt
SET g = topology.TopoGeom_addTopoGeom(tgt.g, src.g)
FROM tt_union.f_hier_puntal_coll AS src
WHERE tgt.id = 1
RETURNING 'union.hierarchical.point-collection-update', type(tgt.g);

WITH src(ord, g) AS (
  SELECT 1, g FROM tt_union.f_hier_puntal_coll
  UNION ALL
  SELECT 2, g FROM tt_union.f_hier_puntal
)
SELECT 'union.hierarchical.collection-point', type(topology.ST_Union(g ORDER BY ord))
FROM src;

WITH child AS (
  INSERT INTO tt_union.f_puntal_child(g)
  SELECT toTopoGeom(geom, 'tt_union', 6)
  FROM (VALUES
    ('POINT(20 20)'::geometry),
    ('POINT(21 21)'::geometry)
  ) AS v(geom)
  RETURNING id, g
),
hier AS (
  INSERT INTO tt_union.f_hier_puntal_child(child_id, g)
  SELECT id(g), topology.CreateTopoGeom('tt_union', 1, 7, ARRAY[ARRAY[id(g), layer_id(g)]])
  FROM child
  RETURNING id, g
),
merged AS (
  SELECT topology.ST_Union(g ORDER BY id) AS g
  FROM hier
)
SELECT 'union.hierarchical.child-layer-points', type(g), ST_AsText(g::geometry)
FROM merged;

WITH edge AS (
  SELECT topology.AddEdge('tt_union', 'LINESTRING(10 0, 11 0)'::geometry) AS edge_id
)
INSERT INTO tt_union.f_lineal(g)
SELECT topology.CreateTopoGeom('tt_union', 2, 5, ARRAY[ARRAY[edge_id, 2]])
FROM edge
UNION ALL
SELECT topology.CreateTopoGeom('tt_union', 2, 5, ARRAY[ARRAY[-edge_id, 2]])
FROM edge;

CREATE TEMP TABLE topogeom_union_lineal AS
SELECT topology.ST_Union(g ORDER BY id) AS g
FROM tt_union.f_lineal;

SELECT 'union.reversed-line-edge.geometry', ST_AsText(g::geometry)
FROM topogeom_union_lineal;

SELECT 'union.reversed-line-edge.components', array_agg(r.element_id ORDER BY r.element_id)
FROM topogeom_union_lineal u
JOIN tt_union.relation r
ON r.topogeo_id = id(u.g)
AND r.layer_id = layer_id(u.g)
WHERE r.element_type = 2;

DROP TABLE topogeom_union_lineal;
DROP TABLE tt_union.f_hier_puntal_child;
DROP TABLE tt_union.f_puntal_child;
DROP TABLE tt_union.f_lineal;
DROP TABLE tt_union.f_hier_puntal_coll;
DROP TABLE tt_union.f_hier_puntal;
DROP TABLE tt_union.f_puntal;
select NULL from droptopology('tt_union');
