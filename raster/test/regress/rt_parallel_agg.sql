CREATE OR REPLACE PROCEDURE p_force_parallel_mode(param_state text) LANGUAGE plpgsql AS
$$
BEGIN
	IF (_postgis_pgsql_version())::integer < 160 THEN
		IF param_state = 'on' THEN
			SET force_parallel_mode = on;
		ELSE
			SET force_parallel_mode = off;
		END IF;
	ELSE
		IF param_state = 'on' THEN
			SET debug_parallel_query = on;
		ELSE
			SET debug_parallel_query = off;
		END IF;
	END IF;
END;
$$;

CREATE OR REPLACE FUNCTION qnodes(q text) RETURNS text
LANGUAGE plpgsql AS
$$
DECLARE
	exp text;
	mat text[];
	ret text[];
BEGIN
	FOR exp IN EXECUTE 'EXPLAIN (COSTS OFF) ' || q LOOP
		mat := regexp_matches(exp, ' *(?:-> *)?(Finalize Aggregate|Gather|Partial Aggregate|Parallel Seq Scan)');
		IF mat IS NOT NULL THEN
			ret := array_append(ret, mat[1]);
		END IF;
	END LOOP;
	RETURN array_to_string(ret, ',');
END;
$$;

CREATE TABLE raster_parallel_agg_test AS
SELECT
	id,
	ST_SetValue(
		ST_SetValue(
			ST_AddBand(
				ST_MakeEmptyRaster(10, 10, 0, 10, 1, -1, 0, 0, 0),
				1, '64BF', id::double precision, 0
			),
			1, 1, 1, -10
		),
		1, 5, 5, id::double precision + 0.5
	) AS rast
FROM generate_series(1, 2000) AS id;

ALTER TABLE raster_parallel_agg_test SET (parallel_workers = 4);
ANALYZE raster_parallel_agg_test (id);

SELECT 'countagg_catalog',
	count(*) FILTER (WHERE a.aggcombinefn <> 0::oid),
	count(*)
FROM pg_proc p
JOIN pg_aggregate a ON a.aggfnoid = p.oid
WHERE p.proname = 'st_countagg';

SELECT 'summarystatsagg_catalog',
	count(*) FILTER (
		WHERE a.aggcombinefn <> 0::oid
			AND a.aggserialfn <> 0::oid
			AND a.aggdeserialfn <> 0::oid
	),
	count(*)
FROM pg_proc p
JOIN pg_aggregate a ON a.aggfnoid = p.oid
WHERE p.proname = 'st_summarystatsagg';

CALL p_force_parallel_mode('off');

CREATE TEMP TABLE raster_parallel_agg_serial AS
SELECT
	ST_CountAgg(rast, 1, TRUE, 1) AS count_full,
	ST_CountAgg(rast, 1, TRUE) AS count_default_sample,
	ST_CountAgg(rast, TRUE) AS count_default_band,
	ST_SummaryStatsAgg(rast, 1, TRUE, 1) AS stats_full,
	ST_SummaryStatsAgg(rast, TRUE, 1) AS stats_default_band,
	ST_SummaryStatsAgg(rast, 1, TRUE) AS stats_default_sample
FROM raster_parallel_agg_test;

CALL p_force_parallel_mode('on');
SET parallel_setup_cost = 0;
SET parallel_tuple_cost = 0;
SET min_parallel_table_scan_size = 0;
SET max_parallel_workers_per_gather = 4;

SELECT 'countagg_plan',
	qnodes('SELECT ST_CountAgg(rast, 1, TRUE, 1) FROM raster_parallel_agg_test');

SELECT 'summarystatsagg_plan',
	qnodes('SELECT ST_SummaryStatsAgg(rast, 1, TRUE, 1) FROM raster_parallel_agg_test');

WITH parallel_result AS (
	SELECT
		ST_CountAgg(rast, 1, TRUE, 1) AS count_full,
		ST_CountAgg(rast, 1, TRUE) AS count_default_sample,
		ST_CountAgg(rast, TRUE) AS count_default_band,
		ST_SummaryStatsAgg(rast, 1, TRUE, 1) AS stats_full,
		ST_SummaryStatsAgg(rast, TRUE, 1) AS stats_default_band,
		ST_SummaryStatsAgg(rast, 1, TRUE) AS stats_default_sample
	FROM raster_parallel_agg_test
)
SELECT 'parallel_results',
	p.count_full = s.count_full,
	p.count_default_sample = s.count_default_sample,
	p.count_default_band = s.count_default_band,
	(p.stats_full).count = (s.stats_full).count,
	round((p.stats_full).sum::numeric, 3) = round((s.stats_full).sum::numeric, 3),
	round((p.stats_full).mean::numeric, 3) = round((s.stats_full).mean::numeric, 3),
	round((p.stats_full).stddev::numeric, 3) = round((s.stats_full).stddev::numeric, 3),
	(p.stats_default_band).count = (s.stats_default_band).count,
	(p.stats_default_sample).count = (s.stats_default_sample).count
FROM parallel_result p, raster_parallel_agg_serial s;

RESET max_parallel_workers_per_gather;
RESET min_parallel_table_scan_size;
RESET parallel_tuple_cost;
RESET parallel_setup_cost;
CALL p_force_parallel_mode('off');

DROP TABLE raster_parallel_agg_serial;
DROP TABLE raster_parallel_agg_test;
DROP FUNCTION qnodes(text);
DROP PROCEDURE p_force_parallel_mode(text);
