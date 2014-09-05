-- #1485
SELECT '#1485', count(*) FROM geometry_columns
WHERE f_table_name = 'raster_columns';

-- #2532
SELECT '#2532.1', NULL::raster @ null::geometry;
SELECT '#2532.2', NULL::geometry @ null::raster;

-- #2911
WITH data AS ( SELECT '#2911' l, ST_Metadata(ST_Rescale(
 ST_AddBand(
  ST_MakeEmptyRaster(10, 10, 0, 0, 1, -1, 0, 0, 0),
  1, '8BUI', 0, 0
 ),
 2.0,
 -2.0
 )) m
) SELECT l, (m).* FROM data;
