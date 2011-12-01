----------------------------------------------------------------------
--
-- $Id$
--
-- Copyright (c) 2009-2010 Pierre Racine <pierre.racine@sbf.ulaval.ca>
--
----------------------------------------------------------------------
CREATE TYPE geomvalxy AS (
    geom geometry,
    val double precision,
    x int,
    y int
);

-----------------------------------------------------------------------
-- ST_PixelAsPolygons
-- Return all the pixels of a raster as a geomval record
-- Should be called like this:
-- SELECT (gv).geom, (gv).val FROM (SELECT ST_PixelAsPolygons(rast) gv FROM mytable) foo
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION ST_PixelAsPolygons(rast raster, band integer) 
    RETURNS SETOF geomvalxy AS 
    $$
    DECLARE
        rast alias for $1;
        w integer;
        h integer;
        x integer;
        y integer;
        numband int;
        result geomvalxy;
    BEGIN
        SELECT ST_Width(rast), ST_Height(rast), ST_NumBands(rast)
        INTO w, h, numband;
        IF band < numband THEN
            numband := band;
        END IF;
        FOR x IN 1..w LOOP
             FOR y IN 1..h LOOP
                 SELECT ST_PixelAsPolygon(rast, numband, x, y), ST_Value(rast, numband, x, y), x, y INTO result;
            RETURN NEXT result;
         END LOOP;
        END LOOP;
        RETURN;
    END;
    $$
    LANGUAGE 'plpgsql';

CREATE FUNCTION ST_PixelAsPolygons(raster) RETURNS SETOF geomvalxy AS 
$$
    SELECT ST_PixelAsPolygons($1, 1);
$$
LANGUAGE SQL;
