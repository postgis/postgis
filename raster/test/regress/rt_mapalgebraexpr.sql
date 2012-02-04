-- Test NULL raster
SELECT ST_MapAlgebraExpr(NULL, 1, NULL, '[rast] + 20', 2) IS NULL FROM ST_TestRaster(0, 0, -1) rast;

-- Test empty raster
SELECT 'T1', ST_IsEmpty(ST_MapAlgebraExpr(ST_MakeEmptyRaster(0, 10, 0, 0, 1, 1, 1, 1, -1), 1, NULL, '[rast] + 20', 2));

-- Test hasnoband raster
SELECT 'T2', ST_HasNoBand(ST_MapAlgebraExpr(ST_MakeEmptyRaster(10, 10, 0, 0, 1, 1, 1, 1, -1), 1, NULL, '[rast] + 20', 2));

-- Test hasnodata value
SELECT 'T3', ST_Value(rast, 1, 1), ST_Value(ST_MapAlgebraExpr(ST_SetBandNoDataValue(rast, NULL), 1, NULL, '[rast] + 20', 2), 1, 1) FROM ST_TestRaster(0, 0, -1) rast;

-- Test nodata value
SELECT 'T4', ST_Value(rast, 1, 1), ST_Value(ST_MapAlgebraExpr(rast, 1, NULL, '[rast] + 20', 2), 1, 1) FROM ST_TestRaster(0, 0, -1) rast;

-- Test 'rast' expression
SELECT 'T5', ST_Value(rast, 1, 1), ST_Value(ST_MapAlgebraExpr(rast, 1, NULL, '[rast]', 2), 1, 1) FROM ST_TestRaster(0, 0, -1) rast;
SELECT 'T6', ST_Value(rast, 1, 1), ST_Value(ST_MapAlgebraExpr(ST_SetBandNoDataValue(rast, NULL), 1, NULL, '[rast]'), 1, 1) FROM ST_TestRaster(0, 0, -1) rast;

-- Test pixeltype
SELECT 'T7', ST_Value(rast, 1, 1), ST_Value(ST_MapAlgebraExpr(rast, 1, '4BUI', '[rast] + 20', 2), 1, 1) FROM ST_TestRaster(0, 0, 100) rast;
SELECT 'T8', ST_Value(rast, 1, 1), ST_Value(ST_MapAlgebraExpr(rast, 1, '4BUId', '[rast] + 20', 2), 1, 1) FROM ST_TestRaster(0, 0, 100) rast;
SELECT 'T9', ST_Value(rast, 1, 1), ST_Value(ST_MapAlgebraExpr(rast, 1, '2BUI', '[rast] + 20', 2), 1, 1) FROM ST_TestRaster(0, 0, 101) rast;

-- Test '[rast.x]', '[rast.y]' and '[rast.val]' substitutions expression
SELECT 'T10.'||x||'.'||y, ST_Value(rast, x, y),
  ST_Value(ST_MapAlgebraExpr(rast, 1, NULL, 'round([rast]*[rast.x]/[rast.y]+[rast.val])'), x, y)
FROM ST_TestRaster(0, 0, 10) rast,
     generate_series(6, 8) as x,
     generate_series(2, 4) as y
ORDER BY x, y;

-- Test evaluations to null (see #1523)
WITH op AS ( select rast AS rin,
  ST_MapAlgebraExpr(rast, 1, NULL, 'g from (select NULL as g) as foo', 2)
  AS rout FROM ST_TestRaster(0, 0, 10) rast )
SELECT 'T11.1', ST_Value(rin, 1, 1), ST_Value(rout, 1, 1) FROM op;
WITH op AS ( select rast AS rin,
  ST_MapAlgebraExpr(rast, 1, NULL, 'g from (select [rast],NULL as g) as foo', 2)
  AS rout FROM ST_TestRaster(0, 0, 10) rast )
SELECT 'T11.2', ST_Value(rin, 1, 1), ST_Value(rout, 1, 1) FROM op;
