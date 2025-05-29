SELECT 'Extrude roof', ST_AsText(CG_ExtrudeStraightSkeleton('POLYGON (( 0 0, 5 0, 5 5, 4 5, 4 4, 0 4, 0 0 ), (1 1, 1 2,2 2, 2 1, 1 1))', 2.0));
WITH version_info AS (
-- The result depends on the SFCGAL Version.
-- Prior to version 2.2, it contained extra patches.
    SELECT regexp_replace(postgis_full_version(), '.*SFCGAL="([^"]+)".*', '\1') AS sfcgal_version
),
skeleton AS (
    SELECT ST_NumPatches(CG_ExtrudeStraightSkeleton('POLYGON (( 0 0, 5 0, 5 5, 4 5, 4 4, 0 4, 0 0 ), (1 1, 1 2,2 2, 2 1, 1 1))', 3.0, 2.0)) AS num_patches
),
check_patches AS (
  SELECT
      sfcgal_version,
      num_patches,
      (
          (sfcgal_version <= '2.1.0' AND num_patches = 49) OR
	  (sfcgal_version > '2.1.0' AND num_patches = 39)
      ) AS test_result
  FROM skeleton, version_info
)
SELECT 'Extrude building and roof', num_patches FROM skeleton;
