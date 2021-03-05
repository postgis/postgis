\set VERBOSITY terse
set client_min_messages to WARNING;

select NULL FROM createtopology('tt', 4326);

-- layer 1 is PUNTUAL
CREATE TABLE tt.f_point(id serial);
SELECT NULL FROM AddTopoGeometryColumn('tt', 'tt', 'f_point', 'g', 'POINT');

-- layer 2 is LINEAL
CREATE TABLE tt.f_line(id serial);
SELECT NULL FROM AddTopoGeometryColumn('tt', 'tt', 'f_line', 'g', 'LINE');

-- layer 3 is AREAL
CREATE TABLE tt.f_area(id serial);
SELECT NULL FROM AddTopoGeometryColumn('tt', 'tt', 'f_area', 'g', 'POLYGON');

-- layer 4 is MIXED
CREATE TABLE tt.f_coll(id serial);
SELECT NULL FROM AddTopoGeometryColumn('tt', 'tt', 'f_coll', 'g', 'COLLECTION');

-- layer 5 is HIERARCHICAL PUNTUAL
CREATE TABLE tt.f_hier_point(id serial);
SELECT NULL FROM AddTopoGeometryColumn('tt', 'tt', 'f_hier_point', 'g', 'POINT', 1);

-- layer 6 is HIERARCHICAL LINEAL
CREATE TABLE tt.f_hier_line(id serial);
SELECT NULL FROM AddTopoGeometryColumn('tt', 'tt', 'f_hier_line', 'g', 'LINE', 2);

-- layer 7 is HIERARCHICAL AREAL
CREATE TABLE tt.f_hier_area(id serial);
SELECT NULL FROM AddTopoGeometryColumn('tt', 'tt', 'f_hier_area', 'g', 'POLYGON', 3);

-- layer 8 is HIERARCHICAL MIXED
CREATE TABLE tt.f_hier_coll(id serial);
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

-- Cleanup
SELECT NULL FROM DropTopology('tt');
