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

SELECT id, name, skewx
    FROM rt_properties_test
    WHERE st_skewx(rast) != skewx;

-----------------------------------------------------------------------
-- st_skewy
-----------------------------------------------------------------------

SELECT id, name, skewy
    FROM rt_properties_test
    WHERE st_skewy(rast) != skewy;

-----------------------------------------------------------------------
-- st_setrotation
-----------------------------------------------------------------------

SELECT id, name, st_rotation(rast) as rotation
    FROM rt_properties_test
    WHERE st_rotation(rast) != 0;

INSERT INTO rt_properties_test
    (id, name, srid, width, height, scalex, scaley, ipx, ipy, skewx, skewy, rast)
    (SELECT id + 100, name, srid, width, height, scalex, scaley, ipx, ipy, skewx, skewy, st_setrotation(rast,0)
    FROM rt_properties_test
    WHERE st_rotation(rast) != 0);

UPDATE rt_properties_test
    SET scalex = st_scalex(rast),
        scaley = st_scaley(rast),
        skewx = st_skewx(rast),
        skewy = st_skewy(rast),
        ipx = st_upperleftx(rast),
        ipy = st_upperlefty(rast)
    WHERE id > 100;

SELECT id, scalex, scaley, skewx, skewy, st_rotation(rast)
    FROM rt_properties_test
    WHERE id > 100;

UPDATE rt_properties_test
    SET rast = st_setrotation(rast,pi()/4)
    WHERE id > 100;

UPDATE rt_properties_test
    SET scalex = st_scalex(rast),
        scaley = st_scaley(rast),
        skewx = st_skewx(rast),
        skewy = st_skewy(rast),
        ipx = st_upperleftx(rast),
        ipy = st_upperlefty(rast)
    WHERE id > 100;

SELECT id, scalex, scaley, skewx, skewy, st_rotation(rast)
    FROM rt_properties_test
    WHERE id > 100;

UPDATE rt_properties_test
    SET rast = st_setrotation(rast,pi()/6)
    WHERE id > 100;

UPDATE rt_properties_test
    SET scalex = st_scalex(rast),
        scaley = st_scaley(rast),
        skewx = st_skewx(rast),
        skewy = st_skewy(rast),
        ipx = st_upperleftx(rast),
        ipy = st_upperlefty(rast)
    WHERE id > 100;

SELECT id, scalex, scaley, skewx, skewy, st_rotation(rast)
    FROM rt_properties_test
    WHERE id > 100;

DELETE FROM rt_properties_test
    WHERE id > 100;
