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

-- #2807: ST_Grayscale is implemented through ST_MapAlgebra. Source rasters 1,
-- 2, and 5 have no NODATA value, so zero-valued pixels in the dump above are
-- real pixel values. Source rasters 3 and 4 have explicit NODATA values, so
-- their matching output pixels remain NULL.
SELECT
	'#2807.grayscale',
	testid,
	rid,
	(ST_BandMetadata(rast, 1)).nodatavalue
FROM raster_grayscale_out
WHERE rid IN (1, 3, 5)
ORDER BY testid, rid;

-- The first RGB band controls generic n-raster MapAlgebra output metadata.
-- ST_Grayscale still needs to preserve NODATA from later RGB bands when the
-- first band has no NODATA value.
WITH mixed AS (
	SELECT ST_Grayscale(ARRAY[
		ROW(red.rast, 1)::rastbandarg,
		ROW(green.rast, 1)::rastbandarg,
		ROW(blue.rast, 1)::rastbandarg
	]::rastbandarg[]) AS rast
	FROM raster_grayscale_in red
	CROSS JOIN raster_grayscale_in green
	CROSS JOIN raster_grayscale_in blue
	WHERE red.rid = 1
	AND green.rid = 3
	AND blue.rid = 3
)
SELECT
	'#2807.grayscale.mixed',
	(ST_BandMetadata(rast, 1)).nodatavalue,
	(ST_DumpValues(rast)).valarray
FROM mixed;

WITH src AS (
	SELECT ST_SetValues(
		ST_AddBand(
			ST_MakeEmptyRaster(2, 1, 0, 0, 1, -1, 0, 0, 0),
			1, '8BUI', 0, 0
		),
		1, 1, 1, ARRAY[[0, 255]]::double precision[]
	) AS rast
), gray AS (
	SELECT ST_Grayscale(
		ARRAY[
			ROW(rast, 1)::rastbandarg,
			ROW(rast, 1)::rastbandarg,
			ROW(rast, 1)::rastbandarg
		]::rastbandarg[]
	) AS rast
	FROM src
)
SELECT
	'#2807.grayscale.8bui-nodata',
	(ST_BandMetadata(rast, 1)).nodatavalue,
	(ST_DumpValues(rast)).valarray
FROM gray;

WITH red AS (
	SELECT ST_SetValues(
		ST_AddBand(
			ST_MakeEmptyRaster(1, 1, 0, 0, 1, -1, 0, 0, 0),
			1, '8BUI', 255, NULL
		),
		1, 1, 1, ARRAY[[255]]::double precision[]
	) AS rast
), gb AS (
	SELECT ST_SetValues(
		ST_AddBand(
			ST_MakeEmptyRaster(1, 1, 1, 0, 1, -1, 0, 0, 0),
			1, '8BUI', 0, NULL
		),
		1, 1, 1, ARRAY[[0]]::double precision[]
	) AS rast
), gray AS (
	SELECT ST_Grayscale(
		ARRAY[
			ROW(red.rast, 1)::rastbandarg,
			ROW(gb.rast, 1)::rastbandarg,
			ROW(gb.rast, 1)::rastbandarg
		]::rastbandarg[],
		'UNION'
	) AS rast
	FROM red CROSS JOIN gb
)
SELECT
	'#2807.grayscale.union-gap',
	(ST_BandMetadata(rast, 1)).nodatavalue,
	(ST_DumpValues(rast)).valarray
FROM gray;

WITH src AS (
	SELECT ST_SetValues(
		ST_AddBand(
			ST_MakeEmptyRaster(1, 1, 0, 0, 1, -1, 0, 0, 0),
			1, '8BUI', 0, NULL
		),
		1, 1, 1, ARRAY[[255]]::double precision[]
	) AS rast
), gray AS (
	SELECT ST_Grayscale(
		ARRAY[
			ROW(rast, 1)::rastbandarg,
			ROW(rast, 1)::rastbandarg,
			ROW(rast, 1)::rastbandarg
		]::rastbandarg[],
		'UNION'
	) AS rast
	FROM src
)
SELECT
	'#2807.grayscale.union-coextensive-white',
	(ST_BandMetadata(rast, 1)).nodatavalue,
	(ST_DumpValues(rast)).valarray
FROM gray;

WITH red AS (
	SELECT ST_SetValues(
		ST_AddBand(
			ST_MakeEmptyRaster(1, 1, 0, 0, 1, -1, 0, 0, 0),
			1, '8BUI', 0, NULL
		),
		1, 1, 1, ARRAY[[255]]::double precision[]
	) AS rast
), gb AS (
	SELECT ST_SetValues(
		ST_AddBand(
			ST_MakeEmptyRaster(2, 1, 0, 0, 1, -1, 0, 0, 0),
			1, '8BUI', 0, NULL
		),
		1, 1, 1, ARRAY[[255, 0]]::double precision[]
	) AS rast
), gray AS (
	SELECT ST_Grayscale(
		ARRAY[
			ROW(red.rast, 1)::rastbandarg,
			ROW(gb.rast, 1)::rastbandarg,
			ROW(gb.rast, 1)::rastbandarg
		]::rastbandarg[],
		'FIRST'
	) AS rast
	FROM red CROSS JOIN gb
)
SELECT
	'#2807.grayscale.first-covered-white',
	(ST_BandMetadata(rast, 1)).nodatavalue,
	(ST_DumpValues(rast)).valarray
FROM gray;

WITH red AS (
	SELECT ST_SetValues(
		ST_AddBand(
			ST_MakeEmptyRaster(2, 1, 0, 0, 1, -1, 0, 0, 0),
			1, '8BUI', 0, NULL
		),
		1, 1, 1, ARRAY[[255, 255]]::double precision[]
	) AS rast
), gb AS (
	SELECT ST_SetValues(
		ST_AddBand(
			ST_MakeEmptyRaster(1, 1, 0, 0, 1, -1, 0, 0, 0),
			1, '8BUI', 0, NULL
		),
		1, 1, 1, ARRAY[[255]]::double precision[]
	) AS rast
), gray AS (
	SELECT ST_Grayscale(
		ARRAY[
			ROW(red.rast, 1)::rastbandarg,
			ROW(gb.rast, 1)::rastbandarg,
			ROW(gb.rast, 1)::rastbandarg
		]::rastbandarg[],
		'FIRST'
	) AS rast
	FROM red CROSS JOIN gb
)
SELECT
	'#2807.grayscale.first-gap-white',
	(ST_BandMetadata(rast, 1)).nodatavalue,
	(ST_DumpValues(rast)).valarray
FROM gray;

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
