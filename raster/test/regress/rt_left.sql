-----------------------------------------------------------------------
-- $Id$
--
-- Copyright (c) 2009 Sandro Santilli <strk@keybit.net>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
-----------------------------------------------------------------------

-------------------------------------------------------------------
-- st_left
-----------------------------------------------------------------------

SELECT 'st_left(X, query(1,1))' as op,
        count(a.y),
        min(a.x) as xmin,
        max(a.x) as xmax,
        min(a.y) as ymin,
        max(a.y) as ymax,
        st_extent(a.tile)
FROM rt_gist_grid_test a, rt_gist_query_test b
WHERE b.x = 1 and b.y = 1
    AND st_left(a.tile, b.tile);

-----------------------------------------------------------------------
-- Test << operator (left)
-----------------------------------------------------------------------

SELECT 'X << query(1,1)' as op,
        count(a.y),
        min(a.x) as xmin,
        max(a.x) as xmax,
        min(a.y) as ymin,
        max(a.y) as ymax,
        st_extent(a.tile)
FROM rt_gist_grid_test a, rt_gist_query_test b
WHERE b.x = 1 and b.y = 1
    AND a.tile << b.tile;
