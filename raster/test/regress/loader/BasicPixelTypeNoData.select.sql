SELECT srid, num_bands, pixel_types, nodata_values::numeric(16,10)[], out_db FROM raster_columns WHERE r_table_name = 'loadedrast' AND r_raster_column = 'rast';
SELECT ST_BandPixelType(rast, 1), ST_BandNoDataValue(rast, 1), ST_Value(rast, 1, 1, 1), ST_Value(rast, 1, 1, 1, FALSE) FROM loadedrast WHERE rid = 1;
SELECT ST_BandPixelType(rast, 2), ST_BandNoDataValue(rast, 2), ST_Value(rast, 2, 11, 1), ST_Value(rast, 2, 11, 1, FALSE) FROM loadedrast WHERE rid = 1;
SELECT ST_BandPixelType(rast, 3), ST_BandNoDataValue(rast, 3), ST_Value(rast, 3, 11, 1), ST_Value(rast, 3, 11, 1, FALSE) FROM loadedrast WHERE rid = 1;
