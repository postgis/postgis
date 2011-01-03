-----------------------------------------------------------------------
-- $Id$
--
-- Copyright (c) 2009 Mateusz Loskot <mateusz@loskot.net>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
-----------------------------------------------------------------------

-----------------------------------------------------------------------
-- st_scalex
-----------------------------------------------------------------------

SELECT id, name, scalex
    FROM rt_properties_test
    WHERE st_scalex(rast) != scalex;

-----------------------------------------------------------------------
-- st_scalex
-----------------------------------------------------------------------

SELECT id, name, scaley
    FROM rt_properties_test
    WHERE st_scaley(rast) != scaley;
