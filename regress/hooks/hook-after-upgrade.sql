-- TODO: move all these views and tables under postgis_upgrade_test_data
DROP VIEW IF EXISTS upgrade_view_test_overlay;
DROP VIEW IF EXISTS upgrade_view_test_unaryunion;
DROP VIEW IF EXISTS upgrade_view_test_subdivide;
DROP VIEW IF EXISTS upgrade_view_test_union;
DROP VIEW IF EXISTS upgrade_view_test_force_dims;
DROP VIEW IF EXISTS upgrade_view_test_askml;
DROP VIEW IF EXISTS upgrade_view_test_dwithin;
DROP VIEW IF EXISTS upgrade_view_test_clusterkmeans;
DROP VIEW IF EXISTS upgrade_view_test_distance;
DROP TABLE IF EXISTS upgrade_test;

-- Drop any upgrade test data
DROP SCHEMA IF EXISTS postgis_upgrade_test_data CASCADE;

-- Drop deprecated functions
\i :regdir/hooks/drop-deprecated-functions.sql
