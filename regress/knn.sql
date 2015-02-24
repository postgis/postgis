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

\i regress_lots_of_points.sql
CREATE INDEX on test using gist (the_geom);

-- Index-supported KNN query

SELECT '<-> idx', qnodes('select * from test order by the_geom <-> ST_MakePoint(0,0) LIMIT 1');
SELECT '<-> res1',num,
  (the_geom <-> 'LINESTRING(0 0,5 5)'::geometry)::numeric(10,2),
  ST_astext(the_geom) from test
  order by the_geom <-> 'LINESTRING(0 0,5 5)'::geometry LIMIT 1;

-- Full table extent: BOX(0.0439142361 0.0197799355,999.955261 999.993652)
SELECT '<#> idx', qnodes('select * from test order by the_geom <#> ST_MakePoint(0,0) LIMIT 1');
SELECT '<#> res1',num,
  (the_geom <#> 'LINESTRING(1000 0,1005 5)'::geometry)::numeric(10,2),
  ST_astext(the_geom) from test
  order by the_geom <#> 'LINESTRING(1000 0,1005 5)'::geometry LIMIT 1;

DROP FUNCTION qnodes(text);

DROP TABLE test;
