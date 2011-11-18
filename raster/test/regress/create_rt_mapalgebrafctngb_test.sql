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
    LANGUAGE 'plpgsql' IMMUTABLE;

CREATE OR REPLACE FUNCTION ST_Nullage(matrix float[][], nodatamode text, VARIADIC args text[])
    RETURNS float AS
    $$
    BEGIN
        RETURN NULL;
    END;
    $$
    LANGUAGE 'plpgsql' IMMUTABLE;


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
