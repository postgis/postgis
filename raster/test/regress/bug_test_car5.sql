-- $Id$
--
-- Copyright (c) 2009 Mateusz Loskot <mateusz@loskot.net>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
-----------------------------------------------------------------------
--
-- Test case for mysterious band truncation revealed recently
-- on 32-bit platform. The problem was reproduced on 32-bit
-- Unix 32-bit system (Linux, Ubuntu 8.10) and was not reproduced on
-- Unix 64-bit system (Linux, Ubuntu 9.04).
-- 
-- There is likely a bug related to reaster data alignment.
-- Problem leaked in SVN trunk r3951.
-----------------------------------------------------------------------
BEGIN;
-- DROP TABLE IF EXISTS car5 CASCADE;
CREATE TABLE car5
(
  rid integer,
  rast raster
);
--
-- Test case: insert the same raster 3 times
--
-- Raster: 5 x 5 pixels, 3 bands, PT_8BUI pixel type, nodata = 0
--
INSERT INTO car5 (rid,rast) VALUES (1, ('01000003009A9999999999A93F9A9999999999A9BF000000E02B274A41000000007719564100000000000000000000000000000000FFFFFFFF050005000400FDFEFDFEFEFDFEFEFDF9FAFEFEFCF9FBFDFEFEFDFCFAFEFEFE04004E627AADD16076B4F9FE6370A9F5FE59637AB0E54F58617087040046566487A1506CA2E3FA5A6CAFFBFE4D566DA4CB3E454C5665')::raster );
INSERT INTO car5 (rid,rast) VALUES (2, ('01000003009A9999999999A93F9A9999999999A9BF000000E02B274A41000000007719564100000000000000000000000000000000FFFFFFFF050005000400FDFEFDFEFEFDFEFEFDF9FAFEFEFCF9FBFDFEFEFDFCFAFEFEFE04004E627AADD16076B4F9FE6370A9F5FE59637AB0E54F58617087040046566487A1506CA2E3FA5A6CAFFBFE4D566DA4CB3E454C5665')::raster );
INSERT INTO car5 (rid,rast) VALUES (3, ('01000003009A9999999999A93F9A9999999999A9BF000000E02B274A41000000007719564100000000000000000000000000000000FFFFFFFF050005000400FDFEFDFEFEFDFEFEFDF9FAFEFEFCF9FBFDFEFEFDFCFAFEFEFE04004E627AADD16076B4F9FE6370A9F5FE59637AB0E54F58617087040046566487A1506CA2E3FA5A6CAFFBFE4D566DA4CB3E454C5665')::raster );

-- Run test
SELECT rid, st_width(rast), st_height(rast), st_bandpixeltype(rast,1), st_bandpixeltype(rast,2), st_bandpixeltype(rast,3) FROM car5;

DROP TABLE car5;

COMMIT;

-----------------------------------------------------------------------
-- Test results
-----------------------------------------------------------------------
-- Expected output included in file bug_test_car5_expected.
-- In case bug leaks, output includes incorrectly reported
-- pixel type, i.e. 1BB (code 0), example:
--BEGIN
--1|5|5|8BUI|8BUI|8BUI
--2|5|5|8BUI|1BB|1BB
--3|5|5|8BUI|8BUI|8BUI
--COMMIT
