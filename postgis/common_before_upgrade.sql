-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--
--
-- PostGIS - Spatial Types for PostgreSQL
-- http://postgis.net
--
-- Copyright (C) 2011-2012 Sandro Santilli <strk@kbt.io>
-- Copyright (C) 2010-2013 Regina Obe <lr@pcorp.us>
-- Copyright (C) 2009      Paul Ramsey <pramsey@cleverelephant.ca>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
--
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--
-- This file contains utility functions for use by upgrade scripts
-- Changes to this file affect *upgrade*.sql script.
--
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

--
-- Helper function to drop functions when they match the full signature
-- Requires schema, name and __exact arguments__ as extracted from pg_catalog.pg_get_function_arguments
-- You can extract the old function arguments using a query like:
-- SELECT  p.oid as oid,
--                 n.nspname as schema,
--                 p.proname as name,
--                 pg_catalog.pg_get_function_arguments(p.oid) as arguments
--         FROM pg_catalog.pg_proc p
--         LEFT JOIN pg_catalog.pg_namespace n ON n.oid = p.pronamespace
--         WHERE
--                 pg_catalog.LOWER(n.nspname) = pg_catalog.LOWER('public') AND
--                 pg_catalog.LOWER(p.proname) = pg_catalog.LOWER('ST_AsGeoJson')
--         ORDER BY 1, 2, 3, 4;
CREATE OR REPLACE FUNCTION _postgis_drop_function_if_needed(
	function_name text,
	function_arguments text,
	deprecated_in_version text DEFAULT 'xxx'
) RETURNS void AS $$
DECLARE
	sql text;
	postgis_namespace OID;
	matching_function pg_catalog.pg_proc;
	detail TEXT;
	deprecated_suffix TEXT := '_deprecated_by_postgis_' || deprecated_in_version;
BEGIN

	-- Fetch install namespace for PostGIS
	SELECT n.oid
	FROM pg_catalog.pg_proc p
	JOIN pg_catalog.pg_namespace n ON p.pronamespace = n.oid
	WHERE proname = 'postgis_full_version'
	INTO postgis_namespace;

	-- Find a function matching the given signature
	SELECT *
	FROM pg_catalog.pg_proc p
	WHERE pronamespace = postgis_namespace
	AND pg_catalog.LOWER(p.proname) = pg_catalog.LOWER(function_name)
	AND pg_catalog.pg_function_is_visible(p.oid)
	AND pg_catalog.LOWER(pg_catalog.pg_get_function_identity_arguments(p.oid)) ~ pg_catalog.LOWER(function_arguments)
	INTO matching_function;

	IF matching_function.oid IS NOT NULL THEN
		sql := format('ALTER FUNCTION %s RENAME TO %I',
			matching_function.oid::regprocedure,
			matching_function.proname || deprecated_suffix
		);
		RAISE WARNING 'SQL query: %', sql;
		BEGIN
			EXECUTE sql;
		EXCEPTION
			WHEN OTHERS THEN
				GET STACKED DIAGNOSTICS detail := PG_EXCEPTION_DETAIL;
				RAISE EXCEPTION 'Could not rename deprecated function %, got % (%)',
					matching_function, SQLERRM, SQLSTATE
				USING DETAIL = detail;
		END;
	END IF;

END;
$$ LANGUAGE plpgsql;
