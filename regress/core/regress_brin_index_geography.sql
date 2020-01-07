--- build a larger database
\i :regdir/core/regress_lots_of_geographies.sql

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
CREATE INDEX brin_geog on test using brin (the_geog) WITH (pages_per_range = 10);

set enable_indexscan = off;
set enable_bitmapscan = off;
set enable_seqscan = on;

SELECT 'scan_seq', qnodes('select * from test where the_geog && ST_GeographyFromText(''SRID=4326;POLYGON((43. 42.,43. 43.,42. 43.,42. 42.,43. 42.))'')');
 select num,ST_astext(the_geog) from test where the_geog && ST_GeographyFromText('SRID=4326;POLYGON((43. 42.,43. 43.,42. 43.,42. 42.,43. 42.))') order by num;

SELECT 'scan_seq', qnodes('SELECT * FROM test WHERE the_geog IS NULL');
 SELECT COUNT(num) FROM test WHERE the_geog IS NULL;

set enable_indexscan = off;
set enable_bitmapscan = on;
set enable_seqscan = off;

SELECT 'scan_idx', qnodes('select * from test where the_geog && ST_GeographyFromText(''SRID=4326;POLYGON((43. 42.,43. 43.,42. 43.,42. 42.,43. 42.))'')');
 select num,ST_astext(the_geog) from test where the_geog && ST_GeographyFromText('SRID=4326;POLYGON((43. 42.,43. 43.,42. 43.,42. 42.,43. 42.))') order by num;

SELECT 'scan_idx', qnodes('SELECT * FROM test WHERE the_geog IS NULL');
 SELECT COUNT(num) FROM test WHERE the_geog IS NULL;

SELECT '#4608-1', count(*) FROM test where ST_Covers(ST_GeogFromText('POLYGON((0 0, 45 0, 45 45, 0 45, 0 0))'), the_geog);
SELECT '#4608-2', count(*) FROM test where ST_CoveredBy(the_geog, ST_GeogFromText('POLYGON((0 0, 45 0, 45 45, 0 45, 0 0))'));

DROP INDEX brin_geog;

-- cleanup
DROP TABLE test;
DROP FUNCTION qnodes(text);

set enable_indexscan = on;
set enable_bitmapscan = on;
set enable_seqscan = on;
