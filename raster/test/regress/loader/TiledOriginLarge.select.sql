SELECT blocksize_x, blocksize_y, same_alignment, regular_blocking FROM raster_columns WHERE r_table_name = 'loadedrast' AND r_raster_column = 'rast';
SELECT count(*) FROM loadedrast;
SELECT ST_Width(rast), ST_Height(rast), round(ST_UpperLeftX(rast)::numeric, 8), round(ST_UpperLeftY(rast)::numeric, 8) FROM loadedrast ORDER BY rid LIMIT 1;
