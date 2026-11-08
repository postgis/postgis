WITH rast AS (
	SELECT ST_SetValues(
		ST_AddBand(ST_MakeEmptyRaster(3, 3, 0, 0, 1, -1, 0, 0, 0), 1, '32BF', 0, -9999),
		1, 1, 1, ARRAY[
			[7, 7, 7],
			[7, 7, 7],
			[7, 7, 7]
		]::double precision[][]
	) AS rast
)
SELECT 'flat', round(ST_Value(ST_EdgeDetect(rast), 2, 2)::numeric, 6)
FROM rast;

WITH rast AS (
	SELECT ST_SetValues(
		ST_AddBand(ST_MakeEmptyRaster(3, 3, 0, 0, 1, -1, 0, 0, 0), 1, '32BF', 0, -9999),
		1, 1, 1, ARRAY[
			[1, 1, 3],
			[1, 1, 3],
			[1, 1, 3]
		]::double precision[][]
	) AS rast
)
SELECT 'vertical', round(ST_Value(ST_EdgeDetect(rast), 2, 2)::numeric, 6)
FROM rast;

WITH rast AS (
	SELECT ST_SetValues(
		ST_AddBand(ST_MakeEmptyRaster(3, 3, 0, 0, 1, -1, 0, 0, 0), 1, '32BF', 0, -9999),
		1, 1, 1, ARRAY[
			[1, 1, 3],
			[1, 1, 3],
			[1, 1, 3]
		]::double precision[][]
	) AS rast
)
SELECT 'dump', ST_DumpValues(ST_EdgeDetect(rast))
FROM rast;

WITH rast AS (
	SELECT ST_SetValues(
		ST_AddBand(ST_MakeEmptyRaster(3, 3, 0, 0, 1, -1, 0, 0, 0), 1, '32BF', 0, -9999),
		1, 1, 1, ARRAY[
			[1, 1, 1],
			[1, 1, 1],
			[3, 3, 3]
		]::double precision[][]
	) AS rast
)
SELECT 'horizontal', round(ST_Value(ST_EdgeDetect(rast, 1, '32BF', 'Sobel'), 2, 2)::numeric, 6)
FROM rast;

WITH rast AS (
	SELECT ST_AddBand(
		ST_SetValues(
			ST_AddBand(ST_MakeEmptyRaster(3, 3, 0, 0, 1, -1, 0, 0, 0), 1, '32BF', 0, -9999),
			1, 1, 1, ARRAY[
				[7, 7, 7],
				[7, 7, 7],
				[7, 7, 7]
			]::double precision[][]
		),
		'32BF'::text,
		0::double precision,
		-9999::double precision
	) AS rast
)
SELECT 'band2', round(ST_Value(ST_EdgeDetect(ST_SetValue(rast, 2, 3, 2, 5), 2), 2, 2)::numeric, 6)
FROM rast;

WITH rast AS (
	SELECT ST_SetValues(
		ST_AddBand(ST_MakeEmptyRaster(5, 5, 0, 0, 1, -1, 0, 0, 0), 1, '32BF', 0, -9999),
		1, 1, 1, ARRAY[
			[1, 1, 1, 1, 1],
			[1, 1, 3, 3, 3],
			[1, 1, 3, 3, 3],
			[1, 1, 3, 3, 3],
			[1, 1, 1, 1, 1]
		]::double precision[][]
	) AS rast
)
SELECT 'customextent', ST_Width(rast), ST_Height(rast)
FROM (
	SELECT ST_EdgeDetect(
		rast,
		1,
		(
			SELECT tile
			FROM ST_Tile(rast, 2, 2) AS tile
			ORDER BY ST_Height(tile) DESC, ST_Width(tile) DESC, ST_UpperLeftX(tile), ST_UpperLeftY(tile)
			LIMIT 1
		),
		'32BF',
		'Sobel'
	) AS rast
	FROM rast
	LIMIT 1
) AS foo;

WITH rast AS (
	SELECT ST_SetBandNoDataValue(
		ST_SetValues(
			ST_AddBand(ST_MakeEmptyRaster(3, 3, 0, 0, 1, -1, 0, 0, 0), 1, '32BF', 0, -9999),
			1, 1, 1, ARRAY[
				[1, 1, 3],
				[1, NULL, 3],
				[1, 1, 3]
			]::double precision[][]
		),
		-9999
	) AS rast
)
SELECT 'interpolate', ST_Value(ST_EdgeDetect(rast, 1, '32BF', 'Sobel', FALSE), 2, 2) IS NULL,
	round(ST_Value(ST_EdgeDetect(rast, 1, '32BF', 'Sobel', TRUE), 2, 2)::numeric, 6)
FROM rast;

WITH rast AS (
	SELECT ST_SetValues(
		ST_AddBand(ST_MakeEmptyRaster(3, 3, 0, 0, 1, -1, 0, 0, 0), 1, '32BF', 0, -9999),
		1, 1, 1, ARRAY[
			[7, 7, 7],
			[7, NULL, 7],
			[7, 7, 7]
		]::double precision[][]
	) AS rast
)
SELECT 'center_nodata', ST_Value(ST_EdgeDetect(rast), 2, 2) IS NULL
FROM rast;

SELECT ST_EdgeDetect(
	ST_SetValues(
		ST_AddBand(ST_MakeEmptyRaster(3, 3, 0, 0, 1, -1, 0, 0, 0), 1, '32BF', 0, -9999),
		1, 1, 1, ARRAY[
			[7, 7, 7],
			[7, 7, 7],
			[7, 7, 7]
		]::double precision[][]
	),
	1, '32BF', 'Canny'
) IS NULL;
