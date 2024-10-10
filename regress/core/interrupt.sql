-- liblwgeom interruption
set client_min_messages to WARNING;

\i :regdir/utils/timecheck.sql

-----------------
-- ST_Segmentize
-----------------

SET statement_timeout TO 100;
-- would run for many seconds if uninterruptible...
SELECT 'segmentize', ST_NPoints(ST_Segmentize(ST_MakeLine(ST_Point(4,39), ST_Point(1,41)), 1e-8));

SELECT _timecheck('segmentize', '300ms');
SET statement_timeout TO 0;
-- Not affected by old timeout
SELECT '1',ST_AsText(ST_Segmentize('LINESTRING(0 0,4 0)'::geometry, 2));

DROP FUNCTION _timecheck(text, interval);
