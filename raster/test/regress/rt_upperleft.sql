-----------------------------------------------------------------------
-- $Id$
--
-- Copyright (c) 2009 Mateusz Loskot <mateusz@loskot.net>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
-----------------------------------------------------------------------

-----------------------------------------------------------------------
-- st_upperleftx
-----------------------------------------------------------------------

SELECT id, name, ipx, st_upperleftx(rast) as ipx_expected
    FROM rt_properties_test
    WHERE st_upperleftx(rast) != ipx;

-----------------------------------------------------------------------
-- st_upperlefty
-----------------------------------------------------------------------

SELECT id, name, ipy, st_upperlefty(rast) as ipy_expected
    FROM rt_properties_test
    WHERE st_upperlefty(rast) != ipy;
