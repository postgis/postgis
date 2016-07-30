--- build a larger database
\i regress_lots_of_3dpoints.sql

--- test some of the searching capabilities

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
CREATE INDEX brin_2d on test using brin (the_geom) WITH (pages_per_range = 10);

set enable_indexscan = off;
set enable_bitmapscan = off;
set enable_seqscan = on;

SELECT 'scan_seq', qnodes('SELECT * FROM test WHERE the_geom && ST_MakePoint(0,0)');
 SELECT num, ST_astext(the_geom) FROM test WHERE the_geom && 'BOX(125 125,126 126)'::box2d order by num;

SELECT 'scan_seq', qnodes('SELECT * FROM test WHERE ST_MakePoint(0,0) ~ the_geom');
 SELECT num, ST_astext(the_geom) FROM test WHERE 'BOX(125 125,126 126)'::box2d ~ the_geom order by num;

SELECT 'scan_seq', qnodes('SELECT * FROM test WHERE the_geom @ ST_MakePoint(0,0)');
 SELECT num, ST_astext(the_geom) FROM test WHERE the_geom @ 'BOX(125 125,126 126)'::box2d order by num;

SELECT 'scan_seq', qnodes('SELECT * FROM test WHERE the_geom IS NULL');
 SELECT COUNT(num) FROM test WHERE the_geom IS NULL;

set enable_indexscan = off;
set enable_bitmapscan = on;
set enable_seqscan = off;

SELECT 'scan_idx', qnodes('SELECT * FROM test WHERE the_geom && ST_MakePoint(0,0)');
 SELECT num, ST_astext(the_geom) FROM test WHERE the_geom && 'BOX(125 125,126 126)'::box2d order by num;

SELECT 'scan_idx', qnodes('SELECT * FROM test WHERE ST_MakePoint(0,0) ~ the_geom');
 SELECT num, ST_astext(the_geom) FROM test WHERE 'BOX(125 125,126 126)'::box2d ~ the_geom order by num;

SELECT 'scan_idx', qnodes('SELECT * FROM test WHERE the_geom @ ST_MakePoint(0,0)');
 SELECT num, ST_astext(the_geom) FROM test WHERE the_geom @ 'BOX(125 125,126 126)'::box2d order by num;

SELECT 'scan_idx', qnodes('SELECT * FROM test WHERE the_geom IS NULL');
 SELECT COUNT(num) FROM test WHERE the_geom IS NULL;

DROP INDEX brin_2d;

-- 3D
CREATE INDEX brin_3d on test using brin (the_geom brin_geometry_inclusion_ops_3d) WITH (pages_per_range = 10);

set enable_indexscan = off;
set enable_bitmapscan = off;
set enable_seqscan = on;

SELECT 'scan_seq', qnodes('SELECT * FROM test WHERE the_geom &&& ST_MakePoint(0,0)');
 SELECT num, ST_astext(the_geom) FROM test WHERE the_geom &&& 'BOX3D(125 125 125,126 126 126)'::box3d order by num;

SELECT 'scan_seq', qnodes('SELECT * FROM test WHERE the_geom IS NULL');
 SELECT COUNT(num) FROM test WHERE the_geom IS NULL;

set enable_indexscan = off;
set enable_bitmapscan = on;
set enable_seqscan = off;

SELECT 'scan_idx', qnodes('SELECT * FROM test WHERE the_geom &&& ST_MakePoint(0,0)');
 SELECT num, ST_astext(the_geom) FROM test WHERE the_geom &&& 'BOX3D(125 125 125,126 126 126)'::box3d order by num;

SELECT 'scan_idx', qnodes('select COUNT(num) FROM test WHERE the_geom IS NULL');
 SELECT COUNT(num) FROM test WHERE the_geom IS NULL;

DROP INDEX brin_3d;

-- 4D
CREATE INDEX brin_4d on test using brin (the_geom brin_geometry_inclusion_ops_4d) WITH (pages_per_range = 10);

set enable_indexscan = off;
set enable_bitmapscan = off;
set enable_seqscan = on;

SELECT 'scan_seq', qnodes('SELECT * FROM test WHERE the_geom &&& ST_MakePoint(0,0)');
 SELECT num, ST_astext(the_geom) FROM test WHERE the_geom &&& 'BOX3D(125 125 125,126 126 126)'::box3d order by num;

SELECT 'scan_seq', qnodes('SELECT * FROM test WHERE the_geom IS NULL');
 SELECT COUNT(num) FROM test WHERE the_geom IS NULL;

set enable_indexscan = off;
set enable_bitmapscan = on;
set enable_seqscan = off;

SELECT 'scan_idx', qnodes('SELECT * FROM test WHERE the_geom &&& ST_MakePoint(0,0)');
 SELECT num, ST_astext(the_geom) FROM test WHERE the_geom &&& 'BOX3D(125 125 125,126 126 126)'::box3d order by num;

SELECT 'scan_idx', qnodes('SELECT * FROM test WHERE the_geom IS NULL');
 SELECT COUNT(num) FROM test WHERE the_geom IS NULL;

DROP INDEX brin_4d;

-- cleanup
DROP TABLE test;
DROP FUNCTION qnodes(text);

set enable_indexscan = on;
set enable_bitmapscan = on;
set enable_seqscan = on;
