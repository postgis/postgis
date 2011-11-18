--
-- A simple 'callback' user function that sums up all the values in a neighborhood.
--
CREATE OR REPLACE FUNCTION ST_Sum(matrix float[][], nodatamode text, variadic args text[])
    RETURNS float AS
    $$
    DECLARE
        _matrix float[][];
        x1 integer;
        x2 integer;
        y1 integer;
        y2 integer;
        sum float;
    BEGIN
        _matrix := matrix;
        sum := 0;
        FOR x in array_lower(matrix, 1)..array_upper(matrix, 1) LOOP
            FOR y in array_lower(matrix, 2)..array_upper(matrix, 2) LOOP
                IF _matrix[x][y] IS NULL THEN
                    IF nodatamode = 'ignore' THEN
                        _matrix[x][y] := 0;
                    ELSE
                        _matrix[x][y] := nodatamode::float;
                    END IF;
                END IF;
                sum := sum + _matrix[x][y];
            END LOOP;
        END LOOP;
        RETURN sum;
    END;
    $$
    LANGUAGE 'plpgsql';

--
--Test rasters
--
CREATE OR REPLACE FUNCTION ST_TestRasterNgb(h integer, w integer, val float8) 
    RETURNS raster AS 
    $$
    DECLARE
    BEGIN
        RETURN ST_AddBand(ST_MakeEmptyRaster(h, w, 0, 0, 1, 1, 0, 0, -1), '32BF', val, -1);
    END;
    $$
    LANGUAGE 'plpgsql';

-- Tests
-- Test NULL Raster. Should be true.
SELECT ST_MapAlgebraFctNgb(NULL, 1, NULL, 1, 1, 'ST_Sum(float[][], text, text[])'::regprocedure, 'NULL', NULL) IS NULL FROM ST_TestRasterNgb(0, 0, -1) rast;

-- Test empty Raster. Should be true.
SELECT ST_IsEmpty(ST_MapAlgebraFctNgb(ST_MakeEmptyRaster(0, 10, 0, 0, 1, 1, 1, 1, -1), 1, NULL, 1, 1, 'ST_Sum(float[][], text, text[])'::regprocedure, 'NULL', NULL));

-- Test has no band raster. Should be true
SELECT ST_HasNoBand(ST_MapAlgebraFctNgb(ST_MakeEmptyRaster(10, 10, 0, 0, 1, 1, 1, 1, -1), 1, NULL, 1, 1, 'ST_Sum(float[][], text, text[])'::regprocedure, 'NULL', NULL));

-- Test has no nodata value. Should return null and 7.
SELECT 
  ST_Value(rast, 2, 2) IS NULL, 
  ST_Value(
    ST_MapAlgebraFctNgb(
      ST_SetBandNoDataValue(rast, NULL), 1, NULL, 1, 1, 'ST_Sum(float[][], text, text[])'::regprocedure, 'NULL', NULL
    ), 2, 2) = 7
 FROM ST_SetValue(ST_TestRasterNgb(3, 3, 1), 2, 2, NULL) AS rast;

---- Test NULL nodatamode. Should return null and null.
SELECT 
  ST_Value(rast, 2, 2) IS NULL, 
  ST_Value(
    ST_MapAlgebraFctNgb(rast, 1, NULL, 1, 1, 'ST_Sum(float[][], text, text[])'::regprocedure, 'NULL', NULL), 2, 2
  ) IS NULL
 FROM ST_SetValue(ST_TestRasterNgb(3, 3, 1), 2, 2, NULL) AS rast;

---- Test ignore nodatamode. Should return null and 8.
SELECT 
  ST_Value(rast, 2, 2) IS NULL, 
  ST_Value(
    ST_MapAlgebraFctNgb(rast, 1, NULL, 1, 1, 'ST_Sum(float[][], text, text[])'::regprocedure, 'ignore', NULL), 2, 2
  ) = 8
 FROM ST_SetValue(ST_TestRasterNgb(3, 3, 1), 2, 2, NULL) AS rast;

