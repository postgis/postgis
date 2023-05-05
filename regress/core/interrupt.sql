-- liblwgeom interruption
set client_min_messages to NOTICE;

CREATE FUNCTION _timecheck(label text, tolerated interval) RETURNS text
AS $$
DECLARE
  ret TEXT;
  lap INTERVAL;
BEGIN
	-- We use now() here to get the time at the
	-- start of the transaction, which started when
	-- this function was called, so the earliest
	-- posssible time
  lap := now()-t FROM _time;

  IF lap <= tolerated THEN
		ret := format(
			'%s interrupted on time',
			label
		);
  ELSE
		ret := format(
			'%s interrupted late: %s (%s tolerated)',
			label, lap, tolerated
		);
  END IF;

  UPDATE _time SET t = clock_timestamp();

  RETURN ret;
END;
$$ LANGUAGE 'plpgsql' VOLATILE;

CREATE TEMPORARY TABLE _time AS SELECT now() t;

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
