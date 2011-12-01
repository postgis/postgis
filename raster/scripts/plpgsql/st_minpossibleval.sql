-----------------------------------------------------------------------
-- ST_MinPossibleVal
-- Return the smallest value for a given pixeltyp. 
-- Should be called like this:
-- SELECT ST_MinPossibleVal(ST_BandPixelType(rast, band))
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION ST_MinPossibleVal(pixeltype text)
    RETURNS float8 AS
    $$
    DECLARE
        newval int := 0;
    BEGIN
        newval := CASE 
            WHEN pixeltype = '1BB' THEN 0
            WHEN pixeltype = '2BUI' THEN 0
            WHEN pixeltype = '4BUI' THEN 0
            WHEN pixeltype = '8BUI' THEN 0
            WHEN pixeltype = '8BSI' THEN -128
            WHEN pixeltype = '16BUI' THEN 0
            WHEN pixeltype = '16BSI' THEN -32768
            WHEN pixeltype = '32BUI' THEN 0
            WHEN pixeltype = '32BSI' THEN -2147483648
            WHEN pixeltype = '32BF' THEN -2147483648 -- Could not find a function returning the smallest real yet
            WHEN pixeltype = '64BF' THEN -2147483648 -- Could not find a function returning the smallest float8 yet
        END;
        RETURN newval;
    END;
    $$
    LANGUAGE 'plpgsql';
