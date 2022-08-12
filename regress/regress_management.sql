-- Test the populate_geometry_columns,DropGeometryTable etc --
\set VERBOSITY terse
SET client_min_messages TO warning;
CREATE TABLE test_pt(gid SERIAL PRIMARY KEY, geom geometry);
INSERT INTO test_pt(geom) VALUES(ST_GeomFromEWKT('SRID=4326;POINT M(1 2 3)'));
SELECT populate_geometry_columns('test_pt'::regclass);
SELECT 'The result: ' || DropGeometryTable('test_pt');
SELECT 'Unexistant: ' || DropGeometryTable('unexistent'); -- see ticket #861
