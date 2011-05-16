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
-- Copyright (c) 2010 David Zwarg <dzwarg@avencia.com>
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
-- RasterLib Version
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION postgis_raster_lib_version()
    RETURNS text
    AS 'MODULE_PATHNAME', 'RASTER_lib_version'
    LANGUAGE 'C' IMMUTABLE; -- a new lib will require a new session

CREATE OR REPLACE FUNCTION postgis_raster_lib_build_date()
    RETURNS text
    AS 'MODULE_PATHNAME', 'RASTER_lib_build_date'
    LANGUAGE 'C' IMMUTABLE; -- a new lib will require a new session

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

CREATE OR REPLACE FUNCTION st_metadata(rast raster,
                                       OUT upperleftx float8,
                                       OUT upperlefty float8,
                                       OUT width int,
                                       OUT height int,
                                       OUT scalex float8,
                                       OUT scaley float8,
                                       OUT skewx float8,
                                       OUT skewy float8,
                                       OUT srid int,
                                       OUT numbands int
                                      )
    AS $$
    SELECT st_upperleftx($1),
           st_upperlefty($1),
           st_width($1),
           st_height($1),
           st_scalex($1),
           st_scaley($1),
           st_skewx($1),
           st_skewy($1),
           st_srid($1),
           st_numbands($1)
    $$
    LANGUAGE SQL IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- Constructors ST_MakeEmptyRaster and ST_AddBand
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_makeemptyraster(width int, height int, upperleftx float8, upperlefty float8, scalex float8, scaley float8, skewx float8, skewy float8, srid int4)
    RETURNS RASTER
    AS 'MODULE_PATHNAME', 'RASTER_makeEmpty'
    LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_makeemptyraster(width int, height int, upperleftx float8, upperlefty float8, pixelsize float8)
    RETURNS raster
    AS 'select st_makeemptyraster($1, $2, $3, $4, $5, $5, 0, 0, -1)'
    LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_makeemptyraster(width int, height int, upperleftx float8, upperlefty float8, scalex float8, scaley float8, skewx float8, skewy float8)
    RETURNS raster
    AS 'select st_makeemptyraster($1, $2, $3, $4, $5, $6, $7, $8, -1)'
    LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_makeemptyraster(rast raster)
    RETURNS raster
    AS 'select st_makeemptyraster(st_width($1), st_height($1), st_upperleftx($1), st_upperlefty($1), st_scalex($1), st_scaley($1), st_skewx($1), st_skewy($1), st_srid($1))'
    LANGUAGE 'SQL' IMMUTABLE STRICT;

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
CREATE OR REPLACE FUNCTION st_band(rast raster, nbands int[])
	RETURNS RASTER
	AS 'MODULE_PATHNAME', 'RASTER_band'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_band(rast raster, nband int)
	RETURNS RASTER
	AS $$ SELECT st_band($1, ARRAY[$2]) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_band(rast raster, nbands text)
	RETURNS RASTER
	AS $$ SELECT st_band($1, regexp_split_to_array(regexp_replace($2, '[[:space:]]', '', 'g'), ',')::int[]) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_band(rast raster, nbands text, delimiter char)
	RETURNS RASTER
	AS $$ SELECT st_band($1, regexp_split_to_array(regexp_replace($2, '[[:space:]]', '', 'g'), $3)::int[]) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_band(rast raster)
	RETURNS RASTER
	AS $$ SELECT st_band($1, ARRAY[1]) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- ST_SummaryStats and ST_ApproxSummaryStats
-----------------------------------------------------------------------
CREATE TYPE summarystats AS (
	count integer,
	mean double precision,
	stddev double precision,
	min double precision,
	max double precision
);

