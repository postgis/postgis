-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--
--
-- PostGIS Raster - Raster Type for PostGIS
-- http://trac.osgeo.org/postgis/wiki/WKTRaster
--
-- Copyright (c) 2009-2012 Sandro Santilli <strk@keybit.net>
-- Copyright (c) 2009-2010 Pierre Racine <pierre.racine@sbf.ulaval.ca>
-- Copyright (c) 2009-2010 Jorge Arevalo <jorge.arevalo@deimos-space.com>
-- Copyright (c) 2009-2010 Mateusz Loskot <mateusz@loskot.net>
-- Copyright (c) 2010 David Zwarg <dzwarg@azavea.com>
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

#include "../../postgis/sqldefines.h"

SET client_min_messages TO warning;

BEGIN;

------------------------------------------------------------------------------
-- RASTER Type
------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION raster_in(cstring)
    RETURNS raster
    AS 'MODULE_PATHNAME','RASTER_in'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION raster_out(raster)
    RETURNS cstring
    AS 'MODULE_PATHNAME','RASTER_out'
    LANGUAGE 'c' IMMUTABLE STRICT;

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
    LANGUAGE 'c' IMMUTABLE; -- a new lib will require a new session

CREATE OR REPLACE FUNCTION postgis_raster_scripts_installed() RETURNS text
       AS _POSTGIS_SQL_SELECT_POSTGIS_SCRIPTS_VERSION
       LANGUAGE 'sql' IMMUTABLE;

CREATE OR REPLACE FUNCTION postgis_raster_lib_build_date()
    RETURNS text
    AS 'MODULE_PATHNAME', 'RASTER_lib_build_date'
    LANGUAGE 'c' IMMUTABLE; -- a new lib will require a new session

CREATE OR REPLACE FUNCTION postgis_gdal_version()
    RETURNS text
    AS 'MODULE_PATHNAME', 'RASTER_gdal_version'
    LANGUAGE 'c' IMMUTABLE;

-----------------------------------------------------------------------
-- Raster Accessors
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION st_convexhull(raster)
    RETURNS geometry
    AS 'MODULE_PATHNAME','RASTER_convex_hull'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION box3d(raster)
    RETURNS box3d
    AS 'select box3d(st_convexhull($1))'
    LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_envelope(raster)
    RETURNS geometry
    AS 'select st_envelope(st_convexhull($1))'
    LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_height(raster)
    RETURNS integer
    AS 'MODULE_PATHNAME','RASTER_getHeight'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_numbands(raster)
    RETURNS integer
    AS 'MODULE_PATHNAME','RASTER_getNumBands'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_scalex(raster)
    RETURNS float8
    AS 'MODULE_PATHNAME','RASTER_getXScale'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_scaley(raster)
    RETURNS float8
    AS 'MODULE_PATHNAME','RASTER_getYScale'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_skewx(raster)
    RETURNS float8
    AS 'MODULE_PATHNAME','RASTER_getXSkew'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_skewy(raster)
    RETURNS float8
    AS 'MODULE_PATHNAME','RASTER_getYSkew'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_srid(raster)
    RETURNS integer
    AS 'MODULE_PATHNAME','RASTER_getSRID'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_upperleftx(raster)
    RETURNS float8
    AS 'MODULE_PATHNAME','RASTER_getXUpperLeft'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_upperlefty(raster)
    RETURNS float8
    AS 'MODULE_PATHNAME','RASTER_getYUpperLeft'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_width(raster)
    RETURNS integer
    AS 'MODULE_PATHNAME','RASTER_getWidth'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_pixelwidth(raster)
    RETURNS float8
    AS 'MODULE_PATHNAME', 'RASTER_getPixelWidth'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_pixelheight(raster)
    RETURNS float8
    AS 'MODULE_PATHNAME', 'RASTER_getPixelHeight'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_geotransform(raster,
    OUT imag double precision,
    OUT jmag double precision,
    OUT theta_i double precision,
    OUT theta_ij double precision,
    OUT xoffset double precision,
    OUT yoffset double precision)
    AS 'MODULE_PATHNAME', 'RASTER_getGeotransform'
    LANGUAGE 'c' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_rotation(raster)
    RETURNS float8
    AS $$ SELECT (ST_Geotransform($1)).theta_i $$
    LANGUAGE 'sql' VOLATILE;

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
	LANGUAGE 'c' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- Constructor ST_MakeEmptyRaster
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_makeemptyraster(width int, height int, upperleftx float8, upperlefty float8, scalex float8, scaley float8, skewx float8, skewy float8, srid int4 DEFAULT 0)
    RETURNS RASTER
    AS 'MODULE_PATHNAME', 'RASTER_makeEmpty'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_makeemptyraster(width int, height int, upperleftx float8, upperlefty float8, pixelsize float8)
    RETURNS raster
    AS $$ SELECT st_makeemptyraster($1, $2, $3, $4, $5, -($5), 0, 0, ST_SRID('POINT(0 0)'::geometry)) $$
    LANGUAGE 'sql' IMMUTABLE STRICT;

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

-----------------------------------------------------------------------
-- Constructor ST_AddBand
-----------------------------------------------------------------------

CREATE TYPE addbandarg AS (
	index int,
	pixeltype text,
	initialvalue float8,
	nodataval float8
);

CREATE OR REPLACE FUNCTION st_addband(rast raster, addbandargset addbandarg[])
	RETURNS RASTER
	AS 'MODULE_PATHNAME', 'RASTER_addBand'
	LANGUAGE 'c' IMMUTABLE STRICT;

-- This function can not be STRICT, because nodataval can be NULL indicating that no nodata value should be set
CREATE OR REPLACE FUNCTION st_addband(
	rast raster,
	index int,
	pixeltype text,
	initialvalue float8 DEFAULT 0.,
	nodataval float8 DEFAULT NULL
)
	RETURNS raster
	AS $$ SELECT st_addband($1, ARRAY[ROW($2, $3, $4, $5)]::addbandarg[]) $$
	LANGUAGE 'sql' IMMUTABLE;

-- This function can not be STRICT, because nodataval can be NULL indicating that no nodata value should be set
CREATE OR REPLACE FUNCTION st_addband(
	rast raster,
	pixeltype text,
	initialvalue float8 DEFAULT 0.,
	nodataval float8 DEFAULT NULL
)
	RETURNS raster
	AS $$ SELECT st_addband($1, ARRAY[ROW(NULL, $2, $3, $4)]::addbandarg[]) $$
	LANGUAGE 'sql' IMMUTABLE;

-- This function can not be STRICT, because torastindex can not be determined (could be st_numbands(raster) though)
CREATE OR REPLACE FUNCTION st_addband(
	torast raster,
	fromrast raster,
	fromband int DEFAULT 1,
	torastindex int DEFAULT NULL
)
	RETURNS raster
	AS 'MODULE_PATHNAME', 'RASTER_copyBand'
	LANGUAGE 'c' IMMUTABLE; 

CREATE OR REPLACE FUNCTION st_addband(
	torast raster,
	fromrasts raster[], fromband integer DEFAULT 1,
	torastindex int DEFAULT NULL
)
	RETURNS raster
	AS 'MODULE_PATHNAME', 'RASTER_addBandRasterArray'
	LANGUAGE 'c' IMMUTABLE;

-----------------------------------------------------------------------
-- Constructor ST_Band
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_band(rast raster, nbands int[] DEFAULT ARRAY[1])
	RETURNS RASTER
	AS 'MODULE_PATHNAME', 'RASTER_band'
	LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_band(rast raster, nband int)
	RETURNS RASTER
	AS $$ SELECT st_band($1, ARRAY[$2]) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_band(rast raster, nbands text, delimiter char DEFAULT ',')
	RETURNS RASTER
	AS $$ SELECT st_band($1, regexp_split_to_array(regexp_replace($2, '[[:space:]]', '', 'g'), $3)::int[]) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- ST_SummaryStats and ST_ApproxSummaryStats
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION _st_summarystats(
	rast raster,
	nband int DEFAULT 1,
	exclude_nodata_value boolean DEFAULT TRUE,
	sample_percent double precision DEFAULT 1,
	OUT count bigint,
	OUT sum double precision,
	OUT mean double precision,
	OUT stddev double precision,
	OUT min double precision,
	OUT max double precision
)
	AS 'MODULE_PATHNAME','RASTER_summaryStats'
	LANGUAGE 'c' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_summarystats(
	rast raster,
	nband int DEFAULT 1,
	exclude_nodata_value boolean DEFAULT TRUE,
	OUT count bigint,
	OUT sum double precision,
	OUT mean double precision,
	OUT stddev double precision,
	OUT min double precision,
	OUT max double precision
)
	AS $$ SELECT _st_summarystats($1, $2, $3, 1) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_summarystats(
	rast raster,
	exclude_nodata_value boolean,
	OUT count bigint,
	OUT sum double precision,
	OUT mean double precision,
	OUT stddev double precision,
	OUT min double precision,
	OUT max double precision
)
	AS $$ SELECT _st_summarystats($1, 1, $2, 1) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxsummarystats(
	rast raster,
	nband int DEFAULT 1,
	exclude_nodata_value boolean DEFAULT TRUE,
	sample_percent double precision DEFAULT 0.1,
	OUT count bigint,
	OUT sum double precision,
	OUT mean double precision,
	OUT stddev double precision,
	OUT min double precision,
	OUT max double precision
)
	AS $$ SELECT _st_summarystats($1, $2, $3, $4) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxsummarystats(
	rast raster,
	nband int,
	sample_percent double precision,
	OUT count bigint,
	OUT sum double precision,
	OUT mean double precision,
	OUT stddev double precision,
	OUT min double precision,
	OUT max double precision
)
	AS $$ SELECT _st_summarystats($1, $2, TRUE, $3) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxsummarystats(
	rast raster,
	exclude_nodata_value boolean,
	sample_percent double precision DEFAULT 0.1,
	OUT count bigint,
	OUT sum double precision,
	OUT mean double precision,
	OUT stddev double precision,
	OUT min double precision,
	OUT max double precision
)
	AS $$ SELECT _st_summarystats($1, 1, $2, $3) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxsummarystats(
	rast raster,
	sample_percent double precision,
	OUT count bigint,
	OUT sum double precision,
	OUT mean double precision,
	OUT stddev double precision,
	OUT min double precision,
	OUT max double precision
)
	AS $$ SELECT _st_summarystats($1, 1, TRUE, $2) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION _st_summarystats(
	rastertable text,
	rastercolumn text,
	nband integer DEFAULT 1,
	exclude_nodata_value boolean DEFAULT TRUE,
	sample_percent double precision DEFAULT 1,
	OUT count bigint,
	OUT sum double precision,
	OUT mean double precision,
	OUT stddev double precision,
	OUT min double precision,
	OUT max double precision
)
	AS 'MODULE_PATHNAME','RASTER_summaryStatsCoverage'
	LANGUAGE 'c' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_summarystats(
	rastertable text,
	rastercolumn text,
	nband integer DEFAULT 1,
	exclude_nodata_value boolean DEFAULT TRUE,
	OUT count bigint,
	OUT sum double precision,
	OUT mean double precision,
	OUT stddev double precision,
	OUT min double precision,
	OUT max double precision
)
	AS $$ SELECT _st_summarystats($1, $2, $3, $4, 1) $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_summarystats(
	rastertable text,
	rastercolumn text,
	exclude_nodata_value boolean,
	OUT count bigint,
	OUT sum double precision,
	OUT mean double precision,
	OUT stddev double precision,
	OUT min double precision,
	OUT max double precision
)
	AS $$ SELECT _st_summarystats($1, $2, 1, $3, 1) $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxsummarystats(
	rastertable text,
	rastercolumn text,
	nband integer DEFAULT 1,
	exclude_nodata_value boolean DEFAULT TRUE,
	sample_percent double precision DEFAULT 0.1,
	OUT count bigint,
	OUT sum double precision,
	OUT mean double precision,
	OUT stddev double precision,
	OUT min double precision,
	OUT max double precision
)
	AS $$ SELECT _st_summarystats($1, $2, $3, $4, $5) $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxsummarystats(
	rastertable text,
	rastercolumn text,
	nband integer,
	sample_percent double precision,
	OUT count bigint,
	OUT sum double precision,
	OUT mean double precision,
	OUT stddev double precision,
	OUT min double precision,
	OUT max double precision
)
	AS $$ SELECT _st_summarystats($1, $2, $3, TRUE, $4) $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxsummarystats(
	rastertable text,
	rastercolumn text,
	exclude_nodata_value boolean,
	OUT count bigint,
	OUT sum double precision,
	OUT mean double precision,
	OUT stddev double precision,
	OUT min double precision,
	OUT max double precision
)
	AS $$ SELECT _st_summarystats($1, $2, 1, $3, 0.1) $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxsummarystats(
	rastertable text,
	rastercolumn text,
	sample_percent double precision,
	OUT count bigint,
	OUT sum double precision,
	OUT mean double precision,
	OUT stddev double precision,
	OUT min double precision,
	OUT max double precision
)
	AS $$ SELECT _st_summarystats($1, $2, 1, TRUE, $3) $$
	LANGUAGE 'sql' STABLE STRICT;

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
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_count(rast raster, exclude_nodata_value boolean)
	RETURNS bigint
	AS $$ SELECT _st_count($1, 1, $2, 1) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxcount(rast raster, nband int DEFAULT 1, exclude_nodata_value boolean DEFAULT TRUE, sample_percent double precision DEFAULT 0.1)
	RETURNS bigint
	AS $$ SELECT _st_count($1, $2, $3, $4) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxcount(rast raster, nband int, sample_percent double precision)
	RETURNS bigint
	AS $$ SELECT _st_count($1, $2, TRUE, $3) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxcount(rast raster, exclude_nodata_value boolean, sample_percent double precision DEFAULT 0.1)
	RETURNS bigint
	AS $$ SELECT _st_count($1, 1, $2, $3) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxcount(rast raster, sample_percent double precision)
	RETURNS bigint
	AS $$ SELECT _st_count($1, 1, TRUE, $2) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION _st_count(rastertable text, rastercolumn text, nband integer DEFAULT 1, exclude_nodata_value boolean DEFAULT TRUE, sample_percent double precision DEFAULT 1)
	RETURNS bigint
	AS $$
	DECLARE
		curs refcursor;

		ctable text;
		ccolumn text;
		rast raster;

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
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_count(rastertable text, rastercolumn text, exclude_nodata_value boolean)
	RETURNS bigint
	AS $$ SELECT _st_count($1, $2, 1, $3, 1) $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxcount(rastertable text, rastercolumn text, nband int DEFAULT 1, exclude_nodata_value boolean DEFAULT TRUE, sample_percent double precision DEFAULT 0.1)
	RETURNS bigint
	AS $$ SELECT _st_count($1, $2, $3, $4, $5) $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxcount(rastertable text, rastercolumn text, nband int, sample_percent double precision)
	RETURNS bigint
	AS $$ SELECT _st_count($1, $2, $3, TRUE, $4) $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxcount(rastertable text, rastercolumn text, exclude_nodata_value boolean, sample_percent double precision DEFAULT 0.1)
	RETURNS bigint
	AS $$ SELECT _st_count($1, $2, 1, $3, $4) $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxcount(rastertable text, rastercolumn text, sample_percent double precision)
	RETURNS bigint
	AS $$ SELECT _st_count($1, $2, 1, TRUE, $3) $$
	LANGUAGE 'sql' STABLE STRICT;

-----------------------------------------------------------------------
-- ST_Histogram and ST_ApproxHistogram
-----------------------------------------------------------------------
-- Cannot be strict as "width", "min" and "max" can be NULL
CREATE OR REPLACE FUNCTION _st_histogram(
	rast raster, nband int DEFAULT 1,
	exclude_nodata_value boolean DEFAULT TRUE,
	sample_percent double precision DEFAULT 1,
	bins int DEFAULT 0, width double precision[] DEFAULT NULL,
	right boolean DEFAULT FALSE,
	min double precision DEFAULT NULL, max double precision DEFAULT NULL,
	OUT min double precision,
	OUT max double precision,
	OUT count bigint,
	OUT percent double precision
)
	RETURNS SETOF record
	AS 'MODULE_PATHNAME','RASTER_histogram'
	LANGUAGE 'c' IMMUTABLE;

-- Cannot be strict as "width" can be NULL
CREATE OR REPLACE FUNCTION st_histogram(
	rast raster, nband int DEFAULT 1,
	exclude_nodata_value boolean DEFAULT TRUE,
	bins int DEFAULT 0, width double precision[] DEFAULT NULL,
	right boolean DEFAULT FALSE,
	OUT min double precision,
	OUT max double precision,
	OUT count bigint,
	OUT percent double precision
)
	RETURNS SETOF record
	AS $$ SELECT min, max, count, percent FROM _st_histogram($1, $2, $3, 1, $4, $5, $6) $$
	LANGUAGE 'sql' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_histogram(
	rast raster, nband int,
	exclude_nodata_value boolean,
	bins int,
	right boolean,
	OUT min double precision,
	OUT max double precision,
	OUT count bigint,
	OUT percent double precision
)
	RETURNS SETOF record
	AS $$ SELECT min, max, count, percent FROM _st_histogram($1, $2, $3, 1, $4, NULL, $5) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

-- Cannot be strict as "width" can be NULL
CREATE OR REPLACE FUNCTION st_histogram(
	rast raster, nband int,
	bins int, width double precision[] DEFAULT NULL,
	right boolean DEFAULT FALSE,
	OUT min double precision,
	OUT max double precision,
	OUT count bigint,
	OUT percent double precision
)
	RETURNS SETOF record
	AS $$ SELECT min, max, count, percent FROM _st_histogram($1, $2, TRUE, 1, $3, $4, $5) $$
	LANGUAGE 'sql' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_histogram(
	rast raster, nband int,
	bins int,
	right boolean,
	OUT min double precision,
	OUT max double precision,
	OUT count bigint,
	OUT percent double precision
)
	RETURNS SETOF record
	AS $$ SELECT min, max, count, percent FROM _st_histogram($1, $2, TRUE, 1, $3, NULL, $4) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

-- Cannot be strict as "width" can be NULL
CREATE OR REPLACE FUNCTION st_approxhistogram(
	rast raster, nband int DEFAULT 1,
	exclude_nodata_value boolean DEFAULT TRUE,
	sample_percent double precision DEFAULT 0.1,
	bins int DEFAULT 0, width double precision[] DEFAULT NULL,
	right boolean DEFAULT FALSE,
	OUT min double precision,
	OUT max double precision,
	OUT count bigint,
	OUT percent double precision
)
	RETURNS SETOF record
	AS $$ SELECT min, max, count, percent FROM _st_histogram($1, $2, $3, $4, $5, $6, $7) $$
	LANGUAGE 'sql' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_approxhistogram(
	rast raster, nband int,
	exclude_nodata_value boolean,
	sample_percent double precision,
	bins int,
	right boolean,
	OUT min double precision,
	OUT max double precision,
	OUT count bigint,
	OUT percent double precision
)
	RETURNS SETOF record
	AS $$ SELECT min, max, count, percent FROM _st_histogram($1, $2, $3, $4, $5, NULL, $6) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxhistogram(
	rast raster, nband int,
	sample_percent double precision,
	OUT min double precision,
	OUT max double precision,
	OUT count bigint,
	OUT percent double precision
)
	RETURNS SETOF record
	AS $$ SELECT min, max, count, percent FROM _st_histogram($1, $2, TRUE, $3, 0, NULL, FALSE) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxhistogram(
	rast raster,
	sample_percent double precision,
	OUT min double precision,
	OUT max double precision,
	OUT count bigint,
	OUT percent double precision
)
	RETURNS SETOF record
	AS $$ SELECT min, max, count, percent FROM _st_histogram($1, 1, TRUE, $2, 0, NULL, FALSE) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

-- Cannot be strict as "width" can be NULL
CREATE OR REPLACE FUNCTION st_approxhistogram(
	rast raster, nband int,
	sample_percent double precision,
	bins int, width double precision[] DEFAULT NULL,
	right boolean DEFAULT FALSE,
	OUT min double precision,
	OUT max double precision,
	OUT count bigint,
	OUT percent double precision
)
	RETURNS SETOF record
	AS $$ SELECT min, max, count, percent FROM _st_histogram($1, $2, TRUE, $3, $4, $5, $6) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxhistogram(
	rast raster, nband int,
	sample_percent double precision,
	bins int, right boolean,
	OUT min double precision,
	OUT max double precision,
	OUT count bigint,
	OUT percent double precision
)
	RETURNS SETOF record
	AS $$ SELECT min, max, count, percent FROM _st_histogram($1, $2, TRUE, $3, $4, NULL, $5) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

-- Cannot be strict as "width" can be NULL
CREATE OR REPLACE FUNCTION _st_histogram(
	rastertable text, rastercolumn text,
	nband int DEFAULT 1,
	exclude_nodata_value boolean DEFAULT TRUE,
	sample_percent double precision DEFAULT 1,
	bins int DEFAULT 0, width double precision[] DEFAULT NULL,
	right boolean DEFAULT FALSE,
	OUT min double precision,
	OUT max double precision,
	OUT count bigint,
	OUT percent double precision
)
	RETURNS SETOF record
	AS 'MODULE_PATHNAME','RASTER_histogramCoverage'
	LANGUAGE 'c' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_histogram(
	rastertable text, rastercolumn text, nband int DEFAULT 1,
	exclude_nodata_value boolean DEFAULT TRUE,
	bins int DEFAULT 0, width double precision[] DEFAULT NULL,
	right boolean DEFAULT FALSE,
	OUT min double precision,
	OUT max double precision,
	OUT count bigint,
	OUT percent double precision
)
	RETURNS SETOF record
	AS $$ SELECT _st_histogram($1, $2, $3, $4, 1, $5, $6, $7) $$
	LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION st_histogram(
	rastertable text, rastercolumn text, nband int,
	exclude_nodata_value boolean,
	bins int,
	right boolean,
	OUT min double precision,
	OUT max double precision,
	OUT count bigint,
	OUT percent double precision
)
	RETURNS SETOF record
	AS $$ SELECT _st_histogram($1, $2, $3, $4, 1, $5, NULL, $6) $$
	LANGUAGE 'sql' STABLE STRICT;

