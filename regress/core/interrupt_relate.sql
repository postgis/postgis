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

BEGIN;
SET LOCAL statement_timeout TO 100;
select ST_Contains(g,g) from _inputs WHERE id = 1; -- 6+ seconds
ROLLBACK;
SELECT _timecheck('contains');

BEGIN;
SET LOCAL statement_timeout TO 100;
select ST_Covers(g,g) from _inputs WHERE id = 1; -- 6+ seconds
ROLLBACK;
SELECT _timecheck('covers');

BEGIN;
SET LOCAL statement_timeout TO 100;
select ST_CoveredBy(g,g) from _inputs WHERE id = 1; -- 6+ seconds
ROLLBACK;
SELECT _timecheck('coveredby');

BEGIN;
SET LOCAL statement_timeout TO 100;
select ST_Crosses(g,g) from _inputs WHERE id = 1; -- 6+ seconds
ROLLBACK;
SELECT _timecheck('crosses');

BEGIN;
SET LOCAL statement_timeout TO 100;
-- NOTE: we're reversing one of the operands to avoid the
--       short-circuit described in #3226
select ST_Equals(g,st_reverse(g)) from _inputs WHERE id = 1; -- 6+ seconds
ROLLBACK;
SELECT _timecheck('equals');

BEGIN;
SET LOCAL statement_timeout TO 100;
-- NOTE: intersects became very fast, so we segmentize
--       input to make it slower
select ST_Intersects(g,ST_Segmentize(g,1e-4)) from _inputs WHERE id = 1; -- 6+ seconds
ROLLBACK;
SELECT _timecheck('intersects');

BEGIN;
SET LOCAL statement_timeout TO 100;
select ST_Overlaps(g,g) from _inputs WHERE id = 1; -- 6+ seconds
ROLLBACK;
SELECT _timecheck('overlaps');

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
