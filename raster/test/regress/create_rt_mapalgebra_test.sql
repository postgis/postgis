CREATE OR REPLACE FUNCTION ST_TestRaster(ulx float8, uly float8, val float8) 
    RETURNS raster AS 
    $$
    DECLARE
    BEGIN
        RETURN ST_AddBand(ST_MakeEmptyRaster(10, 10, ulx, uly, 1, 1, 0, 0, -1), '32BF', val, -1);
    END;
    $$
    LANGUAGE 'plpgsql';


