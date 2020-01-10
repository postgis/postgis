SELECT '1', ST_astext(ST_ChaikinSmoothing('LINESTRING(0 0, 8 8, 0 16)'));
SELECT '2', ST_astext(ST_ChaikinSmoothing('LINESTRING(0 0, 8 8, 0 16)',10));
SELECT '3', ST_astext(ST_ChaikinSmoothing('LINESTRING(0 0, 8 8, 0 16)',0));
SELECT '4', ST_astext(ST_ChaikinSmoothing('LINESTRING(0 0, 8 8, 0 16)',2));
SELECT '5', ST_astext(ST_ChaikinSmoothing('POINT(0 0)'));
SELECT '6', ST_astext(ST_ChaikinSmoothing('GEOMETRYCOLLECTION(POINT(1 1), LINESTRING(1 1, 1 3, 1 5), POLYGON((5 5, 5 10, 10 10, 10 5, 5 5), (6 6, 6 7, 7 7, 7 6, 6 6 )))', 2, 't'));

-- The geometry bbox is updated
WITH geom AS
(
    SELECT ST_ChaikinSmoothing('POLYGON((0 0, 8 8, 0 16, 0 0))') AS g
)
SELECT '7', ST_AsText(g) as geometry, postgis_getbbox(g) AS box from geom;