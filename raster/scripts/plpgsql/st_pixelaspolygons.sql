-- $Id$
----------------------------------------------------------------------
-- Copyright (c) 2009-2010 Pierre Racine <pierre.racine@sbf.ulaval.ca>
----------------------------------------------------------------------

-----------------------------------------------------------------------
-- ST_PixelAsPolygons
-- Return all the pixels of a raster as a geomval record
-- Should be called like this:
-- SELECT (gv).geom, (gv).val FROM (SELECT ST_PixelAsPolygons(rast) gv FROM mytable) foo
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION ST_PixelAsPolygons(rast raster, band integer) 
    RETURNS SETOF geomval AS 
    $$
    DECLARE
        rast alias for $1;
        w integer;
        h integer;
        x integer;
        y integer;
        result geomval;
    BEGIN
        SELECT st_width(rast), st_height(rast)
        INTO w, h;
        FOR x IN 1..w LOOP
             FOR y IN 1..h LOOP
                 SELECT ST_PixelAsPolygon(rast, band, x, y), ST_Value(rast, band, x, y) INTO result;
            RETURN NEXT result;
         END LOOP;
        END LOOP;
        RETURN;
    END;
    $$
    LANGUAGE 'plpgsql';

CREATE FUNCTION ST_PixelAsPolygons(raster) RETURNS SETOF geomval AS 
$$
    SELECT ST_PixelAsPolygons($1, 1);
$$
LANGUAGE SQL;
