DROP FUNCTION ST_TestRaster(ulx float8, uly float8, val float8);
DROP FUNCTION raster_plus_twenty(pixel FLOAT, VARIADIC args TEXT[]);
DROP FUNCTION raster_plus_arg1(pixel FLOAT, VARIADIC args TEXT[]);
DROP FUNCTION raster_polynomial(pixel FLOAT, VARIADIC args TEXT[]);
DROP FUNCTION raster_nullage(pixel FLOAT, VARIADIC args TEXT[]);
DROP FUNCTION raster_x_plus_arg(pixel FLOAT, pos INT[], VARIADIC args TEXT[]);
DROP FUNCTION raster_y_plus_arg(pixel FLOAT, pos INT[], VARIADIC args TEXT[]);
