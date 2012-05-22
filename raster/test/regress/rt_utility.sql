-----------------------------------------------------------------------
-- $Id$
--
-- Copyright (c) 2010 Pierre Racine <pierre.racine@sbf.ulaval.ca>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
-----------------------------------------------------------------------

\i create_rt_utility_test.sql

-----------------------------------------------------------------------
-- Test 1 - st_world2rastercoordx(rast raster, xw float8, yw float8) 
-----------------------------------------------------------------------

SELECT 'test 1.1', id, name
    FROM rt_utility_test
    WHERE COALESCE(st_world2rastercoordx(rast, 
                                ipx, 
                                ipy
                               ), 0) != 1;

SELECT 'test 1.2', id, name
    FROM rt_utility_test
    WHERE COALESCE(st_world2rastercoordx(rast, 
                                scalex * (width - 1) + skewx * (height - 1) + ipx, 
                                skewy * (width - 1) + scaley * (height - 1) + ipy
                               ), 0) != width;

SELECT 'test 1.3', id, name
    FROM rt_utility_test
    WHERE COALESCE(st_world2rastercoordx(rast, 
                                scalex * width + skewx * height + ipx, 
                                skewy * width + scaley * height + ipy
                               ), 0) != width + 1;

-----------------------------------------------------------------------
-- Test 2 - st_world2rastercoordx(rast raster, xw float8) 
-----------------------------------------------------------------------

SELECT 'test 2.1', id, name
    FROM rt_utility_test
    WHERE skewx = 0 and 
          COALESCE(st_world2rastercoordx(rast, 
                                ipx
                               ), 0) != 1;

SELECT 'test 2.2', id, name
    FROM rt_utility_test
    WHERE skewx = 0 and 
          COALESCE(st_world2rastercoordx(rast, 
                                scalex * (width - 1) + ipx
                               ), 0) != width;

SELECT 'test 2.3', id, name
    FROM rt_utility_test
    WHERE skewx = 0 and 
          COALESCE(st_world2rastercoordx(rast, 
                                scalex * width + ipx
                               ), 0) != width + 1;

SELECT 'test 2.4', id, name
    FROM rt_utility_test
    WHERE COALESCE(st_world2rastercoordx(rast, 
                                ipx
                               ), 0) != 1;

-----------------------------------------------------------------------
-- Test 3 - st_world2rastercoordx(rast raster, pt geometry) 
-----------------------------------------------------------------------

SELECT 'test 3.1', id, name
    FROM rt_utility_test
    WHERE COALESCE(st_world2rastercoordx(rast, 
                                st_makepoint(
                                             ipx, 
                                             ipy
                                            )
                               ), 0) != 1;

SELECT 'test 3.2', id, name
    FROM rt_utility_test
    WHERE COALESCE(st_world2rastercoordx(rast, 
                                st_makepoint(
                                             scalex * (width - 1) + skewx * (height - 1) + ipx, 
                                             skewy * (width - 1) + scaley * (height - 1) + ipy
                                            )
                               ), 0) != width;

SELECT 'test 3.3', id, name
    FROM rt_utility_test
    WHERE COALESCE(st_world2rastercoordx(rast, 
                                st_makepoint(
                                             scalex * width + skewx * height + ipx, 
                                             skewy * width + scaley * height + ipy
                                            )
                               ), 0) != width + 1;

-----------------------------------------------------------------------
-- Test 4 - st_world2rastercoordy(rast raster, xw float8, yw float8) 
-----------------------------------------------------------------------

SELECT 'test 4.1', id, name
    FROM rt_utility_test
    WHERE COALESCE(st_world2rastercoordy(rast, 
                                ipx, 
                                ipy
                               ), 0) != 1;

SELECT 'test 4.2', id, name
    FROM rt_utility_test
    WHERE COALESCE(st_world2rastercoordy(rast, 
                                scalex * (width - 1) + skewx * (height - 1) + ipx, 
                                skewy * (width - 1) + scaley * (height - 1) + ipy
                               ), 0) != height;

SELECT 'test 4.3', id, name
    FROM rt_utility_test
    WHERE COALESCE(st_world2rastercoordy(rast, 
                                scalex * width + skewx * height + ipx, 
                                skewy * width + scaley * height + ipy
                               ), 0) != height + 1;

-----------------------------------------------------------------------
-- Test 5 - st_world2rastercoordy(rast raster, yw float8) 
-----------------------------------------------------------------------

SELECT 'test 5.1', id, name
    FROM rt_utility_test
    WHERE skewy = 0 and 
          COALESCE(st_world2rastercoordy(rast, 
                                ipy
                               ), 0) != 1;

SELECT 'test 5.2', id, name
    FROM rt_utility_test
    WHERE skewy = 0 and 
          COALESCE(st_world2rastercoordy(rast, 
                                scaley * (height - 1) + ipy
                               ), 0) != height;

SELECT 'test 5.3', id, name
    FROM rt_utility_test
    WHERE skewy = 0 and 
          COALESCE(st_world2rastercoordy(rast, 
                                scaley * height + ipy
                               ), 0) != height + 1;

