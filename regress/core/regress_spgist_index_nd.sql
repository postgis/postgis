
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

create table tbl_geomcollection_nd (
	k serial,
	g geometry
);

\copy tbl_geomcollection_nd from 'regress_gist_index_nd.data';

create table test_spgist_idx_nd(
	op char(3),
	noidx bigint,
	noidxscan varchar(32),
	spgistidx bigint,
	spgidxscan varchar(32));

-------------------------------------------------------------------------------

set enable_indexscan = off;
set enable_bitmapscan = off;
set enable_seqscan = on;

insert into test_spgist_idx_nd(op, noidx, noidxscan)
select '&&&', count(*), qnodes('select count(*) from tbl_geomcollection_nd t1, tbl_geomcollection_nd t2 where t1.g &&& t2.g') from tbl_geomcollection_nd t1, tbl_geomcollection_nd t2 where t1.g &&& t2.g;
insert into test_spgist_idx_nd(op, noidx, noidxscan)
select '~~', count(*), qnodes('select count(*) from tbl_geomcollection_nd t1, tbl_geomcollection_nd t2 where t1.g ~~ t2.g') from tbl_geomcollection_nd t1, tbl_geomcollection_nd t2 where t1.g ~~ t2.g;
insert into test_spgist_idx_nd(op, noidx, noidxscan)
select '@@', count(*), qnodes('select count(*) from tbl_geomcollection_nd t1, tbl_geomcollection_nd t2 where t1.g @@ t2.g') from tbl_geomcollection_nd t1, tbl_geomcollection_nd t2 where t1.g @@ t2.g;
insert into test_spgist_idx_nd(op, noidx, noidxscan)
select '~~=', count(*), qnodes('select count(*) from tbl_geomcollection_nd t1, tbl_geomcollection_nd t2 where t1.g ~~= t2.g') from tbl_geomcollection_nd t1, tbl_geomcollection_nd t2 where t1.g ~~= t2.g;

------------------------------------------------------------------------------

create index tbl_geomcollection_nd_spgist_nd_idx on tbl_geomcollection_nd using spgist(g spgist_geometry_ops_nd);

set enable_indexscan = on;
set enable_bitmapscan = off;
set enable_seqscan = off;

update test_spgist_idx_nd
set spgistidx = ( select count(*) from tbl_geomcollection_nd t1, tbl_geomcollection_nd t2 where t1.g &&& t2.g ),
spgidxscan = qnodes(' select count(*) from tbl_geomcollection_nd t1, tbl_geomcollection_nd t2 where t1.g &&& t2.g ')
where op = '&&&';
update test_spgist_idx_nd
set spgistidx = ( select count(*) from tbl_geomcollection_nd t1, tbl_geomcollection_nd t2 where t1.g ~~ t2.g ),
spgidxscan = qnodes(' select count(*) from tbl_geomcollection_nd t1, tbl_geomcollection_nd t2 where t1.g ~~ t2.g ')
where op = '~~';
update test_spgist_idx_nd
set spgistidx = ( select count(*) from tbl_geomcollection_nd t1, tbl_geomcollection_nd t2 where t1.g @@ t2.g ),
spgidxscan = qnodes(' select count(*) from tbl_geomcollection_nd t1, tbl_geomcollection_nd t2 where t1.g @@ t2.g ')
where op = '@@';
update test_spgist_idx_nd
set spgistidx = ( select count(*) from tbl_geomcollection_nd t1, tbl_geomcollection_nd t2 where t1.g ~~= t2.g ),
spgidxscan = qnodes(' select count(*) from tbl_geomcollection_nd t1, tbl_geomcollection_nd t2 where t1.g ~~= t2.g ')
where op = '~~=';

-------------------------------------------------------------------------------

select * from test_spgist_idx_nd;

-------------------------------------------------------------------------------
-- SP-GiST ND indexes on EMPTY values and on values with NaN coordinates.
-- The compress function must return a valid key for every non-NULL
-- value: CREATE INDEX, INSERT and UPDATE must work, and these rows must
-- not match &&& (geometry) or && (geography) against a real box.
-- Output: label | plan | EMPTY ids | all ids | ids found through the index

-- EMPTY, geometry: CREATE INDEX on existing rows
create table tbl_spgist_nd_build_geom (id int, g geometry);
insert into tbl_spgist_nd_build_geom values (1, 'POINT(15 15)'), (2, 'POINT EMPTY');
create index tbl_spgist_nd_build_geom_idx on tbl_spgist_nd_build_geom
	using spgist (g spgist_geometry_ops_nd);
select 'nd_empty_build_geom',
	qnodes($q$select * from tbl_spgist_nd_build_geom where g &&& 'POINT(15 15)'::geometry$q$),
	(select string_agg(id::text, ',' order by id) from tbl_spgist_nd_build_geom where ST_IsEmpty(g)),
	(select string_agg(id::text, ',' order by id) from tbl_spgist_nd_build_geom),
	(select string_agg(id::text, ',' order by id) from tbl_spgist_nd_build_geom where g &&& 'POINT(15 15)'::geometry);

-- EMPTY, geometry: INSERT and UPDATE into an existing index
create table tbl_spgist_nd_insert_geom (id int, g geometry);
create index tbl_spgist_nd_insert_geom_idx on tbl_spgist_nd_insert_geom
	using spgist (g spgist_geometry_ops_nd);
