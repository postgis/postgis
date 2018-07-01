\i regress_lots_of_box3d.sql

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

-- GiST index

CREATE INDEX quick_spgist on test using spgist (the_box);

set enable_indexscan = off;
set enable_bitmapscan = off;
set enable_seqscan = on;

SELECT 'scan', qnodes('select count(*) from test where the_box::box3d @> ''BOX3D(0 0 0, 0 0 0)''::box3d');
 select count(*) from test where the_box::box3d @> 'BOX3D(800 800 800,1200 1200 1200)'::box3d ;

set enable_indexscan = on;
set enable_bitmapscan = off;
set enable_seqscan = off;

SELECT 'scan', qnodes('select count(*) from test where the_box::box3d @> ''BOX3D(0 0 0, 0 0 0)''::box3d');
 select count(*) from test where the_box::box3d @> 'BOX3D(800 800 800, 1200 1200 1200)'::box3d ;
set enable_indexscan = off;
set enable_bitmapscan = off;
set enable_seqscan = on;

SELECT 'scan', qnodes('select count(*) from test where the_box::box3d <@ ''BOX3D(0 0 0, 0 0 0)''::box3d');
 select count(*) from test where the_box::box3d <@ 'BOX3D(800 800 800,1200 1200 1200)'::box3d ;

set enable_indexscan = on;
set enable_bitmapscan = off;
set enable_seqscan = off;

SELECT 'scan', qnodes('select count(*) from test where the_box::box3d <@ ''BOX3D(0 0 0, 0 0 0)''::box3d');
 select count(*) from test where the_box::box3d <@ 'BOX3D(800 800 800, 1200 1200 1200)'::box3d ;
set enable_indexscan = off;
set enable_bitmapscan = off;
set enable_seqscan = on;

SELECT 'scan', qnodes('select count(*) from test where the_box::box3d && ''BOX3D(0 0 0, 0 0 0)''::box3d');
 select count(*) from test where the_box::box3d && 'BOX3D(800 800 800,1200 1200 1200)'::box3d ;

set enable_indexscan = on;
set enable_bitmapscan = off;
set enable_seqscan = off;

SELECT 'scan', qnodes('select count(*) from test where the_box::box3d && ''BOX3D(0 0 0, 0 0 0)''::box3d');
 select count(*) from test where the_box::box3d && 'BOX3D(800 800 800, 1200 1200 1200)'::box3d ;
set enable_indexscan = off;
set enable_bitmapscan = off;
set enable_seqscan = on;

SELECT 'scan', qnodes('select count(*) from test where the_box::box3d ~= ''BOX3D(0 0 0, 0 0 0)''::box3d');
 select count(*) from test where the_box::box3d ~= 'BOX3D(800 800 800,1200 1200 1200)'::box3d ;

set enable_indexscan = on;
set enable_bitmapscan = off;
set enable_seqscan = off;

SELECT 'scan', qnodes('select count(*) from test where the_box::box3d ~= ''BOX3D(0 0 0, 0 0 0)''::box3d');
 select count(*) from test where the_box::box3d ~= 'BOX3D(800 800 800, 1200 1200 1200)'::box3d ;
set enable_indexscan = off;
set enable_bitmapscan = off;
set enable_seqscan = on;

SELECT 'scan', qnodes('select count(*) from test where the_box::box3d << ''BOX3D(0 0 0, 0 0 0)''::box3d');
 select count(*) from test where the_box::box3d << 'BOX3D(800 800 800,1200 1200 1200)'::box3d ;

set enable_indexscan = on;
set enable_bitmapscan = off;
set enable_seqscan = off;

SELECT 'scan', qnodes('select count(*) from test where the_box::box3d << ''BOX3D(0 0 0, 0 0 0)''::box3d');
 select count(*) from test where the_box::box3d << 'BOX3D(800 800 800, 1200 1200 1200)'::box3d ;
set enable_indexscan = off;
set enable_bitmapscan = off;
set enable_seqscan = on;

