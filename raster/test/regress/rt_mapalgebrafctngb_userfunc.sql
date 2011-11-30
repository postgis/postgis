-- test st_max4ma, uniform values
SELECT
  ST_Value(rast, 2, 2) = 1,
  ST_Value(
    ST_MapAlgebraFctNgb(rast, 1, NULL, 1, 1, 'st_max4ma(float[][], text, text[])'::regprocedure, NULL, NULL), 2, 2
  ) = 1
  FROM ST_TestRasterNgb(3, 3, 1) AS rast;

-- test st_max4ma, obvious max value
SELECT
  ST_Value(rast, 2, 2) = 1,
  ST_Value(
    ST_MapAlgebraFctNgb(rast, 1, NULL, 1, 1, 'st_max4ma(float[][], text, text[])'::regprocedure, NULL, NULL), 2, 2
  ) = 11
  FROM ST_SetValue(ST_TestRasterNgb(3, 3, 1), 1, 1, 11) AS rast;

-- test st_max4ma, value substitution
SELECT
  ST_Value(rast, 2, 2) = 1,
  ST_Value(
    ST_MapAlgebraFctNgb(rast, 1, NULL, 1, 1, 'st_max4ma(float[][], text, text[])'::regprocedure, '22', NULL), 2, 2
  ) = 22 
  FROM ST_SetValue(ST_TestRasterNgb(3, 3, 1), 3, 3, NULL) AS rast;

-- test st_min4ma, uniform values
SELECT
  ST_Value(rast, 2, 2) = 1,
  ST_Value(
    ST_MapAlgebraFctNgb(rast, 1, NULL, 1, 1, 'st_min4ma(float[][], text, text[])'::regprocedure, NULL, NULL), 2, 2
  ) = 1
  FROM ST_TestRasterNgb(3, 3, 1) AS rast;

-- test st_min4ma, obvious min value
SELECT
  ST_Value(rast, 2, 2) = 11,
  ST_Value(
    ST_MapAlgebraFctNgb(rast, 1, NULL, 1, 1, 'st_min4ma(float[][], text, text[])'::regprocedure, NULL, NULL), 2, 2
  ) = 1
  FROM ST_SetValue(ST_TestRasterNgb(3, 3, 11), 1, 1, 1) AS rast;

-- test st_min4ma, value substitution
SELECT
  ST_Value(rast, 2, 2) = 22,
  ST_Value(
    ST_MapAlgebraFctNgb(rast, 1, NULL, 1, 1, 'st_min4ma(float[][], text, text[])'::regprocedure, '2', NULL), 2, 2
  ) = 2 
  FROM ST_SetValue(ST_TestRasterNgb(3, 3, 22), 3, 3, NULL) AS rast;

-- test st_sum4ma happens in rt_mapalgebrafctngb.sql

-- test st_mean4ma, uniform values
SELECT
  ST_Value(rast, 2, 2) = 1,
  ST_Value(
    ST_MapAlgebraFctNgb(rast, 1, NULL, 1, 1, 'st_mean4ma(float[][], text, text[])'::regprocedure, NULL, NULL), 2, 2
  ) = 1
  FROM ST_TestRasterNgb(3, 3, 1) AS rast;

-- test st_mean4ma, obvious min value
SELECT
  ST_Value(rast, 2, 2) = 10,
  ST_Value(
    ST_MapAlgebraFctNgb(rast, 1, NULL, 1, 1, 'st_mean4ma(float[][], text, text[])'::regprocedure, NULL, NULL), 2, 2
  ) = 9
  FROM ST_SetValue(ST_TestRasterNgb(3, 3, 10), 1, 1, 1) AS rast;

-- test st_mean4ma, value substitution
SELECT
  ST_Value(rast, 2, 2) = 10,
  ST_Value(
    ST_MapAlgebraFctNgb(rast, 1, NULL, 1, 1, 'st_mean4ma(float[][], text, text[])'::regprocedure, '1', NULL), 2, 2
  ) = 9 
  FROM ST_SetValue(ST_TestRasterNgb(3, 3, 10), 3, 3, NULL) AS rast;