-- Cannot be strict as "width" can be NULL
CREATE OR REPLACE FUNCTION st_histogram(
	rastertable text, rastercolumn text, nband int,
	bins int, width double precision[] DEFAULT NULL,
	right boolean DEFAULT FALSE,
	OUT min double precision,
	OUT max double precision,
	OUT count bigint,
	OUT percent double precision
)
	RETURNS SETOF record
	AS $$ SELECT _st_histogram($1, $2, $3, TRUE, 1, $4, $5, $6) $$
	LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION st_histogram(
	rastertable text, rastercolumn text, nband int,
	bins int,
	right boolean,
	OUT min double precision,
	OUT max double precision,
	OUT count bigint,
	OUT percent double precision
)
	RETURNS SETOF record
	AS $$ SELECT _st_histogram($1, $2, $3, TRUE, 1, $4, NULL, $5) $$
	LANGUAGE 'sql' STABLE STRICT;

-- Cannot be strict as "width" can be NULL
CREATE OR REPLACE FUNCTION st_approxhistogram(
	rastertable text, rastercolumn text,
	nband int DEFAULT 1,
	exclude_nodata_value boolean DEFAULT TRUE,
	sample_percent double precision DEFAULT 0.1,
	bins int DEFAULT 0, width double precision[] DEFAULT NULL,
	right boolean DEFAULT FALSE,
	OUT min double precision,
	OUT max double precision,
	OUT count bigint,
	OUT percent double precision
)
	RETURNS SETOF record
	AS $$ SELECT _st_histogram($1, $2, $3, $4, $5, $6, $7, $8) $$
	LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION st_approxhistogram(
	rastertable text, rastercolumn text, nband int,
	exclude_nodata_value boolean,
	sample_percent double precision,
	bins int,
	right boolean,
	OUT min double precision,
	OUT max double precision,
	OUT count bigint,
	OUT percent double precision
)
	RETURNS SETOF record
	AS $$ SELECT _st_histogram($1, $2, $3, $4, $5, $6, NULL, $7) $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxhistogram(
	rastertable text, rastercolumn text, nband int,
	sample_percent double precision,
	OUT min double precision,
	OUT max double precision,
	OUT count bigint,
	OUT percent double precision
)
	RETURNS SETOF record
	AS $$ SELECT _st_histogram($1, $2, $3, TRUE, $4, 0, NULL, FALSE) $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxhistogram(
	rastertable text, rastercolumn text,
	sample_percent double precision,
	OUT min double precision,
	OUT max double precision,
	OUT count bigint,
	OUT percent double precision
)
	RETURNS SETOF record
	AS $$ SELECT _st_histogram($1, $2, 1, TRUE, $3, 0, NULL, FALSE) $$
	LANGUAGE 'sql' STABLE STRICT;

-- Cannot be strict as "width" can be NULL
CREATE OR REPLACE FUNCTION st_approxhistogram(
	rastertable text, rastercolumn text, nband int,
	sample_percent double precision,
	bins int, width double precision[] DEFAULT NULL,
	right boolean DEFAULT FALSE,
	OUT min double precision,
	OUT max double precision,
	OUT count bigint,
	OUT percent double precision
)
	RETURNS SETOF record
	AS $$ SELECT _st_histogram($1, $2, $3, TRUE, $4, $5, $6, $7) $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxhistogram(
	rastertable text, rastercolumn text, nband int,
	sample_percent double precision,
	bins int,
	right boolean,
	OUT min double precision,
	OUT max double precision,
	OUT count bigint,
	OUT percent double precision
)
	RETURNS SETOF record
	AS $$ SELECT _st_histogram($1, $2, $3, TRUE, $4, $5, NULL, $6) $$
	LANGUAGE 'sql' STABLE STRICT;

-----------------------------------------------------------------------
-- ST_Quantile and ST_ApproxQuantile
-----------------------------------------------------------------------
-- Cannot be strict as "quantiles" can be NULL
CREATE OR REPLACE FUNCTION _st_quantile(
	rast raster,
	nband int DEFAULT 1,
	exclude_nodata_value boolean DEFAULT TRUE,
	sample_percent double precision DEFAULT 1,
	quantiles double precision[] DEFAULT NULL,
	OUT quantile double precision,
	OUT value double precision
)
	RETURNS SETOF record
	AS 'MODULE_PATHNAME','RASTER_quantile'
	LANGUAGE 'c' IMMUTABLE;

-- Cannot be strict as "quantiles" can be NULL
CREATE OR REPLACE FUNCTION st_quantile(
	rast raster,
	nband int DEFAULT 1,
	exclude_nodata_value boolean DEFAULT TRUE,
	quantiles double precision[] DEFAULT NULL,
	OUT quantile double precision,
	OUT value double precision
)
	RETURNS SETOF record
	AS $$ SELECT _st_quantile($1, $2, $3, 1, $4) $$
	LANGUAGE 'sql' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_quantile(
	rast raster,
	nband int,
	quantiles double precision[],
	OUT quantile double precision,
	OUT value double precision
)
	RETURNS SETOF record
	AS $$ SELECT _st_quantile($1, $2, TRUE, 1, $3) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_quantile(
	rast raster,
	quantiles double precision[],
	OUT quantile double precision,
	OUT value double precision
)
	RETURNS SETOF record
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
CREATE OR REPLACE FUNCTION st_approxquantile(
	rast raster,
	nband int DEFAULT 1,
	exclude_nodata_value boolean DEFAULT TRUE,
	sample_percent double precision DEFAULT 0.1,
	quantiles double precision[] DEFAULT NULL,
	OUT quantile double precision,
	OUT value double precision
)
	RETURNS SETOF record
	AS $$ SELECT _st_quantile($1, $2, $3, $4, $5) $$
	LANGUAGE 'sql' IMMUTABLE;

-- Cannot be strict as "quantiles" can be NULL
CREATE OR REPLACE FUNCTION st_approxquantile(
	rast raster,
	nband int,
	sample_percent double precision,
	quantiles double precision[] DEFAULT NULL,
	OUT quantile double precision,
	OUT value double precision
)
	RETURNS SETOF record
	AS $$ SELECT _st_quantile($1, $2, TRUE, $3, $4) $$
	LANGUAGE 'sql' IMMUTABLE;

-- Cannot be strict as "quantiles" can be NULL
CREATE OR REPLACE FUNCTION st_approxquantile(
	rast raster,
	sample_percent double precision,
	quantiles double precision[] DEFAULT NULL,
	OUT quantile double precision,
	OUT value double precision
)
	RETURNS SETOF record
	AS $$ SELECT _st_quantile($1, 1, TRUE, $2, $3) $$
	LANGUAGE 'sql' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_approxquantile(
	rast raster,
	quantiles double precision[],
	OUT quantile double precision,
	OUT value double precision
)
	RETURNS SETOF record
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
CREATE OR REPLACE FUNCTION _st_quantile(
	rastertable text,
	rastercolumn text,
	nband int DEFAULT 1,
	exclude_nodata_value boolean DEFAULT TRUE,
	sample_percent double precision DEFAULT 1,
	quantiles double precision[] DEFAULT NULL,
	OUT quantile double precision,
	OUT value double precision
)
	RETURNS SETOF record
	AS 'MODULE_PATHNAME','RASTER_quantileCoverage'
	LANGUAGE 'c' STABLE;

-- Cannot be strict as "quantiles" can be NULL
CREATE OR REPLACE FUNCTION st_quantile(
	rastertable text,
	rastercolumn text,
	nband int DEFAULT 1,
	exclude_nodata_value boolean DEFAULT TRUE,
	quantiles double precision[] DEFAULT NULL,
	OUT quantile double precision,
	OUT value double precision
)
	RETURNS SETOF record
	AS $$ SELECT _st_quantile($1, $2, $3, $4, 1, $5) $$
	LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION st_quantile(
	rastertable text,
	rastercolumn text,
	nband int,
	quantiles double precision[],
	OUT quantile double precision,
	OUT value double precision
)
	RETURNS SETOF record
	AS $$ SELECT _st_quantile($1, $2, $3, TRUE, 1, $4) $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_quantile(
	rastertable text,
	rastercolumn text,
	quantiles double precision[],
	OUT quantile double precision,
	OUT value double precision
)
	RETURNS SETOF record
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
CREATE OR REPLACE FUNCTION st_approxquantile(
	rastertable text,
	rastercolumn text,
	nband int DEFAULT 1,
	exclude_nodata_value boolean DEFAULT TRUE,
	sample_percent double precision DEFAULT 0.1,
	quantiles double precision[] DEFAULT NULL,
	OUT quantile double precision,
	OUT value double precision
)
	RETURNS SETOF record
	AS $$ SELECT _st_quantile($1, $2, $3, $4, $5, $6) $$
	LANGUAGE 'sql' STABLE;

-- Cannot be strict as "quantiles" can be NULL
CREATE OR REPLACE FUNCTION st_approxquantile(
	rastertable text,
	rastercolumn text,
	nband int,
	sample_percent double precision,
	quantiles double precision[] DEFAULT NULL,
	OUT quantile double precision,
	OUT value double precision
)
	RETURNS SETOF record
	AS $$ SELECT _st_quantile($1, $2, $3, TRUE, $4, $5) $$
	LANGUAGE 'sql' STABLE;

-- Cannot be strict as "quantiles" can be NULL
CREATE OR REPLACE FUNCTION st_approxquantile(
	rastertable text,
	rastercolumn text,
	sample_percent double precision,
	quantiles double precision[] DEFAULT NULL,
	OUT quantile double precision,
	OUT value double precision
)
	RETURNS SETOF record
	AS $$ SELECT _st_quantile($1, $2, 1, TRUE, $3, $4) $$
	LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION st_approxquantile(
	rastertable text,
	rastercolumn text,
	quantiles double precision[],
	OUT quantile double precision,
	OUT value double precision
)
	RETURNS SETOF record
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

-- None of the "valuecount" functions with "searchvalues" can be strict as "searchvalues" and "roundto" can be NULL
-- Allowing "searchvalues" to be NULL instructs the function to count all values

CREATE OR REPLACE FUNCTION _st_valuecount(
	rast raster, nband integer DEFAULT 1,
	exclude_nodata_value boolean DEFAULT TRUE,
	searchvalues double precision[] DEFAULT NULL,
	roundto double precision DEFAULT 0,
	OUT value double precision,
	OUT count integer,
	OUT percent double precision
)
	RETURNS SETOF record
	AS 'MODULE_PATHNAME', 'RASTER_valueCount'
	LANGUAGE 'c' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_valuecount(
	rast raster, nband integer DEFAULT 1,
	exclude_nodata_value boolean DEFAULT TRUE,
	searchvalues double precision[] DEFAULT NULL,
	roundto double precision DEFAULT 0,
	OUT value double precision, OUT count integer
)
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

CREATE OR REPLACE FUNCTION st_valuepercent(
	rast raster, nband integer DEFAULT 1,
	exclude_nodata_value boolean DEFAULT TRUE,
	searchvalues double precision[] DEFAULT NULL,
	roundto double precision DEFAULT 0,
	OUT value double precision, OUT percent double precision
)
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

CREATE OR REPLACE FUNCTION _st_valuecount(
	rastertable text,
	rastercolumn text,
	nband integer DEFAULT 1,
	exclude_nodata_value boolean DEFAULT TRUE,
	searchvalues double precision[] DEFAULT NULL,
	roundto double precision DEFAULT 0,
	OUT value double precision,
	OUT count integer,
	OUT percent double precision
)
	RETURNS SETOF record
	AS 'MODULE_PATHNAME', 'RASTER_valueCountCoverage'
	LANGUAGE 'c' STABLE;

CREATE OR REPLACE FUNCTION st_valuecount(
	rastertable text, rastercolumn text,
	nband integer DEFAULT 1,
	exclude_nodata_value boolean DEFAULT TRUE,
	searchvalues double precision[] DEFAULT NULL,
	roundto double precision DEFAULT 0,
	OUT value double precision, OUT count integer
)
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

CREATE OR REPLACE FUNCTION st_valuepercent(
	rastertable text, rastercolumn text,
	nband integer DEFAULT 1,
	exclude_nodata_value boolean DEFAULT TRUE,
	searchvalues double precision[] DEFAULT NULL,
	roundto double precision DEFAULT 0,
	OUT value double precision, OUT percent double precision
)
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
	LANGUAGE 'c' IMMUTABLE STRICT;

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
	LANGUAGE 'sql' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_reclass(rast raster, reclassexpr text, pixeltype text)
	RETURNS raster
	AS $$ SELECT st_reclass($1, ROW(1, $2, $3, NULL)) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- ST_AsGDALRaster and supporting functions
-----------------------------------------------------------------------
-- returns set of available and usable GDAL drivers
CREATE OR REPLACE FUNCTION st_gdaldrivers(OUT idx int, OUT short_name text, OUT long_name text, OUT create_options text)
  RETURNS SETOF record
	AS 'MODULE_PATHNAME', 'RASTER_getGDALDrivers'
	LANGUAGE 'c' IMMUTABLE STRICT;

-- Cannot be strict as "options" and "srid" can be NULL
CREATE OR REPLACE FUNCTION st_asgdalraster(rast raster, format text, options text[] DEFAULT NULL, srid integer DEFAULT NULL)
	RETURNS bytea
	AS 'MODULE_PATHNAME', 'RASTER_asGDALRaster'
	LANGUAGE 'c' IMMUTABLE;

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
	LANGUAGE 'sql' IMMUTABLE;

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
	LANGUAGE 'sql' IMMUTABLE;

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
	LANGUAGE 'sql' IMMUTABLE;

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
	LANGUAGE 'sql' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_asjpeg(rast raster, nband int, quality int)
	RETURNS bytea
	AS $$ SELECT st_asjpeg($1, ARRAY[$2], $3) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

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
	LANGUAGE 'sql' IMMUTABLE;

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
	LANGUAGE 'sql' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_aspng(rast raster, nband int, compression int)
	RETURNS bytea
	AS $$ SELECT st_aspng($1, ARRAY[$2], $3) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

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
	LANGUAGE 'c' STABLE;

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
	LANGUAGE 'c' STABLE;

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
CREATE OR REPLACE FUNCTION st_snaptogrid(
	rast raster,
	gridx double precision, gridy double precision,
	algorithm text DEFAULT 'NearestNeighbour', maxerr double precision DEFAULT 0.125,
	scalex double precision DEFAULT 0, scaley double precision DEFAULT 0
)
	RETURNS raster
	AS $$ SELECT _st_resample($1, $4, $5, NULL, $6, $7, $2, $3) $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_snaptogrid(
	rast raster,
	gridx double precision, gridy double precision,
	scalex double precision, scaley double precision,
	algorithm text DEFAULT 'NearestNeighbour', maxerr double precision DEFAULT 0.125
)
	RETURNS raster
	AS $$ SELECT _st_resample($1, $6, $7, NULL, $4, $5, $2, $3) $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION st_snaptogrid(
	rast raster,
	gridx double precision, gridy double precision,
	scalexy double precision,
	algorithm text DEFAULT 'NearestNeighbour', maxerr double precision DEFAULT 0.125
)
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
    LANGUAGE 'c' IMMUTABLE;

-- This function can not be STRICT, because nodataval can be NULL
-- or pixeltype can not be determined (could be st_bandpixeltype(raster, band) though)
CREATE OR REPLACE FUNCTION st_mapalgebraexpr(rast raster, pixeltype text, expression text,
        nodataval double precision DEFAULT NULL)
    RETURNS raster
    AS $$ SELECT st_mapalgebraexpr($1, 1, $2, $3, $4) $$
    LANGUAGE 'sql';

-- All arguments supplied, use the C implementation.
CREATE OR REPLACE FUNCTION st_mapalgebrafct(rast raster, band integer,
        pixeltype text, onerastuserfunc regprocedure, variadic args text[])
    RETURNS raster
    AS 'MODULE_PATHNAME', 'RASTER_mapAlgebraFct'
    LANGUAGE 'c' IMMUTABLE;

-- Variant 1: missing user args
CREATE OR REPLACE FUNCTION st_mapalgebrafct(rast raster, band integer,
        pixeltype text, onerastuserfunc regprocedure)
    RETURNS raster
    AS $$ SELECT st_mapalgebrafct($1, $2, $3, $4, NULL) $$
    LANGUAGE 'sql';

-- Variant 2: missing pixeltype; default to pixeltype of rast
CREATE OR REPLACE FUNCTION st_mapalgebrafct(rast raster, band integer,
        onerastuserfunc regprocedure, variadic args text[])
    RETURNS raster
    AS $$ SELECT st_mapalgebrafct($1, $2, NULL, $3, VARIADIC $4) $$
    LANGUAGE 'sql';

-- Variant 3: missing pixeltype and user args; default to pixeltype of rast
CREATE OR REPLACE FUNCTION st_mapalgebrafct(rast raster, band integer,
        onerastuserfunc regprocedure)
    RETURNS raster
    AS $$ SELECT st_mapalgebrafct($1, $2, NULL, $3, NULL) $$
    LANGUAGE 'sql';

-- Variant 4: missing band; default to band 1
CREATE OR REPLACE FUNCTION st_mapalgebrafct(rast raster, pixeltype text,
        onerastuserfunc regprocedure, variadic args text[])
    RETURNS raster
    AS $$ SELECT st_mapalgebrafct($1, 1, $2, $3, VARIADIC $4) $$
    LANGUAGE 'sql';

-- Variant 5: missing band and user args; default to band 1
CREATE OR REPLACE FUNCTION st_mapalgebrafct(rast raster, pixeltype text,
        onerastuserfunc regprocedure)
    RETURNS raster
    AS $$ SELECT st_mapalgebrafct($1, 1, $2, $3, NULL) $$
    LANGUAGE 'sql';

-- Variant 6: missing band, and pixeltype; default to band 1, pixeltype of rast.
CREATE OR REPLACE FUNCTION st_mapalgebrafct(rast raster, onerastuserfunc regprocedure,
        variadic args text[])
    RETURNS raster
    AS $$ SELECT st_mapalgebrafct($1, 1, NULL, $2, VARIADIC $3) $$
    LANGUAGE 'sql';

-- Variant 7: missing band, pixeltype, and user args; default to band 1, pixeltype of rast.
CREATE OR REPLACE FUNCTION st_mapalgebrafct(rast raster, onerastuserfunc regprocedure)
    RETURNS raster
    AS $$ SELECT st_mapalgebrafct($1, 1, NULL, $2, NULL) $$
    LANGUAGE 'sql';

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
	LANGUAGE 'c' STABLE;

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
	LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION st_mapalgebrafct(
	rast1 raster, band1 integer,
	rast2 raster, band2 integer,
	tworastuserfunc regprocedure,
	pixeltype text DEFAULT NULL, extenttype text DEFAULT 'INTERSECTION',
	VARIADIC userargs text[] DEFAULT NULL
)
	RETURNS raster
	AS 'MODULE_PATHNAME', 'RASTER_mapAlgebra2'
	LANGUAGE 'c' STABLE;

CREATE OR REPLACE FUNCTION st_mapalgebrafct(
	rast1 raster,
	rast2 raster,
	tworastuserfunc regprocedure,
	pixeltype text DEFAULT NULL, extenttype text DEFAULT 'INTERSECTION',
	VARIADIC userargs text[] DEFAULT NULL
)
	RETURNS raster
	AS $$ SELECT st_mapalgebrafct($1, 1, $2, 1, $3, $4, $5, VARIADIC $6) $$
	LANGUAGE 'sql' STABLE;

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
    LANGUAGE 'c' IMMUTABLE;

-----------------------------------------------------------------------
-- Neighborhood MapAlgebra processing functions.
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_max4ma(matrix float[][], nodatamode text, variadic args text[])
    RETURNS float AS
    $$
    DECLARE
        _matrix float[][];
        max float;
    BEGIN
        _matrix := matrix;
        max := '-Infinity'::float;
        FOR x in array_lower(_matrix, 1)..array_upper(_matrix, 1) LOOP
            FOR y in array_lower(_matrix, 2)..array_upper(_matrix, 2) LOOP
                IF _matrix[x][y] IS NULL THEN
                    IF NOT nodatamode = 'ignore' THEN
                        _matrix[x][y] := nodatamode::float;
                    END IF;
                END IF;
                IF max < _matrix[x][y] THEN
                    max := _matrix[x][y];
                END IF;
            END LOOP;
        END LOOP;
        RETURN max;
    END;
    $$
    LANGUAGE 'plpgsql' IMMUTABLE;


