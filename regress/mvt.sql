-- geometry preprocessing tests
select 'PG1', ST_AsText(ST_Normalize(ST_AsMVTGeom(
	ST_Point(1, 2),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)),
	4096, 0, false)));
select 'PG2', ST_AsText(ST_Normalize(ST_AsMVTGeom(
	ST_Point(1, 2),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096*2, 4096*2)),
	4096, 0, false)));
select 'PG3', ST_AsText(ST_Normalize(ST_AsMVTGeom(
	ST_Point(1, 2),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096/2, 4096/2)),
	4096, 0, false)));
select 'PG4', ST_AsText(ST_Normalize(ST_AsMVTGeom(
	ST_GeomFromText('POLYGON ((0 0, 10 0, 10 5, 0 -5, 0 0))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)),
	4096, 0, false)));
select 'PG5', ST_AsText(ST_Normalize(ST_AsMVTGeom(
	ST_GeomFromText('POLYGON ((0 0, 10 0, 10 5, 0 -5, 0 0))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096*4096, 4096*4096)),
	4096, 0, false)));
select 'PG6', ST_AsText(ST_Normalize(ST_AsMVTGeom(
	ST_GeomFromText('POLYGON ((762780 6474467, 717821 6797045, 1052826 6797045, 762780 6474467))'),
	ST_MakeBox2D(ST_Point(626172.135625, 6261721.35625), ST_Point(1252344.27125, 6887893.49188)),
	4096, 0, false)));
select 'PG7', ST_AsText(ST_Normalize(ST_AsMVTGeom(
    ST_GeomFromText('POLYGON((-7792023.4539488 1411512.60791779,-7785283.40665468 1406282.69482469,-7783978.88137195 1404858.20373788,-7782986.89858399 1402324.91434802,-7779028.02672366 1397370.31802772,
        -7778652.06985644 1394387.75452545,-7779906.76953697 1393279.22658385,-7782212.33678782 1393293.14086794,-7784631.14401331 1394225.4151684,-7786257.27108231 1395867.40241344,-7783978.88137195 1395867.40241344,
        -7783978.88137195 1396646.68250521,-7787752.03959369 1398469.72134299,-7795443.30325373 1405280.43988858,-7797717.16326269 1406217.73286975,-7798831.44531677 1406904.48130551,-7799311.5830898 1408004.24038921,
        -7799085.10302919 1409159.72782477,-7798052.35381919 1411108.84582812,-7797789.63692662 1412213.40365339,-7798224.47868753 1414069.89725829,-7799003.5701851 1415694.42577482,-7799166.63587328 1416966.26267896,
        -7797789.63692662 1417736.81850415,-7793160.38395328 1412417.61222784,-7792023.4539488 1411512.60791779))'),
        ST_MakeBox2D(ST_Point(-20037508.34, 20037508.34), ST_Point(20037508.34, -20037508.34)),
        4096, 10, true)));
select 'PG8', ST_AsText(ST_Normalize(ST_AsMVTGeom(
    ST_GeomFromText('GEOMETRYCOLLECTION(MULTIPOLYGON (((0 0, 10 0, 10 5, 0 -5, 0 0))))'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)),
    4096, 0, false)));
select 'PG9', ST_AsText(ST_Normalize(ST_AsMVTGeom(
	ST_GeomFromText('POLYGON ((0 0, 10 0, 10 10, 0 10, 0 0))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(5, 5)),
	4096, 0, true)));
SELECT 'PG10', ST_AsText(ST_AsMVTGeom(
	'POINT EMPTY'::geometry,
	'BOX(0 0,2 2)'::box2d));

-- geometry encoding tests
SELECT 'TG1', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (SELECT 1 AS c1,
    ST_Normalize(ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false)) AS geom) AS q;
