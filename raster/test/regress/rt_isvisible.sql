SET postgis.gdal_enabled_drivers = 'GTiff';

WITH dem AS (
	SELECT ST_SetSRID(
		ST_AddBand(
			ST_MakeEmptyRaster(5, 5, 0, 5, 1, -1, 0, 0, 0),
			'32BF'::text, 0, NULL
		),
		3857
	) AS rast
)
SELECT 'isvisible-flat',
	ST_IsVisible(
		rast,
		ST_SetSRID(ST_PointZ(0.5, 2.5, 1), 3857),
		ST_SetSRID(ST_PointZ(4.5, 2.5, 1), 3857)
	)
FROM dem;

WITH dem AS (
	SELECT ST_SetSRID(
		ST_AddBand(
			ST_MakeEmptyRaster(5, 5, 0, 5, 1, -1, 0, 0, 0),
			'32BF'::text, 0, NULL
		),
		3857
	) AS rast
)
SELECT 'isvisible-curvature-bool',
	ST_IsVisible(
		rast,
		ST_SetSRID(ST_PointZ(0.5, 2.5, 1), 3857),
		ST_SetSRID(ST_PointZ(4.5, 2.5, 1), 3857),
		true
	)
FROM dem;

WITH dem AS (
	SELECT ST_SetSRID(
		ST_AddBand(
			ST_MakeEmptyRaster(5, 5, 0, 5, 1, -1, 0, 0, 0),
			'32BF'::text, 0, NULL
		),
		3857
	) AS rast
)
SELECT 'isvisible-curvature-coef',
	ST_IsVisible(
		rast,
		ST_SetSRID(ST_PointZ(0.5, 2.5, 1), 3857),
		ST_SetSRID(ST_PointZ(4.5, 2.5, 1), 3857),
		0.0::double precision
	)
FROM dem;

WITH dem AS (
	SELECT ST_SetSRID(
		ST_AddBand(
			ST_MakeEmptyRaster(5, 5, 0, 5, 1, -1, 0, 0, 0),
			'32BF'::text, 0, NULL
		),
		3857
	) AS rast
)
SELECT 'isvisible-target-off-center',
	ST_IsVisible(
		rast,
		ST_SetSRID(ST_PointZ(0.5, 2.5, 1), 3857),
		ST_SetSRID(ST_PointZ(4.01, 2.99, 1), 3857)
	)
FROM dem;

WITH dem AS (
	SELECT ST_SetValue(
		ST_SetSRID(
			ST_AddBand(
				ST_MakeEmptyRaster(5, 5, 0, 5, 1, -1, 0, 0, 0),
				'32BF'::text, 0, NULL
			),
			3857
		),
		1, 3, 3, 10
	) AS rast
)
SELECT 'isvisible-blocked',
	ST_IsVisible(
		rast,
		ST_SetSRID(ST_PointZ(0.5, 2.5, 1), 3857),
		ST_SetSRID(ST_PointZ(4.5, 2.5, 1), 3857)
	)
FROM dem;

WITH dem AS (
	SELECT ST_SetSRID(
		ST_AddBand(
			ST_MakeEmptyRaster(5, 5, 0, 5, 1, -1, 0, 0, 0),
			'32BF'::text, 0, NULL
		),
		3857
	) AS rast
)
SELECT 'isvisible-band2',
	ST_IsVisible(
		rast,
		2,
		ST_SetSRID(ST_PointZ(0.5, 2.5, 1), 3857),
		ST_SetSRID(ST_PointZ(4.5, 2.5, 1), 3857)
	)
FROM dem;

WITH dem AS (
	SELECT ST_AddBand(
		ST_MakeEmptyRaster(5, 1, 0, 1, 1, -1, 0, 0, 0),
		'32BF'::text, 0, NULL
	) AS rast
)
SELECT 'isvisible-srid',
	ST_IsVisible(
		rast,
		ST_SetSRID(ST_PointZ(0.5, 0.5, 1), 4326),
		ST_PointZ(4.5, 0.5, 1)
	)
FROM dem;

WITH dem AS (
	SELECT ST_SetSRID(
		ST_AddBand(
			ST_MakeEmptyRaster(5, 5, 0, 5, 1, -1, 0, 0, 0),
			'32BF'::text, 0, NULL
		),
		3857
	) AS rast
)
SELECT 'isvisible-curvature-nan',
	ST_IsVisible(
		rast,
		ST_SetSRID(ST_PointZ(0.5, 2.5, 1), 3857),
		ST_SetSRID(ST_PointZ(4.5, 2.5, 1), 3857),
		'NaN'::double precision
	)
FROM dem;
