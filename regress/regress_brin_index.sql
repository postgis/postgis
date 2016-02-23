--- build a larger database
\i regress_lots_of_points.sql

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
CREATE INDEX brin_3d on test using brin (the_geom using brin_geometry_inclusion_ops_3d);

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
CREATE INDEX brin_4d on test using brin (the_geom using brin_geometry_inclusion_ops_4d);

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

-- cleanup
DROP TABLE test;
DROP FUNCTION qnodes(text);

set enable_indexscan = on;
set enable_bitmapscan = on;
set enable_seqscan = on;
