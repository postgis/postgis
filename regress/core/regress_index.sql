--- build a larger database
\i :regdir/core/regress_lots_of_points.sql

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

CREATE OR REPLACE FUNCTION qplan_has(q text, pattern text) RETURNS boolean
LANGUAGE 'plpgsql' AS
$$
DECLARE
  exp TEXT;
BEGIN
  FOR exp IN EXECUTE 'EXPLAIN ' || q
  LOOP
    IF exp ~ pattern THEN
      RETURN true;
    END IF;
  END LOOP;
  RETURN false;
END;
$$;

-- GiST index

CREATE INDEX quick_gist on test using gist (the_geom);

CREATE TABLE test_gist_idx_2d AS
SELECT i, ST_MakeEnvelope(i, i, i + 10, i + 10) AS geom
FROM generate_series(1, 1000) i;

CREATE INDEX test_gist_idx_2d_gist on test_gist_idx_2d using gist (geom);

set enable_indexscan = off;
set enable_bitmapscan = off;
set enable_seqscan = on;

SELECT 'scan_idx', qnodes('select * from test where the_geom && ST_MakePoint(0,0)');
 select num,ST_astext(the_geom) from test where the_geom && 'BOX3D(125 125,135 135)'::box3d order by num;
SELECT 'gist 2d @ seq', qnodes('select * from test_gist_idx_2d where geom @ ST_MakeEnvelope(120,120,140,140)'), count(*) FROM test_gist_idx_2d WHERE geom @ ST_MakeEnvelope(120,120,140,140);
SELECT 'gist 2d ~ seq', qnodes('select * from test_gist_idx_2d where geom ~ ST_MakePoint(125,125)'), count(*) FROM test_gist_idx_2d WHERE geom ~ ST_MakePoint(125,125);

set enable_indexscan = on;
set enable_bitmapscan = off;
set enable_seqscan = off;

SELECT 'scan_seq', qnodes('select * from test where the_geom && ST_MakePoint(0,0)');
 select num,ST_astext(the_geom) from test where the_geom && 'BOX3D(125 125,135 135)'::box3d order by num;
SELECT 'gist 2d @ idx', qnodes('select * from test_gist_idx_2d where geom @ ST_MakeEnvelope(120,120,140,140)'), count(*) FROM test_gist_idx_2d WHERE geom @ ST_MakeEnvelope(120,120,140,140);
SELECT 'gist 2d ~ idx', qnodes('select * from test_gist_idx_2d where geom ~ ST_MakePoint(125,125)'), count(*) FROM test_gist_idx_2d WHERE geom ~ ST_MakePoint(125,125);

CREATE FUNCTION estimate_error(qry text, tol int)
RETURNS text
LANGUAGE 'plpgsql' VOLATILE AS $$
DECLARE
  anl TEXT; -- analysis
  err INT; -- absolute difference between planned and actual rows
  est INT; -- estimated count
  act INT; -- actual count
  mat TEXT[];
BEGIN

  -- TODO: rewrite using json output ?
  EXECUTE 'EXPLAIN ANALYZE ' || qry INTO anl;

  SELECT regexp_matches(anl, E' rows=([0-9]*) .* rows=([0-9\.]*) ')
  INTO mat;

  est := mat[1];
  act := mat[2]::numeric::integer;

  err = abs(est-act);

  RETURN act || '+-' || tol || ':' || coalesce(
    nullif((err < tol)::text,'false'),
    'false:'||err::text
    );

END;
$$;

-- There are 50000 points in the table with full extent being
-- BOX(0.001693 0.000185,999.968899 999.997026)
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

SELECT '&&', id, estimate_error(
  'select num from test where the_geom && ' || box, tol )
  FROM sample_queries ORDER BY id;

-- Test selectivity estimation of functional indexes

CREATE INDEX expressional_gist on test using gist ( st_centroid(the_geom) );
ANALYZE test;

SELECT 'expr &&', id, estimate_error(
  'select num from test where st_centroid(the_geom) && ' || box, tol )
  FROM sample_queries ORDER BY id;

