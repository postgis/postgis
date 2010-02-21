-- Regress test for ST_AsKML 

--- EPSG 4326 : WGS 84
INSERT INTO "spatial_ref_sys" ("srid", "proj4text") VALUES (4326, '+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs ');

--- EPSG 1021892 : Bogota 1975 / Colombia Bogota zone (deprecated)
INSERT INTO "spatial_ref_sys" ("srid", "proj4text") VALUES (1021892, '+proj=tmerc +lat_0=4.599047222222222 +lon_0=-74.08091666666667 +k=1.000000 +x_0=1000000 +y_0=1000000 +ellps=intl +towgs84=307,304,-318,0,0,0,0 +units=m +no_defs ');


-- SRID
SELECT 'srid_01', ST_AsKML(GeomFromEWKT('SRID=10;POINT(0 1)'));
SELECT 'srid_02', ST_AsKML(GeomFromEWKT('POINT(0 1)'));

-- Empty Geometry
SELECT 'empty_geom', ST_AsKML(GeomFromEWKT(NULL));

-- Precision
SELECT 'precision_01', ST_AsKML(ST_GeomFromEWKT('SRID=4326;POINT(1.1111111 1.1111111)'), -2);
SELECT 'precision_02', ST_AsKML(ST_GeomFromEWKT('SRID=4326;POINT(1.1111111 1.1111111)'), 19);

-- Version
SELECT 'version_01', ST_AsKML(2, GeomFromEWKT('SRID=4326;POINT(1 1)'));
SELECT 'version_02', ST_AsKML(3, GeomFromEWKT('SRID=4326;POINT(1 1)'));
SELECT 'version_03', ST_AsKML(-4, GeomFromEWKT('SRID=4326;POINT(1 1)'));

-- Projected 
-- National Astronomical Observatory of Colombia - Bogota, Colombia (Placemark)
SELECT 'projection_01', ST_AsKML(ST_GeomFromEWKT('SRID=1021892;POINT(1000000 1000000)'), 3);


DELETE FROM spatial_ref_sys;
