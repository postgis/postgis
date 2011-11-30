--
-- A user callback function that nullifies all cells in the resulting raster.
--
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
