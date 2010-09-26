-----------------------------------------------------------------------
-- $Id$
--
-- Copyright (c) 2009 Mateusz Loskot <mateusz@loskot.net>
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
