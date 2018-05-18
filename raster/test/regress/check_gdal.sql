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
