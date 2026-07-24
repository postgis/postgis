SELECT blocksize_x, blocksize_y, regular_blocking FROM raster_columns WHERE r_table_name = 'loadedrast' AND r_raster_column = 'rast';
SELECT count(*) FROM loadedrast;
SELECT ST_Width(rast), ST_Height(rast) FROM loadedrast ORDER BY rid LIMIT 1;
SELECT ST_Width(rast), ST_Height(rast) FROM loadedrast ORDER BY rid DESC LIMIT 1;
