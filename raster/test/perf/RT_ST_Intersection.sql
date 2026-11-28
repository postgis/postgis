\set ON_ERROR_STOP 1

\echo 'RT_ST_Intersection: raster/geometry performance harness'
\echo 'Creates a synthetic raster, then compares current vectorization with a clipped-raster prepass.'

\timing off
\pset format unaligned
\pset fieldsep '|'
\pset footer off

BEGIN;

CREATE EXTENSION IF NOT EXISTS postgis;
CREATE EXTENSION IF NOT EXISTS postgis_raster;

CREATE TEMP TABLE rt_intersection_perf_raster AS
WITH base AS (
	SELECT ST_AddBand(
		ST_MakeEmptyRaster(256, 256, 0, 256, 1, -1, 0, 0, 0),
		1,
		'8BUI',
		1,
		0
	) AS rast
)
SELECT ST_MapAlgebra(
	rast,
	1,
	'8BUI',
	'(([rast.x] + [rast.y]) % 8) + 1'
) AS rast
FROM base;

CREATE TEMP TABLE rt_intersection_perf_window (
	sort integer PRIMARY KEY,
	label text NOT NULL,
	geom geometry(Polygon, 0) NOT NULL
);

INSERT INTO rt_intersection_perf_window VALUES
	(1, 'small window', ST_MakeEnvelope(24, 24, 56, 56, 0)),
	(2, 'medium window', ST_MakeEnvelope(64, 64, 160, 160, 0)),
	(3, 'large window', ST_MakeEnvelope(18, 18, 238, 238, 0));

ANALYZE rt_intersection_perf_raster;
ANALYZE rt_intersection_perf_window;

CREATE TEMP TABLE rt_intersection_perf_variant (
	sort integer PRIMARY KEY,
	variant text NOT NULL UNIQUE,
	description text NOT NULL
);

INSERT INTO rt_intersection_perf_variant VALUES
	(1, 'baseline', 'ST_Intersection(raster, geometry)'),
	(2, 'clip', 'ST_Intersection(ST_Clip(raster, geometry), geometry), default touched=false'),
	(3, 'clip_touched', 'ST_Intersection(ST_Clip(raster, geometry, touched => true), geometry)');

CREATE TEMP TABLE rt_intersection_perf_result (
	phase text NOT NULL,
	run integer NOT NULL,
	variant_sort integer NOT NULL,
	variant text NOT NULL,
	window_sort integer NOT NULL,
	window_label text NOT NULL,
	parts bigint NOT NULL,
	value_sum numeric NOT NULL,
	area_sum numeric NOT NULL,
	elapsed_ms numeric NOT NULL
);

CREATE OR REPLACE FUNCTION pg_temp.rt_intersection_perf_measure(
	phase text,
	run integer,
	variant text,
	window_sort integer
)
RETURNS void AS $$
DECLARE
	start_time timestamptz;
	row_result record;
