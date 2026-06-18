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
	deprecated_suffix TEXT := pg_catalog.concat('_deprecated_by_postgis_', deprecated_in_version);
  deprecated_suffix_len INT := pg_catalog.length(deprecated_suffix);
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
	AND p.proname ILIKE function_name
	AND pg_catalog.pg_function_is_visible(p.oid)
	AND pg_catalog.pg_get_function_identity_arguments(p.oid) ILIKE function_arguments
	INTO matching_function;

	IF matching_function.oid IS NOT NULL THEN
		sql := pg_catalog.format('ALTER FUNCTION %s RENAME TO %I',
			matching_function.oid::regprocedure,
			--limit to 63 characters
			pg_catalog.concat(pg_catalog.left(matching_function.proname,63-deprecated_suffix_len), deprecated_suffix)
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
    WHERE castsource::pg_catalog.regtype::text ILIKE sourcetype
		AND casttarget::pg_catalog.regtype::text ILIKE targettype;
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

	new_name := pg_catalog.concat(cast_source, '_', cast_target);
	sql := pg_catalog.format(
		'DROP CAST IF EXISTS (topology.%s AS %s)',
		cast_source,
		cast_target
	);
  RAISE DEBUG 'SQL: %', sql;
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
	deprecated_suffix_len INT := pg_catalog.length(deprecated_suffix);
BEGIN

	-- Check if the deprecated function exists
	BEGIN

		SELECT *
		FROM pg_catalog.pg_proc
		WHERE oid = function_signature::regprocedure
		INTO STRICT proc;

	EXCEPTION
	WHEN NO_DATA_FOUND OR UNDEFINED_FUNCTION OR UNDEFINED_OBJECT THEN
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
		-- limit to 63 characters
		pg_catalog.concat(pg_catalog.left(proc.proname,63-deprecated_suffix_len), deprecated_suffix)
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
	domain_oid OID;
	domain_schema TEXT := 'topology';
	domain_constraint RECORD;
	domain_column RECORD;
	domain_default RECORD;
	sql TEXT;
	-- We need the base types of the old and new types - if multidimensional (int[][]), we need just one dimension at most
	old_base_type TEXT := pg_catalog.regexp_replace(old_domain_type, E'(\\[\\])+$', '[]');
	new_base_type TEXT := pg_catalog.regexp_replace(new_domain_type, E'(\\[\\])+$', '[]');
	array_dims INT := (SELECT count(*) FROM pg_catalog.regexp_matches (old_domain_type, E'\\[\\]', 'g'));
BEGIN
	SELECT t.oid
	FROM pg_catalog.pg_type AS t
	WHERE typnamespace::regnamespace::text = domain_schema
	AND typname::text ILIKE domain_name
	-- Databases upgraded by the broken 3.6 path can already have
	-- the new catalog base type while old-width row values remain.
	AND typbasetype IN (old_base_type::regtype::oid, new_base_type::regtype::oid)
	AND typndims = array_dims
	INTO domain_oid;

	IF domain_oid IS NOT NULL
  	THEN
		BEGIN
			FOR domain_constraint IN
				SELECT conname
				FROM pg_catalog.pg_constraint
				WHERE contypid = domain_oid
			LOOP
				sql := pg_catalog.format(
					'ALTER DOMAIN %I.%I DROP CONSTRAINT IF EXISTS %I',
					domain_schema,
					domain_name,
					domain_constraint.conname
				);
				EXECUTE sql;
			END LOOP;

			UPDATE pg_catalog.pg_type
			SET typbasetype = new_base_type::regtype::oid, typndims = array_dims
			WHERE oid = domain_oid;

			-- Child tables are skipped by the row rewrite below because
			-- ALTER TABLE on the root recurses to them, but child defaults
			-- are independent pg_attrdef entries and must be rebuilt too.
			FOR domain_default IN
				SELECT
					a.attrelid,
					a.attname,
					pg_catalog.pg_get_expr(d.adbin, d.adrelid) AS default_expr
				FROM pg_catalog.pg_attribute AS a
				JOIN pg_catalog.pg_class AS c
					ON c.oid = a.attrelid
				JOIN pg_catalog.pg_attrdef AS d
					ON d.adrelid = a.attrelid
					AND d.adnum = a.attnum
				WHERE a.atttypid = domain_oid
				AND a.attnum > 0
				AND NOT a.attisdropped
				AND a.attgenerated = ''
				AND c.relkind IN ('r', 'p')
				AND EXISTS (
					SELECT 1
					FROM pg_catalog.pg_inherits AS i
					JOIN pg_catalog.pg_attribute AS pa
						ON pa.attrelid = i.inhparent
						AND pa.attname = a.attname
						AND pa.atttypid = a.atttypid
						AND pa.attnum > 0
						AND NOT pa.attisdropped
					WHERE i.inhrelid = a.attrelid
				)
			LOOP
				sql := pg_catalog.format(
					'ALTER TABLE %s ALTER COLUMN %I DROP DEFAULT',
					domain_default.attrelid::regclass,
					domain_default.attname
				);
				EXECUTE sql;

				sql := pg_catalog.format(
					'ALTER TABLE %s ALTER COLUMN %I SET DEFAULT ((%s)::text::%s::%I.%I)',
					domain_default.attrelid::regclass,
					domain_default.attname,
					domain_default.default_expr,
					new_domain_type,
					domain_schema,
					domain_name
				);
				EXECUTE sql;
			END LOOP;

			-- Existing table rows are still physically stored with the old
			-- array element width. Whole-array output can mask that fact, so
			-- force every dependent table column through text into the new
			-- array representation before the domain constraints are restored.
			FOR domain_column IN
				SELECT
					a.attrelid,
					a.attname,
					pg_catalog.pg_get_expr(d.adbin, d.adrelid) AS default_expr
				FROM pg_catalog.pg_attribute AS a
				JOIN pg_catalog.pg_class AS c
					ON c.oid = a.attrelid
				LEFT JOIN pg_catalog.pg_attrdef AS d
					ON d.adrelid = a.attrelid
					AND d.adnum = a.attnum
				WHERE a.atttypid = domain_oid
				AND a.attnum > 0
				AND NOT a.attisdropped
				AND a.attgenerated = ''
				AND c.relkind IN ('r', 'p')
				AND NOT EXISTS (
					SELECT 1
					FROM pg_catalog.pg_inherits AS i
					JOIN pg_catalog.pg_attribute AS pa
						ON pa.attrelid = i.inhparent
						AND pa.attname = a.attname
						AND pa.atttypid = a.atttypid
						AND pa.attnum > 0
						AND NOT pa.attisdropped
					WHERE i.inhrelid = a.attrelid
				)
			LOOP
				IF domain_column.default_expr IS NOT NULL THEN
					sql := pg_catalog.format(
						'ALTER TABLE %s ALTER COLUMN %I DROP DEFAULT',
						domain_column.attrelid::regclass,
						domain_column.attname
					);
					EXECUTE sql;
				END IF;

				sql := pg_catalog.format(
					'ALTER TABLE %s ALTER COLUMN %I TYPE %I.%I USING (%I::text::%s::%I.%I)',
					domain_column.attrelid::regclass,
					domain_column.attname,
					domain_schema,
					domain_name,
					domain_column.attname,
					new_domain_type,
					domain_schema,
					domain_name
				);
				EXECUTE sql;

				IF domain_column.default_expr IS NOT NULL THEN
					sql := pg_catalog.format(
						'ALTER TABLE %s ALTER COLUMN %I SET DEFAULT ((%s)::text::%s::%I.%I)',
						domain_column.attrelid::regclass,
						domain_column.attname,
						domain_column.default_expr,
						new_domain_type,
						domain_schema,
						domain_name
					);
					EXECUTE sql;
				END IF;
			END LOOP;

			RAISE INFO 'Upgraded % from % to %', domain_name, old_domain_type, new_domain_type;
		EXCEPTION
		WHEN others THEN
			GET STACKED DIAGNOSTICS detail := PG_EXCEPTION_DETAIL;
			RAISE WARNING 'Could not modify % from % to %, got % (%)',
				domain_name, old_domain_type, new_domain_type, SQLERRM, SQLSTATE USING DETAIL = detail;
			RETURN;
		END;
	ELSE
		RAISE DEBUG 'Deprecated domain (topology.% with type %) does not exist', domain_name, old_domain_type;
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
	tmp text;
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
			join pg_catalog.pg_class on pg_class.oid = pg_type.typrelid
			join pg_catalog.pg_attribute on pg_attribute.attrelid = pg_class.oid
			join pg_catalog.pg_type as pg_attr_type on pg_attr_type.oid = pg_attribute.atttypid
		where pg_type.typname::regtype::text ILIKE type_name
		and pg_type.typnamespace::regnamespace::text = 'topology'
		and attname::text ILIKE attr_name
		and pg_attr_type.typname::regtype::text ILIKE old_attr_type
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
		sql := pg_catalog.format(
			'ALTER TYPE topology.%s ADD ATTRIBUTE %s %s',
			type_name,
			temp_attr_name,
			new_attr_type
		);
		RAISE DEBUG 'SQL: %', sql;
		EXECUTE sql;

		-- Copy the attributes from the temp attribute to the existing attribute
		sql := 'UPDATE pg_attribute AS tgt SET ';

		BEGIN
			-- add in tgt = src assignments for the update
			SELECT pg_catalog.string_agg(pg_catalog.format('%I = src.%I', column_name, column_name), ', ')
			FROM information_schema.columns
			INTO tmp
			WHERE table_name = 'pg_attribute'
			AND column_name <> ALL (excluded_columns)
			AND column_name NOT LIKE 'oid%';
			sql := sql || tmp;

			-- add FROM/WHERE clause
			tmp := pg_catalog.format('tgt.attrelid = %s AND tgt.attname = %L AND src.attrelid = %s AND src.attname = %L',
				proc.attrelid,
				attr_name,
				proc.attrelid,
				temp_attr_name
			);
			sql := pg_catalog.concat(sql, ' FROM pg_attribute AS src WHERE ', tmp);

			RAISE DEBUG 'SQL: %', sql;
			EXECUTE sql;
		END;

		-- Delete temp attribute since we cannot alter the type
		sql := pg_catalog.format(
			'DELETE FROM pg_attribute WHERE attname = %L AND attrelid = %s',
			temp_attr_name,
			proc.attrelid
		);
		RAISE DEBUG 'SQL: %', sql;
		EXECUTE sql;

		-- Reset the number of attributes in pg_class else postgres will complain
		sql := pg_catalog.format(
				'UPDATE pg_class SET relnatts = %s WHERE oid = %s',
				proc.relnatts,
				proc.attrelid
		);
		RAISE DEBUG 'SQL: %', sql;
		EXECUTE sql;

		RAISE INFO 'Upgraded %.% from % to %', type_name, attr_name, old_attr_type, new_attr_type;
	EXCEPTION
	WHEN others THEN
		GET STACKED DIAGNOSTICS detail := PG_EXCEPTION_DETAIL;
		RAISE WARNING 'Could not modify %.% from % to %, got % (%)',
			type_name, attr_name, old_attr_type, new_attr_type, SQLERRM, SQLSTATE
		USING DETAIL = detail;
		RETURN;
	END;
END;
$$ LANGUAGE 'plpgsql';

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

		IF pg_catalog.LENGTH(default_value) > 0 THEN
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

		-- RAISE INFO 'SQL: %', sql;
		EXECUTE sql;


		RAISE INFO 'Modified % added %(%)', tab::text, columnname, data_type;
  END IF;
END;
$BODY$ LANGUAGE 'plpgsql';
