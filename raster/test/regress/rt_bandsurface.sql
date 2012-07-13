DROP TABLE IF EXISTS raster_surface;
CREATE TABLE raster_surface (
	rast raster
);
CREATE OR REPLACE FUNCTION make_test_raster()
	RETURNS void
	AS $$
	DECLARE
		width int := 5;
		height int := 5;
		x int;
		y int;
		rast raster;
	BEGIN
		rast := ST_MakeEmptyRaster(width, height, 0, 0, 1, -1, 0, 0, 0);
		rast := ST_AddBand(rast, 1, '32BUI', 1, 0);

		INSERT INTO raster_surface VALUES (rast);

		RETURN;
	END;
	$$ LANGUAGE 'plpgsql';
SELECT make_test_raster();
DROP FUNCTION make_test_raster();

SELECT
	ST_AsText(ST_BandSurface(rast))
FROM raster_surface;

SELECT
	ST_AsText(ST_BandSurface(rast))
FROM (
	SELECT
		ST_SetValue(
			rast, 1, 1, 1, 0
		) AS rast
	FROM raster_surface
) foo;

SELECT
	ST_AsText(ST_BandSurface(rast))
FROM (
	SELECT
		ST_SetValue(
			ST_SetValue(
				rast, 1, 1, 1, 0
			),
			1, 2, 2, 0
		) AS rast
	FROM raster_surface
) foo;

SELECT
	ST_AsText(ST_BandSurface(rast))
FROM (
	SELECT
		ST_SetValue(
			ST_SetValue(
				ST_SetValue(
					rast, 1, 1, 1, 0
				),
				1, 2, 2, 0
			),
			1, 3, 3, 0
		) AS rast
	FROM raster_surface
) foo;

SELECT
	ST_AsText(ST_BandSurface(rast))
FROM (
	SELECT
		ST_SetValue(
			ST_SetValue(
				ST_SetValue(
					ST_SetValue(
						rast, 1, 1, 1, 0
					),
					1, 2, 2, 0
				),
				1, 3, 3, 0
			),
			1, 4, 4, 0
		) AS rast
	FROM raster_surface
) foo;

SELECT
	ST_AsText(ST_BandSurface(rast))
FROM (
	SELECT
		ST_SetValue(
			ST_SetValue(
				ST_SetValue(
					ST_SetValue(
						ST_SetValue(
							rast, 1, 1, 1, 0
						),
						1, 2, 2, 0
					),
					1, 3, 3, 0
				),
				1, 4, 4, 0
			),
			1, 5, 5, 0
		) AS rast
	FROM raster_surface
) foo;

SELECT
	ST_AsText(ST_BandSurface(rast))
FROM (
	SELECT
		ST_SetValue(
			ST_SetValue(
				ST_SetValue(
					ST_SetValue(
						ST_SetValue(
							ST_SetValue(
								ST_SetValue(
									ST_SetValue(
										ST_SetValue(
											rast, 1, 1, 1, 0
										),
										1, 2, 2, 0
									),
									1, 3, 3, 0
								),
								1, 4, 4, 0
							),
							1, 5, 5, 0
						),
						1, 5, 1, 0
					),
					1, 4, 2, 0
				),
				1, 2, 4, 0
			),
			1, 1, 5, 0
		) AS rast
	FROM raster_surface
) foo;

DROP TABLE IF EXISTS raster_surface;