CREATE OR REPLACE FUNCTION st_min4ma(matrix float[][], nodatamode text, variadic args text[])
    RETURNS float AS
    $$
    DECLARE
        _matrix float[][];
        min float;
    BEGIN
        _matrix := matrix;
        min := 'Infinity'::float;
        FOR x in array_lower(_matrix, 1)..array_upper(_matrix, 1) LOOP
            FOR y in array_lower(_matrix, 2)..array_upper(_matrix, 2) LOOP
                IF _matrix[x][y] IS NULL THEN
                    IF NOT nodatamode = 'ignore' THEN
                        _matrix[x][y] := nodatamode::float;
                    END IF;
                END IF;
                IF min > _matrix[x][y] THEN
                    min := _matrix[x][y];
                END IF;
            END LOOP;
        END LOOP;
        RETURN min;
    END;
    $$
    LANGUAGE 'plpgsql' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_sum4ma(matrix float[][], nodatamode text, variadic args text[])
    RETURNS float AS
    $$
    DECLARE
        _matrix float[][];
        sum float;
    BEGIN
        _matrix := matrix;
        sum := 0;
        FOR x in array_lower(matrix, 1)..array_upper(matrix, 1) LOOP
            FOR y in array_lower(matrix, 2)..array_upper(matrix, 2) LOOP
                IF _matrix[x][y] IS NULL THEN
                    IF nodatamode = 'ignore' THEN
                        _matrix[x][y] := 0;
                    ELSE
                        _matrix[x][y] := nodatamode::float;
                    END IF;
                END IF;
                sum := sum + _matrix[x][y];
            END LOOP;
        END LOOP;
        RETURN sum;
    END;
    $$
    LANGUAGE 'plpgsql' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_mean4ma(matrix float[][], nodatamode text, variadic args text[])
    RETURNS float AS
    $$
    DECLARE
        _matrix float[][];
        sum float;
        count float;
    BEGIN
        _matrix := matrix;
        sum := 0;
        count := 0;
        FOR x in array_lower(matrix, 1)..array_upper(matrix, 1) LOOP
            FOR y in array_lower(matrix, 2)..array_upper(matrix, 2) LOOP
                IF _matrix[x][y] IS NULL THEN
                    IF nodatamode = 'ignore' THEN
                        _matrix[x][y] := 0;
                    ELSE
                        _matrix[x][y] := nodatamode::float;
                        count := count + 1;
                    END IF;
                ELSE
                    count := count + 1;
                END IF;
                sum := sum + _matrix[x][y];
            END LOOP;
        END LOOP;
        IF count = 0 THEN
            RETURN NULL;
        END IF;
        RETURN sum / count;
    END;
    $$
    LANGUAGE 'plpgsql' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_range4ma(matrix float[][], nodatamode text, variadic args text[])
    RETURNS float AS
    $$
    DECLARE
        _matrix float[][];
        min float;
        max float;
    BEGIN
        _matrix := matrix;
        min := 'Infinity'::float;
        max := '-Infinity'::float;
        FOR x in array_lower(matrix, 1)..array_upper(matrix, 1) LOOP
            FOR y in array_lower(matrix, 2)..array_upper(matrix, 2) LOOP
                IF _matrix[x][y] IS NULL THEN
                    IF NOT nodatamode = 'ignore' THEN
                        _matrix[x][y] := nodatamode::float;
                    END IF;
                END IF;
                IF min > _matrix[x][y] THEN
                    min = _matrix[x][y];
                END IF;
                IF max < _matrix[x][y] THEN
                    max = _matrix[x][y];
                END IF;
            END LOOP;
        END LOOP;
        IF max = '-Infinity'::float OR min = 'Infinity'::float THEN
            RETURN NULL;
        END IF;
        RETURN max - min;
    END;
    $$
    LANGUAGE 'plpgsql' IMMUTABLE;

CREATE OR REPLACE FUNCTION _st_slope4ma(matrix float[][], nodatamode text, variadic args text[])
    RETURNS float
    AS
    $$
    DECLARE
        pwidth float;
        pheight float;
        dz_dx float;
        dz_dy float;
    BEGIN
        pwidth := args[1]::float;
        pheight := args[2]::float;
        dz_dx := ((matrix[3][1] + 2.0 * matrix[3][2] + matrix[3][3]) - (matrix[1][1] + 2.0 * matrix[1][2] + matrix[1][3])) / (8.0 * pwidth);
        dz_dy := ((matrix[1][3] + 2.0 * matrix[2][3] + matrix[3][3]) - (matrix[1][1] + 2.0 * matrix[2][1] + matrix[3][1])) / (8.0 * pheight);
        RETURN atan(sqrt(pow(dz_dx, 2.0) + pow(dz_dy, 2.0)));
    END;
    $$
    LANGUAGE 'plpgsql' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_slope(rast raster, band integer, pixeltype text)
    RETURNS RASTER
    AS $$ SELECT st_mapalgebrafctngb($1, $2, $3, 1, 1, '_st_slope4ma(float[][], text, text[])'::regprocedure, 'value', st_pixelwidth($1)::text, st_pixelheight($1)::text) $$
    LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION _st_aspect4ma(matrix float[][], nodatamode text, variadic args text[])
    RETURNS float
    AS
    $$
    DECLARE
        pwidth float;
        pheight float;
        dz_dx float;
        dz_dy float;
        aspect float;
    BEGIN
        pwidth := args[1]::float;
        pheight := args[2]::float;
        dz_dx := ((matrix[3][1] + 2.0 * matrix[3][2] + matrix[3][3]) - (matrix[1][1] + 2.0 * matrix[1][2] + matrix[1][3])) / (8.0 * pwidth);
        dz_dy := ((matrix[1][3] + 2.0 * matrix[2][3] + matrix[3][3]) - (matrix[1][1] + 2.0 * matrix[2][1] + matrix[3][1])) / (8.0 * pheight);
        IF abs(dz_dx) = 0::float AND abs(dz_dy) = 0::float THEN
            RETURN -1;
        END IF;

        aspect := atan2(dz_dy, -dz_dx);
        IF aspect > (pi() / 2.0) THEN
            RETURN (5.0 * pi() / 2.0) - aspect;
        ELSE
            RETURN (pi() / 2.0) - aspect;
        END IF;
    END;
    $$
    LANGUAGE 'plpgsql' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_aspect(rast raster, band integer, pixeltype text)
    RETURNS RASTER
    AS $$ SELECT st_mapalgebrafctngb($1, $2, $3, 1, 1, '_st_aspect4ma(float[][], text, text[])'::regprocedure, 'value', st_pixelwidth($1)::text, st_pixelheight($1)::text) $$
    LANGUAGE 'sql' STABLE;


CREATE OR REPLACE FUNCTION _st_hillshade4ma(matrix float[][], nodatamode text, variadic args text[])
    RETURNS float
    AS
    $$
    DECLARE
        pwidth float;
        pheight float;
        dz_dx float;
        dz_dy float;
        zenith float;
        azimuth float;
        slope float;
        aspect float;
        max_bright float;
        elevation_scale float;
    BEGIN
        pwidth := args[1]::float;
        pheight := args[2]::float;
        azimuth := (5.0 * pi() / 2.0) - args[3]::float;
        zenith := (pi() / 2.0) - args[4]::float;
        dz_dx := ((matrix[3][1] + 2.0 * matrix[3][2] + matrix[3][3]) - (matrix[1][1] + 2.0 * matrix[1][2] + matrix[1][3])) / (8.0 * pwidth);
        dz_dy := ((matrix[1][3] + 2.0 * matrix[2][3] + matrix[3][3]) - (matrix[1][1] + 2.0 * matrix[2][1] + matrix[3][1])) / (8.0 * pheight);
        elevation_scale := args[6]::float;
        slope := atan(sqrt(elevation_scale * pow(dz_dx, 2.0) + pow(dz_dy, 2.0)));
        -- handle special case of 0, 0
        IF abs(dz_dy) = 0::float AND abs(dz_dy) = 0::float THEN
            -- set to pi as that is the expected PostgreSQL answer in Linux
            aspect := pi();
        ELSE
            aspect := atan2(dz_dy, -dz_dx);
        END IF;
        max_bright := args[5]::float;

        IF aspect < 0 THEN
            aspect := aspect + (2.0 * pi());
        END IF;

        RETURN max_bright * ( (cos(zenith)*cos(slope)) + (sin(zenith)*sin(slope)*cos(azimuth - aspect)) );
    END;
    $$
    LANGUAGE 'plpgsql' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_hillshade(rast raster, band integer, pixeltype text, azimuth float, altitude float, max_bright float DEFAULT 255.0, elevation_scale float DEFAULT 1.0)
    RETURNS RASTER
    AS $$ SELECT st_mapalgebrafctngb($1, $2, $3, 1, 1, '_st_hillshade4ma(float[][], text, text[])'::regprocedure, 'value', st_pixelwidth($1)::text, st_pixelheight($1)::text, $4::text, $5::text, $6::text, $7::text) $$
    LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION st_distinct4ma(matrix float[][], nodatamode TEXT, VARIADIC args TEXT[])
    RETURNS float AS
    $$ SELECT COUNT(DISTINCT unnest)::float FROM unnest($1) $$
    LANGUAGE 'sql' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_stddev4ma(matrix float[][], nodatamode TEXT, VARIADIC args TEXT[])
    RETURNS float AS
    $$ SELECT stddev(unnest) FROM unnest($1) $$
    LANGUAGE 'sql' IMMUTABLE;


-----------------------------------------------------------------------
-- Get information about the raster
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_isempty(rast raster)
    RETURNS boolean
    AS 'MODULE_PATHNAME', 'RASTER_isEmpty'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_hasnoband(rast raster, nband int DEFAULT 1)
    RETURNS boolean
    AS 'MODULE_PATHNAME', 'RASTER_hasNoBand'
    LANGUAGE 'c' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- Raster Band Accessors
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION st_bandnodatavalue(rast raster, band integer DEFAULT 1)
    RETURNS double precision
    AS 'MODULE_PATHNAME','RASTER_getBandNoDataValue'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_bandisnodata(rast raster, band integer DEFAULT 1, forceChecking boolean DEFAULT FALSE)
    RETURNS boolean
    AS 'MODULE_PATHNAME', 'RASTER_bandIsNoData'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_bandisnodata(rast raster, forceChecking boolean)
    RETURNS boolean
    AS $$ SELECT st_bandisnodata($1, 1, $2) $$
    LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_bandpath(rast raster, band integer DEFAULT 1)
    RETURNS text
    AS 'MODULE_PATHNAME','RASTER_getBandPath'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_bandpixeltype(rast raster, band integer DEFAULT 1)
    RETURNS text
    AS 'MODULE_PATHNAME','RASTER_getBandPixelTypeName'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_bandmetadata(
	rast raster,
	band int[],
	OUT bandnum int,
	OUT pixeltype text,
	OUT nodatavalue double precision,
	OUT isoutdb boolean,
	OUT path text
)
	AS 'MODULE_PATHNAME','RASTER_bandmetadata'
	LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_bandmetadata(
	rast raster,
	band int DEFAULT 1,
	OUT pixeltype text,
	OUT nodatavalue double precision,
	OUT isoutdb boolean,
	OUT path text
)
	AS $$ SELECT pixeltype, nodatavalue, isoutdb, path FROM st_bandmetadata($1, ARRAY[$2]::int[]) LIMIT 1 $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- Raster Pixel Accessors
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION st_value(rast raster, band integer, x integer, y integer, exclude_nodata_value boolean DEFAULT TRUE)
    RETURNS float8
    AS 'MODULE_PATHNAME','RASTER_getPixelValue'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_value(rast raster, x integer, y integer, exclude_nodata_value boolean DEFAULT TRUE)
    RETURNS float8
    AS $$ SELECT st_value($1, 1, $2, $3, $4) $$
    LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_value(rast raster, band integer, pt geometry, exclude_nodata_value boolean DEFAULT TRUE)
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
                        exclude_nodata_value);
    END;
    $$
    LANGUAGE 'plpgsql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_value(rast raster, pt geometry, exclude_nodata_value boolean DEFAULT TRUE)
    RETURNS float8
    AS $$ SELECT st_value($1, 1, $2, $3) $$
    LANGUAGE 'sql' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- ST_PixelOfValue()
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION st_pixelofvalue(
	rast raster,
	nband integer,
	search double precision[],
	exclude_nodata_value boolean DEFAULT TRUE,
	OUT val double precision,
	OUT x integer,
	OUT y integer
)
	RETURNS SETOF record
	AS 'MODULE_PATHNAME', 'RASTER_pixelOfValue'
	LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_pixelofvalue(
	rast raster,
	search double precision[],
	exclude_nodata_value boolean DEFAULT TRUE,
	OUT val double precision,
	OUT x integer,
	OUT y integer
)
	RETURNS SETOF record
	AS $$ SELECT val, x, y FROM st_pixelofvalue($1, 1, $2, $3) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_pixelofvalue(
	rast raster,
	nband integer,
	search double precision,
	exclude_nodata_value boolean DEFAULT TRUE,
	OUT x integer,
	OUT y integer
)
	RETURNS SETOF record
	AS $$ SELECT x, y FROM st_pixelofvalue($1, $2, ARRAY[$3], $4) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_pixelofvalue(
	rast raster,
	search double precision,
	exclude_nodata_value boolean DEFAULT TRUE,
	OUT x integer,
	OUT y integer
)
	RETURNS SETOF record
	AS $$ SELECT x, y FROM st_pixelofvalue($1, 1, ARRAY[$2], $3) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- Raster Accessors ST_Georeference()
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION st_georeference(rast raster, format text DEFAULT 'GDAL')
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

-----------------------------------------------------------------------
-- Raster Editors
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION st_setscale(rast raster, scale float8)
    RETURNS raster
    AS 'MODULE_PATHNAME','RASTER_setScale'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_setscale(rast raster, scalex float8, scaley float8)
    RETURNS raster
    AS 'MODULE_PATHNAME','RASTER_setScaleXY'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_setskew(rast raster, skew float8)
    RETURNS raster
    AS 'MODULE_PATHNAME','RASTER_setSkew'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_setskew(rast raster, skewx float8, skewy float8)
    RETURNS raster
    AS 'MODULE_PATHNAME','RASTER_setSkewXY'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_setsrid(rast raster, srid integer)
    RETURNS raster
    AS 'MODULE_PATHNAME','RASTER_setSRID'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_setupperleft(rast raster, upperleftx float8, upperlefty float8)
    RETURNS raster
    AS 'MODULE_PATHNAME','RASTER_setUpperLeftXY'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_setrotation(rast raster, rotation float8)
    RETURNS raster
    AS 'MODULE_PATHNAME','RASTER_setRotation'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_setgeotransform(rast raster,
    imag double precision, 
    jmag double precision,
    theta_i double precision,
    theta_ij double precision,
    xoffset double precision,
    yoffset double precision)
    RETURNS raster
    AS 'MODULE_PATHNAME','RASTER_setGeotransform'
    LANGUAGE 'c' IMMUTABLE;

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
    LANGUAGE 'c' IMMUTABLE;

-- This function can not be STRICT, because nodatavalue can be NULL indicating that no nodata value should be set
CREATE OR REPLACE FUNCTION st_setbandnodatavalue(rast raster, nodatavalue float8)
    RETURNS raster
    AS $$ SELECT st_setbandnodatavalue($1, 1, $2, FALSE) $$
    LANGUAGE 'sql';

CREATE OR REPLACE FUNCTION st_setbandisnodata(rast raster, band integer DEFAULT 1)
    RETURNS raster
    AS 'MODULE_PATHNAME', 'RASTER_setBandIsNoData'
    LANGUAGE 'c' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- Raster Pixel Editors
-----------------------------------------------------------------------

-----------------------------------------------------------------------
-- ST_SetValue (set one or more pixels to a single value)
-----------------------------------------------------------------------

-- This function can not be STRICT, because newvalue can be NULL for nodata
CREATE OR REPLACE FUNCTION st_setvalue(rast raster, band integer, x integer, y integer, newvalue float8)
    RETURNS raster
    AS 'MODULE_PATHNAME','RASTER_setPixelValue'
    LANGUAGE 'c' IMMUTABLE;

-- This function can not be STRICT, because newvalue can be NULL for nodata
CREATE OR REPLACE FUNCTION st_setvalue(rast raster, x integer, y integer, newvalue float8)
    RETURNS raster
    AS $$ SELECT st_setvalue($1, 1, $2, $3, $4) $$
    LANGUAGE 'sql';

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
    LANGUAGE 'sql';

-----------------------------------------------------------------------
-- ST_SetValues (set one or more pixels to a one or more values)
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION _st_setvalues(
	rast raster, nband integer,
	x integer, y integer,
	newvalueset double precision[][],
	noset boolean[][] DEFAULT NULL,
	hasnosetvalue boolean DEFAULT FALSE,
	nosetvalue double precision DEFAULT NULL,
	keepnodata boolean DEFAULT FALSE
)
	RETURNS raster
	AS 'MODULE_PATHNAME', 'RASTER_setPixelValuesArray'
	LANGUAGE 'c' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_setvalues(
	rast raster, nband integer,
	x integer, y integer,
	newvalueset double precision[][],
	noset boolean[][] DEFAULT NULL,
	keepnodata boolean DEFAULT FALSE
)
	RETURNS raster
	AS $$ SELECT _st_setvalues($1, $2, $3, $4, $5, $6, FALSE, NULL, $7) $$
	LANGUAGE 'sql' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_setvalues(
	rast raster, nband integer,
	x integer, y integer,
	newvalueset double precision[][],
	nosetvalue double precision,
	keepnodata boolean DEFAULT FALSE
)
	RETURNS raster
	AS $$ SELECT _st_setvalues($1, $2, $3, $4, $5, NULL, TRUE, $6, $7) $$
	LANGUAGE 'sql' IMMUTABLE;

-- cannot be STRICT as newvalue can be NULL
CREATE OR REPLACE FUNCTION st_setvalues(
	rast raster, nband integer,
	x integer, y integer,
	width integer, height integer,
	newvalue double precision,
	keepnodata boolean DEFAULT FALSE
)
	RETURNS raster AS
	$$
	BEGIN
		IF width <= 0 OR height <= 0 THEN
			RAISE EXCEPTION 'Values for width and height must be greater than zero';
			RETURN NULL;
		END IF;
		RETURN _st_setvalues($1, $2, $3, $4, array_fill($7, ARRAY[$6, $5]::int[]), NULL, FALSE, NULL, $8);
	END;
	$$
	LANGUAGE 'plpgsql' IMMUTABLE;

-- cannot be STRICT as newvalue can be NULL
CREATE OR REPLACE FUNCTION st_setvalues(
	rast raster,
	x integer, y integer,
	width integer, height integer,
	newvalue double precision,
	keepnodata boolean DEFAULT FALSE
)
	RETURNS raster AS
	$$
	BEGIN
		IF width <= 0 OR height <= 0 THEN
			RAISE EXCEPTION 'Values for width and height must be greater than zero';
			RETURN NULL;
		END IF;
		RETURN _st_setvalues($1, 1, $2, $3, array_fill($6, ARRAY[$5, $4]::int[]), NULL, FALSE, NULL, $7);
	END;
	$$
	LANGUAGE 'plpgsql' IMMUTABLE;

-----------------------------------------------------------------------
-- Raster Processing Functions
-----------------------------------------------------------------------

CREATE TYPE geomval AS (
	geom geometry,
	val double precision
);

-----------------------------------------------------------------------
-- ST_DumpAsPolygons
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_dumpaspolygons(rast raster, band integer DEFAULT 1, exclude_nodata_value boolean DEFAULT TRUE)
	RETURNS SETOF geomval
	AS 'MODULE_PATHNAME','RASTER_dumpAsPolygons'
	LANGUAGE 'c' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- ST_Polygon
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_polygon(rast raster, band integer DEFAULT 1)
	RETURNS geometry
	AS 'MODULE_PATHNAME','RASTER_getPolygon'
	LANGUAGE 'c' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- ST_PixelAsPolygons
-- Return all the pixels of a raster as a geom, val, x, y record
-- Should be called like this:
-- SELECT (gv).geom, (gv).val FROM (SELECT ST_PixelAsPolygons(rast) gv FROM mytable) foo
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION _st_pixelaspolygons(
	rast raster,
	band integer DEFAULT 1,
	columnx integer DEFAULT NULL,
	rowy integer DEFAULT NULL,
	exclude_nodata_value boolean DEFAULT TRUE,
	OUT geom geometry,
	OUT val double precision,
	OUT x integer,
	OUT y integer
)
	RETURNS SETOF record
	AS 'MODULE_PATHNAME', 'RASTER_getPixelPolygons'
	LANGUAGE 'c' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_pixelaspolygons(
	rast raster,
	band integer DEFAULT 1,
	exclude_nodata_value boolean DEFAULT TRUE,
	OUT geom geometry,
	OUT val double precision,
	OUT x int,
	OUT y int
)
	RETURNS SETOF record
	AS $$ SELECT geom, val, x, y FROM _st_pixelaspolygons($1, $2, NULL, NULL, $3) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- ST_PixelAsPolygon
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION st_pixelaspolygon(rast raster, x integer, y integer)
	RETURNS geometry
	AS $$ SELECT geom FROM _st_pixelaspolygons($1, NULL, $2, $3) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- ST_PixelAsPoints
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION st_pixelaspoints(
	rast raster,
	band integer DEFAULT 1,
	exclude_nodata_value boolean DEFAULT TRUE,
	OUT geom geometry,
	OUT val double precision,
	OUT x int,
	OUT y int
)
	RETURNS SETOF record
	AS $$ SELECT ST_PointN(ST_ExteriorRing(geom), 1), val, x, y FROM _st_pixelaspolygons($1, $2, NULL, NULL, $3) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- ST_PixelAsPoint
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION st_pixelaspoint(rast raster, x integer, y integer)
	RETURNS geometry
	AS $$ SELECT ST_PointN(ST_ExteriorRing(geom), 1) FROM _st_pixelaspolygons($1, NULL, $2, $3) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- ST_PixelAsCentroids
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION st_pixelascentroids(
	rast raster,
	band integer DEFAULT 1,
	exclude_nodata_value boolean DEFAULT TRUE,
	OUT geom geometry,
	OUT val double precision,
	OUT x int,
	OUT y int
)
	RETURNS SETOF record
	AS $$ SELECT ST_Centroid(geom), val, x, y FROM _st_pixelaspolygons($1, $2, NULL, NULL, $3) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- ST_PixelAsCentroid
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION st_pixelascentroid(rast raster, x integer, y integer)
	RETURNS geometry
	AS $$ SELECT ST_Centroid(geom) FROM _st_pixelaspolygons($1, NULL, $2, $3) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- Raster Utility Functions
-----------------------------------------------------------------------

