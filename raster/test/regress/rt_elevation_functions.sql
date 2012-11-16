SET client_min_messages TO warning;

/*
1 1 1 1 1 1 1 1 1
1 2 2 2 1 2 2 2 1
1 2 3 2 2 2 3 2 1
1 2 2 2 1 2 2 2 1
1 1 2 1 3 1 2 1 1
1 2 2 2 1 2 2 2 1
1 2 3 2 2 2 3 2 1
1 2 2 2 1 2 2 2 1
1 1 1 1 1 1 1 1 1
*/

DROP TABLE IF EXISTS raster_elevation;
CREATE TABLE raster_elevation (rid integer, rast raster);
DROP TABLE IF EXISTS raster_elevation_out;
CREATE TABLE raster_elevation_out (rid integer, functype text, rast raster);

INSERT INTO raster_elevation
	SELECT
		0 AS rid,
		ST_SetValues(
			ST_AddBand(ST_MakeEmptyRaster(9, 9, 0, 0, 1, -1, 0, 0, 0), 1, '32BF', 0, -9999),
			1, 1, 1, ARRAY[
				[1, 1, 1, 1, 1, 1, 1, 1, 1],
				[1, 2, 2, 2, 1, 2, 2, 2, 1],
				[1, 2, 3, 2, 2, 2, 3, 2, 1],
				[1, 2, 2, 2, 1, 2, 2, 2, 1],
				[1, 1, 2, 1, 3, 1, 2, 1, 1],
				[1, 2, 2, 2, 1, 2, 2, 2, 1],
				[1, 2, 3, 2, 2, 2, 3, 2, 1],
				[1, 2, 2, 2, 1, 2, 2, 2, 1],
				[1, 1, 1, 1, 1, 1, 1, 1, 1]
			]::double precision[]
		) AS rast
;

INSERT INTO raster_elevation
	SELECT
		1 AS rid,
		ST_SetValues(
			ST_AddBand(ST_MakeEmptyRaster(3, 3, 0, 0, 1, -1, 0, 0, 0), 1, '32BF', 0, -9999),
			1, 1, 1, ARRAY[
				[1, 1, 1],
				[1, 2, 2],
				[1, 2, 3]
			]::double precision[]
		) AS rast
	UNION ALL
	SELECT
		2 AS rid,
		ST_SetValues(
			ST_AddBand(ST_MakeEmptyRaster(3, 3, 0, -3, 1, -1, 0, 0, 0), 1, '32BF', 0, -9999),
			1, 1, 1, ARRAY[
				[1, 2, 2],
				[1, 1, 2],
				[1, 2, 2]
			]::double precision[]
		) AS rast
	UNION ALL
	SELECT
		3 AS rid,
		ST_SetValues(
			ST_AddBand(ST_MakeEmptyRaster(3, 3, 0, -6, 1, -1, 0, 0, 0), 1, '32BF', 0, -9999),
			1, 1, 1, ARRAY[
				[1, 2, 3],
				[1, 2, 2],
				[1, 1, 1]
			]::double precision[]
		) AS rast
	UNION ALL
	SELECT
		4 AS rid,
		ST_SetValues(
			ST_AddBand(ST_MakeEmptyRaster(3, 3, 3, 0, 1, -1, 0, 0, 0), 1, '32BF', 0, -9999),
			1, 1, 1, ARRAY[
				[1, 1, 1],
				[2, 1, 2],
				[2, 2, 2]
			]::double precision[]
		) AS rast
	UNION ALL
	SELECT
		5 AS rid,
		ST_SetValues(
			ST_AddBand(ST_MakeEmptyRaster(3, 3, 3, -3, 1, -1, 0, 0, 0), 1, '32BF', 0, -9999),
			1, 1, 1, ARRAY[
				[2, 1, 2],
				[1, 3, 1],
				[2, 1, 2]
			]::double precision[]
		) AS rast
	UNION ALL
	SELECT
		6 AS rid,
		ST_SetValues(
			ST_AddBand(ST_MakeEmptyRaster(3, 3, 3, -6, 1, -1, 0, 0, 0), 1, '32BF', 0, -9999),
			1, 1, 1, ARRAY[
				[2, 2, 2],
				[2, 1, 2],
				[1, 1, 1]
			]::double precision[]
		) AS rast
	UNION ALL
	SELECT
		7 AS rid,
		ST_SetValues(
			ST_AddBand(ST_MakeEmptyRaster(3, 3, 6, 0, 1, -1, 0, 0, 0), 1, '32BF', 0, -9999),
			1, 1, 1, ARRAY[
				[1, 1, 1],
				[2, 2, 1],
				[3, 2, 1]
			]::double precision[]
		) AS rast
	UNION ALL
	SELECT
		8 AS rid,
		ST_SetValues(
			ST_AddBand(ST_MakeEmptyRaster(3, 3, 6, -3, 1, -1, 0, 0, 0), 1, '32BF', 0, -9999),
			1, 1, 1, ARRAY[
				[2, 2, 1],
				[2, 1, 1],
				[2, 2, 1]
			]::double precision[]
		) AS rast
	UNION ALL
	SELECT
		9 AS rid,
		ST_SetValues(
			ST_AddBand(ST_MakeEmptyRaster(3, 3, 6, -6, 1, -1, 0, 0, 0), 1, '32BF', 0, -9999),
			1, 1, 1, ARRAY[
				[3, 2, 1],
				[2, 2, 1],
				[1, 1, 1]
			]::double precision[]
		) AS rast
