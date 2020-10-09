begin transaction;
select ST_TableFromFlatGeobuf('public', 'flatgeobuf_t1');
select 'T1', id, ST_AsText(geom) from ST_FromFlatGeobuf(null::flatgeobuf_t1, (
    select ST_AsFlatGeobuf(q) fgb from (select ST_MakePoint(1.1, 2.1)) q)
);
commit;
