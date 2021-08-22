
-- some transform() regression
-- prime spatial_ref_sys table with two projections
--- EPSG 100001 : WGS 84 / UTM zone 33N
INSERT INTO "spatial_ref_sys" ("srid","auth_name","auth_srid","srtext","proj4text") VALUES (100001,'EPSG',100001,'PROJCS["WGS 84 / UTM zone 33N",GEOGCS["WGS 84",DATUM["WGS_1984",SPHEROID["WGS 84",6378137,298.257223563,AUTHORITY["EPSG","7030"]],AUTHORITY["EPSG","6326"]],PRIMEM["Greenwich",0,AUTHORITY["EPSG","8901"]],UNIT["degree",0.01745329251994328,AUTHORITY["EPSG","9122"]],AUTHORITY["EPSG","1000002"]],PROJECTION["Transverse_Mercator"],PARAMETER["latitude_of_origin",0],PARAMETER["central_meridian",15],PARAMETER["scale_factor",0.9996],PARAMETER["false_easting",500000],PARAMETER["false_northing",0],UNIT["metre",1,AUTHORITY["EPSG","9001"]],AUTHORITY["EPSG","100001"]]','+proj=utm +zone=33 +ellps=WGS84 +datum=WGS84 +units=m +no_defs ');
--- EPSG 100002 : WGS 84
INSERT INTO "spatial_ref_sys" ("srid","auth_name","auth_srid","srtext","proj4text") VALUES (100002,'EPSG',100002,'GEOGCS["WGS 84",DATUM["WGS_1984",SPHEROID["WGS 84",6378137,298.257223563,AUTHORITY["EPSG","7030"]],AUTHORITY["EPSG","6326"]],PRIMEM["Greenwich",0,AUTHORITY["EPSG","8901"]],UNIT["degree",0.01745329251994328,AUTHORITY["EPSG","9122"]],AUTHORITY["EPSG","100002"]]','+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs ');

-- Repeat all tests with the new function names.
--- test #0: NULL values
SELECT 0,coalesce(ST_AsText(ST_transform(NULL::geometry, 100001)),'EMPTY');

--- test #6: re-projecting a projected value
SELECT 6,round(ST_X(ST_transform(ST_transform(ST_GeomFromEWKT('SRID=100002;POINT(16 48)'),100001), 100002))::numeric,8),round(ST_Y(ST_transform(ST_transform(ST_GeomFromEWKT('SRID=100002;POINT(16 48)'),100001), 100002))::numeric,8);

--- test #7: Should yield an error since input SRID is unknown
SELECT 7, ST_transform(ST_GeomFromEWKT('SRID=0;POINT(0 0)'),100002);

--- test #8: Transforming to same SRID
SELECT 8,ST_AsEWKT(ST_transform(ST_GeomFromEWKT('SRID=100002;POINT(0 0)'),100002));

SELECT 9, ST_AsEWKT(ST_SnapToGrid(ST_Transform(
               ST_GeomFromEWKT('SRID=100002;POINT(16 48)'),
               '+proj=utm +zone=33 +ellps=WGS84 +datum=WGS84 +units=m +no_defs '), 10));

--- test #10: Transform from_proj to_proj
SELECT 10, ST_AsEWKT(ST_SnapToGrid(ST_Transform(
               ST_GeomFromEWKT('POINT(16 48)'),
               '+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs ',
               '+proj=utm +zone=33 +ellps=WGS84 +datum=WGS84 +units=m +no_defs '), 10));

--- test #11: Transform from_proj to_srid
SELECT 11, ST_AsEWKT(ST_SnapToGrid(ST_Transform(
               ST_GeomFromEWKT('POINT(16 48)'),
               '+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs ', 100001), 10));

--- test #12: Transform with bad to_proj
SELECT 12, ST_AsEWKT(ST_Transform(
           ST_GeomFromEWKT('SRID=100002;POINT(16 48)'),
           'invalid projection'));

DELETE FROM spatial_ref_sys WHERE srid in (100001, 100002);
