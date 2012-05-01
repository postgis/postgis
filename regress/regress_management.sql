-- $Id: regress_management.sql 9324 2012-02-27 22:08:12Z pramsey $
-- Test the populate_geometry_columns,DropGeometryTable etc --
\set VERBOSITY terse
DELETE FROM spatial_ref_sys WHERE srid = 4326;
INSERT INTO spatial_ref_sys ( srid, proj4text ) VALUES( 4326, '+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs');
CREATE TABLE test_pt(gid SERIAL PRIMARY KEY, geom geometry);
INSERT INTO test_pt(geom) VALUES(ST_GeomFromEWKT('SRID=4326;POINTM(1 2 3)'));
SELECT populate_geometry_columns('test_pt'::regclass);
SELECT 'The result: ' || DropGeometryTable('test_pt');
SELECT 'Unexistant: ' || DropGeometryTable('unexistent'); -- see ticket #861
DELETE FROM spatial_ref_sys WHERE srid = 4326;