SELECT 'scan', qnodes('select count(*) from test where the_box::box3d &< ''BOX3D(0 0 0, 0 0 0)''::box3d');
 select count(*) from test where the_box::box3d &< 'BOX3D(800 800 800,1200 1200 1200)'::box3d ;

set enable_indexscan = on;
set enable_bitmapscan = off;
set enable_seqscan = off;

SELECT 'scan', qnodes('select count(*) from test where the_box::box3d &< ''BOX3D(0 0 0, 0 0 0)''::box3d');
 select count(*) from test where the_box::box3d &< 'BOX3D(800 800 800, 1200 1200 1200)'::box3d ;
set enable_indexscan = off;
set enable_bitmapscan = off;
set enable_seqscan = on;

SELECT 'scan', qnodes('select count(*) from test where the_box::box3d >> ''BOX3D(0 0 0, 0 0 0)''::box3d');
 select count(*) from test where the_box::box3d >> 'BOX3D(800 800 800,1200 1200 1200)'::box3d ;

set enable_indexscan = on;
set enable_bitmapscan = off;
set enable_seqscan = off;

SELECT 'scan', qnodes('select count(*) from test where the_box::box3d >> ''BOX3D(0 0 0, 0 0 0)''::box3d');
 select count(*) from test where the_box::box3d >> 'BOX3D(800 800 800, 1200 1200 1200)'::box3d ;
set enable_indexscan = off;
set enable_bitmapscan = off;
set enable_seqscan = on;

SELECT 'scan', qnodes('select count(*) from test where the_box::box3d &> ''BOX3D(0 0 0, 0 0 0)''::box3d');
 select count(*) from test where the_box::box3d &> 'BOX3D(800 800 800,1200 1200 1200)'::box3d ;

set enable_indexscan = on;
set enable_bitmapscan = off;
set enable_seqscan = off;

SELECT 'scan', qnodes('select count(*) from test where the_box::box3d &> ''BOX3D(0 0 0, 0 0 0)''::box3d');
 select count(*) from test where the_box::box3d &> 'BOX3D(800 800 800, 1200 1200 1200)'::box3d ;
set enable_indexscan = off;
set enable_bitmapscan = off;
set enable_seqscan = on;

SELECT 'scan', qnodes('select count(*) from test where the_box::box3d <<| ''BOX3D(0 0 0, 0 0 0)''::box3d');
 select count(*) from test where the_box::box3d <<| 'BOX3D(800 800 800,1200 1200 1200)'::box3d ;

set enable_indexscan = on;
set enable_bitmapscan = off;
set enable_seqscan = off;

SELECT 'scan', qnodes('select count(*) from test where the_box::box3d <<| ''BOX3D(0 0 0, 0 0 0)''::box3d');
 select count(*) from test where the_box::box3d <<| 'BOX3D(800 800 800, 1200 1200 1200)'::box3d ;
set enable_indexscan = off;
set enable_bitmapscan = off;
set enable_seqscan = on;

SELECT 'scan', qnodes('select count(*) from test where the_box::box3d &<| ''BOX3D(0 0 0, 0 0 0)''::box3d');
 select count(*) from test where the_box::box3d &<| 'BOX3D(800 800 800,1200 1200 1200)'::box3d ;

set enable_indexscan = on;
set enable_bitmapscan = off;
set enable_seqscan = off;

SELECT 'scan', qnodes('select count(*) from test where the_box::box3d &<| ''BOX3D(0 0 0, 0 0 0)''::box3d');
 select count(*) from test where the_box::box3d &<| 'BOX3D(800 800 800, 1200 1200 1200)'::box3d ;
set enable_indexscan = off;
set enable_bitmapscan = off;
set enable_seqscan = on;

SELECT 'scan', qnodes('select count(*) from test where the_box::box3d |>> ''BOX3D(0 0 0, 0 0 0)''::box3d');
 select count(*) from test where the_box::box3d |>> 'BOX3D(800 800 800,1200 1200 1200)'::box3d ;

set enable_indexscan = on;
set enable_bitmapscan = off;
set enable_seqscan = off;

