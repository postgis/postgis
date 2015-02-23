--- build a larger database
\i regress_lots_of_points.sql

--- test some of the searching capabilities

CREATE FUNCTION qnodes(q text) RETURNS text
LANGUAGE 'plpgsql' AS
$$
DECLARE
  exp TEXT;
BEGIN
  EXECUTE 'EXPLAIN ' || q INTO exp;
  RETURN (regexp_matches(exp, ' *(.*Scan)'))[1];
END;
$$;

-- GiST index

CREATE INDEX quick_gist on test using gist (the_geom);

set enable_indexscan = off;
set enable_bitmapscan = off;
set enable_seqscan = on;

SELECT 'scan_idx', qnodes('select * from test where the_geom && ST_MakePoint(0,0)');
 select num,ST_astext(the_geom) from test where the_geom && 'BOX3D(125 125,135 135)'::box3d order by num;

set enable_indexscan = on;
set enable_bitmapscan = off;
set enable_seqscan = off;

SELECT 'scan_seq', qnodes('select * from test where the_geom && ST_MakePoint(0,0)');
 select num,ST_astext(the_geom) from test where the_geom && 'BOX3D(125 125,135 135)'::box3d order by num;

DROP TABLE test;

DROP FUNCTION qnodes(text);
