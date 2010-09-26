-----------------------------------------------------------------------
-- $Id$
--
-- Copyright (c) 2010 Pierre Racine <pierre.racine@sbf.ulaval.ca>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
-----------------------------------------------------------------------

-----------------------------------------------------------------------
-- Test 1 - st_world2rastercoordx(rast raster, xw float8, yw float8) 
-----------------------------------------------------------------------

SELECT 'test 1.1', id, name
    FROM rt_utility_test
    WHERE st_world2rastercoordx(rast, 
                                ipx, 
                                ipy
                               ) != 1;

SELECT 'test 1.2', id, name
    FROM rt_utility_test
    WHERE st_world2rastercoordx(rast, 
                                scalex * (width - 1) + skewx * (height - 1) + ipx, 
                                skewy * (width - 1) + scaley * (height - 1) + ipy
                               ) != width;

SELECT 'test 1.3', id, name
    FROM rt_utility_test
    WHERE st_world2rastercoordx(rast, 
                                scalex * width + skewx * height + ipx, 
                                skewy * width + scaley * height + ipy
                               ) != width + 1;

-----------------------------------------------------------------------
-- Test 2 - st_world2rastercoordx(rast raster, xw float8) 
-----------------------------------------------------------------------

SELECT 'test 2.1', id, name
    FROM rt_utility_test
    WHERE skewx = 0 and 
          st_world2rastercoordx(rast, 
                                ipx
                               ) != 1;

SELECT 'test 2.2', id, name
    FROM rt_utility_test
    WHERE skewx = 0 and 
          st_world2rastercoordx(rast, 
                                scalex * (width - 1) + ipx
                               ) != width;

SELECT 'test 2.3', id, name
    FROM rt_utility_test
    WHERE skewx = 0 and 
          st_world2rastercoordx(rast, 
                                scalex * width + ipx
                               ) != width + 1;

SELECT 'test 2.4', id, name
    FROM rt_utility_test
    WHERE st_world2rastercoordx(rast, 
                                ipx
                               ) != 1;

-----------------------------------------------------------------------
-- Test 3 - st_world2rastercoordx(rast raster, pt geometry) 
-----------------------------------------------------------------------

SELECT 'test 3.1', id, name
    FROM rt_utility_test
    WHERE st_world2rastercoordx(rast, 
                                st_makepoint(
                                             ipx, 
                                             ipy
                                            )
                               ) != 1;

SELECT 'test 3.2', id, name
    FROM rt_utility_test
    WHERE st_world2rastercoordx(rast, 
                                st_makepoint(
                                             scalex * (width - 1) + skewx * (height - 1) + ipx, 
                                             skewy * (width - 1) + scaley * (height - 1) + ipy
                                            )
                               ) != width;

SELECT 'test 3.3', id, name
    FROM rt_utility_test
    WHERE st_world2rastercoordx(rast, 
                                st_makepoint(
                                             scalex * width + skewx * height + ipx, 
                                             skewy * width + scaley * height + ipy
                                            )
                               ) != width + 1;

-----------------------------------------------------------------------
-- Test 4 - st_world2rastercoordy(rast raster, xw float8, yw float8) 
-----------------------------------------------------------------------

SELECT 'test 4.1', id, name
    FROM rt_utility_test
    WHERE st_world2rastercoordy(rast, 
                                ipx, 
                                ipy
                               ) != 1;

SELECT 'test 4.2', id, name
    FROM rt_utility_test
    WHERE st_world2rastercoordy(rast, 
                                scalex * (width - 1) + skewx * (height - 1) + ipx, 
                                skewy * (width - 1) + scaley * (height - 1) + ipy
                               ) != height;

SELECT 'test 4.3', id, name
    FROM rt_utility_test
    WHERE st_world2rastercoordy(rast, 
                                scalex * width + skewx * height + ipx, 
                                skewy * width + scaley * height + ipy
                               ) != height + 1;

-----------------------------------------------------------------------
-- Test 5 - st_world2rastercoordy(rast raster, yw float8) 
-----------------------------------------------------------------------

SELECT 'test 5.1', id, name
    FROM rt_utility_test
    WHERE skewy = 0 and 
          st_world2rastercoordy(rast, 
                                ipy
                               ) != 1;

