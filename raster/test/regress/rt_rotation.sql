-----------------------------------------------------------------------
-- $Id$
--
-- Copyright (c) 2009 Mateusz Loskot <mateusz@loskot.net>
-- Copyright (c) 2011 David Zwarg <dzwarg@azavea.com>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
-----------------------------------------------------------------------

-----------------------------------------------------------------------
-- st_skewx
-----------------------------------------------------------------------

SELECT 'T1', id, name, skewx
    FROM rt_properties_test
    WHERE st_skewx(rast) != skewx;

-----------------------------------------------------------------------
-- st_skewy
-----------------------------------------------------------------------

SELECT 'T2', id, name, skewy
    FROM rt_properties_test
    WHERE st_skewy(rast) != skewy;

-----------------------------------------------------------------------
-- st_setrotation
-----------------------------------------------------------------------

SELECT 'T3', id, name, round(st_rotation(rast)*1000000000000) as rotation
    FROM rt_properties_test
    WHERE st_rotation(rast) != 0;

INSERT INTO rt_properties_test
    (id, name, srid, width, height, scalex, scaley, ipx, ipy, skewx, skewy, rast)
    (SELECT id + 100, name, srid, width, height, scalex, scaley, ipx, ipy, skewx, skewy, st_setrotation(rast,0)
    FROM rt_properties_test
    WHERE st_rotation(rast) != 0);

UPDATE rt_properties_test
    SET scalex = round(st_scalex(rast)*1000000000000),
        scaley = round(st_scaley(rast)*1000000000000),
        skewx = round(st_skewx(rast)*1000000000000),
        skewy = round(st_skewy(rast)*1000000000000),
        ipx = round(st_upperleftx(rast)*1000000000000),
        ipy = round(st_upperlefty(rast)*1000000000000)
    WHERE id > 100;

SELECT 'T4', id, scalex, scaley, abs(skewx), abs(skewy), st_rotation(rast)
    FROM rt_properties_test
    WHERE id > 100;

UPDATE rt_properties_test
    SET rast = st_setrotation(rast,pi()/4)
    WHERE id > 100;

UPDATE rt_properties_test
    SET scalex = round(st_scalex(rast)*1000000000000),
        scaley = round(st_scaley(rast)*1000000000000),
        skewx = round(st_skewx(rast)*1000000000000),
        skewy = round(st_skewy(rast)*1000000000000),
        ipx = round(st_upperleftx(rast)*1000000000000),
        ipy = round(st_upperlefty(rast)*1000000000000)
    WHERE id > 100;

SELECT 'T5', id, scalex, scaley, skewx, skewy, 
    round(st_rotation(rast)*1000000000000)
    FROM rt_properties_test
    WHERE id > 100;

UPDATE rt_properties_test
    SET rast = st_setrotation(rast,pi()/6)
    WHERE id > 100;

UPDATE rt_properties_test
    SET scalex = round(st_scalex(rast)*1000000000000),
        scaley = round(st_scaley(rast)*1000000000000),
        skewx = round(st_skewx(rast)*1000000000000),
        skewy = round(st_skewy(rast)*1000000000000),
        ipx = round(st_upperleftx(rast)*1000000000000),
        ipy = round(st_upperlefty(rast)*1000000000000)
    WHERE id > 100;

SELECT 'T6', id, scalex, scaley, skewx, skewy, 
    round(st_rotation(rast)*1000000000000)
    FROM rt_properties_test
    WHERE id > 100;

DELETE FROM rt_properties_test
    WHERE id > 100;
