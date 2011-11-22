-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--
-- $Id$
--
-- PostGIS Raster - Raster Type for PostGIS
-- http://trac.osgeo.org/postgis/wiki/WKTRaster
--
-- Copyright (c) 2009 Sandro Santilli <strk@keybit.net>
-- Copyright (c) 2009-2010 Pierre Racine <pierre.racine@sbf.ulaval.ca>
-- Copyright (c) 2009-2010 Jorge Arevalo <jorge.arevalo@deimos-space.com>
-- Copyright (c) 2009-2010 Mateusz Loskot <mateusz@loskot.net>
-- Copyright (c) 2010 David Zwarg <dzwarg@azavea.com>
-- Copyright (C) 2011 Regents of the University of California
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

-- BEGIN;

------------------------------------------------------------------------------
-- RASTER Type
------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION raster_in(cstring)
    RETURNS raster
    AS 'MODULE_PATHNAME','RASTER_in'
    LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION raster_out(raster)
    RETURNS cstring
    AS 'MODULE_PATHNAME','RASTER_out'
    LANGUAGE 'C' IMMUTABLE STRICT;

CREATE TYPE raster (
    alignment = double,
    internallength = variable,
    input = raster_in,
    output = raster_out,
    storage = extended
);

------------------------------------------------------------------------------
-- FUNCTIONS
------------------------------------------------------------------------------

-----------------------------------------------------------------------
-- Raster Version
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION postgis_raster_lib_version()
    RETURNS text
    AS 'MODULE_PATHNAME', 'RASTER_lib_version'
    LANGUAGE 'C' IMMUTABLE; -- a new lib will require a new session

CREATE OR REPLACE FUNCTION postgis_raster_lib_build_date()
    RETURNS text
    AS 'MODULE_PATHNAME', 'RASTER_lib_build_date'
    LANGUAGE 'C' IMMUTABLE; -- a new lib will require a new session

CREATE OR REPLACE FUNCTION postgis_gdal_version()
    RETURNS text
    AS 'MODULE_PATHNAME', 'RASTER_gdal_version'
    LANGUAGE 'C' IMMUTABLE;

-----------------------------------------------------------------------
-- Raster Accessors
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION st_convexhull(raster)
    RETURNS geometry
    AS 'MODULE_PATHNAME','RASTER_convex_hull'
    LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION box2d(raster)
    RETURNS box2d
    AS 'select box2d(st_convexhull($1))'
    LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_envelope(raster)
    RETURNS geometry
    AS 'select st_envelope(st_convexhull($1))'
    LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_height(raster)
    RETURNS integer
    AS 'MODULE_PATHNAME','RASTER_getHeight'
    LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_numbands(raster)
    RETURNS integer
    AS 'MODULE_PATHNAME','RASTER_getNumBands'
    LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_scalex(raster)
    RETURNS float8
    AS 'MODULE_PATHNAME','RASTER_getXScale'
    LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_scaley(raster)
    RETURNS float8
    AS 'MODULE_PATHNAME','RASTER_getYScale'
    LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_skewx(raster)
    RETURNS float8
    AS 'MODULE_PATHNAME','RASTER_getXSkew'
    LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_skewy(raster)
    RETURNS float8
    AS 'MODULE_PATHNAME','RASTER_getYSkew'
    LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_srid(raster)
    RETURNS integer
    AS 'MODULE_PATHNAME','RASTER_getSRID'
    LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_upperleftx(raster)
    RETURNS float8
    AS 'MODULE_PATHNAME','RASTER_getXUpperLeft'
    LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_upperlefty(raster)
    RETURNS float8
    AS 'MODULE_PATHNAME','RASTER_getYUpperLeft'
    LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_width(raster)
    RETURNS integer
    AS 'MODULE_PATHNAME','RASTER_getWidth'
    LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_pixelwidth(raster)
    RETURNS float8
    AS 'MODULE_PATHNAME', 'RASTER_getPixelWidth'
    LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_pixelheight(raster)
    RETURNS float8
    AS 'MODULE_PATHNAME', 'RASTER_getPixelHeight'
    LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_rotation(raster)
    RETURNS float8
    AS 'MODULE_PATHNAME','RASTER_getRotation'
    LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_metadata(
	rast raster,
	OUT upperleftx double precision,
	OUT upperlefty double precision,
	OUT width int,
	OUT height int,
	OUT scalex double precision,
	OUT scaley double precision,
	OUT skewx double precision,
	OUT skewy double precision,
	OUT srid int,
	OUT numbands int
)
	AS 'MODULE_PATHNAME', 'RASTER_metadata'
	LANGUAGE 'C' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- Constructors ST_MakeEmptyRaster and ST_AddBand
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_makeemptyraster(width int, height int, upperleftx float8, upperlefty float8, scalex float8, scaley float8, skewx float8, skewy float8, srid int4)
    RETURNS RASTER
    AS 'MODULE_PATHNAME', 'RASTER_makeEmpty'
    LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_makeemptyraster(width int, height int, upperleftx float8, upperlefty float8, pixelsize float8)
    RETURNS raster
    AS $$ SELECT st_makeemptyraster($1, $2, $3, $4, $5, $5, 0, 0, ST_Srid('POINT(0 0)'::geometry)) $$
    LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_makeemptyraster(width int, height int, upperleftx float8, upperlefty float8, scalex float8, scaley float8, skewx float8, skewy float8)
    RETURNS raster
    AS $$ SELECT st_makeemptyraster($1, $2, $3, $4, $5, $6, $7, $8, ST_Srid('POINT(0 0)'::geometry)) $$
    LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_makeemptyraster(rast raster)
    RETURNS raster
    AS $$
		DECLARE
			w int;
			h int;
			ul_x double precision;
			ul_y double precision;
			scale_x double precision;
			scale_y double precision;
			skew_x double precision;
			skew_y double precision;
			sr_id int;
		BEGIN
			SELECT width, height, upperleftx, upperlefty, scalex, scaley, skewx, skewy, srid INTO w, h, ul_x, ul_y, scale_x, scale_y, skew_x, skew_y, sr_id FROM ST_Metadata(rast);
			RETURN st_makeemptyraster(w, h, ul_x, ul_y, scale_x, scale_y, skew_x, skew_y, sr_id);
		END;
    $$ LANGUAGE 'plpgsql' IMMUTABLE STRICT;

-- This function can not be STRICT, because nodataval can be NULL indicating that no nodata value should be set
CREATE OR REPLACE FUNCTION st_addband(rast raster, index int, pixeltype text, initialvalue float8, nodataval float8)
    RETURNS RASTER
    AS 'MODULE_PATHNAME', 'RASTER_addband'
    LANGUAGE 'C' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_addband(rast raster, pixeltype text)
    RETURNS raster
    AS 'select st_addband($1, NULL, $2, 0, NULL)'
    LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_addband(rast raster, pixeltype text, initialvalue float8)
    RETURNS raster
    AS 'select st_addband($1, NULL, $2, $3, NULL)'
    LANGUAGE 'SQL' IMMUTABLE STRICT;

-- This function can not be STRICT, because nodataval can be NULL indicating that no nodata value should be set
CREATE OR REPLACE FUNCTION st_addband(rast raster, pixeltype text, initialvalue float8, nodataval float8)
    RETURNS raster
    AS 'select st_addband($1, NULL, $2, $3, $4)'
    LANGUAGE 'SQL' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_addband(rast raster, index int, pixeltype text)
    RETURNS raster
    AS 'select st_addband($1, $2, $3, 0, NULL)'
    LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_addband(rast raster, index int, pixeltype text, initialvalue float8)
    RETURNS raster
    AS 'select st_addband($1, $2, $3, $4, NULL)'
    LANGUAGE 'SQL' IMMUTABLE STRICT;

-- This function can not be STRICT, because torastindex can not be determined (could be st_numbands(raster) though)
CREATE OR REPLACE FUNCTION st_addband(torast raster, fromrast raster, fromband int, torastindex int)
    RETURNS RASTER
    AS 'MODULE_PATHNAME', 'RASTER_copyband'
    LANGUAGE 'C' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_addband(torast raster, fromrast raster, fromband int)
    RETURNS raster
    AS 'select st_addband($1, $2, $3, NULL)'
    LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_addband(torast raster, fromrast raster)
    RETURNS raster
    AS 'select st_addband($1, $2, 1, NULL)'
    LANGUAGE 'SQL' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- Constructor ST_Band
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_band(rast raster, nbands int[] DEFAULT ARRAY[1])
	RETURNS RASTER
	AS 'MODULE_PATHNAME', 'RASTER_band'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_band(rast raster, nband int)
	RETURNS RASTER
	AS $$ SELECT st_band($1, ARRAY[$2]) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_band(rast raster, nbands text, delimiter char DEFAULT ',')
	RETURNS RASTER
	AS $$ SELECT st_band($1, regexp_split_to_array(regexp_replace($2, '[[:space:]]', '', 'g'), $3)::int[]) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- ST_SummaryStats and ST_ApproxSummaryStats
-----------------------------------------------------------------------
CREATE TYPE summarystats AS (
	count bigint,
	sum double precision,
	mean double precision,
	stddev double precision,
	min double precision,
	max double precision
);

