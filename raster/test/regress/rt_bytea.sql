-----------------------------------------------------------------------
-- $Id$
--
-- Copyright (c) 2009 Mateusz Loskot <mateusz@loskot.net>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
-----------------------------------------------------------------------

-----------------------------------------------------------------------
--- Test HEX
-----------------------------------------------------------------------

SELECT 
	id,
    name
FROM rt_bytea_test
WHERE
    encode(bytea(rast), 'hex') != encode(rast::bytea, 'hex')
    OR
    encode(bytea(rast), 'hex') != encode(rast, 'hex')
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
    OR
    encode(bytea(rast), 'base64') != encode(rast, 'base64')
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
    OR
    encode(st_asbinary(rast), 'base64') != encode(rast, 'base64')
    ;
