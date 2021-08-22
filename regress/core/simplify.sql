-- Repeat all tests with the new function names.
SELECT '1', ST_astext(ST_Simplify('LINESTRING(0 0, 0 10, 0 51, 50 20, 30 20, 7 32)'::geometry, 2));
SELECT '2', ST_astext(ST_Simplify('LINESTRING(0 0 3, 0 10 6, 0 51 1, 50 20 6, 30 20 9, 7 32 10)'::geometry, 2));
SELECT '3', ST_astext(ST_Simplify('LINESTRINGM(0 0 3, 0 10 6, 0 51 1, 50 20 6, 30 20 9, 7 32 10)'::geometry, 2));
SELECT '4', ST_astext(ST_Simplify('LINESTRING(0 0 3 2, 0 10 6 1, 0 51 1 6, 50 20 6 7, 30 20 9 9, 7 32 10 5)'::geometry, 2));
SELECT '5', ST_astext(ST_Simplify('MULTIPOINT(0 0 3 2, 0 10 6 1, 0 51 1 6, 50 20 6 7, 30 20 9 9, 7 32 10 5)'::geometry, 20));
SELECT '6', ST_astext(ST_Simplify('MULTILINESTRING((0 0 3 2, 0 10 6 1, 0 51 1 6, 50 20 6 7, 30 20 9 9, 7 32 10 5), (0 0 4 3, 1 1 2 3, 20 20 5 30))'::geometry, 20));
SELECT '7', ST_astext(ST_Simplify('POLYGON((0 0 3 2, 0 10 6 1, 0 51 1 6, 50 20 6 7, 30 20 9 9, 7 32 10 5, 0 0 3 2), (1 1 4 3, 1 3 2 3, 18 18 5 30, 1 1 4 3))'::geometry, 20));
SELECT '8', ST_astext(ST_Simplify('POLYGON((0 0 3 2, 0 10 6 1, 0 51 1 6, 50 20 6 7, 30 20 9 9, 7 32 10 5, 0 0 3 2), (1 1 4 3, 1 3 2 3, 18 18 5 30, 1 1 4 3))'::geometry, 1));

SELECT '9', ST_astext(ST_Simplify('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 5 6, 6 6, 8 5, 5 5))'::geometry, 20));
SELECT '10', ST_astext(ST_Simplify('LINESTRING(0 0, 0 10)'::geometry, 20));
SELECT '11', ST_astext(ST_Simplify('MULTIPOLYGON(((100 100, 100 130, 130 130, 130 100, 100 100)), ((0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 5 6, 6 6, 8 5, 5 5)) )'::geometry, 20));
SELECT '12', ST_astext(ST_Simplify('MULTIPOLYGON(((0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 5 6, 6 6, 8 5, 5 5)),((100 100, 100 130, 130 130, 130 100, 100 100)))'::geometry, 20));

SELECT '13', ST_astext(ST_Simplify('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))'::geometry, 20, false));
SELECT '14', ST_astext(ST_Simplify('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))'::geometry, 20, true));

-- Updates the geometry bbox
WITH geom AS
(
    SELECT ST_Simplify('POLYGON((0 0, 10 0, 10 10, 10.6 10, 10.5 10.5, 10 10, 0 10, 0 0))'::geometry, 1) as g
)
Select '15', ST_AsText(g) as geometry, postgis_getbbox(g) AS box from geom;

-- Triangle should collapse if requested
SELECT '16', ST_AsText(ST_Simplify('TRIANGLE ((0 0, 0 9, 9 0, 0 0))'::geometry, 10, true));
SELECT '17', ST_AsText(ST_Simplify('TRIANGLE ((0 0, 0 9, 9 0, 0 0))'::geometry, 10, false));
