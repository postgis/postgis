SELECT srid, scale_x::numeric(16, 10), scale_y::numeric(16, 10), blocksize_x, blocksize_y, same_alignment, regular_blocking, num_bands, pixel_types, nodata_values::numeric(16,10)[], out_db, ST_AsEWKT(extent) FROM raster_columns WHERE r_table_name = 'loadedrast' AND r_raster_column = 'rast';
SELECT ST_BandPixelType(rast, 1), ST_Value(rast, 1, 1, 1) FROM loadedrast WHERE rid = 1;
SELECT ST_BandPixelType(rast, 2), ST_Value(rast, 2, 90, 50) FROM loadedrast WHERE rid = 1;
SELECT ST_BandPixelType(rast, 3), ST_Value(rast, 3, 45, 25) FROM loadedrast WHERE rid = 1;
