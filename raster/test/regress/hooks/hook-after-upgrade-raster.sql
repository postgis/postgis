DROP VIEW upgrade_test_raster_view_st_value;
DROP VIEW upgrade_test_raster_view_st_clip;
DROP VIEW upgrade_test_raster_view_st_intersects;

DROP TABLE upgrade_test_raster;
DROP TABLE upgrade_test_raster_with_regular_blocking;

DROP VIEW upgrade_test_raster_view_st_slope;
DROP VIEW upgrade_test_raster_view_st_aspect;

-- Drop deprecated functions
\i :regdir/hooks/drop-deprecated-functions.sql
