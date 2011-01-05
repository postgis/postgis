----------------------------------------------------------------------
--
-- $Id$
--
-- Copyright (c) 2009-2010 Pierre Racine <pierre.racine@sbf.ulaval.ca>
--
----------------------------------------------------------------------

-- NOTE: The ST_Clip function found in this file still many improvements before being implemented in C.

CREATE OR REPLACE FUNCTION ST_Clip(rast raster, x int, y int, width int, height int) 
    RETURNS raster AS 
    $$
    DECLARE
        newrast raster := ST_MakeEmptyRaster(width, height, ST_UpperLeftX(rast), ST_UpperLeftY(rast), 
                          ST_ScaleX(rast), ST_ScaleY(rast), ST_SkewX(rast), ST_SkewY(rast), ST_SRID(rast));
        numband int := ST_Numbands(rast);
    band int;
    cx int;
    cy int;
    newwidth int := CASE WHEN x + width > ST_Width(rast) THEN ST_Width(rast) - x ELSE width END;
    newheight int := CASE WHEN y + height > ST_Height(rast) THEN ST_Height(rast) - y ELSE height END;
    BEGIN
        FOR b IN 1..numband LOOP
            newrast := ST_AddBand(newrast, ST_PixelType(rast, band), ST_NodataValue(rast, band), ST_NodataValue(rast, band));
            FOR cx IN 1..newwidth LOOP
                FOR cy IN 1..newheight LOOP
                    newrast := ST_SetValue(newrast, band, cx, cy, ST_Value(rast, band, cx + x - 1, cy + y - 1));
                END LOOP;
            END LOOP;
        END LOOP;
    RETURN newrast;
    END;
    $$
    LANGUAGE 'plpgsql';


