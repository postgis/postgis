--set client_min_messages to DEBUG3;
set timezone to UTC;
select ST_FromFlatGeobufToTable('public', 'flatgeobuf_t1', (select ST_AsFlatGeobuf(q) fgb from (select null::geometry) q));

select '--- Null geometry ---';

-- single row null geometry
select 'T1', id, ST_AsText(geom) from ST_FromFlatGeobuf(null::flatgeobuf_t1, (
    select ST_AsFlatGeobuf(q) fgb from (select null::geometry) q)
);

-- no data (should result in header only FlatGeobuf)
--select 'T2', id, ST_AsText(geom) from ST_FromFlatGeobuf(null::flatgeobuf_t1, (
--    select ST_AsFlatGeobuf(q) fgb from (select null::geometry limit 0) q)
--);

select '--- Geometry roundtrips ---';

-- 2D Point
select 'P1', id, ST_AsText(geom) from ST_FromFlatGeobuf(null::flatgeobuf_t1, (
    select ST_AsFlatGeobuf(q) fgb from (select ST_MakePoint(1.1, 2.1)) q)
);
-- 3D Point
select 'P2', id, ST_AsText(geom) from ST_FromFlatGeobuf(null::flatgeobuf_t1, (
    select ST_AsFlatGeobuf(q) fgb from (select ST_MakePoint(1.1, 2.11, 3.2)) q)
);
-- 4D Point
select 'P3', id, ST_AsText(geom) from ST_FromFlatGeobuf(null::flatgeobuf_t1, (
    select ST_AsFlatGeobuf(q) fgb from (select ST_MakePoint(1.1, 2.12, 3.2, 4.3)) q)
);
-- 2D Point with SRID
select 'P4', id, ST_AsEWKT(geom) from ST_FromFlatGeobuf(null::flatgeobuf_t1, (
    select ST_AsFlatGeobuf(q) fgb from (select ST_SetSRID(ST_MakePoint(-71.1043443253471, 42.3150676015829),4326)) q)
);
-- 2D Point with SRID and index
select 'P5', id, ST_AsEWKT(geom) from ST_FromFlatGeobuf(null::flatgeobuf_t1, (
    select ST_AsFlatGeobuf(q, true) fgb from (select ST_SetSRID(ST_MakePoint(-71.1043443253471, 42.3150676015829),4326)) q)
);
-- Empty Point
select 'P6', id, ST_AsEWKT(geom) from ST_FromFlatGeobuf(null::flatgeobuf_t1, (
    select ST_AsFlatGeobuf(q, true) fgb from (select ST_GeomFromText('POINT EMPTY')) q)
);

-- 2D LineString
select 'L1', id, ST_AsText(geom) from ST_FromFlatGeobuf(null::flatgeobuf_t1, (
    select ST_AsFlatGeobuf(q) fgb from (select ST_MakeLine(ST_MakePoint(1, 2), ST_MakePoint(3, 4))) q)
);
-- 3D LineString
select 'L2', id, ST_AsText(geom) from ST_FromFlatGeobuf(null::flatgeobuf_t1, (
    select ST_AsFlatGeobuf(q) fgb from (select ST_MakeLine(ST_MakePoint(1, 2, 3), ST_MakePoint(3, 4, 5))) q)
);
-- 4D LineString
select 'L3', id, ST_AsText(geom) from ST_FromFlatGeobuf(null::flatgeobuf_t1, (
    select ST_AsFlatGeobuf(q) fgb from (select ST_MakeLine(ST_MakePoint(1, 2, 3, 5), ST_MakePoint(3, 4, 5, 6))) q)
);

-- 2D Polygon
select 'P1', id, ST_AsText(geom) from ST_FromFlatGeobuf(null::flatgeobuf_t1, (
    select ST_AsFlatGeobuf(q) fgb from (select ST_Polygon('LINESTRING(75 29, 77 29, 77 29, 75 29)'::geometry, 4326)) q)
);

-- 2D Polygon with hole
select 'P2', id, ST_AsText(geom) from ST_FromFlatGeobuf(null::flatgeobuf_t1, (
    select ST_AsFlatGeobuf(q) from (select
        'POLYGON ((35 10, 45 45, 15 40, 10 20, 35 10), (20 30, 35 35, 30 20, 20 30))'::geometry
    ) q)
);

-- 2D MultiPoint
select 'MP1', id, ST_AsText(geom) from ST_FromFlatGeobuf(null::flatgeobuf_t1, (
    select ST_AsFlatGeobuf(q) from (select
        'MULTIPOINT (10 40, 40 30, 20 20, 30 10)'::geometry
    ) q)
);

-- 2D MultiLineString
select 'ML1', id, ST_AsText(geom) from ST_FromFlatGeobuf(null::flatgeobuf_t1, (
    select ST_AsFlatGeobuf(q) from (select
        'MULTILINESTRING ((10 10, 20 20, 10 40), (40 40, 30 30, 40 20, 30 10))'::geometry
    ) q)
);

