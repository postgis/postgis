\set VERBOSITY terse
set client_min_messages to ERROR;

select 'create', createtopology('tt') > 0;

-- Invalid calls
select totopogeom('POINT(0 0)'::geometry, 'unexistent', 1);
select totopogeom('POINT(0 0)'::geometry, 'tt', 1);
select totopogeom(null, 'tt', 1);
select totopogeom('POINT(0 0)'::geometry, '', 1);
select totopogeom('POINT(0 0)'::geometry, null, 1);
select totopogeom('POINT(0 0)'::geometry, 'tt', null);

-- Create simple puntual layer (will be layer 1)
CREATE TABLE tt.f_puntal(id serial);
SELECT 'simple_puntual_layer', AddTopoGeometryColumn('tt', 'tt', 'f_puntal','g','POINT');

-- Create a hierarchical layer (will be layer 2)
CREATE TABLE tt.f_hier(id serial);
SELECT 'hierarchical_layer', AddTopoGeometryColumn('tt', 'tt', 'f_hier','g','COLLECTION', 1);

-- Create a lineal layer (will be layer 3)
CREATE TABLE tt.f_lineal(id serial);
SELECT 'simple_lineal_layer', AddTopoGeometryColumn('tt', 'tt', 'f_lineal','g','LINE');

-- Create an areal layer (will be layer 4)
CREATE TABLE tt.f_areal(id serial);
SELECT 'simple_areal_layer', AddTopoGeometryColumn('tt', 'tt', 'f_areal','g','POLYGON');

-- Create a collection layer (will be layer 5)
CREATE TABLE tt.f_coll(id serial);
SELECT 'simple_collection_layer', AddTopoGeometryColumn('tt', 'tt', 'f_coll','g','COLLECTION');

-- A couple more invalid calls
select totopogeom('POINT(0 0)'::geometry, 'tt', 30); -- non existent layer
select totopogeom('POINT(0 0)'::geometry, 'tt', 2); -- invalid (hierarchical) layer
select totopogeom('LINESTRING(0 0, 10 10)'::geometry, 'tt', 1); -- invalid (puntual) layer
select totopogeom('LINESTRING(0 0, 10 10)'::geometry, 'tt', 4); -- invalid (areal) layer
select totopogeom('MULTIPOINT(0 0, 10 10)'::geometry, 'tt', 3); -- invalid (lineal) layer
select totopogeom('MULTIPOINT(0 0, 10 10)'::geometry, 'tt', 4); -- invalid (areal) layer
select totopogeom('POLYGON((0 0, 10 10, 10 0, 0 0))'::geometry, 'tt', 1); -- invalid (puntal) layer
select totopogeom('POLYGON((0 0, 10 10, 10 0, 0 0))'::geometry, 'tt', 3); -- invalid (lineal) layer



DROP TABLE tt.f_coll;
DROP TABLE tt.f_areal;
DROP TABLE tt.f_lineal;
DROP TABLE tt.f_hier;
DROP TABLE tt.f_puntal;
select droptopology('tt');
