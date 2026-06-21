WITH src AS (
	SELECT ST_MakeEmptyRaster(3, 2, 10, 20, 5, -5, 0, 0, 4326) AS rast
), idx AS (
	SELECT ST_CreateIndexRaster(rast) AS rast
	FROM src
)
SELECT
	'default',
	(ST_Metadata(rast)).width,
	(ST_Metadata(rast)).height,
	(ST_Metadata(rast)).upperleftx,
	(ST_Metadata(rast)).upperlefty,
	(ST_Metadata(rast)).scalex,
	(ST_Metadata(rast)).scaley,
	(ST_Metadata(rast)).srid,
	ST_BandPixelType(rast),
	(ST_DumpValues(rast)).valarray
FROM idx;

WITH src AS (
	SELECT ST_MakeEmptyRaster(3, 2, 10, 20, 5, -5, 0, 0, 4326) AS rast
), idx AS (
	SELECT ST_CreateIndexRaster(rast, '16BUI', 10, false, false, true, false) AS rast
	FROM src
)
SELECT
	'reverse-column-boustrophedon',
	ST_BandPixelType(rast),
	(ST_DumpValues(rast)).valarray
FROM idx;

WITH src AS (
	SELECT ST_MakeEmptyRaster(3, 2, 10, 20, 5, -5, 0, 0, 4326) AS rast
), idx AS (
	SELECT ST_CreateIndexRaster(rast, '16BUI', 5, true, true, false, true, 10, 30) AS rast
	FROM src
)
SELECT
	'row-first-custom-increments',
	ST_BandPixelType(rast),
	(ST_DumpValues(rast)).valarray
FROM idx;

SELECT 'null-raster', ST_CreateIndexRaster(NULL::raster) IS NULL;

WITH src AS (
	SELECT ST_MakeEmptyRaster(2, 1, 10, 20, 5, -5, 0, 0, 4326) AS rast
), idx AS (
	SELECT ST_CreateIndexRaster(rast, '32BUI', 2147483647) AS rast
	FROM src
)
SELECT
	'wide-32bui',
	ST_BandPixelType(rast),
	(ST_DumpValues(rast)).valarray
FROM idx;

SELECT 'bad-column-increment', ST_CreateIndexRaster(
	ST_MakeEmptyRaster(3, 2, 10, 20, 5, -5, 0, 0, 4326),
	'16BUI', 0, true, true, true, true, 1, NULL
);

SELECT 'zero-row-increment', ST_CreateIndexRaster(
	ST_MakeEmptyRaster(3, 2, 10, 20, 5, -5, 0, 0, 4326),
	'16BUI', 0, true, true, true, true, NULL, 0
);

SELECT 'negative-column-increment', ST_CreateIndexRaster(
	ST_MakeEmptyRaster(3, 2, 10, 20, 5, -5, 0, 0, 4326),
	'16BUI', 0, true, true, true, true, -1, NULL
);
