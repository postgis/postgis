-----------------------------------------------------------------------
-- $Id$
--
-- Copyright (c) 2009 Sandro Santilli <strk@keybit.net>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
-----------------------------------------------------------------------

-----------------------------------------------------------------------
-- makeTileGrid( <gridColumns>, <gridRows>, <gridExtent>,
--               <tileWidth>, <tileHeight> )
--
-- Return a set of tuples in the form:
--     x int, y int, t raster
-- Containing a subdivision of given extent 
-- into a grid composed by rasters of the given
-- dimension in pixels.
--
-- TODO: DROP functiona and type at the end of test
-----------------------------------------------------------------------
CREATE TYPE tile AS (x int, y int, tile raster);
CREATE OR REPLACE FUNCTION makegrid (int, int, box2d, int, int)
    RETURNS SETOF tile
AS
'
DECLARE
    gridCols alias for $1;
    gridRows alias for $2;
    extent alias for $3;
    tileWidth alias for $4;
    tileHeight alias for $5;
    rec tile;
    scalex float8;
    scaley float8;
    ipx float8;
    ipy float8;
BEGIN
	
    -- compute some sizes
    -- each tile extent width is extent.width / gridRows
    scalex = ((ST_xmax(extent)-ST_xmin(extent))/gridCols)/tileWidth;
    scaley = ((ST_ymax(extent)-ST_ymin(extent))/gridRows)/tileHeight;

    FOR y IN 0..gridRows-1 LOOP
        ipy = y*scaley + ST_ymin(extent);
        FOR x IN 0..gridCols-1 LOOP
            ipx = x*scalex + ST_xmin(extent);
            rec.x = x;
            rec.y = y;
            rec.tile = st_MakeEmptyRaster(tileWidth, tileHeight, ipx, ipy,
                                          scalex, scaley, 0, 0);
            RETURN NEXT rec;
        END LOOP;
    END LOOP;

    RETURN;
END;
'
LANGUAGE 'plpgsql';

-----------------------------------------------------------------------
-- Create a grid of 10x10 rasters on an extent of -100..100 both axis
-- and another on the same extent with 3x3 tiles, to use for queries 
-----------------------------------------------------------------------

CREATE TABLE rt_gist_grid_test AS 
    SELECT * FROM makegrid(10, 10, 'BOX(-100 -100, 100 100)', 1, 1);

CREATE TABLE rt_gist_query_test AS
    SELECT * from makegrid(3, 3, 'BOX(-100 -100, 100 100)', 1, 1);
