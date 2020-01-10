--- build a larger database
\i :regdir/core/regress_lots_of_points.sql

--- Test the various BRIN opclass with dataset containing 2D geometries

CREATE OR REPLACE FUNCTION qnodes(q text) RETURNS text
LANGUAGE 'plpgsql' AS
$$
DECLARE
  exp TEXT;
  mat TEXT[];
  ret TEXT[];
BEGIN
  FOR exp IN EXECUTE 'EXPLAIN ' || q
  LOOP
    --RAISE NOTICE 'EXP: %', exp;
    mat := regexp_matches(exp, ' *(?:-> *)?(.*Scan)');
    --RAISE NOTICE 'MAT: %', mat;
    IF mat IS NOT NULL THEN
      ret := array_append(ret, mat[1]);
    END IF;
    --RAISE NOTICE 'RET: %', ret;
  END LOOP;
  RETURN array_to_string(ret,',');
END;
$$;

-- BRIN indexes

-- 2D
CREATE INDEX brin_2d on test using brin (the_geom);

set enable_indexscan = off;
set enable_bitmapscan = off;
set enable_seqscan = on;

SELECT 'scan_seq', qnodes('select * from test where the_geom && ST_MakePoint(0,0)');
 select num,ST_astext(the_geom) from test where the_geom && 'BOX(125 125,135 135)'::box2d order by num;

SELECT 'scan_seq', qnodes('select * from test where ST_MakePoint(0,0) ~ the_geom');
 select num,ST_astext(the_geom) from test where 'BOX(125 125,135 135)'::box2d ~ the_geom order by num;

SELECT 'scan_seq', qnodes('select * from test where the_geom @ ST_MakePoint(0,0)');
 select num,ST_astext(the_geom) from test where the_geom @ 'BOX(125 125,135 135)'::box2d order by num;

set enable_indexscan = off;
set enable_bitmapscan = on;
set enable_seqscan = off;

SELECT 'scan_idx', qnodes('select * from test where the_geom && ST_MakePoint(0,0)');
 select num,ST_astext(the_geom) from test where the_geom && 'BOX(125 125,135 135)'::box2d order by num;

SELECT 'scan_idx', qnodes('select * from test where ST_MakePoint(0,0) ~ the_geom');
 select num,ST_astext(the_geom) from test where 'BOX(125 125,135 135)'::box2d ~ the_geom order by num;

SELECT 'scan_idx', qnodes('select * from test where the_geom @ ST_MakePoint(0,0)');
 select num,ST_astext(the_geom) from test where the_geom @ 'BOX(125 125,135 135)'::box2d order by num;

DROP INDEX brin_2d;

-- 3D
CREATE INDEX brin_3d on test using brin (the_geom brin_geometry_inclusion_ops_3d);

set enable_indexscan = off;
set enable_bitmapscan = off;
set enable_seqscan = on;

SELECT 'scan_seq', qnodes('select * from test where the_geom &&& ST_MakePoint(0,0)');
 select num,ST_astext(the_geom) from test where the_geom &&& 'BOX3D(125 125,135 135)'::box3d order by num;

set enable_indexscan = off;
set enable_bitmapscan = on;
set enable_seqscan = off;

SELECT 'scan_idx', qnodes('select * from test where the_geom &&& ST_MakePoint(0,0)');
 select num,ST_astext(the_geom) from test where the_geom &&& 'BOX3D(125 125,135 135)'::box3d order by num;

DROP INDEX brin_3d;

-- 4D
CREATE INDEX brin_4d on test using brin (the_geom brin_geometry_inclusion_ops_4d);

set enable_indexscan = off;
set enable_bitmapscan = off;
set enable_seqscan = on;

SELECT 'scan_seq', qnodes('select * from test where the_geom &&& ST_MakePoint(0,0)');
 select num,ST_astext(the_geom) from test where the_geom &&& 'BOX3D(125 125,135 135)'::box3d order by num;

set enable_indexscan = off;
set enable_bitmapscan = on;
set enable_seqscan = off;

SELECT 'scan_idx', qnodes('select * from test where the_geom &&& ST_MakePoint(0,0)');
 select num,ST_astext(the_geom) from test where the_geom &&& 'BOX3D(125 125,135 135)'::box3d order by num;

DROP INDEX brin_4d;

-- test adding rows and unsummarized ranges
--

-- 2D
TRUNCATE TABLE test;
INSERT INTO test select 1, st_makepoint(1, 1);
CREATE INDEX brin_2d on test using brin (the_geom) WITH (pages_per_range = 1);
INSERT INTO test select i, st_makepoint(i, i) FROM generate_series(2, 3) i;

