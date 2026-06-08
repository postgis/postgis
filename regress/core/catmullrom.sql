-- ST_CatmullRomSmoothing regression tests

-- Basic 4-point collinear line, default nSegments=5
-- Scale by 5 so interpolated values are integers
SELECT '1', ST_AsText(ST_CatmullRomSmoothing('LINESTRING(0 0, 5 0, 10 0, 15 0)'));

-- Explicit nSegments=2, outputs at half-integer positions
SELECT '2', ST_AsText(ST_CatmullRomSmoothing('LINESTRING(0 0, 1 0, 2 0, 3 0)', 2));

-- Fewer than 4 points: return original unchanged
SELECT '3', ST_AsText(ST_CatmullRomSmoothing('LINESTRING(0 0, 1 1, 2 0)'));

-- 2-point line: return original unchanged
SELECT '4', ST_AsText(ST_CatmullRomSmoothing('LINESTRING(0 0, 10 10)'));

-- Point: pass through unchanged
SELECT '5', ST_AsText(ST_CatmullRomSmoothing('POINT(0 0)'));

-- Empty line: return empty
SELECT '6', ST_AsText(ST_CatmullRomSmoothing('LINESTRING EMPTY'));

-- Polygon with square ring, nSegments=2
SELECT '7', ST_AsText(ST_CatmullRomSmoothing('POLYGON((0 0, 4 0, 4 4, 0 4, 0 0))', 2));

-- GeometryCollection: point passthrough + line smoothing
SELECT '8', ST_AsText(ST_CatmullRomSmoothing(
    'GEOMETRYCOLLECTION(POINT(1 1), LINESTRING(0 0, 1 0, 2 0, 3 0))', 2));

-- Z dimension is preserved and interpolated
SELECT '9', ST_AsText(ST_CatmullRomSmoothing('LINESTRING Z (0 0 0, 1 0 10, 2 0 20, 3 0 30)', 2));

-- The geometry bbox is updated
WITH geom AS (
    SELECT ST_CatmullRomSmoothing('LINESTRING(0 0, 1 0, 2 0, 3 0)', 2) AS g
)
SELECT '10', postgis_getbbox(g) AS box FROM geom;

-- Triangle polygon ring has npoints-1=3 unique pts (<4), return original
SELECT '11', ST_AsText(ST_CatmullRomSmoothing('POLYGON((0 0, 4 0, 2 4, 0 0))', 5));
