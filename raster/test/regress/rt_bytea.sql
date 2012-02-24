-----------------------------------------------------------------------
-- $Id$
--
-- Copyright (c) 2009 Mateusz Loskot <mateusz@loskot.net>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
-----------------------------------------------------------------------

CREATE TABLE rt_bytea_test (
        id numeric,
        name text,
        rast raster
    );

INSERT INTO rt_bytea_test 
VALUES ( 0, '10x20, ip:0.5,0.5 scale:2,3 skew:0,0 srid:10 width:10 height:20',
(
'01' -- little endian (uint8 ndr)
|| 
'0000' -- version (uint16 0)
||
'0000' -- nBands (uint16 0)
||
'0000000000000040' -- scaleX (float64 2)
||
'0000000000000840' -- scaleY (float64 3)
||
'000000000000E03F' -- ipX (float64 0.5)
||
'000000000000E03F' -- ipY (float64 0.5)
||
'0000000000000000' -- skewX (float64 0)
||
'0000000000000000' -- skewY (float64 0)
||
'0A000000' -- SRID (int32 10)
||
'0A00' -- width (uint16 10)
||
'1400' -- height (uint16 20)
)::raster
);

INSERT INTO rt_bytea_test 
VALUES ( 1, '1x1, ip:2.5,2.5 scale:5,5 skew:0,0, srid:1, width:1, height:1',
(
'01' -- little endian (uint8 ndr)
|| 
'0000' -- version (uint16 0)
||
'0000' -- nBands (uint16 0)
||
'0000000000001440' -- scaleX (float64 5)
||
'0000000000001440' -- scaleY (float64 5)
||
'0000000000000440' -- ipX (float64 2.5)
||
'0000000000000440' -- ipY (float64 2.5)
||
'0000000000000000' -- skewX (float64 0)
||
'0000000000000000' -- skewY (float64 0)
||
'0C000000' -- SRID (int32 12)
||
'0100' -- width (uint16 1)
||
'0100' -- height (uint16 1)
)::raster
);

INSERT INTO rt_bytea_test 
VALUES ( 2, '1x1, ip:7.5,2.5 scale:5,5 skew:0,0, srid:0, width:1, height:1',
(
'01' -- little endian (uint8 ndr)
|| 
'0000' -- version (uint16 0)
||
'0000' -- nBands (uint16 0)
||
'0000000000001440' -- scaleX (float64 5)
||
'0000000000001440' -- scaleY (float64 5)
||
'0000000000001E40' -- ipX (float64 2.5)
||
'0000000000000440' -- ipY (float64 2.5)
||
'0000000000000000' -- skewX (float64 0)
||
'0000000000000000' -- skewY (float64 0)
||
'00000000' -- SRID (int32 0)
||
'0100' -- width (uint16 1)
||
'0100' -- height (uint16 1)
)::raster
);

INSERT INTO rt_bytea_test 
VALUES ( 3, '1x1, ip:7.5,2.5 scale:5,5 skew:0,0, srid:-1, width:1, height:1',
(
'01' -- little endian (uint8 ndr)
|| 
'0000' -- version (uint16 0)
||
'0000' -- nBands (uint16 0)
||
'0000000000001440' -- scaleX (float64 5)
||
'0000000000001440' -- scaleY (float64 5)
||
'0000000000001E40' -- ipX (float64 2.5)
||
'0000000000000440' -- ipY (float64 2.5)
||
'0000000000000000' -- skewX (float64 0)
||
'0000000000000000' -- skewY (float64 0)
||
'FFFFFFFF' -- SRID (int32 -1)
||
'0100' -- width (uint16 1)
||
'0100' -- height (uint16 1)
)::raster
); 

-----------------------------------------------------------------------
--- Test HEX
-----------------------------------------------------------------------

SELECT 
	id,
    name
FROM rt_bytea_test
WHERE
    encode(bytea(rast), 'hex') != encode(rast::bytea, 'hex')
    ;

-----------------------------------------------------------------------
--- Test Base64
-----------------------------------------------------------------------

SELECT 
	id,
    name
FROM rt_bytea_test
WHERE
    encode(bytea(rast), 'base64') != encode(rast::bytea, 'base64')
    ;

-----------------------------------------------------------------------
--- Test Binary
-----------------------------------------------------------------------

SELECT 
	id,
    name
FROM rt_bytea_test
WHERE
    encode(st_asbinary(rast), 'base64') != encode(rast::bytea, 'base64')
    ;

-- Cleanup
DROP TABLE rt_bytea_test;
