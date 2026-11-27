
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

-------------------------------------------------------------------------------

create table tbl_geomcollection (
	k serial,
	g geometry
);

\copy tbl_geomcollection from 'regress_spgist_index_3d.data';

create table test_spgist_idx_3d(
	op char(3),
	noidx bigint,
	noidxscan varchar(32),
	spgistidx bigint,
	spgidxscan varchar(32));

-------------------------------------------------------------------------------

set enable_indexscan = off;
set enable_bitmapscan = off;
set enable_seqscan = on;

insert into test_spgist_idx_3d(op, noidx, noidxscan)
select '&/&', count(*), qnodes('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g &/& t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g &/& t2.g;
insert into test_spgist_idx_3d(op, noidx, noidxscan)
select '@>>', count(*), qnodes('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g @>> t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g @>> t2.g;
insert into test_spgist_idx_3d(op, noidx, noidxscan)
select '<<@', count(*), qnodes('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g <<@ t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g <<@ t2.g;
insert into test_spgist_idx_3d(op, noidx, noidxscan)
select '~==', count(*), qnodes('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g ~== t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g ~== t2.g;

------------------------------------------------------------------------------

create index tbl_geomcollection_spgist_3d_idx on tbl_geomcollection using spgist(g spgist_geometry_ops_3d);

set enable_indexscan = on;
set enable_bitmapscan = off;
set enable_seqscan = off;

update test_spgist_idx_3d
set spgistidx = ( select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g &/& t2.g ),
spgidxscan = qnodes(' select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g &/& t2.g ')
where op = '&/&';
update test_spgist_idx_3d
set spgistidx = ( select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g @>> t2.g ),
spgidxscan = qnodes(' select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g @>> t2.g ')
where op = '@>>';
update test_spgist_idx_3d
set spgistidx = ( select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g <<@ t2.g ),
spgidxscan = qnodes(' select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g <<@ t2.g ')
where op = '<<@';
update test_spgist_idx_3d
set spgistidx = ( select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g ~== t2.g ),
spgidxscan = qnodes(' select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g ~== t2.g ')
where op = '~==';

-------------------------------------------------------------------------------

select * from test_spgist_idx_3d;

-------------------------------------------------------------------------------
-- EMPTY geometries with the 3D operators and a SP-GiST 3D index. An
-- EMPTY box overlaps nothing, as with && and &&&. The &/& operator,
-- CREATE INDEX, INSERT and UPDATE must not raise an error.
-- Output: label | plan | EMPTY ids | all ids | ids that match &/&

select '3d_empty_op',
	'POINT EMPTY'::geometry &/& 'POINT Z (15 15 15)'::geometry,
	'POINT Z (15 15 15)'::geometry &/& 'POINT EMPTY'::geometry;

-- Sequential scan
create table tbl_spgist_3d_empty (id int, g geometry);
insert into tbl_spgist_3d_empty values (1, 'POINT Z (15 15 15)'), (2, 'POINT EMPTY');

set enable_indexscan = off;
set enable_bitmapscan = off;
set enable_seqscan = on;

select '3d_empty_seq',
	qnodes($q$select * from tbl_spgist_3d_empty where g &/& 'POINT Z (15 15 15)'::geometry$q$),
	(select string_agg(id::text, ',' order by id) from tbl_spgist_3d_empty where ST_IsEmpty(g)),
	(select string_agg(id::text, ',' order by id) from tbl_spgist_3d_empty),
	(select string_agg(id::text, ',' order by id) from tbl_spgist_3d_empty where g &/& 'POINT Z (15 15 15)'::geometry);

-- CREATE INDEX on existing rows
create index tbl_spgist_3d_empty_idx on tbl_spgist_3d_empty
	using spgist (g spgist_geometry_ops_3d);

set enable_indexscan = on;
set enable_bitmapscan = off;
set enable_seqscan = off;

select '3d_empty_build',
	qnodes($q$select * from tbl_spgist_3d_empty where g &/& 'POINT Z (15 15 15)'::geometry$q$),
	(select string_agg(id::text, ',' order by id) from tbl_spgist_3d_empty where ST_IsEmpty(g)),
	(select string_agg(id::text, ',' order by id) from tbl_spgist_3d_empty),
	(select string_agg(id::text, ',' order by id) from tbl_spgist_3d_empty where g &/& 'POINT Z (15 15 15)'::geometry);

-- INSERT and UPDATE into an existing index
create table tbl_spgist_3d_empty_ins (id int, g geometry);
create index tbl_spgist_3d_empty_ins_idx on tbl_spgist_3d_empty_ins
	using spgist (g spgist_geometry_ops_3d);
insert into tbl_spgist_3d_empty_ins values (1, 'POINT Z (15 15 15)'), (2, 'POINT Z (16 16 16)');
insert into tbl_spgist_3d_empty_ins values (3, 'POINT EMPTY');
update tbl_spgist_3d_empty_ins set g = 'POINT EMPTY' where id = 2;
select '3d_empty_insert',
	qnodes($q$select * from tbl_spgist_3d_empty_ins where g &/& 'POINT Z (15 15 15)'::geometry$q$),
	(select string_agg(id::text, ',' order by id) from tbl_spgist_3d_empty_ins where ST_IsEmpty(g)),
	(select string_agg(id::text, ',' order by id) from tbl_spgist_3d_empty_ins),
	(select string_agg(id::text, ',' order by id) from tbl_spgist_3d_empty_ins where g &/& 'POINT Z (15 15 15)'::geometry);

drop table tbl_spgist_3d_empty;
drop table tbl_spgist_3d_empty_ins;

-------------------------------------------------------------------------------

DROP TABLE tbl_geomcollection CASCADE;
DROP TABLE test_spgist_idx_3d CASCADE;
DROP FUNCTION qnodes;

