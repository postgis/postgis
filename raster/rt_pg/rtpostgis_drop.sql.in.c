-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--
-- $Id: rtpostgis_drop.sql.in.c 7884 2011-09-22 15:07:25Z robe $
--
-- PostGIS Raster - Raster Type for PostGIS
-- http://trac.osgeo.org/postgis/wiki/WKTRaster
--
-- Copyright (C) 2011 Regina Obe <lr@pcorp.us>
-- Copyright (C) 2011-2012 Regents of the University of California
--   <bkpark@ucdavis.edu>
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

-- drop aggregates
DROP AGGREGATE IF EXISTS ST_Union(raster, text, text, text, double precision, text, text, text, double precision, text, text, text, double precision);
DROP AGGREGATE IF EXISTS ST_Union(raster, text, text, text);
DROP AGGREGATE IF EXISTS ST_Union(raster, text, text, text, double precision, text, text, text, double precision);
DROP AGGREGATE IF EXISTS ST_Union(raster, text, text);
DROP AGGREGATE IF EXISTS ST_Union(raster, text, text, text, double precision);
DROP AGGREGATE IF EXISTS ST_Union(raster, text);
DROP AGGREGATE IF EXISTS ST_Union(raster, integer);
DROP AGGREGATE IF EXISTS ST_Union(raster);

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

--these were renamed to ST_MapAlgebraExpr or argument names changed --
DROP FUNCTION IF EXISTS ST_MapAlgebra(raster, integer, text, text, nodatavaluerepl text);
DROP FUNCTION IF EXISTS ST_MapAlgebra(raster, pixeltype text, expression text, nodatavaluerepl text);


--signatures or arg names changed
DROP FUNCTION IF EXISTS ST_MapAlgebraExpr(raster, integer, text, text, text);
DROP FUNCTION IF EXISTS ST_MapAlgebraExpr(raster, text, text, text);
DROP FUNCTION IF EXISTS ST_MapalgebraFct(raster, regprocedure);
DROP FUNCTION IF EXISTS ST_MapAlgebraFct(raster, text, regprocedure, VARIADIC text[]); 
DROP FUNCTION IF EXISTS ST_MapAlgebraFct(raster, text, regprocedure); 
DROP FUNCTION IF EXISTS ST_MapAlgebraFct(raster, regprocedure, VARIADIC text[]);
DROP FUNCTION IF EXISTS ST_MapAlgebraFct(raster, integer, regprocedure, variadic text[]);
DROP FUNCTION IF EXISTS ST_MapAlgebraFct(raster, integer, text, regprocedure, VARIADIC text[]); 
DROP FUNCTION IF EXISTS ST_MapAlgebraFct(raster, integer, text, regprocedure); 
DROP FUNCTION IF EXISTS ST_MapAlgebraFct(raster, integer, regprocedure, variadic text[]);
DROP FUNCTION IF EXISTS ST_MapalgebraFct(raster, integer, regprocedure);
DROP FUNCTION IF EXISTS ST_MapAlgebraFct(raster, raster, regprocedure, text, text, VARIADIC text[]);
DROP FUNCTION IF EXISTS ST_MapAlgebraFct(raster, integer, raster, integer, regprocedure, text, text, VARIADIC text[]);
DROP FUNCTION IF EXISTS ST_MapAlgebraFctNgb(raster, integer, text, integer, integer, regprocedure, text,  VARIADIC text[]);


--dropped functions
DROP FUNCTION IF EXISTS  ST_MapAlgebraFct(raster, raster, regprocedure, VARIADIC text[]);

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

-- signature changed
DROP FUNCTION IF EXISTS ST_Resample(raster, raster, text, double precision);

-- default parameters added
DROP FUNCTION IF EXISTS ST_HasNoBand(raster);

--function out parameters changed so can not just create or replace
DROP FUNCTION IF EXISTS ST_BandMetaData(raster, integer);

