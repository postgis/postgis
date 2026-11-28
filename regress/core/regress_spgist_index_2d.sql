
CREATE OR REPLACE FUNCTION qnodes(q text) RETURNS text
LANGUAGE 'plpgsql' AS
$$
DECLARE
  exp TEXT;
  mat TEXT[];
  ret TEXT;
BEGIN
  FOR exp IN EXECUTE 'EXPLAIN ' || q
  LOOP
    --RAISE NOTICE 'EXP: %', exp;
    mat := regexp_matches(exp, ' *(?:-> *)?(.*Scan)');
    --RAISE NOTICE 'MAT: %', mat;
    IF mat IS NOT NULL THEN
      ret := mat[1];
    END IF;
    --RAISE NOTICE 'RET: %', ret;
  END LOOP;
  RETURN ret;
END;
$$;

create table tbl_geomcollection (
	k serial,
	g geometry
);

\copy tbl_geomcollection from 'regress_spgist_index_2d.data' ;

create table test_spgist_idx_2d(
	op char(3),
	noidx bigint,
	noidxscan varchar(32),
	spgistidx bigint,
	spgidxscan varchar(32));

-------------------------------------------------------------------------------

set enable_indexscan = off;
set enable_bitmapscan = off;
set enable_seqscan = on;

insert into test_spgist_idx_2d(op, noidx, noidxscan)
select '<<', count(*), qnodes('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g << t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g << t2.g;
insert into test_spgist_idx_2d(op, noidx, noidxscan)
select '&<', count(*), qnodes('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g &< t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g &< t2.g;
insert into test_spgist_idx_2d(op, noidx, noidxscan)
select '&&', count(*), qnodes('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g && t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g && t2.g;
insert into test_spgist_idx_2d(op, noidx, noidxscan)
select '&>', count(*), qnodes('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g &> t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g &> t2.g;
insert into test_spgist_idx_2d(op, noidx, noidxscan)
select '>>', count(*), qnodes('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g >> t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g >> t2.g;
insert into test_spgist_idx_2d(op, noidx, noidxscan)
select '~=', count(*), qnodes('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g ~= t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g ~= t2.g;
insert into test_spgist_idx_2d(op, noidx, noidxscan)
select '~', count(*), qnodes('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g ~ t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g ~ t2.g;
insert into test_spgist_idx_2d(op, noidx, noidxscan)
select '@', count(*), qnodes('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g @ t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g @ t2.g;
insert into test_spgist_idx_2d(op, noidx, noidxscan)
select '&<|', count(*), qnodes('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g &<| t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g &<| t2.g;
insert into test_spgist_idx_2d(op, noidx, noidxscan)
select '<<|', count(*), qnodes('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g <<| t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g <<| t2.g;
insert into test_spgist_idx_2d(op, noidx, noidxscan)
select '|>>', count(*), qnodes('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g |>> t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g |>> t2.g;
insert into test_spgist_idx_2d(op, noidx, noidxscan)
select '|&>', count(*), qnodes('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g |&> t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g |&> t2.g;

------------------------------------------------------------------------------

create index tbl_geomcollection_spgist_2d_idx on tbl_geomcollection using spgist(g);

set enable_indexscan = on;
set enable_bitmapscan = off;
set enable_seqscan = off;

update test_spgist_idx_2d
set spgistidx = ( select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g << t2.g ),
spgidxscan = qnodes(' select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g << t2.g ')
where op = '<<';
update test_spgist_idx_2d
set spgistidx = ( select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g &< t2.g ),
spgidxscan = qnodes(' select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g &< t2.g ')
where op = '&<';
update test_spgist_idx_2d
set spgistidx = ( select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g && t2.g ),
spgidxscan = qnodes(' select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g && t2.g ')
where op = '&&';
update test_spgist_idx_2d
set spgistidx = ( select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g &> t2.g ),
spgidxscan = qnodes(' select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g &> t2.g ')
where op = '&>';
update test_spgist_idx_2d
set spgistidx = ( select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g >> t2.g ),
spgidxscan = qnodes(' select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g >> t2.g ')
where op = '>>';
update test_spgist_idx_2d
set spgistidx = ( select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g ~= t2.g ),
spgidxscan = qnodes(' select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g ~= t2.g ')
where op = '~=';
update test_spgist_idx_2d
set spgistidx = ( select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g ~ t2.g ),
spgidxscan = qnodes(' select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g ~ t2.g ')
where op = '~';
update test_spgist_idx_2d
set spgistidx = ( select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g @ t2.g ),
spgidxscan = qnodes(' select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g @ t2.g ')
where op = '@';
update test_spgist_idx_2d
set spgistidx = ( select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g &<| t2.g ),
spgidxscan = qnodes(' select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g &<| t2.g ')
where op = '&<|';
update test_spgist_idx_2d
set spgistidx = ( select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g <<| t2.g ),
spgidxscan = qnodes(' select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g <<| t2.g ')
where op = '<<|';
update test_spgist_idx_2d
set spgistidx = ( select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g |>> t2.g ),
spgidxscan = qnodes(' select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g |>> t2.g ')
where op = '|>>';
update test_spgist_idx_2d
set spgistidx = ( select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g |&> t2.g ),
spgidxscan = qnodes(' select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g |&> t2.g ')
where op = '|&>';

-------------------------------------------------------------------------------

select * from test_spgist_idx_2d;

-------------------------------------------------------------------------------
-- EMPTY and NaN geometries in a SP-GiST 2D index must behave as in a
-- sequential scan: an empty box overlaps nothing, is contained by
-- everything and is the same only as another empty box.

create table tbl_spgist_2d_empty (id int, tag text, g geometry);
insert into tbl_spgist_2d_empty values
	(1, 'in',    ST_SetSRID(ST_MakePoint(15, 15), 4326)),
	(2, 'nan',   ST_SetSRID(ST_MakePoint('NaN', 'NaN'), 4326)),
	(3, 'empty', ST_SetSRID('POINT EMPTY'::geometry, 4326)),
	(4, 'edge',  ST_SetSRID(ST_MakePoint(10, 10), 4326)),
	(5, 'out',   ST_SetSRID(ST_MakePoint(50, 50), 4326));
create index tbl_spgist_2d_empty_idx on tbl_spgist_2d_empty
	using spgist (g spgist_geometry_ops_2d);
analyze tbl_spgist_2d_empty;

set enable_indexscan = on;
set enable_bitmapscan = off;
set enable_seqscan = off;

select 'empty_nan_scan', qnodes($q$select * from tbl_spgist_2d_empty where g && ST_MakeEnvelope(10, 10, 20, 20, 4326)$q$);
select 'empty_nan_&&', coalesce(string_agg(tag, ',' order by tag), '(none)')
	from tbl_spgist_2d_empty where g && ST_MakeEnvelope(10, 10, 20, 20, 4326);
select 'empty_nan_&&_far', coalesce(string_agg(tag, ',' order by tag), '(none)')
	from tbl_spgist_2d_empty where g && ST_MakeEnvelope(0, 0, 1, 1, 4326);
select 'empty_nan_@', coalesce(string_agg(tag, ',' order by tag), '(none)')
	from tbl_spgist_2d_empty where g @ ST_MakeEnvelope(10, 10, 20, 20, 4326);
select 'empty_nan_~=', coalesce(string_agg(tag, ',' order by tag), '(none)')
	from tbl_spgist_2d_empty where g ~= 'POINT EMPTY'::geometry;

drop table tbl_spgist_2d_empty;

-- The same on an index with inner pages: 1000 rows, 10 EMPTY and
-- 10 NaN rows among a grid of points. The expected counts are the
-- sequential scan counts.

create table tbl_spgist_2d_empty_pages (id int, g geometry);
insert into tbl_spgist_2d_empty_pages select i,
	case when i % 100 = 0 then 'POINT EMPTY'::geometry
	     when i % 100 = 50 then ST_MakePoint('NaN', 'NaN')
	     else ST_MakePoint(i % 40, i / 40) end
	from generate_series(1, 1000) i;
create index tbl_spgist_2d_empty_pages_idx on tbl_spgist_2d_empty_pages
	using spgist (g spgist_geometry_ops_2d);
analyze tbl_spgist_2d_empty_pages;

select 'empty_nan_pages_scan', qnodes($q$select * from tbl_spgist_2d_empty_pages where g && ST_MakeEnvelope(10, 10, 20, 20)$q$);
select 'empty_nan_pages_&&', count(*)
	from tbl_spgist_2d_empty_pages where g && ST_MakeEnvelope(10, 10, 20, 20);
select 'empty_nan_pages_@', count(*)
	from tbl_spgist_2d_empty_pages where g @ ST_MakeEnvelope(10, 10, 20, 20);
select 'empty_nan_pages_~=', count(*)
	from tbl_spgist_2d_empty_pages where g ~= 'POINT EMPTY'::geometry;

drop table tbl_spgist_2d_empty_pages;

-------------------------------------------------------------------------------

DROP TABLE tbl_geomcollection CASCADE;
DROP TABLE test_spgist_idx_2d CASCADE;
DROP FUNCTION qnodes;
