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

CREATE OR REPLACE FUNCTION ST_Clip(rast raster, band int, geom geometry, nodata float8 DEFAULT null, trimraster boolean DEFAULT false) 
    RETURNS raster AS 
    $$
    DECLARE
        sourceraster raster := rast;
        newrast raster;
        geomrast raster;
        numband int := ST_Numbands(rast);
        bandstart int;
        bandend int;
        newextent text;
        newnodata float8;
        bandi int;
    BEGIN
        IF rast IS NULL THEN
            RETURN null;
        END IF;
        IF geom IS NULL THEN
            RETURN rast;
        END IF;
        IF band IS NULL THEN
            bandstart := 1;
            bandend := numband;
        ELSEIF ST_HasNoBand(rast, band) THEN
            bandstart := numband;
            bandend := numband;
        ELSE
            bandstart := band;
            bandend := band;
        END IF;
        IF nodata IS NULL THEN
            newnodata := coalesce(ST_BandNodataValue(rast, bandstart), ST_MinPossibleVal(ST_BandPixelType(rast, bandstart)));
        ELSE
            newnodata := nodata;
        END IF;
        newextent := CASE WHEN trimraster THEN 'INTERSECTION' ELSE 'FIRST' END;
 RAISE NOTICE 'bandstart=%, bandend=%', bandstart, bandend;
        -- Convert the geometry to a raster
        geomrast := ST_AsRaster(geom, rast, ST_BandPixelType(rast, band), 1, newnodata);  

        -- Compute the first raster band
        sourceraster := ST_SetBandNodataValue(sourceraster, bandstart, newnodata);
        newrast := ST_MapAlgebraExpr(sourceraster, bandstart, geomrast, 1, 'rast1');
        
        FOR bandi IN bandstart+1..bandend LOOP
 RAISE NOTICE 'bandi=%', bandi;
            -- for each band we must determine the nodata value
            IF nodata IS NULL THEN
                newnodata := coalesce(ST_BandNodataValue(sourceraster, bandi), ST_MinPossibleVal(ST_BandPixelType(rast, bandi)));
            ELSE
                newnodata := nodata;
            END IF;
            sourceraster := ST_SetBandNodataValue(sourceraster, bandi, newnodata);
            newrast := ST_AddBand(newrast, ST_MapAlgebraExpr(sourceraster, bandi, geomrast, 1, 'rast1'));
        END LOOP;
        RETURN newrast;
    END;
    $$
    LANGUAGE 'plpgsql';


-- Test

CREATE OR REPLACE FUNCTION ST_TestRaster(h integer, w integer, val float8) 
    RETURNS raster AS 
    $$
    DECLARE
    BEGIN
        RETURN ST_AddBand(ST_MakeEmptyRaster(h, w, 0, 0, 1, 1, 0, 0, 0), '32BF', val, -1);
    END;
    $$
    LANGUAGE 'plpgsql';

SELECT ST_Clip(ST_TestRaster(10, 10, 2), 1, ST_Buffer(ST_MakePoint(8, 5), 4)) rast

-- Test one band raster
SELECT ST_AsBinary((gv).geom), (gv).val 
FROM ST_PixelAsPolygons(ST_Clip(ST_TestRaster(10, 10, 2), 1, ST_Buffer(ST_MakePoint(8, 5), 4))) gv

-- Test two bands raster
SELECT ST_AsBinary((gv).geom), (gv).val 
FROM ST_PixelAsPolygons(ST_Clip(ST_AddBand(ST_TestRaster(10, 10, 2), '16BUI'::text, 4, 0), null, ST_Buffer(ST_MakePoint(8, 5), 4)), 2) gv


