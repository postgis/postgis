--set client_min_messages to DEBUG3;

select ST_TableFromFlatGeobuf('public', 'flatgeobuf_t1', (select ST_AsFlatGeobuf(q) fgb from (select null::geometry) q));

select 'T1', id, ST_AsText(geom) from ST_FromFlatGeobuf(null::flatgeobuf_t1,
    '\x66676203666762002200000004000000ecffffff080000000100000000000000000000000a0009000400000008003600000004000000d4ffffff04000000e4ffffff04000000020000009a9999999999f13fcdcccccccccc00400800080000000400060008000400'::bytea
);

drop table if exists public.flatgeobuf_t1;