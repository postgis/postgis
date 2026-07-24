SELECT 1, ST_AsText(
    ST_FilterHolesByArea(
    ST_GeomFromText('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0), (1 1, 2 1, 2 2, 1 2, 1 1), (4 4, 8 4, 8 8, 4 8, 4 4))'),
    2.0::float8));

SELECT 2, ST_AsText(
    ST_FilterHolesByArea(
    ST_GeomFromText('MULTIPOLYGON(((0 0, 10 0, 10 10, 0 10, 0 0), (1 1, 2 1, 2 2, 1 2, 1 1)), ((20 0, 30 0, 30 10, 20 10, 20 0), (21 1, 25 1, 25 5, 21 5, 21 1)))'),
    2.0::float8));

SELECT 3, ST_AsText(
    ST_FilterHolesByArea(
    ST_GeomFromText('CURVEPOLYGON((0 0, 10 0, 10 10, 0 10, 0 0), CIRCULARSTRING(1 1, 2 2, 3 1, 2 0, 1 1), CIRCULARSTRING(5 5, 7 7, 9 5, 7 3, 5 5))'),
    4.0::float8));

SELECT 4, ST_AsText(
    ST_FilterHolesByArea(
    ST_GeomFromText('GEOMETRYCOLLECTION(POINT(1 1), POLYGON((0 0, 5 0, 5 5, 0 5, 0 0), (1 1, 2 1, 2 2, 1 2, 1 1)))'),
    2.0::float8));

SELECT 5, ST_AsText(
    ST_FilterHolesByArea(
    ST_GeomFromText('POLYGON((0 0, 5 0, 5 5, 0 5, 0 0), (1 1, 2 1, 2 2, 1 2, 1 1))'),
    0.0::float8));

SELECT 6, ST_AsText(
    ST_FilterHolesByArea(
    ST_GeomFromText('LINESTRING(0 0, 1 1)'),
    2.0::float8));

SELECT 7, ST_AsText(
    ST_FilterHolesByArea(
    ST_GeomFromText('POLYGON((0 0, 3 0, 3 3, 0 3, 0 0), (1 1, 2 1, 2 2, 1 2, 1 1))'),
    1.0::float8));

SELECT 8, ST_AsText(
    ST_FilterHolesByArea(
    ST_GeomFromText('POLYGON((0 0, 5 0, 5 5, 0 5, 0 0), (1 1, 2 1, 2 2, 1 2, 1 1))'),
    -1.0::float8));

SELECT 9, ST_AsText(
    ST_FilterHolesByArea(
    ST_GeomFromText('POLYGON((0 0, 5 0, 5 5, 0 5, 0 0), (1 1, 2 1, 2 2, 1 2, 1 1))'),
    'NaN'::float8));

SELECT 10, ST_AsText(
    ST_FilterHolesByArea(
    ST_GeomFromText('POLYGON((0 0, 5 0, 5 5, 0 5, 0 0), (1 1, 2 1, 2 2, 1 2, 1 1))'),
    'Infinity'::float8));

SELECT 11, ST_AsText(
    ST_FilterHolesByArea(
    ST_GeomFromText('POLYGON((0 0, 5 0, 5 5, 0 5, 0 0), (1 1, 2 1, 2 2, 1 2, 1 1))'),
    '-Infinity'::float8));
