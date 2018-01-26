SET client_min_messages = NOTICE;
DROP TABLE IF EXISTS raster_grayscale_out;
CREATE TABLE raster_grayscale_out (
	testid integer,
	rid integer,
	rast raster
);
DROP TABLE IF EXISTS raster_grayscale_in;
CREATE TABLE raster_grayscale_in (
	rid integer,
	rast raster
);

INSERT INTO raster_grayscale_in
SELECT
	1 AS rid,
	ST_SetValues(
		ST_AddBand(
			ST_MakeEmptyRaster(2, 2, 0, 0, 1, -1, 0, 0, 0),
			1, '8BUI', 0, NULL
		),
		1, 1, 1, ARRAY[
			[ 0, 128],
			[ 254, 255]
		]::double precision[]
	) AS rast
UNION ALL
SELECT
	2 AS rid,
	ST_SetValues(
		ST_AddBand(
			ST_MakeEmptyRaster(2, 2, 0, 0, 1, -1, 0, 0, 0),
			1, '8BSI', 0, NULL
		),
		1, 1, 1, ARRAY[
			[ -128, 0],
			[ 126, 127]
		]::double precision[]
	) AS rast
UNION ALL
SELECT
	3 AS rid,
	ST_SetValues(
		ST_AddBand(
			ST_MakeEmptyRaster(2, 2, 0, 0, 1, -1, 0, 0, 0),
			1, '16BUI', 0, 0
		),
		1, 1, 1, ARRAY[
			[ 0, 32768],
			[ 65534, 65535]
		]::double precision[]
	) AS rast
UNION ALL
SELECT
	4 AS rid,
	ST_SetValues(
		ST_AddBand(
			ST_MakeEmptyRaster(2, 2, 0, 0, 1, -1, 0, 0, 0),
			1, '16BSI', 0, -32768
		),
		1, 1, 1, ARRAY[
			[ -32768, -32767],
			[ 32766, 32767]
		]::double precision[]
	) AS rast
UNION ALL
SELECT
	5 AS rid,
	ST_SetValues(
		ST_AddBand(
			ST_MakeEmptyRaster(2, 3, 0, 0, 1, -1, 0, 0, 0),
			1, '16BSI', 0, NULL
		),
		1, 1, 1, ARRAY[
			[ -32768, -32767],
			[ 0, 0],
			[ 32766, 32767]
		]::double precision[]
	) AS rast
;

INSERT INTO raster_grayscale_out
SELECT
	1,
	rid,
	ST_Grayscale(
		rast,
		1,
		1,
		1
	)
FROM raster_grayscale_in
UNION ALL
SELECT
	2,
	rid,
	ST_Grayscale(
		ARRAY[
			ROW(rast, 1)::rastbandarg,
			ROW(rast, 1)::rastbandarg,
			ROW(rast, 1)::rastbandarg
		]::rastbandarg[]
	)
FROM raster_grayscale_in
UNION ALL
SELECT
	3,
	rid,
	ST_Grayscale(
		ARRAY[
			ROW(rast, 1)::rastbandarg,
			ROW(rast, 1)::rastbandarg,
			ROW(rast, 1)::rastbandarg,
			ROW(rast, 1)::rastbandarg
		]::rastbandarg[]
	)
FROM raster_grayscale_in
ORDER BY rid
;

SELECT
	testid,
	rid,
	(ST_DumpValues(rast)).*
FROM raster_grayscale_out
ORDER BY 1, 2, nband;

-- error because of insufficient bands
BEGIN;
SELECT
	ST_Grayscale(
		ARRAY[
			ROW(rast, 1)::rastbandarg
		]::rastbandarg[]
	)
FROM raster_grayscale_in
ORDER BY rid
LIMIT 1;
ROLLBACK;

-- error because of no band at index
BEGIN;
SELECT
	ST_Grayscale(rast)
FROM raster_grayscale_in
ORDER BY rid
LIMIT 1;
ROLLBACK;

-- error because of no band at index
BEGIN;
SELECT
	ST_Grayscale(
		ARRAY[
			ROW(rast, 1)::rastbandarg,
			ROW(rast, 2)::rastbandarg,
			ROW(rast, 1)::rastbandarg
		]::rastbandarg[]
	)
FROM raster_grayscale_in
ORDER BY rid
LIMIT 1;
ROLLBACK;

DROP TABLE IF EXISTS raster_grayscale_in;
DROP TABLE IF EXISTS raster_grayscale_out;
