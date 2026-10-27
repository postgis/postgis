SELECT 'base', count(*), ST_SameAlignment(rast) FROM loadedrast;
SELECT 'overview', count(*), ST_SameAlignment(rast) FROM o_3_loadedrast;
SELECT r_table_name, scale_x::numeric(16, 10), scale_y::numeric(16, 10),
	CASE WHEN r_table_name = 'o_3_loadedrast' THEN blocksize_x IN (33, 34) ELSE blocksize_x = 100 END,
	CASE WHEN r_table_name = 'o_3_loadedrast' THEN blocksize_y IN (33, 34) ELSE blocksize_y = 100 END,
	same_alignment, ST_AsText(extent)
FROM raster_columns
WHERE r_table_name IN ('loadedrast', 'o_3_loadedrast')
ORDER BY r_table_name;
