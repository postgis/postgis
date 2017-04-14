SELECT encode(ST_AsGeobuf(E'SELECT ST_MakePoint(1.1, 2.1) as geom', 'geom'), 'base64');
SELECT encode(ST_AsGeobuf(E'SELECT \'test\' as test, ST_MakeLine(ST_MakePoint(1,1), ST_MakePoint(2,2)) as geom', 'geom'), 'base64');
SELECT encode(ST_AsGeobuf(E'SELECT ST_GeomFromText(\'POLYGON((0 0,0 1,1 1,1 0,0 0))\') as geom', 'geom'), 'base64');
SELECT encode(ST_AsGeobuf(E'SELECT ST_GeomFromText(\'POLYGON((0 0,0 5,5 5,5 0,0 0), (1 1,1 2,2 2,2 1,1 1))\') as geom', 'geom'), 'base64');
SELECT encode(ST_AsGeobuf(E'SELECT ST_GeomFromText(\'MULTIPOINT (10 40, 40 30, 20 20, 30 10)\') as geom', 'geom'), 'base64');
SELECT encode(ST_AsGeobuf(E'SELECT ST_GeomFromText(\'MULTILINESTRING ((10 10, 20 20, 10 40), (40 40, 30 30, 40 20, 30 10))\') as geom', 'geom'), 'base64');
SELECT encode(ST_AsGeobuf(E'SELECT ST_GeomFromText(\'MULTIPOLYGON (((40 40, 20 45, 45 30, 40 40)), ((20 35, 10 30, 10 10, 30 5, 45 20, 20 35), (30 20, 20 15, 20 25, 30 20)))\') as geom', 'geom'), 'base64');
SELECT encode(ST_AsGeobuf(E'SELECT ST_GeomFromText(\'GEOMETRYCOLLECTION(POINT(4 6),LINESTRING(4 6,7 10))\') as geom', 'geom'), 'base64');
