-----------------------------------------------------------------------
-- $Id$
--
-- Copyright (c) 2011 David Zwarg <dzwarg@azavea.com>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
-----------------------------------------------------------------------

-----------------------------------------------------------------------
-- st_pixelwidth
-----------------------------------------------------------------------

SELECT id, name, scalex, skewy
    FROM rt_properties_test
    WHERE NOT sqrt(scalex*scalex + skewy*skewy) = st_pixelwidth(rast);

-----------------------------------------------------------------------
-- st_pixelheight
-----------------------------------------------------------------------

SELECT id, name, scaley, skewx
    FROM rt_properties_test
    WHERE NOT sqrt(scaley*scaley + skewx*skewx) = st_pixelheight(rast);
