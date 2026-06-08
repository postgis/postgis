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

select '--- Numeric widening ---';

-- integer (int32 in file) widened to bigint in target
select ST_FromFlatGeobufToTable('public', 'flatgeobuf_widen_i', (select ST_AsFlatGeobuf(q) fgb from (select
    null::geometry, null::bigint as val) q));
select 'W1', id, val from ST_FromFlatGeobuf(null::flatgeobuf_widen_i, (
    select ST_AsFlatGeobuf(q) fgb from (select null::geometry, 42::integer as val) q));

-- real (float4 in file) widened to double precision in target
select ST_FromFlatGeobufToTable('public', 'flatgeobuf_widen_f', (select ST_AsFlatGeobuf(q) fgb from (select
    null::geometry, null::double precision as val) q));
select 'W2', id, val from ST_FromFlatGeobuf(null::flatgeobuf_widen_f, (
    select ST_AsFlatGeobuf(q) fgb from (select null::geometry, 1.5::real as val) q));

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
drop table if exists public.flatgeobuf_widen_i;
drop table if exists public.flatgeobuf_widen_f;
