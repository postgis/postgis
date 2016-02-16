-- postgres

SELECT 't1', ST_GeometricMedian(null) IS NULL;
SELECT 't2', ST_AsText(ST_GeometricMedian('LINESTRING (1 1, 2 2)'));
SELECT 't3', 'POINT EMPTY' = ST_AsText(ST_GeometricMedian('POINT EMPTY'));
SELECT 't4', 'POINT EMPTY' = ST_AsText(ST_GeometricMedian('MULTIPOINT EMPTY'));
SELECT 't5', ST_SRID(ST_GeometricMedian('SRID=32611;POINT (1 1)')) = 32611;
SELECT 't6', ST_SRID(ST_GeometricMedian('SRID=32611;MULTIPOINT ((1 1), (2 7))')) = 32611;
SELECT 't7', ST_SRID(ST_GeometricMedian('SRID=32611;MULTIPOINT ((1 1))')) = 32611;
SELECT 't8', ST_Equals('POINTZ (15 15 15)', ST_AsText(ST_GeometricMedian('MULTIPOINT ((10 10 10), (10 20 10), (20 10 10), (20 20 10), (10 10 20), (10 20 20), (20 10 20), (20 20 20))')));
