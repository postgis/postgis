-- Test NULL raster
SELECT ST_MapAlgebraFct(NULL, 1, NULL, 'raster_plus_twenty(float, text[])'::regprocedure) IS NULL FROM ST_TestRaster(0, 0, -1) rast;
SELECT ST_MapAlgebraFct(NULL, 1, NULL, 'raster_plus_twenty(float, text[])'::regprocedure, NULL) IS NULL FROM ST_TestRaster(0, 0, -1) rast;

-- Test empty raster
SELECT ST_IsEmpty(ST_MapAlgebraFct(ST_MakeEmptyRaster(0, 10, 0, 0, 1, 1, 1, 1, -1), 1, NULL, 'raster_plus_twenty(float, text[])'::regprocedure));
SELECT ST_IsEmpty(ST_MapAlgebraFct(ST_MakeEmptyRaster(0, 10, 0, 0, 1, 1, 1, 1, -1), 1, NULL, 'raster_plus_twenty(float, text[])'::regprocedure, NULL));

-- Test hasnoband raster
SELECT ST_HasNoBand(ST_MapAlgebraFct(ST_MakeEmptyRaster(10, 10, 0, 0, 1, 1, 1, 1, -1), 1, NULL, 'raster_plus_twenty(float, text[])'::regprocedure));
SELECT ST_HasNoBand(ST_MapAlgebraFct(ST_MakeEmptyRaster(10, 10, 0, 0, 1, 1, 1, 1, -1), 1, NULL, 'raster_plus_twenty(float, text[])'::regprocedure, NULL));

-- Test hasnodata value
SELECT ST_Value(rast, 1, 1), ST_Value(ST_MapAlgebraFct(ST_SetBandNoDataValue(rast, NULL), 1, NULL, 'raster_plus_twenty(float, text[])'::regprocedure), 1, 1) FROM ST_TestRaster(0, 0, -1) rast;
SELECT ST_Value(rast, 1, 1), ST_Value(ST_MapAlgebraFct(ST_SetBandNoDataValue(rast, NULL), 1, NULL, 'raster_plus_twenty(float, text[])'::regprocedure, NULL), 1, 1) FROM ST_TestRaster(0, 0, -1) rast;

-- Test user function
SELECT ST_Value(rast, 1, 1), ST_Value(ST_MapAlgebraFct(rast, 1, NULL, 'raster_plus_twenty(float, text[])'::regprocedure), 1, 1) FROM ST_TestRaster(0, 0, -1) rast;
SELECT ST_Value(rast, 1, 1), ST_Value(ST_MapAlgebraFct(ST_SetBandNoDataValue(rast, NULL), 1, NULL, 'raster_plus_twenty(float, text[])'::regprocedure, NULL), 1, 1) FROM ST_TestRaster(0, 0, -1) rast;

-- Test pixeltype
SELECT ST_Value(rast, 1, 1), ST_Value(ST_MapAlgebraFct(rast, 1, '4BUI', 'raster_plus_twenty(float, text[])'::regprocedure), 1, 1) FROM ST_TestRaster(0, 0, 100) rast;
SELECT ST_Value(rast, 1, 1), ST_Value(ST_MapAlgebraFct(rast, 1, '4BUId', 'raster_plus_twenty(float, text[])'::regprocedure), 1, 1) FROM ST_TestRaster(0, 0, 100) rast;
SELECT ST_Value(rast, 1, 1), ST_Value(ST_MapAlgebraFct(rast, 1, '2BUI', 'raster_plus_twenty(float, text[])'::regprocedure), 1, 1) FROM ST_TestRaster(0, 0, 101) rast;

-- Test user callbacks
SELECT ST_Value(rast, 1, 1) + 13, ST_Value(ST_MapAlgebraFct(rast, 1, NULL, 'raster_plus_arg1(float, text[])'::regprocedure, '13'), 1, 1) FROM ST_TestRaster(0, 0, 200) AS rast;

SELECT ST_Value(rast, 1, 1) * 21 + 14, ST_Value(ST_MapAlgebraFct(rast, 1, NULL, 'raster_polynomial(float, text[])'::regprocedure, '21', '14'), 1, 1) FROM ST_TestRaster(0, 0, 300) AS rast;
