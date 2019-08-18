SELECT 'X1', ST_X('POINT (0 0)'::geometry);
SELECT 'X2', ST_X('POINTZ (1 2 3)'::geometry);
SELECT 'X3', ST_X('POINTM (6 7 8)'::geometry);
SELECT 'X4', ST_X('POINTZM (10 11 12 13)'::geometry);
SELECT 'X5', ST_X('MULTIPOINT ((0 0), (1 1))'::geometry);
SELECT 'X6', ST_X('LINESTRING (0 0, 1 1)'::geometry);
SELECT 'X7', ST_X('GEOMETRYCOLLECTION (POINT(0 0))'::geometry);
SELECT 'X8', ST_X('GEOMETRYCOLLECTION (POINT(0 1), LINESTRING(0 0, 1 1))'::geometry);

SELECT 'Y1', ST_Y('POINT (0 0)'::geometry);
SELECT 'Y2', ST_Y('POINTZ (1 2 3)'::geometry);
SELECT 'Y3', ST_Y('POINTM (6 7 8)'::geometry);
SELECT 'Y4', ST_Y('POINTZM (10 11 12 13)'::geometry);
SELECT 'Y5', ST_Y('MULTIPOINT ((0 0), (1 1))'::geometry);
SELECT 'Y6', ST_Y('LINESTRING (0 0, 1 1)'::geometry);
SELECT 'Y7', ST_Y('GEOMETRYCOLLECTION (POINT(0 0))'::geometry);
SELECT 'Y8', ST_Y('GEOMETRYCOLLECTION (POINT(0 1), LINESTRING(0 0, 1 1))'::geometry);

SELECT 'Z1', ST_Z('POINT (0 0)'::geometry);
SELECT 'Z2', ST_Z('POINTZ (1 2 3)'::geometry);
SELECT 'Z3', ST_Z('POINTM (6 7 8)'::geometry);
SELECT 'Z4', ST_Z('POINTZM (10 11 12 13)'::geometry);
SELECT 'Z5', ST_Z('MULTIPOINT ((0 0), (1 1))'::geometry);
SELECT 'Z6', ST_Z('LINESTRING (0 0, 1 1)'::geometry);
SELECT 'Z7', ST_Z('GEOMETRYCOLLECTION (POINT(0 0))'::geometry);
SELECT 'Z8', ST_Z('GEOMETRYCOLLECTION (POINT(0 1), LINESTRING(0 0, 1 1))'::geometry);

SELECT 'M1', ST_M('POINT (0 0)'::geometry);
SELECT 'M2', ST_M('POINTZ (1 2 3)'::geometry);
SELECT 'M3', ST_M('POINTM (6 7 8)'::geometry);
SELECT 'M4', ST_M('POINTZM (10 11 12 13)'::geometry);
SELECT 'M5', ST_M('MULTIPOINT ((0 0), (1 1))'::geometry);
SELECT 'M6', ST_M('LINESTRING (0 0, 1 1)'::geometry);
SELECT 'M7', ST_M('GEOMETRYCOLLECTION (POINT(0 0))'::geometry);
SELECT 'M8', ST_M('GEOMETRYCOLLECTION (POINT(0 1), LINESTRING(0 0, 1 1))'::geometry);
