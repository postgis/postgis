SELECT 
	CASE
		WHEN strpos(postgis_gdal_version(), 'GDAL_DATA') <> 0
			THEN false
		ELSE NULL
	END;
