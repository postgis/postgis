SELECT srid, scale_x::numeric(16, 10), scale_y::numeric(16, 10), blocksize_x, blocksize_y, same_alignment, regular_blocking, num_bands, pixel_types, nodata_values::numeric(16,10)[], out_db, ST_AsEWKT(extent) FROM raster_columns WHERE r_table_name = 'o_3_loadedrast' AND r_raster_column = 'rast';
SELECT ST_Value(rast, 1, 27, 1) FROM o_3_loadedrast WHERE rid = 1;
SELECT ST_Value(rast, 2, 23, 13) FROM o_3_loadedrast WHERE rid = 1;
SELECT ST_Value(rast, 3, 30, 17) FROM o_3_loadedrast WHERE rid = 1;
