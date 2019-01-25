SELECT 'J1', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (SELECT '{"c1":1,"c2":"abcd"}'::jsonb,
    ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom) AS q;
SELECT 'J2', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (SELECT '{"c1":"abcd", "c2":"abcd"}'::jsonb,
    ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom) AS q;
SELECT 'J3', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (
    SELECT '{"c1":"abcd", "c2":"abcd"}'::jsonb,
            ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'), ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom
    UNION ALL
    SELECT '{"c3":"abasdadcd", "c1":5}'::jsonb,
            ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'), ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom
) AS q;
SELECT 'J4', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (
	SELECT '1.25'::double precision AS c1,
	ST_AsMVTGeom(ST_GeomFromText('POINT (0 0)'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom,
	'{"test":1.75}'::jsonb as properties
) AS q;