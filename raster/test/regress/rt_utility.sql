-----------------------------------------------------------------------
-- $Id$
--
-- Copyright (c) 2010 Pierre Racine <pierre.racine@sbf.ulaval.ca>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
-----------------------------------------------------------------------

CREATE TABLE rt_utility_test (
    id numeric,
    name text,
    srid integer,
    width integer,
    height integer,
    scalex double precision,
    scaley double precision,
    ipx double precision,
    ipy double precision,
    skewx double precision,
    skewy double precision,
    rast raster
);

INSERT INTO rt_utility_test 
VALUES ( 1, '1217x1156, ip:782325.5,26744042.5 scale:5,-5 skew:0,0 srid:9102707 width:1217 height:1156',
        26919, 1217, 1156, --- SRID, width, height
        5, -5, 782325.5, 26744042.5, 0, 0, --- georeference
(
'01' -- little endian (uint8 ndr)
|| 
'0000' -- version (uint16 0)
||
'0000' -- nBands (uint16 0)
||
'0000000000001440' -- scaleX (float64 5)
||
'00000000000014C0' -- scaleY (float64 -5)
||
'00000000EBDF2741' -- ipX (float64 782325.5)
||
'000000A84E817941' -- ipY (float64 26744042.5)
||
'0000000000000000' -- skewX (float64 0)
||
'0000000000000000' -- skewY (float64 0)
||
'27690000' -- SRID (int32 26919 - UTM 19N)
||
'C104' -- width (uint16 1217)
||
'8404' -- height (uint16 1156)
)::raster
);

INSERT INTO rt_utility_test 
VALUES ( 2, '1217x1156, ip:782325.5,26744042.5 scale:5,-5 skew:3,3 srid:9102707 width:1217 height:1156',
        26919, 1217, 1156, --- SRID, width, height
        5, -5, 782325.5, 26744042.5, 3, 3, --- georeference
(
'01' -- little endian (uint8 ndr)
|| 
'0000' -- version (uint16 0)
||
'0000' -- nBands (uint16 0)
||
'0000000000001440' -- scaleX (float64 5)
||
'00000000000014C0' -- scaleY (float64 -5)
||
'00000000EBDF2741' -- ipX (float64 782325.5)
||
'000000A84E817941' -- ipY (float64 26744042.5)
||
'0000000000000840' -- skewX (float64 3)
||
'0000000000000840' -- skewY (float64 3)
||
'27690000' -- SRID (int32 26919 - UTM 19N)
||
'C104' -- width (uint16 1217)
||
'8404' -- height (uint16 1156)
)::raster
);

INSERT INTO rt_utility_test 
VALUES ( 3, '6000x6000, ip:-75,50 scale:0.000833333333333333,-0.000833333333333333 skew:0,0 srid:4326 width:6000 height:6000',
        4326, 6000, 6000, --- SRID, width, height
        0.000833333333333333, -0.000833333333333333, -75, 50, 0, 0, --- georeference
(
'01' -- little endian (uint8 ndr)
|| 
'0000' -- version (uint16 0)
||
'0000' -- nBands (uint16 0)
||
'4F1BE8B4814E4B3F' -- scaleX (float64 0.000833333333333333)
||
'4F1BE8B4814E4BBF' -- scaleY (float64 -0.000833333333333333)
||
'0000000000C052C0' -- ipX (float64 -75)
||
'0000000000004940' -- ipY (float64 50)
||
'0000000000000000' -- skewX (float64 0)
||
'0000000000000000' -- skewY (float64 0)
||
'E6100000' -- SRID (int32 4326)
||
'7017' -- width (uint16 6000)
||
'7017' -- height (uint16 6000)
)::raster
);

INSERT INTO rt_utility_test 
VALUES ( 4, '6000x6000, ip:-75.5533328537098,49.2824585505576 scale:0.000805965234044584,-0.00080596523404458 skew:0.000211812383858707,0.000211812383858704 srid:4326 width:6000 height:6000',
        4326, 6000, 6000, --- SRID, width, height
        0.000805965234044584, -0.00080596523404458, -75.5533328537098, 49.2824585505576, 0.000211812383858707, 0.000211812383858704, --- georeference
(
'01' -- little endian (uint8 ndr)
|| 
'0000' -- version (uint16 0)
||
'0000' -- nBands (uint16 0)
||
'17263529ED684A3F' -- scaleX (float64 0.000805965234044584)
||
'F9253529ED684ABF' -- scaleY (float64 -0.00080596523404458)
||
'1C9F33CE69E352C0' -- ipX (float64 -75.5533328537098)
||
'718F0E9A27A44840' -- ipY (float64 49.2824585505576)
||
'ED50EB853EC32B3F' -- skewX (float64 0.000211812383858707)
||
'7550EB853EC32B3F' -- skewY (float64 0.000211812383858704)
||
'E6100000' -- SRID (int32 4326)
||
'7017' -- width (uint16 6000)
||
'7017' -- height (uint16 6000)
)::raster
);

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