-----------------------------------------------------------------------
-- ST_World2RasterCoord
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION _st_world2rastercoord(
	rast raster,
	longitude double precision DEFAULT NULL, latitude double precision DEFAULT NULL,
	OUT columnx integer,
	OUT rowy integer
)
	AS 'MODULE_PATHNAME', 'RASTER_worldToRasterCoord'
	LANGUAGE 'c' IMMUTABLE;

---------------------------------------------------------------------------------
-- ST_World2RasterCoord(rast raster, longitude float8, latitude float8)
-- Returns the pixel column and row covering the provided X and Y world
-- coordinates.
-- This function works even if the world coordinates are outside the raster extent.
---------------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_world2rastercoord(
	rast raster,
	longitude double precision, latitude double precision,
	OUT columnx integer,
	OUT rowy integer
)
	AS $$ SELECT columnx, rowy FROM _st_world2rastercoord($1, $2, $3) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

---------------------------------------------------------------------------------
-- ST_World2RasterCoordX(rast raster, pt geometry)
-- Returns the pixel column and row covering the provided point geometry. 
-- This function works even if the point is outside the raster extent.
---------------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_world2rastercoord(
	rast raster, pt geometry,
	OUT columnx integer,
	OUT rowy integer
)
	AS
	$$
	DECLARE
		rx integer;
		ry integer;
	BEGIN
		IF (st_geometrytype(pt) != 'ST_Point') THEN
			RAISE EXCEPTION 'Attempting to compute raster coordinate with a non-point geometry';
		END IF;
		SELECT rc.columnx AS x, rc.rowy AS y INTO columnx, rowy FROM _st_world2rastercoord($1, st_x(pt), st_y(pt)) AS rc;
		RETURN;
	END;
	$$
	LANGUAGE 'plpgsql' IMMUTABLE STRICT;

---------------------------------------------------------------------------------
-- ST_World2RasterCoordX(rast raster, xw float8, yw float8)
-- Returns the column number of the pixel covering the provided X and Y world
-- coordinates.
-- This function works even if the world coordinates are outside the raster extent.
---------------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_world2rastercoordx(rast raster, xw float8, yw float8)
	RETURNS int
	AS $$ SELECT columnx FROM _st_world2rastercoord($1, $2, $3) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

---------------------------------------------------------------------------------
-- ST_World2RasterCoordX(rast raster, xw float8)
-- Returns the column number of the pixels covering the provided world X coordinate
-- for a non-rotated raster.
-- This function works even if the world coordinate is outside the raster extent.
-- This function returns an error if the raster is rotated. In this case you must
-- also provide a Y.
---------------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_world2rastercoordx(rast raster, xw float8)
	RETURNS int
	AS $$ SELECT columnx FROM _st_world2rastercoord($1, $2, NULL) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

---------------------------------------------------------------------------------
-- ST_World2RasterCoordX(rast raster, pt geometry)
-- Returns the column number of the pixel covering the provided point geometry.
-- This function works even if the point is outside the raster extent.
---------------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_world2rastercoordx(rast raster, pt geometry)
	RETURNS int AS
	$$
	DECLARE
		xr integer;
	BEGIN
		IF ( st_geometrytype(pt) != 'ST_Point' ) THEN
			RAISE EXCEPTION 'Attempting to compute raster coordinate with a non-point geometry';
		END IF;
		SELECT columnx INTO xr FROM _st_world2rastercoord($1, st_x(pt), st_y(pt));
		RETURN xr;
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
	RETURNS int
	AS $$ SELECT rowy FROM _st_world2rastercoord($1, $2, $3) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

---------------------------------------------------------------------------------
-- ST_World2RasterCoordY(rast raster, yw float8)
-- Returns the row number of the pixels covering the provided world Y coordinate
-- for a non-rotated raster.
-- This function works even if the world coordinate is outside the raster extent.
-- This function returns an error if the raster is rotated. In this case you must
-- also provide an X.
---------------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_world2rastercoordy(rast raster, yw float8)
	RETURNS int
	AS $$ SELECT rowy FROM _st_world2rastercoord($1, NULL, $2) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

---------------------------------------------------------------------------------
-- ST_World2RasterCoordY(rast raster, pt geometry)
-- Returns the row number of the pixel covering the provided point geometry.
-- This function works even if the point is outside the raster extent.
---------------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_world2rastercoordy(rast raster, pt geometry)
	RETURNS int AS
	$$
	DECLARE
		yr integer;
	BEGIN
		IF ( st_geometrytype(pt) != 'ST_Point' ) THEN
			RAISE EXCEPTION 'Attempting to compute raster coordinate with a non-point geometry';
		END IF;
		SELECT rowy INTO yr FROM _st_world2rastercoord($1, st_x(pt), st_y(pt));
		RETURN yr;
	END;
	$$
	LANGUAGE 'plpgsql' IMMUTABLE STRICT;

---------------------------------------------------------------------------------
-- ST_Raster2WorldCoord
---------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION _st_raster2worldcoord(
	rast raster,
	columnx integer DEFAULT NULL, rowy integer DEFAULT NULL,
	OUT longitude double precision,
	OUT latitude double precision
)
	AS 'MODULE_PATHNAME', 'RASTER_rasterToWorldCoord'
	LANGUAGE 'c' IMMUTABLE;

---------------------------------------------------------------------------------
-- ST_Raster2WorldCoordX(rast raster, xr int, yr int)
-- Returns the longitude and latitude of the upper left corner of the pixel
-- located at the provided pixel column and row.
-- This function works even if the provided raster column and row are beyond or
-- below the raster width and height.
---------------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_raster2worldcoord(
	rast raster,
	columnx integer, rowy integer,
	OUT longitude double precision,
	OUT latitude double precision
)
	AS $$ SELECT longitude, latitude FROM _st_raster2worldcoord($1, $2, $3) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

---------------------------------------------------------------------------------
-- ST_Raster2WorldCoordX(rast raster, xr int, yr int)
-- Returns the X world coordinate of the upper left corner of the pixel located at
-- the provided column and row numbers.
-- This function works even if the provided raster column and row are beyond or
-- below the raster width and height.
---------------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_raster2worldcoordx(rast raster, xr int, yr int)
	RETURNS float8
	AS $$ SELECT longitude FROM _st_raster2worldcoord($1, $2, $3) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

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
	RETURNS float8
	AS $$ SELECT longitude FROM _st_raster2worldcoord($1, $2, NULL) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

---------------------------------------------------------------------------------
-- ST_Raster2WorldCoordY(rast raster, xr int, yr int)
-- Returns the Y world coordinate of the upper left corner of the pixel located at
-- the provided column and row numbers.
-- This function works even if the provided raster column and row are beyond or
-- below the raster width and height.
---------------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_raster2worldcoordy(rast raster, xr int, yr int)
	RETURNS float8
	AS $$ SELECT latitude FROM _st_raster2worldcoord($1, $2, $3) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

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
	RETURNS float8
	AS $$ SELECT latitude FROM _st_raster2worldcoord($1, NULL, $2) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- ST_MinPossibleValue(pixeltype text)
-- Return the smallest value for a given pixeltyp.
-- Should be called like this:
-- SELECT ST_MinPossibleValue(ST_BandPixelType(rast, band))
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION st_minpossiblevalue(pixeltype text)
	RETURNS double precision
	AS 'MODULE_PATHNAME', 'RASTER_minPossibleValue'
	LANGUAGE 'c' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- Raster Outputs
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION st_asbinary(raster)
    RETURNS bytea
    AS 'MODULE_PATHNAME', 'RASTER_to_binary'
    LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION bytea(raster)
    RETURNS bytea
    AS 'MODULE_PATHNAME', 'RASTER_to_bytea'
    LANGUAGE 'c' IMMUTABLE STRICT;

------------------------------------------------------------------------------
--  Casts
------------------------------------------------------------------------------

CREATE CAST (raster AS box3d)
    WITH FUNCTION box3d(raster) AS ASSIGNMENT;

CREATE CAST (raster AS geometry)
    WITH FUNCTION st_convexhull(raster) AS ASSIGNMENT;

CREATE CAST (raster AS bytea)
    WITH FUNCTION bytea(raster) AS ASSIGNMENT;

------------------------------------------------------------------------------
--  GiST index OPERATOR support functions
------------------------------------------------------------------------------
-- raster/raster functions
CREATE OR REPLACE FUNCTION raster_overleft(raster, raster)
    RETURNS bool
    AS 'select $1::geometry &< $2::geometry'
    LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION raster_overright(raster, raster)
    RETURNS bool
    AS 'select $1::geometry &> $2::geometry'
    LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION raster_left(raster, raster)
    RETURNS bool
    AS 'select $1::geometry << $2::geometry'
    LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION raster_right(raster, raster)
    RETURNS bool
    AS 'select $1::geometry >> $2::geometry'
    LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION raster_overabove(raster, raster)
    RETURNS bool
    AS 'select $1::geometry |&> $2::geometry'
    LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION raster_overbelow(raster, raster)
    RETURNS bool
    AS 'select $1::geometry &<| $2::geometry'
    LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION raster_above(raster, raster)
    RETURNS bool
    AS 'select $1::geometry |>> $2::geometry'
    LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION raster_below(raster, raster)
    RETURNS bool
    AS 'select $1::geometry <<| $2::geometry'
    LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION raster_same(raster, raster)
    RETURNS bool
    AS 'select $1::geometry ~= $2::geometry'
    LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION raster_contained(raster, raster)
    RETURNS bool
    AS 'select $1::geometry @ $2::geometry'
    LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION raster_contain(raster, raster)
    RETURNS bool
    AS 'select $1::geometry ~ $2::geometry'
    LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION raster_overlap(raster, raster)
    RETURNS bool
    AS 'select $1::geometry && $2::geometry'
    LANGUAGE 'sql' IMMUTABLE STRICT;

-- raster/geometry functions
CREATE OR REPLACE FUNCTION raster_geometry_contain(raster, geometry)
    RETURNS bool
    AS 'select $1::geometry ~ $2'
    LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION raster_geometry_overlap(raster, geometry)
    RETURNS bool
    AS 'select $1::geometry && $2'
    LANGUAGE 'sql' IMMUTABLE STRICT;
    
-- geometry/raster functions
CREATE OR REPLACE FUNCTION geometry_raster_contain(geometry, raster)
    RETURNS bool
    AS 'select $1 ~ $2::geometry'
    LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION geometry_raster_overlap(geometry, raster)
    RETURNS bool
    AS 'select $1 && $2::geometry'
    LANGUAGE 'sql' IMMUTABLE STRICT;
    
------------------------------------------------------------------------------
--  GiST index OPERATORs
------------------------------------------------------------------------------
-- raster/raster operators
CREATE OPERATOR << (
    LEFTARG = raster, RIGHTARG = raster, PROCEDURE = raster_left,
    COMMUTATOR = '>>',
    RESTRICT = positionsel, JOIN = positionjoinsel
    );

CREATE OPERATOR &< (
    LEFTARG = raster, RIGHTARG = raster, PROCEDURE = raster_overleft,
    COMMUTATOR = '&>',
    RESTRICT = positionsel, JOIN = positionjoinsel
    );

CREATE OPERATOR <<| (
    LEFTARG = raster, RIGHTARG = raster, PROCEDURE = raster_below,
    COMMUTATOR = '|>>',
    RESTRICT = positionsel, JOIN = positionjoinsel
    );

CREATE OPERATOR &<| (
    LEFTARG = raster, RIGHTARG = raster, PROCEDURE = raster_overbelow,
    COMMUTATOR = '|&>',
    RESTRICT = positionsel, JOIN = positionjoinsel
    );

CREATE OPERATOR && (
    LEFTARG = raster, RIGHTARG = raster, PROCEDURE = raster_overlap,
    COMMUTATOR = '&&',
    RESTRICT = contsel, JOIN = contjoinsel
    );

CREATE OPERATOR &> (
    LEFTARG = raster, RIGHTARG = raster, PROCEDURE = raster_overright,
    COMMUTATOR = '&<',
    RESTRICT = positionsel, JOIN = positionjoinsel
    );

CREATE OPERATOR >> (
    LEFTARG = raster, RIGHTARG = raster, PROCEDURE = raster_right,
    COMMUTATOR = '<<',
    RESTRICT = positionsel, JOIN = positionjoinsel
    );

CREATE OPERATOR |&> (
    LEFTARG = raster, RIGHTARG = raster, PROCEDURE = raster_overabove,
    COMMUTATOR = '&<|',
    RESTRICT = positionsel, JOIN = positionjoinsel
    );

CREATE OPERATOR |>> (
    LEFTARG = raster, RIGHTARG = raster, PROCEDURE = raster_above,
    COMMUTATOR = '<<|',
    RESTRICT = positionsel, JOIN = positionjoinsel
    );

CREATE OPERATOR ~= (
    LEFTARG = raster, RIGHTARG = raster, PROCEDURE = raster_same,
    COMMUTATOR = '~=',
    RESTRICT = eqsel, JOIN = eqjoinsel
    );

CREATE OPERATOR @ (
    LEFTARG = raster, RIGHTARG = raster, PROCEDURE = raster_contained,
    COMMUTATOR = '~',
    RESTRICT = contsel, JOIN = contjoinsel
    );

CREATE OPERATOR ~ (
    LEFTARG = raster, RIGHTARG = raster, PROCEDURE = raster_contain,
    COMMUTATOR = '@',
    RESTRICT = contsel, JOIN = contjoinsel
    );

-- raster/geometry operators
CREATE OPERATOR ~ (
    LEFTARG = raster, RIGHTARG = geometry, PROCEDURE = raster_geometry_contain,
    COMMUTATOR = '@',
    RESTRICT = contsel, JOIN = contjoinsel
    );
    
CREATE OPERATOR && (
    LEFTARG = raster, RIGHTARG = geometry, PROCEDURE = raster_geometry_overlap,
    COMMUTATOR = '&&',
    RESTRICT = contsel, JOIN = contjoinsel
    );

-- geometry/raster operators
CREATE OPERATOR ~ (
    LEFTARG = geometry, RIGHTARG = raster, PROCEDURE = geometry_raster_contain,
    COMMUTATOR = '@',
    RESTRICT = contsel, JOIN = contjoinsel
    );
    
CREATE OPERATOR && (
    LEFTARG = geometry, RIGHTARG = raster, PROCEDURE = geometry_raster_overlap,
    COMMUTATOR = '&&',
    RESTRICT = contsel, JOIN = contjoinsel
    );

-----------------------------------------------------------------------
-- Raster/Raster Spatial Relationship
-----------------------------------------------------------------------

-----------------------------------------------------------------------
-- ST_SameAlignment
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION st_samealignment(rast1 raster, rast2 raster)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'RASTER_sameAlignment'
	LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_samealignment(
	ulx1 double precision, uly1 double precision, scalex1 double precision, scaley1 double precision, skewx1 double precision, skewy1 double precision,
	ulx2 double precision, uly2 double precision, scalex2 double precision, scaley2 double precision, skewx2 double precision, skewy2 double precision
)
	RETURNS boolean
	AS $$ SELECT st_samealignment(st_makeemptyraster(1, 1, $1, $2, $3, $4, $5, $6), st_makeemptyraster(1, 1, $7, $8, $9, $10, $11, $12)) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE TYPE agg_samealignment AS (
	refraster raster,
	aligned boolean
);

CREATE OR REPLACE FUNCTION _st_samealignment_transfn(agg agg_samealignment, rast raster)
	RETURNS agg_samealignment AS $$
	DECLARE
		m record;
		aligned boolean;
	BEGIN
		IF agg IS NULL THEN
			agg.refraster := NULL;
			agg.aligned := NULL;
		END IF;

		IF rast IS NULL THEN
			agg.aligned := NULL;
		ELSE
			IF agg.refraster IS NULL THEN
				m := ST_Metadata(rast);
				agg.refraster := ST_MakeEmptyRaster(1, 1, m.upperleftx, m.upperlefty, m.scalex, m.scaley, m.skewx, m.skewy, m.srid);
				agg.aligned := TRUE;
			ELSE IF agg.aligned IS TRUE THEN
					agg.aligned := ST_SameAlignment(agg.refraster, rast);
				END IF;
			END IF;
		END IF;
		RETURN agg;
	END;
	$$ LANGUAGE 'plpgsql' IMMUTABLE;

CREATE OR REPLACE FUNCTION _st_samealignment_finalfn(agg agg_samealignment)
	RETURNS boolean
	AS $$ SELECT $1.aligned $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE AGGREGATE st_samealignment(raster) (
	SFUNC = _st_samealignment_transfn,
	STYPE = agg_samealignment,
	FINALFUNC = _st_samealignment_finalfn
);

-----------------------------------------------------------------------
-- ST_Intersects(raster, raster)
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION _st_intersects(rast1 raster, nband1 integer, rast2 raster, nband2 integer)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'RASTER_intersects'
	LANGUAGE 'c' IMMUTABLE STRICT
	COST 1000;

CREATE OR REPLACE FUNCTION st_intersects(rast1 raster, nband1 integer, rast2 raster, nband2 integer)
	RETURNS boolean
	AS $$ SELECT $1 && $3 AND CASE WHEN $2 IS NULL OR $4 IS NULL THEN _st_intersects(st_convexhull($1), st_convexhull($3)) ELSE _st_intersects($1, $2, $3, $4) END $$
	LANGUAGE 'sql' IMMUTABLE
	COST 1000;

CREATE OR REPLACE FUNCTION st_intersects(rast1 raster, rast2 raster)
	RETURNS boolean
	AS $$ SELECT st_intersects($1, NULL::integer, $2, NULL::integer) $$
	LANGUAGE 'sql' IMMUTABLE
	COST 1000;

-----------------------------------------------------------------------
-- ST_Intersects(geometry, raster)
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
			SELECT CASE WHEN bmd.nodatavalue IS NULL THEN FALSE ELSE NULL END INTO hasnodata FROM ST_BandMetaData(rast, nband) AS bmd;
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
						bintersect := NOT st_isempty(st_intersection(st_pixelaspolygon(rast, x, y), geom));

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
	$$ LANGUAGE 'plpgsql' IMMUTABLE
	COST 1000;

-- This function can not be STRICT
CREATE OR REPLACE FUNCTION st_intersects(geom geometry, rast raster, nband integer DEFAULT NULL)
	RETURNS boolean AS
	$$ SELECT $1 && $2::geometry AND _st_intersects($1, $2, $3); $$
	LANGUAGE 'sql' IMMUTABLE
	COST 1000;

-----------------------------------------------------------------------
-- ST_Intersects(raster, geometry)
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION st_intersects(rast raster, geom geometry, nband integer DEFAULT NULL)
	RETURNS boolean
	AS $$ SELECT $1::geometry && $2 AND _st_intersects($2, $1, $3) $$
	LANGUAGE 'sql' IMMUTABLE
	COST 1000;

CREATE OR REPLACE FUNCTION st_intersects(rast raster, nband integer, geom geometry)
	RETURNS boolean
	AS $$ SELECT $1::geometry && $3 AND _st_intersects($3, $1, $2) $$
	LANGUAGE 'sql' IMMUTABLE
	COST 1000;

-----------------------------------------------------------------------
-- ST_Overlaps(raster, raster)
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION _st_overlaps(rast1 raster, nband1 integer, rast2 raster, nband2 integer)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'RASTER_overlaps'
	LANGUAGE 'c' IMMUTABLE STRICT
	COST 1000;

CREATE OR REPLACE FUNCTION st_overlaps(rast1 raster, nband1 integer, rast2 raster, nband2 integer)
	RETURNS boolean
	AS $$ SELECT $1 && $3 AND CASE WHEN $2 IS NULL OR $4 IS NULL THEN _st_overlaps(st_convexhull($1), st_convexhull($3)) ELSE _st_overlaps($1, $2, $3, $4) END $$
	LANGUAGE 'sql' IMMUTABLE
	COST 1000;

CREATE OR REPLACE FUNCTION st_overlaps(rast1 raster, rast2 raster)
	RETURNS boolean
	AS $$ SELECT st_overlaps($1, NULL::integer, $2, NULL::integer) $$
	LANGUAGE 'sql' IMMUTABLE
	COST 1000;

-----------------------------------------------------------------------
-- ST_Touches(raster, raster)
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION _st_touches(rast1 raster, nband1 integer, rast2 raster, nband2 integer)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'RASTER_touches'
	LANGUAGE 'c' IMMUTABLE STRICT
	COST 1000;

CREATE OR REPLACE FUNCTION st_touches(rast1 raster, nband1 integer, rast2 raster, nband2 integer)
	RETURNS boolean
	AS $$ SELECT $1 && $3 AND CASE WHEN $2 IS NULL OR $4 IS NULL THEN _st_touches(st_convexhull($1), st_convexhull($3)) ELSE _st_touches($1, $2, $3, $4) END $$
	LANGUAGE 'sql' IMMUTABLE
	COST 1000;

CREATE OR REPLACE FUNCTION st_touches(rast1 raster, rast2 raster)
	RETURNS boolean
	AS $$ SELECT st_touches($1, NULL::integer, $2, NULL::integer) $$
	LANGUAGE 'sql' IMMUTABLE
	COST 1000;

-----------------------------------------------------------------------
-- ST_Contains(raster, raster)
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION _st_contains(rast1 raster, nband1 integer, rast2 raster, nband2 integer)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'RASTER_contains'
	LANGUAGE 'c' IMMUTABLE STRICT
	COST 1000;

CREATE OR REPLACE FUNCTION st_contains(rast1 raster, nband1 integer, rast2 raster, nband2 integer)
	RETURNS boolean
	AS $$ SELECT $1 && $3 AND CASE WHEN $2 IS NULL OR $4 IS NULL THEN _st_contains(st_convexhull($1), st_convexhull($3)) ELSE _st_contains($1, $2, $3, $4) END $$
	LANGUAGE 'sql' IMMUTABLE
	COST 1000;

