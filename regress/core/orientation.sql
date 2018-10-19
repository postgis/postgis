-- pg24

-- Non-applicable types
SELECT '1', ST_IsPolygonCCW('POINT (0 0)');
SELECT '2', ST_IsPolygonCCW('MULTIPOINT ((0 0), (1 1))');
SELECT '3', ST_IsPolygonCCW('LINESTRING (1 1, 2 2)');
SELECT '4', ST_IsPolygonCCW('MULTILINESTRING ((1 1, 2 2), (3 3, 0 0))');
SELECT '5', ST_IsPolygonCW('POINT (0 0)');
SELECT '6', ST_IsPolygonCW('MULTIPOINT ((0 0), (1 1))');
SELECT '7', ST_IsPolygonCW('LINESTRING (1 1, 2 2)');
SELECT '8', ST_IsPolygonCW('MULTILINESTRING ((1 1, 2 2), (3 3, 0 0))');

-- NULL handling
SELECT '101', ST_IsPolygonCW(NULL::geometry);
SELECT '102', ST_IsPolygonCCW(NULL::geometry);
SELECT '103', ST_ForcePolygonCW(NULL::geometry);
SELECT '104', ST_ForcePolygonCCW(NULL::geometry);

-- EMPTY handling
SELECT '201', ST_AsText(ST_ForcePolygonCW('POLYGON EMPTY'::geometry));
SELECT '202', ST_AsText(ST_ForcePolygonCCW('POLYGON EMPTY'::geometry));
SELECT '203', ST_IsPolygonCW('POLYGON EMPTY'::geometry);
SELECT '204', ST_IsPolygonCCW('POLYGON EMPTY'::geometry);

-- Preserves SRID
SELECT '301', ST_SRID(ST_ForcePolygonCW('SRID=4269;POLYGON ((0 0, 1 1, 1 0, 0 0))'));
SELECT '302', ST_SRID(ST_ForcePolygonCCW('SRID=4269;POLYGON ((0 0, 1 0, 1 1, 0 0))'));

-- Single polygon, ccw exterior ring only
SELECT '401', ST_IsPolygonCW('POLYGON ((0 0, 1 0, 1 1, 0 0))');
SELECT '402', ST_IsPolygonCCW('POLYGON ((0 0, 1 0, 1 1, 0 0))');

-- Single polygon, cw exterior ring only
SELECT '403', ST_IsPolygonCW('POLYGON ((0 0, 1 1, 1 0, 0 0))');
SELECT '404', ST_IsPolygonCCW('POLYGON ((0 0, 1 1, 1 0, 0 0))');

-- Single polygon, ccw exterior ring, cw interior rings
SELECT '405', ST_IsPolygonCW('POLYGON ((0 0, 10 0, 10 10, 0 10, 0 0), (1 1, 1 2, 2 2, 1 1), (5 5, 5 7, 7 7, 5 5))');
SELECT '406', ST_IsPolygonCCW('POLYGON ((0 0, 10 0, 10 10, 0 10, 0 0), (1 1, 1 2, 2 2, 1 1), (5 5, 5 7, 7 7, 5 5))');

-- Single polygon, cw exterior ring, ccw interior rings
SELECT '407', ST_IsPolygonCW( 'POLYGON ((0 0, 0 10, 10 10, 10 0, 0 0), (1 1, 2 2, 1 2, 1 1), (5 5, 7 7, 5 7, 5 5))');
SELECT '408', ST_IsPolygonCCW('POLYGON ((0 0, 0 10, 10 10, 10 0, 0 0), (1 1, 2 2, 1 2, 1 1), (5 5, 7 7, 5 7, 5 5))');

-- Single polygon, ccw exerior ring, mixed interior rings
SELECT '409', ST_IsPolygonCW( 'POLYGON ((0 0, 10 0, 10 10, 0 10, 0 0), (1 1, 1 2, 2 2, 1 1), (5 5, 7 7, 5 7, 5 5))');
SELECT '410', ST_IsPolygonCCW('POLYGON ((0 0, 10 0, 10 10, 0 10, 0 0), (1 1, 1 2, 2 2, 1 1), (5 5, 7 7, 5 7, 5 5))');

-- Single polygon, cw exterior ring, mixed interior rings
SELECT '411', ST_IsPolygonCW( 'POLYGON ((0 0, 0 10, 10 10, 10 0, 0 0), (1 1, 2 2, 1 2, 1 1), (5 5, 5 7, 7 7, 5 5))');
SELECT '412', ST_IsPolygonCCW('POLYGON ((0 0, 0 10, 10 10, 10 0, 0 0), (1 1, 2 2, 1 2, 1 1), (5 5, 5 7, 7 7, 5 5))');

-- MultiPolygon, ccw exterior rings only
SELECT '413', ST_IsPolygonCW( 'MULTIPOLYGON (((0 0, 1 0, 1 1, 0 0)), ((100 0, 101 0, 101 1, 100 0)))');
SELECT '414', ST_IsPolygonCCW('MULTIPOLYGON (((0 0, 1 0, 1 1, 0 0)), ((100 0, 101 0, 101 1, 100 0)))');

-- MultiPolygon, cw exterior rings only
SELECT '415', ST_IsPolygonCW( 'MULTIPOLYGON (((0 0, 1 1, 1 0, 0 0)), ((100 0, 101 1, 101 0, 100 0)))');
SELECT '416', ST_IsPolygonCCW('MULTIPOLYGON (((0 0, 1 1, 1 0, 0 0)), ((100 0, 101 1, 101 0, 100 0)))');

-- MultiPolygon, mixed exterior rings
SELECT '417', ST_IsPolygonCW( 'MULTIPOLYGON (((0 0, 1 0, 1 1, 0 0)), ((100 0, 101 1, 101 0, 100 0)))');
SELECT '418', ST_IsPolygonCCW('MULTIPOLYGON (((0 0, 1 0, 1 1, 0 0)), ((100 0, 101 1, 101 0, 100 0)))');