---- Test value nodatamode. Should return null and null.
SELECT 
  ST_Value(rast, 2, 2) IS NULL, 
  ST_Value(
    ST_MapAlgebraFctNgb(rast, 1, NULL, 1, 1, 'ST_Sum(float[][], text, text[])'::regprocedure, 'value', NULL), 2, 2
  ) IS NULL 
 FROM ST_SetValue(ST_TestRasterNgb(3, 3, 1), 2, 2, NULL) AS rast;

-- Test value nodatamode. Should return null and 9.
SELECT 
  ST_Value(rast, 1, 1) IS NULL, 
  ST_Value(
    ST_MapAlgebraFctNgb(rast, 1, NULL, 1, 1, 'ST_Sum(float[][], text, text[])'::regprocedure, 'value', NULL), 2, 2
  ) = 9
 FROM ST_SetValue(ST_TestRasterNgb(3, 3, 1), 1, 1, NULL) AS rast;

-- Test value nodatamode. Should return null and 0.
SELECT 
  ST_Value(rast, 2, 2) IS NULL, 
  ST_Value(
    ST_MapAlgebraFctNgb(rast, 1, NULL, 1, 1, 'ST_Sum(float[][], text, text[])'::regprocedure, '-8', NULL), 2, 2
  ) = 0
 FROM ST_SetValue(ST_TestRasterNgb(3, 3, 1), 2, 2, NULL) AS rast;

-- Test ST_Sum user function. Should be 1 and 9.
SELECT 
  ST_Value(rast, 2, 2) = 1, 
  ST_Value(
    ST_MapAlgebraFctNgb(rast, 1, NULL, 1, 1, 'ST_Sum(float[][], text, text[])'::regprocedure, 'NULL', NULL), 2, 2
  ) = 9
 FROM ST_TestRasterNgb(3, 3, 1) AS rast;

-- Test ST_Sum user function on a no nodata value raster. Should be null and -1.
SELECT 
  ST_Value(rast, 2, 2) IS NULL, 
  ST_Value(
    ST_MapAlgebraFctNgb(ST_SetBandNoDataValue(rast, NULL), 1, NULL, 1, 1, 'ST_Sum(float[][], text, text[])'::regprocedure, 'NULL', NULL), 2, 2
  ) = -1
 FROM ST_SetValue(ST_TestRasterNgb(3, 3, 0), 2, 2, NULL) AS rast;

-- Test pixeltype 1. Should return 2 and 15.
SELECT
  ST_Value(rast, 2, 2) = 2,
  ST_Value(
    ST_MapAlgebraFctNgb(rast, 1, '4BUI', 1, 1, 'ST_Sum(float[][], text, text[])'::regprocedure, 'NULL', NULL), 2, 2
  ) = 15
 FROM ST_SetBandNoDataValue(ST_TestRasterNgb(3, 3, 2), 1, NULL) AS rast;

-- Test pixeltype 1. No error, changed to 32BF
SELECT 
  ST_Value(rast, 2, 2) = 2, 
  ST_Value(
    ST_MapAlgebraFctNgb(rast, 1, '4BUId', 1, 1, 'ST_Sum(float[][], text, text[])'::regprocedure, 'NULL', NULL), 2, 2
  ) = 18
 FROM ST_TestRasterNgb(3, 3, 2) AS rast;

-- Test pixeltype 1. Should return 1 and 3.
SELECT 
  ST_Value(rast, 2, 2) = 1, 
  ST_Value(
    ST_MapAlgebraFctNgb(rast, 1, '2BUI', 1, 1, 'ST_Sum(float[][], text, text[])'::regprocedure, 'NULL', NULL), 2, 2
  ) = 3
 FROM ST_SetBandNoDataValue(ST_TestRasterNgb(3, 3, 1), 1, NULL) AS rast;

