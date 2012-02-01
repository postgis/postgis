-----------------------------------------------------------------------
-- $Id$
--
-- Copyright (c) 2010 Pierre Racine <pierre.racine@sbf.ulaval.ca>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
-----------------------------------------------------------------------

-----------------------------------------------------------------------
-- Test 1 - st_value(rast raster, band integer, x integer, y integer) 
-----------------------------------------------------------------------

SELECT 'test 1.1', id
    FROM rt_band_properties_test
    WHERE st_value(rast, 1, 1, 1, TRUE) != b1val;

SELECT 'test 1.2', id
    FROM rt_band_properties_test
    WHERE st_value(rast, 2, 1, 1, TRUE) != b2val;

SELECT 'test 1.3', id
    FROM rt_band_properties_test
    WHERE st_value(rast, 1, 1, 1, FALSE) != b1val;

SELECT 'test 1.4', id
    FROM rt_band_properties_test
    WHERE st_value(rast, 1, 1, 1, NULL) != b1val;

SELECT 'test 1.5', id
    FROM rt_band_properties_test
    WHERE NOT st_value(rast, 1, 1, NULL, NULL) IS NULL;

SELECT 'test 1.6', id
    FROM rt_band_properties_test
    WHERE NOT st_value(rast, 1, NULL, 1, NULL) IS NULL;

SELECT 'test 1.7', id
    FROM rt_band_properties_test
    WHERE NOT st_value(st_setbandnodatavalue(rast, b1val), 1, 1, 1, NULL) IS NULL;

SELECT 'test 1.8', id
    FROM rt_band_properties_test
    WHERE NOT st_value(st_setbandnodatavalue(rast, b1val), 1, 1, 1, TRUE) IS NULL;

SELECT 'test 1.9', id
    FROM rt_band_properties_test
    WHERE st_value(st_setbandnodatavalue(rast, b1val), 1, 1, 1, FALSE) != b1val;

-- Make sure we return only a warning when getting vlue with out of range pixel coordinates
SELECT 'test 1.10', id
    FROM rt_band_properties_test
    WHERE NOT st_value(rast, -1, -1) IS NULL;

-----------------------------------------------------------------------
-- Test 2 - st_value(rast raster, band integer, pt geometry)
-----------------------------------------------------------------------

SELECT 'test 2.1', id
    FROM rt_band_properties_test
    WHERE st_value(rast, 1, st_makepoint(st_upperleftx(rast), st_upperlefty(rast))) != b1val;

SELECT 'test 2.2', id
    FROM rt_band_properties_test
    WHERE st_value(rast, 2, st_makepoint(st_upperleftx(rast), st_upperlefty(rast))) != b2val;

-----------------------------------------------------------------------
-- Test 3 - st_pixelaspolygon(rast raster, x integer, y integer)
-----------------------------------------------------------------------

SELECT 'test 3.1', id
    FROM rt_band_properties_test
    WHERE st_astext(st_pixelaspolygon(rast, 1, 1)) != geomtxt;

-----------------------------------------------------------------------
-- Test 4 - st_setvalue(rast raster, band integer, x integer, y integer, val float8)
-----------------------------------------------------------------------

SELECT 'test 4.1', id
    FROM rt_band_properties_test
    WHERE st_value(st_setvalue(rast, 1, 1, 1, 5), 1, 1, 1) != 5;
    
SELECT 'test 4.2', id
    FROM rt_band_properties_test
    WHERE NOT st_value(st_setvalue(rast, 1, 1, 1, NULL), 1, 1, 1) IS NULL;

SELECT 'test 4.3', id
    FROM rt_band_properties_test
    WHERE st_value(st_setvalue(rast, 1, 1, 1, NULL), 1, 1, 1, FALSE) != st_bandnodatavalue(rast, 1);

SELECT 'test 4.4', id
    FROM rt_band_properties_test
    WHERE st_value(st_setvalue(st_setbandnodatavalue(rast, NULL), 1, 1, 1, NULL), 1, 1, 1) != b1val;

-----------------------------------------------------------------------
-- Test 5 - st_setvalue(rast raster, band integer, pt geometry, val float8)
-----------------------------------------------------------------------

SELECT 'test 5.1', id
    FROM rt_band_properties_test
    WHERE st_value(st_setvalue(rast, 1, st_makepoint(st_upperleftx(rast), st_upperlefty(rast)), 3), 1, 1, 1) != 3;

