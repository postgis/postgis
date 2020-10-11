begin transaction;
select ST_TableFromFlatGeobuf('public', 'flatgeobuf_t1');
-- single row null geometry
select 'T1', id, ST_AsText(geom) from ST_FromFlatGeobuf(null::flatgeobuf_t1, (
    select ST_AsFlatGeobuf(q) fgb from (select null::geometry) q)
);
-- no data (should result in header only FlatGeobuf)
--select 'T2', id, ST_AsText(geom) from ST_FromFlatGeobuf(null::flatgeobuf_t1, (
--    select ST_AsFlatGeobuf(q) fgb from (select null::geometry limit 0) q)
--);
-- 2D Point
select 'T3', id, ST_AsText(geom) from ST_FromFlatGeobuf(null::flatgeobuf_t1, (
    select ST_AsFlatGeobuf(q) fgb from (select ST_MakePoint(1.1, 2.1)) q)
);
-- 3D Point
select 'T4', id, ST_AsText(geom) from ST_FromFlatGeobuf(null::flatgeobuf_t1, (
    select ST_AsFlatGeobuf(q) fgb from (select ST_MakePoint(1.1, 2.11, 3.2)) q)
);
-- 4D Point
select 'T5', id, ST_AsText(geom) from ST_FromFlatGeobuf(null::flatgeobuf_t1, (
    select ST_AsFlatGeobuf(q) fgb from (select ST_MakePoint(1.1, 2.12, 3.2, 4.3)) q)
);

commit;
