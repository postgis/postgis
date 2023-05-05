set client_min_messages to WARNING;

CREATE FUNCTION _timecheck(label text, tolerated interval) RETURNS text
AS $$
DECLARE
  ret TEXT;
  lap INTERVAL;
BEGIN
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

CREATE TEMP TABLE _inputs AS
SELECT 1::int as id, ST_Collect(g) g FROM (
 SELECT ST_MakeLine(
   ST_Point(cos(radians(x)),sin(radians(270-x))),
   ST_Point(sin(radians(x)),cos(radians(60-x)))
   ) g
 FROM generate_series(1,720) x
 ) foo
;

CREATE TEMPORARY TABLE _time AS SELECT now() t;

-----------------------------
-- IM9 based predicates
-----------------------------

SET statement_timeout TO 100;

select ST_Contains(g,g) from _inputs WHERE id = 1; -- 6+ seconds
SELECT _timecheck('contains', '200ms');

select ST_Covers(g,g) from _inputs WHERE id = 1; -- 6+ seconds
SELECT _timecheck('covers', '200ms');

select ST_CoveredBy(g,g) from _inputs WHERE id = 1; -- 6+ seconds
SELECT _timecheck('coveredby', '200ms');

select ST_Crosses(g,g) from _inputs WHERE id = 1; -- 6+ seconds
SELECT _timecheck('crosses', '200ms');

-- NOTE: we're reversing one of the operands to avoid the
--       short-circuit described in #3226
select ST_Equals(g,st_reverse(g)) from _inputs WHERE id = 1; -- 6+ seconds
SELECT _timecheck('equals', '200ms');

-- NOTE: intersects became very fast, so we segmentize
--       input to make it slower
select ST_Intersects(g,ST_Segmentize(g,1e-4)) from _inputs WHERE id = 1; -- 6+ seconds
SELECT _timecheck('intersects', '200ms');

select ST_Overlaps(g,g) from _inputs WHERE id = 1; -- 6+ seconds
SELECT _timecheck('overlaps', '200ms');

select ST_Relate(g,g) from _inputs WHERE id = 1; -- 6+ seconds
SELECT _timecheck('relate', '200ms');

DROP FUNCTION _timecheck(text, interval);
DROP TABLE _inputs;
