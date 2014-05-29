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
-- This program is free software; you can redistribute it and/or
-- modify it under the terms of the GNU General Public License
-- as published by the Free Software Foundation; either version 2
-- of the License, or (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this program; if not, write to the Free Software Foundation,
-- Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
--
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--
-- WARNING: Any change in this file must be evaluated for compatibility.
--
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--
-- Drop obsolete functions (run as last step in an upgrade)
--
-- TODO: tag each item with the version in which it was dropped
-- 
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--

-- drop obsoleted aggregates
DROP AGGREGATE IF EXISTS ST_Union(raster, text, text, text, double precision, text, text, text, double precision, text, text, text, double precision);
DROP AGGREGATE IF EXISTS ST_Union(raster, text, text, text);
DROP AGGREGATE IF EXISTS ST_Union(raster, text, text, text, double precision, text, text, text, double precision);
DROP AGGREGATE IF EXISTS ST_Union(raster, text, text);
DROP AGGREGATE IF EXISTS ST_Union(raster, text, text, text, double precision);
DROP AGGREGATE IF EXISTS ST_Union(raster, record[]);

--these were renamed to ST_MapAlgebraExpr or argument names changed --
DROP FUNCTION IF EXISTS ST_MapAlgebra(raster, integer, text, text, nodatavaluerepl text);
DROP FUNCTION IF EXISTS ST_MapAlgebra(raster, pixeltype text, expression text, nodatavaluerepl text);


--dropped functions
DROP FUNCTION IF EXISTS  ST_MapAlgebraFct(raster, raster, regprocedure, VARIADIC text[]);

--function no longer exists
DROP FUNCTION IF EXISTS ST_BandNoDataValue(raster);

--function no longer exists
DROP FUNCTION IF EXISTS ST_SetGeoReference(raster, text);

--function no longer exists
DROP FUNCTION IF EXISTS st_setbandisnodata(raster);

--function no longer exists
DROP FUNCTION IF EXISTS st_setbandnodatavalue(raster, integer, double precision);

--function no longer exists
DROP FUNCTION IF EXISTS st_dumpaspolygons(raster);

--function no longer exists
DROP FUNCTION IF EXISTS st_polygon(raster);

-- function no longer exists
DROP FUNCTION IF EXISTS st_makeemptyraster(int, int, float8, float8, float8, float8, float8, float8);

-- function no longer exists
DROP FUNCTION IF EXISTS st_addband(raster, text);
DROP FUNCTION IF EXISTS st_addband(raster, text, float8);
DROP FUNCTION IF EXISTS st_addband(raster, int, text);
DROP FUNCTION IF EXISTS st_addband(raster, int, text, float8);
DROP FUNCTION IF EXISTS st_addband(raster, raster, int);
DROP FUNCTION IF EXISTS st_addband(raster, raster);

-- function no longer exists
DROP FUNCTION IF EXISTS st_bandisnodata(raster);
DROP FUNCTION IF EXISTS st_bandisnodata(raster, integer);

-- function no longer exists
DROP FUNCTION IF EXISTS st_bandpath(raster);

-- function no longer exists
DROP FUNCTION IF EXISTS st_bandpixeltype(raster);


-- function no longer exists
DROP FUNCTION IF EXISTS st_georeference(raster);

-- function name change
DROP FUNCTION IF EXISTS dumpaswktpolygons(raster, integer);

-- remove TYPE summarystats
DROP TYPE IF EXISTS summarystats;

-- remove TYPE quantile
DROP TYPE IF EXISTS quantile;

-- remove TYPE valuecount
DROP TYPE IF EXISTS valuecount;

-- remove TYPE histogram
DROP TYPE IF EXISTS histogram;

-- no longer needed functions changed to use out parameters
DROP TYPE IF EXISTS bandmetadata;
DROP TYPE IF EXISTS geomvalxy;

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

-- function was renamed
DROP FUNCTION IF EXISTS st_minpossibleval(text);

-- function deprecated previously
DROP FUNCTION IF EXISTS st_pixelaspolygon(raster, integer, integer, integer);

-- refactoring of functions
DROP FUNCTION IF EXISTS _st_dumpaswktpolygons(raster, integer);
DROP TYPE IF EXISTS wktgeomval;


-- variants of st_intersection with regprocedure no longer exist
DROP FUNCTION IF EXISTS st_intersection(raster, integer, raster, integer, text, regprocedure);
DROP FUNCTION IF EXISTS st_intersection(raster, integer, raster, integer, regprocedure);
DROP FUNCTION IF EXISTS st_intersection(raster, raster, text, regprocedure);
DROP FUNCTION IF EXISTS st_intersection(raster, raster, regprocedure);

-- function deprecated
DROP FUNCTION IF EXISTS st_pixelaspolygons(raster, integer);

-- function deprecated
DROP FUNCTION IF EXISTS st_bandsurface(raster, integer);

-- function no longer exists
DROP FUNCTION IF EXISTS st_overlaps(geometry, raster, integer);
DROP FUNCTION IF EXISTS st_overlaps(raster, integer, geometry);
DROP FUNCTION IF EXISTS st_overlaps(raster, geometry, integer);
DROP FUNCTION IF EXISTS _st_overlaps(raster, geometry, integer);
DROP FUNCTION IF EXISTS _st_overlaps(geometry, raster, integer);

-- function no longer exists
DROP FUNCTION IF EXISTS st_touches(geometry, raster, integer);
DROP FUNCTION IF EXISTS st_touches(raster, geometry, integer);
DROP FUNCTION IF EXISTS st_touches(raster, integer, geometry);
DROP FUNCTION IF EXISTS _st_touches(geometry, raster, integer);
DROP FUNCTION IF EXISTS _st_touches(raster, geometry, integer);

-- function no longer exists
DROP FUNCTION IF EXISTS st_contains(raster, geometry, integer);
DROP FUNCTION IF EXISTS st_contains(raster, integer, geometry);
DROP FUNCTION IF EXISTS st_contains(geometry, raster, integer);
DROP FUNCTION IF EXISTS _st_contains(raster, geometry, integer);
DROP FUNCTION IF EXISTS _st_contains(geometry, raster, integer);


-- function no longer exists
DROP FUNCTION IF EXISTS st_tile(raster, integer, integer, integer[]);
DROP FUNCTION IF EXISTS st_tile(raster, integer, integer, integer);

-- function name change
DROP FUNCTION IF EXISTS st_world2rastercoord(raster, double precision, double precision);
DROP FUNCTION IF EXISTS st_world2rastercoord(raster, geometry);
DROP FUNCTION IF EXISTS _st_world2rastercoord(raster, double precision, double precision);
DROP FUNCTION IF EXISTS st_world2rastercoordx(raster, float8, float8);
DROP FUNCTION IF EXISTS st_world2rastercoordx(raster, float8);
DROP FUNCTION IF EXISTS st_world2rastercoordx(raster, geometry);
DROP FUNCTION IF EXISTS st_world2rastercoordy(raster, float8, float8);
DROP FUNCTION IF EXISTS st_world2rastercoordy(raster, float8);
DROP FUNCTION IF EXISTS st_world2rastercoordy(raster, geometry);
DROP FUNCTION IF EXISTS st_raster2worldcoord( raster, integer, integer);
DROP FUNCTION IF EXISTS _st_raster2worldcoord(raster, integer, integer);
DROP FUNCTION IF EXISTS st_raster2worldcoordx(raster, int, int);
DROP FUNCTION IF EXISTS st_raster2worldcoordx(raster, int);
DROP FUNCTION IF EXISTS st_raster2worldcoordy(raster, int, int);
DROP FUNCTION IF EXISTS st_raster2worldcoordy(raster, int);

-- function name change
DROP FUNCTION IF EXISTS _st_resample(raster, text, double precision, integer, double precision, double precision, double precision, double precision, double precision, double precision, integer, integer);

-- function no longer exists
DROP FUNCTION IF EXISTS _add_raster_constraint_regular_blocking(name, name, name);

-- obsoleted functions
DROP FUNCTION IF EXISTS _st_mapalgebra4unionfinal1(raster);
DROP FUNCTION IF EXISTS _st_mapalgebra4unionstate(raster, raster, int4);
DROP FUNCTION IF EXISTS _st_mapalgebra4unionstate(raster, raster);
DROP FUNCTION IF EXISTS _st_mapalgebra4unionstate(raster, raster, text);
DROP FUNCTION IF EXISTS _st_mapalgebra4unionstate(raster, raster, int4, text);
DROP FUNCTION IF EXISTS _st_mapalgebra4unionstate(raster, raster, text, text, text, float8, text, text, text, float8);

