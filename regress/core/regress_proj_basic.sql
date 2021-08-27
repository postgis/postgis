
-- some transform() regression
-- prime spatial_ref_sys table with two projections
--- EPSG 100001 : WGS 84 / UTM zone 33N
INSERT INTO "spatial_ref_sys" ("srid","auth_name","auth_srid","srtext","proj4text") VALUES (100001,'EPSG',100001,'PROJCS["WGS 84 / UTM zone 33N",GEOGCS["WGS 84",DATUM["WGS_1984",SPHEROID["WGS 84",6378137,298.257223563,AUTHORITY["EPSG","7030"]],AUTHORITY["EPSG","6326"]],PRIMEM["Greenwich",0,AUTHORITY["EPSG","8901"]],UNIT["degree",0.01745329251994328,AUTHORITY["EPSG","9122"]],AUTHORITY["EPSG","1000002"]],PROJECTION["Transverse_Mercator"],PARAMETER["latitude_of_origin",0],PARAMETER["central_meridian",15],PARAMETER["scale_factor",0.9996],PARAMETER["false_easting",500000],PARAMETER["false_northing",0],UNIT["metre",1,AUTHORITY["EPSG","9001"]],AUTHORITY["EPSG","100001"]]','+proj=utm +zone=33 +ellps=WGS84 +datum=WGS84 +units=m +no_defs ');
--- EPSG 100002 : WGS 84
INSERT INTO "spatial_ref_sys" ("srid","auth_name","auth_srid","srtext","proj4text") VALUES (100002,'EPSG',100002,'GEOGCS["WGS 84",DATUM["WGS_1984",SPHEROID["WGS 84",6378137,298.257223563,AUTHORITY["EPSG","7030"]],AUTHORITY["EPSG","6326"]],PRIMEM["Greenwich",0,AUTHORITY["EPSG","8901"]],UNIT["degree",0.01745329251994328,AUTHORITY["EPSG","9122"]],AUTHORITY["EPSG","100002"]]','+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs ');

--- test #1: a simple projection
SELECT 1,ST_AsEWKT(ST_SnapToGrid(ST_transform(ST_GeomFromEWKT('SRID=100002;POINT(16 48)'),100001),10));

--- test #2: same in 3D
SELECT 2,ST_AsEWKT(ST_SnapToGrid(ST_transform(ST_GeomFromEWKT('SRID=100002;POINT(16 48 171)'),100001),10));

--- test #3: same in 4D
SELECT 3,ST_AsEWKT(ST_SnapToGrid(ST_transform(ST_GeomFromEWKT('SRID=100002;POINT(16 48 171 -500)'),100001),10));

--- test #4: LINESTRING projection, 2 points
SELECT 4,ST_AsEWKT(ST_SnapToGrid(ST_transform(ST_GeomFromEWKT('SRID=100002;LINESTRING(16 48, 16 49)'),100001),10));

--- test #5: LINESTRING projection, 2 points, 4D
SELECT 5,ST_AsEWKT(ST_SnapToGrid(ST_transform(ST_GeomFromEWKT('SRID=100002;LINESTRING(16 48 0 0, 16 49 0 0)'),100001),10));

DELETE FROM spatial_ref_sys WHERE srid in (100001, 100002);

SELECT 'M1', ST_AsText(ST_SnapToGrid(st_transform('SRID=4326;POINT(-20 -20)'::geometry, 3857),1));
SELECT 'M2', ST_AsText(ST_SnapToGrid(st_transform('SRID=4326;POINT(-20 -21.5)'::geometry, 3857),1));
SELECT 'M3', ST_AsText(ST_SnapToGrid(st_transform('SRID=4326;POINT(-30 -21.5)'::geometry, 3857),1));
SELECT 'M4', ST_AsText(ST_SnapToGrid(st_transform('SRID=4326;POINT(-72.345 41.3)'::geometry, 3857),1));
SELECT 'M5', ST_AsText(ST_SnapToGrid(st_transform('SRID=4326;POINT(71.999 -42.5)'::geometry, 3857),1));