-- Test that the neighborhood function leaves a border of NODATA
SELECT
  COUNT(*) = 1
 FROM (SELECT
    (ST_DumpAsPolygons(
      ST_MapAlgebraFctNgb(rast, 1, NULL, 1, 1, 'ST_Sum(float[][], text, text[])'::regprocedure, 'NULL', NULL)
    )).*
   FROM ST_TestRasterNgb(5, 5, 1) AS rast) AS foo;

-- Test that the neighborhood function leaves a border of NODATA
SELECT
  ST_Area(geom) = 8, val = 9
 FROM (SELECT
    (ST_DumpAsPolygons(
      ST_MapAlgebraFctNgb(rast, 1, NULL, 1, 1, 'ST_Sum(float[][], text, text[])'::regprocedure, 'NULL', NULL)
    )).*
   FROM ST_SetValue(ST_TestRasterNgb(5, 5, 1), 1, 1, NULL) AS rast) AS foo;

-- Test that the neighborhood function leaves a border of NODATA
-- plus a corner where one cell has a value of 8.
SELECT
  (ST_Area(geom) = 1 AND val = 8) OR (ST_Area(geom) = 8 AND val = 9)
 FROM (SELECT
    (ST_DumpAsPolygons(
      ST_MapAlgebraFctNgb(rast, 1, NULL, 1, 1, 'ST_Sum(float[][], text, text[])'::regprocedure, 'ignore', NULL)
    )).*
   FROM ST_SetValue(ST_TestRasterNgb(5, 5, 1), 1, 1, NULL) AS rast) AS foo;

-- Test that the neighborhood function leaves a border of NODATA
-- plus a hole where 9 cells have NODATA
-- This results in a donut: a polygon with a hole. The polygon has
-- an area of 16, with a hole that has an area of 9
SELECT
  ST_NRings(geom) = 2,
  ST_NumInteriorRings(geom) = 1,
  ST_Area(geom) = 16, 
  val = 9, 
  ST_Area(ST_BuildArea(ST_InteriorRingN(geom, 1))) = 9
 FROM (SELECT
    (ST_DumpAsPolygons(
      ST_MapAlgebraFctNgb(rast, 1, NULL, 1, 1, 'ST_Sum(float[][], text, text[])'::regprocedure, 'NULL', NULL)
    )).*
   FROM ST_SetValue(ST_TestRasterNgb(7, 7, 1), 4, 4, NULL) AS rast) AS foo;

-- Test that the neighborhood function leaves a border of NODATA,
-- and the center pyramids when summed twice, ignoring NODATA values
SELECT
  COUNT(*) = 9, SUM(ST_Area(geom)) = 9, SUM(val) = ((36+54+36) + (54+81+54) + (36+54+36)) 
 FROM (SELECT
    (ST_DumpAsPolygons(
      ST_MapAlgebraFctNgb(
        ST_MapAlgebraFctNgb(rast, 1, NULL, 1, 1, 'ST_Sum(float[][], text, text[])'::regprocedure, 'ignore', NULL), 1, NULL, 1, 1, 'ST_Sum(float[][], text, text[])'::regprocedure, 'ignore', NULL
      )
    )).*
   FROM ST_TestRasterNgb(5, 5, 1) AS rast) AS foo;

-- Test that the neighborhood function leaves a border of NODATA,
-- and the center contains one cel when summed twice, replacing NULL with NODATA values
SELECT
  COUNT(*) = 1, SUM(ST_Area(geom)) = 1, SUM(val) = 81
 FROM (SELECT
    (ST_DumpAsPolygons(
      ST_MapAlgebraFctNgb(
        ST_MapAlgebraFctNgb(rast, 1, NULL, 1, 1, 'ST_Sum(float[][], text, text[])'::regprocedure, 'NULL', NULL), 1, NULL, 1, 1, 'ST_Sum(float[][], text, text[])'::regprocedure, 'NULL', NULL
      )
    )).*
   FROM ST_TestRasterNgb(5, 5, 1) AS rast) AS foo;