SELECT 'scan', qnodes('select count(*) from test where the_box::box3d |>> ''BOX3D(0 0 0, 0 0 0)''::box3d');
 select count(*) from test where the_box::box3d |>> 'BOX3D(800 800 800, 1200 1200 1200)'::box3d ;
set enable_indexscan = off;
set enable_bitmapscan = off;
set enable_seqscan = on;

SELECT 'scan', qnodes('select count(*) from test where the_box::box3d |&> ''BOX3D(0 0 0, 0 0 0)''::box3d');
 select count(*) from test where the_box::box3d |&> 'BOX3D(800 800 800,1200 1200 1200)'::box3d ;

set enable_indexscan = on;
set enable_bitmapscan = off;
set enable_seqscan = off;

SELECT 'scan', qnodes('select count(*) from test where the_box::box3d |&> ''BOX3D(0 0 0, 0 0 0)''::box3d');
 select count(*) from test where the_box::box3d |&> 'BOX3D(800 800 800, 1200 1200 1200)'::box3d ;
set enable_indexscan = off;
set enable_bitmapscan = off;
set enable_seqscan = on;

SELECT 'scan', qnodes('select count(*) from test where the_box::box3d <</ ''BOX3D(0 0 0, 0 0 0)''::box3d');
 select count(*) from test where the_box::box3d <</ 'BOX3D(800 800 800,1200 1200 1200)'::box3d ;

set enable_indexscan = on;
set enable_bitmapscan = off;
set enable_seqscan = off;

SELECT 'scan', qnodes('select count(*) from test where the_box::box3d <</ ''BOX3D(0 0 0, 0 0 0)''::box3d');
 select count(*) from test where the_box::box3d <</ 'BOX3D(800 800 800, 1200 1200 1200)'::box3d ;
set enable_indexscan = off;
set enable_bitmapscan = off;
set enable_seqscan = on;

SELECT 'scan', qnodes('select count(*) from test where the_box::box3d &</ ''BOX3D(0 0 0, 0 0 0)''::box3d');
 select count(*) from test where the_box::box3d &</ 'BOX3D(800 800 800,1200 1200 1200)'::box3d ;

set enable_indexscan = on;
set enable_bitmapscan = off;
set enable_seqscan = off;

SELECT 'scan', qnodes('select count(*) from test where the_box::box3d &</ ''BOX3D(0 0 0, 0 0 0)''::box3d');
 select count(*) from test where the_box::box3d &</ 'BOX3D(800 800 800, 1200 1200 1200)'::box3d ;
set enable_indexscan = off;
set enable_bitmapscan = off;
set enable_seqscan = on;

SELECT 'scan', qnodes('select count(*) from test where the_box::box3d />> ''BOX3D(0 0 0, 0 0 0)''::box3d');
 select count(*) from test where the_box::box3d />> 'BOX3D(800 800 800,1200 1200 1200)'::box3d ;

set enable_indexscan = on;
set enable_bitmapscan = off;
set enable_seqscan = off;

SELECT 'scan', qnodes('select count(*) from test where the_box::box3d />> ''BOX3D(0 0 0, 0 0 0)''::box3d');
 select count(*) from test where the_box::box3d />> 'BOX3D(800 800 800, 1200 1200 1200)'::box3d ;
set enable_indexscan = off;
set enable_bitmapscan = off;
set enable_seqscan = on;

SELECT 'scan', qnodes('select count(*) from test where the_box::box3d /&> ''BOX3D(0 0 0, 0 0 0)''::box3d');
 select count(*) from test where the_box::box3d /&> 'BOX3D(800 800 800,1200 1200 1200)'::box3d ;

set enable_indexscan = on;
set enable_bitmapscan = off;
set enable_seqscan = off;

SELECT 'scan', qnodes('select count(*) from test where the_box::box3d /&> ''BOX3D(0 0 0, 0 0 0)''::box3d');
 select count(*) from test where the_box::box3d /&> 'BOX3D(800 800 800, 1200 1200 1200)'::box3d ;


-- cleanup
DROP INDEX quick_spgist;
DROP TABLE test;
DROP FUNCTION qnodes(text);

set enable_indexscan = on;
set enable_bitmapscan = on;
set enable_seqscan = on;
