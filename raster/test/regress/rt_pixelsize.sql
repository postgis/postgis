-----------------------------------------------------------------------
-- $Id$
--
-- Copyright (c) 2009 Mateusz Loskot <mateusz@loskot.net>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
-----------------------------------------------------------------------

-----------------------------------------------------------------------
-- st_pixelsizex
-----------------------------------------------------------------------

SELECT id, name, scalex
    FROM rt_properties_test
    WHERE st_pixelsizex(rast) != scalex;

-----------------------------------------------------------------------
-- st_pixelsizex
-----------------------------------------------------------------------

SELECT id, name, scaley
    FROM rt_properties_test
    WHERE st_pixelsizey(rast) != scaley;