CREATE OR REPLACE FUNCTION st_contains(rast1 raster, rast2 raster)
	RETURNS boolean
	AS $$ SELECT st_contains($1, NULL::integer, $2, NULL::integer) $$
	LANGUAGE 'sql' IMMUTABLE
	COST 1000;

-----------------------------------------------------------------------
-- ST_ContainsProperly(raster, raster)
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION _st_containsproperly(rast1 raster, nband1 integer, rast2 raster, nband2 integer)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'RASTER_containsProperly'
	LANGUAGE 'c' IMMUTABLE STRICT
	COST 1000;

CREATE OR REPLACE FUNCTION st_containsproperly(rast1 raster, nband1 integer, rast2 raster, nband2 integer)
	RETURNS boolean
	AS $$ SELECT $1 && $3 AND CASE WHEN $2 IS NULL OR $4 IS NULL THEN _st_containsproperly(st_convexhull($1), st_convexhull($3)) ELSE _st_containsproperly($1, $2, $3, $4) END $$
	LANGUAGE 'sql' IMMUTABLE
	COST 1000;

CREATE OR REPLACE FUNCTION st_containsproperly(rast1 raster, rast2 raster)
	RETURNS boolean
	AS $$ SELECT st_containsproperly($1, NULL::integer, $2, NULL::integer) $$
	LANGUAGE 'sql' IMMUTABLE
	COST 1000;

-----------------------------------------------------------------------
-- ST_Covers(raster, raster)
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION _st_covers(rast1 raster, nband1 integer, rast2 raster, nband2 integer)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'RASTER_covers'
	LANGUAGE 'c' IMMUTABLE STRICT
	COST 1000;

CREATE OR REPLACE FUNCTION st_covers(rast1 raster, nband1 integer, rast2 raster, nband2 integer)
	RETURNS boolean
	AS $$ SELECT $1 && $3 AND CASE WHEN $2 IS NULL OR $4 IS NULL THEN _st_covers(st_convexhull($1), st_convexhull($3)) ELSE _st_covers($1, $2, $3, $4) END $$
	LANGUAGE 'sql' IMMUTABLE
	COST 1000;

CREATE OR REPLACE FUNCTION st_covers(rast1 raster, rast2 raster)
	RETURNS boolean
	AS $$ SELECT st_covers($1, NULL::integer, $2, NULL::integer) $$
	LANGUAGE 'sql' IMMUTABLE
	COST 1000;

-----------------------------------------------------------------------
-- ST_CoveredBy(raster, raster)
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION _st_coveredby(rast1 raster, nband1 integer, rast2 raster, nband2 integer)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'RASTER_coveredby'
	LANGUAGE 'c' IMMUTABLE STRICT
	COST 1000;

CREATE OR REPLACE FUNCTION st_coveredby(rast1 raster, nband1 integer, rast2 raster, nband2 integer)
	RETURNS boolean
	AS $$ SELECT $1 && $3 AND CASE WHEN $2 IS NULL OR $4 IS NULL THEN _st_coveredby(st_convexhull($1), st_convexhull($3)) ELSE _st_coveredby($1, $2, $3, $4) END $$
	LANGUAGE 'sql' IMMUTABLE
	COST 1000;

CREATE OR REPLACE FUNCTION st_coveredby(rast1 raster, rast2 raster)
	RETURNS boolean
	AS $$ SELECT st_coveredby($1, NULL::integer, $2, NULL::integer) $$
	LANGUAGE 'sql' IMMUTABLE
	COST 1000;

-----------------------------------------------------------------------
-- ST_Within(raster, raster)
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION _st_within(rast1 raster, nband1 integer, rast2 raster, nband2 integer)
	RETURNS boolean
	AS $$ SELECT _st_contains($3, $4, $1, $2) $$
	LANGUAGE 'sql' IMMUTABLE
	COST 1000;

CREATE OR REPLACE FUNCTION st_within(rast1 raster, nband1 integer, rast2 raster, nband2 integer)
	RETURNS boolean
	AS $$ SELECT $1 && $3 AND CASE WHEN $2 IS NULL OR $4 IS NULL THEN _st_within(st_convexhull($1), st_convexhull($3)) ELSE _st_contains($3, $4, $1, $2) END $$
	LANGUAGE 'sql' IMMUTABLE
	COST 1000;

CREATE OR REPLACE FUNCTION st_within(rast1 raster, rast2 raster)
	RETURNS boolean
	AS $$ SELECT st_within($1, NULL::integer, $2, NULL::integer) $$
	LANGUAGE 'sql' IMMUTABLE
	COST 1000;

-----------------------------------------------------------------------
-- ST_DWithin(raster, raster)
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION _st_dwithin(rast1 raster, nband1 integer, rast2 raster, nband2 integer, distance double precision)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'RASTER_dwithin'
	LANGUAGE 'c' IMMUTABLE STRICT
	COST 1000;

CREATE OR REPLACE FUNCTION st_dwithin(rast1 raster, nband1 integer, rast2 raster, nband2 integer, distance double precision)
	RETURNS boolean
	AS $$ SELECT $1::geometry && ST_Expand(ST_ConvexHull($3), $5) AND $3::geometry && ST_Expand(ST_ConvexHull($1), $5) AND CASE WHEN $2 IS NULL OR $4 IS NULL THEN _st_dwithin(st_convexhull($1), st_convexhull($3), $5) ELSE _st_dwithin($1, $2, $3, $4, $5) END $$
	LANGUAGE 'sql' IMMUTABLE
	COST 1000;

CREATE OR REPLACE FUNCTION st_dwithin(rast1 raster, rast2 raster, distance double precision)
	RETURNS boolean
	AS $$ SELECT st_dwithin($1, NULL::integer, $2, NULL::integer, $3) $$
	LANGUAGE 'sql' IMMUTABLE
	COST 1000;

-----------------------------------------------------------------------
-- ST_DFullyWithin(raster, raster)
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION _st_dfullywithin(rast1 raster, nband1 integer, rast2 raster, nband2 integer, distance double precision)
	RETURNS boolean
	AS 'MODULE_PATHNAME', 'RASTER_dfullywithin'
	LANGUAGE 'c' IMMUTABLE STRICT
	COST 1000;

CREATE OR REPLACE FUNCTION st_dfullywithin(rast1 raster, nband1 integer, rast2 raster, nband2 integer, distance double precision)
	RETURNS boolean
	AS $$ SELECT $1::geometry && ST_Expand(ST_ConvexHull($3), $5) AND $3::geometry && ST_Expand(ST_ConvexHull($1), $5) AND CASE WHEN $2 IS NULL OR $4 IS NULL THEN _st_dfullywithin(st_convexhull($1), st_convexhull($3), $5) ELSE _st_dfullywithin($1, $2, $3, $4, $5) END $$
	LANGUAGE 'sql' IMMUTABLE
	COST 1000;

CREATE OR REPLACE FUNCTION st_dfullywithin(rast1 raster, rast2 raster, distance double precision)
	RETURNS boolean
	AS $$ SELECT st_dfullywithin($1, NULL::integer, $2, NULL::integer, $3) $$
	LANGUAGE 'sql' IMMUTABLE
	COST 1000;

-----------------------------------------------------------------------
-- ST_Disjoint(raster, raster)
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION st_disjoint(rast1 raster, nband1 integer, rast2 raster, nband2 integer)
	RETURNS boolean
	AS $$ SELECT CASE WHEN $2 IS NULL OR $4 IS NULL THEN st_disjoint(st_convexhull($1), st_convexhull($3)) ELSE NOT _st_intersects($1, $2, $3, $4) END $$
	LANGUAGE 'sql' IMMUTABLE
	COST 1000;

CREATE OR REPLACE FUNCTION st_disjoint(rast1 raster, rast2 raster)
	RETURNS boolean
	AS $$ SELECT st_disjoint($1, NULL::integer, $2, NULL::integer) $$
	LANGUAGE 'sql' IMMUTABLE
	COST 1000;

-----------------------------------------------------------------------
-- ST_Intersection(geometry, raster) in geometry-space
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

CREATE OR REPLACE FUNCTION st_intersection(rast raster, band integer, geomin geometry)
	RETURNS SETOF geomval AS
	$$ SELECT st_intersection($3, $1, $2) $$
	LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION st_intersection(rast raster, geomin geometry)
	RETURNS SETOF geomval AS
	$$ SELECT st_intersection($2, $1, 1) $$
	LANGUAGE 'sql' STABLE;

-----------------------------------------------------------------------
-- ST_Intersection(raster, raster)
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_intersection(
	rast1 raster, band1 int,
	rast2 raster, band2 int,
	returnband text DEFAULT 'BOTH',
	nodataval double precision[] DEFAULT NULL
)
	RETURNS raster
	AS $$
	DECLARE
		rtn raster;
		_returnband text;
		newnodata1 float8;
		newnodata2 float8;
	BEGIN
		newnodata1 := coalesce(nodataval[1], ST_BandNodataValue(rast1, band1), ST_MinPossibleValue(ST_BandPixelType(rast1, band1)));
		newnodata2 := coalesce(nodataval[2], ST_BandNodataValue(rast2, band2), ST_MinPossibleValue(ST_BandPixelType(rast2, band2)));
		
		_returnband := upper(returnband);

		rtn := NULL;
		CASE
			WHEN _returnband = 'BAND1' THEN
				rtn := ST_MapAlgebraExpr(rast1, band1, rast2, band2, '[rast1.val]', ST_BandPixelType(rast1, band1), 'INTERSECTION', newnodata1::text, newnodata1::text, newnodata1);
				rtn := ST_SetBandNodataValue(rtn, 1, newnodata1);
			WHEN _returnband = 'BAND2' THEN
				rtn := ST_MapAlgebraExpr(rast1, band1, rast2, band2, '[rast2.val]', ST_BandPixelType(rast2, band2), 'INTERSECTION', newnodata2::text, newnodata2::text, newnodata2);
				rtn := ST_SetBandNodataValue(rtn, 1, newnodata2);
			WHEN _returnband = 'BOTH' THEN
				rtn := ST_MapAlgebraExpr(rast1, band1, rast2, band2, '[rast1.val]', ST_BandPixelType(rast1, band1), 'INTERSECTION', newnodata1::text, newnodata1::text, newnodata1);
				rtn := ST_SetBandNodataValue(rtn, 1, newnodata1);
				rtn := ST_AddBand(rtn, ST_MapAlgebraExpr(rast1, band1, rast2, band2, '[rast2.val]', ST_BandPixelType(rast2, band2), 'INTERSECTION', newnodata2::text, newnodata2::text, newnodata2));
				rtn := ST_SetBandNodataValue(rtn, 2, newnodata2);
			ELSE
				RAISE EXCEPTION 'Unknown value provided for returnband: %', returnband;
				RETURN NULL;
		END CASE;

		RETURN rtn;
	END;
	$$ LANGUAGE 'plpgsql' STABLE;

CREATE OR REPLACE FUNCTION st_intersection(
	rast1 raster, band1 int,
	rast2 raster, band2 int,
	returnband text,
	nodataval double precision
)
	RETURNS raster AS
	$$ SELECT st_intersection($1, $2, $3, $4, $5, ARRAY[$6, $6]) $$
	LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION st_intersection(
	rast1 raster, band1 int,
	rast2 raster, band2 int,
	nodataval double precision[]
)
	RETURNS raster AS
	$$ SELECT st_intersection($1, $2, $3, $4, 'BOTH', $5) $$
	LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION st_intersection(
	rast1 raster, band1 int,
	rast2 raster, band2 int,
	nodataval double precision
)
	RETURNS raster AS
	$$ SELECT st_intersection($1, $2, $3, $4, 'BOTH', ARRAY[$5, $5]) $$
	LANGUAGE 'sql' STABLE;

-- Variants without band number
CREATE OR REPLACE FUNCTION st_intersection(
	rast1 raster,
	rast2 raster,
	returnband text DEFAULT 'BOTH',
	nodataval double precision[] DEFAULT NULL
)
	RETURNS raster AS
	$$ SELECT st_intersection($1, 1, $2, 1, $3, $4) $$
	LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION st_intersection(
	rast1 raster,
	rast2 raster,
	returnband text,
	nodataval double precision
)
	RETURNS raster AS
	$$ SELECT st_intersection($1, 1, $2, 1, $3, ARRAY[$4, $4]) $$
	LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION st_intersection(
	rast1 raster,
	rast2 raster,
	nodataval double precision[]
)
	RETURNS raster AS
	$$ SELECT st_intersection($1, 1, $2, 1, 'BOTH', $3) $$
	LANGUAGE 'sql' STABLE;

CREATE OR REPLACE FUNCTION st_intersection(
	rast1 raster,
	rast2 raster,
	nodataval double precision
)
	RETURNS raster AS
	$$ SELECT st_intersection($1, 1, $2, 1, 'BOTH', ARRAY[$3, $3]) $$
	LANGUAGE 'sql' STABLE;

-----------------------------------------------------------------------
-- st_union aggregate
-----------------------------------------------------------------------
-- Main state function
CREATE OR REPLACE FUNCTION _ST_MapAlgebra4UnionState(rast1 raster,  rast2 raster, p_expression text, p_nodata1expr text, p_nodata2expr text, p_nodatanodataval double precision,t_expression text,t_nodata1expr text, t_nodata2expr text,t_nodatanodataval double precision)
    RETURNS raster AS
    $$
    DECLARE
        t_raster raster;
        p_raster raster;
    BEGIN
        -- With the new ST_MapAlgebraExpr we must split the main expression in three expressions: expression, nodata1expr, nodata2expr and a nodatanodataval
        -- ST_MapAlgebraExpr(rast1 raster, band1 integer, rast2 raster, band2 integer, expression text, pixeltype text, extentexpr text, nodata1expr text, nodata2expr text, nodatanodatadaval double precision)
        -- We must make sure that when NULL is passed as the first raster to ST_MapAlgebraExpr, ST_MapAlgebraExpr resolve the nodata1expr
        -- Note: rast2 is always a single band raster since it is the accumulated raster thus far
        -- 		There we always set that to band 1 regardless of what band num is requested
        IF upper(p_expression) = 'LAST' THEN
            --RAISE NOTICE 'last asked for ';
            RETURN ST_MapAlgebraExpr(rast1, 1, rast2, 1, '[rast2.val]'::text, NULL::text, 'UNION'::text, '[rast2.val]'::text, '[rast1.val]'::text, NULL::double precision);
        ELSIF upper(p_expression) = 'FIRST' THEN
            RETURN ST_MapAlgebraExpr(rast1, 1, rast2, 1, '[rast1.val]'::text, NULL::text, 'UNION'::text, '[rast2.val]'::text, '[rast1.val]'::text, NULL::double precision);
        ELSIF upper(p_expression) = 'MIN' THEN
            RETURN ST_MapAlgebraExpr(rast1, 1, rast2, 1, 'LEAST([rast1.val], [rast2.val])'::text, NULL::text, 'UNION'::text, '[rast2.val]'::text, '[rast1.val]'::text, NULL::double precision);
        ELSIF upper(p_expression) = 'MAX' THEN
            RETURN ST_MapAlgebraExpr(rast1, 1, rast2, 1, 'GREATEST([rast1.val], [rast2.val])'::text, NULL::text, 'UNION'::text, '[rast2.val]'::text, '[rast1.val]'::text, NULL::double precision);
        ELSIF upper(p_expression) = 'COUNT' THEN
            RETURN ST_MapAlgebraExpr(rast1, 1, rast2, 1, '[rast1.val] + 1'::text, NULL::text, 'UNION'::text, '1'::text, '[rast1.val]'::text, 0::double precision);
        ELSIF upper(p_expression) = 'SUM' THEN
            RETURN ST_MapAlgebraExpr(rast1, 1, rast2, 1, '[rast1.val] + [rast2.val]'::text, NULL::text, 'UNION'::text, '[rast2.val]'::text, '[rast1.val]'::text, NULL::double precision);
        ELSIF upper(p_expression) = 'RANGE' THEN
        -- have no idea what this is 
            t_raster = ST_MapAlgebraExpr(rast1, 2, rast2, 1, 'LEAST([rast1.val], [rast2.val])'::text, NULL::text, 'UNION'::text, '[rast2.val]'::text, '[rast1.val]'::text, NULL::double precision);
            p_raster := _ST_MapAlgebra4UnionState(rast1, rast2, 'MAX'::text, NULL::text, NULL::text, NULL::double precision, NULL::text, NULL::text, NULL::text, NULL::double precision);
            RETURN ST_AddBand(p_raster, t_raster, 1, 2);
        ELSIF upper(p_expression) = 'MEAN' THEN
        -- looks like t_raster is used to keep track of accumulated count
        -- and p_raster is there to keep track of accumulated sum and final state function
        -- would then do a final map to divide them.  This one is currently broken because 
        	-- have not reworked it so it can do without a final function
            t_raster = ST_MapAlgebraExpr(rast1, 2, rast2, 1, '[rast1.val] + 1'::text, NULL::text, 'UNION'::text, '1'::text, '[rast1.val]'::text, 0::double precision);
            p_raster := _ST_MapAlgebra4UnionState(rast1, rast2, 'SUM'::text, NULL::text, NULL::text, NULL::double precision, NULL::text, NULL::text, NULL::text, NULL::double precision);
            RETURN ST_AddBand(p_raster, t_raster, 1, 2);
        ELSE
            IF t_expression NOTNULL AND t_expression != '' THEN
                t_raster = ST_MapAlgebraExpr(rast1, 2, rast2, 1, t_expression, NULL::text, 'UNION'::text, t_nodata1expr, t_nodata2expr, t_nodatanodataval::double precision);
                p_raster = ST_MapAlgebraExpr(rast1, 1, rast2, 1, p_expression, NULL::text, 'UNION'::text, p_nodata1expr, p_nodata2expr, p_nodatanodataval::double precision);
                RETURN ST_AddBand(p_raster, t_raster, 1, 2);
            END IF;
            RETURN ST_MapAlgebraExpr(rast1, 1, rast2, 1, p_expression, NULL, 'UNION'::text, NULL::text, NULL::text, NULL::double precision);
        END IF;
    END;
    $$
    LANGUAGE 'plpgsql';

-- State function when there is a primary expression, band number and no alternative nodata expressions
CREATE OR REPLACE FUNCTION _ST_MapAlgebra4UnionState(rast1 raster,rast2 raster,bandnum integer, p_expression text)
    RETURNS raster
    AS $$
        SELECT _ST_MapAlgebra4UnionState($1, ST_Band($2,$3), $4, NULL, NULL, NULL, NULL, NULL, NULL, NULL)
    $$ LANGUAGE 'sql';

-- State function when there is no expressions but allows specifying band
CREATE OR REPLACE FUNCTION _ST_MapAlgebra4UnionState(rast1 raster,rast2 raster, bandnum integer)
    RETURNS raster
    AS $$
        SELECT _ST_MapAlgebra4UnionState($1,ST_Band($2,$3), 'LAST', NULL, NULL, NULL, NULL, NULL, NULL, NULL)
    $$ LANGUAGE 'sql';
    
-- State function when there is no expressions and assumes band 1
CREATE OR REPLACE FUNCTION _ST_MapAlgebra4UnionState(rast1 raster,rast2 raster)
    RETURNS raster
    AS $$
        SELECT _ST_MapAlgebra4UnionState($1,$2, 'LAST', NULL, NULL, NULL, NULL, NULL, NULL, NULL)
    $$ LANGUAGE 'sql';
    
-- State function when there isan expressions and assumes band 1
CREATE OR REPLACE FUNCTION _ST_MapAlgebra4UnionState(rast1 raster,rast2 raster, p_expression text)
    RETURNS raster
    AS $$
        SELECT _ST_MapAlgebra4UnionState($1,$2, $3, NULL, NULL, NULL, NULL, NULL, NULL, NULL)
    $$ LANGUAGE 'sql';
    
-- Final function with only the primary expression
CREATE OR REPLACE FUNCTION _ST_MapAlgebra4UnionFinal1(rast raster)
    RETURNS raster AS
    $$
    DECLARE
    BEGIN
    	-- NOTE: I have to sacrifice RANGE.  Sorry RANGE.  Any 2 banded raster is going to be treated
    	-- as a MEAN
        IF ST_NumBands(rast) = 2 THEN
            RETURN ST_MapAlgebraExpr(rast, 1, rast, 2, 'CASE WHEN [rast2.val] > 0 THEN [rast1.val] / [rast2.val]::float8 ELSE NULL END'::text, NULL::text, 'UNION'::text, NULL::text, NULL::text, NULL::double precision);
        ELSE
            RETURN rast;
        END IF;
    END;
    $$
    LANGUAGE 'plpgsql';
    
-- Variant with primary expression defaulting to 'LAST' and working on first band
CREATE AGGREGATE ST_Union(raster) (
    SFUNC = _ST_MapAlgebra4UnionState,
    STYPE = raster,
    FINALFUNC = _ST_MapAlgebra4UnionFinal1
);

-- Variant with primary expression defaulting to 'LAST' and working on specified band
CREATE AGGREGATE ST_Union(raster, integer) (
    SFUNC = _ST_MapAlgebra4UnionState,
    STYPE = raster,
    FINALFUNC = _ST_MapAlgebra4UnionFinal1
);

-- Variant with simple primary expressions but without alternative nodata, temporary and final expressions
-- and working on first band
-- supports LAST, MIN,MAX,MEAN,FIRST,SUM
CREATE AGGREGATE ST_Union(raster, text) (
    SFUNC = _ST_MapAlgebra4UnionState,
    STYPE = raster,
    FINALFUNC = _ST_MapAlgebra4UnionFinal1
);

CREATE AGGREGATE ST_Union(raster, integer, text) (
    SFUNC = _ST_MapAlgebra4UnionState,
    STYPE = raster,
    FINALFUNC = _ST_MapAlgebra4UnionFinal1
);

