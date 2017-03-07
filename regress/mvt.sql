-- geometry preprocessing tests
select 'PG1', ST_AsText(ST_AsMVTGeom(
	ST_Point(1, 2),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)),
	4096, 0, false));
select 'PG2', ST_AsText(ST_AsMVTGeom(
	ST_Point(1, 2),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096*2, 4096*2)),
	4096, 0, false));
select 'PG3', ST_AsText(ST_AsMVTGeom(
	ST_Point(1, 2),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096/2, 4096/2)),
	4096, 0, false));
select 'PG4', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('POLYGON ((0 0, 10 0, 10 5, 0 -5, 0 0))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)),
	4096, 0, false));
select 'PG5', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('POLYGON ((0 0, 10 0, 10 5, 0 -5, 0 0))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(1, 1)),
	4096, 0, false));

-- geometry encoding tests
SELECT 'TG1', encode(ST_AsMVT('test', 4096, 'geom', q), 'base64') FROM (SELECT 1 AS c1,
    ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom) AS q;
SELECT 'TG2', encode(ST_AsMVT('test', 4096, 'geom', q), 'base64') FROM (SELECT 1 AS c1,
    ST_AsMVTGeom(ST_GeomFromText('MULTIPOINT(25 17, 26 18)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom) AS q;
SELECT 'TG3', encode(ST_AsMVT('test', 4096, 'geom', q), 'base64') FROM (SELECT 1 AS c1,
    ST_AsMVTGeom(ST_GeomFromText('LINESTRING(0 0, 1000 1000)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom) AS q;
SELECT 'TG4', encode(ST_AsMVT('test', 4096, 'geom', q), 'base64') FROM (SELECT 1 AS c1,
    ST_AsMVTGeom(ST_GeomFromText('LINESTRING(0 0, 500 500, 1000 1000)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom) AS q;
SELECT 'TG5', encode(ST_AsMVT('test', 4096, 'geom', q), 'base64') FROM (SELECT 1 AS c1,
    ST_AsMVTGeom(ST_GeomFromText('MULTILINESTRING((1 1, 501 501, 1001 1001),(2 2, 502 502, 1002 1002))'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom) AS q;
SELECT 'TG6', encode(ST_AsMVT('test', 4096, 'geom', q), 'base64') FROM (SELECT 1 AS c1,
    ST_AsMVTGeom(ST_GeomFromText('POLYGON ((35 10, 45 45, 15 40, 10 20, 35 10), (20 30, 35 35, 30 20, 20 30))'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom) AS q;
SELECT 'TG7', encode(ST_AsMVT('test', 4096, 'geom', q), 'base64') FROM (SELECT 1 AS c1,
    ST_AsMVTGeom(ST_GeomFromText('MULTIPOLYGON (((40 40, 20 45, 45 30, 40 40)), ((20 35, 10 30, 10 10, 30 5, 45 20, 20 35), (30 20, 20 15, 20 25, 30 20)))'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom) AS q;
SELECT 'TG8', encode(ST_AsMVT('test', 4096, 'geom', q), 'base64') FROM (SELECT 1 AS c1,
    ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom) AS q;
SELECT 'TG9', encode(ST_AsMVT('test', 4096, 'geom', q), 'base64') FROM (SELECT 1 AS c1,
    ST_AsMVTGeom(ST_GeomFromText('MULTIPOINT(25 17, -26 -18)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom) AS q;

-- attribute encoding tests
SELECT 'TA1', encode(ST_AsMVT('test', 4096, 'geom', q), 'base64') FROM (SELECT 1 AS c1, 'abcd'::text AS c2,
    ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom) AS q;
SELECT 'TA2', encode(ST_AsMVT('test', 4096, 'geom', q), 'base64') FROM (SELECT 1.1::double precision AS c1,
    ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom) AS q;
SELECT 'TA3', encode(ST_AsMVT('test',  4096, 'geom', q), 'base64') FROM (SELECT NULL::integer AS c1,
    ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom) AS q;
SELECT 'TA4', encode(ST_AsMVT('test', 4096, 'geom', q), 'base64') FROM (
    SELECT 1 AS c1, ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom
    UNION
    SELECT 2 AS c1, ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom) AS q;
SELECT 'TA5', encode(ST_AsMVT('test', 4096, 'geom', q), 'base64') FROM (SELECT 
    ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom, 1 AS c1, 'abcd'::text AS c2) AS q;
SELECT 'TA6', encode(ST_AsMVT('test', 4096, 'geom', q), 'base64') FROM (SELECT 1 AS c1, -1 AS c2,
    ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom) AS q;

-- unsupported input
SELECT 'TU2';
SELECT encode(ST_AsMVT('test', 4096, 'geom', 1), 'base64');
SELECT 'TU3';
SELECT encode(ST_AsMVT('test', 4096, 'geom', q), 'base64')
    FROM (SELECT NULL::integer AS c1, NULL AS geom) AS q;