BEGIN
	start_time := clock_timestamp();

	IF variant = 'baseline' THEN
		SELECT
			v.sort AS variant_sort,
			w.sort AS window_sort,
			w.label AS window_label,
			count((gv).geom) AS parts,
			round(coalesce(sum((gv).val), 0)::numeric, 2) AS value_sum,
			round(coalesce(sum(ST_Area((gv).geom)), 0)::numeric, 2) AS area_sum
		INTO row_result
		FROM rt_intersection_perf_raster r
		CROSS JOIN rt_intersection_perf_window w
		CROSS JOIN rt_intersection_perf_variant v
		LEFT JOIN LATERAL ST_Intersection(r.rast, w.geom) AS gv ON TRUE
		WHERE w.sort = window_sort AND v.variant = 'baseline'
		GROUP BY v.sort, w.sort, w.label;
	ELSIF variant = 'clip' THEN
		SELECT
			v.sort AS variant_sort,
			w.sort AS window_sort,
			w.label AS window_label,
			count((gv).geom) AS parts,
			round(coalesce(sum((gv).val), 0)::numeric, 2) AS value_sum,
			round(coalesce(sum(ST_Area((gv).geom)), 0)::numeric, 2) AS area_sum
		INTO row_result
		FROM rt_intersection_perf_raster r
		CROSS JOIN rt_intersection_perf_window w
		CROSS JOIN rt_intersection_perf_variant v
		LEFT JOIN LATERAL ST_Intersection(ST_Clip(r.rast, w.geom, true), w.geom) AS gv ON TRUE
		WHERE w.sort = window_sort AND v.variant = 'clip'
		GROUP BY v.sort, w.sort, w.label;
	ELSIF variant = 'clip_touched' THEN
		SELECT
			v.sort AS variant_sort,
			w.sort AS window_sort,
			w.label AS window_label,
			count((gv).geom) AS parts,
			round(coalesce(sum((gv).val), 0)::numeric, 2) AS value_sum,
			round(coalesce(sum(ST_Area((gv).geom)), 0)::numeric, 2) AS area_sum
		INTO row_result
		FROM rt_intersection_perf_raster r
		CROSS JOIN rt_intersection_perf_window w
		CROSS JOIN rt_intersection_perf_variant v
		LEFT JOIN LATERAL ST_Intersection(ST_Clip(r.rast, w.geom, true, touched => true), w.geom) AS gv ON TRUE
		WHERE w.sort = window_sort AND v.variant = 'clip_touched'
		GROUP BY v.sort, w.sort, w.label;
	ELSE
		RAISE EXCEPTION 'Unknown ST_Intersection perf variant: %', variant;
	END IF;

	INSERT INTO rt_intersection_perf_result (
		phase,
		run,
		variant_sort,
		variant,
		window_sort,
		window_label,
		parts,
		value_sum,
		area_sum,
		elapsed_ms
	)
	VALUES (
		phase,
		run,
		row_result.variant_sort,
		variant,
		row_result.window_sort,
		row_result.window_label,
		row_result.parts,
		row_result.value_sum,
		row_result.area_sum,
		round((extract(epoch FROM clock_timestamp() - start_time) * 1000)::numeric, 3)
	);
END;
$$ LANGUAGE plpgsql;

\echo 'Raster setup'
SELECT
	ST_Width(rast) AS width,
	ST_Height(rast) AS height,
	ST_NumBands(rast) AS bands,
	count(*) AS polygonized_parts
FROM rt_intersection_perf_raster
CROSS JOIN LATERAL ST_DumpAsPolygons(rast, 1) AS gv
GROUP BY rast;

\echo 'Warm-up: run each variant/window once before measured passes'
SELECT pg_temp.rt_intersection_perf_measure('warmup', 0, v.variant, w.sort)
FROM rt_intersection_perf_variant v
CROSS JOIN rt_intersection_perf_window w
ORDER BY v.sort, w.sort;

\echo 'Measured passes: five runs with rotated variant order'
SELECT pg_temp.rt_intersection_perf_measure('measured', run, variant, window_sort)
FROM (
	SELECT
		run,
		v.variant,
		w.sort AS window_sort,
		((v.sort + run) % (SELECT count(*) FROM rt_intersection_perf_variant)) AS variant_order
	FROM generate_series(1, 5) AS run
	CROSS JOIN rt_intersection_perf_variant v
	CROSS JOIN rt_intersection_perf_window w
) AS ordered_runs
ORDER BY run, variant_order, window_sort;

\echo 'Warm-up timing rows'
SELECT
	phase,
	run,
	variant,
	window_label,
	parts,
	value_sum,
	area_sum,
	elapsed_ms
FROM rt_intersection_perf_result
WHERE phase = 'warmup'
ORDER BY variant_sort, window_sort;

\echo 'Measured timing rows'
SELECT
	phase,
	run,
	variant,
	window_label,
	parts,
	value_sum,
	area_sum,
	elapsed_ms
FROM rt_intersection_perf_result
WHERE phase = 'measured'
ORDER BY run, variant_sort, window_sort;

\echo 'Measured timing summary'
SELECT
	variant,
	window_label,
	count(*) AS runs,
	min(elapsed_ms) AS min_ms,
	round(percentile_cont(0.5) WITHIN GROUP (ORDER BY elapsed_ms)::numeric, 3) AS median_ms,
	max(elapsed_ms) AS max_ms,
	parts,
	value_sum,
	area_sum
FROM rt_intersection_perf_result
WHERE phase = 'measured'
GROUP BY variant_sort, variant, window_sort, window_label, parts, value_sum, area_sum
ORDER BY variant_sort, window_sort;

ROLLBACK;
