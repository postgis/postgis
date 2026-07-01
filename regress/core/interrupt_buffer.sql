set client_min_messages to WARNING;

CREATE TEMP TABLE _inputs AS
SELECT 1::int as id, ST_Collect(g) g FROM (
 SELECT ST_MakeLine(
   ST_Point(cos(radians(x)),sin(radians(270-x))),
   ST_Point(sin(radians(x)),cos(radians(60-x)))
   ) g
 FROM generate_series(1,720) x
 ) foo
;

\i :regdir/utils/timecheck.sql

-----------------
-- ST_Buffer
-----------------

SET statement_timeout TO 100;

select ST_Buffer(g,100) from _inputs WHERE id = 1;
--( select (st_dumppoints(st_buffer(st_makepoint(0,0),10000,100000))).geom g) foo;
-- it may take some more to interrupt st_buffer, see
SELECT _timecheck('buffer', '350ms');

-- Not affected by old timeout
SELECT '1', ST_NPoints(ST_Buffer('POINT(4 0)'::geometry, 2, 1));

DROP FUNCTION _timecheck(text, interval);
