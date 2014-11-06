CREATE TEMPORARY TABLE _time AS SELECT now() t;

SET statement_timeout TO 100;

select ST_Buffer(g,100) from (
  select (st_dumppoints(st_buffer(st_makepoint(0,0),10000,100000))).geom g
) foo;

SELECT 'buffer_interrupted_on_time', now()-t < '150ms'::interval
FROM _time; UPDATE _time SET t = now();

SELECT
ST_Segmentize(ST_MakeLine(ST_MakePoint(4,39), ST_MakePoint(1,41)),
  1e-100);

SELECT 'segmentize_interrupted_on_time', now()-t < '150ms'::interval
FROM _time; UPDATE _time SET t = now();

SET statement_timeout TO 0;

-- Not affected by timeout
SELECT '1',ST_AsText(ST_Segmentize('LINESTRING(0 0,4 0)'::geometry, 2));
