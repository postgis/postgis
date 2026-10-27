SET client_min_messages TO warning;

WITH dst AS (
	SELECT ST_AddBand(
		ST_MakeEmptyRaster(4, 4, 0, 0, 1, -1, 0, 0, 0),
		1, '8BUI', 1, 0
	) AS rast
), src AS (
	SELECT ST_SetValues(
		ST_AddBand(
			ST_MakeEmptyRaster(2, 2, 1, -1, 1, -1, 0, 0, 0),
			1, '8BUI', 0, 0
		),
		1, 1, 1,
		ARRAY[[10, 20], [30, 40]]::double precision[][]
	) AS rast
)
SELECT 'copy_aligned',
	ST_DumpValues(ST_SetValues(dst.rast, 1, 1, 1, 4, 4, src.rast, 1, false, false), 1, false)
FROM dst, src;

WITH dst AS (
	SELECT ST_AddBand(
		ST_MakeEmptyRaster(4, 4, 0, 0, 1, -1, 0, 0, 0),
		1, '8BUI', 1, 0
	) AS rast
), src AS (
	SELECT ST_SetValues(
		ST_AddBand(
			ST_MakeEmptyRaster(2, 2, 1, -1, 1, -1, 0, 0, 0),
			1, '8BUI', 0, 0
		),
		1, 1, 1,
		ARRAY[[10, 0], [30, 40]]::double precision[][]
	) AS rast
)
SELECT 'skip_source_nodata',
	ST_DumpValues(ST_SetValues(dst.rast, 1, 1, 1, 4, 4, src.rast, 1, false, true), 1, false)
FROM dst, src;

WITH dst AS (
	SELECT ST_SetValue(
		ST_AddBand(
			ST_MakeEmptyRaster(4, 4, 0, 0, 1, -1, 0, 0, 0),
			1, '8BUI', 1, 0
		),
		1, 2, 2, 0
	) AS rast
), src AS (
	SELECT ST_SetValues(
		ST_AddBand(
			ST_MakeEmptyRaster(2, 2, 1, -1, 1, -1, 0, 0, 0),
			1, '8BUI', 0, 0
		),
		1, 1, 1,
		ARRAY[[10, 20], [30, 40]]::double precision[][]
	) AS rast
)
SELECT 'keep_dest_nodata',
	ST_DumpValues(ST_SetValues(dst.rast, 1, 1, 1, 4, 4, src.rast, 1, true, false), 1, false)
FROM dst, src;

WITH dst AS (
	SELECT ST_AddBand(
		ST_MakeEmptyRaster(3, 3, 0, 0, 1, -1, 0, 0, 0),
		1, '8BUI', 1, 0
	) AS rast
), src AS (
	SELECT ST_SetValues(
		ST_AddBand(
			ST_MakeEmptyRaster(1, 1, 1, -1, 1, -1, 0, 0, 0),
			1, '8BUI', 0, 0
		),
		1, 1, 1,
		ARRAY[[99]]::double precision[][]
	) AS rast
)
SELECT 'first_band_wrapper',
	ST_DumpValues(ST_SetValues(dst.rast, 1, 1, 3, 3, src.rast, 1, false, false), 1, false)
FROM dst, src;

WITH r AS (
	SELECT ST_AddBand(
		ST_MakeEmptyRaster(3, 3, 0, 0, 1, -1, 0, 0, 0),
		1, '8BUI', 1, 0
	) AS rast
)
SELECT 'null_scalar_fill',
	ST_DumpValues(ST_SetValues(rast, 1, 1, 1, 2, 2, NULL), 1, false)
FROM r;

CREATE OR REPLACE FUNCTION _test_setvalues_raster_error(rast1 raster, rast2 raster)
RETURNS text AS $$
BEGIN
	PERFORM ST_SetValues(rast1, 1, 1, 1, 2, 2, rast2, 1, false, false);
	RETURN 'no error';
EXCEPTION WHEN OTHERS THEN
	RETURN SQLERRM;
END;
$$ LANGUAGE plpgsql;

WITH dst AS (
	SELECT ST_AddBand(
		ST_MakeEmptyRaster(2, 2, 0, 0, 1, -1, 0, 0, 0),
		1, '8BUI', 1, 0
	) AS rast
), src AS (
	SELECT ST_AddBand(
		ST_MakeEmptyRaster(2, 2, 0, 0, 1, -1, 0, 0, 4326),
		1, '8BUI', 1, 0
	) AS rast
)
SELECT 'srid_mismatch', _test_setvalues_raster_error(dst.rast, src.rast)
FROM dst, src;

WITH dst AS (
	SELECT ST_AddBand(
		ST_MakeEmptyRaster(2, 2, 0, 0, 1, -1, 0, 0, 0),
		1, '8BUI', 1, 0
	) AS rast
), src AS (
	SELECT ST_AddBand(
		ST_MakeEmptyRaster(2, 2, 0.5, -0.5, 1, -1, 0, 0, 0),
		1, '8BUI', 1, 0
	) AS rast
)
SELECT 'alignment_mismatch', _test_setvalues_raster_error(dst.rast, src.rast)
FROM dst, src;

DROP FUNCTION _test_setvalues_raster_error(raster, raster);
