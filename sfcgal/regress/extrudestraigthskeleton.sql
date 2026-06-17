WITH
  version_info AS (
    SELECT
      string_to_array(postgis_sfcgal_version (), '.')::INT[] AS sfcgal_version_array
  ),
  roof AS (
    SELECT
      CG_ExtrudeStraightSkeleton (
        'POLYGON (( 0 0, 5 0, 5 5, 4 5, 4 4, 0 4, 0 0 ), (1 1, 1 2,2 2, 2 1, 1 1))',
        2.0
      ) AS geom
  ),
  check_roof AS (
    SELECT
      sfcgal_version_array,
      GeometryType (geom) AS geometry_type,
      ST_CoordDim (geom) AS coord_dim,
      ST_IsEmpty (geom) AS is_empty,
      ST_NumPatches (geom) AS num_patches
    FROM
      roof,
      version_info
  )
SELECT
  'Extrude roof',
  (
    geometry_type = 'POLYHEDRALSURFACE'
    AND coord_dim = 3
    AND NOT is_empty
    AND (
      (
        sfcgal_version_array < ARRAY[2, 3, 0]
        AND num_patches = 38
      )
      OR (
        sfcgal_version_array >= ARRAY[2, 3, 0]
        AND num_patches = 11
      )
    )
  )
FROM
  check_roof;
WITH
  version_info AS (
    -- The result depends on the SFCGAL Version.
    -- Prior to version 2.2, it contained extra patches.
    SELECT
      string_to_array(postgis_sfcgal_version (), '.')::INT[] AS sfcgal_version_array
  ),
  skeleton AS (
    SELECT
      ST_NumPatches (
        CG_ExtrudeStraightSkeleton (
          'POLYGON (( 0 0, 5 0, 5 5, 4 5, 4 4, 0 4, 0 0 ), (1 1, 1 2,2 2, 2 1, 1 1))',
          3.0,
          2.0
        )
      ) AS num_patches
  ),
  check_patches AS (
    SELECT
      sfcgal_version_array,
      num_patches,
      (
        (
          sfcgal_version_array <= ARRAY[2, 1, 0]
          AND num_patches = 49
        )
        OR (
          sfcgal_version_array > ARRAY[2, 1, 0]
          AND sfcgal_version_array < ARRAY[2, 3, 0]
          AND num_patches = 39
        )
        OR (
          sfcgal_version_array >= ARRAY[2, 3, 0]
          AND num_patches = 21
        )
      ) AS test_result
    FROM
      skeleton,
      version_info
  )
SELECT
  'Extrude building and roof',
  test_result
FROM
  check_patches;
SELECT 'Empty building and roof', ST_AsText(CG_ExtrudeStraightSkeleton(ST_GeomFromText('POLYGON EMPTY',4326), 20.1, 20.1));
SELECT 'Empty roof', ST_AsText(CG_ExtrudeStraightSkeleton(ST_GeomFromText('POLYGON EMPTY',4326), 20.1));
