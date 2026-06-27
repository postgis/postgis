\set ON_ERROR_STOP 1

\echo 'RT_ST_Intersection: raster/geometry performance harness'
\echo 'Creates a synthetic raster, then compares current vectorization with a clipped-raster prepass.'

\timing off

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

\echo 'Raster setup'
SELECT
	ST_Width(rast) AS width,
	ST_Height(rast) AS height,
	ST_NumBands(rast) AS bands,
	count(*) AS polygonized_parts
FROM rt_intersection_perf_raster
CROSS JOIN LATERAL ST_DumpAsPolygons(rast, 1) AS gv
GROUP BY rast;

\echo 'Warm-up: run each variant once before collecting timed output'
SELECT count((gv).geom) AS baseline_parts
FROM rt_intersection_perf_raster r
CROSS JOIN rt_intersection_perf_window w
LEFT JOIN LATERAL ST_Intersection(r.rast, w.geom) AS gv ON TRUE;

SELECT count((gv).geom) AS clipped_parts
FROM rt_intersection_perf_raster r
CROSS JOIN rt_intersection_perf_window w
LEFT JOIN LATERAL ST_Intersection(ST_Clip(r.rast, w.geom, true), w.geom) AS gv ON TRUE;

SELECT count((gv).geom) AS clipped_touched_parts
FROM rt_intersection_perf_raster r
CROSS JOIN rt_intersection_perf_window w
LEFT JOIN LATERAL ST_Intersection(ST_Clip(r.rast, w.geom, true, touched => true), w.geom) AS gv ON TRUE;

\timing on

\echo 'Baseline: ST_Intersection(raster, geometry)'
SELECT
	w.label,
	count((gv).geom) AS parts,
	round(coalesce(sum((gv).val), 0)::numeric, 2) AS value_sum,
	round(coalesce(sum(ST_Area((gv).geom)), 0)::numeric, 2) AS area_sum
FROM rt_intersection_perf_raster r
CROSS JOIN rt_intersection_perf_window w
LEFT JOIN LATERAL ST_Intersection(r.rast, w.geom) AS gv ON TRUE
GROUP BY w.sort, w.label
ORDER BY w.sort;

\echo 'Candidate: ST_Intersection(ST_Clip(raster, geometry), geometry), default touched=false'
SELECT
	w.label,
	count((gv).geom) AS parts,
	round(coalesce(sum((gv).val), 0)::numeric, 2) AS value_sum,
	round(coalesce(sum(ST_Area((gv).geom)), 0)::numeric, 2) AS area_sum
FROM rt_intersection_perf_raster r
CROSS JOIN rt_intersection_perf_window w
LEFT JOIN LATERAL ST_Intersection(ST_Clip(r.rast, w.geom, true), w.geom) AS gv ON TRUE
GROUP BY w.sort, w.label
ORDER BY w.sort;

\echo 'Candidate: ST_Intersection(ST_Clip(raster, geometry, touched => true), geometry)'
SELECT
	w.label,
	count((gv).geom) AS parts,
	round(coalesce(sum((gv).val), 0)::numeric, 2) AS value_sum,
	round(coalesce(sum(ST_Area((gv).geom)), 0)::numeric, 2) AS area_sum
FROM rt_intersection_perf_raster r
CROSS JOIN rt_intersection_perf_window w
LEFT JOIN LATERAL ST_Intersection(ST_Clip(r.rast, w.geom, true, touched => true), w.geom) AS gv ON TRUE
GROUP BY w.sort, w.label
ORDER BY w.sort;

ROLLBACK;
