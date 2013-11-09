SELECT 
	CASE
		WHEN strpos(postgis_gdal_version(), 'GDAL_DATA') <> 0
			THEN false
		ELSE NULL
	END;
SET postgis.gdal_datapath = '';
SELECT 
	CASE
		WHEN strpos(postgis_gdal_version(), 'GDAL_DATA') <> 0
			THEN NULL
		ELSE TRUE
	END;
SET postgis.gdal_datapath = default;
SELECT 
	CASE
		WHEN strpos(postgis_gdal_version(), 'GDAL_DATA') <> 0
			THEN false
		ELSE NULL
	END;