;

/* ST_Slope */
INSERT INTO raster_elevation_out
SELECT
	rid,
	'slope',
	ST_Slope(rast)
FROM raster_elevation
WHERE rid = 0
UNION ALL
SELECT
	rid,
	'aspect',
	ST_Aspect(rast)
FROM raster_elevation
WHERE rid = 0
UNION ALL
SELECT
	rid,
	'hillshade',
	ST_Hillshade(rast)
FROM raster_elevation
WHERE rid = 0
;

/* with coverage */
DO $$
BEGIN
-- this ONLY works for PostgreSQL version 9.1 or higher
IF array_to_string(regexp_matches(split_part(version(), ' ', 2), E'([0-9]+)\.([0-9]+)'), '')::int > 90 THEN

	INSERT INTO raster_elevation_out (
		SELECT
			t1.rid,
			'slope',
			ST_Slope(ST_Union(t2.rast), 1, t1.rast) AS rast
		FROM raster_elevation t1
		CROSS JOIN raster_elevation t2
		WHERE t1.rid != 0
			AND t2.rid != 0
			AND ST_Intersects(t1.rast, t2.rast)
		GROUP BY t1.rid, t1.rast
		ORDER BY t1.rid
	) UNION ALL (
		SELECT
			t1.rid,
			'aspect',
			ST_Aspect(ST_Union(t2.rast), 1, t1.rast) AS rast
		FROM raster_elevation t1
		CROSS JOIN raster_elevation t2
		WHERE t1.rid != 0
			AND t2.rid != 0
			AND ST_Intersects(t1.rast, t2.rast)
		GROUP BY t1.rid, t1.rast
		ORDER BY t1.rid
	) UNION ALL (
		SELECT
			t1.rid,
			'hillshade',
			ST_Hillshade(ST_Union(t2.rast), 1, t1.rast) AS rast
		FROM raster_elevation t1
		CROSS JOIN raster_elevation t2
		WHERE t1.rid != 0
			AND t2.rid != 0
			AND ST_Intersects(t1.rast, t2.rast)
		GROUP BY t1.rid, t1.rast
		ORDER BY t1.rid
	);

ELSE

	INSERT INTO raster_elevation_out (
		WITH foo AS (
			SELECT
				t1.rid,
				ST_Union(t2.rast) AS rast
			FROM raster_elevation t1
			JOIN raster_elevation t2
				ON ST_Intersects(t1.rast, t2.rast)
					AND t1.rid != 0
					AND t2.rid != 0
			GROUP BY t1.rid
		)
		SELECT
			t1.rid,
			'slope',
			ST_Slope(t2.rast, 1, t1.rast) AS rast
		FROM raster_elevation t1
		JOIN foo t2
			ON t1.rid = t2.rid
		ORDER BY t1.rid
	) UNION ALL (
		WITH foo AS (
			SELECT
				t1.rid,
				ST_Union(t2.rast) AS rast
			FROM raster_elevation t1
			JOIN raster_elevation t2
				ON ST_Intersects(t1.rast, t2.rast)
					AND t1.rid != 0
					AND t2.rid != 0
			GROUP BY t1.rid
		)
		SELECT
			t1.rid,
			'aspect',
			ST_Aspect(t2.rast, 1, t1.rast) AS rast
		FROM raster_elevation t1
		JOIN foo t2
			ON t1.rid = t2.rid
		ORDER BY t1.rid
	) UNION ALL (
		WITH foo AS (
			SELECT
				t1.rid,
				ST_Union(t2.rast) AS rast
			FROM raster_elevation t1
			JOIN raster_elevation t2
				ON ST_Intersects(t1.rast, t2.rast)
					AND t1.rid != 0
					AND t2.rid != 0
				GROUP BY t1.rid
		)
		SELECT
			t1.rid,
			'hillshade',
			ST_Hillshade(t2.rast, 1, t1.rast) AS rast
		FROM raster_elevation t1
		JOIN foo t2
			ON t1.rid = t2.rid
		ORDER BY t1.rid
	);

END IF;
END $$;

WITH foo AS (
	SELECT
		rid,
		functype,
		ST_PixelAsPoints(rast) AS papt
	FROM raster_elevation_out
)
SELECT
	rid,
	functype,
	(papt).x,
	(papt).y,
	round((papt).val::numeric, 6) AS val
FROM foo
ORDER BY 2, 1, 4, 3;

DROP TABLE IF EXISTS raster_elevation_out;
DROP TABLE IF EXISTS raster_elevation;
