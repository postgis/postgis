-----------------------------------------------------------------------
-- $Id$
--
-- Copyright (c) 2009 Sandro Santilli <strk@keybit.net>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
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

CREATE TABLE rt_gist_grid_test AS 
    SELECT * FROM makegrid(10, 10, 'BOX(-100 -100, 100 100)', 1, 1);

CREATE TABLE rt_gist_query_test AS
    SELECT * from makegrid(3, 3, 'BOX(-100 -100, 100 100)', 1, 1);

-------------------------------------------------------------------
-- raster_contained
-----------------------------------------------------------------------

SELECT 'raster_contained(X, query(1,1))' as op,
        count(a.y),
        min(a.x) as xmin,
        max(a.x) as xmax,
        min(a.y) as ymin,
        max(a.y) as ymax,
        st_extent(a.tile::geometry)
FROM rt_gist_grid_test a, rt_gist_query_test b
WHERE b.x = 1 and b.y = 1
    AND raster_contained(a.tile, b.tile);

-----------------------------------------------------------------------
-- Test @ operator (contained by)
-----------------------------------------------------------------------

SELECT 'X @ query(1,1)' as op,
        count(a.y),
        min(a.x) as xmin,
        max(a.x) as xmax,
        min(a.y) as ymin,
        max(a.y) as ymax,
        st_extent(a.tile::geometry)
FROM rt_gist_grid_test a, rt_gist_query_test b
WHERE b.x = 1 and b.y = 1
    AND a.tile @ b.tile;

DROP FUNCTION makegrid(integer,integer,box2d,integer,integer);
DROP table rt_gist_grid_test;
DROP table rt_gist_query_test;
DROP type tile;
