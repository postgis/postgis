-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--
--
-- PostGIS Raster - Raster Type for PostGIS
-- http://trac.osgeo.org/postgis/wiki/WKTRaster
--
-- Copyright (C) 2011 Regina Obe <lr@pcorp.us>
-- Copyright (C) 2011-2012 Regents of the University of California
-- <bkpark@ucdavis.edu>
--
-- This program is free software; you can redistribute it and/or
-- modify it under the terms of the GNU General Public License
-- as published by the Free Software Foundation; either version 2
-- of the License, or (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this program; if not, write to the Free Software Foundation,
-- Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
--
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--
-- WARNING: Any change in this file must be evaluated for compatibility.
--
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--
-- This file will be used to drop obsoleted objects.
-- It will be only used for upgrades.
-- It will be loaded _after_ the objects upgrade statements, so
-- that objects previously dependent on these objects have a chance
-- to get upgraded to remove the dependency.
--
-- Remember: only put _obsoleted_ signatures in this file.
--
-- TODO: tag each item with the version in which it was dropped
--
--

-- drop obsoleted aggregates
DROP AGGREGATE IF EXISTS ST_Union(raster, text, text, text, double precision, text, text, text, double precision, text, text, text, double precision);
DROP AGGREGATE IF EXISTS ST_Union(raster, text, text, text);
DROP AGGREGATE IF EXISTS ST_Union(raster, text, text, text, double precision, text, text, text, double precision);
DROP AGGREGATE IF EXISTS ST_Union(raster, text, text);
DROP AGGREGATE IF EXISTS ST_Union(raster, text, text, text, double precision);
DROP AGGREGATE IF EXISTS ST_Union(raster, record[]);

-- Removed in 2.0.0 ? see ticket #5489
SELECT _postgis_drop_function_by_signature('ST_Intersects(raster,boolean,geometry)');
-- Removed in 2.0.0 ? see ticket #5489
SELECT _postgis_drop_function_by_signature('ST_Intersects(geometry,raster,boolean)');
-- Removed in 2.0.0 ? see ticket #5489
SELECT _postgis_drop_function_by_signature('ST_Intersects(raster,geometry)');
-- Removed in 2.0.0 ? see ticket #5489
SELECT _postgis_drop_function_by_signature('ST_Intersects(geometry,raster)');
-- Removed in 2.0.0 ? see ticket #5489
SELECT _postgis_drop_function_by_signature('ST_Intersects(raster, integer, boolean, geometry)');
-- Removed in 2.0.0 ? see ticket #5489
SELECT _postgis_drop_function_by_signature('ST_Intersects(geometry, raster, integer, boolean)');
-- Removed in 2.1.0, see ticket #5489
SELECT _postgis_drop_function_by_signature('_st_intersects(raster, geometry, integer)');

-- Removed in 2.1.0, see ticket #5490
SELECT _postgis_drop_function_by_signature('_st_slope4ma(float8[], text, text[])');
-- Removed in 2.0.0 ? see ticket #5490
SELECT _postgis_drop_function_by_signature('st_slope(raster, integer, text, boolean)');

-- Removed in 2.1.0, see ticket #5491
SELECT _postgis_drop_function_by_signature('_st_aspect4ma(float8[], text, text[])');
-- Removed before 2.0.0 ? see ticket #5491
SELECT _postgis_drop_function_by_signature('st_aspect(raster, integer, text, boolean)');

SELECT _postgis_drop_function_by_signature('ST_Intersection(raster,raster, integer, integer)');
SELECT _postgis_drop_function_by_signature('ST_Intersection(geometry,raster)');

-- Never made it into 2.0.0, see https://trac.osgeo.org/postgis/ticket/5500
SELECT _postgis_drop_function_by_signature('ST_MapAlgebraFct(raster, raster, regprocedure, text[])');
-- Removed in 2.1.0
SELECT _postgis_drop_function_by_signature('_st_mapalgebra4unionfinal1(raster)');
-- Removed in 2.1.0
SELECT _postgis_drop_function_by_signature('_st_mapalgebra4unionstate(raster, raster, int4)');
-- Removed in 2.1.0
SELECT _postgis_drop_function_by_signature('_st_mapalgebra4unionstate(raster, raster)');
-- Removed in 2.1.0
SELECT _postgis_drop_function_by_signature('_st_mapalgebra4unionstate(raster, raster, text)');
-- Removed in 2.1.0
SELECT _postgis_drop_function_by_signature('_st_mapalgebra4unionstate(raster, raster, int4, text)');
-- Removed in 2.1.0
SELECT _postgis_drop_function_by_signature('_st_mapalgebra4unionstate(raster, raster, text, text, text, float8, text, text, text, float8)');
-- Removed in 2.1.0
SELECT _postgis_drop_function_by_signature('st_clip(raster, int, geometry, float8[], boolean)');
-- Removed in 2.2.0
SELECT _postgis_drop_function_by_signature('_st_mapalgebra(rastbandarg[],regprocedure,text,integer,integer,text,raster,text[])');

-- Removed in 3.1.0
SELECT _postgis_drop_function_by_signature('_st_count(text, text, integer, boolean, double precision)');
SELECT _postgis_drop_function_by_signature('st_count(text, text, int, boolean)');
SELECT _postgis_drop_function_by_signature('st_count(text, text, boolean)');
SELECT _postgis_drop_function_by_signature('st_approxcount(text, text, int, boolean, double precision)');
SELECT _postgis_drop_function_by_signature('st_approxcount(text, text, int, boolean, double precision)');
SELECT _postgis_drop_function_by_signature('st_approxcount(text, text, int, double precision)');
SELECT _postgis_drop_function_by_signature('st_approxcount(text, text, boolean, double precision)');
SELECT _postgis_drop_function_by_signature('st_approxcount(text, text, double precision)');

SELECT _postgis_drop_function_by_signature('_st_summarystats(text, text, integer, boolean, double precision)');
SELECT _postgis_drop_function_by_signature('st_summarystats(text, text, integer, boolean)');
SELECT _postgis_drop_function_by_signature('st_summarystats(text, text, boolean)');
SELECT _postgis_drop_function_by_signature('st_approxsummarystats(text, text, integer, boolean, double precision)');
SELECT _postgis_drop_function_by_signature('st_approxsummarystats(text, text, integer, double precision)');
SELECT _postgis_drop_function_by_signature('st_approxsummarystats(text, text, boolean)');
SELECT _postgis_drop_function_by_signature('st_approxsummarystats(text, text, double precision)');

SELECT _postgis_drop_function_by_signature('_st_histogram(text, text, int, boolean, double precision, int,double precision[], boolean)');
SELECT _postgis_drop_function_by_signature('st_histogram(text, text, int, boolean, int, double precision[], boolean)');
SELECT _postgis_drop_function_by_signature('st_histogram(text, text, int, boolean, int, boolean)');
SELECT _postgis_drop_function_by_signature('st_histogram(text, text, int, int, double precision[], boolean)');
SELECT _postgis_drop_function_by_signature('st_histogram(text, text, int, int, boolean)', '3.1.0');
SELECT _postgis_drop_function_by_signature('st_approxhistogram(text, text, int, boolean, double precision, int, double precision[], boolean)');
SELECT _postgis_drop_function_by_signature('st_approxhistogram(text, text, int, boolean, double precision, int, boolean)');
SELECT _postgis_drop_function_by_signature('st_approxhistogram(text, text, int,double precision)');
SELECT _postgis_drop_function_by_signature('st_approxhistogram(text, text, double precision)');
SELECT _postgis_drop_function_by_signature('st_approxhistogram(text, text, int, double precision, int, double precision[], boolean)');
SELECT _postgis_drop_function_by_signature('st_approxhistogram(text, text, int, double precision, int, boolean)');

SELECT _postgis_drop_function_by_identity('st_quantile', 'rastertable text, rastercolumn text, nband integer, exclude_nodata_value boolean, quantile double precision','3.0');
SELECT _postgis_drop_function_by_identity('st_quantile', 'rastertable text, rastercolumn text, nband integer, quantile double precision','3.0');
SELECT _postgis_drop_function_by_identity('st_quantile', 'rastertable text, rastercolumn text, exclude_nodata_value boolean, quantile double precision','3.0');
SELECT _postgis_drop_function_by_identity('st_quantile', 'rastertable text, rastercolumn text, quantile double precision','3.0');
SELECT _postgis_drop_function_by_identity('st_approxquantile', 'rastertable text, rastercolumn text, nband integer, exclude_nodata_value boolean, sample_percent double precision, quantile double precision','3.0');
SELECT _postgis_drop_function_by_identity('st_approxquantile', 'rastertable text, rastercolumn text, nband integer, sample_percent double precision, quantile double precision','3.0');
SELECT _postgis_drop_function_by_identity('st_approxquantile', 'rastertable text, rastercolumn text, sample_percent double precision, quantile double precision','3.0');
SELECT _postgis_drop_function_by_identity('st_approxquantile', 'rastertable text, rastercolumn text, exclude_nodata_value boolean, quantile double precision','3.0');
SELECT _postgis_drop_function_by_identity('st_approxquantile', 'rastertable text, rastercolumn text, quantile double precision','3.0');

SELECT _postgis_drop_function_by_identity('st_value','rast raster, band integer, pt geometry, exclude_nodata_value boolean');

SELECT _postgis_drop_function_by_signature('st_gdalopenoptions(text[])');

SELECT _postgis_drop_function_by_identity('st_clip','rast raster, geom geometry, nodataval float8[], crop boolean', '3.5');

