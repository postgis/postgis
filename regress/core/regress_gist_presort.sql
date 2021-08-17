-------------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION info(q text) RETURNS text
LANGUAGE 'plpgsql' AS
$$
DECLARE
  exp TEXT;
  mat TEXT[];
  scan TEXT;
  ret TEXT[];
  tmp TEXT[];
BEGIN
  FOR exp IN EXECUTE 'EXPLAIN(ANALYZE,VERBOSE,BUFFERS) ' || q
  LOOP
	mat = array_append(mat, exp);
  END LOOP;  
  RETURN array_to_string(mat, ';');
END;
$$;

CREATE TABLE tbl_geomcollection (
  k serial,
  g geometry
);

\COPY tbl_geomcollection FROM 'regress_gist_point_small.data';

create table test_gist_presort_2d(
  op char(3),
  idx bigint,
  idxscan varchar(4096)
);
-------------------------------------------------------------------------------
\timing

CREATE INDEX ON tbl_geomcollection USING gist(g gist_geometry_ops_2d);

\timing

-------------------------------------------------------------------------------
-- set enable_indexscan = off;
-- set enable_bitmapscan = off;
-- set enable_seqscan = on;

set enable_indexscan = on;
set enable_bitmapscan = off;
set enable_seqscan = off;

insert into test_gist_presort_2d(op, idx, idxscan)
select '&&', count(*), info('select count(*) from tbl_geomcollection where g && ST_MakeBox2D(ST_MakePoint(0, 0), ST_MakePoint(0.5, 0.5))')
  from tbl_geomcollection where g && ST_MakeBox2D(ST_MakePoint(0, 0), ST_MakePoint(0.5, 0.5));

-- insert into test_gist_presort_2d(op, idx, idxscan)
-- select '<<', count(*), info('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g << t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g << t2.g;
-- insert into test_gist_presort_2d(op, idx, idxscan)
-- select '&<', count(*), info('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g &< t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g &< t2.g;
-- insert into test_gist_presort_2d(op, idx, idxscan)
-- select '&&', count(*), info('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g && t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g && t2.g;
-- insert into test_gist_presort_2d(op, idx, idxscan)
-- select '&>', count(*), info('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g &> t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g &> t2.g;
-- insert into test_gist_presort_2d(op, idx, idxscan)
-- select '>>', count(*), info('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g >> t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g >> t2.g;
-- insert into test_gist_presort_2d(op, idx, idxscan)
-- select '~=', count(*), info('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g ~= t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g ~= t2.g;
-- insert into test_gist_presort_2d(op, idx, idxscan)
-- select '~', count(*), info('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g ~ t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g ~ t2.g;
-- insert into test_gist_presort_2d(op, idx, idxscan)
-- select '@', count(*), info('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g @ t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g @ t2.g;
-- insert into test_gist_presort_2d(op, idx, idxscan)
-- select '&<|', count(*), info('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g &<| t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g &<| t2.g;
-- insert into test_gist_presort_2d(op, idx, idxscan)
-- select '<<|', count(*), info('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g <<| t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g <<| t2.g;
-- insert into test_gist_presort_2d(op, idx, idxscan)
-- select '|>>', count(*), info('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g |>> t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g |>> t2.g;
-- insert into test_gist_presort_2d(op, idx, idxscan)
-- select '|&>', count(*), info('select count(*) from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g |&> t2.g') from tbl_geomcollection t1, tbl_geomcollection t2 where t1.g |&> t2.g;

SELECT * FROM test_gist_presort_2d;
-------------------------------------------------------------------------------
DROP TABLE test_gist_presort_2d CASCADE;
DROP TABLE tbl_geomcollection CASCADE;
DROP FUNCTION info;