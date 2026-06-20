SELECT srid, num_bands, pixel_types, nodata_values::numeric(16,10)[], out_db FROM raster_columns WHERE r_table_name = 'loadedrast' AND r_raster_column = 'rast';
SELECT ST_BandPixelType(rast, 1),
       ST_BandNoDataValue(rast, 1),
       ST_Value(rast, 1, 1, 1), ST_Value(rast, 1, 1, 1, FALSE),
       ST_Value(rast, 1, 2, 1), ST_Value(rast, 1, 2, 1, FALSE),
       ST_Value(rast, 1, 3, 1), ST_Value(rast, 1, 3, 1, FALSE),
       ST_Value(rast, 1, 4, 1), ST_Value(rast, 1, 4, 1, FALSE)
FROM loadedrast WHERE rid = 1;
