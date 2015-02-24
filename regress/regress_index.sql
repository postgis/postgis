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

-- Index-supported KNN query

SELECT '<-> idx', qnodes('select * from test order by the_geom <-> ST_MakePoint(0,0)');
SELECT '<-> res1',num,
  (the_geom <-> 'LINESTRING(0 0,5 5)'::geometry)::numeric(10,2),
  ST_astext(the_geom) from test
  order by the_geom <-> 'LINESTRING(0 0,5 5)'::geometry LIMIT 1;

-- Full table extent: BOX(0.0439142361 0.0197799355,999.955261 999.993652)
SELECT '<#> idx', qnodes('select * from test order by the_geom <#> ST_MakePoint(0,0)');
SELECT '<#> res1',num,
  (the_geom <#> 'LINESTRING(1000 0,1005 5)'::geometry)::numeric(10,2),
  ST_astext(the_geom) from test
  order by the_geom <#> 'LINESTRING(1000 0,1005 5)'::geometry LIMIT 1;

DROP TABLE test;

DROP FUNCTION qnodes(text);
