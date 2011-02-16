-- Test NULL raster
SELECT ST_MapAlgebra(NULL, 1, 'rast + 20', '2') IS NULL FROM ST_TestRaster(0, 0, -1) rast;

-- Test empty raster
SELECT ST_IsEmpty(ST_MapAlgebra(ST_MakeEmptyRaster(0, 10, 0, 0, 1, 1, 1, 1, -1), 1, 'rast + 20', '2'));

-- Test hasnoband raster
SELECT ST_HasNoBand(ST_MapAlgebra(ST_MakeEmptyRaster(10, 10, 0, 0, 1, 1, 1, 1, -1), 1, 'rast + 20', '2'));

-- Test hasnodata value
SELECT ST_Value(rast, 1, 1), ST_Value(ST_MapAlgebra(ST_SetBandNoDataValue(rast, NULL), 1, 'rast + 20', '2'), 1, 1) FROM ST_TestRaster(0, 0, -1) rast;

-- Test nodata value
SELECT ST_Value(rast, 1, 1), ST_Value(ST_MapAlgebra(rast, 1, 'rast + 20', 'rast + 2'), 1, 1) FROM ST_TestRaster(0, 0, -1) rast;

-- Test 'rast' expression
SELECT ST_Value(rast, 1, 1), ST_Value(ST_MapAlgebra(rast, 1, 'rast', 'rast'), 1, 1) FROM ST_TestRaster(0, 0, -1) rast;
SELECT ST_Value(rast, 1, 1), ST_Value(ST_MapAlgebra(ST_SetBandNoDataValue(rast, NULL), 1, 'rast', NULL), 1, 1) FROM ST_TestRaster(0, 0, -1) rast;

-- Test pixeltype
SELECT ST_Value(rast, 1, 1), ST_Value(ST_MapAlgebra(rast, 1, 'rast + 20', 'rast + 2', '4BUI'), 1, 1) FROM ST_TestRaster(0, 0, 100) rast;
SELECT ST_Value(rast, 1, 1), ST_Value(ST_MapAlgebra(rast, 1, 'rast + 20', 'rast + 2', '4BUId'), 1, 1) FROM ST_TestRaster(0, 0, 100) rast;
SELECT ST_Value(rast, 1, 1), ST_Value(ST_MapAlgebra(rast, 1, 'rast + 20', 'rast + 2', '2BUI'), 1, 1) FROM ST_TestRaster(0, 0, 101) rast;
