--
-- spatial_ref_sys data
--
INSERT INTO "spatial_ref_sys" ("srid","auth_name","auth_srid","proj4text") VALUES (4326,'EPSG',4326,'+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs ');


-- Empty Geometry
SELECT 'empty_geom', ST_AsGeoJson(GeomFromEWKT(NULL));

-- Precision
SELECT 'precision_01', ST_AsGeoJson(GeomFromEWKT('SRID=4326;POINT(1.1111111 1.1111111)'), -2);
SELECT 'precision_02', ST_AsGeoJson(GeomFromEWKT('SRID=4326;POINT(1.1111111 1.1111111)'), 19);


-- Version
SELECT 'version_01', ST_AsGeoJson(1, GeomFromEWKT('SRID=4326;POINT(1 1)'));
SELECT 'version_02', ST_AsGeoJson(21, GeomFromEWKT('SRID=4326;POINT(1 1)'));
SELECT 'version_03', ST_AsGeoJson(-4, GeomFromEWKT('SRID=4326;POINT(1 1)'));


-- CRS
SELECT 'crs_01', ST_AsGeoJson(GeomFromEWKT('SRID=4326;POINT(1 1)'), 0, 2);
SELECT 'crs_02', ST_AsGeoJson(GeomFromEWKT('SRID=-1;POINT(1 1)'), 0, 2);
SELECT 'crs_03', ST_AsGeoJson(GeomFromEWKT('SRID=4326;POINT(1 1)'), 0, 4);
SELECT 'crs_04', ST_AsGeoJson(GeomFromEWKT('SRID=-1;POINT(1 1)'), 0, 4);
SELECT 'crs_05', ST_AsGeoJson(GeomFromEWKT('SRID=1;POINT(1 1)'), 0, 2);
SELECT 'crs_06', ST_AsGeoJson(GeomFromEWKT('SRID=1;POINT(1 1)'), 0, 4);

-- Bbox
SELECT 'bbox_01', ST_AsGeoJson(GeomFromEWKT('LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0);
SELECT 'bbox_02', ST_AsGeoJson(GeomFromEWKT('LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0, 1);
SELECT 'bbox_03', ST_AsGeoJson(GeomFromEWKT('LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0, 3);
SELECT 'bbox_04', ST_AsGeoJson(GeomFromEWKT('LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0, 5);

-- CRS and Bbox
SELECT 'options_01', ST_AsGeoJson(GeomFromEWKT('SRID=-1;LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0, 0);
SELECT 'options_02', ST_AsGeoJson(GeomFromEWKT('SRID=4326;LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0);
SELECT 'options_03', ST_AsGeoJson(GeomFromEWKT('SRID=-1;LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0, 1);
SELECT 'options_04', ST_AsGeoJson(GeomFromEWKT('SRID=4326;LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0, 1);
SELECT 'options_05', ST_AsGeoJson(GeomFromEWKT('SRID=-1;LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0, 2);
SELECT 'options_06', ST_AsGeoJson(GeomFromEWKT('SRID=4326;LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0, 2);
SELECT 'options_07', ST_AsGeoJson(GeomFromEWKT('SRID=-1;LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0, 3);
SELECT 'options_08', ST_AsGeoJson(GeomFromEWKT('SRID=4326;LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0, 3);
SELECT 'options_09', ST_AsGeoJson(GeomFromEWKT('SRID=-1;LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0, 4);
SELECT 'options_10', ST_AsGeoJson(GeomFromEWKT('SRID=4326;LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0, 4);
SELECT 'options_10', ST_AsGeoJson(GeomFromEWKT('SRID=-1;LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0, 5);
SELECT 'options_11', ST_AsGeoJson(GeomFromEWKT('SRID=4326;LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0, 5);
SELECT 'options_12', ST_AsGeoJson(GeomFromEWKT('SRID=-1;LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0, 6);
SELECT 'options_13', ST_AsGeoJson(GeomFromEWKT('SRID=4326;LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0, 6);
SELECT 'options_14', ST_AsGeoJson(GeomFromEWKT('SRID=-1;LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0, 7);
SELECT 'options_15', ST_AsGeoJson(GeomFromEWKT('SRID=4326;LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0, 7);



--
-- Delete inserted spatial data
--
DELETE FROM spatial_ref_sys;
