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
-- Drop obsolete functions 
-- (which fully obsolete, changed to take default args, or outp params changed) --

DROP FUNCTION IF EXISTS st_summarystats(rastertable text, rastercolumn text, nband integer, exclude_nodata_value boolean, sample_percent double precision) ;
DROP FUNCTION IF EXISTS st_summarystats(rastertable text, rastercolumn text, exclude_nodata_value boolean) ;
DROP FUNCTION IF EXISTS st_summarystats(rast raster, exclude_nodata_value boolean) ;
DROP FUNCTION IF EXISTS st_summarystats(rast raster, nband integer, exclude_nodata_value boolean) ;

DROP FUNCTION IF EXISTS _st_summarystats(raster, integer, exclude_nodata_value boolean , double precision, bigint, double precision, double precision);
DROP FUNCTION IF EXISTS _st_summarystats(rast raster, nband integer, exclude_nodata_value boolean, sample_percent double precision) ;

--return types changed evidentally. Dropping first allows us to recreate cleanly --
DROP FUNCTION IF EXISTS st_valuecount(text, text, integer, double precision, double precision);
DROP FUNCTION IF EXISTS st_valuecount(text, text, integer, boolean, double precision[], double precision);
DROP FUNCTION IF EXISTS st_valuecount(text, text, double precision[], double precision);
DROP FUNCTION IF EXISTS st_valuecount(text, text, integer, double precision[], double precision);
DROP FUNCTION IF EXISTS st_valuecount(text, text, integer, boolean, double precision, double precision);
DROP FUNCTION IF EXISTS st_valuecount(text, text, double precision, double precision);
DROP FUNCTION IF EXISTS st_valuecount(raster, integer, boolean, double precision[], double precision);
DROP FUNCTION IF EXISTS st_valuecount(raster, integer, double precision[], double precision);
DROP FUNCTION IF EXISTS st_valuecount(raster, double precision[], double precision);
DROP FUNCTION IF EXISTS _st_valuecount(text, text, integer, boolean, double precision[], double precision);
DROP FUNCTION IF EXISTS _st_valuecount(raster, integer, boolean, double precision[], double precision);

DROP FUNCTION IF EXISTS ST_Intersects(raster,boolean,geometry);
DROP FUNCTION IF EXISTS ST_Intersects(geometry,raster,boolean);
DROP FUNCTION IF EXISTS ST_Intersects(raster,geometry);
DROP FUNCTION IF EXISTS ST_Intersects(geometry,raster);
DROP FUNCTION IF EXISTS ST_Intersects(raster, integer, boolean  , geometry);
DROP FUNCTION IF EXISTS ST_Intersects(geometry , raster, integer , boolean);
DROP FUNCTION IF EXISTS ST_Intersection(raster,raster, integer, integer);
DROP FUNCTION IF EXISTS ST_Intersection(geometry,raster);
DROP FUNCTION IF EXISTS ST_Intersection(raster, geometry);
DROP FUNCTION IF EXISTS ST_Intersection(raster, integer, geometry);

--these were renamed to ST_MapAlgebraExpr --
DROP FUNCTION IF EXISTS ST_MapAlgebra(raster, integer, text, text, nodatavaluerepl text);
DROP FUNCTION IF EXISTS ST_MapAlgebra(raster, pixeltype text, expression text, nodatavaluerepl text);

--added extra parameter so these are obsolete --
DROP FUNCTION IF EXISTS ST_AsRaster(geometry , integer , integer , double precision , double precision , text , double precision , double precision , double precision , double precision );
DROP FUNCTION IF EXISTS ST_AsRaster(geometry , integer , integer , text[] , double precision[] , double precision[] , double precision , double precision , double precision , double precision );
DROP FUNCTION IF EXISTS ST_AsRaster(geometry , integer , integer , text , double precision , double precision , double precision , double precision , double precision , double precision );
DROP FUNCTION IF EXISTS ST_AsRaster(geometry , integer , integer , double precision , double precision , text[] , double precision[] , double precision[] , double precision , double precision );
DROP FUNCTION IF EXISTS ST_AsRaster(geometry , integer , integer , double precision , double precision , text[] , double precision[] , double precision[] , double precision , double precision );
DROP FUNCTION IF EXISTS ST_AsRaster(geometry , double precision , double precision , text , double precision , double precision , double precision , double precision , double precision , double precision );
DROP FUNCTION IF EXISTS ST_AsRaster(geometry , raster , text , double precision , double precision );
DROP FUNCTION IF EXISTS _ST_AsRaster(geometry,double precision , double precision, integer , integer,text[] , double precision[] ,double precision[] ,  double precision,  double precision, double precision,double precision, double precision, double precision,touched boolean);
-- arg names changed
DROP FUNCTION IF EXISTS _ST_Resample(raster, text, double precision, integer, double precision, double precision, double precision, double precision, double precision, double precision);

-- default parameters added
DROP FUNCTION IF EXISTS ST_HasNoBand(raster);

--function out parameters changed so can not just create or replace
DROP FUNCTION IF EXISTS ST_BandMetaData(raster, integer);
