CREATE TABLE upgrade_test_raster(r raster);
INSERT INTO upgrade_test_raster(r) VALUES
(
	ST_AddBand(
		ST_MakeEmptyRaster(
			10, 10, 1, 1, 2, 2, 0, 0,4326
		),
		1,
		'8BSI'::text,
		-129,
		NULL
	)
);

--SET client_min_messages TO ERROR;
SELECT AddRasterConstraints('upgrade_test_raster', 'r');
