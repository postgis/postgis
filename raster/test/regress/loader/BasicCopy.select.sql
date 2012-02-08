SELECT srid, scale_x::numeric(16, 10), scale_y::numeric(16, 10), blocksize_x, blocksize_y, same_alignment, regular_blocking, num_bands, pixel_types, nodata_values::numeric(16,10)[], out_db, extent FROM raster_columns WHERE r_table_name = 'loadedrast' AND r_raster_column = 'the_rast';
SELECT geom, val FROM (SELECT (ST_DumpAsPolygons(the_rast, 1)).* FROM loadedrast) foo;
SELECT geom, val FROM (SELECT (ST_DumpAsPolygons(the_rast, 2)).* FROM loadedrast) foo;
SELECT geom, val FROM (SELECT (ST_DumpAsPolygons(the_rast, 3)).* FROM loadedrast) foo;
