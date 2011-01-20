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

CREATE TABLE rt_band_properties_test (
    id numeric,
    description text,
    nbband integer,
    b1pixeltype text,
    b1hasnodatavalue boolean,
    b1nodatavalue float4,
    b1val float4,
    b2pixeltype text,
    b2hasnodatavalue boolean,
    b2nodatavalue float4,
    b2val float4,
    geomtxt text,
    rast raster
);

INSERT INTO rt_band_properties_test 
VALUES ( 1, '1x1, nbband:2 b1pixeltype:4BUI b1hasnodatavalue:true b1nodatavalue:3 b2pixeltype:16BSI b2hasnodatavalue:false b2nodatavalue:13',
        2, --- nbband
        '4BUI', true, 3, 2,   --- b1pixeltype, b1hasnodatavalue, b1nodatavalue
        '16BSI', false, 13, 4, --- b2pixeltype, b2hasnodatavalue, b2nodatavalue
        'POLYGON((782325.5 26744042.5,782330.5 26744045.5,782333.5 26744040.5,782328.5 26744037.5,782325.5 26744042.5))',
(
'01' -- big endian (uint8 xdr)
|| 
'0000' -- version (uint16 0)
||
'0200' -- nBands (uint16 2)
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
'73E58A00' -- SRID (int32 9102707)
||
'0100' -- width (uint16 1)
||
'0100' -- height (uint16 1)
||
'4' -- hasnodatavalue set to true
||
'2' -- first band type (4BUI) 
||
'03' -- novalue==3
||
'02' -- pixel(0,0)==2
||
'0' -- hasnodatavalue set to false
||
'5' -- second band type (16BSI)
||
'0D00' -- novalue==13
||
'0400' -- pixel(0,0)==4
)::raster
);

INSERT INTO rt_band_properties_test 
VALUES ( 2, '1x1, nbband:2 b1pixeltype:4BUI b1hasnodatavalue:true b1nodatavalue:3 b2pixeltype:16BSI b2hasnodatavalue:false b2nodatavalue:13',
        2, --- nbband
        '4BUI', true, 3, 2,   --- b1pixeltype, b1hasnodatavalue, b1nodatavalue
        '16BSI', false, 13, 4, --- b2pixeltype, b2hasnodatavalue, b2nodatavalue
        'POLYGON((-75.5533328537098 49.2824585505576,-75.5525268884758 49.2826703629415,-75.5523150760919 49.2818643977075,-75.553121041326 49.2816525853236,-75.5533328537098 49.2824585505576))',
(
'01' -- little endian (uint8 ndr)
|| 
'0000' -- version (uint16 0)
||
'0200' -- nBands (uint16 0)
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
'0100' -- width (uint16 1)
||
'0100' -- height (uint16 1)
||
'4' -- hasnodatavalue set to true
||
'2' -- first band type (4BUI) 
||
'03' -- novalue==3
||
'02' -- pixel(0,0)==2
||
'0' -- hasnodatavalue set to false
||
'5' -- second band type (16BSI)
||
'0D00' -- novalue==13
||
'0400' -- pixel(0,0)==4
)::raster
);

INSERT INTO rt_band_properties_test 
VALUES ( 3, '1x1, nbband:2 b1pixeltype:4BUI b1hasnodatavalue:true b1nodatavalue:3 b2pixeltype:16BSI b2hasnodatavalue:false b2nodatavalue:13',
        2, --- nbband
        '4BUI', true, 3, 2,   --- b1pixeltype, b1hasnodatavalue, b1nodatavalue
        '16BSI', false, 13, 4, --- b2pixeltype, b2hasnodatavalue, b2nodatavalue
        'POLYGON((-75.5533328537098 49.2824585505576,-75.5525268884758 49.2826703629415,-75.5523150760919 49.2818643977075,-75.553121041326 49.2816525853236,-75.5533328537098 49.2824585505576))',
(
'01' -- little endian (uint8 ndr)
|| 
'0000' -- version (uint16 0)
||
'0200' -- nBands (uint16 0)
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
'0100' -- width (uint16 1)
||
'0100' -- height (uint16 1)
||
'4' -- hasnodatavalue set to true
||
'2' -- first band type (4BUI) 
||
'03' -- novalue==3
||
'03' -- pixel(0,0)==3 (same that nodata)
||
'0' -- hasnodatavalue set to false
||
'5' -- second band type (16BSI)
||
'0D00' -- novalue==13
||
'0400' -- pixel(0,0)==4
)::raster
);
