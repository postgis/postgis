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
SELECT c.relname FROM pg_class c, pg_tables t
  WHERE c.relname = t.tablename
    AND t.tablename = 'raster_columns'