-------------------------------------------------------------------
-- ST_Clip(rast raster, band int, geom geometry, nodata float8 DEFAULT null, crop boolean DEFAULT true)
-- Clip the values of a raster to the shape of a polygon.
--
-- rast   - raster to be clipped
-- band   - limit the result to only one band
-- geom   - geometry defining the shape to clip the raster
-- nodata - define (if there is none defined) or replace the raster nodata value with this value
-- crop   - limit the extent of the result to the extent of the geometry
-----------------------------------------------------------------------
-- ST_Clip
-----------------------------------------------------------------------
-- nodataval as array series

-- Major variant
CREATE OR REPLACE FUNCTION st_clip(rast raster, band int, geom geometry, nodataval double precision[] DEFAULT NULL, crop boolean DEFAULT TRUE)
	RETURNS raster
	AS $$
	DECLARE
		newrast raster;
		geomrast raster;
		numband int;
		bandstart int;
		bandend int;
		newextent text;
		newnodataval double precision;
		newpixtype text;
		bandi int;
	BEGIN
		IF rast IS NULL THEN
			RETURN NULL;
		END IF;
		IF geom IS NULL THEN
			RETURN rast;
		END IF;
		numband := ST_Numbands(rast);
		IF band IS NULL THEN
			bandstart := 1;
			bandend := numband;
		ELSEIF ST_HasNoBand(rast, band) THEN
			RAISE NOTICE 'Raster do not have band %. Returning null', band;
			RETURN NULL;
		ELSE
			bandstart := band;
			bandend := band;
		END IF;

		newpixtype := ST_BandPixelType(rast, bandstart);
		newnodataval := coalesce(nodataval[1], ST_BandNodataValue(rast, bandstart), ST_MinPossibleValue(newpixtype));
		newextent := CASE WHEN crop THEN 'INTERSECTION' ELSE 'FIRST' END;

		-- Convert the geometry to a raster
		geomrast := ST_AsRaster(geom, rast, ST_BandPixelType(rast, band), 1, newnodataval);

		-- Compute the first raster band
		newrast := ST_MapAlgebraExpr(rast, bandstart, geomrast, 1, '[rast1.val]', newpixtype, newextent, newnodataval::text, newnodataval::text, newnodataval);
		-- Set the newnodataval
		newrast := ST_SetBandNodataValue(newrast, bandstart, newnodataval);

		FOR bandi IN bandstart+1..bandend LOOP
			-- for each band we must determine the nodata value
			newpixtype := ST_BandPixelType(rast, bandi);
			newnodataval := coalesce(nodataval[bandi], nodataval[array_upper(nodataval, 1)], ST_BandNodataValue(rast, bandi), ST_MinPossibleValue(newpixtype));
			newrast := ST_AddBand(newrast, ST_MapAlgebraExpr(rast, bandi, geomrast, 1, '[rast1.val]', newpixtype, newextent, newnodataval::text, newnodataval::text, newnodataval));
			newrast := ST_SetBandNodataValue(newrast, bandi, newnodataval);
		END LOOP;

		RETURN newrast;
	END;
	$$ LANGUAGE 'plpgsql' STABLE;

-- Nodata values as integer series
CREATE OR REPLACE FUNCTION st_clip(rast raster, band int, geom geometry, nodataval double precision, crop boolean DEFAULT TRUE)
	RETURNS raster AS
	$$ SELECT ST_Clip($1, $2, $3, ARRAY[$4], $5) $$
	LANGUAGE 'sql' STABLE;

-- Variant defaulting nodataval to the one of the raster or the min possible value
CREATE OR REPLACE FUNCTION st_clip(rast raster, band int, geom geometry, crop boolean)
	RETURNS raster AS
	$$ SELECT ST_Clip($1, $2, $3, null::float8[], $4) $$
	LANGUAGE 'sql' STABLE;

-- Variant defaulting to all bands
CREATE OR REPLACE FUNCTION st_clip(rast raster, geom geometry, nodataval double precision[] DEFAULT NULL, crop boolean DEFAULT TRUE)
	RETURNS raster AS
	$$ SELECT ST_Clip($1, NULL, $2, $3, $4) $$
	LANGUAGE 'sql' STABLE;

-- Variant defaulting to all bands
CREATE OR REPLACE FUNCTION st_clip(rast raster, geom geometry, nodataval double precision, crop boolean DEFAULT TRUE)
	RETURNS raster AS
	$$ SELECT ST_Clip($1, NULL, $2, ARRAY[$3], $4) $$
	LANGUAGE 'sql' STABLE;

-- Variant defaulting nodataval to the one of the raster or the min possible value and returning all bands
CREATE OR REPLACE FUNCTION st_clip(rast raster, geom geometry, crop boolean)
	RETURNS raster AS
	$$ SELECT ST_Clip($1, NULL, $2, null::float8[], $3) $$
	LANGUAGE 'sql' STABLE;

-----------------------------------------------------------------------
-- ST_NearestValue
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION st_nearestvalue(
	rast raster, band integer,
	pt geometry,
	exclude_nodata_value boolean DEFAULT TRUE
)
	RETURNS double precision
	AS 'MODULE_PATHNAME', 'RASTER_nearestValue'
	LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_nearestvalue(
	rast raster,
	pt geometry,
	exclude_nodata_value boolean DEFAULT TRUE
)
	RETURNS double precision
	AS $$ SELECT st_nearestvalue($1, 1, $2, $3) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_nearestvalue(
	rast raster, band integer,
	columnx integer, rowy integer,
	exclude_nodata_value boolean DEFAULT TRUE
)
	RETURNS double precision
	AS $$ SELECT st_nearestvalue($1, $2, st_setsrid(st_makepoint(st_raster2worldcoordx($1, $3, $4), st_raster2worldcoordy($1, $3, $4)), st_srid($1)), $5) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_nearestvalue(
	rast raster,
	columnx integer, rowy integer,
	exclude_nodata_value boolean DEFAULT TRUE
)
	RETURNS double precision
	AS $$ SELECT st_nearestvalue($1, 1, st_setsrid(st_makepoint(st_raster2worldcoordx($1, $2, $3), st_raster2worldcoordy($1, $2, $3)), st_srid($1)), $4) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- ST_Neighborhood
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION st_neighborhood(
	rast raster, band integer,
	columnx integer, rowy integer,
	distance integer,
	exclude_nodata_value boolean DEFAULT TRUE
)
	RETURNS double precision[][]
	AS 'MODULE_PATHNAME', 'RASTER_neighborhood'
	LANGUAGE 'c' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_neighborhood(
	rast raster,
	columnx integer, rowy integer,
	distance integer,
	exclude_nodata_value boolean DEFAULT TRUE
)
	RETURNS double precision[][]
	AS $$ SELECT st_neighborhood($1, 1, $2, $3, $4, $5) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_neighborhood(
	rast raster, band integer,
	pt geometry,
	distance integer,
	exclude_nodata_value boolean DEFAULT TRUE
)
	RETURNS double precision[][]
	AS $$
	DECLARE
		wx int;
		wy int;
		rtn double precision[][];
	BEGIN
		IF (st_geometrytype($3) != 'ST_Point') THEN
			RAISE EXCEPTION 'Attempting to get the neighbor of a pixel with a non-point geometry';
		END IF;
		wx := st_x($3);
		wy := st_y($3);

		SELECT st_neighborhood(
			$1, $2,
			st_world2rastercoordx(rast, wx, wy),
			st_world2rastercoordy(rast, wx, wy),
			$4,
			$5
		) INTO rtn;
		RETURN rtn;
	END;
	$$ LANGUAGE 'plpgsql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_neighborhood(
	rast raster,
	pt geometry,
	distance integer,
	exclude_nodata_value boolean DEFAULT TRUE
)
	RETURNS double precision[][]
	AS $$ SELECT st_neighborhood($1, 1, $2, $3, $4) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

------------------------------------------------------------------------------
-- raster constraint functions
-------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION _add_raster_constraint(cn name, sql text)
	RETURNS boolean AS $$
	BEGIN
		BEGIN
			EXECUTE sql;
		EXCEPTION
			WHEN duplicate_object THEN
				RAISE NOTICE 'The constraint "%" already exists.  To replace the existing constraint, delete the constraint and call ApplyRasterConstraints again', cn;
			WHEN OTHERS THEN
				RAISE NOTICE 'Unable to add constraint "%"', cn;
				RETURN FALSE;
		END;

		RETURN TRUE;
	END;
	$$ LANGUAGE 'plpgsql' VOLATILE STRICT
	COST 100;

CREATE OR REPLACE FUNCTION _drop_raster_constraint(rastschema name, rasttable name, cn name)
	RETURNS boolean AS $$
	DECLARE
		fqtn text;
	BEGIN
		fqtn := '';
		IF length($1) > 0 THEN
			fqtn := quote_ident($1) || '.';
		END IF;
		fqtn := fqtn || quote_ident($2);

		BEGIN
			EXECUTE 'ALTER TABLE '
				|| fqtn
				|| ' DROP CONSTRAINT '
				|| quote_ident(cn);
			RETURN TRUE;
		EXCEPTION
			WHEN undefined_object THEN
				RAISE NOTICE 'The constraint "%" does not exist.  Skipping', cn;
			WHEN OTHERS THEN
				RAISE NOTICE 'Unable to drop constraint "%"', cn;
				RETURN FALSE;
		END;

		RETURN TRUE;
	END;
	$$ LANGUAGE 'plpgsql' VOLATILE STRICT
	COST 100;

CREATE OR REPLACE FUNCTION _raster_constraint_info_srid(rastschema name, rasttable name, rastcolumn name)
	RETURNS integer AS $$
	SELECT
		replace(replace(split_part(s.consrc, ' = ', 2), ')', ''), '(', '')::integer
	FROM pg_class c, pg_namespace n, pg_attribute a, pg_constraint s
	WHERE n.nspname = $1
		AND c.relname = $2
		AND a.attname = $3
		AND a.attrelid = c.oid
		AND s.connamespace = n.oid
		AND s.conrelid = c.oid
		AND a.attnum = ANY (s.conkey)
		AND s.consrc LIKE '%st_srid(% = %';
	$$ LANGUAGE sql STABLE STRICT
  COST 100;

CREATE OR REPLACE FUNCTION _add_raster_constraint_srid(rastschema name, rasttable name, rastcolumn name)
	RETURNS boolean AS $$
	DECLARE
		fqtn text;
		cn name;
		sql text;
		attr int;
	BEGIN
		fqtn := '';
		IF length($1) > 0 THEN
			fqtn := quote_ident($1) || '.';
		END IF;
		fqtn := fqtn || quote_ident($2);

		cn := 'enforce_srid_' || $3;

		sql := 'SELECT st_srid('
			|| quote_ident($3)
			|| ') FROM ' || fqtn
			|| ' LIMIT 1';
		BEGIN
			EXECUTE sql INTO attr;
		EXCEPTION WHEN OTHERS THEN
			RAISE NOTICE 'Unable to get the SRID of a sample raster';
			RETURN FALSE;
		END;

		sql := 'ALTER TABLE ' || fqtn
			|| ' ADD CONSTRAINT ' || quote_ident(cn)
			|| ' CHECK (st_srid('
			|| quote_ident($3)
			|| ') = ' || attr || ')';

		RETURN _add_raster_constraint(cn, sql);
	END;
	$$ LANGUAGE 'plpgsql' VOLATILE STRICT
	COST 100;

CREATE OR REPLACE FUNCTION _drop_raster_constraint_srid(rastschema name, rasttable name, rastcolumn name)
	RETURNS boolean AS
	$$ SELECT _drop_raster_constraint($1, $2, 'enforce_srid_' || $3) $$
	LANGUAGE 'sql' VOLATILE STRICT
	COST 100;

CREATE OR REPLACE FUNCTION _raster_constraint_info_scale(rastschema name, rasttable name, rastcolumn name, axis char)
	RETURNS double precision AS $$
	SELECT
		replace(replace(split_part(split_part(s.consrc, ' = ', 2), '::', 1), ')', ''), '(', '')::double precision
	FROM pg_class c, pg_namespace n, pg_attribute a, pg_constraint s
	WHERE n.nspname = $1
		AND c.relname = $2
		AND a.attname = $3
		AND a.attrelid = c.oid
		AND s.connamespace = n.oid
		AND s.conrelid = c.oid
		AND a.attnum = ANY (s.conkey)
		AND s.consrc LIKE '%st_scale' || $4 || '(% = %';
	$$ LANGUAGE sql STABLE STRICT
  COST 100;

CREATE OR REPLACE FUNCTION _add_raster_constraint_scale(rastschema name, rasttable name, rastcolumn name, axis char)
	RETURNS boolean AS $$
	DECLARE
		fqtn text;
		cn name;
		sql text;
		attr double precision;
	BEGIN
		IF lower($4) != 'x' AND lower($4) != 'y' THEN
			RAISE EXCEPTION 'axis must be either "x" or "y"';
			RETURN FALSE;
		END IF;

		fqtn := '';
		IF length($1) > 0 THEN
			fqtn := quote_ident($1) || '.';
		END IF;
		fqtn := fqtn || quote_ident($2);

		cn := 'enforce_scale' || $4 || '_' || $3;

		sql := 'SELECT st_scale' || $4 || '('
			|| quote_ident($3)
			|| ') FROM '
			|| fqtn
			|| ' LIMIT 1';
		BEGIN
			EXECUTE sql INTO attr;
		EXCEPTION WHEN OTHERS THEN
			RAISE NOTICE 'Unable to get the %-scale of a sample raster', upper($4);
			RETURN FALSE;
		END;

		sql := 'ALTER TABLE ' || fqtn
			|| ' ADD CONSTRAINT ' || quote_ident(cn)
			|| ' CHECK (st_scale' || $4 || '('
			|| quote_ident($3)
			|| ')::numeric(16,10) = (' || attr || ')::numeric(16,10))';
		RETURN _add_raster_constraint(cn, sql);
	END;
	$$ LANGUAGE 'plpgsql' VOLATILE STRICT
	COST 100;

CREATE OR REPLACE FUNCTION _drop_raster_constraint_scale(rastschema name, rasttable name, rastcolumn name, axis char)
	RETURNS boolean AS $$
	BEGIN
		IF lower($4) != 'x' AND lower($4) != 'y' THEN
			RAISE EXCEPTION 'axis must be either "x" or "y"';
			RETURN FALSE;
		END IF;

		RETURN _drop_raster_constraint($1, $2, 'enforce_scale' || $4 || '_' || $3);
	END;
	$$ LANGUAGE 'plpgsql' VOLATILE STRICT
	COST 100;

CREATE OR REPLACE FUNCTION _raster_constraint_info_blocksize(rastschema name, rasttable name, rastcolumn name, axis text)
	RETURNS integer AS $$
	SELECT
		replace(replace(split_part(s.consrc, ' = ', 2), ')', ''), '(', '')::integer
	FROM pg_class c, pg_namespace n, pg_attribute a, pg_constraint s
	WHERE n.nspname = $1
		AND c.relname = $2
		AND a.attname = $3
		AND a.attrelid = c.oid
		AND s.connamespace = n.oid
		AND s.conrelid = c.oid
		AND a.attnum = ANY (s.conkey)
		AND s.consrc LIKE '%st_' || $4 || '(% = %';
	$$ LANGUAGE sql STABLE STRICT
  COST 100;

CREATE OR REPLACE FUNCTION _add_raster_constraint_blocksize(rastschema name, rasttable name, rastcolumn name, axis text)
	RETURNS boolean AS $$
	DECLARE
		fqtn text;
		cn name;
		sql text;
		attr int;
	BEGIN
		IF lower($4) != 'width' AND lower($4) != 'height' THEN
			RAISE EXCEPTION 'axis must be either "width" or "height"';
			RETURN FALSE;
		END IF;

		fqtn := '';
		IF length($1) > 0 THEN
			fqtn := quote_ident($1) || '.';
		END IF;
		fqtn := fqtn || quote_ident($2);

		cn := 'enforce_' || $4 || '_' || $3;

		sql := 'SELECT st_' || $4 || '('
			|| quote_ident($3)
			|| ') FROM ' || fqtn
			|| ' LIMIT 1';
		BEGIN
			EXECUTE sql INTO attr;
		EXCEPTION WHEN OTHERS THEN
			RAISE NOTICE 'Unable to get the % of a sample raster', $4;
			RETURN FALSE;
		END;

		sql := 'ALTER TABLE ' || fqtn
			|| ' ADD CONSTRAINT ' || quote_ident(cn)
			|| ' CHECK (st_' || $4 || '('
			|| quote_ident($3)
			|| ') = ' || attr || ')';
		RETURN _add_raster_constraint(cn, sql);
	END;
	$$ LANGUAGE 'plpgsql' VOLATILE STRICT
	COST 100;

CREATE OR REPLACE FUNCTION _drop_raster_constraint_blocksize(rastschema name, rasttable name, rastcolumn name, axis text)
	RETURNS boolean AS $$
	BEGIN
		IF lower($4) != 'width' AND lower($4) != 'height' THEN
			RAISE EXCEPTION 'axis must be either "width" or "height"';
			RETURN FALSE;
		END IF;

		RETURN _drop_raster_constraint($1, $2, 'enforce_' || $4 || '_' || $3);
	END;
	$$ LANGUAGE 'plpgsql' VOLATILE STRICT
	COST 100;

