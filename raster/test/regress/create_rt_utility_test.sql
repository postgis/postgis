-----------------------------------------------------------------------
-- $Id$
--
-- Copyright (c) 2009 Pierre Racine <pierre.racine@sbf.ulaval.ca>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
-----------------------------------------------------------------------

-----------------------------------------------------------------------
--- Test of "Get" functions for properties of the raster.
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
