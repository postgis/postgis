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

-- Create simple puntual layer ( will be layer 2 )
CREATE TABLE tt.f_puntal(id serial);
SELECT 'simple_puntual_layer', AddTopoGeometryColumn('tt', 'tt', 'f_puntal','g','POINT');

-- Create a hierarchical layer (will be layer 2)
CREATE TABLE tt.f_hier(id serial);
SELECT 'hierarchical_layer', AddTopoGeometryColumn('tt', 'tt', 'f_hier','g','COLLECTION', 1);

-- A couple more invalid calls
select totopogeom('POINT(0 0)'::geometry, 'tt', 3); -- non existent layer
select totopogeom('POINT(0 0)'::geometry, 'tt', 2); -- invalid (hierarchical) layer
-- TODO: add more invalid calls due to type mismatch


DROP TABLE tt.f_hier;
DROP TABLE tt.f_puntal;
select droptopology('tt');
