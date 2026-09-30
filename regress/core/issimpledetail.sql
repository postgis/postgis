SELECT 't1', simple, ST_AsText(location)  FROM ST_IsSimpleDetail('LINESTRING(0 0, 10 0)'::geometry);
SELECT 't2', simple, ST_AsText(location) FROM ST_IsSimpleDetail('LINESTRING(0 0, 10 0, 5 10, 5 -10)'::geometry);
SELECT 't3', simple, ST_AsText(location) FROM ST_IsSimpleDetail('LINESTRING(0 0, 10 0, 5 10, 5 -10, 2 2)'::geometry, 1);
SELECT 't4', simple, ST_AsText(location) FROM ST_IsSimpleDetail('MULTILINESTRING((0 0, -10 0, -10 -10, 0 0),(0 0,10 0,10 10,0 0))'::geometry, 1, 1);
SELECT 't5', simple, ST_AsText(location) FROM ST_IsSimpleDetail('MULTILINESTRING((0 0, -10 0, -10 -10, 0 0),(0 0,10 0,10 10,0 0))'::geometry, 1, 2);
