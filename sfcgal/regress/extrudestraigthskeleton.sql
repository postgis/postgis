WITH version_info AS (
  SELECT string_to_array(postgis_sfcgal_version(), '.')::int[] AS sfcgal_version_array
),

roof AS (
  SELECT
    ST_NumPatches(
      CG_ExtrudeStraightSkeleton(
        'POLYGON ((0 0, 5 0, 5 5, 4 5, 4 4, 0 4, 0 0), (1 1, 1 2, 2 2, 2 1, 1 1))',
        2.0
      )
    ) AS num_patches,
    sfcgal_version_array
  FROM version_info
),

building_and_roof AS (
  SELECT
    ST_NumPatches(
      CG_ExtrudeStraightSkeleton(
        'POLYGON ((0 0, 5 0, 5 5, 4 5, 4 4, 0 4, 0 0), (1 1, 1 2, 2 2, 2 1, 1 1))',
        3.0,
        2.0
      )
    ) AS num_patches,
    sfcgal_version_array
  FROM version_info
)

SELECT 'Extrude roof',
       -- The result depends on the SFCGAL version.
       -- Since version 2.3, the result is simplified by merging
       -- triangles before storing the result, resulting in fewer
       -- patches.
       (
         (
	   sfcgal_version_array < ARRAY[2,3,0]
	   AND num_patches = 38
	 )
         OR (
	   sfcgal_version_array >= ARRAY[2,3,0]
	   AND num_patches = 11
	 )
       )
FROM roof

UNION ALL

SELECT 'Extrude building and roof',
       -- The result depends on the SFCGAL version.
       -- Prior to version 2.2, it contained extra patches.
       -- Since version 2.3, the result is simplified by merging
       -- triangles before storing the result, resulting in fewer
       -- patches.
       (
         (
	   sfcgal_version_array < ARRAY[2,2,0]
	   AND num_patches = 49
	 )
         OR (
	   sfcgal_version_array >= ARRAY[2,2,0]
	   AND sfcgal_version_array < ARRAY[2,3,0]
	   AND num_patches = 39
	 )
         OR (
	   sfcgal_version_array >= ARRAY[2,3,0]
	   AND num_patches = 21
	 )
       )
FROM building_and_roof;
SELECT 'Empty building and roof', ST_AsText(CG_ExtrudeStraightSkeleton(ST_GeomFromText('POLYGON EMPTY',4326), 20.1, 20.1));
SELECT 'Empty roof', ST_AsText(CG_ExtrudeStraightSkeleton(ST_GeomFromText('POLYGON EMPTY',4326), 20.1));
