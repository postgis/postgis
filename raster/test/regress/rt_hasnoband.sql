-----------------------------------------------------------------------
-- $Id$
--
-- Copyright (c) 2011 Jorge Arevalo <jorge.arevalo@deimos-space.com>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
-----------------------------------------------------------------------

-------------------------------------------------------------------
-- st_hasnodata
-----------------------------------------------------------------------
select st_hasnoband(rast) from empty_raster_test;