-- test st_range4ma, uniform values
SELECT
  ST_Value(rast, 2, 2) = 1,
  ST_Value(
    ST_MapAlgebraFctNgb(rast, 1, NULL, 1, 1, 'st_range4ma(float[][], text, text[])'::regprocedure, NULL, NULL), 2, 2
  ) = 0
  FROM ST_TestRasterNgb(3, 3, 1) AS rast;

-- test st_range4ma, obvious min value
SELECT
  ST_Value(rast, 2, 2) = 10,
  ST_Value(
    ST_MapAlgebraFctNgb(rast, 1, NULL, 1, 1, 'st_range4ma(float[][], text, text[])'::regprocedure, NULL, NULL), 2, 2
  ) = 9
  FROM ST_SetValue(ST_TestRasterNgb(3, 3, 10), 1, 1, 1) AS rast;

-- test st_range4ma, value substitution
SELECT
  ST_Value(rast, 2, 2) = 10,
  ST_Value(
    ST_MapAlgebraFctNgb(rast, 1, NULL, 1, 1, 'st_range4ma(float[][], text, text[])'::regprocedure, '1', NULL), 2, 2
  ) = 9 
  FROM ST_SetValue(ST_TestRasterNgb(3, 3, 10), 3, 3, NULL) AS rast;

-- test st_slope, flat plane
SELECT
  ST_Value(rast, 2, 2) = 1,
  ST_Value(
    ST_Slope(rast, 1, NULL), 2, 2
  ) = 0
  FROM ST_TestRasterNgb(3, 3, 1) AS rast;

-- test st_slope, corner spike
SELECT
  ST_Value(rast, 2, 2) = 1,
  round(degrees(ST_Value(
    ST_Slope(rast, 1, NULL), 2, 2
  ))*100000) = 5784902
  FROM ST_SetValue(ST_TestRasterNgb(3, 3, 1), 1, 1, 10) AS rast;

-- test st_slope, corner droop
SELECT
  ST_Value(rast, 2, 2) = 10,
  round(degrees(ST_Value(
    ST_Slope(rast, 1, NULL), 2, 2
  ))*100000) = 5784902
  FROM ST_SetValue(ST_TestRasterNgb(3, 3, 10), 1, 1, 1) AS rast;

-- test st_aspect, flat plane
SELECT
  ST_Value(rast, 2, 2) = 1,
  round(degrees(ST_Value(
    ST_Aspect(rast, 1, NULL), 2, 2
  ))*10000) = 2700000
  FROM ST_TestRasterNgb(3, 3, 1) AS rast;

-- test st_aspect, North
SELECT
  ST_Value(rast, 2, 2) = 1,
  round(degrees(ST_Value(
    ST_Aspect(rast, 1, NULL), 2, 2
  ))*10000) = 0
  FROM ST_SetValue(ST_TestRasterNgb(3, 3, 1), 2, 1, 0) AS rast;

-- test st_aspect, South
SELECT
  ST_Value(rast, 2, 2) = 1,
  round(degrees(ST_Value(
    ST_Aspect(rast, 1, NULL), 2, 2
  ))*10000) = 1800000
  FROM ST_SetValue(ST_TestRasterNgb(3, 3, 1), 2, 3, 0) AS rast;

-- test st_aspect, East
SELECT
  ST_Value(rast, 2, 2) = 1,
  round(degrees(ST_Value(
    ST_Aspect(rast, 1, NULL), 2, 2
  ))*10000) = 900000
  FROM ST_SetValue(ST_TestRasterNgb(3, 3, 1), 3, 2, 0) AS rast;

-- test st_aspect, West
SELECT
  ST_Value(rast, 2, 2) = 1,
  round(degrees(ST_Value(
    ST_Aspect(rast, 1, NULL), 2, 2
  ))*10000) = 2700000
  FROM ST_SetValue(ST_TestRasterNgb(3, 3, 1), 1, 2, 0) AS rast;
