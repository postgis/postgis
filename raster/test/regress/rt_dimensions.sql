-----------------------------------------------------------------------
-- $Id$
--
-- Copyright (c) 2009 Mateusz Loskot <mateusz@loskot.net>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
-----------------------------------------------------------------------

-----------------------------------------------------------------------
--- st_width
-----------------------------------------------------------------------

SELECT id, name, width
    FROM rt_properties_test
    WHERE st_width(rast) != width;

-----------------------------------------------------------------------
--- st_height
-----------------------------------------------------------------------

SELECT id, name, height
    FROM rt_properties_test
    WHERE st_height(rast) != height;
