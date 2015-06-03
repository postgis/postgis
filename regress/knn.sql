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

-- Index-supported KNN query

CREATE INDEX test_gist_2d on test using gist (the_geom);

SELECT '<-> idx', qnodes('select * from test order by the_geom <-> ST_MakePoint(0,0) LIMIT 1');
SELECT '<-> res1',num,
  (the_geom <-> 'POINT(2.5 2.5)'::geometry)::numeric(10,2),
  ST_astext(the_geom) from test
  order by the_geom <-> 'POINT(2.5 2.5)'::geometry LIMIT 1;

-- Full table extent: BOX(0.0439142361 0.0197799355,999.955261 999.993652)
SELECT '<#> idx', qnodes('select * from test order by the_geom <#> ST_MakePoint(0,0) LIMIT 1');
SELECT '<#> res1',num,
  (the_geom <#> 'LINESTRING(1000 0,1005 5)'::geometry)::numeric(10,2),
  ST_astext(the_geom) from test
  order by the_geom <#> 'LINESTRING(1000 0,1005 5)'::geometry LIMIT 1;

-- Index-supported nd-KNN query

DROP INDEX test_gist_2d;

UPDATE test set the_geom = ST_MakePoint(
    ST_X(the_geom), ST_Y(the_geom),
    num, -num);

SELECT '<<->> seq', qnodes('select * from test order by the_geom <<->> ST_MakePoint(0,0)');

CREATE INDEX test_gist_nd on test using gist (the_geom gist_geometry_ops_nd);

ANALYZE test;

--  EXT       X                Y          Z        M
-- min    0.0439142361 |   0.0197799355|     1| -50000
-- max  999.955261     | 999.993652    | 50000|     -1
--SELECT min(st_x(the_geom)) as minx, min(st_y(the_geom)) as miny,
--       min(st_z(the_geom)) as minz, min(st_m(the_geom)) as minm,
--       max(st_x(the_geom)) as maxx, max(st_y(the_geom)) as maxy,
--       max(st_z(the_geom)) as maxz, max(st_m(the_geom)) as maxm
--FROM test;


SELECT '<<->> idx', qnodes('select * from test order by the_geom <<->> ST_MakePoint(0,0) LIMIT 1');
SELECT '<<->> res1',num,
  (the_geom <<->> 'LINESTRING(0 0,5 5)'::geometry)::numeric(10,2),
  ST_astext(the_geom) from test
  order by the_geom <<->> 'LINESTRING(0 0,5 5)'::geometry LIMIT 1;
SELECT '<<->> res2',num,
  (the_geom <<->> 'POINT(95 23 25024 -25025)'::geometry)::numeric(10,2),
  ST_astext(the_geom) from test
  order by the_geom <<->> 'POINT(95 23 25024 -25025)'::geometry LIMIT 1;
SELECT '<<->> res3',num,
  (the_geom <<->> 'POINT(631 729 25023 -25022)'::geometry)::numeric(10,2),
  ST_astext(the_geom) from test
  order by the_geom <<->> 'POINT(631 729 25023 -25022)'::geometry LIMIT 1;


-- Cleanup

DROP FUNCTION qnodes(text);

DROP TABLE test;

-- nd box centroid distance  <<->>

select '<<->> ndcd1', 'LINESTRING(0 0,0 10,10 10)'::geometry <<->>
                'LINESTRING(6 2,6 8)'::geometry; -- 1
select '<<->> ndcd2', 'LINESTRING(0 0,0 10,10 10)'::geometry <<->>
                'LINESTRING(11 0,19 10)'::geometry; -- 10
select '<<->> ndcd3', 'POINTM(0 0 0)'::geometry <<->>
                'POINTM(0 0 5)'::geometry; -- 5
select '<<->> ndcd4', 'POINTZ(0 0 15)'::geometry <<->>
                'POINTZ(0 0 10)'::geometry; -- 5
select '<<->> ndcd5', 'POINTZM(1 2 3 4)'::geometry <<->>
                'POINTZM(2 3 4 5)'::geometry; -- 2
select '<<->> ndcd6', 'POINTZM(9 9 3 4)'::geometry <<->>
                'POINT(9 8)'::geometry; -- 1, higher dimensions overlapping
