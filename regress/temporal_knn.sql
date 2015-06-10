CREATE OR REPLACE FUNCTION qnodes(q text) RETURNS text
LANGUAGE 'plpgsql' AS
$$
DECLARE
  exp TEXT;
  mat TEXT[];
  ret TEXT[];
BEGIN
  --RAISE NOTICE 'Q: %', q;
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

-- create table
CREATE TABLE knn_cpa AS
WITH points AS (
  SELECT t,
         ST_MakePoint(x-t,x+t) p
  FROM generate_series(0,1000,5) t -- trajectories
      ,generate_series(-100,100,10) x
)
SELECT t, ST_AddMeasure(
  CASE WHEN t%2 = 0 THEN ST_Reverse(ST_MakeLine(p))
       ELSE ST_MakeLine(p) END,
  10, 20) tr
FROM points GROUP BY t;
--ALTER TABLE knn_cpa ADD PRIMARY KEY(t);

\set qt 'ST_AddMeasure(ST_MakeLine(ST_MakePointM(-260,380,0),ST_MakePointM(-360,540,0)),10,20)'

SELECT '|=| no idx', qnodes('select * from knn_cpa ORDER BY tr |=| ' || quote_literal(:qt ::text) || ' LIMIT 1');
CREATE TABLE knn_cpa_no_index AS
SELECT row_number() over () n, t, d FROM (
  SELECT t,
  ST_DistanceCPA(tr,:qt) d
  FROM knn_cpa ORDER BY tr |=| :qt LIMIT 5
) foo;

CREATE INDEX on knn_cpa USING gist (tr gist_geometry_ops_nd);
ANALYZE knn_cpa;
set enable_seqscan to off;

SELECT '|=| idx', qnodes('select * from knn_cpa ORDER BY tr |=| ' || quote_literal(:qt ::text) || ' LIMIT 1');
CREATE TABLE knn_cpa_index AS
SELECT row_number() over () n, t, d FROM (
  SELECT t, ST_DistanceCPA(tr,:qt) d
  FROM knn_cpa ORDER BY tr |=| :qt LIMIT 5
) foo;

--SELECT * FROM knn_cpa_no_index;
--SELECT * FROM knn_cpa_index;

SELECT a.n,
  CASE WHEN a.t = b.t THEN a.t||'' ELSE a.t || ' vs ' || b.t END closest,
  CASE WHEN a.d = b.d THEN 'dist:' || a.d::numeric(10,2) ELSE 'diff:' || (a.d - b.d) END dist
FROM knn_cpa_no_index a, knn_cpa_index b
WHERE a.n = b.n
ORDER BY a.n;

-- Cleanup

DROP FUNCTION qnodes(text);
DROP TABLE knn_cpa;
DROP TABLE knn_cpa_no_index;
DROP TABLE knn_cpa_index;
