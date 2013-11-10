-- #1485
SELECT '#1485', count(*) FROM geometry_columns
WHERE f_table_name = 'raster_columns';

-- #2532
SELECT NULL::raster @ null::geometry;
SELECT NULL::geometry @ null::raster;
