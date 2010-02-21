--
-- spatial_ref_sys data
--
INSERT INTO "spatial_ref_sys" ("srid","auth_name","auth_srid","srtext","proj4text") VALUES (4326,'EPSG',4326,'GEOGCS["WGS 84",DATUM["WGS_1984",SPHEROID["WGS 84",6378137,298.257223563,AUTHORITY["EPSG","7030"]],TOWGS84[0,0,0,0,0,0,0],AUTHORITY["EPSG","6326"]],PRIMEM["Greenwich",0,AUTHORITY["EPSG","8901"]],UNIT["degree",0.01745329251994328,AUTHORITY["EPSG","9122"]],AUTHORITY["EPSG","4326"]]','+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs ');

-- Empty Geometry
SELECT 'empty_geom', ST_AsGML(GeomFromEWKT(NULL));

-- Precision
SELECT 'precision_01', ST_AsGML(GeomFromEWKT('SRID=4326;POINT(1.1111111 1.1111111)'), -2);
SELECT 'precision_02', ST_AsGML(GeomFromEWKT('SRID=4326;POINT(1.1111111 1.1111111)'), 19);

-- Version
SELECT 'version_01', ST_AsGML(2, GeomFromEWKT('SRID=4326;POINT(1 1)'));
SELECT 'version_02', ST_AsGML(3, GeomFromEWKT('SRID=4326;POINT(1 1)'));
SELECT 'version_03', ST_AsGML(21, GeomFromEWKT('SRID=4326;POINT(1 1)'));
SELECT 'version_04', ST_AsGML(-4, GeomFromEWKT('SRID=4326;POINT(1 1)'));

-- Option
SELECT 'option_01', ST_AsGML(2, GeomFromEWKT('SRID=4326;POINT(1 1)'), 0, 0);
SELECT 'option_02', ST_AsGML(3, GeomFromEWKT('SRID=4326;POINT(1 1)'), 0, 1);

-- Deegree data
SELECT 'deegree_01', ST_AsGML(3, GeomFromEWKT('SRID=4326;POINT(1 2)'), 0, 0);
SELECT 'deegree_02', ST_AsGML(2, GeomFromEWKT('SRID=4326;POINT(1 2)'), 0, 16);
SELECT 'deegree_03', ST_AsGML(3, GeomFromEWKT('SRID=4326;POINT(1 2)'), 0, 16);
SELECT 'deegree_04', ST_AsGML(3, GeomFromEWKT('SRID=4326;POINT(1 2 3)'), 0, 0);
SELECT 'deegree_05', ST_AsGML(2, GeomFromEWKT('SRID=4326;POINT(1 2 3)'), 0, 16);
SELECT 'deegree_06', ST_AsGML(3, GeomFromEWKT('SRID=4326;POINT(1 2 3)'), 0, 16);
SELECT 'deegree_07', ST_AsGML(3, GeomFromEWKT('SRID=4326;LINESTRING(1 2, 2 3, 4 5)'), 0, 0);
SELECT 'deegree_08', ST_AsGML(2, GeomFromEWKT('SRID=4326;LINESTRING(1 2, 2 3, 4 5)'), 0, 16);
SELECT 'deegree_09', ST_AsGML(3, GeomFromEWKT('SRID=4326;LINESTRING(1 2, 2 3, 4 5)'), 0, 16);
SELECT 'deegree_10', ST_AsGML(3, GeomFromEWKT('SRID=4326;POLYGON((1 2, 2 3, 4 5, 3 2, 1 2))'), 0, 0);
SELECT 'deegree_11', ST_AsGML(2, GeomFromEWKT('SRID=4326;POLYGON((1 2, 2 3, 4 5, 3 2, 1 2))'), 0, 16);
SELECT 'deegree_12', ST_AsGML(3, GeomFromEWKT('SRID=4326;POLYGON((1 2, 2 3, 4 5, 3 2, 1 2))'), 0, 16);
SELECT 'deegree_13', ST_AsGML(3, GeomFromEWKT('SRID=4326;MULTIPOINT(1 2, 2 3, 4 5)'), 0, 0);
SELECT 'deegree_14', ST_AsGML(2, GeomFromEWKT('SRID=4326;MULTIPOINT(1 2, 2 3, 4 5)'), 0, 16);
SELECT 'deegree_15', ST_AsGML(3, GeomFromEWKT('SRID=4326;MULTIPOINT(1 2, 2 3, 4 5)'), 0, 16);
SELECT 'deegree_16', ST_AsGML(3, GeomFromEWKT('SRID=4326;MULTILINESTRING((1 2, 2 3, 4 5))'), 0, 0);
SELECT 'deegree_17', ST_AsGML(2, GeomFromEWKT('SRID=4326;MULTILINESTRING((1 2, 2 3, 4 5))'), 0, 16);
SELECT 'deegree_18', ST_AsGML(3, GeomFromEWKT('SRID=4326;MULTILINESTRING((1 2, 2 3, 4 5))'), 0, 16);
SELECT 'deegree_19', ST_AsGML(3, GeomFromEWKT('SRID=4326;MULTIPOLYGON(((1 2, 2 3, 4 5, 3 2, 1 2)))'), 0, 0);
SELECT 'deegree_20', ST_AsGML(2, GeomFromEWKT('SRID=4326;MULTIPOLYGON(((1 2, 2 3, 4 5, 3 2, 1 2)))'), 0, 16);
SELECT 'deegree_21', ST_AsGML(3, GeomFromEWKT('SRID=4326;MULTIPOLYGON(((1 2, 2 3, 4 5, 3 2, 1 2)))'), 0, 16);
SELECT 'deegree_22', ST_AsGML(3, GeomFromEWKT('SRID=4326;GEOMETRYCOLLECTION(MULTIPOLYGON(((1 2, 2 3, 4 5, 3 2, 1 2))), MULTILINESTRING((1 2, 2 3, 4 5)), MULTIPOINT(1 2, 3 4))'), 0, 0);
SELECT 'deegree_23', ST_AsGML(2, GeomFromEWKT('SRID=4326;GEOMETRYCOLLECTION(MULTIPOLYGON(((1 2, 2 3, 4 5, 3 2, 1 2))), MULTILINESTRING((1 2, 2 3, 4 5)), MULTIPOINT(1 2, 3 4))'), 0, 16);
SELECT 'deegree_24', ST_AsGML(3, GeomFromEWKT('SRID=4326;GEOMETRYCOLLECTION(MULTIPOLYGON(((1 2, 2 3, 4 5, 3 2, 1 2))), MULTILINESTRING((1 2, 2 3, 4 5)), MULTIPOINT(1 2, 3 4))'), 0, 16);


--
-- Delete inserted spatial data
--
DELETE FROM spatial_ref_sys WHERE srid = 4326;
