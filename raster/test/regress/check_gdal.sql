-- postgis.gdal_datapath
SELECT
	CASE
		WHEN strpos(postgis_gdal_version(), 'GDAL_DATA') <> 0
			THEN false
		ELSE NULL
	END;
SET postgis.gdal_datapath = 'invalid_path';
SHOW postgis.gdal_datapath;
SET postgis.gdal_datapath = default;
SELECT
	CASE
		WHEN strpos(postgis_gdal_version(), 'GDAL_DATA') <> 0
			THEN false
		ELSE NULL
	END;

-- postgis.gdal_enabled_drivers
-- File-oriented options are not valid in the process-wide VSI option GUC.
SET client_min_messages TO error;
SET postgis.gdal_vsi_options = 'GDAL_HTTP_HEADER_FILE=/tmp/postgis-header';
SHOW postgis.gdal_vsi_options;
SET client_min_messages TO warning;

SET client_min_messages TO warning;
SET postgis.gdal_enabled_drivers = 'DISABLE_ALL';
SELECT count(*) = 0 FROM ST_GDALDrivers();
SHOW postgis.gdal_enabled_drivers;

SET postgis.gdal_enabled_drivers = 'ENABLE_ALL';
SHOW postgis.gdal_enabled_drivers;
SELECT count(*) > 0 FROM ST_GDALDrivers();

SET postgis.gdal_enabled_drivers = 'GTiff PNG JPEG';
SHOW postgis.gdal_enabled_drivers;
SELECT count(*) = 3 FROM ST_GDALDrivers();

SET postgis.gdal_enabled_drivers = 'DISABLE_ALL';
SHOW postgis.gdal_enabled_drivers;
SELECT count(*) = 0 FROM ST_GDALDrivers();

SET postgis.gdal_enabled_drivers = 'ENABLE_ALL';
CREATE TEMP TABLE gdal_open_check AS
	SELECT ST_AsGDALRaster(
		ST_AddBand(ST_MakeEmptyRaster(1, 1, 0, 0, 1, -1, 0, 0, 0), 1, '8BUI', 7, 0),
		'GTiff'
	) AS rast;
SET postgis.gdal_enabled_drivers = 'DISABLE_ALL';
CREATE FUNCTION pg_temp.gdal_open_check() RETURNS text AS $$
BEGIN
	PERFORM ST_FromGDALRaster(rast) FROM gdal_open_check;
	RETURN 'opened';
EXCEPTION WHEN OTHERS THEN
	RETURN SQLERRM;
END;
$$ LANGUAGE plpgsql;
SELECT pg_temp.gdal_open_check() = 'rt_util_gdal_open: Cannot open file. All GDAL drivers disabled';
SET postgis.gdal_enabled_drivers = 'ENABLE_ALL';
SELECT pg_temp.gdal_open_check() = 'opened';
