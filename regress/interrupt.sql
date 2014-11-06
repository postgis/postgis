CREATE TEMPORARY TABLE _time AS SELECT now() t;

SET statement_timeout TO 100;

select ST_Buffer(g,100) from (
  select (st_dumppoints(st_buffer(st_makepoint(0,0),10000,100000))).geom g
) foo;

-- it may take some more to interrupt st_buffer, see
-- https://travis-ci.org/postgis/postgis/builds/40211116#L2222-L2223
SELECT CASE WHEN now()-t < '200ms'::interval THEN
    'buffer interrupted on time'
  ELSE
    'buffer interrupted late: ' || ( now()-t )::text
  END FROM _time; UPDATE _time SET t = now();

SELECT
ST_Segmentize(ST_MakeLine(ST_MakePoint(4,39), ST_MakePoint(1,41)),
  1e-100);

SELECT CASE WHEN now()-t < '150ms'::interval THEN
    'segmentize interrupted on time'
  ELSE
    'segmentize interrupted late: ' || ( now()-t )::text
  END FROM _time; UPDATE _time SET t = now();

SET statement_timeout TO 0;

-- Not affected by timeout
SELECT '1',ST_AsText(ST_Segmentize('LINESTRING(0 0,4 0)'::geometry, 2));
