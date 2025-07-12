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
--
-- Requires name and __exact arguments__ as extracted from pg_catalog.pg_get_function_arguments
-- You can extract the old function arguments using a query like:
--
--  SELECT pg_get_function_arguments('st_intersection(geometry,geometry,float8)'::regprocedure);
--
-- Or:
--
--  SELECT pg_get_function_arguments(oid) as args
--  FROM pg_catalog.pg_proc
--  WHERE proname = 'st_asgeojson'
--
CREATE OR REPLACE FUNCTION _postgis_drop_function_by_identity(
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
  deprecated_suffix_len INT := length(deprecated_suffix);
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
	AND pg_catalog.LOWER(pg_catalog.pg_get_function_identity_arguments(p.oid)) = pg_catalog.LOWER(function_arguments)
	INTO matching_function;

	IF matching_function.oid IS NOT NULL THEN
		sql := format('ALTER FUNCTION %s RENAME TO %I',
			matching_function.oid::regprocedure,
			SUBSTRING(matching_function.proname,1,63-deprecated_suffix_len) || deprecated_suffix --limit to 63 characters
		);
		RAISE DEBUG 'SQL query: %', sql;
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

CREATE OR REPLACE FUNCTION _postgis_drop_cast_by_types(
  sourcetype text,
	targettype text,
	deprecated_in_version text DEFAULT 'xxx'
) RETURNS void AS $$
DECLARE
	sql text;
	detail TEXT;
	cast_source TEXT;
	cast_target TEXT;
	new_name TEXT;
BEGIN
	-- Check if the cast exists
	BEGIN
		SELECT castsource::pg_catalog.regtype::text, casttarget::pg_catalog.regtype::text
		INTO STRICT cast_source, cast_target
		FROM pg_catalog.pg_cast
    WHERE castsource::pg_catalog.regtype::text = lower(sourcetype)
		AND casttarget::pg_catalog.regtype::text = lower(targettype);
	EXCEPTION
	WHEN NO_DATA_FOUND THEN
		RAISE DEBUG 'Deprecated cast (topology.% as %) does not exist', sourcetype, targettype;
		RETURN;
	WHEN others THEN
		GET STACKED DIAGNOSTICS detail := PG_EXCEPTION_DETAIL;
		RAISE WARNING 'Could not check deprecated cast (% as %) existence, got % (%), assuming it does not exist',
			sourcetype, targettype, SQLERRM, SQLSTATE
		USING DETAIL = detail;
		RETURN;
	END;

	new_name := cast_source || '_' || cast_target;
	sql := pg_catalog.format(
		'DROP CAST IF EXISTS (topology.%s AS %s)',
		cast_source,
		cast_target
	);
  RAISE INFO 'SQL: %', sql;
	EXECUTE sql;

END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION _postgis_drop_function_by_signature(
  function_signature text,
  deprecated_in_version text DEFAULT 'xxx'
) RETURNS void AS $$
DECLARE
	sql text;
	detail TEXT;
	newname TEXT;
	proc RECORD;
	deprecated_suffix TEXT := '_deprecated_by_postgis_' || deprecated_in_version;
	deprecated_suffix_len INT := length(deprecated_suffix);
BEGIN

	-- Check if the deprecated function exists
	BEGIN

		SELECT *
		FROM pg_catalog.pg_proc
		WHERE oid = function_signature::regprocedure
		INTO STRICT proc;

	EXCEPTION
	WHEN NO_DATA_FOUND THEN
		RAISE DEBUG 'Deprecated function % does not exist', function_signature;
		RETURN;
	WHEN others THEN
		GET STACKED DIAGNOSTICS detail := PG_EXCEPTION_DETAIL;
		RAISE WARNING 'Could not check deprecated function % existence, got % (%), assuming it does not exist',
			function_signature, SQLERRM, SQLSTATE
		USING DETAIL = detail;
		RETURN;
	END;

	sql := pg_catalog.format(
		'ALTER FUNCTION %s RENAME TO %I',
		proc.oid::regprocedure,
		SUBSTRING(proc.proname,1,63-deprecated_suffix_len) || deprecated_suffix -- limit to 63 characters
	);
	EXECUTE sql;

END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION _postgis_topology_upgrade_domain_type(
  domain_name text,
	old_domain_type text,
	new_domain_type text,
	deprecated_in_version text DEFAULT 'xxx'
) RETURNS void AS $$
DECLARE
	detail TEXT;
	-- We need the base types of the old and new types - if multidimensional (int[][]), we need just one dimension at most
	old_base_type TEXT := regexp_replace(old_domain_type, E'(\\[\\])+$', '[]');
	new_base_type TEXT := regexp_replace(new_domain_type, E'(\\[\\])+$', '[]');
	array_dims INT := (SELECT count(*) FROM regexp_matches (old_domain_type, E'\\[\\]', 'g'));
BEGIN
	IF EXISTS (SELECT 1 FROM pg_catalog.pg_type AS t
		WHERE  typnamespace::regnamespace::text = 'topology'
		AND typname::text = lower(domain_name)
		AND typbasetype::regtype::text = lower(old_base_type)
    AND typndims = array_dims)
  	THEN
		BEGIN
			-- HACK: We need to swap out the base type and number of dimensions
			UPDATE pg_catalog.pg_type
			SET typbasetype = new_base_type::regtype::oid, typndims = array_dims
			WHERE typnamespace::regnamespace::text = 'topology'
			AND typname::text = lower(domain_name)
			AND typbasetype::regtype::text = lower(old_base_type)
			AND typndims = array_dims;

			RAISE INFO 'Upgraded % from % to %', domain_name, old_domain_type, new_domain_type;
		EXCEPTION
		WHEN others THEN
			GET STACKED DIAGNOSTICS detail := PG_EXCEPTION_DETAIL;
			RAISE INFO 'Could not modify % from % to %, got % (%)',
				domain_name, old_domain_type, new_domain_type, SQLERRM, SQLSTATE USING DETAIL = detail;
			RETURN;
		END;
	ELSE
		RAISE INFO 'Deprecated domain (topology.% with type %) does not exist', domain_name, old_domain_type;
		RETURN;
	END IF;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION _postgis_topology_upgrade_user_type_attribute(
  type_name text,
	attr_name text,
	old_attr_type text,
	new_attr_type text,
	deprecated_in_version text DEFAULT 'xxx'
) RETURNS void AS $$
DECLARE
	sql text;
	detail TEXT;
	num_updated integer;
	proc RECORD;
	temp_attr_name TEXT := attr_name || '_tmp';
	colname text;
	excluded_columns text[] := ARRAY['attnum', 'attname', 'attrelid'];
BEGIN
	BEGIN
		-- Check if the attribute exists
		-- Get the attribute id and number of attributes (so we can reset relnatts after adding/deleting temp attribute)
		SELECT pg_type.typrelid attrelid, pg_class.relnatts
		FROM pg_catalog.pg_type
			join pg_class on pg_class.oid = pg_type.typrelid
			join pg_attribute on pg_attribute.attrelid = pg_class.oid
			join pg_type as pg_attr_type on pg_attr_type.oid = pg_attribute.atttypid
		where pg_type.typname::regtype::text = lower(type_name)
		and pg_type.typnamespace::regnamespace::text = 'topology'
		and attname::text = lower(attr_name)
		and pg_attr_type.typname::regtype::text = lower(old_attr_type)
		INTO STRICT proc;

	EXCEPTION
	WHEN NO_DATA_FOUND THEN
		RAISE DEBUG 'Deprecated type (topology.% with attribute %(%)) does not exist', type_name, attr_name, old_attr_type;
		RETURN;
	WHEN others THEN
		GET STACKED DIAGNOSTICS detail := PG_EXCEPTION_DETAIL;
		RAISE WARNING 'Could not check deprecated type (topology.% with attribute %(%)) existence, got % (%), assuming it does not exist',
			type_name, attr_name, old_attr_type, SQLERRM, SQLSTATE
		USING DETAIL = detail;
		RETURN;
	END;

	BEGIN
		-- Add a temporary attribute with the required type
		-- This is the cleanest way to ensure the type is valid and all constraints are met
		sql := format(
			'ALTER TYPE topology.%s ADD ATTRIBUTE %s %s',
			type_name,
			temp_attr_name,
			new_attr_type
		);
		--RAISE INFO 'SQL: %', sql;
		EXECUTE sql;

		-- Copy the attributes from the temp attribute to the existing attribute
		sql := 'UPDATE pg_attribute AS tgt SET ';

		BEGIN
  		FOR colname IN
    	SELECT column_name
    	FROM information_schema.columns
    	WHERE table_name = 'pg_attribute'
      AND column_name <> ALL (excluded_columns)
  		LOOP
    		sql := sql || format('%I = src.%I, ', colname, colname);
  		END LOOP;

  		-- remove trailing comma and add FROM/WHERE clause
  		sql := left(sql, -2) || ' FROM pg_attribute AS src WHERE ' || format('tgt.attrelid = %s AND tgt.attname = %L AND src.attrelid = %s AND src.attname = %L',
				proc.attrelid,
				attr_name,
				proc.attrelid,
				temp_attr_name
			);

  		--RAISE INFO 'SQL: %', sql;
  		EXECUTE sql;
		END;

		-- Delete temp attribute since we cannot alter the type
		sql := format(
			'DELETE FROM pg_attribute WHERE attname = %L AND attrelid = %s',
			temp_attr_name,
			proc.attrelid
		);
		--RAISE INFO 'SQL: %', sql;
		EXECUTE sql;

		-- Reset the number of attributes in pg_class else postgres will complain
		sql := format(
				'UPDATE pg_class SET relnatts = %s WHERE oid = %s',
				proc.relnatts,
				proc.attrelid
		);
		--RAISE INFO 'SQL: %', sql;
		EXECUTE sql;

		--GET DIAGNOSTICS num_updated = ROW_COUNT;

		-- IF num_updated = 0 THEN
		-- 	RAISE WARNING 'No attributes updated for %.% from % to %', type_name, attr_name, old_attr_type, new_attr_type;
		-- 	RETURN;
		-- END IF;

		RAISE INFO 'Upgraded %.% from % to %', type_name, attr_name, old_attr_type, new_attr_type;
	EXCEPTION
	WHEN others THEN
		GET STACKED DIAGNOSTICS detail := PG_EXCEPTION_DETAIL;
		RAISE INFO 'Could not modify %.% from % to %, got % (%)',
			type_name, attr_name, old_attr_type, new_attr_type, SQLERRM, SQLSTATE
		USING DETAIL = detail;
		RETURN;
	END;
END;
$$ LANGUAGE plpgsql;

-- Add useslargeids to topology
CREATE OR REPLACE FUNCTION _postgis_add_column_to_table(
  tab regclass,
	columnname name,
	data_type text,
	is_not_null BOOLEAN,
	default_value text,
	deprecated_in_version text DEFAULT 'xxx'
	) RETURNS void
AS $BODY$
DECLARE
	schemaname TEXT;
	tablename TEXT;
	default_clause TEXT := '';
	null_clause TEXT := '';
	sql text;
BEGIN
	-- Check if the table exists and get its schema and name
	SELECT n.nspname::text, c.relname::text INTO schemaname, tablename
  FROM pg_class c
  JOIN pg_namespace n ON c.relnamespace = n.oid
  WHERE c.oid = tab;

  IF NOT EXISTS(SELECT 1 FROM information_schema.columns
    WHERE table_schema=schemaname AND table_name=tablename AND column_name=columnname)
  THEN
		IF is_not_null THEN
			null_clause := 'NOT NULL';
		END IF;

		IF LENGTH(default_value) > 0 THEN
			default_clause := 'DEFAULT ' || default_value;
		END IF;

		-- Add the column
		-- Postgres manages adding the column with default and not null correctly
		-- See https://www.postgresql.org/docs/current/sql-altertable.html
		sql := pg_catalog.format(
			'ALTER TABLE %s ADD COLUMN %s %s %s %s',
			tab::text,
			columnname,
			data_type,
			null_clause,
			default_clause
		);

		--RAISE INFO 'SQL: %', sql;
		EXECUTE sql;


		RAISE INFO 'Modified % added %(%)', tab::text, columnname, data_type;
  END IF;
END;
$BODY$ LANGUAGE 'plpgsql';