SELECT 'test 5.4', id, name
    FROM rt_utility_test
    WHERE COALESCE(st_world2rastercoordy(rast, 
                                ipy
                               ), 0) != 1;


-----------------------------------------------------------------------
-- Test 6 - st_world2rastercoordy(rast raster, pt geometry) 
-----------------------------------------------------------------------

SELECT 'test 6.1', id, name
    FROM rt_utility_test
    WHERE COALESCE(st_world2rastercoordy(rast, 
                                st_makepoint(
                                             ipx, 
                                             ipy
                                            )
                               ), 0) != 1;

SELECT 'test 6.2', id, name
    FROM rt_utility_test
    WHERE COALESCE(st_world2rastercoordy(rast, 
                                st_makepoint(
                                             scalex * (width - 1) + skewx * (height - 1) + ipx, 
                                             skewy * (width - 1) + scaley * (height - 1) + ipy
                                            )
                               ), 0) != height;

SELECT 'test 6.3', id, name
    FROM rt_utility_test
    WHERE COALESCE(st_world2rastercoordy(rast, 
                                st_makepoint(
                                             scalex * width + skewx * height + ipx, 
                                             skewy * width + scaley * height + ipy
                                            )
                               ), 0) != height + 1;

-----------------------------------------------------------------------
-- Test 7 - st_raster2worldcoordx(rast raster, xr int, yr int)
-----------------------------------------------------------------------

SELECT 'test 7.1', id, name
    FROM rt_utility_test
    WHERE COALESCE(st_raster2worldcoordx(rast, 1, 1), 0)::numeric != ipx::numeric;
    
SELECT 'test 7.2', id, name
    FROM rt_utility_test
    WHERE COALESCE(st_raster2worldcoordx(rast, width, height), 0)::numeric != (scalex * (width - 1) + skewx * (height - 1) + ipx)::numeric;

-----------------------------------------------------------------------
-- Test 8 - st_raster2worldcoordx(rast raster, xr int)
-----------------------------------------------------------------------

SELECT 'test 8.1', id, name
    FROM rt_utility_test
    WHERE skewx = 0 and COALESCE(st_raster2worldcoordx(rast, 1), 0)::numeric != ipx::numeric;
    
SELECT 'test 8.2', id, name
    FROM rt_utility_test
    WHERE skewx = 0 and COALESCE(st_raster2worldcoordx(rast, width), 0)::numeric != (scalex * (width - 1) + ipx)::numeric;

SELECT 'test 8.3', id, name
    FROM rt_utility_test
    WHERE COALESCE(st_raster2worldcoordx(rast, 1), 0)::numeric != ipx::numeric;
 
-----------------------------------------------------------------------
-- Test 9 - st_raster2worldcoordy(rast raster, xr int, yr int)
-----------------------------------------------------------------------

SELECT 'test 9.1', id, name
    FROM rt_utility_test
    WHERE COALESCE(st_raster2worldcoordy(rast, 1, 1), 0)::numeric != ipy::numeric;
    
SELECT 'test 9.2', id, name
    FROM rt_utility_test
    WHERE round(COALESCE(st_raster2worldcoordy(rast, width, height), 0)::numeric, 10) != round((skewy * (width - 1) + scaley * (height - 1) + ipy)::numeric, 10);

-----------------------------------------------------------------------
-- Test 10 - st_raster2worldcoordy(rast raster, yr int) 
-----------------------------------------------------------------------

SELECT 'test 10.1', id, name
    FROM rt_utility_test
    WHERE skewy = 0 and COALESCE(st_raster2worldcoordy(rast, 1, 1), 0)::numeric != ipy::numeric;
    
SELECT 'test 10.2', id, name
    FROM rt_utility_test
    WHERE skewy = 0 and COALESCE(st_raster2worldcoordy(rast, width, height), 0)::numeric != (scaley * (height - 1) + ipy)::numeric;

SELECT 'test 10.3', id, name
    FROM rt_utility_test
    WHERE COALESCE(st_raster2worldcoordy(rast, 1), 0)::numeric != ipy::numeric;
    
-----------------------------------------------------------------------
-- Test 11 - st_minpossiblevalue(pixtype text)
-----------------------------------------------------------------------

SELECT 'test 11.1', st_minpossiblevalue('1BB') = 0.;
SELECT 'test 11.2', st_minpossiblevalue('2BUI') = 0.;
SELECT 'test 11.3', st_minpossiblevalue('4BUI') = 0.;
SELECT 'test 11.4', st_minpossiblevalue('8BUI') = 0.;
SELECT 'test 11.5', st_minpossiblevalue('8BSI') < 0.;
SELECT 'test 11.6', st_minpossiblevalue('16BUI') = 0.;
SELECT 'test 11.7', st_minpossiblevalue('16BSI') < 0.;
SELECT 'test 11.8', st_minpossiblevalue('32BUI') = 0.;
SELECT 'test 11.9', st_minpossiblevalue('32BSI') < 0.;
SELECT 'test 11.10', st_minpossiblevalue('32BF') < 0.;
SELECT 'test 11.11', st_minpossiblevalue('64BF') < 0.;

DROP TABLE rt_utility_test;
