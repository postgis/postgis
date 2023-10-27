DO $SANDBOX$
BEGIN

	EXECUTE format(
		$$
			ALTER DATABASE %I
			SET session_preload_libraries = 'pgextwlist.so'
		$$,
		current_database()
	);

	EXECUTE format(
		$$
			ALTER DATABASE %I
			SET extwlist.extensions='postgis,postgis_raster,postgis_topology,postgis_sfcgal,address_standardizer,postgis_tiger_geocoder'
		$$,
		current_database()
	);

END;
$SANDBOX$ LANGUAGE 'plpgsql';
