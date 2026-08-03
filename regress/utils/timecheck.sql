CREATE FUNCTION _timecheck_start() RETURNS void
AS $$
BEGIN
  UPDATE _time SET t = clock_timestamp();
END;
$$ LANGUAGE 'plpgsql' VOLATILE;

CREATE FUNCTION _timecheck_baseline(label text) RETURNS void
AS $$
BEGIN
  INSERT INTO _time_baseline
  SELECT label, clock_timestamp() - t
  FROM _time;

  UPDATE _time SET t = clock_timestamp();
END;
$$ LANGUAGE 'plpgsql' VOLATILE;

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

	-- The preceding query's expected ERROR proves it was cancelled. Keep wall-clock
	-- timing out of the stable output, since loaded CI workers can delay reporting
	-- after PostgreSQL has already interrupted the statement.
	ret := format(
		'%s interrupted',
		label
	);

  UPDATE _time SET t = clock_timestamp();

  RETURN ret;
END;
$$ LANGUAGE 'plpgsql' VOLATILE;

CREATE FUNCTION _timecheck(label text) RETURNS text
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
  SELECT now() - t lap, b.baseline, b.baseline * 0.9 tolerated
	FROM _time t
	JOIN _time_baseline b ON b.label = _timecheck.label
	INTO rec;

	RAISE DEBUG 'Uninterrupted baseline: %', rec.baseline;
	RAISE DEBUG 'Resulting tolerance: %', rec.tolerated;

  IF rec.lap < rec.tolerated THEN
		ret := format(
			'%s interrupted on time',
			label
		);
  ELSE
		ret := format(
			'%s interrupted late: %s (%s tolerated)',
			label, rec.lap, rec.tolerated
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

CREATE TEMPORARY TABLE _time_baseline (
  label text PRIMARY KEY,
  baseline interval NOT NULL
);
