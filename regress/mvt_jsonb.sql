SELECT 'TA9', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (SELECT '{"c1":1,"c2":"abcd"}'::jsonb,
    ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom) AS q;
SELECT 'TA10', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (SELECT '{"c1":"abcd", "c2":"abcd"}'::jsonb,
    ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom) AS q;
