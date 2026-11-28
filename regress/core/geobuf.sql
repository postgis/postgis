--set client_min_messages to DEBUG3;
SELECT 'T1', encode(ST_AsGeobuf(q, 'geom'), 'base64')
    FROM (SELECT ST_MakePoint(1.1, 2.1) AS geom) AS q;
SELECT 'T2', encode(ST_AsGeobuf(q, 'geom'), 'base64')
    FROM (SELECT 'test' as test_str, 1 as test_pos_int, -1 as test_neg_int, 1.1 as test_numeric, 1.1::float as test_float, ST_MakeLine(ST_MakePoint(1,1), ST_MakePoint(2,2)) as geom) AS q;
SELECT 'T3', encode(ST_AsGeobuf(q, 'geom'), 'base64')
    FROM (SELECT ST_GeomFromText('POLYGON((0 0,0 1,1 1,1 0,0 0))') as geom) AS q;
SELECT 'T4', encode(ST_AsGeobuf(q, 'geom'), 'base64')
    FROM (SELECT ST_GeomFromText('POLYGON((0 0,0 5,5 5,5 0,0 0), (1 1,1 2,2 2,2 1,1 1))') as geom) AS q;
SELECT 'T5', encode(ST_AsGeobuf(q, 'geom'), 'base64')
    FROM (SELECT ST_GeomFromText('MULTIPOINT (10 40, 40 30, 20 20, 30 10)') as geom) AS q;
SELECT 'T6', encode(ST_AsGeobuf(q, 'geom'), 'base64')
    FROM (SELECT ST_GeomFromText('MULTILINESTRING ((10 10, 20 20, 10 40), (40 40, 30 30, 40 20, 30 10))') as geom) AS q;
SELECT 'T7', encode(ST_AsGeobuf(q, 'geom'), 'base64')
    FROM (SELECT ST_GeomFromText('MULTIPOLYGON (((40 40, 20 45, 45 30, 40 40)), ((20 35, 10 30, 10 10, 30 5, 45 20, 20 35), (30 20, 20 15, 20 25, 30 20)))') as geom) AS q;
SELECT 'T8', encode(ST_AsGeobuf(q, 'geom'), 'base64')
    FROM (SELECT ST_GeomFromText('GEOMETRYCOLLECTION(POINT(4 6),LINESTRING(4 6,7 10))') as geom) AS q;
SELECT 'T9', encode(ST_AsGeobuf(q, 'geom'), 'base64')
    FROM (SELECT ST_MakePoint(1, 2, 3) as geom) AS q;
SELECT 'T10', encode(ST_AsGeobuf(q), 'base64')
    FROM (SELECT ST_MakePoint(1, 2, 3) as geom) AS q;

WITH geom AS (
	SELECT 'TRIANGLE((0 0, 1 1, 0 1, 0 0))'::geometry geom
	union all
	SELECT 'TIN(((0 0, 1 1, 0 1, 0 0)))'::geometry geom
	union all
	SELECT 'TRIANGLE EMPTY'::geometry geom
)
select '#4399', 'ST_AsGeobuf', ST_AsGeobuf(geom.*)::text from geom;

SELECT '#4916.a', ST_AsGeobuf(NULL::pg_class, 'g') over (order by b)
FROM (VALUES ('POINT(0 0)'::geometry, 'A0006', 300),
	         ('POINT(1 1)'::geometry, 'A0006', 302)) t(g, a, b);

SELECT '#4916.b', ST_AsGeobuf(NULL::pg_class) over (order by b)
FROM (VALUES ('POINT(0 0)'::geometry, 'A0006', 300),
	         ('POINT(1 1)'::geometry, 'A0006', 302)) t(g, a, b);

-- NULL rows are skipped by ST_AsGeobuf
CREATE TEMP TABLE geobuf_null_row (id int, geom geometry);
INSERT INTO geobuf_null_row VALUES (1, 'POINT(1 1)'), (2, 'POINT(2 2)'), (3, 'POINT(3 3)');
SELECT 'null_row.a', ST_AsGeobuf(NULL::geobuf_null_row) IS NULL;
SELECT 'null_row.b', ST_AsGeobuf(CASE WHEN id = 2 THEN NULL::geobuf_null_row ELSE t END) =
	(SELECT ST_AsGeobuf(t) FROM geobuf_null_row t WHERE id <> 2)
FROM geobuf_null_row t;
SELECT 'null_row.c', ST_AsGeobuf(CASE WHEN id = 3 THEN NULL::geobuf_null_row ELSE t END) =
	(SELECT ST_AsGeobuf(t) FROM geobuf_null_row t WHERE id <> 3)
FROM geobuf_null_row t;
DROP TABLE geobuf_null_row;
SELECT 'T11', encode(ST_AsGeobuf(q, 'geom'), 'base64')
    FROM (SELECT 1::bigint AS geom) AS q;

-- Reject rows with a different record layout instead of deforming them
-- through the first row's cached descriptor.
DO $$
DECLARE
    error_message text;
BEGIN
    BEGIN
        PERFORM ST_AsGeobuf(v.r)
        FROM (
            SELECT ROW('POINT(0 0)'::geometry, 1::integer) AS r
            UNION ALL
            SELECT ROW(1::integer, 'POINT(1 1)'::geometry)
        ) AS v;
    EXCEPTION WHEN SQLSTATE 'XX000' THEN
        GET STACKED DIAGNOSTICS error_message = MESSAGE_TEXT;
        IF error_message <> 'geobuf_agg_transfn: all rows must share the same column layout' THEN
            RAISE;
        END IF;
        RETURN;
    END;
    RAISE EXCEPTION 'Geobuf accepted rows with different record layouts';
END;
$$;
