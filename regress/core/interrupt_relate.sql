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

-----------------------------
-- IM9 based predicates
-----------------------------

DO $$
BEGIN
  PERFORM _timecheck_start();
  PERFORM ST_Contains(g,g) FROM _inputs WHERE id = 1;
  PERFORM _timecheck_baseline('contains');
END;
$$;

BEGIN;
SET LOCAL statement_timeout TO 100;
select ST_Contains(g,g) from _inputs WHERE id = 1; -- 6+ seconds
ROLLBACK;
SELECT _timecheck('contains');

DO $$
BEGIN
  PERFORM _timecheck_start();
  PERFORM ST_Covers(g,g) FROM _inputs WHERE id = 1;
  PERFORM _timecheck_baseline('covers');
END;
$$;

BEGIN;
SET LOCAL statement_timeout TO 100;
select ST_Covers(g,g) from _inputs WHERE id = 1; -- 6+ seconds
ROLLBACK;
SELECT _timecheck('covers');

DO $$
BEGIN
  PERFORM _timecheck_start();
  PERFORM ST_CoveredBy(g,g) FROM _inputs WHERE id = 1;
  PERFORM _timecheck_baseline('coveredby');
END;
$$;

BEGIN;
SET LOCAL statement_timeout TO 100;
select ST_CoveredBy(g,g) from _inputs WHERE id = 1; -- 6+ seconds
ROLLBACK;
SELECT _timecheck('coveredby');

DO $$
BEGIN
  PERFORM _timecheck_start();
  PERFORM ST_Crosses(g,g) FROM _inputs WHERE id = 1;
  PERFORM _timecheck_baseline('crosses');
END;
$$;

BEGIN;
SET LOCAL statement_timeout TO 100;
select ST_Crosses(g,g) from _inputs WHERE id = 1; -- 6+ seconds
ROLLBACK;
SELECT _timecheck('crosses');

DO $$
BEGIN
  PERFORM _timecheck_start();
  PERFORM ST_Equals(g,st_reverse(g)) FROM _inputs WHERE id = 1;
  PERFORM _timecheck_baseline('equals');
END;
$$;

-- NOTE: we're reversing one of the operands to avoid the
--       short-circuit described in #3226
BEGIN;
SET LOCAL statement_timeout TO 100;
select ST_Equals(g,st_reverse(g)) from _inputs WHERE id = 1; -- 6+ seconds
ROLLBACK;
SELECT _timecheck('equals');

DO $$
BEGIN
  PERFORM _timecheck_start();
  PERFORM ST_Intersects(g,ST_Segmentize(g,1e-4)) FROM _inputs WHERE id = 1;
  PERFORM _timecheck_baseline('intersects');
END;
$$;

-- NOTE: intersects became very fast, so we segmentize
--       input to make it slower
BEGIN;
SET LOCAL statement_timeout TO 100;
select ST_Intersects(g,ST_Segmentize(g,1e-4)) from _inputs WHERE id = 1; -- 6+ seconds
ROLLBACK;
SELECT _timecheck('intersects');

DO $$
BEGIN
  PERFORM _timecheck_start();
  PERFORM ST_Overlaps(g,g) FROM _inputs WHERE id = 1;
  PERFORM _timecheck_baseline('overlaps');
END;
$$;

BEGIN;
SET LOCAL statement_timeout TO 100;
select ST_Overlaps(g,g) from _inputs WHERE id = 1; -- 6+ seconds
ROLLBACK;
SELECT _timecheck('overlaps');

DO $$
BEGIN
  PERFORM _timecheck_start();
  PERFORM ST_Relate(g,g) FROM _inputs WHERE id = 1;
  PERFORM _timecheck_baseline('relate');
END;
$$;

BEGIN;
SET LOCAL statement_timeout TO 100;
select ST_Relate(g,g) from _inputs WHERE id = 1; -- 6+ seconds
ROLLBACK;
SELECT _timecheck('relate');

DROP FUNCTION _timecheck(text);
DROP FUNCTION _timecheck(text, interval);
DROP FUNCTION _timecheck_baseline(text);
DROP FUNCTION _timecheck_start();
DROP TABLE _inputs;