set enable_indexscan = off;
set enable_bitmapscan = on;
set enable_seqscan = off;

SELECT 'scan_idx', qnodes('select count(*) from test where the_geom && ''BOX(2.1 2.1, 3.1 3.1)''::box2d');
 select '2d', count(*) from test where the_geom && 'BOX(2.1 2.1, 3.1 3.1)'::box2d;

INSERT INTO test select i, st_makepoint(i, i) FROM generate_series(4, 1000) i;
SELECT 'scan_idx', qnodes('select count(*) from test where the_geom && ''BOX(900.1 900.1, 920.1 920.1)''::box2d');
 select '2d', count(*) from test where the_geom && 'BOX(900.1 900.1, 920.1 920.1)'::box2d;

SELECT 'summarize 2d', brin_summarize_new_values('brin_2d');

SELECT 'scan_idx', qnodes('select count(*) from test where the_geom && ''BOX(900.1 900.1, 920.1 920.1)''::box2d');
 select '2d', count(*) from test where the_geom && 'BOX(900.1 900.1, 920.1 920.1)'::box2d;

DROP INDEX brin_2d;

-- 3D
TRUNCATE TABLE test;
INSERT INTO test select 1, st_makepoint(1, 1);
CREATE INDEX brin_3d on test using brin (the_geom brin_geometry_inclusion_ops_3d) WITH (pages_per_range = 1);
INSERT INTO test select i, st_makepoint(i, i) FROM generate_series(2, 3) i;

set enable_indexscan = off;
set enable_bitmapscan = on;
set enable_seqscan = off;

SELECT 'scan_idx', qnodes('select count(*) from test where the_geom &&& ''BOX3D(2.1 2.1 2.1, 3.1 3.1 3.1)''::box3d');
 select '3d', count(*) from test where the_geom &&& 'BOX3D(2.1 2.1 2.1, 3.1 3.1 3.1)'::box3d;

INSERT INTO test select i, st_makepoint(i, i) FROM generate_series(4, 1000) i;
SELECT 'scan_idx', qnodes('select count(*) from test where the_geom &&& ''BOX3D(900.1 900.1 900.1, 920.1 920.1 920.1)''::box3d');
 select '3d', count(*) from test where the_geom &&& 'BOX3D(900.1 900.1 900.1, 920.1 920.1 920.1)'::box3d;

SELECT 'summarize 3d', brin_summarize_new_values('brin_3d');

SELECT 'scan_idx', qnodes('select count(*) from test where the_geom &&& ''BOX3D(900.1 900.1 900.1, 920.1 920.1 920.1)''::box3d');
 select '3d', count(*) from test where the_geom &&& 'BOX3D(900.1 900.1 900.1, 920.1 920.1 920.1)'::box3d;

DROP INDEX brin_3d;

-- 4D
TRUNCATE TABLE test;
INSERT INTO test select 1, st_makepoint(1, 1);
CREATE INDEX brin_4d on test using brin (the_geom brin_geometry_inclusion_ops_4d) WITH (pages_per_range = 1);
INSERT INTO test select i, st_makepoint(i, i) FROM generate_series(2, 3) i;

set enable_indexscan = off;
set enable_bitmapscan = on;
set enable_seqscan = off;

SELECT 'scan_idx', qnodes('select count(*) from test where the_geom &&& ''BOX3D(2.1 2.1 2.1, 3.1 3.1 3.1)''::box3d');
 select '4d', count(*) from test where the_geom &&& 'BOX3D(2.1 2.1 2.1, 3.1 3.1 3.1)'::box3d;

INSERT INTO test select i, st_makepoint(i, i) FROM generate_series(4, 1000) i;
SELECT 'scan_idx', qnodes('select count(*) from test where the_geom &&& ''BOX3D(900.1 900.1 900.1, 920.1 920.1 920.1)''::box3d');
 select '4d', count(*) from test where the_geom &&& 'BOX3D(900.1 900.1 900.1, 920.1 920.1 920.1)'::box3d;

SELECT 'summarize 4d', brin_summarize_new_values('brin_4d');

SELECT 'scan_idx', qnodes('select count(*) from test where the_geom &&& ''BOX3D(900.1 900.1 900.1, 920.1 920.1 920.1)''::box3d');
 select '4d', count(*) from test where the_geom &&& 'BOX3D(900.1 900.1 900.1, 920.1 920.1 920.1)'::box3d;

DROP INDEX brin_4d;

-- cleanup
DROP TABLE test;
DROP FUNCTION qnodes(text);

set enable_indexscan = on;
set enable_bitmapscan = on;
set enable_seqscan = on;
