CREATE FUNCTION _timecheck(label text, tolerated interval) RETURNS text
AS $$
DECLARE
  ret TEXT;
  lap INTERVAL;
	rec RECORD;
BEGIN
	-- We use now() here to get the time at the
	-- start of the transaction, which started when
	-- this function was called, so the earliest
	-- possible time
  SELECT now()-t lap, sf slow_factor
	FROM _time INTO rec;

	RAISE DEBUG 'Requested tolerance: %', tolerated;
	RAISE DEBUG 'Slow factor: %', rec.slow_factor;

	tolerated := tolerated * rec.slow_factor;

	RAISE DEBUG 'Resulting tolerance: %', tolerated;

  IF rec.lap <= tolerated THEN
		ret := format(
			'%s interrupted on time',
			label
		);
  ELSE
		ret := format(
			'%s interrupted late: %s (%s tolerated)',
			label, rec.lap, tolerated
		);
  END IF;

  UPDATE _time SET t = clock_timestamp();

  RETURN ret;
END;
$$ LANGUAGE 'plpgsql' VOLATILE;

CREATE TEMPORARY TABLE _time AS
SELECT
	now() t,
	COALESCE(
    current_setting('test.executor_slow_factor', true),
    '1'
  )::float8 sf;

