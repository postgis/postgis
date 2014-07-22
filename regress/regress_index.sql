--- build a larger database
\i regress_lots_of_points.sql

--- test some of the searching capabilities

-- GiST index

CREATE INDEX quick_gist on test using gist (the_geom);

 select num,ST_astext(the_geom) from test where the_geom && 'BOX3D(125 125,135 135)'::box3d order by num;

set enable_seqscan = off;

 select num,ST_astext(the_geom) from test where the_geom && 'BOX3D(125 125,135 135)'::box3d  order by num;

CREATE FUNCTION estimate_error(qry text, tol int)
RETURNS text
LANGUAGE 'plpgsql' VOLATILE AS $$
DECLARE
  anl XML; -- analisys
  err INT; -- absolute difference between planned and actual rows
  est INT; -- estimated count
  act INT; -- actual count
BEGIN
  EXECUTE 'EXPLAIN (ANALYZE, FORMAT XML) ' || qry INTO STRICT anl;

  SELECT (xpath('//x:Plan-Rows/text()', anl,
         ARRAY[ARRAY['x', 'http://www.postgresql.org/2009/explain']]))[1]
         ::text::int
  INTO est;

  SELECT (xpath('//x:Actual-Rows/text()', anl,
         ARRAY[ARRAY['x', 'http://www.postgresql.org/2009/explain']]))[1]
         ::text::int
  INTO act;

  err = abs(est-act);

  RETURN act || '+=' || tol || ':' || coalesce(
    nullif((err < tol)::text,'false'),
    'false:'||err::text
    );

END;
$$;

-- There are 50000 points in the table with full extent being
-- BOX(0.0439142361 0.0197799355,999.955261 999.993652)
CREATE TABLE sample_queries AS
SELECT 1 as id, 5 as tol, 'ST_MakeEnvelope(125,125,135,135)' as box
 UNION ALL
SELECT 2, 60, 'ST_MakeEnvelope(0,0,135,135)'
 UNION ALL
SELECT 3, 500, 'ST_MakeEnvelope(0,0,500,500)'
 UNION ALL
SELECT 4, 600, 'ST_MakeEnvelope(0,0,1000,1000)'
;

-- We raise the statistics target to the limit 
ALTER TABLE test ALTER COLUMN the_geom SET STATISTICS 10000;

ANALYZE test;

SELECT estimate_error(
  'select num from test where the_geom && ' || box, tol )
  FROM sample_queries ORDER BY id;

-- Test selectivity estimation of functional indexes

CREATE INDEX expressional_gist on test using gist ( st_centroid(the_geom) );
ANALYZE test;

SELECT 'expr', estimate_error(
  'select num from test where st_centroid(the_geom) && ' || box, tol )
  FROM sample_queries ORDER BY id;

DROP TABLE test;
DROP TABLE sample_queries;

DROP FUNCTION estimate_error(text, int);