-- 2D MultiPolygon
select 'MP1', id, ST_AsText(geom) from ST_FromFlatGeobuf(null::flatgeobuf_t1, (
    select ST_AsFlatGeobuf(q) from (select
        'MULTIPOLYGON (((40 40, 20 45, 45 30, 40 40)), ((20 35, 10 30, 10 10, 30 5, 45 20, 20 35), (30 20, 20 15, 20 25, 30 20)))'::geometry
    ) q)
);

-- 2D GeometryCollection
select 'GC1', id, ST_AsText(geom) from ST_FromFlatGeobuf(null::flatgeobuf_t1, (
    select ST_AsFlatGeobuf(q) from (select
        'GEOMETRYCOLLECTION (POINT (40 10), LINESTRING (10 10, 20 20, 10 40), POLYGON ((40 40, 20 45, 45 30, 40 40)))'::geometry
    ) q)
);

select '--- Multiple rows ---';

-- Multiple 2D points
select 'P1', id, ST_AsText(geom) from ST_FromFlatGeobuf(null::flatgeobuf_t1, (
    select ST_AsFlatGeobuf(q) from (values
        ('POINT (1 2)'::geometry),
        ('POINT (3 4)'::geometry)
    ) q)
);

-- Multiple 2D points and a polygon
select 'PL1', id, ST_AsText(geom) from ST_FromFlatGeobuf(null::flatgeobuf_t1, (
    select ST_AsFlatGeobuf(q) from (values
        ('POINT (1 2)'::geometry),
        ('POINT (3 4)'::geometry),
        ('POLYGON ((35 10, 45 45, 15 40, 10 20, 35 10), (20 30, 35 35, 30 20, 20 30))'::geometry)
    ) q)
);

select '--- Attribute roundtrips ---';

select ST_FromFlatGeobufToTable('public', 'flatgeobuf_a1', (select ST_AsFlatGeobuf(q) fgb from (select
        null::geometry,
        null::boolean as bool_1,
        null::integer as int_1,
        null::integer as int_2,
        null::smallint as smallint_1,
        null::bigint as bigint_1,
        null::real as float_1,
        null::double precision as double_1,
        null::timestamptz as timestamptz_1,
        'hello'::text as string_1
    ) q));
select 'A1', id, ST_AsText(geom), bool_1, int_1, int_2, smallint_1, bigint_1, float_1, double_1, timestamptz_1, string_1 from ST_FromFlatGeobuf(null::flatgeobuf_a1, (
    select ST_AsFlatGeobuf(q) fgb from (select
        null::geometry,
        true::boolean as bool_1,
        1::integer as int_1,
        2::integer as int_2,
        3::smallint as smallint_1,
        4::bigint as bigint_1,
        1.2::real as float_1,
        1.3::double precision as double_1,
        '2016-06-22 20:44:52.134125-07'::timestamptz as timestamptz_1,
        'hello'::text as string_1
    ) q)
);

select '--- Exotic roundtrips ---';

-- 2D Point geometry at column index 2
select ST_FromFlatGeobufToTable('public', 'flatgeobuf_e1', (select ST_AsFlatGeobuf(q) fgb from (select
        true::boolean as bool_1,
        ST_MakePoint(1.1, 2.1),
        false::boolean as bool_2
    ) q));
select 'E1', id, bool_1, ST_AsText(geom), bool_2 from ST_FromFlatGeobuf(null::flatgeobuf_e1, (
    select ST_AsFlatGeobuf(q) fgb from (select
        true::boolean as bool_1,
        ST_MakePoint(1.1, 2.1),
        false::boolean as bool_2
    ) q)
);

select '--- Type mismatch detection ---';

-- The first two output fields are decoded as int4 fid and geometry.
create type flatgeobuf_bad_fid as (fid text, geom geometry);
create type flatgeobuf_bad_geom as (fid integer, geom text);
create domain flatgeobuf_fid_domain as integer;
create domain flatgeobuf_geom_domain as geometry;
create type flatgeobuf_domain_fields as (fid flatgeobuf_fid_domain, geom flatgeobuf_geom_domain);
select 'FM1' from ST_FromFlatGeobuf(null::flatgeobuf_bad_fid, (
    select ST_AsFlatGeobuf(q) from (select ST_MakePoint(1, 2)) q));
select 'FM2' from ST_FromFlatGeobuf(null::flatgeobuf_bad_geom, (
    select ST_AsFlatGeobuf(q) from (select ST_MakePoint(1, 2)) q));
select 'FM3', fid, ST_AsText(geom) from ST_FromFlatGeobuf(null::flatgeobuf_domain_fields, (
    select ST_AsFlatGeobuf(q) from (select ST_MakePoint(1, 2)) q));
drop type flatgeobuf_bad_fid;
drop type flatgeobuf_bad_geom;
drop type flatgeobuf_domain_fields;
drop domain flatgeobuf_fid_domain;
drop domain flatgeobuf_geom_domain;

