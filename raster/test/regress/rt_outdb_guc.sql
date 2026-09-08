CREATE FUNCTION outdb_guc_can_read_metadata(rast raster)
RETURNS boolean
AS $$
BEGIN
	PERFORM ST_BandFileSize(rast, 1);
	RETURN true;
EXCEPTION WHEN OTHERS THEN
	RETURN false;
END;
$$ LANGUAGE plpgsql;

CREATE FUNCTION outdb_guc_can_read_pixel(rast raster)
RETURNS boolean
AS $$
BEGIN
	PERFORM ST_Value(rast, 1, 1, 1);
	RETURN true;
EXCEPTION WHEN OTHERS THEN
	RETURN false;
END;
$$ LANGUAGE plpgsql;

DO $$
BEGIN
	PERFORM postgis_raster_lib_version();
END;
$$;

CREATE ROLE outdb_guc_regression_guest;
GRANT SELECT ON raster_outdb_template TO outdb_guc_regression_guest;
GRANT EXECUTE ON FUNCTION outdb_guc_can_read_metadata(raster) TO outdb_guc_regression_guest;
GRANT EXECUTE ON FUNCTION outdb_guc_can_read_pixel(raster) TO outdb_guc_regression_guest;

SET postgis.gdal_enabled_drivers = 'GTiff';
RESET postgis.enable_outdb_rasters;
SHOW postgis.enable_outdb_rasters;
SET ROLE outdb_guc_regression_guest;
SELECT 'off_metadata', outdb_guc_can_read_metadata(rast)
FROM raster_outdb_template WHERE rid = 1;
SELECT 'off_pixel', outdb_guc_can_read_pixel(rast)
FROM raster_outdb_template WHERE rid = 1;
SELECT 'off_indb_pixel', (
	ST_Value(
		ST_AddBand(
			ST_MakeEmptyRaster(1, 1, 0, 0, 1, -1, 0, 0, 0),
			1, '8BUI', 7, 0
		),
		1, 1, 1
	) = 7
);

RESET ROLE;
SET postgis.enable_outdb_rasters = true;
SET ROLE outdb_guc_regression_guest;
SELECT 'on_metadata', outdb_guc_can_read_metadata(rast)
FROM raster_outdb_template WHERE rid = 1;
SELECT 'on_pixel', outdb_guc_can_read_pixel(rast)
FROM raster_outdb_template WHERE rid = 1;

RESET ROLE;
RESET postgis.enable_outdb_rasters;
SHOW postgis.enable_outdb_rasters;
SET ROLE outdb_guc_regression_guest;
SELECT 'reset_metadata', outdb_guc_can_read_metadata(rast)
FROM raster_outdb_template WHERE rid = 1;
SELECT 'reset_pixel', outdb_guc_can_read_pixel(rast)
FROM raster_outdb_template WHERE rid = 1;

RESET ROLE;
BEGIN;
SET postgis.enable_outdb_rasters = true;
SHOW postgis.enable_outdb_rasters;
ROLLBACK;
SHOW postgis.enable_outdb_rasters;
SET ROLE outdb_guc_regression_guest;
SELECT 'rollback_metadata', outdb_guc_can_read_metadata(rast)
FROM raster_outdb_template WHERE rid = 1;
SELECT 'rollback_pixel', outdb_guc_can_read_pixel(rast)
FROM raster_outdb_template WHERE rid = 1;

RESET ROLE;
SET postgis.enable_outdb_rasters = true;
REVOKE SELECT ON raster_outdb_template FROM outdb_guc_regression_guest;
REVOKE EXECUTE ON FUNCTION outdb_guc_can_read_metadata(raster) FROM outdb_guc_regression_guest;
REVOKE EXECUTE ON FUNCTION outdb_guc_can_read_pixel(raster) FROM outdb_guc_regression_guest;
DROP FUNCTION outdb_guc_can_read_metadata(raster);
DROP FUNCTION outdb_guc_can_read_pixel(raster);
DROP ROLE outdb_guc_regression_guest;
