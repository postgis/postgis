---
--- Regression tests for PostGIS SFCGAL backend
---

-- We only care about testing PostGIS prototype here
-- Behaviour is already handled by SFCGAL own tests

SELECT 'postgis_sfcgal_version', count(*) FROM (SELECT postgis_sfcgal_version()) AS f;
SELECT 'ST_Tesselate', ST_AsText(ST_Tesselate('GEOMETRYCOLLECTION(POINT(4 4),POLYGON((0 0,1 0,1 1,0 1,0 0)))'));
SELECT 'ST_3DArea', ST_3DArea('POLYGON((0 0 0,1 0 0,1 1 0,0 1 0,0 0 0))');
SELECT 'ST_Extrude_point', ST_AsText(ST_Extrude('POINT(0 0)', 1, 0, 0));
SELECT 'ST_Extrude_line', ST_AsText(ST_Extrude(ST_Extrude('POINT(0 0)', 1, 0, 0), 0, 1, 0));
SELECT 'ST_Extrude_surface', ST_AsText(ST_Extrude(ST_Extrude(ST_Extrude('POINT(0 0)', 1, 0, 0), 0, 1, 0), 0, 0, 1));
SELECT 'ST_ForceLHR', ST_AsText(ST_ForceLHR('POLYGON((0 0,0 1,1 1,1 0,0 0))'));
SELECT 'ST_Orientation_1', ST_Orientation(ST_ForceLHR('POLYGON((0 0,0 1,1 1,1 0,0 0))'));
SELECT 'ST_Orientation_2', ST_Orientation(ST_ForceRHR('POLYGON((0 0,0 1,1 1,1 0,0 0))'));
SELECT 'ST_MinkowskiSum', ST_AsText(ST_MinkowskiSum('LINESTRING(0 0,4 0)','POLYGON((0 0,1 0,1 1,0 1,0 0))'));
SELECT 'ST_StraightSkeleton', ST_AsText(ST_StraightSkeleton('POLYGON((0 0,1 0,1 1,0 1,0 0))'));

-- Backend switch tests
SET postgis.backend = 'geos';
SELECT 'intersection_geos', ST_astext(ST_intersection('LINESTRING(0 10, 0 -10)', 'LINESTRING(0 0, 1 1)'));

SET postgis.backend = 'sfcgal';
SELECT 'intersection_sfcgal', ST_astext(ST_intersection('LINESTRING(0 10, 0 -10)', 'LINESTRING(0 0, 1 1)'));

SET postgis.backend = 'foo';
SET postgis.backend = '';
