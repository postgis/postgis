SELECT 
	CASE
		WHEN strpos(postgis_gdal_version(), 'MISSING') <> 0
			THEN false
		ELSE NULL
	END;