--function out parameter changed
DROP FUNCTION IF EXISTS ST_BandNoDataValue(raster, integer);
--function no longer exists
DROP FUNCTION IF EXISTS ST_BandNoDataValue(raster);

--function no longer exists
DROP FUNCTION IF EXISTS ST_SetGeoReference(raster, text);
-- signature changed
DROP FUNCTION IF EXISTS ST_SetGeoReference(raster, text, text);

--function no longer exists
DROP FUNCTION IF EXISTS st_setbandisnodata(raster);
-- signature changed
DROP FUNCTION IF EXISTS st_setbandisnodata(raster, integer);

--function no longer exists
DROP FUNCTION IF EXISTS st_setbandnodatavalue(raster, integer, double precision);
-- signature changed
DROP FUNCTION IF EXISTS st_setbandnodatavalue(raster, integer, double precision, boolean);

--function no longer exists
DROP FUNCTION IF EXISTS st_dumpaspolygons(raster);
-- signature changed
DROP FUNCTION IF EXISTS st_dumpaspolygons(raster, integer);

--function no longer exists
DROP FUNCTION IF EXISTS st_polygon(raster);
-- signature changed
DROP FUNCTION IF EXISTS st_polygon(raster, integer);

-- function no longer exists
DROP FUNCTION IF EXISTS st_makeemptyraster(int, int, float8, float8, float8, float8, float8, float8);
-- signature changed
DROP FUNCTION IF EXISTS st_makeemptyraster(int, int, float8, float8, float8, float8, float8, float8, int4);

-- function no longer exists
DROP FUNCTION IF EXISTS st_addband(raster, text);
DROP FUNCTION IF EXISTS st_addband(raster, text, float8);
DROP FUNCTION IF EXISTS st_addband(raster, int, text);
DROP FUNCTION IF EXISTS st_addband(raster, int, text, float8);
DROP FUNCTION IF EXISTS st_addband(raster, raster, int);
DROP FUNCTION IF EXISTS st_addband(raster, raster);
-- signature changed
DROP FUNCTION IF EXISTS st_addband(raster, text, float8, float8);
DROP FUNCTION IF EXISTS st_addband(raster, int, text, float8, float8);
DROP FUNCTION IF EXISTS st_addband(raster, raster, int, int);

-- function no longer exists
DROP FUNCTION IF EXISTS st_bandisnodata(raster);
DROP FUNCTION IF EXISTS st_bandisnodata(raster, integer);
-- signature changed
DROP FUNCTION IF EXISTS st_bandisnodata(raster, integer, boolean);

-- function no longer exists
DROP FUNCTION IF EXISTS st_bandpath(raster);
-- signature changed
DROP FUNCTION IF EXISTS st_bandpath(raster, integer);

-- function no longer exists
DROP FUNCTION IF EXISTS st_bandpixeltype(raster);
-- signature changed
DROP FUNCTION IF EXISTS st_bandpixeltype(raster, integer);


-- signature changed and some functions dropped --
-- Note: I am only including the surviving variants 
-- since some people may be using the dead ones which are in scripts 
-- and we do not have a replace for those
DROP AGGREGATE IF EXISTS ST_Union(raster);
DROP AGGREGATE IF EXISTS ST_Union(raster, integer, text); 

-- function no longer exists
DROP FUNCTION IF EXISTS st_value(raster, integer, integer, integer);
DROP FUNCTION IF EXISTS st_value(raster, integer, integer);
DROP FUNCTION IF EXISTS st_value(raster, integer, geometry);
DROP FUNCTION IF EXISTS st_value(raster, geometry);
-- signature changed
DROP FUNCTION IF EXISTS st_value(raster, integer, integer, integer, boolean);
DROP FUNCTION IF EXISTS st_value(raster, integer, integer, boolean);
DROP FUNCTION IF EXISTS st_value(raster, integer, geometry, boolean);
DROP FUNCTION IF EXISTS st_value(raster, geometry, boolean);

