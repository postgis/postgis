-----------------------------------------------------------------------
-- $Id$
--
-- Copyright (c) 2009 Sandro Santilli <strk@keybit.net>
--
-- This program is free software; you can redistribute it and/or
-- modify it under the terms of the GNU General Public License
-- as published by the Free Software Foundation; either version 2
-- of the License, or (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this program; if not, write to the Free Software Foundation,
-- Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
-----------------------------------------------------------------------

-------------------------------------------------------------------
-- raster_contain
-----------------------------------------------------------------------

SELECT 'raster_contain(query(1,1), X)' as op,
        count(a.y),
        min(a.x) as xmin,
        max(a.x) as xmax,
        min(a.y) as ymin,
        max(a.y) as ymax,
        st_extent(a.tile::geometry)
FROM rt_gist_grid_test a, rt_gist_query_test b
WHERE b.x = 1 and b.y = 1
    AND raster_contain(b.tile, a.tile);

-------------------------------------------------------------------
-- raster_geometry_contain
-----------------------------------------------------------------------

SELECT 'raster_geometry_contain(query(1,1), X)' as op,
        count(a.y),
        min(a.x) as xmin,
        max(a.x) as xmax,
        min(a.y) as ymin,
        max(a.y) as ymax,
        st_extent(a.tile::geometry)
FROM rt_gist_grid_test a, rt_gist_query_test b
WHERE b.x = 1 and b.y = 1
    AND raster_geometry_contain(b.tile, a.tile::geometry);

-------------------------------------------------------------------
-- geometry_raster_contain
-----------------------------------------------------------------------

SELECT 'geometry_raster_contain(query(1,1), X)' as op,
        count(a.y),
        min(a.x) as xmin,
        max(a.x) as xmax,
        min(a.y) as ymin,
        max(a.y) as ymax,
        st_extent(a.tile::geometry)
FROM rt_gist_grid_test a, rt_gist_query_test b
WHERE b.x = 1 and b.y = 1
    AND geometry_raster_contain(b.tile::geometry, a.tile);

-----------------------------------------------------------------------
-- Test ~ operator (raster contains raster)
-----------------------------------------------------------------------

SELECT 'query(1,1) ~ X' as op,
        count(a.y),
        min(a.x) as xmin,
        max(a.x) as xmax,
        min(a.y) as ymin,
        max(a.y) as ymax,
        st_extent(a.tile::geometry)
FROM rt_gist_grid_test a, rt_gist_query_test b
WHERE b.x = 1 and b.y = 1
    AND b.tile ~ a.tile;

-----------------------------------------------------------------------
-- Test ~ operator (raster contains geometry)
-----------------------------------------------------------------------

SELECT 'query(1,1) ~ X' as op,
        count(a.y),
        min(a.x) as xmin,
        max(a.x) as xmax,
        min(a.y) as ymin,
        max(a.y) as ymax,
        st_extent(a.tile::geometry)
FROM rt_gist_grid_test a, rt_gist_query_test b
WHERE b.x = 1 and b.y = 1
    AND b.tile ~ a.tile::geometry;

-----------------------------------------------------------------------
-- Test ~ operator (geometry contains raster )
-----------------------------------------------------------------------

SELECT 'query(1,1) ~ X' as op,
        count(a.y),
        min(a.x) as xmin,
        max(a.x) as xmax,
        min(a.y) as ymin,
        max(a.y) as ymax,
        st_extent(a.tile::geometry)
FROM rt_gist_grid_test a, rt_gist_query_test b
WHERE b.x = 1 and b.y = 1
    AND b.tile::geometry ~ a.tile;
