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
select NULL from droptopology('tt');