-- function no longer exists
DROP FUNCTION IF EXISTS st_georeference(raster);
-- signature changed
DROP FUNCTION IF EXISTS st_georeference(raster, text);

-- function name change
DROP FUNCTION IF EXISTS dumpaswktpolygons(raster, integer);

-- signature changed
DROP FUNCTION IF EXISTS st_bandmetadata(raster, VARIADIC int[]);

--change to use default parameters
DROP FUNCTION IF EXISTS ST_PixelAsPolygons(raster); 
DROP FUNCTION IF EXISTS ST_PixelAsPolygons(raster,integer);

-- no longer needed functions changed to use out parameters
DROP TYPE IF EXISTS bandmetadata;
DROP TYPE IF EXISTS geomvalxy;

-- raster_columns and raster_overviews tables are deprecated
DROP FUNCTION IF EXISTS _rename_raster_tables();
CREATE OR REPLACE FUNCTION _rename_raster_tables()
	RETURNS void AS $$
	DECLARE
		cnt int;
	BEGIN
		SELECT count(*) INTO cnt
		FROM pg_class c
		JOIN pg_namespace n
			ON c.relnamespace = n.oid
		WHERE c.relname = 'raster_columns'
			AND c.relkind = 'r'::char
			AND NOT pg_is_other_temp_schema(c.relnamespace);

		IF cnt > 0 THEN
			EXECUTE 'ALTER TABLE raster_columns RENAME TO deprecated_raster_columns';
		END IF;

		SELECT count(*) INTO cnt
		FROM pg_class c
		JOIN pg_namespace n
			ON c.relnamespace = n.oid
		WHERE c.relname = 'raster_overviews'
			AND c.relkind = 'r'::char
			AND NOT pg_is_other_temp_schema(c.relnamespace);

		IF cnt > 0 THEN
			EXECUTE 'ALTER TABLE raster_overviews RENAME TO deprecated_raster_overviews';
		END IF;

	END;
	$$ LANGUAGE 'plpgsql' VOLATILE;
SELECT _rename_raster_tables();
DROP FUNCTION _rename_raster_tables();

-- inserted new column into view
DROP VIEW IF EXISTS raster_columns;

-- functions no longer supported
DROP FUNCTION IF EXISTS AddRasterColumn(varchar, varchar, varchar, varchar, integer, varchar[], boolean, boolean, double precision[], double precision, double precision, integer, integer, geometry);
DROP FUNCTION IF EXISTS AddRasterColumn(varchar, varchar, varchar, integer, varchar[], boolean, boolean, double precision[], double precision, double precision, integer, integer, geometry);
DROP FUNCTION IF EXISTS AddRasterColumn(varchar, varchar, integer, varchar[], boolean, boolean, double precision[], double precision, double precision, integer, integer, geometry);
DROP FUNCTION IF EXISTS DropRasterColumn(varchar, varchar, varchar, varchar);
DROP FUNCTION IF EXISTS DropRasterColumn(varchar, varchar, varchar);
DROP FUNCTION IF EXISTS DropRasterColumn(varchar, varchar);
DROP FUNCTION IF EXISTS DropRasterTable(varchar, varchar, varchar);
DROP FUNCTION IF EXISTS DropRasterTable(varchar, varchar);
DROP FUNCTION IF EXISTS DropRasterTable(varchar);

-- function parameters added
DROP FUNCTION IF EXISTS AddRasterConstraints(name, name, name, boolean, boolean, boolean, boolean, boolean, boolean, boolean, boolean, boolean, boolean, boolean);
DROP FUNCTION IF EXISTS AddRasterConstraints(name, name, boolean, boolean, boolean, boolean, boolean, boolean, boolean, boolean, boolean, boolean, boolean);
DROP FUNCTION IF EXISTS DropRasterConstraints(name, name, name, boolean, boolean, boolean, boolean, boolean, boolean, boolean, boolean, boolean, boolean, boolean);
DROP FUNCTION IF EXISTS DropRasterConstraints(name, name, boolean, boolean, boolean, boolean, boolean, boolean, boolean, boolean, boolean, boolean, boolean);