-- Setup: a long (bigint) column and a text column in separate tables
select ST_FromFlatGeobufToTable('public', 'flatgeobuf_mm_long', (select ST_AsFlatGeobuf(q) fgb from (select
    null::geometry, null::bigint as val) q));
select ST_FromFlatGeobufToTable('public', 'flatgeobuf_mm_text', (select ST_AsFlatGeobuf(q) fgb from (select
    null::geometry, null::text as val) q));
select ST_FromFlatGeobufToTable('public', 'flatgeobuf_mm_twocols', (select ST_AsFlatGeobuf(q) fgb from (select
    null::geometry, null::bigint as val1, null::bigint as val2) q));

-- Type mismatch: file has bigint (long), target expects text
select 'MM1' from ST_FromFlatGeobuf(null::flatgeobuf_mm_text, (
    select ST_AsFlatGeobuf(q) fgb from (select null::geometry, 42::bigint as val) q));

-- Count mismatch: file has 2 property columns, target type has 1
select 'MM2' from ST_FromFlatGeobuf(null::flatgeobuf_mm_long, (
    select ST_AsFlatGeobuf(q) fgb from (select null::geometry, 42::bigint as val1, 43::bigint as val2) q));

select '--- Malformed input detection ---';

-- Magic bytes plus a truncated size prefix.
select 'MI1' from ST_FromFlatGeobuf(null::flatgeobuf_t1, decode('6667620366676201010203', 'hex'));

-- Valid magic bytes plus a header size prefix that exceeds the remaining input.
select 'MI2' from ST_FromFlatGeobuf(null::flatgeobuf_t1, decode('6667620366676201ffffffff', 'hex'));

-- NULL bytea input should be handled by STRICT rather than reaching the C decoder.
select 'MI3', count(*) from ST_FromFlatGeobuf(null::flatgeobuf_t1, null::bytea);

select '--- Quoted identifiers ---';

-- Verify that special characters in column names are properly quoted
select ST_FromFlatGeobufToTable('public', 'flatgeobuf_qi', (
    select ST_AsFlatGeobuf(q) from (
        select null::geometry, null::text as "col name; with special--chars"
    ) q
));
select 'QI1' where exists (
    select 1 from information_schema.columns
    where table_schema = 'public'
      and table_name = 'flatgeobuf_qi'
      and column_name = 'col name; with special--chars'
);
select 'QI2' where exists (
    select 1 from information_schema.tables
    where table_schema = 'public' and table_name = 'flatgeobuf_t1'
);

drop table if exists public.flatgeobuf_t1;
drop table if exists public.flatgeobuf_a1;
drop table if exists public.flatgeobuf_e1;
drop table if exists public.flatgeobuf_qi;
drop table if exists public.flatgeobuf_mm_long;
drop table if exists public.flatgeobuf_mm_text;
drop table if exists public.flatgeobuf_mm_twocols;

-- https://trac.osgeo.org/postgis/ticket/6147
-- NULL rows are skipped by ST_AsFlatGeobuf
CREATE TEMP TABLE fgb_null_row (id int, geom geometry);
INSERT INTO fgb_null_row VALUES (1, 'POINT(1 1)'), (2, 'POINT(2 2)'), (3, 'POINT(3 3)');
SELECT '#6147.a', ST_AsFlatGeobuf(NULL::fgb_null_row) IS NULL;
SELECT '#6147.b', ST_AsFlatGeobuf(CASE WHEN id = 2 THEN NULL::fgb_null_row ELSE t END) =
	(SELECT ST_AsFlatGeobuf(t) FROM fgb_null_row t WHERE id <> 2)
FROM fgb_null_row t;
SELECT '#6147.c', ST_AsFlatGeobuf(CASE WHEN id = 1 THEN NULL::fgb_null_row ELSE t END) =
	(SELECT ST_AsFlatGeobuf(t) FROM fgb_null_row t WHERE id <> 1)
FROM fgb_null_row t;
DROP TABLE fgb_null_row;

-- NULL values of a non-row type are still rejected
SELECT '#6147.d', ST_AsFlatGeobuf(NULL::int);
select 'TG1', ST_AsFlatGeobuf(q, false, 'geom') from (select 1::bigint as geom) q;

-- Reject rows with a different record layout instead of deforming them
-- through the first row's cached descriptor.
DO $$
DECLARE
    error_message text;
BEGIN
    BEGIN
        PERFORM ST_AsFlatGeobuf(v.r)
        FROM (
            SELECT ROW('POINT(0 0)'::geometry, 1::integer) AS r
            UNION ALL
            SELECT ROW(1::integer, 'POINT(1 1)'::geometry)
        ) AS v;
    EXCEPTION WHEN SQLSTATE 'XX000' THEN
        GET STACKED DIAGNOSTICS error_message = MESSAGE_TEXT;
        IF error_message <> 'flatgeobuf_agg_transfn: all rows must share the same column layout' THEN
            RAISE;
        END IF;
        RETURN;
    END;
    RAISE EXCEPTION 'FlatGeobuf accepted rows with different record layouts';
END;
$$;
