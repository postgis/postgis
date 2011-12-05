-----------------------------------------------------------------------
-- $Id$
--
-- Copyright (c) 2010 Mateusz Loskot <mateusz@loskot.net>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
-----------------------------------------------------------------------

-----------------------------------------------------------------------
--- Test RASTER_COLUMNS table
-----------------------------------------------------------------------

-- Check table exists
SELECT c.relname FROM pg_class c, pg_views v
  WHERE c.relname = v.viewname
    AND v.viewname = 'raster_columns'