-- function parameters renamed
CREATE OR REPLACE FUNCTION _drop_st_samealignment()
	RETURNS void AS $$
	DECLARE
		cnt int;
	BEGIN
		SELECT count(*) INTO cnt
		FROM pg_proc
		WHERE lower(proname) = 'st_samealignment'
			AND pronargs = 2
			AND (
				proargnames = '{rasta,rastb}'::text[] OR
				proargnames = '{rastA,rastB}'::text[]
			);

		IF cnt > 0 THEN
			RAISE NOTICE 'Dropping ST_SameAlignment(raster, raster) due to parameter name changes.  Unfortunately, this is a DROP ... CASCADE as the alignment raster constraint uses ST_SameAlignment(raster, raster).  You will need to reapply AddRasterConstraint(''SCHEMA'', ''TABLE'', ''COLUMN'', ''alignment'') to any raster column that requires this constraint.';
			DROP FUNCTION IF EXISTS st_samealignment(raster, raster) CASCADE;
		END IF;
	END;
	$$ LANGUAGE 'plpgsql' VOLATILE;
SELECT _drop_st_samealignment();
DROP FUNCTION _drop_st_samealignment();
DROP FUNCTION IF EXISTS _st_intersects(raster, integer, raster, integer);
DROP FUNCTION IF EXISTS st_intersects(raster, integer, raster, integer);
DROP FUNCTION IF EXISTS st_intersects(raster, raster);

-- functions have changed dramatically
DROP FUNCTION IF EXISTS st_intersection(rast raster, band integer, geom geometry);
DROP FUNCTION IF EXISTS st_intersection(rast raster, geom geometry);

-- function was renamed
DROP FUNCTION IF EXISTS st_minpossibleval(text);

-- function deprecated previously
DROP FUNCTION IF EXISTS st_pixelaspolygon(raster, integer, integer, integer);

-- function signatures changed
DROP FUNCTION IF EXISTS st_intersection(raster, int, geometry, text, regprocedure);
DROP FUNCTION IF EXISTS st_intersection(raster, int, geometry, regprocedure);
DROP FUNCTION IF EXISTS st_intersection(raster, geometry, text, regprocedure);
DROP FUNCTION IF EXISTS st_intersection(raster, geometry, regprocedure);
DROP FUNCTION IF EXISTS st_clip(raster, integer, geometry, boolean);
DROP FUNCTION IF EXISTS st_clip(raster, geometry, float8, boolean);
DROP FUNCTION IF EXISTS st_clip(raster, geometry, boolean);
DROP FUNCTION IF EXISTS st_clip(raster, int, geometry, float8, boolean);
DROP FUNCTION IF EXISTS st_clip(raster, geometry, float8[], boolean);
DROP FUNCTION IF EXISTS st_clip(raster, integer, geometry, float8[], boolean);

-- refactoring of functions
DROP FUNCTION IF EXISTS _st_dumpaswktpolygons(raster, integer);
DROP TYPE IF EXISTS wktgeomval;

-- function parameter names changed
DROP FUNCTION IF EXISTS st_nearestvalue(raster, integer, integer, integer, boolean);
DROP FUNCTION IF EXISTS st_nearestvalue(raster, integer, integer, boolean);
DROP FUNCTION IF EXISTS st_neighborhood(raster, integer, integer, integer, integer, boolean);
DROP FUNCTION IF EXISTS st_neighborhood(raster, integer, integer, integer, boolean);
DROP FUNCTION IF EXISTS st_neighborhood(raster, integer, geometry, integer, boolean);
DROP FUNCTION IF EXISTS st_neighborhood(raster, geometry, integer, boolean);
