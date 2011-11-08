-- Test NULL raster
SELECT ST_MapAlgebraExpr(NULL, 1, NULL, 'rast + 20', 2) IS NULL FROM ST_TestRaster(0, 0, -1) rast;

-- Test empty raster
SELECT ST_IsEmpty(ST_MapAlgebraExpr(ST_MakeEmptyRaster(0, 10, 0, 0, 1, 1, 1, 1, -1), 1, NULL, 'rast + 20', 2));

-- Test hasnoband raster
SELECT ST_HasNoBand(ST_MapAlgebraExpr(ST_MakeEmptyRaster(10, 10, 0, 0, 1, 1, 1, 1, -1), 1, NULL, 'rast + 20', 2));

-- Test hasnodata value
SELECT ST_Value(rast, 1, 1), ST_Value(ST_MapAlgebraExpr(ST_SetBandNoDataValue(rast, NULL), 1, NULL, 'rast + 20', 2), 1, 1) FROM ST_TestRaster(0, 0, -1) rast;

-- Test nodata value
SELECT ST_Value(rast, 1, 1), ST_Value(ST_MapAlgebraExpr(rast, 1, NULL, 'rast + 20', 2), 1, 1) FROM ST_TestRaster(0, 0, -1) rast;

-- Test 'rast' expression
SELECT ST_Value(rast, 1, 1), ST_Value(ST_MapAlgebraExpr(rast, 1, NULL, 'rast', 2), 1, 1) FROM ST_TestRaster(0, 0, -1) rast;
SELECT ST_Value(rast, 1, 1), ST_Value(ST_MapAlgebraExpr(ST_SetBandNoDataValue(rast, NULL), 1, NULL, 'rast'), 1, 1) FROM ST_TestRaster(0, 0, -1) rast;

-- Test pixeltype
SELECT ST_Value(rast, 1, 1), ST_Value(ST_MapAlgebraExpr(rast, 1, '4BUI', 'rast + 20', 2), 1, 1) FROM ST_TestRaster(0, 0, 100) rast;
SELECT ST_Value(rast, 1, 1), ST_Value(ST_MapAlgebraExpr(rast, 1, '4BUId', 'rast + 20', 2), 1, 1) FROM ST_TestRaster(0, 0, 100) rast;
SELECT ST_Value(rast, 1, 1), ST_Value(ST_MapAlgebraExpr(rast, 1, '2BUI', 'rast + 20', 2), 1, 1) FROM ST_TestRaster(0, 0, 101) rast;
