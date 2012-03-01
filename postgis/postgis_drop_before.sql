-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--
-- $Id$
--
-- PostGIS - Spatial Types for PostgreSQL
-- http://postgis.refractions.net
--
-- Copyright (C) 2011-2012 Sandro Santilli <strk@keybit.net>
-- Copyright (C) 2010-2012 Regina Obe <lr@pcorp.us>
-- Copyright (C) 2009      Paul Ramsey <pramsey@cleverelephant.ca>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
--
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--
-- This file contains drop commands for obsoleted items that need
-- to be dropped _before_ upgrade of old functions.
-- Changes to this file affect postgis_upgrade*.sql script.
--
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DROP FUNCTION IF EXISTS AddGeometryColumn(varchar,varchar,varchar,varchar,integer,varchar,integer,boolean);
DROP FUNCTION IF EXISTS ST_MakeEnvelope(float8, float8, float8, float8);
--changed name of prec arg to be consistent with ST_AsGML/KML
DROP FUNCTION IF EXISTS ST_AsX3D(geometry, integer, integer); 
--changed name of arg: http://trac.osgeo.org/postgis/ticket/1606
DROP FUNCTION IF EXISTS UpdateGeometrySRID(varchar,varchar,varchar,varchar,integer);