SELECT 'test 5.2', id, name
    FROM rt_utility_test
    WHERE skewy = 0 and 
          st_world2rastercoordy(rast, 
                                scaley * (height - 1) + ipy
                               ) != height;

SELECT 'test 5.3', id, name
    FROM rt_utility_test
    WHERE skewy = 0 and 
          st_world2rastercoordy(rast, 
                                scaley * height + ipy
                               ) != height + 1;

SELECT 'test 5.4', id, name
    FROM rt_utility_test
    WHERE st_world2rastercoordy(rast, 
                                ipy
                               ) != 1;


-----------------------------------------------------------------------
-- Test 6 - st_world2rastercoordy(rast raster, pt geometry) 
-----------------------------------------------------------------------

SELECT 'test 6.1', id, name
    FROM rt_utility_test
    WHERE st_world2rastercoordy(rast, 
                                st_makepoint(
                                             ipx, 
                                             ipy
                                            )
                               ) != 1;

SELECT 'test 6.2', id, name
    FROM rt_utility_test
    WHERE st_world2rastercoordy(rast, 
                                st_makepoint(
                                             scalex * (width - 1) + skewx * (height - 1) + ipx, 
                                             skewy * (width - 1) + scaley * (height - 1) + ipy
                                            )
                               ) != height;

SELECT 'test 6.3', id, name
    FROM rt_utility_test
    WHERE st_world2rastercoordy(rast, 
                                st_makepoint(
                                             scalex * width + skewx * height + ipx, 
                                             skewy * width + scaley * height + ipy
                                            )
                               ) != height + 1;

-----------------------------------------------------------------------
-- Test 7 - st_raster2worldcoordx(rast raster, xr int, yr int)
-----------------------------------------------------------------------

SELECT 'test 7.1', id, name
    FROM rt_utility_test
    WHERE st_raster2worldcoordx(rast, 1, 1)::numeric != ipx::numeric;
    
SELECT 'test 7.2', id, name
    FROM rt_utility_test
    WHERE st_raster2worldcoordx(rast, width, height)::numeric != (scalex * (width - 1) + skewx * (height - 1) + ipx)::numeric;

-----------------------------------------------------------------------
-- Test 8 - st_raster2worldcoordx(rast raster, xr int)
-----------------------------------------------------------------------

SELECT 'test 8.1', id, name
    FROM rt_utility_test
    WHERE skewx = 0 and st_raster2worldcoordx(rast, 1)::numeric != ipx::numeric;
    
SELECT 'test 8.2', id, name
    FROM rt_utility_test
    WHERE skewx = 0 and st_raster2worldcoordx(rast, width)::numeric != (scalex * (width - 1) + ipx)::numeric;

SELECT 'test 8.3', id, name
    FROM rt_utility_test
    WHERE st_raster2worldcoordx(rast, 1)::numeric != ipx::numeric;
 
-----------------------------------------------------------------------
-- Test 9 - st_raster2worldcoordy(rast raster, xr int, yr int)
-----------------------------------------------------------------------

SELECT 'test 9.1', id, name
    FROM rt_utility_test
    WHERE st_raster2worldcoordy(rast, 1, 1)::numeric != ipy::numeric;
    
SELECT 'test 9.2', id, name
    FROM rt_utility_test
    WHERE st_raster2worldcoordy(rast, width, height)::numeric != (skewy * (width - 1) + scaley * (height - 1) + ipy)::numeric;

-----------------------------------------------------------------------
-- Test 10 - st_raster2worldcoordy(rast raster, yr int) 
-----------------------------------------------------------------------

SELECT 'test 10.1', id, name
    FROM rt_utility_test
    WHERE skewy = 0 and st_raster2worldcoordy(rast, 1, 1)::numeric != ipy::numeric;
    
SELECT 'test 10.2', id, name
    FROM rt_utility_test
    WHERE skewy = 0 and st_raster2worldcoordy(rast, width, height)::numeric != (scaley * (height - 1) + ipy)::numeric;

SELECT 'test 10.3', id, name
    FROM rt_utility_test
    WHERE st_raster2worldcoordy(rast, 1)::numeric != ipy::numeric;
    
