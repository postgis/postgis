SELECT encode(ST_AsGeobuf(E'SELECT ST_MakePoint(1.1, 2.1) as geom', 'geom'), 'base64');
SELECT encode(ST_AsGeobuf(E'SELECT \'test\' as test, ST_MakeLine(ST_MakePoint(1,1), ST_MakePoint(2,2)) as geom', 'geom'), 'base64');
SELECT encode(ST_AsGeobuf(E'SELECT ST_GeomFromText(\'POLYGON((0 0,0 1,1 1,1 0,0 0))\') as geom', 'geom'), 'base64');
SELECT encode(ST_AsGeobuf(E'SELECT ST_GeomFromText(\'POLYGON((0 0,0 5,5 5,5 0,0 0), (1 1,1 2,2 2,2 1,1 1))\') as geom', 'geom'), 'base64');