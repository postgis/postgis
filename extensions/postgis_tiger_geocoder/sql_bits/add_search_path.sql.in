-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
-- 
-- $Id: add_search_path.sql.in 10934 2012-12-26 13:44:51Z robe $
----
-- PostGIS - Spatial Types for PostgreSQL
-- http://postgis.net
--
-- Copyright (C) 2012 Regina Obe <lr@pcorp.us>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
--
-- Author: Regina Obe <lr@pcorp.us>
--  
-- This adds the tiger schema to search path
-- Functions in tiger are not schema qualified 
-- so this is needed for them to work

SELECT postgis_extension_AddToSearchPath('tiger');
