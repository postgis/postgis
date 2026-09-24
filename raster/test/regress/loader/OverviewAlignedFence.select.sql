SELECT 'base', count(*), ST_SameAlignment(rast) FROM loadedrast;
SELECT 'overview', count(*), ST_SameAlignment(rast) FROM o_3_loadedrast;
SELECT r_table_name, scale_x::numeric(16, 10), scale_y::numeric(16, 10),
	blocksize_x, blocksize_y, same_alignment, ST_AsText(extent)
FROM raster_columns
WHERE r_table_name IN ('loadedrast', 'o_3_loadedrast')
ORDER BY r_table_name;