SELECT 'st_orderingequals_idx',
  qnodes('select * from test where ST_OrderingEquals(the_geom, ST_MakePoint(0,0))');

CREATE SEQUENCE st_orderingequals_volatile_calls;

CREATE OR REPLACE FUNCTION st_orderingequals_volatile_geom(g geometry)
RETURNS geometry
LANGUAGE 'plpgsql' VOLATILE AS $$
BEGIN
  PERFORM nextval('st_orderingequals_volatile_calls');
  RETURN g;
END;
$$;

SELECT 'st_orderingequals_volatile_count', count(*)
FROM (VALUES (ST_MakePoint(0,0))) AS q(g)
JOIN test t ON ST_OrderingEquals(st_orderingequals_volatile_geom(t.the_geom), q.g);

SELECT 'st_orderingequals_volatile', last_value FROM st_orderingequals_volatile_calls;

DROP FUNCTION st_orderingequals_volatile_geom(geometry);
DROP SEQUENCE st_orderingequals_volatile_calls;

CREATE SCHEMA st_orderingequals_search_path;
CREATE FUNCTION st_orderingequals_search_path.geometry_bbox_false(geometry, geometry)
RETURNS boolean
LANGUAGE 'sql' IMMUTABLE AS 'SELECT false';
CREATE OPERATOR st_orderingequals_search_path.~= (
  LEFTARG = geometry,
  RIGHTARG = geometry,
  PROCEDURE = st_orderingequals_search_path.geometry_bbox_false,
  COMMUTATOR = '~='
);

SET search_path = st_orderingequals_search_path, public;
SELECT 'st_orderingequals_qualified_bbox', count(*)
FROM (VALUES (ST_MakePoint(0,0))) AS g1(g)
JOIN (VALUES (ST_MakePoint(0,0))) AS g2(g)
  ON ST_OrderingEquals(g1.g, g2.g);
RESET search_path;

DROP OPERATOR st_orderingequals_search_path.~= (geometry, geometry);
DROP FUNCTION st_orderingequals_search_path.geometry_bbox_false(geometry, geometry);
DROP SCHEMA st_orderingequals_search_path;

WITH geoms AS (SELECT ST_MakePoint(0, 'NaN'::float8) AS g)
SELECT 'st_orderingequals_nan_bbox', ST_OrderingEquals(g, g), g = g, g ~= g
FROM geoms;

SELECT 'st_orderingequals_nan_join', count(*)
FROM (VALUES (ST_MakePoint(0, 'NaN'::float8))) AS g1(g)
JOIN (VALUES (ST_MakePoint(0, 'NaN'::float8))) AS g2(g)
  ON ST_OrderingEquals(g1.g, g2.g);

-- ST_OrderingEquals is exact geometry equality, so the planner should see
-- the hash/merge-joinable geometry operator instead of only a function filter.
set enable_nestloop = off;
SELECT 'st_orderingequals_join',
  qplan_has(
    'SELECT t1.num FROM test t1, test t2 WHERE t1.num > t2.num AND ST_OrderingEquals(t1.the_geom, t2.the_geom)',
    '(Hash|Merge) Join'
  ),
  qplan_has(
    'SELECT t1.num FROM test t1, test t2 WHERE t1.num > t2.num AND ST_OrderingEquals(t1.the_geom, t2.the_geom)',
    'Nested Loop'
  );
set enable_nestloop = on;

DROP TABLE test;
DROP TABLE test_gist_idx_2d;
DROP TABLE sample_queries;

DROP FUNCTION estimate_error(text, int);

DROP FUNCTION qplan_has(text, text);
DROP FUNCTION qnodes(text);

set enable_indexscan = on;
set enable_bitmapscan = on;
set enable_seqscan = on;

-- _ST_SortableHash is a work around Postgres parallel sort requiring recalculation of abbreviated keys.
select '_st_sortablehash', _ST_SortableHash('POINT(0 0)'), _ST_SortableHash('SRID=4326;POINT(0 0)'), _ST_SortableHash('SRID=3857;POINT(0 0)');