CREATE OR REPLACE FUNCTION _raster_constraint_info_extent(rastschema name, rasttable name, rastcolumn name)
	RETURNS geometry AS $$
	SELECT
		trim(both '''' from split_part(trim(split_part(s.consrc, ',', 2)), '::', 1))::geometry
	FROM pg_class c, pg_namespace n, pg_attribute a, pg_constraint s
	WHERE n.nspname = $1
		AND c.relname = $2
		AND a.attname = $3
		AND a.attrelid = c.oid
		AND s.connamespace = n.oid
		AND s.conrelid = c.oid
		AND a.attnum = ANY (s.conkey)
		AND s.consrc LIKE '%st_coveredby(st_convexhull(%';
	$$ LANGUAGE sql STABLE STRICT
  COST 100;

CREATE OR REPLACE FUNCTION _add_raster_constraint_extent(rastschema name, rasttable name, rastcolumn name)
	RETURNS boolean AS $$
	DECLARE
		fqtn text;
		cn name;
		sql text;
		attr text;
	BEGIN
		fqtn := '';
		IF length($1) > 0 THEN
			fqtn := quote_ident($1) || '.';
		END IF;
		fqtn := fqtn || quote_ident($2);

		cn := 'enforce_max_extent_' || $3;

		sql := 'SELECT st_ashexewkb(st_convexhull(st_collect(st_convexhull('
			|| quote_ident($3)
			|| ')))) FROM '
			|| fqtn;
		BEGIN
			EXECUTE sql INTO attr;
		EXCEPTION WHEN OTHERS THEN
			RAISE NOTICE 'Unable to get the extent of a sample raster';
			RETURN FALSE;
		END;

		sql := 'ALTER TABLE ' || fqtn
			|| ' ADD CONSTRAINT ' || quote_ident(cn)
			|| ' CHECK (st_coveredby(st_convexhull('
			|| quote_ident($3)
			|| '), ''' || attr || '''::geometry))';
		RETURN _add_raster_constraint(cn, sql);
	END;
	$$ LANGUAGE 'plpgsql' VOLATILE STRICT
	COST 100;

CREATE OR REPLACE FUNCTION _drop_raster_constraint_extent(rastschema name, rasttable name, rastcolumn name)
	RETURNS boolean AS
	$$ SELECT _drop_raster_constraint($1, $2, 'enforce_max_extent_' || $3) $$
	LANGUAGE 'sql' VOLATILE STRICT
	COST 100;

CREATE OR REPLACE FUNCTION _raster_constraint_info_alignment(rastschema name, rasttable name, rastcolumn name)
	RETURNS boolean AS $$
	SELECT
		TRUE
	FROM pg_class c, pg_namespace n, pg_attribute a, pg_constraint s
	WHERE n.nspname = $1
		AND c.relname = $2
		AND a.attname = $3
		AND a.attrelid = c.oid
		AND s.connamespace = n.oid
		AND s.conrelid = c.oid
		AND a.attnum = ANY (s.conkey)
		AND s.consrc LIKE '%st_samealignment(%';
	$$ LANGUAGE sql STABLE STRICT
  COST 100;

CREATE OR REPLACE FUNCTION _add_raster_constraint_alignment(rastschema name, rasttable name, rastcolumn name)
	RETURNS boolean AS $$
	DECLARE
		fqtn text;
		cn name;
		sql text;
		attr text;
	BEGIN
		fqtn := '';
		IF length($1) > 0 THEN
			fqtn := quote_ident($1) || '.';
		END IF;
		fqtn := fqtn || quote_ident($2);

		cn := 'enforce_same_alignment_' || $3;

		sql := 'SELECT st_makeemptyraster(1, 1, upperleftx, upperlefty, scalex, scaley, skewx, skewy, srid) FROM st_metadata((SELECT '
			|| quote_ident($3)
			|| ' FROM ' || fqtn || ' LIMIT 1))';
		BEGIN
			EXECUTE sql INTO attr;
		EXCEPTION WHEN OTHERS THEN
			RAISE NOTICE 'Unable to get the alignment of a sample raster';
			RETURN FALSE;
		END;

		sql := 'ALTER TABLE ' || fqtn ||
			' ADD CONSTRAINT ' || quote_ident(cn) ||
			' CHECK (st_samealignment(' || quote_ident($3) || ', ''' || attr || '''::raster))';
		RETURN _add_raster_constraint(cn, sql);
	END;
	$$ LANGUAGE 'plpgsql' VOLATILE STRICT
	COST 100;

CREATE OR REPLACE FUNCTION _drop_raster_constraint_alignment(rastschema name, rasttable name, rastcolumn name)
	RETURNS boolean AS
	$$ SELECT _drop_raster_constraint($1, $2, 'enforce_same_alignment_' || $3) $$
	LANGUAGE 'sql' VOLATILE STRICT
	COST 100;

CREATE OR REPLACE FUNCTION _raster_constraint_info_regular_blocking(rastschema name, rasttable name, rastcolumn name)
	RETURNS boolean
	AS $$
	DECLARE
		cn text;
		sql text;
		rtn boolean;
	BEGIN
		cn := 'enforce_regular_blocking_' || $3;

		sql := 'SELECT TRUE FROM pg_class c, pg_namespace n, pg_constraint s'
			|| ' WHERE n.nspname = ' || quote_literal($1)
			|| ' AND c.relname = ' || quote_literal($2)
			|| ' AND s.connamespace = n.oid AND s.conrelid = c.oid'
			|| ' AND s.conname = ' || quote_literal(cn);
		EXECUTE sql INTO rtn;
		RETURN rtn;
	END;
	$$ LANGUAGE 'plpgsql' STABLE STRICT
  COST 100;

CREATE OR REPLACE FUNCTION _add_raster_constraint_regular_blocking(rastschema name, rasttable name, rastcolumn name)
	RETURNS boolean AS $$
	DECLARE
		fqtn text;
		cn name;
		sql text;
	BEGIN

		RAISE INFO 'The regular_blocking constraint is just a flag indicating that the column "%" is regularly blocked.  It is up to the end-user to ensure that the column is truely regularly blocked.', quote_ident($3);

		fqtn := '';
		IF length($1) > 0 THEN
			fqtn := quote_ident($1) || '.';
		END IF;
		fqtn := fqtn || quote_ident($2);

		cn := 'enforce_regular_blocking_' || $3;

		sql := 'ALTER TABLE ' || fqtn
			|| ' ADD CONSTRAINT ' || quote_ident(cn)
			|| ' CHECK (TRUE)';
		RETURN _add_raster_constraint(cn, sql);
	END;
	$$ LANGUAGE 'plpgsql' VOLATILE STRICT
	COST 100;

CREATE OR REPLACE FUNCTION _drop_raster_constraint_regular_blocking(rastschema name, rasttable name, rastcolumn name)
	RETURNS boolean AS
	$$ SELECT _drop_raster_constraint($1, $2, 'enforce_regular_blocking_' || $3) $$
	LANGUAGE 'sql' VOLATILE STRICT
	COST 100;

CREATE OR REPLACE FUNCTION _raster_constraint_info_num_bands(rastschema name, rasttable name, rastcolumn name)
	RETURNS integer AS $$
	SELECT
		replace(replace(split_part(s.consrc, ' = ', 2), ')', ''), '(', '')::integer
	FROM pg_class c, pg_namespace n, pg_attribute a, pg_constraint s
	WHERE n.nspname = $1
		AND c.relname = $2
		AND a.attname = $3
		AND a.attrelid = c.oid
		AND s.connamespace = n.oid
		AND s.conrelid = c.oid
		AND a.attnum = ANY (s.conkey)
		AND s.consrc LIKE '%st_numbands(%';
	$$ LANGUAGE sql STABLE STRICT
  COST 100;

CREATE OR REPLACE FUNCTION _add_raster_constraint_num_bands(rastschema name, rasttable name, rastcolumn name)
	RETURNS boolean AS $$
	DECLARE
		fqtn text;
		cn name;
		sql text;
		attr int;
	BEGIN
		fqtn := '';
		IF length($1) > 0 THEN
			fqtn := quote_ident($1) || '.';
		END IF;
		fqtn := fqtn || quote_ident($2);

		cn := 'enforce_num_bands_' || $3;

		sql := 'SELECT st_numbands(' || quote_ident($3)
			|| ') FROM ' || fqtn
			|| ' LIMIT 1';
		BEGIN
			EXECUTE sql INTO attr;
		EXCEPTION WHEN OTHERS THEN
			RAISE NOTICE 'Unable to get the number of bands of a sample raster';
			RETURN FALSE;
		END;

		sql := 'ALTER TABLE ' || fqtn
			|| ' ADD CONSTRAINT ' || quote_ident(cn)
			|| ' CHECK (st_numbands(' || quote_ident($3)
			|| ') = ' || attr
			|| ')';
		RETURN _add_raster_constraint(cn, sql);
	END;
	$$ LANGUAGE 'plpgsql' VOLATILE STRICT
	COST 100;

CREATE OR REPLACE FUNCTION _drop_raster_constraint_num_bands(rastschema name, rasttable name, rastcolumn name)
	RETURNS boolean AS
	$$ SELECT _drop_raster_constraint($1, $2, 'enforce_num_bands_' || $3) $$
	LANGUAGE 'sql' VOLATILE STRICT
	COST 100;

CREATE OR REPLACE FUNCTION _raster_constraint_info_pixel_types(rastschema name, rasttable name, rastcolumn name)
	RETURNS text[] AS $$
	SELECT
		trim(both '''' from split_part(replace(replace(split_part(s.consrc, ' = ', 2), ')', ''), '(', ''), '::', 1))::text[]
	FROM pg_class c, pg_namespace n, pg_attribute a, pg_constraint s
	WHERE n.nspname = $1
		AND c.relname = $2
		AND a.attname = $3
		AND a.attrelid = c.oid
		AND s.connamespace = n.oid
		AND s.conrelid = c.oid
		AND a.attnum = ANY (s.conkey)
		AND s.consrc LIKE '%_raster_constraint_pixel_types(%';
	$$ LANGUAGE sql STABLE STRICT
  COST 100;

CREATE OR REPLACE FUNCTION _raster_constraint_pixel_types(rast raster)
	RETURNS text[] AS
	$$ SELECT array_agg(pixeltype)::text[] FROM st_bandmetadata($1, ARRAY[]::int[]); $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION _add_raster_constraint_pixel_types(rastschema name, rasttable name, rastcolumn name)
	RETURNS boolean AS $$
	DECLARE
		fqtn text;
		cn name;
		sql text;
		attr text[];
		max int;
	BEGIN
		fqtn := '';
		IF length($1) > 0 THEN
			fqtn := quote_ident($1) || '.';
		END IF;
		fqtn := fqtn || quote_ident($2);

		cn := 'enforce_pixel_types_' || $3;

		sql := 'SELECT _raster_constraint_pixel_types(' || quote_ident($3)
			|| ') FROM ' || fqtn
			|| ' LIMIT 1';
		BEGIN
			EXECUTE sql INTO attr;
		EXCEPTION WHEN OTHERS THEN
			RAISE NOTICE 'Unable to get the pixel types of a sample raster';
			RETURN FALSE;
		END;
		max := array_length(attr, 1);
		IF max < 1 OR max IS NULL THEN
			RAISE NOTICE 'Unable to get the pixel types of a sample raster';
			RETURN FALSE;
		END IF;

		sql := 'ALTER TABLE ' || fqtn
			|| ' ADD CONSTRAINT ' || quote_ident(cn)
			|| ' CHECK (_raster_constraint_pixel_types(' || quote_ident($3)
			|| ') = ''{';
		FOR x in 1..max LOOP
			sql := sql || '"' || attr[x] || '"';
			IF x < max THEN
				sql := sql || ',';
			END IF;
		END LOOP;
		sql := sql || '}''::text[])';

		RETURN _add_raster_constraint(cn, sql);
	END;
	$$ LANGUAGE 'plpgsql' VOLATILE STRICT
	COST 100;

CREATE OR REPLACE FUNCTION _drop_raster_constraint_pixel_types(rastschema name, rasttable name, rastcolumn name)
	RETURNS boolean AS
	$$ SELECT _drop_raster_constraint($1, $2, 'enforce_pixel_types_' || $3) $$
	LANGUAGE 'sql' VOLATILE STRICT
	COST 100;

CREATE OR REPLACE FUNCTION _raster_constraint_info_nodata_values(rastschema name, rasttable name, rastcolumn name)
	RETURNS double precision[] AS $$
	SELECT
		trim(both '''' from split_part(replace(replace(split_part(s.consrc, ' = ', 2), ')', ''), '(', ''), '::', 1))::double precision[]
	FROM pg_class c, pg_namespace n, pg_attribute a, pg_constraint s
	WHERE n.nspname = $1
		AND c.relname = $2
		AND a.attname = $3
		AND a.attrelid = c.oid
		AND s.connamespace = n.oid
		AND s.conrelid = c.oid
		AND a.attnum = ANY (s.conkey)
		AND s.consrc LIKE '%_raster_constraint_nodata_values(%';
	$$ LANGUAGE sql STABLE STRICT
  COST 100;

CREATE OR REPLACE FUNCTION _raster_constraint_nodata_values(rast raster)
	RETURNS double precision[] AS
	$$ SELECT array_agg(nodatavalue)::double precision[] FROM st_bandmetadata($1, ARRAY[]::int[]); $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION _add_raster_constraint_nodata_values(rastschema name, rasttable name, rastcolumn name)
	RETURNS boolean AS $$
	DECLARE
		fqtn text;
		cn name;
		sql text;
		attr double precision[];
		max int;
	BEGIN
		fqtn := '';
		IF length($1) > 0 THEN
			fqtn := quote_ident($1) || '.';
		END IF;
		fqtn := fqtn || quote_ident($2);

		cn := 'enforce_nodata_values_' || $3;

		sql := 'SELECT _raster_constraint_nodata_values(' || quote_ident($3)
			|| ') FROM ' || fqtn
			|| ' LIMIT 1';
		BEGIN
			EXECUTE sql INTO attr;
		EXCEPTION WHEN OTHERS THEN
			RAISE NOTICE 'Unable to get the nodata values of a sample raster';
			RETURN FALSE;
		END;
		max := array_length(attr, 1);
		IF max < 1 OR max IS NULL THEN
			RAISE NOTICE 'Unable to get the nodata values of a sample raster';
			RETURN FALSE;
		END IF;

		sql := 'ALTER TABLE ' || fqtn
			|| ' ADD CONSTRAINT ' || quote_ident(cn)
			|| ' CHECK (_raster_constraint_nodata_values(' || quote_ident($3)
			|| ')::numeric(16,10)[] = ''{';
		FOR x in 1..max LOOP
			IF attr[x] IS NULL THEN
				sql := sql || 'NULL';
			ELSE
				sql := sql || attr[x];
			END IF;
			IF x < max THEN
				sql := sql || ',';
			END IF;
		END LOOP;
		sql := sql || '}''::numeric(16,10)[])';

		RETURN _add_raster_constraint(cn, sql);
	END;
	$$ LANGUAGE 'plpgsql' VOLATILE STRICT
	COST 100;

CREATE OR REPLACE FUNCTION _drop_raster_constraint_nodata_values(rastschema name, rasttable name, rastcolumn name)
	RETURNS boolean AS
	$$ SELECT _drop_raster_constraint($1, $2, 'enforce_nodata_values_' || $3) $$
	LANGUAGE 'sql' VOLATILE STRICT
	COST 100;

CREATE OR REPLACE FUNCTION _raster_constraint_info_out_db(rastschema name, rasttable name, rastcolumn name)
	RETURNS boolean[] AS $$
	SELECT
		trim(both '''' from split_part(replace(replace(split_part(s.consrc, ' = ', 2), ')', ''), '(', ''), '::', 1))::boolean[]
	FROM pg_class c, pg_namespace n, pg_attribute a, pg_constraint s
	WHERE n.nspname = $1
		AND c.relname = $2
		AND a.attname = $3
		AND a.attrelid = c.oid
		AND s.connamespace = n.oid
		AND s.conrelid = c.oid
		AND a.attnum = ANY (s.conkey)
		AND s.consrc LIKE '%_raster_constraint_out_db(%';
	$$ LANGUAGE sql STABLE STRICT
  COST 100;

CREATE OR REPLACE FUNCTION _raster_constraint_out_db(rast raster)
	RETURNS boolean[] AS
	$$ SELECT array_agg(isoutdb)::boolean[] FROM st_bandmetadata($1, ARRAY[]::int[]); $$
	LANGUAGE 'sql' STABLE STRICT;

CREATE OR REPLACE FUNCTION _add_raster_constraint_out_db(rastschema name, rasttable name, rastcolumn name)
	RETURNS boolean AS $$
	DECLARE
		fqtn text;
		cn name;
		sql text;
		attr boolean[];
		max int;
	BEGIN
		fqtn := '';
		IF length($1) > 0 THEN
			fqtn := quote_ident($1) || '.';
		END IF;
		fqtn := fqtn || quote_ident($2);

		cn := 'enforce_out_db_' || $3;

		sql := 'SELECT _raster_constraint_out_db(' || quote_ident($3)
			|| ') FROM ' || fqtn
			|| ' LIMIT 1';
		BEGIN
			EXECUTE sql INTO attr;
		EXCEPTION WHEN OTHERS THEN
			RAISE NOTICE 'Unable to get the out-of-database bands of a sample raster';
			RETURN FALSE;
		END;
		max := array_length(attr, 1);
		IF max < 1 OR max IS NULL THEN
			RAISE NOTICE 'Unable to get the out-of-database bands of a sample raster';
			RETURN FALSE;
		END IF;

		sql := 'ALTER TABLE ' || fqtn
			|| ' ADD CONSTRAINT ' || quote_ident(cn)
			|| ' CHECK (_raster_constraint_out_db(' || quote_ident($3)
			|| ') = ''{';
		FOR x in 1..max LOOP
			IF attr[x] IS FALSE THEN
				sql := sql || 'FALSE';
			ELSE
				sql := sql || 'TRUE';
			END IF;
			IF x < max THEN
				sql := sql || ',';
			END IF;
		END LOOP;
		sql := sql || '}''::boolean[])';

		RETURN _add_raster_constraint(cn, sql);
	END;
	$$ LANGUAGE 'plpgsql' VOLATILE STRICT
	COST 100;

CREATE OR REPLACE FUNCTION _drop_raster_constraint_out_db(rastschema name, rasttable name, rastcolumn name)
	RETURNS boolean AS
	$$ SELECT _drop_raster_constraint($1, $2, 'enforce_out_db_' || $3) $$
	LANGUAGE 'sql' VOLATILE STRICT
	COST 100;

------------------------------------------------------------------------------
-- AddRasterConstraints
------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION AddRasterConstraints (
	rastschema name,
	rasttable name,
	rastcolumn name,
	VARIADIC constraints text[]
)
	RETURNS boolean
	AS $$
	DECLARE
		max int;
		cnt int;
		sql text;
		schema name;
		x int;
		kw text;
		rtn boolean;
	BEGIN
		cnt := 0;
		max := array_length(constraints, 1);
		IF max < 1 THEN
			RAISE NOTICE 'No constraints indicated to be added.  Doing nothing';
			RETURN TRUE;
		END IF;

		-- validate schema
		schema := NULL;
		IF length($1) > 0 THEN
			sql := 'SELECT nspname FROM pg_namespace '
				|| 'WHERE nspname = ' || quote_literal($1)
				|| 'LIMIT 1';
			EXECUTE sql INTO schema;

			IF schema IS NULL THEN
				RAISE EXCEPTION 'The value provided for schema is invalid';
				RETURN FALSE;
			END IF;
		END IF;

		IF schema IS NULL THEN
			sql := 'SELECT n.nspname AS schemaname '
				|| 'FROM pg_catalog.pg_class c '
				|| 'JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace '
				|| 'WHERE c.relkind = ' || quote_literal('r')
				|| ' AND n.nspname NOT IN (' || quote_literal('pg_catalog')
				|| ', ' || quote_literal('pg_toast')
				|| ') AND pg_catalog.pg_table_is_visible(c.oid)'
				|| ' AND c.relname = ' || quote_literal($2);
			EXECUTE sql INTO schema;

			IF schema IS NULL THEN
				RAISE EXCEPTION 'The table % does not occur in the search_path', quote_literal($2);
				RETURN FALSE;
			END IF;
		END IF;

		<<kwloop>>
		FOR x in 1..max LOOP
			kw := trim(both from lower(constraints[x]));

			BEGIN
				CASE
					WHEN kw = 'srid' THEN
						RAISE NOTICE 'Adding SRID constraint';
						rtn := _add_raster_constraint_srid(schema, $2, $3);
					WHEN kw IN ('scale_x', 'scalex') THEN
						RAISE NOTICE 'Adding scale-X constraint';
						rtn := _add_raster_constraint_scale(schema, $2, $3, 'x');
					WHEN kw IN ('scale_y', 'scaley') THEN
						RAISE NOTICE 'Adding scale-Y constraint';
						rtn := _add_raster_constraint_scale(schema, $2, $3, 'y');
					WHEN kw = 'scale' THEN
						RAISE NOTICE 'Adding scale-X constraint';
						rtn := _add_raster_constraint_scale(schema, $2, $3, 'x');
						RAISE NOTICE 'Adding scale-Y constraint';
						rtn := _add_raster_constraint_scale(schema, $2, $3, 'y');
					WHEN kw IN ('blocksize_x', 'blocksizex', 'width') THEN
						RAISE NOTICE 'Adding blocksize-X constraint';
						rtn := _add_raster_constraint_blocksize(schema, $2, $3, 'width');
					WHEN kw IN ('blocksize_y', 'blocksizey', 'height') THEN
						RAISE NOTICE 'Adding blocksize-Y constraint';
						rtn := _add_raster_constraint_blocksize(schema, $2, $3, 'height');
					WHEN kw = 'blocksize' THEN
						RAISE NOTICE 'Adding blocksize-X constraint';
						rtn := _add_raster_constraint_blocksize(schema, $2, $3, 'width');
						RAISE NOTICE 'Adding blocksize-Y constraint';
						rtn := _add_raster_constraint_blocksize(schema, $2, $3, 'height');
					WHEN kw IN ('same_alignment', 'samealignment', 'alignment') THEN
						RAISE NOTICE 'Adding alignment constraint';
						rtn := _add_raster_constraint_alignment(schema, $2, $3);
					WHEN kw IN ('regular_blocking', 'regularblocking') THEN
						RAISE NOTICE 'Adding regular blocking constraint';
						rtn := _add_raster_constraint_regular_blocking(schema, $2, $3);
					WHEN kw IN ('num_bands', 'numbands') THEN
						RAISE NOTICE 'Adding number of bands constraint';
						rtn := _add_raster_constraint_num_bands(schema, $2, $3);
					WHEN kw IN ('pixel_types', 'pixeltypes') THEN
						RAISE NOTICE 'Adding pixel type constraint';
						rtn := _add_raster_constraint_pixel_types(schema, $2, $3);
					WHEN kw IN ('nodata_values', 'nodatavalues', 'nodata') THEN
						RAISE NOTICE 'Adding nodata value constraint';
						rtn := _add_raster_constraint_nodata_values(schema, $2, $3);
					WHEN kw IN ('out_db', 'outdb') THEN
						RAISE NOTICE 'Adding out-of-database constraint';
						rtn := _add_raster_constraint_out_db(schema, $2, $3);
					WHEN kw = 'extent' THEN
						RAISE NOTICE 'Adding maximum extent constraint';
						rtn := _add_raster_constraint_extent(schema, $2, $3);
					ELSE
						RAISE NOTICE 'Unknown constraint: %.  Skipping', quote_literal(constraints[x]);
						CONTINUE kwloop;
				END CASE;
			END;

			IF rtn IS FALSE THEN
				cnt := cnt + 1;
				RAISE WARNING 'Unable to add constraint: %.  Skipping', quote_literal(constraints[x]);
			END IF;

		END LOOP kwloop;

		IF cnt = max THEN
			RAISE EXCEPTION 'None of the constraints specified could be added.  Is the schema name, table name or column name incorrect?';
			RETURN FALSE;
		END IF;

		RETURN TRUE;
	END;
	$$ LANGUAGE 'plpgsql' VOLATILE STRICT
	COST 100;

CREATE OR REPLACE FUNCTION AddRasterConstraints (
	rasttable name,
	rastcolumn name,
	VARIADIC constraints text[]
)
	RETURNS boolean AS
	$$ SELECT AddRasterConstraints('', $1, $2, VARIADIC $3) $$
	LANGUAGE 'sql' VOLATILE STRICT
	COST 100;

CREATE OR REPLACE FUNCTION AddRasterConstraints (
	rastschema name,
	rasttable name,
	rastcolumn name,
	srid boolean DEFAULT TRUE,
	scale_x boolean DEFAULT TRUE,
	scale_y boolean DEFAULT TRUE,
	blocksize_x boolean DEFAULT TRUE,
	blocksize_y boolean DEFAULT TRUE,
	same_alignment boolean DEFAULT TRUE,
	regular_blocking boolean DEFAULT FALSE, -- false as regular_blocking is not a usable constraint
	num_bands boolean DEFAULT TRUE,
	pixel_types boolean DEFAULT TRUE,
	nodata_values boolean DEFAULT TRUE,
	out_db boolean DEFAULT TRUE,
	extent boolean DEFAULT TRUE
)
	RETURNS boolean
	AS $$
	DECLARE
		constraints text[];
	BEGIN
		IF srid IS TRUE THEN
			constraints := constraints || 'srid'::text;
		END IF;

		IF scale_x IS TRUE THEN
			constraints := constraints || 'scale_x'::text;
		END IF;

		IF scale_y IS TRUE THEN
			constraints := constraints || 'scale_y'::text;
		END IF;

		IF blocksize_x IS TRUE THEN
			constraints := constraints || 'blocksize_x'::text;
		END IF;

		IF blocksize_y IS TRUE THEN
			constraints := constraints || 'blocksize_y'::text;
		END IF;

		IF same_alignment IS TRUE THEN
			constraints := constraints || 'same_alignment'::text;
		END IF;

		IF regular_blocking IS TRUE THEN
			constraints := constraints || 'regular_blocking'::text;
		END IF;

		IF num_bands IS TRUE THEN
			constraints := constraints || 'num_bands'::text;
		END IF;

		IF pixel_types IS TRUE THEN
			constraints := constraints || 'pixel_types'::text;
		END IF;

		IF nodata_values IS TRUE THEN
			constraints := constraints || 'nodata_values'::text;
		END IF;

		IF out_db IS TRUE THEN
			constraints := constraints || 'out_db'::text;
		END IF;

		IF extent IS TRUE THEN
			constraints := constraints || 'extent'::text;
		END IF;

		RETURN AddRasterConstraints($1, $2, $3, VARIADIC constraints);
	END;
	$$ LANGUAGE 'plpgsql' VOLATILE STRICT
	COST 100;

CREATE OR REPLACE FUNCTION AddRasterConstraints (
	rasttable name,
	rastcolumn name,
	srid boolean DEFAULT TRUE,
	scale_x boolean DEFAULT TRUE,
	scale_y boolean DEFAULT TRUE,
	blocksize_x boolean DEFAULT TRUE,
	blocksize_y boolean DEFAULT TRUE,
	same_alignment boolean DEFAULT TRUE,
	regular_blocking boolean DEFAULT FALSE, -- false as regular_blocking is not a usable constraint
	num_bands boolean DEFAULT TRUE,
	pixel_types boolean DEFAULT TRUE,
	nodata_values boolean DEFAULT TRUE,
	out_db boolean DEFAULT TRUE,
	extent boolean DEFAULT TRUE
)
	RETURNS boolean AS
	$$ SELECT AddRasterConstraints('', $1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14) $$
	LANGUAGE 'sql' VOLATILE STRICT
	COST 100;

------------------------------------------------------------------------------
-- DropRasterConstraints
------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION DropRasterConstraints (
	rastschema name,
	rasttable name,
	rastcolumn name,
	VARIADIC constraints text[]
)
	RETURNS boolean
	AS $$
	DECLARE
		max int;
		x int;
		schema name;
		sql text;
		kw text;
		rtn boolean;
		cnt int;
	BEGIN
		cnt := 0;
		max := array_length(constraints, 1);
		IF max < 1 THEN
			RAISE NOTICE 'No constraints indicated to be dropped.  Doing nothing';
			RETURN TRUE;
		END IF;

		-- validate schema
		schema := NULL;
		IF length($1) > 0 THEN
			sql := 'SELECT nspname FROM pg_namespace '
				|| 'WHERE nspname = ' || quote_literal($1)
				|| 'LIMIT 1';
			EXECUTE sql INTO schema;

			IF schema IS NULL THEN
				RAISE EXCEPTION 'The value provided for schema is invalid';
				RETURN FALSE;
			END IF;
		END IF;

		IF schema IS NULL THEN
			sql := 'SELECT n.nspname AS schemaname '
				|| 'FROM pg_catalog.pg_class c '
				|| 'JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace '
				|| 'WHERE c.relkind = ' || quote_literal('r')
				|| ' AND n.nspname NOT IN (' || quote_literal('pg_catalog')
				|| ', ' || quote_literal('pg_toast')
				|| ') AND pg_catalog.pg_table_is_visible(c.oid)'
				|| ' AND c.relname = ' || quote_literal($2);
			EXECUTE sql INTO schema;

			IF schema IS NULL THEN
				RAISE EXCEPTION 'The table % does not occur in the search_path', quote_literal($2);
				RETURN FALSE;
			END IF;
		END IF;

		<<kwloop>>
		FOR x in 1..max LOOP
			kw := trim(both from lower(constraints[x]));

			BEGIN
				CASE
					WHEN kw = 'srid' THEN
						RAISE NOTICE 'Dropping SRID constraint';
						rtn := _drop_raster_constraint_srid(schema, $2, $3);
					WHEN kw IN ('scale_x', 'scalex') THEN
						RAISE NOTICE 'Dropping scale-X constraint';
						rtn := _drop_raster_constraint_scale(schema, $2, $3, 'x');
					WHEN kw IN ('scale_y', 'scaley') THEN
						RAISE NOTICE 'Dropping scale-Y constraint';
						rtn := _drop_raster_constraint_scale(schema, $2, $3, 'y');
					WHEN kw = 'scale' THEN
						RAISE NOTICE 'Dropping scale-X constraint';
						rtn := _drop_raster_constraint_scale(schema, $2, $3, 'x');
						RAISE NOTICE 'Dropping scale-Y constraint';
						rtn := _drop_raster_constraint_scale(schema, $2, $3, 'y');
					WHEN kw IN ('blocksize_x', 'blocksizex', 'width') THEN
						RAISE NOTICE 'Dropping blocksize-X constraint';
						rtn := _drop_raster_constraint_blocksize(schema, $2, $3, 'width');
					WHEN kw IN ('blocksize_y', 'blocksizey', 'height') THEN
						RAISE NOTICE 'Dropping blocksize-Y constraint';
						rtn := _drop_raster_constraint_blocksize(schema, $2, $3, 'height');
					WHEN kw = 'blocksize' THEN
						RAISE NOTICE 'Dropping blocksize-X constraint';
						rtn := _drop_raster_constraint_blocksize(schema, $2, $3, 'width');
						RAISE NOTICE 'Dropping blocksize-Y constraint';
						rtn := _drop_raster_constraint_blocksize(schema, $2, $3, 'height');
					WHEN kw IN ('same_alignment', 'samealignment', 'alignment') THEN
						RAISE NOTICE 'Dropping alignment constraint';
						rtn := _drop_raster_constraint_alignment(schema, $2, $3);
					WHEN kw IN ('regular_blocking', 'regularblocking') THEN
						RAISE NOTICE 'Dropping regular blocking constraint';
						rtn := _drop_raster_constraint_regular_blocking(schema, $2, $3);
					WHEN kw IN ('num_bands', 'numbands') THEN
						RAISE NOTICE 'Dropping number of bands constraint';
						rtn := _drop_raster_constraint_num_bands(schema, $2, $3);
					WHEN kw IN ('pixel_types', 'pixeltypes') THEN
						RAISE NOTICE 'Dropping pixel type constraint';
						rtn := _drop_raster_constraint_pixel_types(schema, $2, $3);
					WHEN kw IN ('nodata_values', 'nodatavalues', 'nodata') THEN
						RAISE NOTICE 'Dropping nodata value constraint';
						rtn := _drop_raster_constraint_nodata_values(schema, $2, $3);
					WHEN kw IN ('out_db', 'outdb') THEN
						RAISE NOTICE 'Dropping out-of-database constraint';
						rtn := _drop_raster_constraint_out_db(schema, $2, $3);
					WHEN kw = 'extent' THEN
						RAISE NOTICE 'Dropping maximum extent constraint';
						rtn := _drop_raster_constraint_extent(schema, $2, $3);
					ELSE
						RAISE NOTICE 'Unknown constraint: %.  Skipping', quote_literal(constraints[x]);
						CONTINUE kwloop;
				END CASE;
			END;

			IF rtn IS FALSE THEN
				cnt := cnt + 1;
				RAISE WARNING 'Unable to drop constraint: %.  Skipping', quote_literal(constraints[x]);
			END IF;

		END LOOP kwloop;

		IF cnt = max THEN
			RAISE EXCEPTION 'None of the constraints specified could be dropped.  Is the schema name, table name or column name incorrect?';
			RETURN FALSE;
		END IF;

		RETURN TRUE;
	END;
	$$ LANGUAGE 'plpgsql' VOLATILE STRICT
	COST 100;

CREATE OR REPLACE FUNCTION DropRasterConstraints (
	rasttable name,
	rastcolumn name,
	VARIADIC constraints text[]
)
	RETURNS boolean AS
	$$ SELECT DropRasterConstraints('', $1, $2, VARIADIC $3) $$
	LANGUAGE 'sql' VOLATILE STRICT
	COST 100;

CREATE OR REPLACE FUNCTION DropRasterConstraints (
	rastschema name,
	rasttable name,
	rastcolumn name,
	srid boolean DEFAULT TRUE,
	scale_x boolean DEFAULT TRUE,
	scale_y boolean DEFAULT TRUE,
	blocksize_x boolean DEFAULT TRUE,
	blocksize_y boolean DEFAULT TRUE,
	same_alignment boolean DEFAULT TRUE,
	regular_blocking boolean DEFAULT TRUE,
	num_bands boolean DEFAULT TRUE,
	pixel_types boolean DEFAULT TRUE,
	nodata_values boolean DEFAULT TRUE,
	out_db boolean DEFAULT TRUE,
	extent boolean DEFAULT TRUE
)
	RETURNS boolean
	AS $$
	DECLARE
		constraints text[];
	BEGIN
		IF srid IS TRUE THEN
			constraints := constraints || 'srid'::text;
		END IF;

		IF scale_x IS TRUE THEN
			constraints := constraints || 'scale_x'::text;
		END IF;

		IF scale_y IS TRUE THEN
			constraints := constraints || 'scale_y'::text;
		END IF;

		IF blocksize_x IS TRUE THEN
			constraints := constraints || 'blocksize_x'::text;
		END IF;

		IF blocksize_y IS TRUE THEN
			constraints := constraints || 'blocksize_y'::text;
		END IF;

		IF same_alignment IS TRUE THEN
			constraints := constraints || 'same_alignment'::text;
		END IF;

		IF regular_blocking IS TRUE THEN
			constraints := constraints || 'regular_blocking'::text;
		END IF;

		IF num_bands IS TRUE THEN
			constraints := constraints || 'num_bands'::text;
		END IF;

		IF pixel_types IS TRUE THEN
			constraints := constraints || 'pixel_types'::text;
		END IF;

		IF nodata_values IS TRUE THEN
			constraints := constraints || 'nodata_values'::text;
		END IF;

		IF out_db IS TRUE THEN
			constraints := constraints || 'out_db'::text;
		END IF;

		IF extent IS TRUE THEN
			constraints := constraints || 'extent'::text;
		END IF;

		RETURN DropRasterConstraints($1, $2, $3, VARIADIC constraints);
	END;
	$$ LANGUAGE 'plpgsql' VOLATILE STRICT
	COST 100;

CREATE OR REPLACE FUNCTION DropRasterConstraints (
	rasttable name,
	rastcolumn name,
	srid boolean DEFAULT TRUE,
	scale_x boolean DEFAULT TRUE,
	scale_y boolean DEFAULT TRUE,
	blocksize_x boolean DEFAULT TRUE,
	blocksize_y boolean DEFAULT TRUE,
	same_alignment boolean DEFAULT TRUE,
	regular_blocking boolean DEFAULT TRUE,
	num_bands boolean DEFAULT TRUE,
	pixel_types boolean DEFAULT TRUE,
	nodata_values boolean DEFAULT TRUE,
	out_db boolean DEFAULT TRUE,
	extent boolean DEFAULT TRUE
)
	RETURNS boolean AS
	$$ SELECT DropRasterConstraints('', $1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14) $$
	LANGUAGE 'sql' VOLATILE STRICT
	COST 100;

------------------------------------------------------------------------------
-- raster_columns
--
-- The metadata is documented in the PostGIS Raster specification:
-- http://trac.osgeo.org/postgis/wiki/WKTRaster/SpecificationFinal01
------------------------------------------------------------------------------

CREATE OR REPLACE VIEW raster_columns AS
	SELECT
		current_database() AS r_table_catalog,
		n.nspname AS r_table_schema,
		c.relname AS r_table_name,
		a.attname AS r_raster_column,
		COALESCE(_raster_constraint_info_srid(n.nspname, c.relname, a.attname), (SELECT ST_SRID('POINT(0 0)'::geometry))) AS srid,
		_raster_constraint_info_scale(n.nspname, c.relname, a.attname, 'x') AS scale_x,
		_raster_constraint_info_scale(n.nspname, c.relname, a.attname, 'y') AS scale_y,
		_raster_constraint_info_blocksize(n.nspname, c.relname, a.attname, 'width') AS blocksize_x,
		_raster_constraint_info_blocksize(n.nspname, c.relname, a.attname, 'height') AS blocksize_y,
		COALESCE(_raster_constraint_info_alignment(n.nspname, c.relname, a.attname), FALSE) AS same_alignment,
		COALESCE(_raster_constraint_info_regular_blocking(n.nspname, c.relname, a.attname), FALSE) AS regular_blocking,
		_raster_constraint_info_num_bands(n.nspname, c.relname, a.attname) AS num_bands,
		_raster_constraint_info_pixel_types(n.nspname, c.relname, a.attname) AS pixel_types,
		_raster_constraint_info_nodata_values(n.nspname, c.relname, a.attname) AS nodata_values,
		_raster_constraint_info_out_db(n.nspname, c.relname, a.attname) AS out_db,
		_raster_constraint_info_extent(n.nspname, c.relname, a.attname) AS extent
	FROM
		pg_class c,
		pg_attribute a,
		pg_type t,
		pg_namespace n
	WHERE t.typname = 'raster'::name
		AND a.attisdropped = false
		AND a.atttypid = t.oid
		AND a.attrelid = c.oid
		AND c.relnamespace = n.oid
		AND (c.relkind = 'r'::"char" OR c.relkind = 'v'::"char")
		AND NOT pg_is_other_temp_schema(c.relnamespace);

------------------------------------------------------------------------------
-- overview constraint functions
-------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION _overview_constraint(ov raster, factor integer, refschema name, reftable name, refcolumn name)
	RETURNS boolean AS
	$$ SELECT COALESCE((SELECT TRUE FROM raster_columns WHERE r_table_catalog = current_database() AND r_table_schema = $3 AND r_table_name = $4 AND r_raster_column = $5), FALSE) $$
	LANGUAGE 'sql' STABLE
	COST 100;

CREATE OR REPLACE FUNCTION _overview_constraint_info(
	ovschema name, ovtable name, ovcolumn name,
	OUT refschema name, OUT reftable name, OUT refcolumn name, OUT factor integer
)
	AS $$
	SELECT
		split_part(split_part(s.consrc, '''::name', 1), '''', 2)::name,
		split_part(split_part(s.consrc, '''::name', 2), '''', 2)::name,
		split_part(split_part(s.consrc, '''::name', 3), '''', 2)::name,
		trim(both from split_part(s.consrc, ',', 2))::integer
	FROM pg_class c, pg_namespace n, pg_attribute a, pg_constraint s
	WHERE n.nspname = $1
		AND c.relname = $2
		AND a.attname = $3
		AND a.attrelid = c.oid
		AND s.connamespace = n.oid
		AND s.conrelid = c.oid
		AND a.attnum = ANY (s.conkey)
		AND s.consrc LIKE '%_overview_constraint(%'
	$$ LANGUAGE sql STABLE STRICT
  COST 100;

CREATE OR REPLACE FUNCTION _add_overview_constraint(
	ovschema name, ovtable name, ovcolumn name,
	refschema name, reftable name, refcolumn name,
	factor integer
)
	RETURNS boolean AS $$
	DECLARE
		fqtn text;
		cn name;
		sql text;
	BEGIN
		fqtn := '';
		IF length($1) > 0 THEN
			fqtn := quote_ident($1) || '.';
		END IF;
		fqtn := fqtn || quote_ident($2);

		cn := 'enforce_overview_' || $3;

		sql := 'ALTER TABLE ' || fqtn
			|| ' ADD CONSTRAINT ' || quote_ident(cn)
			|| ' CHECK (_overview_constraint(' || quote_ident($3)
			|| ',' || $7
			|| ',' || quote_literal($4)
			|| ',' || quote_literal($5)
			|| ',' || quote_literal($6)
			|| '))';

		RETURN _add_raster_constraint(cn, sql);
	END;
	$$ LANGUAGE 'plpgsql' VOLATILE STRICT
	COST 100;

CREATE OR REPLACE FUNCTION _drop_overview_constraint(ovschema name, ovtable name, ovcolumn name)
	RETURNS boolean AS
	$$ SELECT _drop_raster_constraint($1, $2, 'enforce_overview_' || $3) $$
	LANGUAGE 'sql' VOLATILE STRICT
	COST 100;

------------------------------------------------------------------------------
-- RASTER_OVERVIEWS
------------------------------------------------------------------------------

CREATE OR REPLACE VIEW raster_overviews AS
	SELECT
		current_database() AS o_table_catalog,
		n.nspname AS o_table_schema,
		c.relname AS o_table_name,
		a.attname AS o_raster_column,
		current_database() AS r_table_catalog,
		split_part(split_part(s.consrc, '''::name', 1), '''', 2)::name AS r_table_schema,
		split_part(split_part(s.consrc, '''::name', 2), '''', 2)::name AS r_table_name,
		split_part(split_part(s.consrc, '''::name', 3), '''', 2)::name AS r_raster_column,
		trim(both from split_part(s.consrc, ',', 2))::integer AS overview_factor
	FROM
		pg_class c,
		pg_attribute a,
		pg_type t,
		pg_namespace n,
		pg_constraint s
	WHERE t.typname = 'raster'::name
		AND a.attisdropped = false
		AND a.atttypid = t.oid
		AND a.attrelid = c.oid
		AND c.relnamespace = n.oid
		AND (c.relkind = 'r'::"char" OR c.relkind = 'v'::"char")
		AND s.connamespace = n.oid
		AND s.conrelid = c.oid
		AND s.consrc LIKE '%_overview_constraint(%'
		AND NOT pg_is_other_temp_schema(c.relnamespace);

------------------------------------------------------------------------------
-- AddOverviewConstraints
------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION AddOverviewConstraints (
	ovschema name, ovtable name, ovcolumn name,
	refschema name, reftable name, refcolumn name,
	ovfactor int
)
	RETURNS boolean
	AS $$
	DECLARE
		x int;
		s name;
		t name;
		oschema name;
		rschema name;
		sql text;
		rtn boolean;
	BEGIN
		FOR x IN 1..2 LOOP
			s := '';

			IF x = 1 THEN
				s := $1;
				t := $2;
			ELSE
				s := $4;
				t := $5;
			END IF;

			-- validate user-provided schema
			IF length(s) > 0 THEN
				sql := 'SELECT nspname FROM pg_namespace '
					|| 'WHERE nspname = ' || quote_literal(s)
					|| 'LIMIT 1';
				EXECUTE sql INTO s;

				IF s IS NULL THEN
					RAISE EXCEPTION 'The value % is not a valid schema', quote_literal(s);
					RETURN FALSE;
				END IF;
			END IF;

			-- no schema, determine what it could be using the table
			IF length(s) < 1 THEN
				sql := 'SELECT n.nspname AS schemaname '
					|| 'FROM pg_catalog.pg_class c '
					|| 'JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace '
					|| 'WHERE c.relkind = ' || quote_literal('r')
					|| ' AND n.nspname NOT IN (' || quote_literal('pg_catalog')
					|| ', ' || quote_literal('pg_toast')
					|| ') AND pg_catalog.pg_table_is_visible(c.oid)'
					|| ' AND c.relname = ' || quote_literal(t);
				EXECUTE sql INTO s;

				IF s IS NULL THEN
					RAISE EXCEPTION 'The table % does not occur in the search_path', quote_literal(t);
					RETURN FALSE;
				END IF;
			END IF;

			IF x = 1 THEN
				oschema := s;
			ELSE
				rschema := s;
			END IF;
		END LOOP;

		-- reference raster
		rtn := _add_overview_constraint(oschema, $2, $3, rschema, $5, $6, $7);
		IF rtn IS FALSE THEN
			RAISE EXCEPTION 'Unable to add the overview constraint.  Is the schema name, table name or column name incorrect?';
			RETURN FALSE;
		END IF;

		RETURN TRUE;
	END;
	$$ LANGUAGE 'plpgsql' VOLATILE STRICT
	COST 100;

CREATE OR REPLACE FUNCTION AddOverviewConstraints (
	ovtable name, ovcolumn name,
	reftable name, refcolumn name,
	ovfactor int
)
	RETURNS boolean
	AS $$ SELECT AddOverviewConstraints('', $1, $2, '', $3, $4, $5) $$
	LANGUAGE 'sql' VOLATILE STRICT
	COST 100;

------------------------------------------------------------------------------
-- DropOverviewConstraints
------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION DropOverviewConstraints (
	ovschema name,
	ovtable name,
	ovcolumn name
)
	RETURNS boolean
	AS $$
	DECLARE
		schema name;
		sql text;
		rtn boolean;
	BEGIN
		-- validate schema
		schema := NULL;
		IF length($1) > 0 THEN
			sql := 'SELECT nspname FROM pg_namespace '
				|| 'WHERE nspname = ' || quote_literal($1)
				|| 'LIMIT 1';
			EXECUTE sql INTO schema;

			IF schema IS NULL THEN
				RAISE EXCEPTION 'The value provided for schema is invalid';
				RETURN FALSE;
			END IF;
		END IF;

		IF schema IS NULL THEN
			sql := 'SELECT n.nspname AS schemaname '
				|| 'FROM pg_catalog.pg_class c '
				|| 'JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace '
				|| 'WHERE c.relkind = ' || quote_literal('r')
				|| ' AND n.nspname NOT IN (' || quote_literal('pg_catalog')
				|| ', ' || quote_literal('pg_toast')
				|| ') AND pg_catalog.pg_table_is_visible(c.oid)'
				|| ' AND c.relname = ' || quote_literal($2);
			EXECUTE sql INTO schema;

			IF schema IS NULL THEN
				RAISE EXCEPTION 'The table % does not occur in the search_path', quote_literal($2);
				RETURN FALSE;
			END IF;
		END IF;

		rtn := _drop_overview_constraint(schema, $2, $3);
		IF rtn IS FALSE THEN
			RAISE EXCEPTION 'Unable to drop the overview constraint .  Is the schema name, table name or column name incorrect?';
			RETURN FALSE;
		END IF;

		RETURN TRUE;
	END;
	$$ LANGUAGE 'plpgsql' VOLATILE STRICT
	COST 100;

CREATE OR REPLACE FUNCTION DropOverviewConstraints (
	ovtable name,
	ovcolumn name
)
	RETURNS boolean
	AS $$ SELECT DropOverviewConstraints('', $1, $2) $$
	LANGUAGE 'sql' VOLATILE STRICT
	COST 100;

-------------------------------------------------------------------
--  END
-------------------------------------------------------------------

COMMIT;