SELECT 'TG2', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (SELECT 1 AS c1,
    ST_Normalize(ST_AsMVTGeom(ST_GeomFromText('MULTIPOINT(25 17, 26 18)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false)) AS geom) AS q;
SELECT 'TG3', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (SELECT 1 AS c1,
    ST_Normalize(ST_AsMVTGeom(ST_GeomFromText('LINESTRING(0 0, 1000 1000)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false)) AS geom) AS q;
SELECT 'TG4', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (SELECT 1 AS c1,
    ST_Normalize(ST_AsMVTGeom(ST_GeomFromText('LINESTRING(0 0, 500 500, 1000 1000)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false)) AS geom) AS q;
SELECT 'TG5', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (SELECT 1 AS c1,
    ST_Normalize(ST_AsMVTGeom(ST_GeomFromText('MULTILINESTRING((1 1, 501 501, 1001 1001),(2 2, 502 502, 1002 1002))'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false)) AS geom) AS q;
SELECT 'TG6', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (SELECT 1 AS c1,
    ST_Normalize(ST_AsMVTGeom(ST_GeomFromText('POLYGON ((35 10, 45 45, 15 40, 10 20, 35 10), (20 30, 35 35, 30 20, 20 30))'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false)) AS geom) AS q;
SELECT 'TG7', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (SELECT 1 AS c1,
    ST_Normalize(ST_AsMVTGeom(ST_GeomFromText('MULTIPOLYGON (((40 40, 20 45, 45 30, 40 40)), ((20 35, 10 30, 10 10, 30 5, 45 20, 20 35), (30 20, 20 15, 20 25, 30 20)))'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false)) AS geom) AS q;
SELECT 'TG8', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (SELECT 1 AS c1,
    ST_Normalize(ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false)) AS geom) AS q;
SELECT 'TG9', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (SELECT 1 AS c1,
    ST_Normalize(ST_AsMVTGeom(ST_GeomFromText('MULTIPOINT(25 17, -26 -18)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false)) AS geom) AS q;

-- attribute encoding tests
SELECT 'TA1', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (SELECT 1 AS c1, 'abcd'::text AS c2,
    ST_Normalize(ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false)) AS geom) AS q;
SELECT 'TA2', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (SELECT 1.1::double precision AS c1,
    ST_Normalize(ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false)) AS geom) AS q;
SELECT 'TA3', encode(ST_AsMVT(q, 'test',  4096, 'geom'), 'base64') FROM (SELECT NULL::integer AS c1,
    ST_Normalize(ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false)) AS geom) AS q;
SELECT 'TA4', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (
    SELECT 1 AS c1, ST_Normalize(ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false)) AS geom
    UNION
    SELECT 2 AS c1, ST_Normalize(ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false)) AS geom) AS q;
SELECT 'TA5', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (SELECT
    ST_Normalize(ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false)) AS geom, 1 AS c1, 'abcd'::text AS c2) AS q;
SELECT 'TA6', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (SELECT 1 AS c1, -1 AS c2,
    ST_Normalize(ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false)) AS geom) AS q;
SELECT 'TA7', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (
    SELECT 'test' AS c1, ST_Normalize(ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false)) AS geom
    UNION
    SELECT 'test' AS c1, ST_Normalize(ST_AsMVTGeom(ST_GeomFromText('POINT(26 18)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false)) AS geom
    UNION
    SELECT 'othertest' AS c1, ST_Normalize(ST_AsMVTGeom(ST_GeomFromText('POINT(26 18)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false)) AS geom) AS q;
SELECT 'TA8', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (
    SELECT 1::int AS c1, ST_Normalize(ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false)) AS geom
    UNION
    SELECT 1::int AS c1, ST_Normalize(ST_AsMVTGeom(ST_GeomFromText('POINT(26 18)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false)) AS geom
    UNION
    SELECT 2::int AS c1, ST_Normalize(ST_AsMVTGeom(ST_GeomFromText('POINT(26 18)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false)) AS geom) AS q;
SELECT 'TA9', length(ST_AsMVT(q))
FROM (
	SELECT 1 AS c1, -1 AS c2,
    ST_Normalize(ST_AsMVTGeom(
		'POINT(25 17)'::geometry,
		ST_MakeBox2D(ST_Point(0, 0), ST_Point(4, 4))
	)) AS geom
) AS q;
SELECT 'TA10', length(ST_AsMVT(q))
FROM (
	SELECT 1 AS c1, -1 AS c2,
    ST_Normalize(ST_AsMVTGeom(
		'POINT(25 17)'::geometry,
		ST_MakeBox2D(ST_Point(0, 0), ST_Point(48, 48))
	)) AS geom
) AS q;


-- default values tests
SELECT 'D1', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (SELECT 1 AS c1, 'abcd'::text AS c2,
    ST_Normalize(ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false)) AS geom) AS q;
SELECT 'D2', encode(ST_AsMVT(q, 'test', 4096), 'base64') FROM (SELECT 1 AS c1, 'abcd'::text AS c2,
    ST_Normalize(ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false)) AS geom) AS q;
SELECT 'D3', encode(ST_AsMVT(q, 'test'), 'base64') FROM (SELECT 1 AS c1, 'abcd'::text AS c2,
    ST_Normalize(ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false)) AS geom) AS q;
SELECT 'D4', encode(ST_AsMVT(q), 'base64') FROM (SELECT 1 AS c1, 'abcd'::text AS c2,
    ST_Normalize(ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
    ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false)) AS geom) AS q;
select 'D5', ST_AsText(ST_Normalize(ST_AsMVTGeom(
	ST_Point(1, 2),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)),
	4096, 0)));
select 'D6', ST_AsText(ST_Normalize(ST_AsMVTGeom(
	ST_Point(1, 2),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)),
	4096)));
select 'D7', ST_AsText(ST_Normalize(ST_AsMVTGeom(
	ST_Point(1, 2),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)))));

-- unsupported input
SELECT 'TU2';
SELECT encode(ST_AsMVT(1, 'test', 4096, 'geom'), 'base64');
SELECT 'TU3', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64')
    FROM (SELECT NULL::integer AS c1, NULL AS geom) AS q;
