DROP VIEW upgrade_test_raster_view_st_value;
DROP VIEW upgrade_test_raster_view_st_intersects;

DROP TABLE upgrade_test_raster;
DROP TABLE upgrade_test_raster_with_regular_blocking;

DROP VIEW upgrade_test_raster_view_st_slope;
DROP VIEW upgrade_test_raster_view_st_aspect;

-- Drop functions deprecated on upgrade
DROP FUNCTION IF EXISTS st_value_deprecated_by_postgis_201(raster,geometry,boolean);