insert into tbl_spgist_nd_insert_geom values (1, 'POINT(15 15)'), (2, 'POINT(16 16)');
insert into tbl_spgist_nd_insert_geom values (3, 'POINT EMPTY');
update tbl_spgist_nd_insert_geom set g = 'POINT EMPTY' where id = 2;
select 'nd_empty_insert_geom',
	qnodes($q$select * from tbl_spgist_nd_insert_geom where g &&& 'POINT(15 15)'::geometry$q$),
	(select string_agg(id::text, ',' order by id) from tbl_spgist_nd_insert_geom where ST_IsEmpty(g)),
	(select string_agg(id::text, ',' order by id) from tbl_spgist_nd_insert_geom),
	(select string_agg(id::text, ',' order by id) from tbl_spgist_nd_insert_geom where g &&& 'POINT(15 15)'::geometry);

-- EMPTY, geography: CREATE INDEX on existing rows
create table tbl_spgist_nd_build_geog (id int, g geography);
insert into tbl_spgist_nd_build_geog values
	(1, 'SRID=4326;POINT(15 15)'::geography), (2, 'SRID=4326;POINT EMPTY'::geography);
create index tbl_spgist_nd_build_geog_idx on tbl_spgist_nd_build_geog
	using spgist (g spgist_geography_ops_nd);
select 'nd_empty_build_geog',
	qnodes($q$select * from tbl_spgist_nd_build_geog where g && 'SRID=4326;POINT(15 15)'::geography$q$),
	(select string_agg(id::text, ',' order by id) from tbl_spgist_nd_build_geog where ST_IsEmpty(g::geometry)),
	(select string_agg(id::text, ',' order by id) from tbl_spgist_nd_build_geog),
	(select string_agg(id::text, ',' order by id) from tbl_spgist_nd_build_geog where g && 'SRID=4326;POINT(15 15)'::geography);

-- EMPTY, geography: INSERT and UPDATE into an existing index
create table tbl_spgist_nd_insert_geog (id int, g geography);
create index tbl_spgist_nd_insert_geog_idx on tbl_spgist_nd_insert_geog
	using spgist (g spgist_geography_ops_nd);
insert into tbl_spgist_nd_insert_geog values
	(1, 'SRID=4326;POINT(15 15)'::geography), (2, 'SRID=4326;POINT(16 16)'::geography);
insert into tbl_spgist_nd_insert_geog values (3, 'SRID=4326;POINT EMPTY'::geography);
update tbl_spgist_nd_insert_geog set g = 'SRID=4326;POINT EMPTY'::geography where id = 2;
select 'nd_empty_insert_geog',
	qnodes($q$select * from tbl_spgist_nd_insert_geog where g && 'SRID=4326;POINT(15 15)'::geography$q$),
	(select string_agg(id::text, ',' order by id) from tbl_spgist_nd_insert_geog where ST_IsEmpty(g::geometry)),
	(select string_agg(id::text, ',' order by id) from tbl_spgist_nd_insert_geog),
	(select string_agg(id::text, ',' order by id) from tbl_spgist_nd_insert_geog where g && 'SRID=4326;POINT(15 15)'::geography);

-- NaN rows: check only that CREATE INDEX and INSERT work and that the
-- rows are stored. A sequential scan finds NaN rows with &&& and && for
-- any box, a GiST ND index does not, so the query result is left out.
-- Output: label | all ids

-- NaN, geography: CREATE INDEX on existing rows
create table tbl_spgist_nd_nan_geog (id int, g geography);
insert into tbl_spgist_nd_nan_geog values
	(1, 'SRID=4326;POINT(15 15)'::geography),
	(2, ST_SetSRID(ST_MakePoint('NaN', 'NaN'), 4326)::geography);
create index tbl_spgist_nd_nan_geog_idx on tbl_spgist_nd_nan_geog
	using spgist (g spgist_geography_ops_nd);
-- NaN, geography: INSERT into an existing index
insert into tbl_spgist_nd_nan_geog values
	(3, ST_SetSRID(ST_MakePoint('NaN', 'NaN'), 4326)::geography);
select 'nd_nan_geog', string_agg(id::text, ',' order by id) from tbl_spgist_nd_nan_geog;

-- NaN, geometry: CREATE INDEX on existing rows
create table tbl_spgist_nd_nan_geom (id int, g geometry);
insert into tbl_spgist_nd_nan_geom values (1, 'POINT(15 15)'), (2, ST_MakePoint('NaN', 'NaN'));
create index tbl_spgist_nd_nan_geom_idx on tbl_spgist_nd_nan_geom
	using spgist (g spgist_geometry_ops_nd);
select 'nd_nan_geom', string_agg(id::text, ',' order by id) from tbl_spgist_nd_nan_geom;

drop table tbl_spgist_nd_build_geom;
drop table tbl_spgist_nd_insert_geom;
drop table tbl_spgist_nd_build_geog;
drop table tbl_spgist_nd_insert_geog;
drop table tbl_spgist_nd_nan_geog;
drop table tbl_spgist_nd_nan_geom;

-------------------------------------------------------------------------------

DROP TABLE tbl_geomcollection_nd CASCADE;
DROP TABLE test_spgist_idx_nd CASCADE;
DROP FUNCTION qnodes;

