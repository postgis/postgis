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

INSERT INTO tt.f_puntal(g) VALUES
  (toTopoGeom('POINT(1 1)'::geometry, 'tt', 1)),
  (toTopoGeom('POINT(0 0)'::geometry, 'tt', 1)),
  (topology.CreateTopoGeom('tt', 1, 1));

INSERT INTO tt.f_hier_puntal(g)
SELECT topology.CreateTopoGeom('tt', 1, 5, ARRAY[ARRAY[id(g), layer_id(g)]]::topology.TopoElementArray)
FROM tt.f_puntal
WHERE id IN (1, 2);

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

WITH unioned AS (
  SELECT topology.ST_Union(a.g, b.g) AS g, a.g AS a, b.g AS b
  FROM tt.f_puntal a, tt.f_puntal b
  WHERE a.id = 1 AND b.id = 2
)
SELECT 'st_union.poi2poi',
  ST_AsText(g),
  id(g) NOT IN (id(a), id(b)),
  GetTopoGeomElementArray(a),
  GetTopoGeomElementArray(b)
FROM unioned;

WITH unioned AS (
  SELECT topology.ST_Union(a.g, b.g) AS g, a.g AS a, b.g AS b
  FROM tt.f_puntal a, tt.f_puntal b
  WHERE a.id = 3 AND b.id = 1
)
SELECT 'st_union.empty_poi',
  ST_AsText(g),
  id(g) NOT IN (id(a), id(b)),
  GetTopoGeomElementArray(a),
  GetTopoGeomElementArray(b)
FROM unioned;

INSERT INTO tt.f_hier_puntal(g)
SELECT topology.ST_Union(a.g, b.g)
FROM tt.f_hier_puntal a, tt.f_hier_puntal b
WHERE a.id = 1 AND b.id = 2;

SELECT 'st_union.hier_poi2poi',
  ST_AsText(u.g),
  id(u.g) NOT IN (id(a.g), id(b.g)),
  (SELECT topology.TopoElementArray_agg(ARRAY[element_id, element_type]
    ORDER BY element_id, element_type)
   FROM tt.relation
   WHERE topogeo_id = id(u.g)
     AND layer_id = layer_id(u.g)),
  (SELECT topology.TopoElementArray_agg(ARRAY[element_id, element_type])
   FROM tt.relation
   WHERE topogeo_id = id(a.g)
     AND layer_id = layer_id(a.g)),
  (SELECT topology.TopoElementArray_agg(ARRAY[element_id, element_type])
   FROM tt.relation
   WHERE topogeo_id = id(b.g)
     AND layer_id = layer_id(b.g))
FROM tt.f_hier_puntal u, tt.f_hier_puntal a, tt.f_hier_puntal b
WHERE u.id = 3 AND a.id = 1 AND b.id = 2;

CREATE FUNCTION tt.try_st_union(a topology.TopoGeometry, b topology.TopoGeometry)
RETURNS text
AS $$
BEGIN
  PERFORM topology.ST_Union(a, b);
  RETURN 'unexpected success';
EXCEPTION WHEN OTHERS THEN
  RETURN SQLERRM;
END
$$ LANGUAGE 'plpgsql' VOLATILE;

SELECT 'st_union.hier_simple', tt.try_st_union(h.g, s.g)
FROM tt.f_hier_puntal h, tt.f_puntal s
WHERE h.id = 1 AND s.id = 1;

SELECT 'st_union.simple_hier', tt.try_st_union(s.g, h.g)
FROM tt.f_puntal s, tt.f_hier_puntal h
WHERE s.id = 1 AND h.id = 1;

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
DROP TABLE tt.f_hier_puntal;
DROP TABLE tt.f_puntal;
DROP FUNCTION tt.try_st_union(topology.TopoGeometry, topology.TopoGeometry);
select NULL from droptopology('tt');
