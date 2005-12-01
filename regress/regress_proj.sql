
-- some transform() regression

-- prime spatial_ref_sys table with two projections

--- EPSG 1000001 : WGS 84 / UTM zone 33N
INSERT INTO "spatial_ref_sys" ("srid","auth_name","auth_srid","srtext","proj4text") VALUES (1000001,'EPSG',1000001,'PROJCS["WGS 84 / UTM zone 33N",GEOGCS["WGS 84",DATUM["WGS_1984",SPHEROID["WGS 84",6378137,298.257223563,AUTHORITY["EPSG","7030"]],AUTHORITY["EPSG","6326"]],PRIMEM["Greenwich",0,AUTHORITY["EPSG","8901"]],UNIT["degree",0.01745329251994328,AUTHORITY["EPSG","9122"]],AUTHORITY["EPSG","1000002"]],PROJECTION["Transverse_Mercator"],PARAMETER["latitude_of_origin",0],PARAMETER["central_meridian",15],PARAMETER["scale_factor",0.9996],PARAMETER["false_easting",500000],PARAMETER["false_northing",0],UNIT["metre",1,AUTHORITY["EPSG","9001"]],AUTHORITY["EPSG","1000001"]]','+proj=utm +zone=33 +ellps=WGS84 +datum=WGS84 +units=m +no_defs ');
--- EPSG 1000002 : WGS 84
INSERT INTO "spatial_ref_sys" ("srid","auth_name","auth_srid","srtext","proj4text") VALUES (1000002,'EPSG',1000002,'GEOGCS["WGS 84",DATUM["WGS_1984",SPHEROID["WGS 84",6378137,298.257223563,AUTHORITY["EPSG","7030"]],AUTHORITY["EPSG","6326"]],PRIMEM["Greenwich",0,AUTHORITY["EPSG","8901"]],UNIT["degree",0.01745329251994328,AUTHORITY["EPSG","9122"]],AUTHORITY["EPSG","1000002"]]','+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs ');

--- test #0: NULL values
SELECT 0,coalesce(AsText(transform(NULL, 1000001)),'EMPTY');

--- test #1: a simple projection
SELECT 1,AsEWKT(SnapToGrid(transform(GeomFromEWKT('SRID=1000002;POINT(16 48)'),1000001),10));

--- test #2: same in 3D
SELECT 2,AsEWKT(SnapToGrid(transform(GeomFromEWKT('SRID=1000002;POINT(16 48 171)'),1000001),10));

--- test #3: same in 4D
SELECT 3,AsEWKT(SnapToGrid(transform(GeomFromEWKT('SRID=1000002;POINT(16 48 171 -500)'),1000001),10));

--- test #4: LINESTRING projection, 2 points
SELECT 4,AsEWKT(SnapToGrid(transform(GeomFromEWKT('SRID=1000002;LINESTRING(16 48, 16 49)'),1000001),10));

--- test #5: LINESTRING projection, 2 points, 4D
SELECT 5,AsEWKT(SnapToGrid(transform(GeomFromEWKT('SRID=1000002;LINESTRING(16 48 0 0, 16 49 0 0)'),1000001),10));

--- test #6: re-projecting a projected value
SELECT 6,round(X(transform(transform(GeomFromEWKT('SRID=1000002;POINT(16 48)'),1000001), 1000002))::numeric,8),round(Y(transform(transform(GeomFromEWKT('SRID=1000002;POINT(16 48)'),1000001), 1000002))::numeric,8);

--- test #7: Should yield an error since input SRID is unknown
SELECT transform(GeomFromEWKT('SRID=-1;POINT(0 0)'),1000002);

--- test #8: Transforming to same SRID
SELECT 8,AsEWKT(transform(GeomFromEWKT('SRID=1000002;POINT(0 0)'),1000002));

DELETE FROM spatial_ref_sys WHERE srid >= 1000000;

