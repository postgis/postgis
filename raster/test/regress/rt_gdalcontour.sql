DROP TABLE IF EXISTS contour_raster;


CREATE TABLE contour_raster AS
SELECT ST_SetValues(
ST_AddBand(
ST_MakeEmptyRaster(7,7, 1, 1, 1, 1, 0, 0, srid => 4326)
, 1, '16BSI', 0, NULL)
, 1, 1, 1,
ARRAY[
[1,1,1,1,1,1,1],
[1,2,2,2,2,2,1],
[1,2,3,3,3,2,1],
[1,2,3,4,3,2,1],
[1,2,3,3,3,2,1],
[1,2,2,2,2,2,1],
[1,1,1,1,1,1,1]]::float8[][]) AS rast, 1 as rid;


INSERT INTO contour_raster
SELECT ST_SetValues(
ST_AddBand(
ST_MakeEmptyRaster(7,7, 1, 1, 1, 1, 0, 0, srid => 4326)
, 1, '16BSI', 0, NULL)
, 1, 1, 1,
ARRAY[
[1,1,1,1,1,1,1],
[1,2,2,2,2,2,2],
[1,2,3,3,3,3,3],
[1,2,3,4,4,4,4],
[1,2,3,4,5,5,5],
[1,2,2,4,5,6,6],
[1,2,3,4,5,6,7]]::float8[][]) AS rast, 2 as rid;

WITH c AS (
SELECT (ST_Contour(rast, 1, fixed_levels => ARRAY[3.0])).*
FROM contour_raster WHERE rid = 1
)
SELECT 'aa', st_geometrytype(st_snaptogrid(geom, 0.01)), id, value
FROM c LIMIT 1;

WITH c AS (
SELECT (ST_Contour(rast, 1, fixed_levels => ARRAY[2.0])).*
FROM contour_raster WHERE rid = 1
)
SELECT 'ab', st_geometrytype(st_snaptogrid(geom, 0.01)), id, value
FROM c LIMIT 1;


WITH c AS (
SELECT (ST_Contour(rast, 1, fixed_levels => ARRAY[2.0])).*
FROM contour_raster WHERE rid = 2
)
SELECT 'ac', st_geometrytype(st_snaptogrid(geom, 0.01)), id, value
FROM c LIMIT 1;


WITH c AS (
SELECT (ST_Contour(rast, 1, fixed_levels => ARRAY[6.0])).*
FROM contour_raster WHERE rid = 2
)
SELECT 'ad', st_geometrytype(st_snaptogrid(st_simplify(geom,0.01), 0.01)), id, value
FROM c LIMIT 1;

WITH c AS (
SELECT (ST_Contour(rast, 1, fixed_levels => ARRAY[3.0])).*
FROM contour_raster WHERE rid = 2
)
SELECT 'ae', st_geometrytype(st_snaptogrid(st_simplify(geom,0.01), 0.01)), id, value
FROM c LIMIT 1;

DROP TABLE IF EXISTS contour_raster;

