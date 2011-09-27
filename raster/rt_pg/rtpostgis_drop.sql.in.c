-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--
-- $Id: rtpostgis_drop.sql.in.c 7884 2011-09-22 15:07:25Z robe $
--
-- PostGIS Raster - Raster Type for PostGIS
-- http://trac.osgeo.org/postgis/wiki/WKTRaster
--
-- Copyright (C) 2011 Regina Obe
--   <lr@pcorp.us>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
--
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--
-- WARNING: Any change in this file must be evaluated for compatibility.
--
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
-- This file will be used to drop obselete functions and other objects.
-- It will be used for both upgrade and uninstall
-- Drop obsolete functions (which fully obsolete or changed to take default args) --
DROP FUNCTION IF EXISTS ST_Intersects(raster,boolean,geometry);
DROP FUNCTION IF EXISTS ST_Intersects(geometry,raster,boolean);
DROP FUNCTION IF EXISTS ST_Intersects(raster,geometry);
DROP FUNCTION IF EXISTS ST_Intersects(geometry,raster);
DROP FUNCTION IF EXISTS ST_Intersects(raster rast , integer band , boolean exclude_nodata_value , geometry geommin);
DROP FUNCTION IF EXISTS ST_Intersects(geometry geommin , raster rast , integer band , boolean exclude_nodata_value);

--changed to take default args --
DROP FUNCTION IF EXISTS ST_Intersection(geometry,raster);

--added extra parameter so these are obsolete --
DROP FUNCTION IF EXISTS ST_AsRaster(geometry , integer , integer , double precision , double precision , text , double precision , double precision , double precision , double precision );
DROP FUNCTION IF EXISTS ST_AsRaster(geometry , integer , integer , text[] , double precision[] , double precision[] , double precision , double precision , double precision , double precision );
DROP FUNCTION IF EXISTS ST_AsRaster(geometry , integer , integer , text , double precision , double precision , double precision , double precision , double precision , double precision );
DROP FUNCTION IF EXISTS ST_AsRaster(geometry , integer , integer , double precision , double precision , text[] , double precision[] , double precision[] , double precision , double precision );
DROP FUNCTION IF EXISTS ST_AsRaster(geometry , integer , integer , double precision , double precision , text[] , double precision[] , double precision[] , double precision , double precision );
FUNCTION ST_AsRaster(geometry , double precision , double precision , text , double precision , double precision , double precision , double precision , double precision , double precision )
DROP FUNCTION IF EXISTS ST_AsRaster(geometry , raster , text , double precision , double precision );
DROP FUNCTION IF EXISTS  _ST_AsRaster(
	geom geometry,
	scalex double precision , scaley double precision,
	width integer , height integer,
	pixeltype text[] ,
	value double precision[] ,
	nodataval double precision[] ,
	upperleftx double precision, upperlefty double precision,
	gridx double precision, gridy double precision,
	skewx double precision, skewy double precision,
	touched boolean);
