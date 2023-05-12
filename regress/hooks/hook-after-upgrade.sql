DROP VIEW IF EXISTS upgrade_view_test_overlay;
DROP VIEW IF EXISTS upgrade_view_test_unaryunion;
DROP VIEW IF EXISTS upgrade_view_test_subdivide;
DROP VIEW IF EXISTS upgrade_view_test_union;
DROP VIEW IF EXISTS upgrade_view_test_force_dims;
DROP VIEW IF EXISTS upgrade_view_test_askml;
DROP VIEW IF EXISTS upgrade_view_test_dwithin;
DROP VIEW IF EXISTS upgrade_view_test_clusterkmeans;
DROP VIEW IF EXISTS upgrade_view_test_distance;
DROP TABLE upgrade_test;

-- Drop functions deprecated on upgrade
DROP FUNCTION IF EXISTS st_force3dz_deprecated_by_postgis_301(geometry);
DROP FUNCTION IF EXISTS st_force3d_deprecated_by_postgis_301(geometry);
DROP FUNCTION IF EXISTS st_force3dm_deprecated_by_postgis_301(geometry);
DROP FUNCTION IF EXISTS st_force4d_deprecated_by_postgis_301(geometry);
DROP FUNCTION IF EXISTS st_intersection_deprecated_by_postgis_301(geometry,geometry);
DROP FUNCTION IF EXISTS st_difference_deprecated_by_postgis_301(geometry,geometry);
DROP FUNCTION IF EXISTS st_symdifference_deprecated_by_postgis_301(geometry,geometry);
DROP FUNCTION IF EXISTS st_unaryunion_deprecated_by_postgis_301(geometry);
DROP FUNCTION IF EXISTS st_subdivide_deprecated_by_postgis_301(geometry,integer);
DROP FUNCTION IF EXISTS st_askml_deprecated_by_postgis_200(geometry,integer);
DROP FUNCTION IF EXISTS st_askml_deprecated_by_postgis_200(geography,integer);
DROP FUNCTION IF EXISTS st_dwithin_deprecated_by_postgis_300(geography,geography,float8);
DROP FUNCTION IF EXISTS st_dwithin_deprecated_by_postgis_300(text,text,float8);
DROP FUNCTION IF EXISTS st_clusterkmeans_deprecated_by_postgis_302(geometry,integer);