CREATE OR REPLACE FUNCTION _st_summarystats(rast raster, nband int DEFAULT 1, exclude_nodata_value boolean DEFAULT TRUE, sample_percent double precision DEFAULT 1)
	RETURNS summarystats
	AS 'MODULE_PATHNAME','RASTER_summaryStats'
	LANGUAGE 'C' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_summarystats(rast raster, nband int DEFAULT 1, exclude_nodata_value boolean DEFAULT TRUE)
	RETURNS summarystats
	AS $$ SELECT _st_summarystats($1, $2, $3, 1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_summarystats(rast raster, exclude_nodata_value boolean)
	RETURNS summarystats
	AS $$ SELECT _st_summarystats($1, 1, $2, 1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxsummarystats(rast raster, nband int DEFAULT 1, exclude_nodata_value boolean DEFAULT TRUE, sample_percent double precision DEFAULT 0.1)
	RETURNS summarystats
	AS $$ SELECT _st_summarystats($1, $2, $3, $4) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxsummarystats(rast raster, nband int, sample_percent double precision)
	RETURNS summarystats
	AS $$ SELECT _st_summarystats($1, $2, TRUE, $3) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxsummarystats(rast raster, exclude_nodata_value boolean, sample_percent double precision DEFAULT 0.1)
	RETURNS summarystats
	AS $$ SELECT _st_summarystats($1, 1, $2, $3) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxsummarystats(rast raster, sample_percent double precision)
	RETURNS summarystats
	AS $$ SELECT _st_summarystats($1, 1, TRUE, $2) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION _st_summarystats(rastertable text, rastercolumn text, nband integer DEFAULT 1, exclude_nodata_value boolean DEFAULT TRUE, sample_percent double precision DEFAULT 1)
	RETURNS summarystats
	AS 'MODULE_PATHNAME','RASTER_summaryStatsCoverage'
	LANGUAGE 'C' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_summarystats(rastertable text, rastercolumn text, nband integer DEFAULT 1, exclude_nodata_value boolean DEFAULT TRUE)
	RETURNS summarystats
	AS $$ SELECT _st_summarystats($1, $2, $3, $4, 1) $$
	LANGUAGE 'SQL' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_summarystats(rastertable text, rastercolumn text, exclude_nodata_value boolean)
	RETURNS summarystats
	AS $$ SELECT _st_summarystats($1, $2, 1, $3, 1) $$
	LANGUAGE 'SQL' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxsummarystats(rastertable text, rastercolumn text, nband integer DEFAULT 1, exclude_nodata_value boolean DEFAULT TRUE, sample_percent double precision DEFAULT 0.1)
	RETURNS summarystats
	AS $$ SELECT _st_summarystats($1, $2, $3, $4, $5) $$
	LANGUAGE 'SQL' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxsummarystats(rastertable text, rastercolumn text, nband integer, sample_percent double precision)
	RETURNS summarystats
	AS $$ SELECT _st_summarystats($1, $2, $3, TRUE, $4) $$
	LANGUAGE 'SQL' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxsummarystats(rastertable text, rastercolumn text, exclude_nodata_value boolean)
	RETURNS summarystats
	AS $$ SELECT _st_summarystats($1, $2, 1, $3, 0.1) $$
	LANGUAGE 'SQL' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxsummarystats(rastertable text, rastercolumn text, sample_percent double precision)
	RETURNS summarystats
	AS $$ SELECT _st_summarystats($1, $2, 1, TRUE, $3) $$
	LANGUAGE 'SQL' STABLE STRICT;

-----------------------------------------------------------------------
-- ST_Count and ST_ApproxCount
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION _st_count(rast raster, nband int DEFAULT 1, exclude_nodata_value boolean DEFAULT TRUE, sample_percent double precision DEFAULT 1)
	RETURNS bigint
	AS $$
	DECLARE
		rtn bigint;
	BEGIN
		IF exclude_nodata_value IS FALSE THEN
			SELECT width * height INTO rtn FROM ST_Metadata(rast);
		ELSE
			SELECT count INTO rtn FROM _st_summarystats($1, $2, $3, $4);
		END IF;

		RETURN rtn;
	END;
	$$ LANGUAGE 'plpgsql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_count(rast raster, nband int DEFAULT 1, exclude_nodata_value boolean DEFAULT TRUE)
	RETURNS bigint
	AS $$ SELECT _st_count($1, $2, $3, 1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_count(rast raster, exclude_nodata_value boolean)
	RETURNS bigint
	AS $$ SELECT _st_count($1, 1, $2, 1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxcount(rast raster, nband int DEFAULT 1, exclude_nodata_value boolean DEFAULT TRUE, sample_percent double precision DEFAULT 0.1)
	RETURNS bigint
	AS $$ SELECT _st_count($1, $2, $3, $4) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxcount(rast raster, nband int, sample_percent double precision)
	RETURNS bigint
	AS $$ SELECT _st_count($1, $2, TRUE, $3) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxcount(rast raster, exclude_nodata_value boolean, sample_percent double precision DEFAULT 0.1)
	RETURNS bigint
	AS $$ SELECT _st_count($1, 1, $2, $3) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxcount(rast raster, sample_percent double precision)
	RETURNS bigint
	AS $$ SELECT _st_count($1, 1, TRUE, $2) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION _st_count(rastertable text, rastercolumn text, nband integer DEFAULT 1, exclude_nodata_value boolean DEFAULT TRUE, sample_percent double precision DEFAULT 1)
	RETURNS bigint
	AS $$
	DECLARE
		curs refcursor;

		ctable text;
		ccolumn text;
		rast raster;
		stats summarystats;

		rtn bigint;
		tmp bigint;
	BEGIN
		-- nband
		IF nband < 1 THEN
			RAISE WARNING 'Invalid band index (must use 1-based). Returning NULL';
			RETURN NULL;
		END IF;

		-- sample percent
		IF sample_percent < 0 OR sample_percent > 1 THEN
			RAISE WARNING 'Invalid sample percentage (must be between 0 and 1). Returning NULL';
			RETURN NULL;
		END IF;

		-- exclude_nodata_value IS TRUE
		IF exclude_nodata_value IS TRUE THEN
			SELECT count INTO rtn FROM _st_summarystats($1, $2, $3, $4, $5);
			RETURN rtn;
		END IF;

		-- clean rastertable and rastercolumn
		ctable := quote_ident(rastertable);
		ccolumn := quote_ident(rastercolumn);

		BEGIN
			OPEN curs FOR EXECUTE 'SELECT '
					|| ccolumn
					|| ' FROM '
					|| ctable
					|| ' WHERE '
					|| ccolumn
					|| ' IS NOT NULL';
		EXCEPTION
			WHEN OTHERS THEN
				RAISE WARNING 'Invalid table or column name. Returning NULL';
				RETURN NULL;
		END;

		rtn := 0;
		LOOP
			FETCH curs INTO rast;
			EXIT WHEN NOT FOUND;

			SELECT (width * height) INTO tmp FROM ST_Metadata(rast);
			rtn := rtn + tmp;
		END LOOP;

		CLOSE curs;

		RETURN rtn;
	END;
	$$ LANGUAGE 'plpgsql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_count(rastertable text, rastercolumn text, nband int DEFAULT 1, exclude_nodata_value boolean DEFAULT TRUE)
	RETURNS bigint
	AS $$ SELECT _st_count($1, $2, $3, $4, 1) $$
	LANGUAGE 'SQL' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_count(rastertable text, rastercolumn text, exclude_nodata_value boolean)
	RETURNS bigint
	AS $$ SELECT _st_count($1, $2, 1, $3, 1) $$
	LANGUAGE 'SQL' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxcount(rastertable text, rastercolumn text, nband int DEFAULT 1, exclude_nodata_value boolean DEFAULT TRUE, sample_percent double precision DEFAULT 0.1)
	RETURNS bigint
	AS $$ SELECT _st_count($1, $2, $3, $4, $5) $$
	LANGUAGE 'SQL' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxcount(rastertable text, rastercolumn text, nband int, sample_percent double precision)
	RETURNS bigint
	AS $$ SELECT _st_count($1, $2, $3, TRUE, $4) $$
	LANGUAGE 'SQL' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxcount(rastertable text, rastercolumn text, exclude_nodata_value boolean, sample_percent double precision DEFAULT 0.1)
	RETURNS bigint
	AS $$ SELECT _st_count($1, $2, 1, $3, $4) $$
	LANGUAGE 'SQL' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxcount(rastertable text, rastercolumn text, sample_percent double precision)
	RETURNS bigint
	AS $$ SELECT _st_count($1, $2, 1, TRUE, $3) $$
	LANGUAGE 'SQL' STABLE STRICT;

-----------------------------------------------------------------------
-- ST_Histogram and ST_ApproxHistogram
-----------------------------------------------------------------------
CREATE TYPE histogram AS (
	min double precision,
	max double precision,
	count bigint,
	percent double precision
);

-- Cannot be strict as "width", "min" and "max" can be NULL
CREATE OR REPLACE FUNCTION _st_histogram(rast raster, nband int DEFAULT 1, exclude_nodata_value boolean DEFAULT TRUE, sample_percent double precision DEFAULT 1, bins int DEFAULT 0, width double precision[] DEFAULT NULL, right boolean DEFAULT FALSE, min double precision DEFAULT NULL, max double precision DEFAULT NULL)
	RETURNS SETOF histogram
	AS 'MODULE_PATHNAME','RASTER_histogram'
	LANGUAGE 'C' IMMUTABLE;

-- Cannot be strict as "width" can be NULL
CREATE OR REPLACE FUNCTION st_histogram(rast raster, nband int DEFAULT 1, exclude_nodata_value boolean DEFAULT TRUE, bins int DEFAULT 0, width double precision[] DEFAULT NULL, right boolean DEFAULT FALSE)
	RETURNS SETOF histogram
	AS $$ SELECT min, max, count, percent FROM _st_histogram($1, $2, $3, 1, $4, $5, $6) $$
	LANGUAGE 'sql' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_histogram(rast raster, nband int, exclude_nodata_value boolean, bins int, right boolean)
	RETURNS SETOF histogram
	AS $$ SELECT min, max, count, percent FROM _st_histogram($1, $2, $3, 1, $4, NULL, $5) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

-- Cannot be strict as "width" can be NULL
CREATE OR REPLACE FUNCTION st_histogram(rast raster, nband int, bins int, width double precision[] DEFAULT NULL, right boolean DEFAULT FALSE)
	RETURNS SETOF histogram
	AS $$ SELECT min, max, count, percent FROM _st_histogram($1, $2, TRUE, 1, $3, $4, $5) $$
	LANGUAGE 'sql' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_histogram(rast raster, nband int, bins int, right boolean)
	RETURNS SETOF histogram
	AS $$ SELECT min, max, count, percent FROM _st_histogram($1, $2, TRUE, 1, $3, NULL, $4) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

-- Cannot be strict as "width" can be NULL
CREATE OR REPLACE FUNCTION st_approxhistogram(rast raster, nband int DEFAULT 1, exclude_nodata_value boolean DEFAULT TRUE, sample_percent double precision DEFAULT 0.1, bins int DEFAULT 0, width double precision[] DEFAULT NULL, right boolean DEFAULT FALSE)
	RETURNS SETOF histogram
	AS $$ SELECT min, max, count, percent FROM _st_histogram($1, $2, $3, $4, $5, $6, $7) $$
	LANGUAGE 'sql' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_approxhistogram(rast raster, nband int, exclude_nodata_value boolean, sample_percent double precision, bins int, right boolean)
	RETURNS SETOF histogram
	AS $$ SELECT min, max, count, percent FROM _st_histogram($1, $2, $3, $4, $5, NULL, $6) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxhistogram(rast raster, nband int, sample_percent double precision)
	RETURNS SETOF histogram
	AS $$ SELECT min, max, count, percent FROM _st_histogram($1, $2, TRUE, $3, 0, NULL, FALSE) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxhistogram(rast raster, sample_percent double precision)
	RETURNS SETOF histogram
	AS $$ SELECT min, max, count, percent FROM _st_histogram($1, 1, TRUE, $2, 0, NULL, FALSE) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

-- Cannot be strict as "width" can be NULL
CREATE OR REPLACE FUNCTION st_approxhistogram(rast raster, nband int, sample_percent double precision, bins int, width double precision[] DEFAULT NULL, right boolean DEFAULT FALSE)
	RETURNS SETOF histogram
	AS $$ SELECT min, max, count, percent FROM _st_histogram($1, $2, TRUE, $3, $4, $5, $6) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxhistogram(rast raster, nband int, sample_percent double precision, bins int, right boolean)
	RETURNS SETOF histogram
	AS $$ SELECT min, max, count, percent FROM _st_histogram($1, $2, TRUE, $3, $4, NULL, $5) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

-- Cannot be strict as "width" can be NULL
CREATE OR REPLACE FUNCTION _st_histogram(rastertable text, rastercolumn text, nband int DEFAULT 1, exclude_nodata_value boolean DEFAULT TRUE, sample_percent double precision DEFAULT 1, bins int DEFAULT 0, width double precision[] DEFAULT NULL, right boolean DEFAULT FALSE)
	RETURNS SETOF histogram
	AS 'MODULE_PATHNAME','RASTER_histogramCoverage'
	LANGUAGE 'C' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_histogram(rastertable text, rastercolumn text, nband int DEFAULT 1, exclude_nodata_value boolean DEFAULT TRUE, bins int DEFAULT 0, width double precision[] DEFAULT NULL, right boolean DEFAULT FALSE)
	RETURNS SETOF histogram
	AS $$ SELECT _st_histogram($1, $2, $3, $4, 1, $5, $6, $7) $$
	LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION st_histogram(rastertable text, rastercolumn text, nband int, exclude_nodata_value boolean, bins int, right boolean)
	RETURNS SETOF histogram
	AS $$ SELECT _st_histogram($1, $2, $3, $4, 1, $5, NULL, $6) $$
	LANGUAGE 'sql' STABLE STRICT;

-- Cannot be strict as "width" can be NULL
CREATE OR REPLACE FUNCTION st_histogram(rastertable text, rastercolumn text, nband int, bins int, width double precision[] DEFAULT NULL, right boolean DEFAULT FALSE)
	RETURNS SETOF histogram
	AS $$ SELECT _st_histogram($1, $2, $3, TRUE, 1, $4, $5, $6) $$
	LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION st_histogram(rastertable text, rastercolumn text, nband int, bins int, right boolean)
	RETURNS SETOF histogram
	AS $$ SELECT _st_histogram($1, $2, $3, TRUE, 1, $4, NULL, $5) $$
	LANGUAGE 'sql' STABLE STRICT;

-- Cannot be strict as "width" can be NULL
CREATE OR REPLACE FUNCTION st_approxhistogram(rastertable text, rastercolumn text, nband int DEFAULT 1, exclude_nodata_value boolean DEFAULT TRUE, sample_percent double precision DEFAULT 0.1, bins int DEFAULT 0, width double precision[] DEFAULT NULL, right boolean DEFAULT FALSE)
	RETURNS SETOF histogram
	AS $$ SELECT _st_histogram($1, $2, $3, $4, $5, $6, $7, $8) $$
	LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION st_approxhistogram(rastertable text, rastercolumn text, nband int, exclude_nodata_value boolean, sample_percent double precision, bins int, right boolean)
	RETURNS SETOF histogram
	AS $$ SELECT _st_histogram($1, $2, $3, $4, $5, $6, NULL, $7) $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxhistogram(rastertable text, rastercolumn text, nband int, sample_percent double precision)
	RETURNS SETOF histogram
	AS $$ SELECT _st_histogram($1, $2, $3, TRUE, $4, 0, NULL, FALSE) $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxhistogram(rastertable text, rastercolumn text, sample_percent double precision)
	RETURNS SETOF histogram
	AS $$ SELECT _st_histogram($1, $2, 1, TRUE, $3, 0, NULL, FALSE) $$
	LANGUAGE 'sql' STABLE STRICT;

-- Cannot be strict as "width" can be NULL
CREATE OR REPLACE FUNCTION st_approxhistogram(rastertable text, rastercolumn text, nband int, sample_percent double precision, bins int, width double precision[] DEFAULT NULL, right boolean DEFAULT FALSE)
	RETURNS SETOF histogram
	AS $$ SELECT _st_histogram($1, $2, $3, TRUE, $4, $5, $6, $7) $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxhistogram(rastertable text, rastercolumn text, nband int, sample_percent double precision, bins int, right boolean)
	RETURNS SETOF histogram
	AS $$ SELECT _st_histogram($1, $2, $3, TRUE, $4, $5, NULL, $6) $$
	LANGUAGE 'sql' STABLE STRICT;

-----------------------------------------------------------------------
-- ST_Quantile and ST_ApproxQuantile
-----------------------------------------------------------------------
CREATE TYPE quantile AS (
	quantile double precision,
	value double precision
);

-- Cannot be strict as "quantiles" can be NULL
CREATE OR REPLACE FUNCTION _st_quantile(rast raster, nband int DEFAULT 1, exclude_nodata_value boolean DEFAULT TRUE, sample_percent double precision DEFAULT 1, quantiles double precision[] DEFAULT NULL)
	RETURNS SETOF quantile
	AS 'MODULE_PATHNAME','RASTER_quantile'
	LANGUAGE 'C' IMMUTABLE;

-- Cannot be strict as "quantiles" can be NULL
CREATE OR REPLACE FUNCTION st_quantile(rast raster, nband int DEFAULT 1, exclude_nodata_value boolean DEFAULT TRUE, quantiles double precision[] DEFAULT NULL)
	RETURNS SETOF quantile
	AS $$ SELECT _st_quantile($1, $2, $3, 1, $4) $$
	LANGUAGE 'sql' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_quantile(rast raster, nband int, quantiles double precision[])
	RETURNS SETOF quantile
	AS $$ SELECT _st_quantile($1, $2, TRUE, 1, $3) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_quantile(rast raster, quantiles double precision[])
	RETURNS SETOF quantile
	AS $$ SELECT _st_quantile($1, 1, TRUE, 1, $2) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_quantile(rast raster, nband int, exclude_nodata_value boolean, quantile double precision)
	RETURNS double precision
	AS $$ SELECT (_st_quantile($1, $2, $3, 1, ARRAY[$4]::double precision[])).value $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_quantile(rast raster, nband int, quantile double precision)
	RETURNS double precision
	AS $$ SELECT (_st_quantile($1, $2, TRUE, 1, ARRAY[$3]::double precision[])).value $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

-- Cannot be strict as "quantile" can be NULL
CREATE OR REPLACE FUNCTION st_quantile(rast raster, exclude_nodata_value boolean, quantile double precision DEFAULT NULL)
	RETURNS double precision
	AS $$ SELECT (_st_quantile($1, 1, $2, 1, ARRAY[$3]::double precision[])).value $$
	LANGUAGE 'sql' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_quantile(rast raster, quantile double precision)
	RETURNS double precision
	AS $$ SELECT (_st_quantile($1, 1, TRUE, 1, ARRAY[$2]::double precision[])).value $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

-- Cannot be strict as "quantiles" can be NULL
CREATE OR REPLACE FUNCTION st_approxquantile(rast raster, nband int DEFAULT 1, exclude_nodata_value boolean DEFAULT TRUE, sample_percent double precision DEFAULT 0.1, quantiles double precision[] DEFAULT NULL)
	RETURNS SETOF quantile
	AS $$ SELECT _st_quantile($1, $2, $3, $4, $5) $$
	LANGUAGE 'sql' IMMUTABLE;

-- Cannot be strict as "quantiles" can be NULL
CREATE OR REPLACE FUNCTION st_approxquantile(rast raster, nband int, sample_percent double precision, quantiles double precision[] DEFAULT NULL)
	RETURNS SETOF quantile
	AS $$ SELECT _st_quantile($1, $2, TRUE, $3, $4) $$
	LANGUAGE 'sql' IMMUTABLE;

-- Cannot be strict as "quantiles" can be NULL
CREATE OR REPLACE FUNCTION st_approxquantile(rast raster, sample_percent double precision, quantiles double precision[] DEFAULT NULL)
	RETURNS SETOF quantile
	AS $$ SELECT _st_quantile($1, 1, TRUE, $2, $3) $$
	LANGUAGE 'sql' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_approxquantile(rast raster, quantiles double precision[])
	RETURNS SETOF quantile
	AS $$ SELECT _st_quantile($1, 1, TRUE, 0.1, $2) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxquantile(rast raster, nband int, exclude_nodata_value boolean, sample_percent double precision, quantile double precision)
	RETURNS double precision
	AS $$ SELECT (_st_quantile($1, $2, $3, $4, ARRAY[$5]::double precision[])).value $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxquantile(rast raster, nband int, sample_percent double precision, quantile double precision)
	RETURNS double precision
	AS $$ SELECT (_st_quantile($1, $2, TRUE, $3, ARRAY[$4]::double precision[])).value $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxquantile(rast raster, sample_percent double precision, quantile double precision)
	RETURNS double precision
	AS $$ SELECT (_st_quantile($1, 1, TRUE, $2, ARRAY[$3]::double precision[])).value $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

-- Cannot be strict as "quantile" can be NULL
CREATE OR REPLACE FUNCTION st_approxquantile(rast raster, exclude_nodata_value boolean, quantile double precision DEFAULT NULL)
	RETURNS double precision
	AS $$ SELECT (_st_quantile($1, 1, $2, 0.1, ARRAY[$3]::double precision[])).value $$
	LANGUAGE 'sql' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_approxquantile(rast raster, quantile double precision)
	RETURNS double precision
	AS $$ SELECT (_st_quantile($1, 1, TRUE, 0.1, ARRAY[$2]::double precision[])).value $$
	LANGUAGE 'sql' IMMUTABLE;

-- Cannot be strict as "quantiles" can be NULL
CREATE OR REPLACE FUNCTION _st_quantile(rastertable text, rastercolumn text, nband int DEFAULT 1, exclude_nodata_value boolean DEFAULT TRUE, sample_percent double precision DEFAULT 1, quantiles double precision[] DEFAULT NULL)
	RETURNS SETOF quantile
	AS 'MODULE_PATHNAME','RASTER_quantileCoverage'
	LANGUAGE 'C' STABLE;

-- Cannot be strict as "quantiles" can be NULL
CREATE OR REPLACE FUNCTION st_quantile(rastertable text, rastercolumn text, nband int DEFAULT 1, exclude_nodata_value boolean DEFAULT TRUE, quantiles double precision[] DEFAULT NULL)
	RETURNS SETOF quantile
	AS $$ SELECT _st_quantile($1, $2, $3, $4, 1, $5) $$
	LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION st_quantile(rastertable text, rastercolumn text, nband int, quantiles double precision[])
	RETURNS SETOF quantile
	AS $$ SELECT _st_quantile($1, $2, $3, TRUE, 1, $4) $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_quantile(rastertable text, rastercolumn text, quantiles double precision[])
	RETURNS SETOF quantile
	AS $$ SELECT _st_quantile($1, $2, 1, TRUE, 1, $3) $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_quantile(rastertable text, rastercolumn text, nband int, exclude_nodata_value boolean, quantile double precision)
	RETURNS double precision
	AS $$ SELECT (_st_quantile($1, $2, $3, $4, 1, ARRAY[$5]::double precision[])).value $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_quantile(rastertable text, rastercolumn text, nband int, quantile double precision)
	RETURNS double precision
	AS $$ SELECT (_st_quantile($1, $2, $3, TRUE, 1, ARRAY[$4]::double precision[])).value $$
	LANGUAGE 'sql' STABLE STRICT;

-- Cannot be strict as "quantile" can be NULL
CREATE OR REPLACE FUNCTION st_quantile(rastertable text, rastercolumn text, exclude_nodata_value boolean, quantile double precision DEFAULT NULL)
	RETURNS double precision
	AS $$ SELECT (_st_quantile($1, $2, 1, $3, 1, ARRAY[$4]::double precision[])).value $$
	LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION st_quantile(rastertable text, rastercolumn text, quantile double precision)
	RETURNS double precision
	AS $$ SELECT (_st_quantile($1, $2, 1, TRUE, 1, ARRAY[$3]::double precision[])).value $$
	LANGUAGE 'sql' STABLE STRICT;

-- Cannot be strict as "quantiles" can be NULL
CREATE OR REPLACE FUNCTION st_approxquantile(rastertable text, rastercolumn text, nband int DEFAULT 1, exclude_nodata_value boolean DEFAULT TRUE, sample_percent double precision DEFAULT 0.1, quantiles double precision[] DEFAULT NULL)
	RETURNS SETOF quantile
	AS $$ SELECT _st_quantile($1, $2, $3, $4, $5, $6) $$
	LANGUAGE 'sql' STABLE;

-- Cannot be strict as "quantiles" can be NULL
CREATE OR REPLACE FUNCTION st_approxquantile(rastertable text, rastercolumn text, nband int, sample_percent double precision, quantiles double precision[] DEFAULT NULL)
	RETURNS SETOF quantile
	AS $$ SELECT _st_quantile($1, $2, $3, TRUE, $4, $5) $$
	LANGUAGE 'sql' STABLE;

-- Cannot be strict as "quantiles" can be NULL
CREATE OR REPLACE FUNCTION st_approxquantile(rastertable text, rastercolumn text, sample_percent double precision, quantiles double precision[] DEFAULT NULL)
	RETURNS SETOF quantile
	AS $$ SELECT _st_quantile($1, $2, 1, TRUE, $3, $4) $$
	LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION st_approxquantile(rastertable text, rastercolumn text, quantiles double precision[])
	RETURNS SETOF quantile
	AS $$ SELECT _st_quantile($1, $2, 1, TRUE, 0.1, $3) $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxquantile(rastertable text, rastercolumn text, nband int, exclude_nodata_value boolean, sample_percent double precision, quantile double precision)
	RETURNS double precision
	AS $$ SELECT (_st_quantile($1, $2, $3, $4, $5, ARRAY[$6]::double precision[])).value $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxquantile(rastertable text, rastercolumn text, nband int, sample_percent double precision, quantile double precision)
	RETURNS double precision
	AS $$ SELECT (_st_quantile($1, $2, $3, TRUE, $4, ARRAY[$5]::double precision[])).value $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxquantile(rastertable text, rastercolumn text, sample_percent double precision, quantile double precision)
	RETURNS double precision
	AS $$ SELECT (_st_quantile($1, $2, 1, TRUE, $3, ARRAY[$4]::double precision[])).value $$
	LANGUAGE 'sql' STABLE STRICT;

-- Cannot be strict as "quantile" can be NULL
CREATE OR REPLACE FUNCTION st_approxquantile(rastertable text, rastercolumn text, exclude_nodata_value boolean, quantile double precision DEFAULT NULL)
	RETURNS double precision
	AS $$ SELECT (_st_quantile($1, $2, 1, $3, 0.1, ARRAY[$4]::double precision[])).value $$
	LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION st_approxquantile(rastertable text, rastercolumn text, quantile double precision)
	RETURNS double precision
	AS $$ SELECT (_st_quantile($1, $2, 1, TRUE, 0.1, ARRAY[$3]::double precision[])).value $$
	LANGUAGE 'sql' STABLE;

-----------------------------------------------------------------------
-- ST_ValueCount and ST_ValuePercent
-----------------------------------------------------------------------
CREATE TYPE valuecount AS (
	value double precision,
	count integer,
	percent double precision
);

-- None of the "valuecount" functions with "searchvalues" can be strict as "searchvalues" and "roundto" can be NULL
-- Allowing "searchvalues" to be NULL instructs the function to count all values

CREATE OR REPLACE FUNCTION _st_valuecount(rast raster, nband integer DEFAULT 1, exclude_nodata_value boolean DEFAULT TRUE, searchvalues double precision[] DEFAULT NULL, roundto double precision DEFAULT 0)
	RETURNS SETOF valuecount
	AS 'MODULE_PATHNAME', 'RASTER_valueCount'
	LANGUAGE 'C' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_valuecount(rast raster, nband integer DEFAULT 1, exclude_nodata_value boolean DEFAULT TRUE, searchvalues double precision[] DEFAULT NULL, roundto double precision DEFAULT 0, OUT value double precision, OUT count integer)
	RETURNS SETOF record
	AS $$ SELECT value, count FROM _st_valuecount($1, $2, $3, $4, $5) $$
	LANGUAGE 'sql' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_valuecount(rast raster, nband integer, searchvalues double precision[], roundto double precision DEFAULT 0, OUT value double precision, OUT count integer)
	RETURNS SETOF record
	AS $$ SELECT value, count FROM _st_valuecount($1, $2, TRUE, $3, $4) $$
	LANGUAGE 'sql' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_valuecount(rast raster, searchvalues double precision[], roundto double precision DEFAULT 0, OUT value double precision, OUT count integer)
	RETURNS SETOF record
	AS $$ SELECT value, count FROM _st_valuecount($1, 1, TRUE, $2, $3) $$
	LANGUAGE 'sql' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_valuecount(rast raster, nband integer, exclude_nodata_value boolean, searchvalue double precision, roundto double precision DEFAULT 0)
	RETURNS integer
	AS $$ SELECT (_st_valuecount($1, $2, $3, ARRAY[$4]::double precision[], $5)).count $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_valuecount(rast raster, nband integer, searchvalue double precision, roundto double precision DEFAULT 0)
	RETURNS integer
	AS $$ SELECT (_st_valuecount($1, $2, TRUE, ARRAY[$3]::double precision[], $4)).count $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_valuecount(rast raster, searchvalue double precision, roundto double precision DEFAULT 0)
	RETURNS integer
	AS $$ SELECT (_st_valuecount($1, 1, TRUE, ARRAY[$2]::double precision[], $3)).count $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_valuepercent(rast raster, nband integer DEFAULT 1, exclude_nodata_value boolean DEFAULT TRUE, searchvalues double precision[] DEFAULT NULL, roundto double precision DEFAULT 0, OUT value double precision, OUT percent double precision)
	RETURNS SETOF record
	AS $$ SELECT value, percent FROM _st_valuecount($1, $2, $3, $4, $5) $$
	LANGUAGE 'sql' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_valuepercent(rast raster, nband integer, searchvalues double precision[], roundto double precision DEFAULT 0, OUT value double precision, OUT percent double precision)
	RETURNS SETOF record
	AS $$ SELECT value, percent FROM _st_valuecount($1, $2, TRUE, $3, $4) $$
	LANGUAGE 'sql' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_valuepercent(rast raster, searchvalues double precision[], roundto double precision DEFAULT 0, OUT value double precision, OUT percent double precision)
	RETURNS SETOF record
	AS $$ SELECT value, percent FROM _st_valuecount($1, 1, TRUE, $2, $3) $$
	LANGUAGE 'sql' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_valuepercent(rast raster, nband integer, exclude_nodata_value boolean, searchvalue double precision, roundto double precision DEFAULT 0)
	RETURNS double precision
	AS $$ SELECT (_st_valuecount($1, $2, $3, ARRAY[$4]::double precision[], $5)).percent $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_valuepercent(rast raster, nband integer, searchvalue double precision, roundto double precision DEFAULT 0)
	RETURNS double precision
	AS $$ SELECT (_st_valuecount($1, $2, TRUE, ARRAY[$3]::double precision[], $4)).percent $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_valuepercent(rast raster, searchvalue double precision, roundto double precision DEFAULT 0)
	RETURNS double precision
	AS $$ SELECT (_st_valuecount($1, 1, TRUE, ARRAY[$2]::double precision[], $3)).percent $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION _st_valuecount(rastertable text, rastercolumn text, nband integer DEFAULT 1, exclude_nodata_value boolean DEFAULT TRUE, searchvalues double precision[] DEFAULT NULL, roundto double precision DEFAULT 0)
	RETURNS SETOF valuecount
	AS 'MODULE_PATHNAME', 'RASTER_valueCountCoverage'
	LANGUAGE 'C' STABLE;

CREATE OR REPLACE FUNCTION st_valuecount(rastertable text, rastercolumn text, nband integer DEFAULT 1, exclude_nodata_value boolean DEFAULT TRUE, searchvalues double precision[] DEFAULT NULL, roundto double precision DEFAULT 0, OUT value double precision, OUT count integer)
	RETURNS SETOF record
	AS $$ SELECT value, count FROM _st_valuecount($1, $2, $3, $4, $5, $6) $$
	LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION st_valuecount(rastertable text, rastercolumn text, nband integer, searchvalues double precision[], roundto double precision DEFAULT 0, OUT value double precision, OUT count integer)
	RETURNS SETOF record
	AS $$ SELECT value, count FROM _st_valuecount($1, $2, $3, TRUE, $4, $5) $$
	LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION st_valuecount(rastertable text, rastercolumn text, searchvalues double precision[], roundto double precision DEFAULT 0, OUT value double precision, OUT count integer)
	RETURNS SETOF record
	AS $$ SELECT value, count FROM _st_valuecount($1, $2, 1, TRUE, $3, $4) $$
	LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION st_valuecount(rastertable text, rastercolumn text, nband integer, exclude_nodata_value boolean, searchvalue double precision, roundto double precision DEFAULT 0)
	RETURNS integer
	AS $$ SELECT (_st_valuecount($1, $2, $3, $4, ARRAY[$5]::double precision[], $6)).count $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_valuecount(rastertable text, rastercolumn text, nband integer, searchvalue double precision, roundto double precision DEFAULT 0)
	RETURNS integer
	AS $$ SELECT (_st_valuecount($1, $2, $3, TRUE, ARRAY[$4]::double precision[], $5)).count $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_valuecount(rastertable text, rastercolumn text, searchvalue double precision, roundto double precision DEFAULT 0)
	RETURNS integer
	AS $$ SELECT (_st_valuecount($1, $2, 1, TRUE, ARRAY[$3]::double precision[], $4)).count $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_valuepercent(rastertable text, rastercolumn text, nband integer DEFAULT 1, exclude_nodata_value boolean DEFAULT TRUE, searchvalues double precision[] DEFAULT NULL, roundto double precision DEFAULT 0, OUT value double precision, OUT percent double precision)
	RETURNS SETOF record
	AS $$ SELECT value, percent FROM _st_valuecount($1, $2, $3, $4, $5, $6) $$
	LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION st_valuepercent(rastertable text, rastercolumn text, nband integer, searchvalues double precision[], roundto double precision DEFAULT 0, OUT value double precision, OUT percent double precision)
	RETURNS SETOF record
	AS $$ SELECT value, percent FROM _st_valuecount($1, $2, $3, TRUE, $4, $5) $$
	LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION st_valuepercent(rastertable text, rastercolumn text, searchvalues double precision[], roundto double precision DEFAULT 0, OUT value double precision, OUT percent double precision)
	RETURNS SETOF record
	AS $$ SELECT value, percent FROM _st_valuecount($1, $2, 1, TRUE, $3, $4) $$
	LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION st_valuepercent(rastertable text, rastercolumn text, nband integer, exclude_nodata_value boolean, searchvalue double precision, roundto double precision DEFAULT 0)
	RETURNS double precision
	AS $$ SELECT (_st_valuecount($1, $2, $3, $4, ARRAY[$5]::double precision[], $6)).percent $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_valuepercent(rastertable text, rastercolumn text, nband integer, searchvalue double precision, roundto double precision DEFAULT 0)
	RETURNS double precision
	AS $$ SELECT (_st_valuecount($1, $2, $3, TRUE, ARRAY[$4]::double precision[], $5)).percent $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_valuepercent(rastertable text, rastercolumn text, searchvalue double precision, roundto double precision DEFAULT 0)
	RETURNS double precision
	AS $$ SELECT (_st_valuecount($1, $2, 1, TRUE, ARRAY[$3]::double precision[], $4)).percent $$
	LANGUAGE 'sql' STABLE STRICT;

-----------------------------------------------------------------------
-- ST_Reclass
-----------------------------------------------------------------------
CREATE TYPE reclassarg AS (
	nband int,
	reclassexpr text,
	pixeltype text,
	nodataval double precision
);

CREATE OR REPLACE FUNCTION _st_reclass(rast raster, VARIADIC reclassargset reclassarg[])
	RETURNS raster
	AS 'MODULE_PATHNAME', 'RASTER_reclass'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_reclass(rast raster, VARIADIC reclassargset reclassarg[])
	RETURNS raster
	AS $$
	DECLARE
		i int;
		expr text;
	BEGIN
		-- for each reclassarg, validate elements as all except nodataval cannot be NULL
		FOR i IN SELECT * FROM generate_subscripts($2, 1) LOOP
			IF $2[i].nband IS NULL OR $2[i].reclassexpr IS NULL OR $2[i].pixeltype IS NULL THEN
				RAISE WARNING 'Values are required for the nband, reclassexpr and pixeltype attributes.';
				RETURN rast;
			END IF;
		END LOOP;

		RETURN _st_reclass($1, VARIADIC $2);
	END;
	$$ LANGUAGE 'plpgsql' IMMUTABLE STRICT;

-- Cannot be strict as "nodataval" can be NULL
CREATE OR REPLACE FUNCTION st_reclass(rast raster, nband int, reclassexpr text, pixeltype text, nodataval double precision DEFAULT NULL)
	RETURNS raster
	AS $$ SELECT st_reclass($1, ROW($2, $3, $4, $5)) $$
	LANGUAGE 'SQL' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_reclass(rast raster, reclassexpr text, pixeltype text)
	RETURNS raster
	AS $$ SELECT st_reclass($1, ROW(1, $2, $3, NULL)) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- ST_AsGDALRaster and supporting functions
-----------------------------------------------------------------------
-- returns set of available and usable GDAL drivers
CREATE OR REPLACE FUNCTION st_gdaldrivers(OUT idx int, OUT short_name text, OUT long_name text, OUT create_options text)
  RETURNS SETOF record
	AS 'MODULE_PATHNAME', 'RASTER_getGDALDrivers'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Cannot be strict as "options" and "srid" can be NULL
CREATE OR REPLACE FUNCTION st_asgdalraster(rast raster, format text, options text[] DEFAULT NULL, srid integer DEFAULT NULL)
	RETURNS bytea
	AS 'MODULE_PATHNAME', 'RASTER_asGDALRaster'
	LANGUAGE 'C' IMMUTABLE;

-----------------------------------------------------------------------
-- ST_AsTIFF
-----------------------------------------------------------------------
-- Cannot be strict as "options" and "srid" can be NULL
CREATE OR REPLACE FUNCTION st_astiff(rast raster, options text[] DEFAULT NULL, srid integer DEFAULT NULL)
	RETURNS bytea
	AS $$
	DECLARE
		i int;
		num_bands int;
		nodata double precision;
		last_nodata double precision;
	BEGIN
		num_bands := st_numbands($1);

		-- TIFF only allows one NODATA value for ALL bands
		FOR i IN 1..num_bands LOOP
			nodata := st_bandnodatavalue($1, i);
			IF last_nodata IS NULL THEN
				last_nodata := nodata;
			ELSEIF nodata != last_nodata THEN
				RAISE NOTICE 'The TIFF format only permits one NODATA value for all bands.  The value used will be the last band with a NODATA value.';
			END IF;
		END LOOP;

		RETURN st_asgdalraster($1, 'GTiff', $2, $3);
	END;
	$$ LANGUAGE 'plpgsql' IMMUTABLE;

-- Cannot be strict as "options" and "srid" can be NULL
CREATE OR REPLACE FUNCTION st_astiff(rast raster, nbands int[], options text[] DEFAULT NULL, srid integer DEFAULT NULL)
	RETURNS bytea
	AS $$ SELECT st_astiff(st_band($1, $2), $3, $4) $$
	LANGUAGE 'SQL' IMMUTABLE;

-- Cannot be strict as "srid" can be NULL
CREATE OR REPLACE FUNCTION st_astiff(rast raster, compression text, srid integer DEFAULT NULL)
	RETURNS bytea
	AS $$
	DECLARE
		compression2 text;
		c_type text;
		c_level int;
		i int;
		num_bands int;
		options text[];
	BEGIN
		compression2 := trim(both from upper(compression));

		IF length(compression2) > 0 THEN
			-- JPEG
			IF position('JPEG' in compression2) != 0 THEN
				c_type := 'JPEG';
				c_level := substring(compression2 from '[0-9]+$');

				IF c_level IS NOT NULL THEN
					IF c_level > 100 THEN
						c_level := 100;
					ELSEIF c_level < 1 THEN
						c_level := 1;
					END IF;

					options := array_append(options, 'JPEG_QUALITY=' || c_level);
				END IF;

				-- per band pixel type check
				num_bands := st_numbands($1);
				FOR i IN 1..num_bands LOOP
					IF st_bandpixeltype($1, i) != '8BUI' THEN
						RAISE EXCEPTION 'The pixel type of band % in the raster is not 8BUI.  JPEG compression can only be used with the 8BUI pixel type.', i;
					END IF;
				END LOOP;

			-- DEFLATE
			ELSEIF position('DEFLATE' in compression2) != 0 THEN
				c_type := 'DEFLATE';
				c_level := substring(compression2 from '[0-9]+$');

				IF c_level IS NOT NULL THEN
					IF c_level > 9 THEN
						c_level := 9;
					ELSEIF c_level < 1 THEN
						c_level := 1;
					END IF;

					options := array_append(options, 'ZLEVEL=' || c_level);
				END IF;

			ELSE
				c_type := compression2;

				-- CCITT
				IF position('CCITT' in compression2) THEN
					-- per band pixel type check
					num_bands := st_numbands($1);
					FOR i IN 1..num_bands LOOP
						IF st_bandpixeltype($1, i) != '1BB' THEN
							RAISE EXCEPTION 'The pixel type of band % in the raster is not 1BB.  CCITT compression can only be used with the 1BB pixel type.', i;
						END IF;
					END LOOP;
				END IF;

			END IF;

			-- compression type check
			IF ARRAY[c_type] <@ ARRAY['JPEG', 'LZW', 'PACKBITS', 'DEFLATE', 'CCITTRLE', 'CCITTFAX3', 'CCITTFAX4', 'NONE'] THEN
				options := array_append(options, 'COMPRESS=' || c_type);
			ELSE
				RAISE NOTICE 'Unknown compression type: %.  The outputted TIFF will not be COMPRESSED.', c_type;
			END IF;
		END IF;

		RETURN st_astiff($1, options, $3);
	END;
	$$ LANGUAGE 'plpgsql' IMMUTABLE;

-- Cannot be strict as "srid" can be NULL
CREATE OR REPLACE FUNCTION st_astiff(rast raster, nbands int[], compression text, srid integer DEFAULT NULL)
	RETURNS bytea
	AS $$ SELECT st_astiff(st_band($1, $2), $3, $4) $$
	LANGUAGE 'SQL' IMMUTABLE;

-----------------------------------------------------------------------
-- ST_AsJPEG
-----------------------------------------------------------------------
-- Cannot be strict as "options" can be NULL
CREATE OR REPLACE FUNCTION st_asjpeg(rast raster, options text[] DEFAULT NULL)
	RETURNS bytea
	AS $$
	DECLARE
		rast2 raster;
		num_bands int;
		i int;
	BEGIN
		num_bands := st_numbands($1);

		-- JPEG allows 1 or 3 bands
		IF num_bands <> 1 AND num_bands <> 3 THEN
			RAISE NOTICE 'The JPEG format only permits one or three bands.  The first band will be used.';
			rast2 := st_band(rast, ARRAY[1]);
			num_bands := st_numbands(rast);
		ELSE
			rast2 := rast;
		END IF;

		-- JPEG only supports 8BUI pixeltype
		FOR i IN 1..num_bands LOOP
			IF st_bandpixeltype(rast, i) != '8BUI' THEN
				RAISE EXCEPTION 'The pixel type of band % in the raster is not 8BUI.  The JPEG format can only be used with the 8BUI pixel type.', i;
			END IF;
		END LOOP;

		RETURN st_asgdalraster(rast2, 'JPEG', $2, NULL);
	END;
	$$ LANGUAGE 'plpgsql' IMMUTABLE;

-- Cannot be strict as "options" can be NULL
CREATE OR REPLACE FUNCTION st_asjpeg(rast raster, nbands int[], options text[] DEFAULT NULL)
	RETURNS bytea
	AS $$ SELECT st_asjpeg(st_band($1, $2), $3) $$
	LANGUAGE 'SQL' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_asjpeg(rast raster, nbands int[], quality int)
	RETURNS bytea
	AS $$
	DECLARE
		quality2 int;
		options text[];
	BEGIN
		IF quality IS NOT NULL THEN
			IF quality > 100 THEN
				quality2 := 100;
			ELSEIF quality < 10 THEN
				quality2 := 10;
			ELSE
				quality2 := quality;
			END IF;

			options := array_append(options, 'QUALITY=' || quality2);
		END IF;

		RETURN st_asjpeg(st_band($1, $2), options);
	END;
	$$ LANGUAGE 'plpgsql' IMMUTABLE STRICT;

-- Cannot be strict as "options" can be NULL
CREATE OR REPLACE FUNCTION st_asjpeg(rast raster, nband int, options text[] DEFAULT NULL)
	RETURNS bytea
	AS $$ SELECT st_asjpeg(st_band($1, $2), $3) $$
	LANGUAGE 'SQL' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_asjpeg(rast raster, nband int, quality int)
	RETURNS bytea
	AS $$ SELECT st_asjpeg($1, ARRAY[$2], $3) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- ST_AsPNG
-----------------------------------------------------------------------
-- Cannot be strict as "options" can be NULL
CREATE OR REPLACE FUNCTION st_aspng(rast raster, options text[] DEFAULT NULL)
	RETURNS bytea
	AS $$
	DECLARE
		rast2 raster;
		num_bands int;
		i int;
		pt text;
	BEGIN
		num_bands := st_numbands($1);

		-- PNG allows 1, 3 or 4 bands
		IF num_bands <> 1 AND num_bands <> 3 AND num_bands <> 4 THEN
			RAISE NOTICE 'The PNG format only permits one, three or four bands.  The first band will be used.';
			rast2 := st_band($1, ARRAY[1]);
			num_bands := st_numbands(rast2);
		ELSE
			rast2 := rast;
		END IF;

		-- PNG only supports 8BUI and 16BUI pixeltype
		FOR i IN 1..num_bands LOOP
			pt = st_bandpixeltype(rast, i);
			IF pt != '8BUI' AND pt != '16BUI' THEN
				RAISE EXCEPTION 'The pixel type of band % in the raster is not 8BUI or 16BUI.  The PNG format can only be used with 8BUI and 16BUI pixel types.', i;
			END IF;
		END LOOP;

		RETURN st_asgdalraster(rast2, 'PNG', $2, NULL);
	END;
	$$ LANGUAGE 'plpgsql' IMMUTABLE;

-- Cannot be strict as "options" can be NULL
CREATE OR REPLACE FUNCTION st_aspng(rast raster, nbands int[], options text[] DEFAULT NULL)
	RETURNS bytea
	AS $$ SELECT st_aspng(st_band($1, $2), $3) $$
	LANGUAGE 'SQL' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_aspng(rast raster, nbands int[], compression int)
	RETURNS bytea
	AS $$
	DECLARE
		compression2 int;
		options text[];
	BEGIN
		IF compression IS NOT NULL THEN
			IF compression > 9 THEN
				compression2 := 9;
			ELSEIF compression < 1 THEN
				compression2 := 1;
			ELSE
				compression2 := compression;
			END IF;

			options := array_append(options, 'ZLEVEL=' || compression2);
		END IF;

		RETURN st_aspng(st_band($1, $2), options);
	END;
	$$ LANGUAGE 'plpgsql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_aspng(rast raster, nband int, options text[] DEFAULT NULL)
	RETURNS bytea
	AS $$ SELECT st_aspng(st_band($1, $2), $3) $$
	LANGUAGE 'SQL' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_aspng(rast raster, nband int, compression int)
	RETURNS bytea
	AS $$ SELECT st_aspng($1, ARRAY[$2], $3) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- ST_AsRaster
-----------------------------------------------------------------------
-- None of the ST_AsRaster can be strict as some parameters can be NULL
CREATE OR REPLACE FUNCTION _st_asraster(
	geom geometry,
	scalex double precision DEFAULT 0, scaley double precision DEFAULT 0,
	width integer DEFAULT 0, height integer DEFAULT 0,
	pixeltype text[] DEFAULT ARRAY['8BUI']::text[],
	value double precision[] DEFAULT ARRAY[1]::double precision[],
	nodataval double precision[] DEFAULT ARRAY[0]::double precision[],
	upperleftx double precision DEFAULT NULL, upperlefty double precision DEFAULT NULL,
	gridx double precision DEFAULT NULL, gridy double precision DEFAULT NULL,
	skewx double precision DEFAULT 0, skewy double precision DEFAULT 0,
	touched boolean DEFAULT FALSE
)
	RETURNS raster
	AS 'MODULE_PATHNAME', 'RASTER_asRaster'
	LANGUAGE 'C' STABLE;

CREATE OR REPLACE FUNCTION st_asraster(
	geom geometry,
	scalex double precision, scaley double precision,
	gridx double precision DEFAULT NULL, gridy double precision DEFAULT NULL,
	pixeltype text[] DEFAULT ARRAY['8BUI']::text[],
	value double precision[] DEFAULT ARRAY[1]::double precision[],
	nodataval double precision[] DEFAULT ARRAY[0]::double precision[],
	skewx double precision DEFAULT 0, skewy double precision DEFAULT 0,
	touched boolean DEFAULT FALSE
)
	RETURNS raster
	AS $$ SELECT _st_asraster($1, $2, $3, NULL, NULL, $6, $7, $8, NULL, NULL, $4, $5, $9, $10, $11) $$
	LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION st_asraster(
	geom geometry,
	scalex double precision, scaley double precision,
	pixeltype text[],
	value double precision[] DEFAULT ARRAY[1]::double precision[],
	nodataval double precision[] DEFAULT ARRAY[0]::double precision[],
	upperleftx double precision DEFAULT NULL, upperlefty double precision DEFAULT NULL,
	skewx double precision DEFAULT 0, skewy double precision DEFAULT 0,
	touched boolean DEFAULT FALSE
)
	RETURNS raster
	AS $$ SELECT _st_asraster($1, $2, $3, NULL, NULL, $4, $5, $6, $7, $8, NULL, NULL,	$9, $10, $11) $$
	LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION st_asraster(
	geom geometry,
	width integer, height integer,
	gridx double precision DEFAULT NULL, gridy double precision DEFAULT NULL,
	pixeltype text[] DEFAULT ARRAY['8BUI']::text[],
	value double precision[] DEFAULT ARRAY[1]::double precision[],
	nodataval double precision[] DEFAULT ARRAY[0]::double precision[],
	skewx double precision DEFAULT 0, skewy double precision DEFAULT 0,
	touched boolean DEFAULT FALSE
)
	RETURNS raster
	AS $$ SELECT _st_asraster($1, NULL, NULL, $2, $3, $6, $7, $8, NULL, NULL, $4, $5, $9, $10, $11) $$
	LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION st_asraster(
	geom geometry,
	width integer, height integer,
	pixeltype text[],
	value double precision[] DEFAULT ARRAY[1]::double precision[],
	nodataval double precision[] DEFAULT ARRAY[0]::double precision[],
	upperleftx double precision DEFAULT NULL, upperlefty double precision DEFAULT NULL,
	skewx double precision DEFAULT 0, skewy double precision DEFAULT 0,
	touched boolean DEFAULT FALSE
)
	RETURNS raster
	AS $$ SELECT _st_asraster($1, NULL, NULL, $2, $3, $4, $5, $6, $7, $8, NULL, NULL,	$9, $10, $11) $$
	LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION st_asraster(
	geom geometry,
	scalex double precision, scaley double precision,
	gridx double precision, gridy double precision,
	pixeltype text,
	value double precision DEFAULT 1,
	nodataval double precision DEFAULT 0,
	skewx double precision DEFAULT 0, skewy double precision DEFAULT 0,
	touched boolean DEFAULT FALSE
)
	RETURNS raster
	AS $$ SELECT _st_asraster($1, $2, $3, NULL, NULL, ARRAY[$6]::text[], ARRAY[$7]::double precision[], ARRAY[$8]::double precision[], NULL, NULL, $4, $5, $9, $10, $11) $$
	LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION st_asraster(
	geom geometry,
	scalex double precision, scaley double precision,
	pixeltype text,
	value double precision DEFAULT 1,
	nodataval double precision DEFAULT 0,
	upperleftx double precision DEFAULT NULL, upperlefty double precision DEFAULT NULL,
	skewx double precision DEFAULT 0, skewy double precision DEFAULT 0,
	touched boolean DEFAULT FALSE
)
	RETURNS raster
	AS $$ SELECT _st_asraster($1, $2, $3, NULL, NULL, ARRAY[$4]::text[], ARRAY[$5]::double precision[], ARRAY[$6]::double precision[], $7, $8, NULL, NULL, $9, $10, $11) $$
	LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION st_asraster(
	geom geometry,
	width integer, height integer,
	gridx double precision, gridy double precision,
	pixeltype text,
	value double precision DEFAULT 1,
	nodataval double precision DEFAULT 0,
	skewx double precision DEFAULT 0, skewy double precision DEFAULT 0,
	touched boolean DEFAULT FALSE
)
	RETURNS raster
	AS $$ SELECT _st_asraster($1, NULL, NULL, $2, $3, ARRAY[$6]::text[], ARRAY[$7]::double precision[], ARRAY[$8]::double precision[], NULL, NULL, $4, $5, $9, $10, $11) $$
	LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION st_asraster(
	geom geometry,
	width integer, height integer,
	pixeltype text,
	value double precision DEFAULT 1,
	nodataval double precision DEFAULT 0,
	upperleftx double precision DEFAULT NULL, upperlefty double precision DEFAULT NULL,
	skewx double precision DEFAULT 0, skewy double precision DEFAULT 0,
	touched boolean DEFAULT FALSE
)
	RETURNS raster
	AS $$ SELECT _st_asraster($1, NULL, NULL, $2, $3, ARRAY[$4]::text[], ARRAY[$5]::double precision[], ARRAY[$6]::double precision[], $7, $8, NULL, NULL,$9, $10, $11) $$
	LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION st_asraster(
	geom geometry,
	ref raster,
	pixeltype text[] DEFAULT ARRAY['8BUI']::text[],
	value double precision[] DEFAULT ARRAY[1]::double precision[],
	nodataval double precision[] DEFAULT ARRAY[0]::double precision[],
	touched boolean DEFAULT FALSE
)
	RETURNS raster
	AS $$
	DECLARE
		g geometry;
		g_srid integer;

		ul_x double precision;
		ul_y double precision;
		scale_x double precision;
		scale_y double precision;
		skew_x double precision;
		skew_y double precision;
		sr_id integer;
	BEGIN
		SELECT upperleftx, upperlefty, scalex, scaley, skewx, skewy, srid INTO ul_x, ul_y, scale_x, scale_y, skew_x, skew_y, sr_id FROM ST_Metadata(ref);
		--RAISE NOTICE '%, %, %, %, %, %, %', ul_x, ul_y, scale_x, scale_y, skew_x, skew_y, sr_id;

		-- geometry and raster has different SRID
		g_srid := ST_SRID(geom);
		IF g_srid != sr_id THEN
			RAISE NOTICE 'The geometry''s SRID (%) is not the same as the raster''s SRID (%).  The geometry will be transformed to the raster''s projection', g_srid, sr_id;
			g := ST_Transform(geom, sr_id);
		ELSE
			g := geom;
		END IF;
	
		RETURN _st_asraster(g, scale_x, scale_y, NULL, NULL, $3, $4, $5, NULL, NULL, ul_x, ul_y, skew_x, skew_y, $6);
	END;
	$$ LANGUAGE 'plpgsql' STABLE;

CREATE OR REPLACE FUNCTION st_asraster(
	geom geometry,
	ref raster,
	pixeltype text,
	value double precision DEFAULT 1,
	nodataval double precision DEFAULT 0,
	touched boolean DEFAULT FALSE
)
	RETURNS raster
	AS $$ SELECT st_asraster($1, $2, ARRAY[$3]::text[], ARRAY[$4]::double precision[], ARRAY[$5]::double precision[], $6) $$
	LANGUAGE 'sql' STABLE;

-----------------------------------------------------------------------
-- ST_Resample
-----------------------------------------------------------------------
-- cannot be strict as almost all parameters can be NULL
CREATE OR REPLACE FUNCTION _st_resample(
	rast raster,
	algorithm text DEFAULT 'NearestNeighbour', maxerr double precision DEFAULT 0.125,
	srid integer DEFAULT NULL,
	scalex double precision DEFAULT 0, scaley double precision DEFAULT 0,
	gridx double precision DEFAULT NULL, gridy double precision DEFAULT NULL,
	skewx double precision DEFAULT 0, skewy double precision DEFAULT 0,
	width integer DEFAULT NULL, height integer DEFAULT NULL
)
	RETURNS raster
	AS 'MODULE_PATHNAME', 'RASTER_resample'
	LANGUAGE 'C' STABLE;

CREATE OR REPLACE FUNCTION st_resample(
	rast raster,
	srid integer DEFAULT NULL,
	scalex double precision DEFAULT 0, scaley double precision DEFAULT 0,
	gridx double precision DEFAULT NULL, gridy double precision DEFAULT NULL,
	skewx double precision DEFAULT 0, skewy double precision DEFAULT 0,
	algorithm text DEFAULT 'NearestNeighbour', maxerr double precision DEFAULT 0.125
)
	RETURNS raster
	AS $$ SELECT _st_resample($1, $9,	$10, $2, $3, $4, $5, $6, $7, $8) $$
	LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION st_resample(
	rast raster,
	width integer, height integer,
	srid integer DEFAULT NULL,
	gridx double precision DEFAULT NULL, gridy double precision DEFAULT NULL,
	skewx double precision DEFAULT 0, skewy double precision DEFAULT 0,
	algorithm text DEFAULT 'NearestNeighbour', maxerr double precision DEFAULT 0.125
)
	RETURNS raster
	AS $$ SELECT _st_resample($1, $9,	$10, $4, NULL, NULL, $5, $6, $7, $8, $2, $3) $$
	LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION st_resample(
	rast raster,
	ref raster,
	algorithm text DEFAULT 'NearestNeighbour',
	maxerr double precision DEFAULT 0.125,
	usescale boolean DEFAULT TRUE
)
	RETURNS raster
	AS $$
	DECLARE
		sr_id int;
		dim_x int;
		dim_y int;
		scale_x double precision;
		scale_y double precision;
		grid_x double precision;
		grid_y double precision;
		skew_x double precision;
		skew_y double precision;
	BEGIN
		SELECT srid, width, height, scalex, scaley, upperleftx, upperlefty, skewx, skewy INTO sr_id, dim_x, dim_y, scale_x, scale_y, grid_x, grid_y, skew_x, skew_y FROM st_metadata($2);

		IF usescale IS TRUE THEN
			dim_x := NULL;
			dim_y := NULL;
		ELSE
			scale_x := NULL;
			scale_y := NULL;
		END IF;

		RETURN _st_resample($1, $3, $4, sr_id, scale_x, scale_y, grid_x, grid_y, skew_x, skew_y, dim_x, dim_y);
	END;
	$$ LANGUAGE 'plpgsql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_resample(
	rast raster,
	ref raster,
	usescale boolean,
	algorithm text DEFAULT 'NearestNeighbour',
	maxerr double precision DEFAULT 0.125
)
	RETURNS raster
	AS $$ SELECT st_resample($1, $2, $4, $5, $3) $$
	LANGUAGE 'sql' STABLE STRICT;

-----------------------------------------------------------------------
-- ST_Transform
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_transform(rast raster, srid integer, algorithm text DEFAULT 'NearestNeighbour', maxerr double precision DEFAULT 0.125, scalex double precision DEFAULT 0, scaley double precision DEFAULT 0)
	RETURNS raster
	AS $$ SELECT _st_resample($1, $3, $4, $2, $5, $6) $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_transform(rast raster, srid integer, scalex double precision, scaley double precision, algorithm text DEFAULT 'NearestNeighbour', maxerr double precision DEFAULT 0.125)
	RETURNS raster
	AS $$ SELECT _st_resample($1, $5, $6, $2, $3, $4) $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_transform(rast raster, srid integer, scalexy double precision, algorithm text DEFAULT 'NearestNeighbour', maxerr double precision DEFAULT 0.125)
	RETURNS raster
	AS $$ SELECT _st_resample($1, $4, $5, $2, $3, $3) $$
	LANGUAGE 'sql' STABLE STRICT;

-----------------------------------------------------------------------
-- ST_Rescale
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_rescale(rast raster, scalex double precision, scaley double precision, algorithm text DEFAULT 'NearestNeighbour', maxerr double precision DEFAULT 0.125)
	RETURNS raster
	AS $$ SELECT _st_resample($1, $4, $5, NULL, $2, $3) $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_rescale(rast raster, scalexy double precision, algorithm text DEFAULT 'NearestNeighbour', maxerr double precision DEFAULT 0.125)
	RETURNS raster
	AS $$ SELECT _st_resample($1, $3, $4, NULL, $2, $2) $$
	LANGUAGE 'sql' STABLE STRICT;

-----------------------------------------------------------------------
-- ST_Reskew
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_reskew(rast raster, skewx double precision, skewy double precision, algorithm text DEFAULT 'NearestNeighbour', maxerr double precision DEFAULT 0.125)
	RETURNS raster
	AS $$ SELECT _st_resample($1, $4, $5, NULL, 0, 0, NULL, NULL, $2, $3) $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_reskew(rast raster, skewxy double precision, algorithm text DEFAULT 'NearestNeighbour', maxerr double precision DEFAULT 0.125)
	RETURNS raster
	AS $$ SELECT _st_resample($1, $3, $4, NULL, 0, 0, NULL, NULL, $2, $2) $$
	LANGUAGE 'sql' STABLE STRICT;

-----------------------------------------------------------------------
-- ST_SnapToGrid
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_snaptogrid(rast raster, gridx double precision, gridy double precision, algorithm text DEFAULT 'NearestNeighbour', maxerr double precision DEFAULT 0.125, scalex double precision DEFAULT 0, scaley double precision DEFAULT 0)
	RETURNS raster
	AS $$ SELECT _st_resample($1, $4, $5, NULL, $6, $7, $2, $3) $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_snaptogrid(rast raster, gridx double precision, gridy double precision, scalex double precision, scaley double precision, algorithm text DEFAULT 'NearestNeighbour', maxerr double precision DEFAULT 0.125)
	RETURNS raster
	AS $$ SELECT _st_resample($1, $6, $7, NULL, $4, $5, $2, $3) $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_snaptogrid(rast raster, gridx double precision, gridy double precision, scalexy double precision, algorithm text DEFAULT 'NearestNeighbour', maxerr double precision DEFAULT 0.125)
	RETURNS raster
	AS $$ SELECT _st_resample($1, $5, $6, NULL, $4, $4, $2, $3) $$
	LANGUAGE 'sql' STABLE STRICT;

-----------------------------------------------------------------------
-- One Raster ST_MapAlgebra
-----------------------------------------------------------------------
-- This function can not be STRICT, because nodataval can be NULL
-- or pixeltype can not be determined (could be st_bandpixeltype(raster, band) though)
CREATE OR REPLACE FUNCTION st_mapalgebraexpr(rast raster, band integer, pixeltype text,
        expression text, nodataval double precision DEFAULT NULL)
    RETURNS raster
    AS 'MODULE_PATHNAME', 'RASTER_mapAlgebraExpr'
    LANGUAGE 'C' IMMUTABLE;

-- This function can not be STRICT, because nodataval can be NULL
-- or pixeltype can not be determined (could be st_bandpixeltype(raster, band) though)
CREATE OR REPLACE FUNCTION st_mapalgebraexpr(rast raster, pixeltype text, expression text,
        nodataval double precision DEFAULT NULL)
    RETURNS raster
    AS $$ SELECT st_mapalgebraexpr($1, 1, $2, $3, $4) $$
    LANGUAGE SQL;

-- All arguments supplied, use the C implementation.
CREATE OR REPLACE FUNCTION st_mapalgebrafct(rast raster, band integer, 
        pixeltype text, onerastuserfunc regprocedure, variadic args text[]) 
    RETURNS raster
    AS 'MODULE_PATHNAME', 'RASTER_mapAlgebraFct'
    LANGUAGE 'C' IMMUTABLE;

-- Variant 1: missing user args
CREATE OR REPLACE FUNCTION st_mapalgebrafct(rast raster, band integer,
        pixeltype text, onerastuserfunc regprocedure)
    RETURNS raster
    AS $$ SELECT st_mapalgebrafct($1, $2, $3, $4, NULL) $$
    LANGUAGE SQL;

-- Variant 2: missing pixeltype; default to pixeltype of rast
CREATE OR REPLACE FUNCTION st_mapalgebrafct(rast raster, band integer,
        onerastuserfunc regprocedure, variadic args text[])
    RETURNS raster
    AS $$ SELECT st_mapalgebrafct($1, $2, NULL, $3, VARIADIC $4) $$
    LANGUAGE SQL;
 
-- Variant 3: missing pixeltype and user args; default to pixeltype of rast
CREATE OR REPLACE FUNCTION st_mapalgebrafct(rast raster, band integer,
        onerastuserfunc regprocedure)
    RETURNS raster
    AS $$ SELECT st_mapalgebrafct($1, $2, NULL, $3, NULL) $$
    LANGUAGE SQL;

-- Variant 4: missing band; default to band 1
CREATE OR REPLACE FUNCTION st_mapalgebrafct(rast raster, pixeltype text,
        onerastuserfunc regprocedure, variadic args text[])
    RETURNS raster
    AS $$ SELECT st_mapalgebrafct($1, 1, $2, $3, VARIADIC $4) $$
    LANGUAGE SQL;

-- Variant 5: missing band and user args; default to band 1
CREATE OR REPLACE FUNCTION st_mapalgebrafct(rast raster, pixeltype text,
        onerastuserfunc regprocedure)
    RETURNS raster
    AS $$ SELECT st_mapalgebrafct($1, 1, $2, $3, NULL) $$
    LANGUAGE SQL;

-- Variant 6: missing band, and pixeltype; default to band 1, pixeltype of rast.
CREATE OR REPLACE FUNCTION st_mapalgebrafct(rast raster, onerastuserfunc regprocedure,
        variadic args text[])
    RETURNS raster
    AS $$ SELECT st_mapalgebrafct($1, 1, NULL, $2, VARIADIC $3) $$
    LANGUAGE SQL;

-- Variant 7: missing band, pixeltype, and user args; default to band 1, pixeltype of rast.
CREATE OR REPLACE FUNCTION st_mapalgebrafct(rast raster, onerastuserfunc regprocedure)
    RETURNS raster
    AS $$ SELECT st_mapalgebrafct($1, 1, NULL, $2, NULL) $$
    LANGUAGE SQL;

-----------------------------------------------------------------------
-- Two Raster ST_MapAlgebra
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_mapalgebraexpr(
	rast1 raster, band1 integer,
	rast2 raster, band2 integer,
	expression text,
	pixeltype text DEFAULT NULL, extenttype text DEFAULT 'INTERSECTION',
	nodata1expr text DEFAULT NULL, nodata2expr text DEFAULT NULL,
	nodatanodataval double precision DEFAULT NULL
)
	RETURNS raster
	AS 'MODULE_PATHNAME', 'RASTER_mapAlgebra2'
	LANGUAGE 'C' STABLE;

CREATE OR REPLACE FUNCTION st_mapalgebraexpr(
	rast1 raster,
	rast2 raster,
	expression text,
	pixeltype text DEFAULT NULL, extenttype text DEFAULT 'INTERSECTION',
	nodata1expr text DEFAULT NULL, nodata2expr text DEFAULT NULL,
	nodatanodataval double precision DEFAULT NULL
)
	RETURNS raster
	AS $$ SELECT st_mapalgebraexpr($1, 1, $2, 1, $3, $4, $5, $6, $7, $8) $$
	LANGUAGE 'SQL' STABLE;

CREATE OR REPLACE FUNCTION st_mapalgebrafct(
	rast1 raster, band1 integer,
	rast2 raster, band2 integer,
	tworastuserfunc regprocedure,
	pixeltype text DEFAULT NULL, extenttype text DEFAULT 'INTERSECTION',
	VARIADIC userargs text[] DEFAULT NULL
)
	RETURNS raster
	AS 'MODULE_PATHNAME', 'RASTER_mapAlgebra2'
	LANGUAGE 'C' STABLE;

CREATE OR REPLACE FUNCTION st_mapalgebrafct(
	rast1 raster,
	rast2 raster,
	tworastuserfunc regprocedure,
	pixeltype text DEFAULT NULL, extenttype text DEFAULT 'INTERSECTION',
	VARIADIC userargs text[] DEFAULT NULL
)
	RETURNS raster
	AS $$ SELECT st_mapalgebrafct($1, 1, $2, 1, $3, $4, $5, VARIADIC $6) $$
	LANGUAGE 'SQL' STABLE;

-----------------------------------------------------------------------
-- Neighborhood single raster map algebra
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_mapalgebrafctngb(
    rast raster,
    band integer,
    pixeltype text,
    ngbwidth integer,
    ngbheight integer,
    onerastngbuserfunc regprocedure,
    nodatamode text,
    variadic args text[]
)
    RETURNS raster
    AS 'MODULE_PATHNAME', 'RASTER_mapAlgebraFctNgb'
    LANGUAGE 'C' IMMUTABLE;

-----------------------------------------------------------------------
-- Get information about the raster
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_isempty(rast raster)
    RETURNS boolean
    AS 'MODULE_PATHNAME', 'RASTER_isEmpty'
    LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_hasnoband(rast raster, nband int DEFAULT 1)
    RETURNS boolean
    AS 'MODULE_PATHNAME', 'RASTER_hasNoBand'
    LANGUAGE 'C' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- Raster Band Accessors
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION st_bandnodatavalue(rast raster, band integer DEFAULT 1)
    RETURNS double precision
    AS 'MODULE_PATHNAME','RASTER_getBandNoDataValue'
    LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_bandisnodata(rast raster, band integer,
        forceChecking boolean)
    RETURNS boolean
    AS 'MODULE_PATHNAME', 'RASTER_bandIsNoData'
    LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_bandisnodata(rast raster, forceChecking boolean)
    RETURNS boolean
    AS $$ SELECT st_bandisnodata($1, 1, $2) $$
    LANGUAGE SQL IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_bandisnodata(rast raster, band integer)
    RETURNS boolean
    AS $$ SELECT st_bandisnodata($1, $2, FALSE) $$
    LANGUAGE SQL IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_bandisnodata(rast raster)
    RETURNS boolean
    AS $$ SELECT st_bandisnodata($1, 1, FALSE) $$
    LANGUAGE SQL IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_bandpath(rast raster, band integer)
    RETURNS text
    AS 'MODULE_PATHNAME','RASTER_getBandPath'
    LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_bandpath(raster)
    RETURNS text
    AS $$ SELECT st_bandpath($1, 1) $$
    LANGUAGE SQL IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_bandpixeltype(rast raster, band integer)
    RETURNS text
    AS 'MODULE_PATHNAME','RASTER_getBandPixelTypeName'
    LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_bandpixeltype(raster)
    RETURNS text
    AS $$ SELECT st_bandpixeltype($1, 1) $$
    LANGUAGE SQL IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_bandmetadata(
	rast raster,
	band int DEFAULT 1,
	OUT pixeltype text,
	OUT hasnodata boolean,
	OUT nodatavalue double precision,
	OUT isoutdb boolean,
	OUT path text
)
	AS 'MODULE_PATHNAME','RASTER_bandmetadata'
	LANGUAGE 'C' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- Raster Pixel Accessors
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION st_value(rast raster, band integer, x integer, y integer, hasnodata boolean)
    RETURNS float8
    AS 'MODULE_PATHNAME','RASTER_getPixelValue'
    LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_value(rast raster, band integer, x integer, y integer)
    RETURNS float8
    AS $$ SELECT st_value($1, $2, $3, $4, TRUE) $$
    LANGUAGE SQL IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_value(rast raster, x integer, y integer, hasnodata boolean)
    RETURNS float8
    AS $$ SELECT st_value($1, 1, $2, $3, $4) $$
    LANGUAGE SQL IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_value(rast raster, x integer, y integer)
    RETURNS float8
    AS $$ SELECT st_value($1, 1, $2, $3, TRUE) $$
    LANGUAGE SQL IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_value(rast raster, band integer, pt geometry, hasnodata boolean)
    RETURNS float8 AS
    $$
    DECLARE
        x float8;
        y float8;
        gtype text;
    BEGIN
        gtype := st_geometrytype(pt);
        IF ( gtype != 'ST_Point' ) THEN
            RAISE EXCEPTION 'Attempting to get the value of a pixel with a non-point geometry';
        END IF;
        x := st_x(pt);
        y := st_y(pt);
        RETURN st_value(rast,
                        band,
                        st_world2rastercoordx(rast, x, y),
                        st_world2rastercoordy(rast, x, y),
                        hasnodata);
    END;
    $$
    LANGUAGE 'plpgsql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_value(rast raster, band integer, pt geometry)
    RETURNS float8
    AS $$ SELECT st_value($1, $2, $3, TRUE) $$
    LANGUAGE SQL IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_value(rast raster, pt geometry, hasnodata boolean)
    RETURNS float8
    AS $$ SELECT st_value($1, 1, $2, $3) $$
    LANGUAGE SQL IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_value(rast raster, pt geometry)
    RETURNS float8
    AS $$ SELECT st_value($1, 1, $2, TRUE) $$
    LANGUAGE SQL IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- Raster Accessors ST_Georeference()
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION st_georeference(rast raster, format text)
    RETURNS text AS
    $$
    DECLARE
				scale_x numeric;
				scale_y numeric;
				skew_x numeric;
				skew_y numeric;
				ul_x numeric;
				ul_y numeric;

        result text;
    BEGIN
			SELECT scalex::numeric, scaley::numeric, skewx::numeric, skewy::numeric, upperleftx::numeric, upperlefty::numeric
				INTO scale_x, scale_y, skew_x, skew_y, ul_x, ul_y FROM ST_Metadata(rast);

						-- scale x
            result := trunc(scale_x, 10) || E'\n';

						-- skew y
            result := result || trunc(skew_y, 10) || E'\n';

						-- skew x
            result := result || trunc(skew_x, 10) || E'\n';

						-- scale y
            result := result || trunc(scale_y, 10) || E'\n';

        IF format = 'ESRI' THEN
						-- upper left x
            result := result || trunc((ul_x + scale_x * 0.5), 10) || E'\n';

						-- upper left y
            result = result || trunc((ul_y + scale_y * 0.5), 10) || E'\n';
        ELSE -- IF format = 'GDAL' THEN
						-- upper left x
            result := result || trunc(ul_x, 10) || E'\n';

						-- upper left y
            result := result || trunc(ul_y, 10) || E'\n';
        END IF;

        RETURN result;
    END;
    $$
    LANGUAGE 'plpgsql' IMMUTABLE STRICT; -- WITH (isstrict);

CREATE OR REPLACE FUNCTION st_georeference(raster)
    RETURNS text
    AS $$ select st_georeference($1,'GDAL') $$
    LANGUAGE 'SQL' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- Raster Editors
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION st_setscale(rast raster, scale float8)
    RETURNS raster
    AS 'MODULE_PATHNAME','RASTER_setScale'
    LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_setscale(rast raster, scalex float8, scaley float8)
    RETURNS raster
    AS 'MODULE_PATHNAME','RASTER_setScaleXY'
    LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_setskew(rast raster, skew float8)
    RETURNS raster
    AS 'MODULE_PATHNAME','RASTER_setSkew'
    LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_setskew(rast raster, skewx float8, skewy float8)
    RETURNS raster
    AS 'MODULE_PATHNAME','RASTER_setSkewXY'
    LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_setsrid(rast raster, srid integer)
    RETURNS raster
    AS 'MODULE_PATHNAME','RASTER_setSRID'
    LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_setupperleft(rast raster, upperleftx float8, upperlefty float8)
    RETURNS raster
    AS 'MODULE_PATHNAME','RASTER_setUpperLeftXY'
    LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_setrotation(rast raster, rotation float8)
    RETURNS raster
    AS 'MODULE_PATHNAME','RASTER_setRotation'
    LANGUAGE 'C' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- Raster Editors ST_SetGeoreference()
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_setgeoreference(rast raster, georef text, format text DEFAULT 'GDAL')
    RETURNS raster AS
    $$
    DECLARE
        params text[];
        rastout raster;
    BEGIN
        IF rast IS NULL THEN
            RAISE WARNING 'Cannot set georeferencing on a null raster in st_setgeoreference.';
            RETURN rastout;
        END IF;

        SELECT regexp_matches(georef,
            E'(-?\\d+(?:\\.\\d+)?)\\s(-?\\d+(?:\\.\\d+)?)\\s(-?\\d+(?:\\.\\d+)?)\\s' ||
            E'(-?\\d+(?:\\.\\d+)?)\\s(-?\\d+(?:\\.\\d+)?)\\s(-?\\d+(?:\\.\\d+)?)') INTO params;

        IF NOT FOUND THEN
            RAISE EXCEPTION 'st_setgeoreference requires a string with 6 floating point values.';
        END IF;

        IF format = 'ESRI' THEN
            -- params array is now:
            -- {scalex, skewy, skewx, scaley, upperleftx, upperlefty}
            rastout := st_setscale(rast, params[1]::float8, params[4]::float8);
            rastout := st_setskew(rastout, params[3]::float8, params[2]::float8);
            rastout := st_setupperleft(rastout,
                                   params[5]::float8 - (params[1]::float8 * 0.5),
                                   params[6]::float8 - (params[4]::float8 * 0.5));
        ELSE
            IF format != 'GDAL' THEN
                RAISE WARNING E'Format \'%\' is not recognized, defaulting to GDAL format.', format;
            END IF;
            -- params array is now:
            -- {scalex, skewy, skewx, scaley, upperleftx, upperlefty}

            rastout := st_setscale(rast, params[1]::float8, params[4]::float8);
            rastout := st_setskew( rastout, params[3]::float8, params[2]::float8);
            rastout := st_setupperleft(rastout, params[5]::float8, params[6]::float8);
        END IF;
        RETURN rastout;
    END;
    $$
    LANGUAGE 'plpgsql' IMMUTABLE STRICT; -- WITH (isstrict);

-----------------------------------------------------------------------
-- Raster Band Editors
-----------------------------------------------------------------------

-- This function can not be STRICT, because nodatavalue can be NULL indicating that no nodata value should be set
CREATE OR REPLACE FUNCTION st_setbandnodatavalue(rast raster, band integer, nodatavalue float8, forceChecking boolean DEFAULT FALSE)
    RETURNS raster
    AS 'MODULE_PATHNAME','RASTER_setBandNoDataValue'
    LANGUAGE 'C' IMMUTABLE;

-- This function can not be STRICT, because nodatavalue can be NULL indicating that no nodata value should be set
CREATE OR REPLACE FUNCTION st_setbandnodatavalue(rast raster, nodatavalue float8)
    RETURNS raster
    AS $$ SELECT st_setbandnodatavalue($1, 1, $2, FALSE) $$
    LANGUAGE SQL;

CREATE OR REPLACE FUNCTION st_setbandisnodata(rast raster, band integer DEFAULT 1)
    RETURNS raster
    AS 'MODULE_PATHNAME', 'RASTER_setBandIsNoData'
    LANGUAGE 'C' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- Raster Pixel Editors
-----------------------------------------------------------------------

-- This function can not be STRICT, because newvalue can be NULL for nodata
CREATE OR REPLACE FUNCTION st_setvalue(rast raster, band integer, x integer, y integer, newvalue float8)
    RETURNS raster
    AS 'MODULE_PATHNAME','RASTER_setPixelValue'
    LANGUAGE 'C' IMMUTABLE;

-- This function can not be STRICT, because newvalue can be NULL for nodata
CREATE OR REPLACE FUNCTION st_setvalue(rast raster, x integer, y integer, newvalue float8)
    RETURNS raster
    AS $$ SELECT st_setvalue($1, 1, $2, $3, $4) $$
    LANGUAGE SQL;

-- This function can not be STRICT, because newvalue can be NULL for nodata
CREATE OR REPLACE FUNCTION st_setvalue(rast raster, band integer, pt geometry, newvalue float8)
    RETURNS raster AS
    $$
    DECLARE
        x float8;
        y float8;
        gtype text;
    BEGIN
        gtype := st_geometrytype(pt);
        IF ( gtype != 'ST_Point' ) THEN
            RAISE EXCEPTION 'Attempting to get the value of a pixel with a non-point geometry';
        END IF;
        x := st_x(pt);
        y := st_y(pt);
        RETURN st_setvalue(rast,
                           band,
                           st_world2rastercoordx(rast, x, y),
                           st_world2rastercoordy(rast, x, y),
                           newvalue);
    END;
    $$
    LANGUAGE 'plpgsql' IMMUTABLE;

-- This function can not be STRICT, because newvalue can be NULL for nodata
CREATE OR REPLACE FUNCTION st_setvalue(rast raster, pt geometry, newvalue float8)
    RETURNS raster
    AS $$ SELECT st_setvalue($1, 1, $2, $3) $$
    LANGUAGE SQL;


-----------------------------------------------------------------------
-- Raster Processing Functions
-----------------------------------------------------------------------

CREATE TYPE geomval AS (
    geom geometry,
    val double precision
);

CREATE TYPE wktgeomval AS (
    wktgeom text,
    val double precision,
    srid int
);

CREATE OR REPLACE FUNCTION dumpaswktpolygons(rast raster, band integer)
    RETURNS SETOF wktgeomval
    AS 'MODULE_PATHNAME','RASTER_dumpAsWKTPolygons'
    LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_dumpaspolygons(rast raster, band integer DEFAULT 1)
    RETURNS SETOF geomval AS
    $$
    SELECT st_geomfromtext(wktgeomval.wktgeom, wktgeomval.srid), wktgeomval.val
    FROM dumpaswktpolygons($1, $2) AS wktgeomval;
    $$
    LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_polygon(rast raster, band integer DEFAULT 1)
    RETURNS geometry AS
    $$
    SELECT st_union(f.geom) AS singlegeom
    FROM (SELECT (st_dumpaspolygons($1, $2)).geom AS geom) AS f;
    $$
    LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_pixelaspolygon(rast raster, band integer, x integer, y integer)
    RETURNS geometry AS
    $$
    DECLARE
        w int;
        h int;
        scale_x float8;
        scale_y float8;
        skew_x float8;
        skew_y float8;
        ul_x float8;
        ul_y float8;
				sr_id int;
        x1 float8;
        y1 float8;
        x2 float8;
        y2 float8;
        x3 float8;
        y3 float8;
        x4 float8;
        y4 float8;
    BEGIN
				SELECT scalex, scaley, skewx, skewy, upperleftx, upperlefty, srid INTO scale_x, scale_y, skew_x, skew_y, ul_x, ul_y, sr_id FROM ST_Metadata(rast);
        x1 := scale_x * (x - 1) + skew_x * (y - 1) + ul_x;
        y1 := scale_y * (y - 1) + skew_y * (x - 1) + ul_y;
        x2 := x1 + scale_x;
        y2 := y1 + skew_y;
        x3 := x1 + scale_x + skew_x;
        y3 := y1 + scale_y + skew_y;
        x4 := x1 + skew_x;
        y4 := y1 + scale_y;
        RETURN st_setsrid(st_makepolygon(st_makeline(ARRAY[st_makepoint(x1, y1),
                                                           st_makepoint(x2, y2),
                                                           st_makepoint(x3, y3),
                                                           st_makepoint(x4, y4),
                                                           st_makepoint(x1, y1)]
                                                    )
                                        ),
                          sr_id
                         );
    END;
    $$
    LANGUAGE 'plpgsql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_pixelaspolygon(rast raster, x integer, y integer)
    RETURNS geometry AS
    $$
        SELECT st_pixelaspolygon($1, 1, $2, $3)
    $$
    LANGUAGE SQL IMMUTABLE STRICT;


-----------------------------------------------------------------------
-- Raster Utility Functions
-----------------------------------------------------------------------

---------------------------------------------------------------------------------
-- ST_World2RasterCoordX(rast raster, xw float8, yw float8)
-- Returns the column number of the pixel covering the provided X and Y world
-- coordinates.
-- This function works even if the world coordinates are outside the raster extent.
---------------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_world2rastercoordx(rast raster, xw float8, yw float8)
    RETURNS int AS
    $$
    DECLARE
        a float8 := 0.0;
        b float8 := 0.0;
        c float8 := 0.0;
        d float8 := 0.0;
        e float8 := 0.0;
        f float8 := 0.0;
        xr numeric := 0.0;
    BEGIN
				SELECT scalex, skewx, upperleftx, skewy, scaley, upperlefty INTO a, b, c, d, e, f FROM ST_Metadata(rast);
        IF ( b * d - a * e = 0 ) THEN
            RAISE EXCEPTION 'Attempting to compute raster coordinate on a raster with scale equal to 0';
        END IF;
        xr := (b * (yw - f) - e * (xw - c)) / (b * d - a * e);
        RETURN floor(xr) + 1;
    END;
    $$
    LANGUAGE 'plpgsql' IMMUTABLE STRICT;

---------------------------------------------------------------------------------
-- ST_World2RasterCoordX(rast raster, xw float8)
-- Returns the column number of the pixels covering the provided world X coordinate
-- for a non-rotated raster.
-- This function works even if the world coordinate is outside the raster extent.
-- This function returns an error if the raster is rotated. In this case you must
-- also provide a Y.
---------------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_world2rastercoordx(rast raster, xw float8)
    RETURNS int AS
    $$
    DECLARE
        a float8 := 0.0;
        b float8 := 0.0;
        c float8 := 0.0;
        d float8 := 0.0;
        e float8 := 0.0;
        f float8 := 0.0;
        xr numeric := 0.0;
    BEGIN
 				SELECT scalex, skewx, upperleftx, skewy, scaley, upperlefty INTO a, b, c, d, e, f FROM ST_Metadata(rast);
        IF ( b * d - a * e = 0 ) THEN
            RAISE EXCEPTION 'Attempting to compute raster coordinate on a raster with scale equal to 0';
        END IF;
        IF ( b != 0 OR d != 0) THEN
            RAISE EXCEPTION 'Attempting to compute raster coordinate on a raster with rotation providing x only. A y must also be provided';
        END IF;
        xr := (e * (xw - c)) / (a * e);
        RETURN floor(xr) + 1;
    END;
    $$
    LANGUAGE 'plpgsql' IMMUTABLE STRICT;

---------------------------------------------------------------------------------
-- ST_World2RasterCoordX(rast raster, pt geometry)
-- Returns the column number of the pixel covering the provided point geometry.
-- This function works even if the point is outside the raster extent.
---------------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_world2rastercoordx(rast raster, pt geometry)
    RETURNS int AS
    $$
    DECLARE
    BEGIN
        IF ( st_geometrytype(pt) != 'ST_Point' ) THEN
            RAISE EXCEPTION 'Attempting to compute raster coordinate with a non-point geometry';
        END IF;
        RETURN st_world2rastercoordx($1, st_x(pt), st_y(pt));
    END;
    $$
    LANGUAGE 'plpgsql' IMMUTABLE STRICT;

---------------------------------------------------------------------------------
-- ST_World2RasterCoordY(rast raster, xw float8, yw float8)
-- Returns the row number of the pixel covering the provided X and Y world
-- coordinates.
-- This function works even if the world coordinates are outside the raster extent.
---------------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_world2rastercoordy(rast raster, xw float8, yw float8)
    RETURNS int AS
    $$
    DECLARE
        a float8 := 0.0;
        b float8 := 0.0;
        c float8 := 0.0;
        d float8 := 0.0;
        e float8 := 0.0;
        f float8 := 0.0;
        yr numeric := 0.0;
    BEGIN
 				SELECT scalex, skewx, upperleftx, skewy, scaley, upperlefty INTO a, b, c, d, e, f FROM ST_Metadata(rast);
        IF ( b * d - a * e = 0 ) THEN
            RAISE EXCEPTION 'Attempting to compute raster coordinate on a raster with scale equal to 0';
        END IF;
        yr := (a * (yw - f) + d * (c - xw)) / (a * e - b * d);
        RETURN floor(yr) + 1;
    END;
    $$
    LANGUAGE 'plpgsql' IMMUTABLE STRICT;

---------------------------------------------------------------------------------
-- ST_World2RasterCoordY(rast raster, yw float8)
-- Returns the row number of the pixels covering the provided world Y coordinate
-- for a non-rotated raster.
-- This function works even if the world coordinate is outside the raster extent.
-- This function returns an error if the raster is rotated. In this case you must
-- also provide an X.
---------------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_world2rastercoordy(rast raster, yw float8)
    RETURNS int AS
    $$
    DECLARE
        a float8 := 0.0;
        b float8 := 0.0;
        c float8 := 0.0;
        d float8 := 0.0;
        e float8 := 0.0;
        f float8 := 0.0;
        yr numeric := 0.0;
    BEGIN
 				SELECT scalex, skewx, upperleftx, skewy, scaley, upperlefty INTO a, b, c, d, e, f FROM ST_Metadata(rast);
        IF ( b * d - a * e = 0 ) THEN
            RAISE EXCEPTION 'Attempting to compute raster coordinate on a raster with scale equal to 0';
        END IF;
        IF ( b != 0 OR d != 0) THEN
            RAISE EXCEPTION 'Attempting to compute raster coordinate on a raster with rotation providing y only. A x must also be provided';
        END IF;
        yr := (a * (yw - f)) / (a * e);
        RETURN floor(yr) + 1;
    END;
    $$
    LANGUAGE 'plpgsql' IMMUTABLE STRICT;

---------------------------------------------------------------------------------
-- ST_World2RasterCoordY(rast raster, pt geometry)
-- Returns the row number of the pixel covering the provided point geometry.
-- This function works even if the point is outside the raster extent.
---------------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_world2rastercoordy(rast raster, pt geometry)
    RETURNS int AS
    $$
    DECLARE
    BEGIN
        IF ( st_geometrytype(pt) != 'ST_Point' ) THEN
            RAISE EXCEPTION 'Attempting to compute raster coordinate with a non-point geometry';
        END IF;
        RETURN st_world2rastercoordy($1, st_x(pt), st_y(pt));
    END;
    $$
    LANGUAGE 'plpgsql' IMMUTABLE STRICT;

---------------------------------------------------------------------------------
-- ST_Raster2WorldCoordX(rast raster, xr int, yr int)
-- Returns the X world coordinate of the upper left corner of the pixel located at
-- the provided column and row numbers.
-- This function works even if the provided raster column and row are beyond or
-- below the raster width and height.
---------------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_raster2worldcoordx(rast raster, xr int, yr int)
    RETURNS float8 AS
    $$
    DECLARE
        a float8 := 0.0;
        b float8 := 0.0;
        c float8 := 0.0;
        xw numeric := 0.0;
    BEGIN
				SELECT scalex, skewx, upperleftx INTO a, b, c FROM ST_Metadata(rast);
        xw := (a::numeric * (xr::numeric - 1.0) + b::numeric * (yr::numeric - 1.0) + c::numeric)::numeric;
        RETURN xw;
    END;
    $$
    LANGUAGE 'plpgsql' IMMUTABLE STRICT;

---------------------------------------------------------------------------------
-- ST_Raster2WorldCoordX(rast raster, xr int)
-- Returns the X world coordinate of the upper left corner of the pixel located at
-- the provided column number for a non-rotated raster.
-- This function works even if the provided raster column is beyond or below the
-- raster width.
-- This function returns an error if the raster is rotated. In this case you must
-- also provide a Y.
---------------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_raster2worldcoordx(rast raster, xr int)
    RETURNS float8 AS
    $$
    DECLARE
        a float8 := 0.0;
        b float8 := 0.0;
        c float8 := 0.0;
        xw numeric := 0.0;
    BEGIN
				SELECT scalex, skewx, upperleftx INTO a, b, c FROM ST_Metadata(rast);
        IF ( b != 0 ) THEN
            RAISE EXCEPTION 'Attempting to compute raster coordinates on a raster with rotation providing X only. A Y coordinate must also be provided';
        END IF;
        xw := (a::numeric * (xr::numeric - 1.0) + c::numeric)::numeric;
        RETURN xw;
    END;
    $$
    LANGUAGE 'plpgsql' IMMUTABLE STRICT;

---------------------------------------------------------------------------------
-- ST_Raster2WorldCoordY(rast raster, xr int, yr int)
-- Returns the Y world coordinate of the upper left corner of the pixel located at
-- the provided column and row numbers.
-- This function works even if the provided raster column and row are beyond or
-- below the raster width and height.
---------------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_raster2worldcoordy(rast raster, xr int, yr int)
    RETURNS float8 AS
    $$
    DECLARE
        d float8 := 0.0;
        e float8 := 0.0;
        f float8 := 0.0;
        yw numeric := 0.0;
    BEGIN
				SELECT skewy, scaley, upperlefty INTO d, e, f FROM ST_Metadata(rast);
        yw := (d::numeric * (xr::numeric - 1.0) + e::numeric * (yr::numeric - 1.0) + f::numeric)::numeric;
        RETURN yw;
    END;
    $$
    LANGUAGE 'plpgsql' IMMUTABLE STRICT;

---------------------------------------------------------------------------------
-- ST_Raster2WorldCoordY(rast raster, yr int)
-- Returns the Y world coordinate of the upper left corner of the pixel located at
-- the provided row number for a non-rotated raster.
-- This function works even if the provided raster row is beyond or below the
-- raster height.
-- This function returns an error if the raster is rotated. In this case you must
-- also provide an X.
---------------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_raster2worldcoordy(rast raster, yr int)
    RETURNS float8 AS
    $$
    DECLARE
        d float8 := 0.0;
        e float8 := 0.0;
        f float8 := 0.0;
        yw numeric := 0.0;
    BEGIN
				SELECT skewy, scaley, upperlefty INTO d, e, f FROM ST_Metadata(rast);
        IF ( d != 0 ) THEN
            RAISE EXCEPTION 'Attempting to compute raster coordinates on a raster with rotation providing Y only. An X coordinate must also be provided';
        END IF;
        yw := (e::numeric * (yr::numeric - 1.0) + f::numeric)::numeric;
        RETURN yw;
    END;
    $$
    LANGUAGE 'plpgsql' IMMUTABLE STRICT;


-----------------------------------------------------------------------
-- Raster Outputs
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION st_asbinary(raster)
    RETURNS bytea
    AS 'MODULE_PATHNAME', 'RASTER_to_binary'
    LANGUAGE 'C' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_bytea(raster)
    RETURNS bytea
    AS 'MODULE_PATHNAME', 'RASTER_to_bytea'
    LANGUAGE 'C' IMMUTABLE;

------------------------------------------------------------------------------
--  Casts
------------------------------------------------------------------------------

CREATE CAST (raster AS box2d)
    WITH FUNCTION box2d(raster) AS IMPLICIT;

CREATE CAST (raster AS geometry)
    WITH FUNCTION st_convexhull(raster) AS IMPLICIT;

CREATE CAST (raster AS bytea)
    WITH FUNCTION st_bytea(raster) AS IMPLICIT;

------------------------------------------------------------------------------
--  GiST index OPERATOR support functions
------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION st_overleft(raster, raster)
    RETURNS bool
    AS 'select $1::geometry &< $2::geometry'
    LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_overright(raster, raster)
    RETURNS bool
    AS 'select $1::geometry &> $2::geometry'
    LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_left(raster, raster)
    RETURNS bool
    AS 'select $1::geometry << $2::geometry'
    LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_right(raster, raster)
    RETURNS bool
    AS 'select $1::geometry >> $2::geometry'
    LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_overabove(raster, raster)
    RETURNS bool
    AS 'select $1::geometry |&> $2::geometry'
    LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_overbelow(raster, raster)
    RETURNS bool
    AS 'select $1::geometry &<| $2::geometry'
    LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_above(raster, raster)
    RETURNS bool
    AS 'select $1::geometry |>> $2::geometry'
    LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_below(raster, raster)
    RETURNS bool
    AS 'select $1::geometry <<| $2::geometry'
    LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_same(raster, raster)
    RETURNS bool
    AS 'select $1::geometry ~= $2::geometry'
    LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_contained(raster, raster)
    RETURNS bool
    AS 'select $1::geometry @ $2::geometry'
    LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_contain(raster, raster)
    RETURNS bool
    AS 'select $1::geometry ~ $2::geometry'
    LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_overlap(raster, raster)
    RETURNS bool
    AS 'select $1::geometry && $2::geometry'
    LANGUAGE 'SQL' IMMUTABLE STRICT;

------------------------------------------------------------------------------
--  GiST index OPERATORs
------------------------------------------------------------------------------

CREATE OPERATOR << (
    LEFTARG = raster, RIGHTARG = raster, PROCEDURE = st_left,
    COMMUTATOR = '>>',
    RESTRICT = positionsel, JOIN = positionjoinsel
    );

CREATE OPERATOR &< (
    LEFTARG = raster, RIGHTARG = raster, PROCEDURE = st_overleft,
    COMMUTATOR = '&>',
    RESTRICT = positionsel, JOIN = positionjoinsel
    );

CREATE OPERATOR <<| (
    LEFTARG = raster, RIGHTARG = raster, PROCEDURE = st_below,
    COMMUTATOR = '|>>',
    RESTRICT = positionsel, JOIN = positionjoinsel
    );

CREATE OPERATOR &<| (
    LEFTARG = raster, RIGHTARG = raster, PROCEDURE = st_overbelow,
    COMMUTATOR = '|&>',
    RESTRICT = positionsel, JOIN = positionjoinsel
    );

CREATE OPERATOR && (
    LEFTARG = raster, RIGHTARG = raster, PROCEDURE = st_overlap,
    COMMUTATOR = '&&',
    RESTRICT = contsel, JOIN = contjoinsel
    );

CREATE OPERATOR &> (
    LEFTARG = raster, RIGHTARG = raster, PROCEDURE = st_overright,
    COMMUTATOR = '&<',
    RESTRICT = positionsel, JOIN = positionjoinsel
    );

CREATE OPERATOR >> (
    LEFTARG = raster, RIGHTARG = raster, PROCEDURE = st_right,
    COMMUTATOR = '<<',
    RESTRICT = positionsel, JOIN = positionjoinsel
    );

CREATE OPERATOR |&> (
    LEFTARG = raster, RIGHTARG = raster, PROCEDURE = st_overabove,
    COMMUTATOR = '&<|',
    RESTRICT = positionsel, JOIN = positionjoinsel
    );

CREATE OPERATOR |>> (
    LEFTARG = raster, RIGHTARG = raster, PROCEDURE = st_above,
    COMMUTATOR = '<<|',
    RESTRICT = positionsel, JOIN = positionjoinsel
    );

CREATE OPERATOR ~= (
    LEFTARG = raster, RIGHTARG = raster, PROCEDURE = st_same,
    COMMUTATOR = '~=',
    RESTRICT = eqsel, JOIN = eqjoinsel
    );

CREATE OPERATOR @ (
    LEFTARG = raster, RIGHTARG = raster, PROCEDURE = st_contained,
    COMMUTATOR = '~',
    RESTRICT = contsel, JOIN = contjoinsel
    );

CREATE OPERATOR ~ (
    LEFTARG = raster, RIGHTARG = raster, PROCEDURE = st_contain,
    COMMUTATOR = '@',
    RESTRICT = contsel, JOIN = contjoinsel
    );

-----------------------------------------------------------------------
-- Raster/Raster Spatial Relationship
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_samealignment(rastA raster, rastB raster)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'RASTER_sameAlignment'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_samealignment(
	ulx1 double precision, uly1 double precision, scalex1 double precision, scaley1 double precision, skewx1 double precision, skewy1 double precision,
	ulx2 double precision, uly2 double precision, scalex2 double precision, scaley2 double precision, skewx2 double precision, skewy2 double precision
)
	RETURNS boolean
	AS $$ SELECT st_samealignment(st_makeemptyraster(1, 1, $1, $2, $3, $4, $5, $6), st_makeemptyraster(1, 1, $7, $8, $9, $10, $11, $12)) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION _st_intersects(rastA raster, nbandA integer, rastB raster, nbandB integer)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'RASTER_intersects'
	LANGUAGE 'C' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_intersects(rastA raster, nbandA integer, rastB raster, nbandB integer)
	RETURNS boolean
	AS $$ SELECT $1 && $3 AND _st_intersects($1, $2, $3, $4) $$
	LANGUAGE 'SQL' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_intersects(rastA raster, rastB raster)
	RETURNS boolean
	AS $$ SELECT $1 && $2 AND _st_intersects($1, 1, $2, 1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- Raster/Geometry Spatial Relationship
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION _st_intersects(rast raster, geom geometry, nband integer DEFAULT NULL)
	RETURNS boolean
	AS $$
	DECLARE
		gr raster;
		scale double precision;
	BEGIN
		IF ST_Intersects(geom, ST_ConvexHull(rast)) IS NOT TRUE THEN
			RETURN FALSE;
		ELSEIF nband IS NULL THEN
			RETURN TRUE;
		END IF;

		-- scale is set to 1/100th of raster for granularity
		SELECT least(scalex, scaley) / 100. INTO scale FROM ST_Metadata(rast);
		gr := _st_asraster(geom, scale, scale);
		IF gr IS NULL THEN
			RAISE EXCEPTION 'Unable to convert geometry to a raster';
			RETURN FALSE;
		END IF;

		RETURN ST_Intersects(rast, nband, gr, 1);
	END;
	$$ LANGUAGE 'plpgsql' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_intersects(rast raster, geom geometry, nband integer DEFAULT NULL)
	RETURNS boolean
	AS $$ SELECT $1 && $2 AND _st_intersects($1, $2, $3) $$
	LANGUAGE 'SQL' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_intersects(rast raster, nband integer, geom geometry)
	RETURNS boolean
	AS $$ SELECT $1 && $3 AND _st_intersects($1, $3, $2) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- _st_intersects(geom geometry, rast raster, nband integer)
-- If band is provided, check for the presence of withvalue pixels in the area
-- shared by the raster and the geometry. If only nodata value pixels are found, the
-- geometry does not intersect with the raster.
-----------------------------------------------------------------------
-- This function can not be STRICT
CREATE OR REPLACE FUNCTION _st_intersects(geom geometry, rast raster, nband integer DEFAULT NULL)
	RETURNS boolean AS $$
	DECLARE
		hasnodata boolean := TRUE;
		nodata float8 := 0.0;
		convexhull geometry;
		geomintersect geometry;
		x1w double precision := 0.0;
		x2w double precision := 0.0;
		y1w double precision := 0.0;
		y2w double precision := 0.0;
		x1 integer := 0;
		x2 integer := 0;
		x3 integer := 0;
		x4 integer := 0;
		y1 integer := 0;
		y2 integer := 0;
		y3 integer := 0;
		y4 integer := 0;
		x integer := 0;
		y integer := 0;
		xinc integer := 0;
		yinc integer := 0;
		pixelval double precision;
		bintersect boolean := FALSE;
		gtype text;
		scale float8;
		w int;
		h int;
	BEGIN
		convexhull := ST_ConvexHull(rast);
		IF nband IS NOT NULL THEN
			SELECT bmd.hasnodata INTO hasnodata FROM ST_BandMetaData(rast, nband) AS bmd;
		END IF;

		IF ST_Intersects(geom, convexhull) IS NOT TRUE THEN
			RETURN FALSE;
		ELSEIF nband IS NULL OR hasnodata IS FALSE THEN
			RETURN TRUE;
		END IF;

		-- Get the intersection between with the geometry.
		-- We will search for withvalue pixel only in this area.
		geomintersect := st_intersection(geom, convexhull);

--RAISE NOTICE 'geomintersect=%', st_astext(geomintersect);

		-- If the intersection is empty, return false
		IF st_isempty(geomintersect) THEN
			RETURN FALSE;
		END IF;

		-- We create a minimalistic buffer around the intersection in order to scan every pixels
		-- that would touch the edge or intersect with the geometry
		SELECT sqrt(scalex * scalex + skewy * skewy), width, height INTO scale, w, h FROM ST_Metadata(rast);
		IF scale != 0 THEN
			geomintersect := st_buffer(geomintersect, scale / 1000000);
		END IF;

--RAISE NOTICE 'geomintersect2=%', st_astext(geomintersect);

		-- Find the world coordinates of the bounding box of the intersecting area
		x1w := st_xmin(geomintersect);
		y1w := st_ymin(geomintersect);
		x2w := st_xmax(geomintersect);
		y2w := st_ymax(geomintersect);
		nodata := st_bandnodatavalue(rast, nband);

--RAISE NOTICE 'x1w=%, y1w=%, x2w=%, y2w=%', x1w, y1w, x2w, y2w;

		-- Convert world coordinates to raster coordinates
		x1 := st_world2rastercoordx(rast, x1w, y1w);
		y1 := st_world2rastercoordy(rast, x1w, y1w);
		x2 := st_world2rastercoordx(rast, x2w, y1w);
		y2 := st_world2rastercoordy(rast, x2w, y1w);
		x3 := st_world2rastercoordx(rast, x1w, y2w);
		y3 := st_world2rastercoordy(rast, x1w, y2w);
		x4 := st_world2rastercoordx(rast, x2w, y2w);
		y4 := st_world2rastercoordy(rast, x2w, y2w);

--RAISE NOTICE 'x1=%, y1=%, x2=%, y2=%, x3=%, y3=%, x4=%, y4=%', x1, y1, x2, y2, x3, y3, x4, y4;

		-- Order the raster coordinates for the upcoming FOR loop.
		x1 := int4smaller(int4smaller(int4smaller(x1, x2), x3), x4);
		y1 := int4smaller(int4smaller(int4smaller(y1, y2), y3), y4);
		x2 := int4larger(int4larger(int4larger(x1, x2), x3), x4);
		y2 := int4larger(int4larger(int4larger(y1, y2), y3), y4);

		-- Make sure the range is not lower than 1.
		-- This can happen when world coordinate are exactly on the left border
		-- of the raster and that they do not span on more than one pixel.
		x1 := int4smaller(int4larger(x1, 1), w);
		y1 := int4smaller(int4larger(y1, 1), h);

		-- Also make sure the range does not exceed the width and height of the raster.
		-- This can happen when world coordinate are exactly on the lower right border
		-- of the raster.
		x2 := int4smaller(x2, w);
		y2 := int4smaller(y2, h);

--RAISE NOTICE 'x1=%, y1=%, x2=%, y2=%', x1, y1, x2, y2;

		-- Search exhaustively for withvalue pixel on a moving 3x3 grid
		-- (very often more efficient than searching on a mere 1x1 grid)
		FOR xinc in 0..2 LOOP
			FOR yinc in 0..2 LOOP
				FOR x IN x1+xinc..x2 BY 3 LOOP
					FOR y IN y1+yinc..y2 BY 3 LOOP
						-- Check first if the pixel intersects with the geometry. Often many won't.
						bintersect := NOT st_isempty(st_intersection(st_pixelaspolygon(rast, nband, x, y), geom));
						
						IF bintersect THEN
							-- If the pixel really intersects, check its value. Return TRUE if with value.
							pixelval := st_value(rast, nband, x, y);
							IF pixelval != nodata THEN
								RETURN TRUE;
							END IF;
						END IF;
					END LOOP;
				END LOOP;
			END LOOP;
		END LOOP;

		RETURN FALSE;
	END;
	$$ LANGUAGE 'plpgsql' IMMUTABLE;

-- This function can not be STRICT
CREATE OR REPLACE FUNCTION st_intersects(geom geometry, rast raster, nband integer DEFAULT NULL)
	RETURNS boolean AS
	$$ SELECT $1 && $2 AND _st_intersects($1, $2, $3); $$
	LANGUAGE 'SQL' IMMUTABLE;

-----------------------------------------------------------------------
-- _st_intersection(geom geometry, rast raster, band integer)
-- Returns a geometry set that represents the shared portion of the
-- provided geometry and the geometries produced by the vectorization of rast.
-- Return an empty geometry if the geometry does not intersect with the
-- raster.
-- Raster nodata value areas are not vectorized and hence do not intersect
-- with any geometries.
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_intersection(geomin geometry, rast raster, band integer DEFAULT 1)
	RETURNS SETOF geomval AS $$
	DECLARE
		intersects boolean := FALSE;
	BEGIN
		intersects := ST_Intersects(geomin, rast, band);
		IF intersects THEN
			-- Return the intersections of the geometry with the vectorized parts of
			-- the raster and the values associated with those parts, if really their
			-- intersection is not empty.
			RETURN QUERY
				SELECT
					intgeom,
					val
				FROM (
					SELECT
						ST_Intersection((gv).geom, geomin) AS intgeom,
						(gv).val
					FROM ST_DumpAsPolygons(rast, band) gv
					WHERE ST_Intersects((gv).geom, geomin)
				) foo
				WHERE NOT ST_IsEmpty(intgeom);
		ELSE
			-- If the geometry does not intersect with the raster, return an empty
			-- geometry and a null value
			RETURN QUERY
				SELECT
					emptygeom,
					NULL::float8
				FROM ST_GeomCollFromText('GEOMETRYCOLLECTION EMPTY', ST_SRID($1)) emptygeom;
		END IF;
	END;
	$$
	LANGUAGE 'plpgsql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_intersection(rast raster, geom geometry)
	RETURNS SETOF geomval
	AS $$ SELECT (gv).geom, (gv).val FROM st_intersection($2, $1, 1) gv; $$
	LANGUAGE SQL IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_intersection(rast raster, band integer, geom geometry)
	RETURNS SETOF geomval
	AS $$ SELECT (gv).geom, (gv).val FROM st_intersection($3, $1, $2) gv; $$
	LANGUAGE SQL IMMUTABLE STRICT;

------------------------------------------------------------------------------
-- RASTER_COLUMNS
--
-- The metadata is documented in the PostGIS Raster specification:
-- http://trac.osgeo.org/postgis/wiki/WKTRaster/SpecificationFinal01
------------------------------------------------------------------------------
CREATE TABLE raster_columns (
	r_table_catalog varchar(256) not null,
	r_table_schema varchar(256) not null,
	r_table_name varchar(256) not null,
	r_column varchar(256) not null,
	srid integer not null,
	pixel_types varchar[] not null,
	out_db boolean not null,
	regular_blocking boolean not null,
	nodata_values double precision[],
	scale_x double precision,
	scale_y double precision,
	blocksize_x integer,
	blocksize_y integer,
	extent geometry,

	CONSTRAINT raster_columns_pk PRIMARY KEY (
		r_table_catalog,
		r_table_schema,
		r_table_name,
		r_column
	)
)
	WITHOUT OIDS;

------------------------------------------------------------------------------
-- RASTER_OVERVIEWS
------------------------------------------------------------------------------
CREATE TABLE raster_overviews (
	o_table_catalog character varying(256) NOT NULL,
	o_table_schema character varying(256) NOT NULL,
	o_table_name character varying(256) NOT NULL,
	o_column character varying(256) NOT NULL,
	r_table_catalog character varying(256) NOT NULL,
	r_table_schema character varying(256) NOT NULL,
	r_table_name character varying(256) NOT NULL,
	r_column character varying(256) NOT NULL,
	out_db boolean NOT NULL,
	overview_factor integer NOT NULL,

	CONSTRAINT raster_overviews_pk PRIMARY KEY (
		o_table_catalog,
		o_table_schema,
		o_table_name,
		o_column, overview_factor
	)
)
	WITHOUT OIDS;

------------------------------------------------------------------------------
-- AddRasterColumn
-------------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION AddRasterColumn(p_catalog_name varchar,
                                           p_schema_name varchar,
                                           p_table_name varchar,
                                           p_column_name varchar,
                                           p_srid integer,
                                           p_pixel_types varchar[],
                                           p_out_db boolean,
                                           p_regular_blocking boolean,
                                           p_nodata_values double precision[],
                                           p_scale_x double precision,
                                           p_scale_y double precision,
                                           p_blocksize_x integer,
                                           p_blocksize_y integer,
                                           p_extent geometry)

    RETURNS text AS
    $$
    DECLARE
        sql text;
        real_schema name;
        srid_into varchar;
        geometry_op_into varchar;
        pixel_types varchar[];
        pixel_types_size integer := 0;
        pixel_types_found integer := 0;
        nodata_values_size integer := 0;

    BEGIN

		/*
        RAISE DEBUG 'Parameters: catalog=%, schema=%, table=%, column=%, srid=%, pixel_types=%, out_db=%, regular_blocking=%, nodata_values=%, scale_x=%, scale_y=%, blocksize_x=%, blocksize_y=%',
                     p_catalog_name, p_schema_name, p_table_name, p_column_name, p_srid, p_pixel_types, p_out_db, p_regular_blocking, p_nodata_values, p_scale_x, p_scale_y, p_blocksize_x, p_blocksize_y;
		*/

        -- Validate required parametersa and combinations
        IF ( (p_catalog_name IS NULL) OR (p_schema_name IS NULL)
              OR (p_table_name IS NULL) OR (p_column_name IS NULL) ) THEN
            RAISE EXCEPTION 'Name of catalog, schema, table or column IS NULL, value expected';
            RETURN 'fail';
        END IF;

        IF ( p_srid IS NULL ) THEN
            RAISE EXCEPTION 'SRID IS NULL, value expected';
            RETURN 'fail';
        END IF;

        IF ( p_pixel_types IS NULL ) THEN
            RAISE EXCEPTION 'Array of pixel types IS NULL, value expected';
            RETURN 'fail';
        END IF;

        IF ( p_out_db IS NULL ) THEN
            RAISE EXCEPTION 'out_db IS NULL, value expected';
            RETURN 'fail';
        END IF;

        IF ( p_regular_blocking IS NULL ) THEN
            RAISE EXCEPTION 'regular_blocking IS NULL, value expected';
            RETURN 'fail';
        END IF;

        IF ( p_regular_blocking = true ) THEN
           IF ( p_blocksize_x IS NULL or p_blocksize_y IS NULL ) THEN
               RAISE EXCEPTION 'blocksize_x/blocksize_y IS NULL, value expected if regular_blocking is TRUE';
               RETURN 'fail';
           END IF;
        ELSE
           IF ( p_blocksize_x IS NOT NULL or p_blocksize_y IS NOT NULL ) THEN
               RAISE EXCEPTION 'blocksize_x/blocksize_y values given, but regular_blocking is FALSE';
               RETURN 'fail';
           END IF;
        END IF;


        -- Verify SRID
        IF ( (p_srid != 0) AND (p_srid != -1) ) THEN
            SELECT SRID INTO srid_into FROM spatial_ref_sys WHERE SRID = p_srid;
            IF NOT FOUND THEN
                RAISE EXCEPTION 'Invalid SRID';
                RETURN 'fail';
            END IF;
            --RAISE DEBUG 'Verified SRID = %', p_srid;
        END IF;


        -- Verify PIXEL TYPE
        -- TODO: If only PostgreSQL 8.2+ supported, use @> operator instead of brute-force lookup
        --       SELECT p_pixel_types <@ pixel_types INTO pixel_types_found_into; -- boolean
        pixel_types := ARRAY['1BB','2BUI','4BUI','8BSI','8BUI','16BSI','16BUI','32BSI','32BUI','16BF','32BF','64BF'];

        FOR npti IN array_lower(p_pixel_types, 1) .. array_upper(p_pixel_types, 1) LOOP

            pixel_types_found := 0;
            FOR pti IN array_lower(pixel_types, 1) .. array_upper(pixel_types, 1) LOOP
                IF p_pixel_types[npti] = pixel_types[pti] THEN
                    pixel_types_found := 1;
                    --RAISE DEBUG 'Identified pixel type %', p_pixel_types[npti];
                END IF;
            END LOOP;

            IF pixel_types_found = 0 THEN
                RAISE EXCEPTION 'Invalid pixel type % - valid ones are %', p_pixel_types[npti], pixel_types;
                RETURN 'fail';
            END IF;

            pixel_types_size := pixel_types_size + 1;
        END LOOP;

        -- Verify nodata
        -- TODO: Validate if nodata values matche range of corresponding pixel types
        nodata_values_size := 1 + array_upper(p_nodata_values, 1) - array_lower(p_nodata_values, 1);
        IF ( pixel_types_size != nodata_values_size ) THEN
            RAISE EXCEPTION 'Number of pixel types (%) and nodata values (%) do not match',
                            pixel_types_size, nodata_values_size;
            RETURN 'fail';
        END IF;


        -- Verify extent geometry
        IF ( p_extent IS NOT NULL ) THEN

            -- Verify POLYGON type
            SELECT GeometryType(p_extent) INTO geometry_op_into;
            IF ( NOT ( geometry_op_into = 'POLYGON' ) ) THEN
                RAISE EXCEPTION 'extent is of invalid type (%), expected simple and non-rotated POLYGON', geometry_op_into;
                RETURN 'fail';
            END IF;

            -- Verify SRID
            SELECT ST_SRID(p_extent) INTO srid_into;
            IF ( p_srid != srid_into::integer ) THEN
                RAISE EXCEPTION 'SRID values for raster (%) and extent (%) do not match', p_srid, srid_into;
                RETURN 'fail';
            END IF;
        END IF;


        -- Verify regular_blocking
        IF ( p_regular_blocking = true ) THEN

            IF ( p_blocksize_x IS NULL or p_blocksize_y IS NULL ) THEN
                RAISE EXCEPTION 'unexpected NULL for blocksize_x or blocksize_y';
                RETURN 'fail';
            END IF;

            -- Verify extent is non-rotated rectangle
            IF ( p_extent IS NOT NULL ) THEN
                -- TODO: Replace with bounding box overlapping test (&&)
                SELECT ST_Equals(p_extent, ST_Envelope(p_extent)) INTO geometry_op_into;
                IF ( NOT ( geometry_op_into = 't' ) ) THEN
                    RAISE EXCEPTION 'extent does not represent non-rotated rectangle';
                    RETURN 'fail';
                END IF;
            END IF;

            -- TODO: Set number of constraints on target table:
            --       - all tiles have the same size (blocksize_x and blocksize_y)
            --       - all tiles do not overlap
            --       - all tiles appear on the regular block grid
            --       - top left block start at the top left corner of the extent
        END IF;



        -- Verify SCHEMA
        IF ( p_schema_name IS NOT NULL AND p_schema_name != '' ) THEN
            sql := 'SELECT nspname FROM pg_namespace '
                || 'WHERE text(nspname) = ' || quote_literal(p_schema_name)
                || 'LIMIT 1';
            --RAISE DEBUG '%', sql;
            EXECUTE sql INTO real_schema;

            IF ( real_schema IS NULL ) THEN
                RAISE EXCEPTION 'Schema % is not a valid schemaname', quote_literal(p_schema_name);
                RETURN 'fail';
            END IF;
        END IF;

        IF ( real_schema IS NULL ) THEN
            --RAISE DEBUG 'Detecting schema';
            sql := 'SELECT n.nspname AS schemaname '
                || 'FROM pg_catalog.pg_class c '
                || 'JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace '
                || 'WHERE c.relkind = ' || quote_literal('r')
                || ' AND n.nspname NOT IN ('
                || quote_literal('pg_catalog') || ', ' || quote_literal('pg_toast') || ')'
                || ' AND pg_catalog.pg_table_is_visible(c.oid)'
                || ' AND c.relname = ' || quote_literal(p_table_name);
            --RAISE DEBUG '%', sql;
            EXECUTE sql INTO real_schema;

            IF ( real_schema IS NULL ) THEN
                RAISE EXCEPTION 'Table % does not occur in the search_path',
                                quote_literal(p_table_name);
                RETURN 'fail';
            END IF;
        END IF;


        -- Add raster column to target table
        sql := 'ALTER TABLE '
            || quote_ident(real_schema) || '.' || quote_ident(p_table_name)
            || ' ADD COLUMN ' || quote_ident(p_column_name) ||  ' raster ';
        --RAISE DEBUG '%', sql;
        EXECUTE sql;


        -- Delete stale record in RASTER_COLUMNS (if any)
        sql := 'DELETE FROM raster_columns '
            || ' WHERE r_table_catalog = ' || quote_literal('')
            || ' AND r_table_schema = ' || quote_literal(real_schema)
            || ' AND r_table_name = ' || quote_literal(p_table_name)
            || ' AND r_column = ' || quote_literal(p_column_name);
        --RAISE DEBUG '%', sql;
        EXECUTE sql;


        -- Add record in RASTER_COLUMNS
        sql := 'INSERT INTO raster_columns '
            || '(r_table_catalog, r_table_schema, r_table_name, r_column, srid, '
            || 'pixel_types, out_db, regular_blocking, nodata_values, '
            || 'scale_x, scale_y, blocksize_x, blocksize_y, extent) '
            || 'VALUES ('
            || quote_literal('') || ','
            || quote_literal(real_schema) || ','
            || quote_literal(p_table_name) || ','
            || quote_literal(p_column_name) || ','
            || p_srid::text || ','
            || quote_literal(p_pixel_types::text) || ','
            || p_out_db::text || ','
            || p_regular_blocking::text || ','
            || COALESCE(quote_literal(p_nodata_values::text), 'NULL') || ','
            || COALESCE(quote_literal(p_scale_x), 'NULL') || ','
            || COALESCE(quote_literal(p_scale_y), 'NULL') || ','
            || COALESCE(quote_literal(p_blocksize_x), 'NULL') || ','
            || COALESCE(quote_literal(p_blocksize_y), 'NULL') || ','
            || COALESCE(quote_literal(p_extent::text), 'NULL') || ')';
        --RAISE DEBUG '%', sql;
        EXECUTE sql;


        -- Add CHECK for SRID
        sql := 'ALTER TABLE '
            || quote_ident(real_schema) || '.' || quote_ident(p_table_name)
            || ' ADD CONSTRAINT '
            || quote_ident('enforce_srid_' || p_column_name)
            || ' CHECK (ST_SRID(' || quote_ident(p_column_name)
            || ') = ' || p_srid::text || ')';
        --RAISE DEBUG '%', sql;
        EXECUTE sql;


        -- TODO: Add more CHECKs
        -- - Add CHECK for pixel types
        -- - Add CHECK for scale
        -- - Do we need CHECK for nodata values?


        RETURN p_schema_name || '.' || p_table_name || '.' || p_column_name
            || ' srid:' || p_srid::text
            || ' pixel_types:' || p_pixel_types::text
            || ' out_db:' || p_out_db::text
            || ' regular_blocking:' || p_regular_blocking::text
            || ' nodata_values:' || COALESCE(quote_literal(p_nodata_values::text), 'NULL')
            || ' scale_x:' || COALESCE(quote_literal(p_scale_x), 'NULL')
            || ' scale_y:' || COALESCE(quote_literal(p_scale_y), 'NULL')
            || ' blocksize_x:' || COALESCE(quote_literal(p_blocksize_x), 'NULL')
            || ' blocksize_y:' || COALESCE(quote_literal(p_blocksize_y), 'NULL')
            || ' extent:' || COALESCE(ST_AsText(p_extent), 'NULL');

    END;
    $$
    LANGUAGE 'plpgsql' VOLATILE;


------------------------------------------------------------------------------
-- AddRasterColumn (with default catalog)
-------------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION AddRasterColumn(schema varchar,
                                           p_table varchar,
                                           p_column varchar,
                                           p_srid integer,
                                           p_pixel_types varchar[],
                                           p_out_db boolean,
                                           p_regular_blocking boolean,
                                           p_nodata_values double precision[],
                                           p_scale_x double precision,
                                           p_scale_y double precision,
                                           p_blocksize_x integer,
                                           p_blocksize_y integer,
                                           p_extent geometry)
    RETURNS text AS
    $$
    DECLARE
        ret text;
    BEGIN
        SELECT AddRasterColumn('',$1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13) INTO ret;
        RETURN ret;
    END;
    $$
    LANGUAGE 'plpgsql' STABLE;


------------------------------------------------------------------------------
-- AddRasterColumn (with default catalog and schema)
-------------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION AddRasterColumn(p_table varchar,
                                           p_column varchar,
                                           p_srid integer,
                                           p_pixel_types varchar[],
                                           p_out_db boolean,
                                           p_regular_blocking boolean,
                                           p_nodata_values double precision[],
                                           p_scale_x double precision,
                                           p_scale_y double precision,
                                           p_blocksize_x integer,
                                           p_blocksize_y integer,
                                           p_extent geometry)
    RETURNS text AS
    $$
    DECLARE
        ret text;
    BEGIN
        SELECT AddRasterColumn('','',$1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12) INTO ret;
        RETURN ret;
    END;
    $$
    LANGUAGE 'plpgsql' STABLE;


-------------------------------------------------------------------------------
-- DropRasterColumn
-------------------------------------------------------------------------------
-- FIXME: Use 'name' type for table,column and other names
-------------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION DropRasterColumn(catalog_name varchar,
                                            schema_name varchar,
                                            table_name varchar,
                                            column_name varchar)
    RETURNS text AS
    $$
    DECLARE
        myrec record;
        real_schema name;
        okay boolean;
    BEGIN
        -- Find, check or fix schema_name
        IF ( schema_name != '' ) THEN
            okay = 'f';

            FOR myrec IN SELECT nspname FROM pg_namespace WHERE text(nspname) = schema_name LOOP
                okay := 't';
            END LOOP;

            IF ( okay <> 't' ) THEN
                RAISE NOTICE 'invalid schema name - using current_schema()';
                SELECT current_schema() INTO real_schema;
            ELSE
                real_schema = schema_name;
            END IF;
        ELSE
            SELECT current_schema() INTO real_schema;
        END IF;

        -- Find out if the column is in the raster_columns table
        okay = 'f';
        FOR myrec IN SELECT * FROM raster_columns WHERE r_table_schema = text(real_schema)
                     AND r_table_name = table_name AND r_column = column_name LOOP
            okay := 't';
        END LOOP;
        IF (okay <> 't') THEN
            RAISE EXCEPTION 'column % not found in raster_columns table', column_name;
            RETURN 'f';
        END IF;

        -- Remove ref from raster_columns table
        EXECUTE 'DELETE FROM raster_columns WHERE r_table_schema = '
                || quote_literal(real_schema) || ' AND r_table_name = '
                || quote_literal(table_name)  || ' AND r_column = '
                || quote_literal(column_name);

        -- Remove table column
        EXECUTE 'ALTER TABLE ' || quote_ident(real_schema) || '.'
                || quote_ident(table_name) || ' DROP COLUMN '
                || quote_ident(column_name);

        RETURN schema_name || '.' || table_name || '.' || column_name ||' effectively removed.';
    END;
    $$
    LANGUAGE 'plpgsql' VOLATILE STRICT; -- WITH (isstrict);


-----------------------------------------------------------------------
-- DropRasterColumn (with default catalog name)
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION DropRasterColumn(schema_name varchar,
                                            table_name varchar,
                                            column_name varchar)
    RETURNS text AS
    $$
    DECLARE
        ret text;
    BEGIN
        SELECT DropRasterColumn('', schema_name, table_name, column_name) INTO ret;
        RETURN ret;
    END;
    $$
    LANGUAGE 'plpgsql' VOLATILE STRICT; -- WITH (isstrict);

-----------------------------------------------------------------------
-- DropRasterColumn (with default catalog and schema name)
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION DropRasterColumn(table_name varchar,
                                            column_name varchar)
    RETURNS text AS
    $$
    DECLARE
        ret text;
    BEGIN
        SELECT DropRasterColumn('', '', table_name, column_name) INTO ret;
        RETURN ret;
    END;
    $$
    LANGUAGE 'plpgsql' VOLATILE STRICT; -- WITH (isstrict);

-----------------------------------------------------------------------
-- DropRasterTable
-- Drop a table and all its references in raster_columns
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION DropRasterTable(catalog_name varchar,
                                           schema_name varchar,
                                           table_name varchar)
    RETURNS text AS
    $$
    DECLARE
        real_schema name;
    BEGIN
        IF ( schema_name = '' ) THEN
            SELECT current_schema() into real_schema;
        ELSE
            real_schema = schema_name;
        END IF;

        -- Remove refs from raster_columns table
        EXECUTE 'DELETE FROM raster_columns WHERE '
                || 'r_table_schema = ' || quote_literal(real_schema)
                || ' AND ' || ' r_table_name = ' || quote_literal(table_name);

        -- Remove table
        EXECUTE 'DROP TABLE ' || quote_ident(real_schema) || '.'
                || quote_ident(table_name);

        RETURN real_schema || '.' || table_name || ' effectively dropped.';
    END;
    $$
    LANGUAGE 'plpgsql' VOLATILE STRICT; -- WITH (isstrict);

-----------------------------------------------------------------------
-- DropRasterTable (with default catalog name)
-- Drop a table and all its references in raster_columns
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION DropRasterTable(schema_name varchar,
                                           table_name varchar)
    RETURNS text AS
    $$
    DECLARE
        ret text;
    BEGIN
        SELECT DropRasterTable('', schema_name, table_name) INTO ret;
        RETURN ret;
    END;
    $$
    LANGUAGE 'plpgsql' VOLATILE STRICT; -- WITH (isstrict);

-----------------------------------------------------------------------
-- DropRasterTable (with default catalog and schema name)
-- Drop a table and all its references in raster_columns
-- For PG>=73 use current_schema()
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION DropRasterTable(table_name varchar)
    RETURNS text AS
    $$
    DECLARE
        ret text;
    BEGIN
        SELECT DropRasterTable('', '', table_name) INTO ret;
        RETURN ret;
    END;
    $$
    LANGUAGE 'plpgsql' VOLATILE STRICT; -- WITH (isstrict);

-------------------------------------------------------------------
--  END
-------------------------------------------------------------------

-- COMMIT;
