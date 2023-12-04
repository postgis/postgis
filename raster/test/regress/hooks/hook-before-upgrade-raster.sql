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

CREATE TABLE upgrade_test_raster_with_regular_blocking(r raster);
INSERT INTO upgrade_test_raster_with_regular_blocking(r) VALUES
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
SELECT AddRasterConstraints('upgrade_test_raster_with_regular_blocking', 'r', 'regular_blocking');

-- See https://trac.osgeo.org/postgis/ticket/5484
CREATE VIEW upgrade_test_raster_view_st_value AS
SELECT
	ST_Value(r, ST_MakePoint(0, 0))
FROM upgrade_test_raster;

-- See https://trac.osgeo.org/postgis/ticket/5489
CREATE VIEW upgrade_test_raster_view_st_intersects AS
SELECT
	st_intersects(NULL::raster, NULL::int, NULL::raster, NULL::integer) sig1,
	st_intersects(NULL::raster, NULL::raster) sig2,
	st_intersects(NULL::geometry, NULL::raster, NULL::int) sig3,
	st_intersects(NULL::raster, NULL::geometry, NULL::int) sig4,
	st_intersects(NULL::raster, NULL::int, NULL::geometry)  sig5;

-- See https://trac.osgeo.org/postgis/ticket/5490
CREATE VIEW upgrade_test_raster_view_st_slope AS
SELECT
	st_slope(NULL::raster, NULL::int, NULL::raster) sig1,
	st_slope(NULL::raster, NULL::int) sig2;

-- See https://trac.osgeo.org/postgis/ticket/5491
CREATE VIEW upgrade_test_raster_view_st_aspect AS
SELECT
	st_aspect(NULL::raster, NULL::int, NULL::raster) sig1,
	st_aspect(NULL::raster) sig2;
