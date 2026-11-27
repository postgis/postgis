SET client_min_messages TO warning;

WITH src AS (
	SELECT ST_SetValues(
		ST_AddBand(ST_MakeEmptyRaster(3, 3, 0, 0, 1, -1, 0, 0, 0), 1, '32BF', 0, -9999),
		1, 1, 1, ARRAY[
			[1, 1, 1],
			[1, 5, 1],
			[1, 1, 1]
		]::double precision[][]
	) AS rast
)
SELECT 'umm_default', x, y, round(ST_Value(ST_Sharpen(rast, 'UMM'), x, y)::numeric, 6)
FROM src
CROSS JOIN generate_series(1, 3) AS y
CROSS JOIN generate_series(1, 3) AS x
ORDER BY y, x;

WITH src AS (
	SELECT ST_SetValues(
		ST_AddBand(ST_MakeEmptyRaster(3, 3, 0, 0, 1, -1, 0, 0, 0), 1, '32BF', 0, -9999),
		1, 1, 1, ARRAY[
			[1, 1, 1],
			[1, 5, 1],
			[1, 1, 1]
		]::double precision[][]
	) AS rast
)
SELECT 'umm_amount', round(ST_Value(ST_Sharpen(rast, 1, '32BF', 'UMM', ARRAY[' amount = 0.5 ']), 2, 2)::numeric, 6)
FROM src;

WITH src AS (
	SELECT ST_AddBand(ST_MakeEmptyRaster(1, 1, 0, 0, 1, -1, 0, 0, 0), 1, '32BF', 1, -9999) AS rast
)
SELECT 'bad_method', ST_Sharpen(rast, 'LoG') FROM src;

WITH src AS (
	SELECT ST_AddBand(ST_MakeEmptyRaster(1, 1, 0, 0, 1, -1, 0, 0, 0), 1, '32BF', 1, -9999) AS rast
)
SELECT 'bad_option', ST_Sharpen(rast, 1, '32BF', 'UMM', ARRAY['radius=2']) FROM src;

WITH src AS (
	SELECT ST_AddBand(ST_MakeEmptyRaster(1, 1, 0, 0, 1, -1, 0, 0, 0), 1, '32BF', 1, -9999) AS rast
)
SELECT 'bad_amount', ST_Sharpen(rast, 1, '32BF', 'UMM', ARRAY['amount=Infinity']) FROM src;

WITH src AS (
	SELECT ST_AddBand(ST_MakeEmptyRaster(1, 1, 0, 0, 1, -1, 0, 0, 0), 1, '32BF', 1, -9999) AS rast
)
SELECT 'bad_amount_negative', ST_Sharpen(rast, 1, '32BF', 'UMM', ARRAY['amount=-0.1']) FROM src;