CREATE OR REPLACE FUNCTION _st_summarystats(rast raster, nband int, hasnodata boolean, sample_percent double precision)
	RETURNS summarystats
	AS 'MODULE_PATHNAME','RASTER_summaryStats'
	LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_summarystats(rast raster, nband int, hasnodata boolean)
	RETURNS summarystats
	AS $$ SELECT count, mean, stddev, min, max FROM _st_summarystats($1, $2, $3, 1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_summarystats(rast raster, nband int)
	RETURNS summarystats
	AS $$ SELECT count, mean, stddev, min, max FROM _st_summarystats($1, $2, FALSE, 1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_summarystats(rast raster, hasnodata boolean)
	RETURNS summarystats
	AS $$ SELECT count, mean, stddev, min, max FROM _st_summarystats($1, 1, $2, 1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_summarystats(rast raster)
	RETURNS summarystats
	AS $$ SELECT count, mean, stddev, min, max FROM _st_summarystats($1, 1, FALSE, 1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxsummarystats(rast raster, nband int, hasnodata boolean, sample_percent double precision)
	RETURNS summarystats
	AS $$ SELECT count, mean, stddev, min, max FROM _st_summarystats($1, $2, $3, $4) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxsummarystats(rast raster, nband int, sample_percent double precision)
	RETURNS summarystats
	AS $$ SELECT count, mean, stddev, min, max FROM _st_summarystats($1, $2, FALSE, $3) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxsummarystats(rast raster, hasnodata boolean, sample_percent double precision)
	RETURNS summarystats
	AS $$ SELECT count, mean, stddev, min, max FROM _st_summarystats($1, 1, $2, $3) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxsummarystats(rast raster, sample_percent double precision)
	RETURNS summarystats
	AS $$ SELECT count, mean, stddev, min, max FROM _st_summarystats($1, 1, FALSE, $2) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxsummarystats(rast raster)
	RETURNS summarystats
	AS $$ SELECT count, mean, stddev, min, max FROM _st_summarystats($1, 1, FALSE, 0.1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION _st_summarystats(rastertable text, rastercolumn text, nband integer, hasnodata boolean, sample_percent double precision)
	RETURNS summarystats
	AS $$
	DECLARE
		curs refcursor;

		ctable text;
		ccolumn text;
		rast raster;
		stats summarystats;
		rtn summarystats;

		mean double precision;
		tmp double precision;
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

		mean := 0;
		tmp := 0;
		LOOP
			FETCH curs INTO rast;
			EXIT WHEN NOT FOUND;

			SELECT * FROM _st_summarystats(rast, nband, hasnodata, sample_percent) INTO stats;
			IF stats IS NULL OR stats.count < 1 THEN
				CONTINUE;
			END IF;
			--RAISE NOTICE 'stats = %', stats;

			mean := mean + (stats.count * stats.mean); -- not the mean
			-- sum of "sum of squares"
			IF sample_percent > 0 AND sample_percent < 1 THEN
				tmp := tmp + (power(stats.stddev, 2) * (stats.count - 1));
			ELSE
				tmp := tmp + (power(stats.stddev, 2) * stats.count);
			END IF;

			IF rtn.count IS NULL THEN
				rtn := stats;
			ELSE
				rtn.count = rtn.count + stats.count;
				
				IF stats.min < rtn.min THEN
					rtn.min := stats.min;
				END IF;
				IF stats.max > rtn.max THEN
					rtn.max := stats.max;
				END IF;
			END IF;

		END LOOP;

		CLOSE curs;

		-- weighted mean and cumulative stddev
		IF rtn.count IS NOT NULL AND rtn.count > 0 THEN
			rtn.mean := mean / rtn.count;
			rtn.stddev := sqrt(tmp / rtn.count);
		END IF;

		RETURN rtn;
	END;
	$$ LANGUAGE 'plpgsql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_summarystats(rastertable text, rastercolumn text, nband integer, hasnodata boolean)
	RETURNS summarystats
	AS $$ SELECT count, mean, stddev, min, max FROM _st_summarystats($1, $2, $3, $4, 1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_summarystats(rastertable text, rastercolumn text, nband integer)
	RETURNS summarystats
	AS $$ SELECT count, mean, stddev, min, max FROM _st_summarystats($1, $2, $3, FALSE, 1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_summarystats(rastertable text, rastercolumn text)
	RETURNS summarystats
	AS $$ SELECT count, mean, stddev, min, max FROM _st_summarystats($1, $2, 1, FALSE, 1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxsummarystats(rastertable text, rastercolumn text, nband integer, hasnodata boolean, sample_percent double precision)
	RETURNS summarystats
	AS $$ SELECT count, mean, stddev, min, max FROM _st_summarystats($1, $2, $3, $4, $5) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxsummarystats(rastertable text, rastercolumn text, nband integer, sample_percent double precision)
	RETURNS summarystats
	AS $$ SELECT count, mean, stddev, min, max FROM _st_summarystats($1, $2, $3, FALSE, $4) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxsummarystats(rastertable text, rastercolumn text, nband integer)
	RETURNS summarystats
	AS $$ SELECT count, mean, stddev, min, max FROM _st_summarystats($1, $2, $3, FALSE, 0.1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxsummarystats(rastertable text, rastercolumn text, sample_percent double precision)
	RETURNS summarystats
	AS $$ SELECT count, mean, stddev, min, max FROM _st_summarystats($1, $2, 1, FALSE, $3) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxsummarystats(rastertable text, rastercolumn text)
	RETURNS summarystats
	AS $$ SELECT count, mean, stddev, min, max FROM _st_summarystats($1, $2, 1, FALSE, 0.1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- ST_Mean and ST_ApproxMean
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_mean(rast raster, nband int, hasnodata boolean, OUT mean double precision)
	RETURNS double precision
	AS $$ SELECT mean FROM _st_summarystats($1, $2, $3, 1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_mean(rast raster, nband int, OUT mean double precision)
	RETURNS double precision
	AS $$ SELECT mean FROM _st_summarystats($1, $2, FALSE, 1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_mean(rast raster, hasnodata boolean, OUT mean double precision)
	RETURNS double precision
	AS $$ SELECT mean FROM _st_summarystats($1, 1, $2, 1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_mean(rast raster, OUT mean double precision)
	RETURNS double precision
	AS $$ SELECT mean FROM _st_summarystats($1, 1, FALSE, 1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxmean(rast raster, nband int, hasnodata boolean, sample_percent double precision, OUT mean double precision)
	RETURNS double precision
	AS $$ SELECT mean FROM _st_summarystats($1, $2, $3, $4) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxmean(rast raster, nband int, sample_percent double precision, OUT mean double precision)
	RETURNS double precision
	AS $$ SELECT mean FROM _st_summarystats($1, $2, FALSE, $3) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxmean(rast raster, hasnodata boolean, sample_percent double precision, OUT mean double precision)
	RETURNS double precision
	AS $$ SELECT mean FROM _st_summarystats($1, 1, $2, $3) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxmean(rast raster, sample_percent double precision, OUT mean double precision)
	RETURNS double precision
	AS $$ SELECT mean FROM _st_summarystats($1, 1, FALSE, $2) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxmean(rast raster, OUT mean double precision)
	RETURNS double precision
	AS $$ SELECT mean FROM _st_summarystats($1, 1, FALSE, 0.1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_mean(rastertable text, rastercolumn text, nband int, hasnodata boolean, OUT mean double precision)
	RETURNS double precision
	AS $$ SELECT mean FROM _st_summarystats($1, $2, $3, $4, 1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_mean(rastertable text, rastercolumn text, nband int, OUT mean double precision)
	RETURNS double precision
	AS $$ SELECT mean FROM _st_summarystats($1, $2, $3, FALSE, 1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_mean(rastertable text, rastercolumn text, hasnodata boolean, OUT mean double precision)
	RETURNS double precision
	AS $$ SELECT mean FROM _st_summarystats($1, $2, 1, $3, 1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_mean(rastertable text, rastercolumn text, OUT mean double precision)
	RETURNS double precision
	AS $$ SELECT mean FROM _st_summarystats($1, $2, 1, FALSE, 1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxmean(rastertable text, rastercolumn text, nband int, hasnodata boolean, sample_percent double precision, OUT mean double precision)
	RETURNS double precision
	AS $$ SELECT mean FROM _st_summarystats($1, $2, $3, $4, $5) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxmean(rastertable text, rastercolumn text, nband int, sample_percent double precision, OUT mean double precision)
	RETURNS double precision
	AS $$ SELECT mean FROM _st_summarystats($1, $2, $3, FALSE, $4) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxmean(rastertable text, rastercolumn text, hasnodata boolean, sample_percent double precision, OUT mean double precision)
	RETURNS double precision
	AS $$ SELECT mean FROM _st_summarystats($1, $2, 1, $3, $4) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxmean(rastertable text, rastercolumn text, sample_percent double precision, OUT mean double precision)
	RETURNS double precision
	AS $$ SELECT mean FROM _st_summarystats($1, $2, 1, FALSE, $3) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxmean(rastertable text, rastercolumn text, OUT mean double precision)
	RETURNS double precision
	AS $$ SELECT mean FROM _st_summarystats($1, $2, 1, FALSE, 0.1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- ST_StdDev and ST_ApproxStdDev
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_stddev(rast raster, nband int, hasnodata boolean, OUT stddev double precision)
	RETURNS double precision
	AS $$ SELECT stddev FROM _st_summarystats($1, $2, $3, 1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_stddev(rast raster, nband int, OUT stddev double precision)
	RETURNS double precision
	AS $$ SELECT stddev FROM _st_summarystats($1, $2, FALSE, 1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_stddev(rast raster, hasnodata boolean, OUT stddev double precision)
	RETURNS double precision
	AS $$ SELECT stddev FROM _st_summarystats($1, 1, $2, 1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_stddev(rast raster, OUT stddev double precision)
	RETURNS double precision
	AS $$ SELECT stddev FROM _st_summarystats($1, 1, FALSE, 1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxstddev(rast raster, nband int, hasnodata boolean, sample_percent double precision, OUT stddev double precision)
	RETURNS double precision
	AS $$ SELECT stddev FROM _st_summarystats($1, $2, $3, $4) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxstddev(rast raster, nband int, sample_percent double precision, OUT stddev double precision)
	RETURNS double precision
	AS $$ SELECT stddev FROM _st_summarystats($1, $2, FALSE, $3) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxstddev(rast raster, hasnodata boolean, sample_percent double precision, OUT stddev double precision)
	RETURNS double precision
	AS $$ SELECT stddev FROM _st_summarystats($1, 1, $2, $3) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxstddev(rast raster, sample_percent double precision, OUT stddev double precision)
	RETURNS double precision
	AS $$ SELECT stddev FROM _st_summarystats($1, 1, FALSE, $2) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxstddev(rast raster, OUT stddev double precision)
	RETURNS double precision
	AS $$ SELECT stddev FROM _st_summarystats($1, 1, FALSE, 0.1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_stddev(rastertable text, rastercolumn text, nband int, hasnodata boolean, OUT stddev double precision)
	RETURNS double precision
	AS $$ SELECT stddev FROM _st_summarystats($1, $2, $3, $4, 1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_stddev(rastertable text, rastercolumn text, nband int, OUT stddev double precision)
	RETURNS double precision
	AS $$ SELECT stddev FROM _st_summarystats($1, $2, $3, FALSE, 1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_stddev(rastertable text, rastercolumn text, hasnodata boolean, OUT stddev double precision)
	RETURNS double precision
	AS $$ SELECT stddev FROM _st_summarystats($1, $2, 1, $3, 1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_stddev(rastertable text, rastercolumn text, OUT stddev double precision)
	RETURNS double precision
	AS $$ SELECT stddev FROM _st_summarystats($1, $2, 1, FALSE, 1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxstddev(rastertable text, rastercolumn text, nband int, hasnodata boolean, sample_percent double precision, OUT stddev double precision)
	RETURNS double precision
	AS $$ SELECT stddev FROM _st_summarystats($1, $2, $3, $4, $5) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxstddev(rastertable text, rastercolumn text, nband int, sample_percent double precision, OUT stddev double precision)
	RETURNS double precision
	AS $$ SELECT stddev FROM _st_summarystats($1, $2, $3, FALSE, $4) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxstddev(rastertable text, rastercolumn text, hasnodata boolean, sample_percent double precision, OUT stddev double precision)
	RETURNS double precision
	AS $$ SELECT stddev FROM _st_summarystats($1, $2, 1, $3, $4) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxstddev(rastertable text, rastercolumn text, sample_percent double precision, OUT stddev double precision)
	RETURNS double precision
	AS $$ SELECT stddev FROM _st_summarystats($1, $2, 1, FALSE, $3) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxstddev(rastertable text, rastercolumn text, OUT stddev double precision)
	RETURNS double precision
	AS $$ SELECT stddev FROM _st_summarystats($1, $2, 1, FALSE, 0.1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- ST_MinMax and ST_ApproxMinMax
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_minmax(rast raster, nband int, hasnodata boolean, OUT min double precision, OUT max double precision)
	RETURNS record
	AS $$ SELECT min, max FROM _st_summarystats($1, $2, $3, 1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_minmax(rast raster, nband int, OUT min double precision, OUT max double precision)
	RETURNS record
	AS $$ SELECT min, max FROM _st_summarystats($1, $2, FALSE, 1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_minmax(rast raster, hasnodata boolean, OUT min double precision, OUT max double precision)
	RETURNS record
	AS $$ SELECT min, max FROM _st_summarystats($1, 1, $2, 1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_minmax(rast raster, OUT min double precision, OUT max double precision)
	RETURNS record
	AS $$ SELECT min, max FROM _st_summarystats($1, 1, FALSE, 1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxminmax(rast raster, nband int, hasnodata boolean, sample_percent double precision, OUT min double precision, OUT max double precision)
	RETURNS record
	AS $$ SELECT min, max FROM _st_summarystats($1, $2, $3, $4) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxminmax(rast raster, nband int, sample_percent double precision, OUT min double precision, OUT max double precision)
	RETURNS record
	AS $$ SELECT min, max FROM _st_summarystats($1, $2, FALSE, $3) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxminmax(rast raster, hasnodata boolean, sample_percent double precision, OUT min double precision, OUT max double precision)
	RETURNS record
	AS $$ SELECT min, max FROM _st_summarystats($1, 1, $2, $3) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxminmax(rast raster, sample_percent double precision, OUT min double precision, OUT max double precision)
	RETURNS record
	AS $$ SELECT min, max FROM _st_summarystats($1, 1, FALSE, $2) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxminmax(rast raster, OUT min double precision, OUT max double precision)
	RETURNS record
	AS $$ SELECT min, max FROM _st_summarystats($1, 1, FALSE, 0.1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_minmax(rastertable text, rastercolumn text, nband int, hasnodata boolean, OUT min double precision, OUT max double precision)
	RETURNS record
	AS $$ SELECT min, max FROM _st_summarystats($1, $2, $3, $4, 1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_minmax(rastertable text, rastercolumn text, nband int, OUT min double precision, OUT max double precision)
	RETURNS record
	AS $$ SELECT min, max FROM _st_summarystats($1, $2, $3, FALSE, 1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_minmax(rastertable text, rastercolumn text, hasnodata boolean, OUT min double precision, OUT max double precision)
	RETURNS record
	AS $$ SELECT min, max FROM _st_summarystats($1, $2, 1, $3, 1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_minmax(rastertable text, rastercolumn text, OUT min double precision, OUT max double precision)
	RETURNS record
	AS $$ SELECT min, max FROM _st_summarystats($1, $2, 1, FALSE, 1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxminmax(rastertable text, rastercolumn text, nband int, hasnodata boolean, sample_percent double precision, OUT min double precision, OUT max double precision)
	RETURNS record
	AS $$ SELECT min, max FROM _st_summarystats($1, $2, $3, $4, $5) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxminmax(rastertable text, rastercolumn text, nband int, sample_percent double precision, OUT min double precision, OUT max double precision)
	RETURNS record
	AS $$ SELECT min, max FROM _st_summarystats($1, $2, $3, FALSE, $4) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxminmax(rastertable text, rastercolumn text, hasnodata boolean, sample_percent double precision, OUT min double precision, OUT max double precision)
	RETURNS record
	AS $$ SELECT min, max FROM _st_summarystats($1, $2, 1, $3, $4) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxminmax(rastertable text, rastercolumn text, sample_percent double precision, OUT min double precision, OUT max double precision)
	RETURNS record
	AS $$ SELECT min, max FROM _st_summarystats($1, $2, 1, FALSE, $3) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxminmax(rastertable text, rastercolumn text, OUT min double precision, OUT max double precision)
	RETURNS record
	AS $$ SELECT min, max FROM _st_summarystats($1, $2, 1, FALSE, 0.1) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- ST_Histogram and ST_ApproxHistogram
-----------------------------------------------------------------------
CREATE TYPE histogram AS (
	min double precision,
	max double precision,
	count integer,
	proportion double precision
);

-- Cannot be strict as "width" can be NULL
CREATE OR REPLACE FUNCTION _st_histogram(rast raster, nband int, hasnodata boolean, sample_percent double precision, bins int, width double precision[], right boolean)
	RETURNS SETOF histogram
	AS 'MODULE_PATHNAME','RASTER_histogram'
	LANGUAGE 'C' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_histogram(rast raster, nband int, hasnodata boolean, bins int, width double precision[], right boolean)
	RETURNS SETOF histogram
	AS $$ SELECT min, max, count, proportion FROM _st_histogram($1, $2, $3, 1, $4, $5, $6) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_histogram(rast raster, nband int, hasnodata boolean, bins int, right boolean)
	RETURNS SETOF histogram
	AS $$ SELECT min, max, count, proportion FROM _st_histogram($1, $2, $3, 1, $4, NULL, $5) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_histogram(rast raster, nband int, hasnodata boolean, bins int)
	RETURNS SETOF histogram
	AS $$ SELECT min, max, count, proportion FROM _st_histogram($1, $2, $3, 1, $4, NULL, FALSE) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_histogram(rast raster, nband int, hasnodata boolean)
	RETURNS SETOF histogram
	AS $$ SELECT min, max, count, proportion FROM _st_histogram($1, $2, $3, 1, 0, NULL, FALSE) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_histogram(rast raster, nband int)
	RETURNS SETOF histogram
	AS $$ SELECT min, max, count, proportion FROM _st_histogram($1, $2, FALSE, 1, 0, NULL, FALSE) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_histogram(rast raster)
	RETURNS SETOF histogram
	AS $$ SELECT min, max, count, proportion FROM _st_histogram($1, 1, FALSE, 1, 0, NULL, FALSE) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_histogram(rast raster, nband int, bins int, width double precision[], right boolean)
	RETURNS SETOF histogram
	AS $$ SELECT min, max, count, proportion FROM _st_histogram($1, $2, FALSE, 1, $3, $4, $5) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_histogram(rast raster, nband int, bins int, right boolean)
	RETURNS SETOF histogram
	AS $$ SELECT min, max, count, proportion FROM _st_histogram($1, $2, FALSE, 1, $3, NULL, $4) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_histogram(rast raster, nband int, bins int)
	RETURNS SETOF histogram
	AS $$ SELECT min, max, count, proportion FROM _st_histogram($1, $2, FALSE, 1, $3, NULL, FALSE) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxhistogram(rast raster, nband int, hasnodata boolean, sample_percent double precision, bins int, width double precision[], right boolean)
	RETURNS SETOF histogram
	AS $$ SELECT min, max, count, proportion FROM _st_histogram($1, $2, $3, $4, $5, $6, $7) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxhistogram(rast raster, nband int, hasnodata boolean, sample_percent double precision, bins int, right boolean)
	RETURNS SETOF histogram
	AS $$ SELECT min, max, count, proportion FROM _st_histogram($1, $2, $3, $4, $5, NULL, $6) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxhistogram(rast raster, nband int, hasnodata boolean, sample_percent double precision, bins int)
	RETURNS SETOF histogram
	AS $$ SELECT min, max, count, proportion FROM _st_histogram($1, $2, $3, $4, $5, NULL, FALSE) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxhistogram(rast raster, nband int, hasnodata boolean, sample_percent double precision)
	RETURNS SETOF histogram
	AS $$ SELECT min, max, count, proportion FROM _st_histogram($1, $2, $3, $4, 0, NULL, FALSE) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxhistogram(rast raster, nband int, sample_percent double precision)
	RETURNS SETOF histogram
	AS $$ SELECT min, max, count, proportion FROM _st_histogram($1, $2, FALSE, $3, 0, NULL, FALSE) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxhistogram(rast raster, nband int)
	RETURNS SETOF histogram
	AS $$ SELECT min, max, count, proportion FROM _st_histogram($1, $2, FALSE, 0.1, 0, NULL, FALSE) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxhistogram(rast raster, sample_percent double precision)
	RETURNS SETOF histogram
	AS $$ SELECT min, max, count, proportion FROM _st_histogram($1, 1, FALSE, $2, 0, NULL, FALSE) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxhistogram(rast raster)
	RETURNS SETOF histogram
	AS $$ SELECT min, max, count, proportion FROM _st_histogram($1, 1, FALSE, 0.1, 0, NULL, FALSE) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxhistogram(rast raster, nband int, sample_percent double precision, bins int, width double precision[], right boolean)
	RETURNS SETOF histogram
	AS $$ SELECT min, max, count, proportion FROM _st_histogram($1, $2, FALSE, $3, $4, $5, $6) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxhistogram(rast raster, nband int, sample_percent double precision, bins int, right boolean)
	RETURNS SETOF histogram
	AS $$ SELECT min, max, count, proportion FROM _st_histogram($1, $2, FALSE, $3, $4, NULL, $5) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxhistogram(rast raster, nband int, sample_percent double precision, bins int)
	RETURNS SETOF histogram
	AS $$ SELECT min, max, count, proportion FROM _st_histogram($1, $2, FALSE, $3, $4, NULL, FALSE) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxhistogram(rast raster, nband int, sample_percent double precision)
	RETURNS SETOF histogram
	AS $$ SELECT min, max, count, proportion FROM _st_histogram($1, $2, FALSE, $3, 0, NULL, FALSE) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- ST_Quantile and ST_ApproxQuantile
-----------------------------------------------------------------------
CREATE TYPE quantile AS (
	quantile double precision,
	value double precision
);

-- Cannot be strict as "quantile" can be NULL
CREATE OR REPLACE FUNCTION _st_quantile(rast raster, nband int, hasnodata boolean, sample_percent double precision, quantiles double precision[])
	RETURNS SETOF quantile
	AS 'MODULE_PATHNAME','RASTER_quantile'
	LANGUAGE 'C' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_quantile(rast raster, nband int, hasnodata boolean, quantiles double precision[])
	RETURNS SETOF quantile
	AS $$ SELECT quantile, value FROM _st_quantile($1, $2, $3, 1, $4) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_quantile(rast raster, nband int, quantiles double precision[])
	RETURNS SETOF quantile
	AS $$ SELECT quantile, value FROM _st_quantile($1, $2, FALSE, 1, $3) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_quantile(rast raster, nband int, hasnodata boolean)
	RETURNS SETOF quantile
	AS $$ SELECT quantile, value FROM _st_quantile($1, $2, $3, 1, NULL) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_quantile(rast raster, nband int)
	RETURNS SETOF quantile
	AS $$ SELECT quantile, value FROM _st_quantile($1, $2, FALSE, 1, NULL) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_quantile(rast raster, quantiles double precision[])
	RETURNS SETOF quantile
	AS $$ SELECT quantile, value FROM _st_quantile($1, 1, FALSE, 1, $2) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_quantile(rast raster)
	RETURNS SETOF quantile
	AS $$ SELECT quantile, value FROM _st_quantile($1, 1, FALSE, 1, NULL) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_quantile(rast raster, nband int, hasnodata boolean, quantile double precision)
	RETURNS quantile
	AS $$ SELECT quantile, value FROM _st_quantile($1, $2, $3, 1, ARRAY[$4]::double precision[]) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_quantile(rast raster, nband int, quantile double precision)
	RETURNS quantile
	AS $$ SELECT quantile, value FROM _st_quantile($1, $2, FALSE, 1, ARRAY[$3]::double precision[]) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_quantile(rast raster, hasnodata boolean, quantile double precision)
	RETURNS quantile
	AS $$ SELECT quantile, value FROM _st_quantile($1, 1, $2, 1, ARRAY[$3]::double precision[]) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_quantile(rast raster, quantile double precision)
	RETURNS quantile
	AS $$ SELECT quantile, value FROM _st_quantile($1, 1, FALSE, 1, ARRAY[$2]::double precision[]) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxquantile(rast raster, nband int, hasnodata boolean, sample_percent double precision, quantiles double precision[])
	RETURNS SETOF quantile
	AS $$ SELECT quantile, value FROM _st_quantile($1, $2, $3, $4, $5) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxquantile(rast raster, nband int, sample_percent double precision, quantiles double precision[])
	RETURNS SETOF quantile
	AS $$ SELECT quantile, value FROM _st_quantile($1, $2, FALSE, $3, $4) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxquantile(rast raster, nband int, sample_percent double precision)
	RETURNS SETOF quantile
	AS $$ SELECT quantile, value FROM _st_quantile($1, $2, FALSE, $3, NULL) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxquantile(rast raster, sample_percent double precision, quantiles double precision[])
	RETURNS SETOF quantile
	AS $$ SELECT quantile, value FROM _st_quantile($1, 1, FALSE, $2, $3) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxquantile(rast raster, sample_percent double precision)
	RETURNS SETOF quantile
	AS $$ SELECT quantile, value FROM _st_quantile($1, 1, FALSE, $2, NULL) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxquantile(rast raster, quantiles double precision[])
	RETURNS SETOF quantile
	AS $$ SELECT quantile, value FROM _st_quantile($1, 1, FALSE, 0.1, $2) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxquantile(rast raster, nband int)
	RETURNS SETOF quantile
	AS $$ SELECT quantile, value FROM _st_quantile($1, $2, FALSE, 0.1, NULL) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxquantile(rast raster)
	RETURNS SETOF quantile
	AS $$ SELECT quantile, value FROM _st_quantile($1, 1, FALSE, 0.1, NULL) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxquantile(rast raster, nband int, hasnodata boolean, sample_percent double precision, quantile double precision)
	RETURNS quantile
	AS $$ SELECT quantile, value FROM _st_quantile($1, $2, $3, $4, ARRAY[$5]::double precision[]) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxquantile(rast raster, nband int, sample_percent double precision, quantile double precision)
	RETURNS quantile
	AS $$ SELECT quantile, value FROM _st_quantile($1, $2, FALSE, $3, ARRAY[$4]::double precision[]) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxquantile(rast raster, sample_percent double precision, quantile double precision)
	RETURNS quantile
	AS $$ SELECT quantile, value FROM _st_quantile($1, 1, FALSE, $2, ARRAY[$3]::double precision[]) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_approxquantile(rast raster, hasnodata boolean, quantile double precision)
	RETURNS quantile
	AS $$ SELECT quantile, value FROM _st_quantile($1, 1, $2, 1, ARRAY[$3]::double precision[]) $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

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

CREATE OR REPLACE FUNCTION st_reclass(rast raster, nband int, reclassexpr text, pixeltype text, nodataval double precision)
	RETURNS raster
	AS $$ SELECT st_reclass($1, ROW($2, $3, $4, $5)) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_reclass(rast raster, nband int, reclassexpr text, pixeltype text)
	RETURNS raster
	AS $$ SELECT st_reclass($1, ROW($2, $3, $4, NULL)) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_reclass(rast raster, reclassexpr text, pixeltype text)
	RETURNS raster
	AS $$ SELECT st_reclass($1, ROW(1, $2, $3, NULL)) $$
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- MapAlgebra
-----------------------------------------------------------------------
-- This function can not be STRICT, because nodatavalueexpr can be NULL (could be just '' though)
-- or pixeltype can not be determined (could be st_bandpixeltype(raster, band) though)
CREATE OR REPLACE FUNCTION st_mapalgebra(rast raster, band integer,
        expression text, nodatavalueexpr text, pixeltype text)
    RETURNS raster
    AS 'MODULE_PATHNAME', 'RASTER_mapAlgebra'
    LANGUAGE 'C' IMMUTABLE;

CREATE OR REPLACE FUNCTION st_mapalgebra(rast raster, band integer,
        expression text)
    RETURNS raster
    AS $$ SELECT st_mapalgebra($1, $2, $3, NULL, NULL) $$
    LANGUAGE SQL IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_mapalgebra(rast raster, expression text,
        pixeltype text)
    RETURNS raster
    AS $$ SELECT st_mapalgebra($1, 1, $2, NULL, $3) $$
    LANGUAGE SQL IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_mapalgebra(rast raster, expression text)
    RETURNS raster
    AS $$ SELECT st_mapalgebra($1, 1, $2, NULL, NULL) $$
    LANGUAGE SQL IMMUTABLE STRICT;

-- This function can not be STRICT, because nodatavalueexpr can be NULL (could be just '' though)
CREATE OR REPLACE FUNCTION st_mapalgebra(rast raster, band integer,
        expression text, nodatavalueexpr text)
    RETURNS raster
    AS $$ SELECT st_mapalgebra($1, $2, $3, $4, NULL) $$
    LANGUAGE SQL;

-- This function can not be STRICT, because nodatavalueexpr can be NULL (could be just '' though)
-- or pixeltype can not be determined (could be st_bandpixeltype(raster, band) though)
CREATE OR REPLACE FUNCTION st_mapalgebra(rast raster, expression text,
        nodatavalueexpr text, pixeltype text)
    RETURNS raster
    AS $$ SELECT st_mapalgebra($1, 1, $2, $3, $4) $$
    LANGUAGE SQL;

-- This function can not be STRICT, because nodatavalueexpr can be NULL (could be just '' though)
CREATE OR REPLACE FUNCTION st_mapalgebra(rast raster, expression text,
        nodatavalueexpr text)
    RETURNS raster
    AS $$ SELECT st_mapalgebra($1, 1, $2, $3, NULL) $$
    LANGUAGE SQL;


-----------------------------------------------------------------------
-- Get information about the raster
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_isempty(rast raster)
    RETURNS boolean
    AS 'MODULE_PATHNAME', 'RASTER_isEmpty'
    LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_hasnoband(rast raster, nband int)
    RETURNS boolean
    AS 'MODULE_PATHNAME', 'RASTER_hasNoBand'
    LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_hasnoband(rast raster)
    RETURNS boolean
    AS 'select st_hasnoband($1, 1)'
    LANGUAGE 'SQL' IMMUTABLE STRICT;

-----------------------------------------------------------------------
-- Raster Band Accessors
-----------------------------------------------------------------------

CREATE OR REPLACE FUNCTION st_bandnodatavalue(rast raster, band integer)
    RETURNS float4
    AS 'MODULE_PATHNAME','RASTER_getBandNoDataValue'
    LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_bandnodatavalue(raster)
    RETURNS float4
    AS $$ SELECT st_bandnodatavalue($1, 1) $$
    LANGUAGE SQL IMMUTABLE STRICT;

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

CREATE OR REPLACE FUNCTION st_bandmetadata(rast raster,
                                           band int,
                                           OUT pixeltype text,
                                           OUT hasnodatavalue boolean,
                                           OUT nodatavalue float4,
                                           OUT isoutdb boolean,
                                           OUT path text)
    AS $$
    SELECT st_bandpixeltype($1, $2),
       st_bandnodatavalue($1, $2) IS NOT NULL,
       st_bandnodatavalue($1, $2),
       st_bandpath($1, $2) IS NOT NULL,
       st_bandpath($1, $2)
    $$
    LANGUAGE SQL IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_bandmetadata(rast raster,
                                           OUT pixeltype text,
                                           OUT hasnodatavalue boolean,
                                           OUT nodatavalue float4,
                                           OUT isoutdb boolean,
                                           OUT path text)
    AS $$
    SELECT st_bandpixeltype($1, 1),
       st_bandnodatavalue($1, 1) IS NOT NULL,
       st_bandnodatavalue($1, 1),
       st_bandpath($1, 1) IS NOT NULL,
       st_bandpath($1, 1)
    $$
    LANGUAGE SQL IMMUTABLE STRICT;

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
        x numeric;
        result text;
    BEGIN
            x := st_scalex(rast)::numeric;
            result := trunc(x, 10) || E'\n';

            x := st_skewy(rast)::numeric;
            result := result || trunc(x, 10) || E'\n';

            x := st_skewx(rast)::numeric;
            result := result || trunc(x, 10) || E'\n';

            x := st_scaley(rast)::numeric;
            result := result || trunc(x, 10) || E'\n';

        IF format = 'ESRI' THEN
            x := (st_upperleftx(rast) + st_scalex(rast)*0.5)::numeric;
            result := result || trunc(x, 10) || E'\n';

            x := (st_upperlefty(rast) + st_scaley(rast)*0.5)::numeric;
            result = result || trunc(x, 10) || E'\n';
        ELSE -- IF format = 'GDAL' THEN
            x := st_upperleftx(rast)::numeric;
            result := result || trunc(x, 10) || E'\n';

            x := st_upperlefty(rast)::numeric;
            result := result || trunc(x, 10) || E'\n';
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

-----------------------------------------------------------------------
-- Raster Editors ST_SetGeoreference()
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION st_setgeoreference(rast raster, georef text, format text)
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

CREATE OR REPLACE FUNCTION st_setgeoreference(rast raster, georef text)
    RETURNS raster AS
    $$
        SELECT st_setgeoreference($1, $2, 'GDAL');
    $$
    LANGUAGE 'SQL' IMMUTABLE STRICT; -- WITH (isstrict);

-----------------------------------------------------------------------
-- Raster Band Editors
-----------------------------------------------------------------------

-- This function can not be STRICT, because nodatavalue can be NULL indicating that no nodata value should be set
CREATE OR REPLACE FUNCTION st_setbandnodatavalue(rast raster, band integer,
        nodatavalue float8, forceChecking boolean)
    RETURNS raster
    AS 'MODULE_PATHNAME','RASTER_setBandNoDataValue'
    LANGUAGE 'C' IMMUTABLE;

-- This function can not be STRICT, because nodatavalue can be NULL indicating that no nodata value should be set
CREATE OR REPLACE FUNCTION st_setbandnodatavalue(rast raster, band integer, nodatavalue float8)
    RETURNS raster
    AS $$ SELECT st_setbandnodatavalue($1, $2, $3, FALSE) $$
    LANGUAGE SQL;

-- This function can not be STRICT, because nodatavalue can be NULL indicating that no nodata value should be set
CREATE OR REPLACE FUNCTION st_setbandnodatavalue(rast raster, nodatavalue float8)
    RETURNS raster
    AS $$ SELECT st_setbandnodatavalue($1, 1, $2, FALSE) $$
    LANGUAGE SQL;

CREATE OR REPLACE FUNCTION st_setbandisnodata(rast raster, band integer)
    RETURNS raster
    AS 'MODULE_PATHNAME', 'RASTER_setBandIsNoData'
    LANGUAGE 'C' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_setbandisnodata(rast raster)
    RETURNS raster
    AS  $$ SELECT st_setbandisnodata($1, 1) $$
    LANGUAGE SQL IMMUTABLE STRICT;

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

CREATE OR REPLACE FUNCTION st_dumpaspolygons(rast raster, band integer)
    RETURNS SETOF geomval AS
    $$
    SELECT st_geomfromtext(wktgeomval.wktgeom, wktgeomval.srid), wktgeomval.val
    FROM dumpaswktpolygons($1, $2) AS wktgeomval;
    $$
    LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_dumpaspolygons(raster)
    RETURNS SETOF geomval AS
    $$
    SELECT st_geomfromtext(wktgeomval.wktgeom, wktgeomval.srid), wktgeomval.val
    FROM dumpaswktpolygons($1, 1) AS wktgeomval;
    $$
    LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_polygon(rast raster, band integer)
    RETURNS geometry AS
    $$
    SELECT st_union(f.geom) AS singlegeom
    FROM (SELECT (st_dumpaspolygons($1, $2)).geom AS geom) AS f;
    $$
    LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_polygon(raster)
    RETURNS geometry AS
    $$
    SELECT st_union(f.geom) AS singlegeom
    FROM (SELECT (st_dumpaspolygons($1, 1)).geom AS geom) AS f;
    $$
    LANGUAGE 'SQL' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_pixelaspolygon(rast raster, band integer, x integer, y integer)
    RETURNS geometry AS
    $$
    DECLARE
        w integer;
        h integer;
        scalex float8;
        scaley float8;
        skewx float8;
        skewy float8;
        x1 float8;
        y1 float8;
        x2 float8;
        y2 float8;
        x3 float8;
        y3 float8;
        x4 float8;
        y4 float8;
    BEGIN
        scalex := st_scalex(rast);
        skewx := st_skewy(rast);
        skewy := st_skewx(rast);
        scaley := st_scaley(rast);
        x1 := scalex * (x - 1) + skewx * (y - 1) + st_upperleftx(rast);
        y1 := scaley * (y - 1) + skewy * (x - 1) + st_upperlefty(rast);
        x2 := x1 + scalex;
        y2 := y1 + skewy;
        x3 := x1 + scalex + skewx;
        y3 := y1 + scaley + skewy;
        x4 := x1 + skewx;
        y4 := y1 + scaley;
        RETURN st_setsrid(st_makepolygon(st_makeline(ARRAY[st_makepoint(x1, y1),
                                                           st_makepoint(x2, y2),
                                                           st_makepoint(x3, y3),
                                                           st_makepoint(x4, y4),
                                                           st_makepoint(x1, y1)]
                                                    )
                                        ),
                          st_srid(rast)
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
        a := st_scalex(rast);
        d := st_skewy(rast);
        b := st_skewx(rast);
        e := st_scaley(rast);
        c := st_upperleftx(rast);
        f := st_upperlefty(rast);
        IF ( b * d - a * e = 0 ) THEN
            RAISE EXCEPTION 'Attempting to compute raster coordinate on a raster with scale equal to 0';
        END IF;
        xr := (b * (yw - f) - e * (xw - c)) / (b * d - a * e);
        RETURN floor(xr) + 1;
    END;
    $$
    LANGUAGE 'plpgsql' IMMUTABLE STRICT;

---------------------------------------------------------------------------------
-- ST_World2RasteCoordX(rast raster, xw float8)
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
        a := st_scalex(rast);
        d := st_skewy(rast);
        b := st_skewx(rast);
        e := st_scaley(rast);
        c := st_upperleftx(rast);
        f := st_upperlefty(rast);
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
        a := st_scalex(rast);
        d := st_skewy(rast);
        b := st_skewx(rast);
        e := st_scaley(rast);
        c := st_upperleftx(rast);
        f := st_upperlefty(rast);
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
        a := st_scalex(rast);
        d := st_skewy(rast);
        b := st_skewx(rast);
        e := st_scaley(rast);
        c := st_upperleftx(rast);
        f := st_upperlefty(rast);
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
        a := st_scalex(rast);
        b := st_skewx(rast);
        c := st_upperleftx(rast);
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
        a := st_scalex(rast);
        b := st_skewx(rast);
        c := st_upperleftx(rast);
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
        d := st_skewy(rast);
        e := st_scaley(rast);
        f := st_upperlefty(rast);
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
        d := st_skewy(rast);
        e := st_scaley(rast);
        f := st_upperlefty(rast);
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
    RESTRICT = geometry_gist_sel, JOIN = geometry_gist_joinsel
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
-- Raster/Geometry Spatial Relationship
-----------------------------------------------------------------------

-----------------------------------------------------------------------
-- _st_intersects(geomin geometry, rast raster, band integer, hasnodata boolean)
-- If hasnodata is true, check for the presence of withvalue pixels in the area
-- shared by the raster and the geometry. If only nodata value pixels are found, the
-- geometry does not intersect with the raster.
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION _st_intersects(geomin geometry, rast raster, band integer, hasnodata boolean)
    RETURNS boolean AS
    $$
    DECLARE
        nodata float8 := 0.0;
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
    BEGIN

        -- Get the intersection between with the geometry.
        -- We will search for withvalue pixel only in this area.
        geomintersect := st_intersection(geomin, st_convexhull(rast));

--RAISE NOTICE 'geomintersect1=%', astext(geomintersect);

        -- If the intersection is empty, return false
        IF st_isempty(geomintersect) THEN
            RETURN FALSE;
        END IF;

        -- If the band does not have a nodatavalue, there is no need to search for with value pixels
        IF NOT hasnodata OR st_bandnodatavalue(rast, band) IS NULL THEN
            RETURN TRUE;
        END IF;

        -- We create a minimalistic buffer around the intersection in order to scan every pixels
        -- that would touch the edge or intersect with the geometry
        scale := st_scalex(rast) + st_skewy(rast);
        geomintersect := st_buffer(geomintersect, scale / 1000000);

--RAISE NOTICE 'geomintersect2=%', astext(geomintersect);

        -- Find the world coordinates of the bounding box of the intersecting area
        x1w := st_xmin(geomintersect);
        y1w := st_ymin(geomintersect);
        x2w := st_xmax(geomintersect);
        y2w := st_ymax(geomintersect);
        nodata := st_bandnodatavalue(rast, band);

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
        x1 := int4smaller(int4larger(x1, 1), st_width(rast));
        y1 := int4smaller(int4larger(y1, 1), st_height(rast));

        -- Also make sure the range does not exceed the width and height of the raster.
        -- This can happen when world coordinate are exactly on the lower right border
        -- of the raster.
        x2 := int4smaller(x2, st_width(rast));
        y2 := int4smaller(y2, st_height(rast));

--RAISE NOTICE 'x1=%, y1=%, x2=%, y2=%', x1, y1, x2, y2;

        -- Search exhaustively for withvalue pixel on a moving 3x3 grid
        -- (very often more efficient than searching on a mere 1x1 grid)
        FOR xinc in 0..2 LOOP
            FOR yinc in 0..2 LOOP
                FOR x IN x1+xinc..x2 BY 3 LOOP
                    FOR y IN y1+yinc..y2 BY 3 LOOP
                        -- Check first if the pixel intersects with the geometry. Often many won't.
                        bintersect := NOT st_isempty(st_intersection(st_pixelaspolygon(rast, band, x, y), geomin));

                        IF bintersect THEN
                            -- If the pixel really intersects, check its value. Return TRUE if with value.
                            pixelval := st_value(rast, band, x, y);
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
    $$
    LANGUAGE 'plpgsql' IMMUTABLE STRICT;


-- This function can not be STRICT
CREATE OR REPLACE FUNCTION st_intersects(geometry, raster, integer)
    RETURNS boolean AS
    $$ SELECT $1 && $2 AND _st_intersects($1, $2, $3, TRUE);
    $$ LANGUAGE 'SQL' IMMUTABLE;

-- This function can not be STRICT
CREATE OR REPLACE FUNCTION st_intersects(raster, integer, geometry)
    RETURNS boolean AS
    $$ SELECT st_intersects($3, $1, $2);
    $$ LANGUAGE 'SQL' IMMUTABLE;

-- This function can not be STRICT
CREATE OR REPLACE FUNCTION st_intersects(geometry, raster)
    RETURNS boolean AS
    $$ SELECT st_intersects($1, $2, 1);
    $$ LANGUAGE 'SQL' IMMUTABLE;

-- This function can not be STRICT
CREATE OR REPLACE FUNCTION st_intersects(raster, geometry)
    RETURNS boolean AS
    $$ SELECT st_intersects($2, $1, 1);
    $$ LANGUAGE 'SQL' IMMUTABLE;

-- This function can not be STRICT
CREATE OR REPLACE FUNCTION st_intersects(geometry, raster, integer, boolean)
    RETURNS boolean AS
    $$ SELECT $1 && $2 AND _st_intersects($1, $2, $3, $4);
    $$ LANGUAGE 'SQL' IMMUTABLE;

-- This function can not be STRICT
CREATE OR REPLACE FUNCTION st_intersects(raster, integer, boolean, geometry)
    RETURNS boolean AS
    $$ SELECT st_intersects($4, $1, $2, $3);
    $$ LANGUAGE 'SQL' IMMUTABLE;

-- This function can not be STRICT
CREATE OR REPLACE FUNCTION st_intersects(geometry, raster, boolean)
    RETURNS boolean AS
    $$ SELECT st_intersects($1, $2, 1, $3);
    $$ LANGUAGE 'SQL' IMMUTABLE;

-- This function can not be STRICT
CREATE OR REPLACE FUNCTION st_intersects(raster, boolean, geometry)
    RETURNS boolean AS
    $$ SELECT st_intersects($3, $1, 1, $2);
    $$ LANGUAGE 'SQL' IMMUTABLE;

-----------------------------------------------------------------------
-- _st_intersection(geom geometry, rast raster, band integer)
-- Returns a geometry set that represents the shared portion of the
-- provided geometry and the geometries produced by the vectorization of rast.
-- Return an empty geometry if the geometry does not intersect with the
-- raster.
-- Raster nodata value areas are not vectorized and hence do not intersect
-- with any geometries.
-----------------------------------------------------------------------
CREATE OR REPLACE FUNCTION ST_Intersection(geomin geometry, rast raster, band integer)
  RETURNS SETOF geomval AS
$BODY$
    DECLARE
        intersects boolean := FALSE;
    BEGIN
        intersects := ST_Intersects(geomin, rast, band);
        IF intersects THEN
            -- Return the intersections of the geometry with the vectorized parts of
            -- the raster and the values associated with those parts, if really their
            -- intersection is not empty.
            RETURN QUERY
                SELECT intgeom,
                       val
                FROM (SELECT ST_Intersection((gv).geom, geomin) AS intgeom,
                             (gv).val
                      FROM ST_DumpAsPolygons(rast, band) gv
                      WHERE ST_Intersects((gv).geom, geomin)
                     ) foo
                WHERE NOT ST_IsEmpty(intgeom);
        ELSE
            -- If the geometry does not intersect with the raster, return an empty
            -- geometry and a null value
            RETURN QUERY
                SELECT emptygeom,
                       NULL::float8
                FROM ST_GeomCollFromText('GEOMETRYCOLLECTION EMPTY', ST_SRID($1)) emptygeom;
        END IF;
    END;
    $BODY$
  LANGUAGE 'plpgsql' IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_intersection(rast raster, geom geometry)
    RETURNS SETOF geomval AS
    $$
        SELECT (gv).geom, (gv).val FROM st_intersection($2, $1, 1) gv;
    $$
    LANGUAGE SQL IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_intersection(geom geometry, rast raster)
    RETURNS SETOF geomval AS
    $$
        SELECT (gv).geom, (gv).val FROM st_intersection($1, $2, 1) gv;
    $$
    LANGUAGE SQL IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION st_intersection(rast raster, band integer, geom geometry)
    RETURNS SETOF geomval AS
    $$
        SELECT (gv).geom, (gv).val FROM st_intersection($3, $1, $2) gv;
    $$
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
        r_column )
    ) WITH OIDS;


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
      o_column, overview_factor )
    ) WITH OIDS;


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

        RAISE DEBUG 'Parameters: catalog=%, schema=%, table=%, column=%, srid=%, pixel_types=%, out_db=%, regular_blocking=%, nodata_values=%, scale_x=%, scale_y=%, blocksize_x=%, blocksize_y=%',
                     p_catalog_name, p_schema_name, p_table_name, p_column_name, p_srid, p_pixel_types, p_out_db, p_regular_blocking, p_nodata_values, p_scale_x, p_scale_y, p_blocksize_x, p_blocksize_y;

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
            RAISE DEBUG 'Verified SRID = %', p_srid;
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
                    RAISE DEBUG 'Identified pixel type %', p_pixel_types[npti];
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
            RAISE DEBUG '%', sql;
            EXECUTE sql INTO real_schema;

            IF ( real_schema IS NULL ) THEN
                RAISE EXCEPTION 'Schema % is not a valid schemaname', quote_literal(p_schema_name);
                RETURN 'fail';
            END IF;
        END IF;

        IF ( real_schema IS NULL ) THEN
            RAISE DEBUG 'Detecting schema';
            sql := 'SELECT n.nspname AS schemaname '
                || 'FROM pg_catalog.pg_class c '
                || 'JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace '
                || 'WHERE c.relkind = ' || quote_literal('r')
                || ' AND n.nspname NOT IN ('
                || quote_literal('pg_catalog') || ', ' || quote_literal('pg_toast') || ')'
                || ' AND pg_catalog.pg_table_is_visible(c.oid)'
                || ' AND c.relname = ' || quote_literal(p_table_name);
            RAISE DEBUG '%', sql;
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
        RAISE DEBUG '%', sql;
        EXECUTE sql;


        -- Delete stale record in RASTER_COLUMNS (if any)
        sql := 'DELETE FROM raster_columns '
            || ' WHERE r_table_catalog = ' || quote_literal('')
            || ' AND r_table_schema = ' || quote_literal(real_schema)
            || ' AND r_table_name = ' || quote_literal(p_table_name)
            || ' AND r_column = ' || quote_literal(p_column_name);
        RAISE DEBUG '%', sql;
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
        RAISE DEBUG '%', sql;
        EXECUTE sql;


        -- Add CHECK for SRID
        sql := 'ALTER TABLE '
            || quote_ident(real_schema) || '.' || quote_ident(p_table_name)
            || ' ADD CONSTRAINT '
            || quote_ident('enforce_srid_' || p_column_name)
            || ' CHECK (ST_SRID(' || quote_ident(p_column_name)
            || ') = ' || p_srid::text || ')';
        RAISE DEBUG '%', sql;
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
