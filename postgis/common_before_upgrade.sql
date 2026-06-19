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
	context TEXT;
	detail TEXT;
	domain_oid OID;
	domain_array_oid OID;
	domain_att_type_oids OID[];
	nested_topology_type_oids OID[];
	domain_schema TEXT := 'topology';
	domain_constraint RECORD;
	domain_constraint_defs JSONB := '[]'::JSONB;
	domain_column RECORD;
	domain_default_expr TEXT;
	domain_default RECORD;
	domain_generated_column RECORD;
	domain_generated_source_column RECORD;
	domain_skipped_column RECORD;
	sql TEXT;
	skipped_repair BOOLEAN := false;
	restored_domain_array_columns BOOLEAN := false;
	-- We need the base types of the old and new types - if multidimensional (int[][]), we need just one dimension at most
	old_base_type TEXT := pg_catalog.regexp_replace(old_domain_type, E'(\\[\\])+$', '[]');
	new_base_type TEXT := pg_catalog.regexp_replace(new_domain_type, E'(\\[\\])+$', '[]');
	array_dims INT := (SELECT count(*) FROM pg_catalog.regexp_matches (old_domain_type, E'\\[\\]', 'g'));
	marker_version TEXT := CASE
		WHEN deprecated_in_version ~ '^[0-9]+[.][0-9]+'
		THEN pg_catalog.concat(
			pg_catalog.split_part(deprecated_in_version, '.', 1),
			pg_catalog.lpad(pg_catalog.split_part(deprecated_in_version, '.', 2), 2, '0')
		)
		ELSE deprecated_in_version
	END;
	repair_marker TEXT := 'postgis-topology-domain-storage-repaired-' || marker_version;
	constraint_not_valid_marker TEXT := 'postgis-topology-domain-constraint-not-valid-by-repair-' || marker_version;
BEGIN
	SELECT
		t.oid,
		t.typarray,
		CASE
			WHEN t.typdefaultbin IS NOT NULL THEN pg_catalog.pg_get_expr(t.typdefaultbin, 0)
			ELSE NULL
		END
	FROM pg_catalog.pg_type AS t
	WHERE typnamespace::regnamespace::text = domain_schema
	AND typname::text ILIKE domain_name
	-- Databases upgraded by the broken 3.6 path can already have
	-- the new catalog base type while old-width row values remain.
	AND (
		typbasetype = old_base_type::regtype::oid
		OR (
			typbasetype = new_base_type::regtype::oid
			AND COALESCE(pg_catalog.obj_description(t.oid, 'pg_type'), '') NOT LIKE '%' || repair_marker || '%'
		)
	)
	AND typndims = array_dims
	INTO domain_oid, domain_array_oid, domain_default_expr;

	IF domain_oid IS NOT NULL THEN
		BEGIN
			domain_att_type_oids := pg_catalog.array_remove(ARRAY[domain_oid, domain_array_oid], 0::oid);

			-- Generated expressions and user-visible container types can hide
			-- topology domains behind a different atttypid. Use one recursive
			-- closure so every dependency check reasons about the same set of
			-- exact and nested domain-storage carriers.
			WITH RECURSIVE nested_type(type_oid) AS (
				SELECT pg_catalog.unnest(domain_att_type_oids)
				UNION
				SELECT nested_candidate.type_oid
				FROM nested_type
				JOIN LATERAL (
					SELECT t.oid AS type_oid
					FROM pg_catalog.pg_type AS t
					WHERE t.typbasetype = nested_type.type_oid
					OR t.typelem = nested_type.type_oid
					UNION
					SELECT ct.oid AS type_oid
					FROM pg_catalog.pg_attribute AS ta
					JOIN pg_catalog.pg_class AS tc
						ON tc.oid = ta.attrelid
						AND tc.relkind = 'c'
					JOIN pg_catalog.pg_type AS ct
						ON ct.typrelid = tc.oid
					WHERE ta.atttypid = nested_type.type_oid
					AND ta.attnum > 0
					AND NOT ta.attisdropped
				) AS nested_candidate ON true
			)
			SELECT pg_catalog.array_agg(type_oid)
			FROM nested_type
				INTO nested_topology_type_oids;

				FOR domain_constraint IN
					SELECT
						conname,
						pg_catalog.pg_get_constraintdef(oid) AS constraint_def,
						pg_catalog.obj_description(oid, 'pg_constraint') LIKE '%' || constraint_not_valid_marker || '%' AS not_valid_by_repair
					FROM pg_catalog.pg_constraint
					WHERE contypid = domain_oid
				LOOP
					domain_constraint_defs := domain_constraint_defs || pg_catalog.jsonb_build_array(
						pg_catalog.jsonb_build_object(
							'name', domain_constraint.conname,
							'definition', domain_constraint.constraint_def,
							'not_valid_by_repair', domain_constraint.not_valid_by_repair
						)
					);
					sql := pg_catalog.format(
						'ALTER DOMAIN %I.%I DROP CONSTRAINT IF EXISTS %I',
					domain_schema,
					domain_name,
					domain_constraint.conname
				);
					EXECUTE sql;
				END LOOP;

			-- Older upgrade sources may not have constraints introduced after
			-- the original domain was created.  Treat those canonical topology
			-- constraints like captured constraints so incomplete repairs keep
				-- them NOT VALID. Complete repairs install the fresh-domain
				-- constraint set as validated unless rewritten domain-array
				-- columns still depend on the domain and force NOT VALID restore.
			IF pg_catalog.lower(domain_name) = 'topoelement' THEN
				IF NOT EXISTS (
					SELECT 1
					FROM pg_catalog.jsonb_array_elements(domain_constraint_defs) AS value
					WHERE value->>'name' = 'dimensions'
				) THEN
					domain_constraint_defs := domain_constraint_defs || pg_catalog.jsonb_build_array(
						pg_catalog.jsonb_build_object(
							'name', 'dimensions',
							'definition', 'CHECK (array_upper(VALUE, 2) IS NULL AND array_upper(VALUE, 1) = 2)',
							'not_valid_by_repair', true
						)
					);
				END IF;

				IF NOT EXISTS (
					SELECT 1
					FROM pg_catalog.jsonb_array_elements(domain_constraint_defs) AS value
					WHERE value->>'name' = 'type_range'
				) THEN
					domain_constraint_defs := domain_constraint_defs || pg_catalog.jsonb_build_array(
						pg_catalog.jsonb_build_object(
							'name', 'type_range',
							'definition', 'CHECK (VALUE[2] > 0)',
							'not_valid_by_repair', true
						)
					);
				END IF;

				IF NOT EXISTS (
					SELECT 1
					FROM pg_catalog.jsonb_array_elements(domain_constraint_defs) AS value
					WHERE value->>'name' = 'lower_dimension'
				) THEN
					domain_constraint_defs := domain_constraint_defs || pg_catalog.jsonb_build_array(
						pg_catalog.jsonb_build_object(
							'name', 'lower_dimension',
							'definition', 'CHECK (array_lower(VALUE, 1) = 1)',
							'not_valid_by_repair', true
						)
					);
				END IF;
			ELSIF pg_catalog.lower(domain_name) = 'topoelementarray' THEN
				IF NOT EXISTS (
					SELECT 1
					FROM pg_catalog.jsonb_array_elements(domain_constraint_defs) AS value
					WHERE value->>'name' = 'type_range'
				) THEN
					domain_constraint_defs := domain_constraint_defs || pg_catalog.jsonb_build_array(
						pg_catalog.jsonb_build_object(
							'name', 'type_range',
							'definition', 'CHECK (array_upper(VALUE, 2) = 2 AND array_upper(VALUE, 3) IS NULL)',
							'not_valid_by_repair', true
						)
					);
				END IF;
			END IF;

			UPDATE pg_catalog.pg_type
			SET typbasetype = new_base_type::regtype::oid, typndims = array_dims
			WHERE oid = domain_oid;

			IF domain_default_expr IS NOT NULL THEN
				sql := pg_catalog.format(
					'ALTER DOMAIN %I.%I DROP DEFAULT',
					domain_schema,
					domain_name
				);
				EXECUTE sql;

				CREATE TEMP TABLE IF NOT EXISTS pg_temp._postgis_topology_domain_defaults (
					domain_schema TEXT,
					domain_name TEXT,
					default_expr TEXT,
					new_domain_type TEXT
				) ON COMMIT DROP;

				INSERT INTO pg_temp._postgis_topology_domain_defaults
					VALUES (
						domain_schema,
						domain_name,
						domain_default_expr,
						new_domain_type
					);
			END IF;

			-- Rewrite-rule, row-level security policy, trigger,
			-- SQL-function/procedure, publication-column, and
			-- generated-expression dependencies make ALTER COLUMN TYPE fail.
			-- Skip those columns and withhold the repair marker so a later
			-- manual cleanup can retry instead of rolling back unrelated
			-- domain repairs.
			-- PostgreSQL 14 records generated-expression dependencies as
			-- pg_class attribute dependencies; later releases also expose
			-- pg_attrdef dependencies.
			FOR domain_skipped_column IN
				SELECT
					a.attrelid,
					a.attname,
					c.relkind,
					dependent_view.view_relid,
					dependent_view.view_relkind,
					dependent_view.rulename,
					dependent_policy.policy_oid,
					dependent_policy.policy_name,
					dependent_trigger.trigger_oid,
					dependent_trigger.trigger_name,
					dependent_function.function_oid,
					dependent_function.function_name,
					dependent_publication.publication_relid,
					dependent_publication.publication_name,
					dependent_generated_column.attrelid AS generated_attrelid,
					dependent_generated_column.attname AS generated_attname,
					dependent_constraint.constraint_oid,
					dependent_constraint.constraint_name,
					dependent_index.index_oid,
					dependent_index.index_name
				FROM pg_catalog.pg_attribute AS a
				JOIN pg_catalog.pg_class AS c
					ON c.oid = a.attrelid
				LEFT JOIN LATERAL (
					SELECT
						vc.oid AS view_relid,
						vc.relkind AS view_relkind,
						rw.rulename
					FROM pg_catalog.pg_depend AS dep
					JOIN pg_catalog.pg_rewrite AS rw
						ON rw.oid = dep.objid
					JOIN pg_catalog.pg_class AS vc
						ON vc.oid = rw.ev_class
					WHERE dep.refobjid = a.attrelid
					AND dep.refobjsubid IN (0, a.attnum)
					LIMIT 1
				) AS dependent_view ON true
				LEFT JOIN LATERAL (
					SELECT
						p.oid AS policy_oid,
						p.polname AS policy_name
					FROM pg_catalog.pg_depend AS dep
					JOIN pg_catalog.pg_policy AS p
						ON p.oid = dep.objid
					WHERE dep.classid = 'pg_catalog.pg_policy'::regclass
					AND dep.refobjid = a.attrelid
					AND dep.refobjsubid IN (0, a.attnum)
					LIMIT 1
				) AS dependent_policy ON true
				LEFT JOIN LATERAL (
					SELECT
						t.oid AS trigger_oid,
						t.tgname AS trigger_name
					FROM pg_catalog.pg_depend AS dep
					JOIN pg_catalog.pg_trigger AS t
						ON t.oid = dep.objid
					WHERE dep.classid = 'pg_catalog.pg_trigger'::regclass
					AND dep.refobjid = a.attrelid
					AND dep.refobjsubid IN (0, a.attnum)
					LIMIT 1
				) AS dependent_trigger ON true
				LEFT JOIN LATERAL (
					SELECT
						p.oid AS function_oid,
						p.oid::regprocedure::text AS function_name
					FROM pg_catalog.pg_depend AS dep
					JOIN pg_catalog.pg_proc AS p
						ON p.oid = dep.objid
					WHERE dep.classid = 'pg_catalog.pg_proc'::regclass
					AND dep.refobjid = a.attrelid
					AND dep.refobjsubid IN (0, a.attnum)
					LIMIT 1
				) AS dependent_function ON true
				LEFT JOIN LATERAL (
					SELECT
						pr.oid AS publication_relid,
						pub.pubname AS publication_name
					FROM pg_catalog.pg_depend AS dep
					JOIN pg_catalog.pg_publication_rel AS pr
						ON pr.oid = dep.objid
					JOIN pg_catalog.pg_publication AS pub
						ON pub.oid = pr.prpubid
					WHERE dep.classid = 'pg_catalog.pg_publication_rel'::regclass
					AND dep.refobjid = a.attrelid
					AND dep.refobjsubid = a.attnum
					LIMIT 1
				) AS dependent_publication ON true
				LEFT JOIN LATERAL (
					SELECT
						generated_dependency.attrelid,
						generated_dependency.attname
					FROM (
						SELECT
							ga.attrelid,
							ga.attname
						FROM pg_catalog.pg_depend AS dep
						JOIN pg_catalog.pg_attrdef AS gd
							ON gd.oid = dep.objid
						JOIN pg_catalog.pg_attribute AS ga
							ON ga.attrelid = gd.adrelid
							AND ga.attnum = gd.adnum
						WHERE dep.classid = 'pg_catalog.pg_attrdef'::regclass
						AND dep.refobjid = a.attrelid
						AND dep.refobjsubid IN (0, a.attnum)
						AND ga.attgenerated <> ''
						UNION ALL
						SELECT
							ga.attrelid,
							ga.attname
						FROM pg_catalog.pg_depend AS dep
						JOIN pg_catalog.pg_attribute AS ga
							ON ga.attrelid = dep.objid
							AND ga.attnum = dep.objsubid
						WHERE dep.classid = 'pg_catalog.pg_class'::regclass
						AND dep.refobjid = a.attrelid
						AND dep.refobjsubid IN (0, a.attnum)
						AND ga.attgenerated <> ''
					) AS generated_dependency
					LIMIT 1
				) AS dependent_generated_column ON true
				LEFT JOIN LATERAL (
					SELECT
						con.oid AS constraint_oid,
						con.conname AS constraint_name
					FROM pg_catalog.pg_depend AS dep
					JOIN pg_catalog.pg_constraint AS con
						ON con.oid = dep.objid
					WHERE dep.classid = 'pg_catalog.pg_constraint'::regclass
					AND dep.refobjid = a.attrelid
					AND dep.refobjsubid IN (0, a.attnum)
					AND a.atttypid = domain_array_oid
					LIMIT 1
				) AS dependent_constraint ON true
				LEFT JOIN LATERAL (
					SELECT
						ic.oid AS index_oid,
						ic.relname AS index_name
					FROM pg_catalog.pg_depend AS dep
					JOIN pg_catalog.pg_class AS ic
						ON ic.oid = dep.objid
					JOIN pg_catalog.pg_index AS ix
						ON ix.indexrelid = ic.oid
					WHERE dep.classid = 'pg_catalog.pg_class'::regclass
					AND dep.refobjid = a.attrelid
					AND dep.refobjsubid IN (0, a.attnum)
					AND ic.relkind = 'i'
					AND a.atttypid = domain_array_oid
					LIMIT 1
				) AS dependent_index ON true
				WHERE a.atttypid = ANY(domain_att_type_oids)
				AND a.attnum > 0
				AND NOT a.attisdropped
				AND (
					c.relkind = 'm'
					OR dependent_view.view_relid IS NOT NULL
					OR dependent_policy.policy_oid IS NOT NULL
					OR dependent_trigger.trigger_oid IS NOT NULL
					OR dependent_function.function_oid IS NOT NULL
					OR dependent_publication.publication_relid IS NOT NULL
					OR dependent_generated_column.attrelid IS NOT NULL
					OR dependent_constraint.constraint_oid IS NOT NULL
					OR dependent_index.index_oid IS NOT NULL
				)
			LOOP
				skipped_repair := true;
				IF domain_skipped_column.relkind = 'm' THEN
					RAISE WARNING 'Could not rewrite materialized view column %.% for topology.%; refresh or recreate the materialized view after upgrade',
						domain_skipped_column.attrelid::regclass,
						domain_skipped_column.attname,
						domain_name;
				ELSIF domain_skipped_column.view_relkind = 'v' THEN
					RAISE WARNING 'Could not rewrite %.% for topology.% because view % depends on it; drop and recreate the view, then repair the column after upgrade',
						domain_skipped_column.attrelid::regclass,
						domain_skipped_column.attname,
						domain_name,
						domain_skipped_column.view_relid::regclass;
				ELSIF domain_skipped_column.view_relkind IN ('r', 'p') THEN
					RAISE WARNING 'Could not rewrite %.% for topology.% because rule % on % depends on it; drop and recreate the rule, then repair the column after upgrade',
						domain_skipped_column.attrelid::regclass,
						domain_skipped_column.attname,
						domain_name,
						domain_skipped_column.rulename,
						domain_skipped_column.view_relid::regclass;
				ELSIF domain_skipped_column.policy_oid IS NOT NULL THEN
					RAISE WARNING 'Could not rewrite %.% for topology.% because row-level security policy % depends on it; drop and recreate the policy, then repair the column after upgrade',
						domain_skipped_column.attrelid::regclass,
						domain_skipped_column.attname,
						domain_name,
						domain_skipped_column.policy_name;
				ELSIF domain_skipped_column.trigger_oid IS NOT NULL THEN
					RAISE WARNING 'Could not rewrite %.% for topology.% because trigger % depends on it; drop and recreate the trigger, then repair the column after upgrade',
						domain_skipped_column.attrelid::regclass,
						domain_skipped_column.attname,
						domain_name,
						domain_skipped_column.trigger_name;
				ELSIF domain_skipped_column.function_oid IS NOT NULL THEN
					RAISE WARNING 'Could not rewrite %.% for topology.% because function or procedure % depends on it; drop and recreate the routine, then repair the column after upgrade',
						domain_skipped_column.attrelid::regclass,
						domain_skipped_column.attname,
						domain_name,
						domain_skipped_column.function_name;
				ELSIF domain_skipped_column.publication_relid IS NOT NULL THEN
					RAISE WARNING 'Could not rewrite %.% for topology.% because publication % depends on it; drop and recreate the publication relation entry, then repair the column after upgrade',
						domain_skipped_column.attrelid::regclass,
						domain_skipped_column.attname,
						domain_name,
						domain_skipped_column.publication_name;
				ELSIF domain_skipped_column.generated_attrelid IS NOT NULL THEN
					RAISE WARNING 'Could not rewrite %.% for topology.% because generated column %.% depends on it; drop and recreate the generated column, then repair the column after upgrade',
						domain_skipped_column.attrelid::regclass,
						domain_skipped_column.attname,
						domain_name,
						domain_skipped_column.generated_attrelid::regclass,
						domain_skipped_column.generated_attname;
				ELSIF domain_skipped_column.constraint_oid IS NOT NULL THEN
					RAISE WARNING 'Could not rewrite %.% for topology.% because constraint % depends on its domain array type; drop and recreate the constraint, then repair the column after upgrade',
						domain_skipped_column.attrelid::regclass,
						domain_skipped_column.attname,
						domain_name,
						domain_skipped_column.constraint_name;
				ELSIF domain_skipped_column.index_oid IS NOT NULL THEN
					RAISE WARNING 'Could not rewrite %.% for topology.% because index % depends on its domain array type; drop and recreate the index, then repair the column after upgrade',
						domain_skipped_column.attrelid::regclass,
						domain_skipped_column.attname,
						domain_name,
						domain_skipped_column.index_name;
				ELSE
					RAISE WARNING 'Could not rewrite %.% for topology.% because materialized view % depends on it; refresh or recreate the materialized view and repair the column after upgrade',
						domain_skipped_column.attrelid::regclass,
						domain_skipped_column.attname,
						domain_name,
						domain_skipped_column.view_relid::regclass;
				END IF;
			END LOOP;

			-- User domains and composite types can hide topology domains behind
			-- another column type OID, so the exact-OID rewrite below cannot
			-- prove that all old-width storage was repaired.
			FOR domain_skipped_column IN
				SELECT DISTINCT
					a.attrelid,
					a.attname,
					a.atttypid::regtype AS nested_type_name
				FROM pg_catalog.pg_attribute AS a
				JOIN pg_catalog.pg_class AS c
					ON c.oid = a.attrelid
				JOIN pg_catalog.unnest(nested_topology_type_oids) AS nt(type_oid)
					ON nt.type_oid = a.atttypid
				WHERE a.atttypid <> ALL(domain_att_type_oids)
				AND a.attnum > 0
				AND NOT a.attisdropped
				AND c.relkind IN ('r', 'p', 'm')
			LOOP
				skipped_repair := true;
				RAISE WARNING 'Could not rewrite %.% for topology.% because its type % contains that domain; move the value through text into a freshly created type after upgrade',
					domain_skipped_column.attrelid::regclass,
					domain_skipped_column.attname,
					domain_name,
					domain_skipped_column.nested_type_name;
			END LOOP;

			FOR domain_generated_source_column IN
				SELECT
					a.attrelid,
					a.attname,
					generated_source.attrelid AS source_attrelid,
					generated_source.attname AS source_attname
				FROM pg_catalog.pg_attribute AS a
				JOIN pg_catalog.pg_class AS c
					ON c.oid = a.attrelid
				JOIN LATERAL (
					SELECT
						source_dependency.attrelid,
						source_dependency.attname
					FROM (
						SELECT
							sa.attrelid,
							sa.attname
						FROM pg_catalog.pg_attrdef AS gd
						JOIN pg_catalog.pg_depend AS dep
							ON dep.classid = 'pg_catalog.pg_attrdef'::regclass
							AND dep.objid = gd.oid
						JOIN pg_catalog.pg_attribute AS sa
							ON sa.attrelid = dep.refobjid
							AND sa.attnum = dep.refobjsubid
						WHERE gd.adrelid = a.attrelid
						AND gd.adnum = a.attnum
						AND sa.atttypid = ANY(nested_topology_type_oids)
						AND sa.attnum > 0
						AND NOT sa.attisdropped
						AND sa.attnum <> a.attnum
						UNION ALL
						SELECT
							sa.attrelid,
							sa.attname
						FROM pg_catalog.pg_depend AS dep
						JOIN pg_catalog.pg_attribute AS sa
							ON sa.attrelid = dep.refobjid
							AND sa.attnum = dep.refobjsubid
						WHERE dep.classid = 'pg_catalog.pg_class'::regclass
						AND dep.objid = a.attrelid
						AND dep.objsubid = a.attnum
						AND sa.atttypid = ANY(nested_topology_type_oids)
						AND sa.attnum > 0
						AND NOT sa.attisdropped
						AND sa.attnum <> a.attnum
					) AS source_dependency
					LIMIT 1
				) AS generated_source ON true
				WHERE a.atttypid = ANY(domain_att_type_oids)
				AND a.attnum > 0
				AND NOT a.attisdropped
				AND a.attgenerated = 's'
				AND c.relkind IN ('r', 'p')
			LOOP
				skipped_repair := true;
				RAISE WARNING 'Could not rewrite generated column %.% for topology.% because it reads unrepaired column %.%; drop and recreate the generated column after repairing its source column',
					domain_generated_source_column.attrelid::regclass,
					domain_generated_source_column.attname,
					domain_name,
					domain_generated_source_column.source_attrelid::regclass,
					domain_generated_source_column.source_attname;
			END LOOP;

			-- Some table columns cannot enter the row rewrite below, but their
			-- defaults are independent pg_attrdef entries and can still be
			-- rebuilt through the new array storage. Nested carrier defaults
			-- need the same treatment so they do not keep creating old-width
			-- topology domain values after their storage rewrite is skipped.
			FOR domain_default IN
				SELECT
					a.attrelid,
					a.attname,
					pg_catalog.pg_get_expr(d.adbin, d.adrelid) AS default_expr,
					CASE
						WHEN a.atttypid = domain_array_oid THEN
							pg_catalog.format('%I.%I[]', domain_schema, domain_name)
						WHEN a.atttypid <> ALL(domain_att_type_oids) THEN
							a.atttypid::regtype::text
						ELSE
							pg_catalog.format('%I.%I', domain_schema, domain_name)
					END AS target_domain_type,
					CASE
						WHEN a.atttypid = domain_array_oid THEN
							'pg_catalog.text[]'
						WHEN a.atttypid <> ALL(domain_att_type_oids) THEN
							a.atttypid::regtype::text
						ELSE
							new_domain_type
					END AS storage_cast_type
				FROM pg_catalog.pg_attribute AS a
				JOIN pg_catalog.pg_class AS c
					ON c.oid = a.attrelid
				JOIN pg_catalog.pg_attrdef AS d
					ON d.adrelid = a.attrelid
					AND d.adnum = a.attnum
				WHERE (
					a.atttypid = ANY(domain_att_type_oids)
					OR (
						a.atttypid = ANY(nested_topology_type_oids)
						AND a.atttypid <> ALL(domain_att_type_oids)
					)
				)
				AND a.attnum > 0
				AND NOT a.attisdropped
				AND a.attgenerated = ''
				AND c.relkind IN ('r', 'p')
				AND (
					(
						a.atttypid = ANY(nested_topology_type_oids)
						AND a.atttypid <> ALL(domain_att_type_oids)
					)
					OR EXISTS (
						SELECT 1
						FROM pg_catalog.pg_depend AS dep
						JOIN pg_catalog.pg_rewrite AS rw
							ON rw.oid = dep.objid
						WHERE dep.refobjid = a.attrelid
						AND dep.refobjsubid IN (0, a.attnum)
					)
						OR EXISTS (
							SELECT 1
							FROM pg_catalog.pg_depend AS dep
							WHERE dep.classid = 'pg_catalog.pg_policy'::regclass
							AND dep.refobjid = a.attrelid
							AND dep.refobjsubid IN (0, a.attnum)
						)
						OR EXISTS (
							SELECT 1
							FROM pg_catalog.pg_depend AS dep
							WHERE dep.classid = 'pg_catalog.pg_trigger'::regclass
							AND dep.refobjid = a.attrelid
							AND dep.refobjsubid IN (0, a.attnum)
						)
						OR (
							a.atttypid = domain_array_oid
							AND EXISTS (
								SELECT 1
								FROM pg_catalog.pg_depend AS dep
								WHERE dep.classid = 'pg_catalog.pg_constraint'::regclass
								AND dep.refobjid = a.attrelid
								AND dep.refobjsubid IN (0, a.attnum)
							)
						)
						OR (
							a.atttypid = domain_array_oid
							AND EXISTS (
								SELECT 1
								FROM pg_catalog.pg_depend AS dep
								JOIN pg_catalog.pg_class AS ic
									ON ic.oid = dep.objid
									AND ic.relkind = 'i'
								JOIN pg_catalog.pg_index AS ix
									ON ix.indexrelid = ic.oid
								WHERE dep.classid = 'pg_catalog.pg_class'::regclass
								AND dep.refobjid = a.attrelid
								AND dep.refobjsubid IN (0, a.attnum)
							)
						)
						OR EXISTS (
							SELECT 1
							FROM pg_catalog.pg_depend AS dep
							WHERE dep.classid = 'pg_catalog.pg_proc'::regclass
							AND dep.refobjid = a.attrelid
							AND dep.refobjsubid IN (0, a.attnum)
						)
						OR EXISTS (
							SELECT 1
							FROM pg_catalog.pg_depend AS dep
							WHERE dep.classid = 'pg_catalog.pg_publication_rel'::regclass
							AND dep.refobjid = a.attrelid
							AND dep.refobjsubid = a.attnum
						)
						OR EXISTS (
							SELECT 1
							FROM (
							SELECT 1
							FROM pg_catalog.pg_depend AS dep
							JOIN pg_catalog.pg_attrdef AS gd
								ON gd.oid = dep.objid
							JOIN pg_catalog.pg_attribute AS ga
								ON ga.attrelid = gd.adrelid
								AND ga.attnum = gd.adnum
							WHERE dep.classid = 'pg_catalog.pg_attrdef'::regclass
							AND dep.refobjid = a.attrelid
							AND dep.refobjsubid IN (0, a.attnum)
							AND ga.attgenerated <> ''
							UNION ALL
							SELECT 1
							FROM pg_catalog.pg_depend AS dep
							JOIN pg_catalog.pg_attribute AS ga
								ON ga.attrelid = dep.objid
								AND ga.attnum = dep.objsubid
							WHERE dep.classid = 'pg_catalog.pg_class'::regclass
							AND dep.refobjid = a.attrelid
							AND dep.refobjsubid IN (0, a.attnum)
							AND ga.attgenerated <> ''
						) AS generated_dependency
					)
						OR EXISTS (
							WITH RECURSIVE inherited_relid(attrelid) AS (
								SELECT i.inhrelid
							FROM pg_catalog.pg_inherits AS i
							WHERE i.inhparent = a.attrelid
							UNION ALL
							SELECT i.inhrelid
							FROM pg_catalog.pg_inherits AS i
							JOIN inherited_relid AS ir
								ON ir.attrelid = i.inhparent
						)
						SELECT 1
						FROM inherited_relid AS i
							JOIN pg_catalog.pg_attribute AS ca
								ON ca.attrelid = i.attrelid
								AND ca.attname = a.attname
								AND ca.atttypid = a.atttypid
								AND ca.attnum > 0
								AND NOT ca.attisdropped
							JOIN pg_catalog.pg_depend AS dep
								ON dep.refobjid = ca.attrelid
								AND dep.refobjsubid IN (0, ca.attnum)
							WHERE dep.classid = 'pg_catalog.pg_policy'::regclass
						)
						OR EXISTS (
							WITH RECURSIVE inherited_relid(attrelid) AS (
								SELECT i.inhrelid
								FROM pg_catalog.pg_inherits AS i
								WHERE i.inhparent = a.attrelid
								UNION ALL
								SELECT i.inhrelid
								FROM pg_catalog.pg_inherits AS i
								JOIN inherited_relid AS ir
									ON ir.attrelid = i.inhparent
						)
						SELECT 1
						FROM inherited_relid AS i
						JOIN pg_catalog.pg_attribute AS ca
							ON ca.attrelid = i.attrelid
							AND ca.attname = a.attname
							AND ca.atttypid = a.atttypid
							AND ca.attnum > 0
							AND NOT ca.attisdropped
						JOIN pg_catalog.pg_depend AS dep
							ON dep.refobjid = ca.attrelid
							AND (
								(
									dep.classid IN (
										'pg_catalog.pg_publication_rel'::regclass
									)
									AND dep.refobjsubid = ca.attnum
								)
								OR (
									dep.classid = 'pg_catalog.pg_trigger'::regclass
									AND dep.refobjsubid IN (0, ca.attnum)
								)
								OR (
									dep.classid = 'pg_catalog.pg_proc'::regclass
									AND dep.refobjsubid IN (0, ca.attnum)
								)
							)
					)
						OR EXISTS (
							WITH RECURSIVE inherited_relid(attrelid) AS (
								SELECT i.inhrelid
							FROM pg_catalog.pg_inherits AS i
							WHERE i.inhparent = a.attrelid
							UNION ALL
							SELECT i.inhrelid
							FROM pg_catalog.pg_inherits AS i
							JOIN inherited_relid AS ir
								ON ir.attrelid = i.inhparent
						)
						SELECT 1
						FROM inherited_relid AS i
						JOIN pg_catalog.pg_attribute AS ca
							ON ca.attrelid = i.attrelid
							AND ca.attname = a.attname
							AND ca.atttypid = a.atttypid
							AND ca.attnum > 0
							AND NOT ca.attisdropped
						JOIN pg_catalog.pg_depend AS dep
							ON dep.refobjid = ca.attrelid
							AND dep.refobjsubid IN (0, ca.attnum)
						JOIN pg_catalog.pg_rewrite AS rw
							ON rw.oid = dep.objid
					)
					OR EXISTS (
						WITH RECURSIVE inherited_relid(attrelid) AS (
							SELECT i.inhrelid
							FROM pg_catalog.pg_inherits AS i
							WHERE i.inhparent = a.attrelid
							UNION ALL
							SELECT i.inhrelid
							FROM pg_catalog.pg_inherits AS i
							JOIN inherited_relid AS ir
								ON ir.attrelid = i.inhparent
						)
						SELECT 1
						FROM inherited_relid AS i
						JOIN pg_catalog.pg_attribute AS ca
							ON ca.attrelid = i.attrelid
							AND ca.attname = a.attname
							AND ca.atttypid = a.atttypid
							AND ca.attnum > 0
							AND NOT ca.attisdropped
						JOIN pg_catalog.pg_depend AS dep
							ON dep.refobjid = ca.attrelid
							AND dep.refobjsubid IN (0, ca.attnum)
						JOIN pg_catalog.pg_attrdef AS gd
							ON gd.oid = dep.objid
						JOIN pg_catalog.pg_attribute AS ga
							ON ga.attrelid = gd.adrelid
							AND ga.attnum = gd.adnum
						WHERE dep.classid = 'pg_catalog.pg_attrdef'::regclass
						AND ga.attgenerated <> ''
						UNION ALL
						SELECT 1
						FROM inherited_relid AS i
						JOIN pg_catalog.pg_attribute AS ca
							ON ca.attrelid = i.attrelid
							AND ca.attname = a.attname
							AND ca.atttypid = a.atttypid
							AND ca.attnum > 0
								AND NOT ca.attisdropped
							JOIN pg_catalog.pg_depend AS dep
								ON dep.refobjid = ca.attrelid
								AND dep.refobjsubid IN (0, ca.attnum)
							JOIN pg_catalog.pg_attribute AS ga
								ON ga.attrelid = dep.objid
								AND ga.attnum = dep.objsubid
							WHERE dep.classid = 'pg_catalog.pg_class'::regclass
							AND ga.attgenerated <> ''
						)
						OR (
							a.atttypid = domain_array_oid
							AND EXISTS (
								WITH RECURSIVE inherited_relid(attrelid) AS (
									SELECT i.inhrelid
									FROM pg_catalog.pg_inherits AS i
									WHERE i.inhparent = a.attrelid
									UNION ALL
									SELECT i.inhrelid
									FROM pg_catalog.pg_inherits AS i
									JOIN inherited_relid AS ir
										ON ir.attrelid = i.inhparent
								)
								SELECT 1
								FROM inherited_relid AS i
								JOIN pg_catalog.pg_attribute AS ca
									ON ca.attrelid = i.attrelid
									AND ca.attname = a.attname
									AND ca.atttypid = a.atttypid
									AND ca.attnum > 0
									AND NOT ca.attisdropped
								JOIN pg_catalog.pg_depend AS dep
									ON dep.classid = 'pg_catalog.pg_constraint'::regclass
									AND dep.refobjid = ca.attrelid
									AND dep.refobjsubid IN (0, ca.attnum)
							)
						)
						OR (
							a.atttypid = domain_array_oid
							AND EXISTS (
								WITH RECURSIVE inherited_relid(attrelid) AS (
									SELECT i.inhrelid
									FROM pg_catalog.pg_inherits AS i
									WHERE i.inhparent = a.attrelid
									UNION ALL
									SELECT i.inhrelid
									FROM pg_catalog.pg_inherits AS i
									JOIN inherited_relid AS ir
										ON ir.attrelid = i.inhparent
								)
								SELECT 1
								FROM inherited_relid AS i
								JOIN pg_catalog.pg_attribute AS ca
									ON ca.attrelid = i.attrelid
									AND ca.attname = a.attname
									AND ca.atttypid = a.atttypid
									AND ca.attnum > 0
									AND NOT ca.attisdropped
								JOIN pg_catalog.pg_depend AS dep
									ON dep.classid = 'pg_catalog.pg_class'::regclass
									AND dep.refobjid = ca.attrelid
									AND dep.refobjsubid IN (0, ca.attnum)
								JOIN pg_catalog.pg_class AS ic
									ON ic.oid = dep.objid
									AND ic.relkind = 'i'
								JOIN pg_catalog.pg_index AS ix
									ON ix.indexrelid = ic.oid
							)
						)
						OR EXISTS (
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
				)
			LOOP
				sql := pg_catalog.format(
					'ALTER TABLE %s ALTER COLUMN %I DROP DEFAULT',
					domain_default.attrelid::regclass,
					domain_default.attname
				);
				EXECUTE sql;

				sql := pg_catalog.format(
					'ALTER TABLE %s ALTER COLUMN %I SET DEFAULT ((%s)::text::%s::%s)',
					domain_default.attrelid::regclass,
					domain_default.attname,
					domain_default.default_expr,
					domain_default.storage_cast_type,
					domain_default.target_domain_type
				);
				EXECUTE sql;
			END LOOP;

			-- Existing table rows are still physically stored with the old
			-- array element width. Whole-array output can mask that fact, so
			-- force every dependent table column through text into the new
			-- array representation before the domain constraints are restored.
			-- Roots with blocked children are skipped as a unit because
			-- ALTER TABLE would recurse into those children and abort the
			-- entire domain repair.
			FOR domain_column IN
				SELECT
					a.attrelid,
					a.attname,
					pg_catalog.pg_get_expr(d.adbin, d.adrelid) AS default_expr,
					CASE
						WHEN a.atttypid = domain_array_oid THEN
							pg_catalog.format('%I.%I[]', domain_schema, domain_name)
						ELSE
							pg_catalog.format('%I.%I', domain_schema, domain_name)
					END AS target_domain_type,
					CASE
						WHEN a.atttypid = domain_array_oid THEN
							'pg_catalog.text[]'
						ELSE
							new_domain_type
					END AS storage_cast_type,
					a.atttypid = domain_array_oid AS is_domain_array
				FROM pg_catalog.pg_attribute AS a
				JOIN pg_catalog.pg_class AS c
					ON c.oid = a.attrelid
				LEFT JOIN pg_catalog.pg_attrdef AS d
					ON d.adrelid = a.attrelid
					AND d.adnum = a.attnum
				WHERE a.atttypid = ANY(domain_att_type_oids)
				AND a.attnum > 0
				AND NOT a.attisdropped
				AND a.attgenerated = ''
				AND c.relkind IN ('r', 'p')
				AND NOT EXISTS (
					SELECT 1
					FROM pg_catalog.pg_depend AS dep
					JOIN pg_catalog.pg_rewrite AS rw
						ON rw.oid = dep.objid
					JOIN pg_catalog.pg_class AS mc
						ON mc.oid = rw.ev_class
					WHERE dep.refobjid = a.attrelid
					AND dep.refobjsubid IN (0, a.attnum)
				)
					AND NOT EXISTS (
						SELECT 1
						FROM pg_catalog.pg_depend AS dep
						WHERE dep.classid = 'pg_catalog.pg_policy'::regclass
						AND dep.refobjid = a.attrelid
						AND dep.refobjsubid IN (0, a.attnum)
					)
					AND NOT EXISTS (
						SELECT 1
						FROM pg_catalog.pg_depend AS dep
						WHERE dep.classid = 'pg_catalog.pg_trigger'::regclass
						AND dep.refobjid = a.attrelid
						AND dep.refobjsubid IN (0, a.attnum)
					)
					AND NOT (
						a.atttypid = domain_array_oid
						AND EXISTS (
							SELECT 1
							FROM pg_catalog.pg_depend AS dep
							WHERE dep.classid = 'pg_catalog.pg_constraint'::regclass
							AND dep.refobjid = a.attrelid
							AND dep.refobjsubid IN (0, a.attnum)
						)
					)
					AND NOT (
						a.atttypid = domain_array_oid
						AND EXISTS (
							SELECT 1
							FROM pg_catalog.pg_depend AS dep
							JOIN pg_catalog.pg_class AS ic
								ON ic.oid = dep.objid
								AND ic.relkind = 'i'
							JOIN pg_catalog.pg_index AS ix
								ON ix.indexrelid = ic.oid
							WHERE dep.classid = 'pg_catalog.pg_class'::regclass
							AND dep.refobjid = a.attrelid
							AND dep.refobjsubid IN (0, a.attnum)
						)
					)
					AND NOT EXISTS (
						SELECT 1
						FROM pg_catalog.pg_depend AS dep
						WHERE dep.classid = 'pg_catalog.pg_proc'::regclass
						AND dep.refobjid = a.attrelid
						AND dep.refobjsubid IN (0, a.attnum)
					)
					AND NOT EXISTS (
						SELECT 1
						FROM pg_catalog.pg_depend AS dep
						WHERE dep.classid = 'pg_catalog.pg_publication_rel'::regclass
						AND dep.refobjid = a.attrelid
						AND dep.refobjsubid = a.attnum
					)
					AND NOT EXISTS (
						SELECT 1
						FROM (
						SELECT 1
						FROM pg_catalog.pg_depend AS dep
						JOIN pg_catalog.pg_attrdef AS gd
							ON gd.oid = dep.objid
						JOIN pg_catalog.pg_attribute AS ga
							ON ga.attrelid = gd.adrelid
							AND ga.attnum = gd.adnum
						WHERE dep.classid = 'pg_catalog.pg_attrdef'::regclass
						AND dep.refobjid = a.attrelid
						AND dep.refobjsubid IN (0, a.attnum)
						AND ga.attgenerated <> ''
						UNION ALL
						SELECT 1
						FROM pg_catalog.pg_depend AS dep
						JOIN pg_catalog.pg_attribute AS ga
							ON ga.attrelid = dep.objid
							AND ga.attnum = dep.objsubid
						WHERE dep.classid = 'pg_catalog.pg_class'::regclass
						AND dep.refobjid = a.attrelid
						AND dep.refobjsubid IN (0, a.attnum)
						AND ga.attgenerated <> ''
					) AS generated_dependency
				)
					AND NOT EXISTS (
						WITH RECURSIVE inherited_relid(attrelid) AS (
							SELECT i.inhrelid
						FROM pg_catalog.pg_inherits AS i
						WHERE i.inhparent = a.attrelid
						UNION ALL
						SELECT i.inhrelid
						FROM pg_catalog.pg_inherits AS i
						JOIN inherited_relid AS ir
							ON ir.attrelid = i.inhparent
					)
					SELECT 1
					FROM inherited_relid AS i
						JOIN pg_catalog.pg_attribute AS ca
							ON ca.attrelid = i.attrelid
							AND ca.attname = a.attname
							AND ca.atttypid = a.atttypid
							AND ca.attnum > 0
							AND NOT ca.attisdropped
						JOIN pg_catalog.pg_depend AS dep
							ON dep.refobjid = ca.attrelid
							AND dep.refobjsubid IN (0, ca.attnum)
						WHERE dep.classid = 'pg_catalog.pg_policy'::regclass
					)
					AND NOT EXISTS (
						WITH RECURSIVE inherited_relid(attrelid) AS (
							SELECT i.inhrelid
							FROM pg_catalog.pg_inherits AS i
							WHERE i.inhparent = a.attrelid
							UNION ALL
							SELECT i.inhrelid
							FROM pg_catalog.pg_inherits AS i
							JOIN inherited_relid AS ir
								ON ir.attrelid = i.inhparent
						)
						SELECT 1
						FROM inherited_relid AS i
						JOIN pg_catalog.pg_attribute AS ca
							ON ca.attrelid = i.attrelid
							AND ca.attname = a.attname
							AND ca.atttypid = a.atttypid
							AND ca.attnum > 0
							AND NOT ca.attisdropped
						JOIN pg_catalog.pg_depend AS dep
							ON dep.refobjid = ca.attrelid
							AND (
								(
									dep.classid IN (
										'pg_catalog.pg_publication_rel'::regclass
									)
									AND dep.refobjsubid = ca.attnum
								)
								OR (
									dep.classid = 'pg_catalog.pg_trigger'::regclass
									AND dep.refobjsubid IN (0, ca.attnum)
								)
								OR (
									dep.classid = 'pg_catalog.pg_proc'::regclass
									AND dep.refobjsubid IN (0, ca.attnum)
								)
							)
					)
					AND NOT EXISTS (
						WITH RECURSIVE inherited_relid(attrelid) AS (
							SELECT i.inhrelid
						FROM pg_catalog.pg_inherits AS i
						WHERE i.inhparent = a.attrelid
						UNION ALL
						SELECT i.inhrelid
						FROM pg_catalog.pg_inherits AS i
						JOIN inherited_relid AS ir
							ON ir.attrelid = i.inhparent
					)
					SELECT 1
					FROM inherited_relid AS i
					JOIN pg_catalog.pg_attribute AS ca
						ON ca.attrelid = i.attrelid
						AND ca.attname = a.attname
						AND ca.atttypid = a.atttypid
						AND ca.attnum > 0
						AND NOT ca.attisdropped
					JOIN pg_catalog.pg_depend AS dep
						ON dep.refobjid = ca.attrelid
						AND dep.refobjsubid IN (0, ca.attnum)
					JOIN pg_catalog.pg_rewrite AS rw
						ON rw.oid = dep.objid
				)
					AND NOT (
						a.atttypid = domain_array_oid
						AND EXISTS (
							WITH RECURSIVE inherited_relid(attrelid) AS (
								SELECT i.inhrelid
								FROM pg_catalog.pg_inherits AS i
								WHERE i.inhparent = a.attrelid
								UNION ALL
								SELECT i.inhrelid
								FROM pg_catalog.pg_inherits AS i
								JOIN inherited_relid AS ir
									ON ir.attrelid = i.inhparent
							)
							SELECT 1
							FROM inherited_relid AS i
							JOIN pg_catalog.pg_attribute AS ca
								ON ca.attrelid = i.attrelid
								AND ca.attname = a.attname
								AND ca.atttypid = a.atttypid
								AND ca.attnum > 0
								AND NOT ca.attisdropped
							JOIN pg_catalog.pg_depend AS dep
								ON dep.classid = 'pg_catalog.pg_constraint'::regclass
								AND dep.refobjid = ca.attrelid
								AND dep.refobjsubid IN (0, ca.attnum)
						)
					)
					AND NOT (
						a.atttypid = domain_array_oid
						AND EXISTS (
							WITH RECURSIVE inherited_relid(attrelid) AS (
								SELECT i.inhrelid
								FROM pg_catalog.pg_inherits AS i
								WHERE i.inhparent = a.attrelid
								UNION ALL
								SELECT i.inhrelid
								FROM pg_catalog.pg_inherits AS i
								JOIN inherited_relid AS ir
									ON ir.attrelid = i.inhparent
							)
							SELECT 1
							FROM inherited_relid AS i
							JOIN pg_catalog.pg_attribute AS ca
								ON ca.attrelid = i.attrelid
								AND ca.attname = a.attname
								AND ca.atttypid = a.atttypid
								AND ca.attnum > 0
								AND NOT ca.attisdropped
							JOIN pg_catalog.pg_depend AS dep
								ON dep.classid = 'pg_catalog.pg_class'::regclass
								AND dep.refobjid = ca.attrelid
								AND dep.refobjsubid IN (0, ca.attnum)
							JOIN pg_catalog.pg_class AS ic
								ON ic.oid = dep.objid
								AND ic.relkind = 'i'
							JOIN pg_catalog.pg_index AS ix
								ON ix.indexrelid = ic.oid
						)
					)
					AND NOT EXISTS (
						WITH RECURSIVE inherited_relid(attrelid) AS (
							SELECT i.inhrelid
						FROM pg_catalog.pg_inherits AS i
						WHERE i.inhparent = a.attrelid
						UNION ALL
						SELECT i.inhrelid
						FROM pg_catalog.pg_inherits AS i
						JOIN inherited_relid AS ir
							ON ir.attrelid = i.inhparent
					)
					SELECT 1
					FROM inherited_relid AS i
					JOIN pg_catalog.pg_attribute AS ca
						ON ca.attrelid = i.attrelid
						AND ca.attname = a.attname
						AND ca.atttypid = a.atttypid
						AND ca.attnum > 0
						AND NOT ca.attisdropped
					JOIN pg_catalog.pg_depend AS dep
						ON dep.refobjid = ca.attrelid
						AND dep.refobjsubid IN (0, ca.attnum)
					JOIN pg_catalog.pg_attrdef AS gd
						ON gd.oid = dep.objid
					JOIN pg_catalog.pg_attribute AS ga
						ON ga.attrelid = gd.adrelid
						AND ga.attnum = gd.adnum
					WHERE dep.classid = 'pg_catalog.pg_attrdef'::regclass
					AND ga.attgenerated <> ''
					UNION ALL
					SELECT 1
					FROM inherited_relid AS i
					JOIN pg_catalog.pg_attribute AS ca
						ON ca.attrelid = i.attrelid
						AND ca.attname = a.attname
						AND ca.atttypid = a.atttypid
						AND ca.attnum > 0
						AND NOT ca.attisdropped
					JOIN pg_catalog.pg_depend AS dep
						ON dep.refobjid = ca.attrelid
						AND dep.refobjsubid IN (0, ca.attnum)
					JOIN pg_catalog.pg_attribute AS ga
						ON ga.attrelid = dep.objid
						AND ga.attnum = dep.objsubid
					WHERE dep.classid = 'pg_catalog.pg_class'::regclass
					AND ga.attgenerated <> ''
				)
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

				IF domain_column.is_domain_array THEN
					CREATE TEMP TABLE IF NOT EXISTS pg_temp._postgis_topology_domain_array_columns (
						attrelid OID,
						attname NAME,
						target_domain_type TEXT,
						default_expr TEXT
					) ON COMMIT DROP;

					INSERT INTO pg_temp._postgis_topology_domain_array_columns
						VALUES (
							domain_column.attrelid,
							domain_column.attname,
							domain_column.target_domain_type,
							domain_column.default_expr
						);
				END IF;

				IF domain_column.is_domain_array THEN
					-- text[] keeps NULL domain-array elements representable while
					-- the column is detached from the topology domain.
					sql := pg_catalog.format(
						'ALTER TABLE %s ALTER COLUMN %I TYPE pg_catalog.text[] USING (%I::pg_catalog.text[])',
						domain_column.attrelid::regclass,
						domain_column.attname,
						domain_column.attname
					);
				ELSE
					sql := pg_catalog.format(
						'ALTER TABLE %s ALTER COLUMN %I TYPE %s USING (%I::text::%s)',
						domain_column.attrelid::regclass,
						domain_column.attname,
						domain_column.target_domain_type,
						domain_column.attname,
						domain_column.storage_cast_type
					);
				END IF;
				EXECUTE sql;

				IF domain_column.default_expr IS NOT NULL AND NOT domain_column.is_domain_array THEN
					sql := pg_catalog.format(
						'ALTER TABLE %s ALTER COLUMN %I SET DEFAULT ((%s)::text::%s::%s)',
						domain_column.attrelid::regclass,
						domain_column.attname,
						domain_column.default_expr,
						domain_column.storage_cast_type,
						domain_column.target_domain_type
					);
					EXECUTE sql;
				END IF;
			END LOOP;

			-- PostgreSQL 17 can recompute stored generated columns without
			-- dropping them. Older supported releases do not expose a safe
			-- equivalent that preserves dependent objects and column metadata.
			FOR domain_generated_column IN
				SELECT
					a.attrelid,
					a.attname,
					pg_catalog.pg_get_expr(d.adbin, d.adrelid) AS generation_expr
				FROM pg_catalog.pg_attribute AS a
				JOIN pg_catalog.pg_class AS c
					ON c.oid = a.attrelid
				JOIN pg_catalog.pg_attrdef AS d
					ON d.adrelid = a.attrelid
					AND d.adnum = a.attnum
				WHERE a.atttypid = ANY(domain_att_type_oids)
				AND a.attnum > 0
				AND NOT a.attisdropped
				AND a.attgenerated = 's'
				AND c.relkind IN ('r', 'p')
				AND NOT EXISTS (
					SELECT 1
					FROM pg_catalog.pg_depend AS dep
					JOIN pg_catalog.pg_rewrite AS rw
						ON rw.oid = dep.objid
					JOIN pg_catalog.pg_class AS mc
						ON mc.oid = rw.ev_class
					WHERE dep.refobjid = a.attrelid
					AND dep.refobjsubid IN (0, a.attnum)
				)
					AND NOT EXISTS (
						SELECT 1
						FROM pg_catalog.pg_depend AS dep
						WHERE dep.classid = 'pg_catalog.pg_policy'::regclass
						AND dep.refobjid = a.attrelid
						AND dep.refobjsubid IN (0, a.attnum)
					)
					AND NOT EXISTS (
						SELECT 1
						FROM pg_catalog.pg_depend AS dep
						WHERE dep.classid = 'pg_catalog.pg_trigger'::regclass
						AND dep.refobjid = a.attrelid
						AND dep.refobjsubid IN (0, a.attnum)
					)
					AND NOT EXISTS (
						SELECT 1
						FROM pg_catalog.pg_depend AS dep
						WHERE dep.classid = 'pg_catalog.pg_proc'::regclass
						AND dep.refobjid = a.attrelid
						AND dep.refobjsubid IN (0, a.attnum)
					)
					AND NOT EXISTS (
						SELECT 1
						FROM pg_catalog.pg_depend AS dep
						WHERE dep.classid = 'pg_catalog.pg_publication_rel'::regclass
						AND dep.refobjid = a.attrelid
						AND dep.refobjsubid = a.attnum
					)
					AND NOT EXISTS (
						SELECT 1
						FROM (
						SELECT 1
						FROM pg_catalog.pg_depend AS dep
						JOIN pg_catalog.pg_attrdef AS gd
							ON gd.oid = dep.objid
						JOIN pg_catalog.pg_attribute AS ga
							ON ga.attrelid = gd.adrelid
							AND ga.attnum = gd.adnum
						WHERE dep.classid = 'pg_catalog.pg_attrdef'::regclass
						AND dep.refobjid = a.attrelid
						AND dep.refobjsubid IN (0, a.attnum)
						AND ga.attgenerated <> ''
						UNION ALL
						SELECT 1
						FROM pg_catalog.pg_depend AS dep
						JOIN pg_catalog.pg_attribute AS ga
							ON ga.attrelid = dep.objid
							AND ga.attnum = dep.objsubid
						WHERE dep.classid = 'pg_catalog.pg_class'::regclass
						AND dep.refobjid = a.attrelid
						AND dep.refobjsubid IN (0, a.attnum)
						AND ga.attgenerated <> ''
					) AS generated_dependency
				)
				AND NOT EXISTS (
					SELECT 1
					FROM (
						SELECT 1
						FROM pg_catalog.pg_attrdef AS gd
						JOIN pg_catalog.pg_depend AS dep
							ON dep.classid = 'pg_catalog.pg_attrdef'::regclass
							AND dep.objid = gd.oid
						JOIN pg_catalog.pg_attribute AS sa
							ON sa.attrelid = dep.refobjid
							AND sa.attnum = dep.refobjsubid
						WHERE gd.adrelid = a.attrelid
						AND gd.adnum = a.attnum
						AND sa.atttypid = ANY(nested_topology_type_oids)
						AND sa.attnum > 0
						AND NOT sa.attisdropped
						AND sa.attnum <> a.attnum
						UNION ALL
						SELECT 1
						FROM pg_catalog.pg_depend AS dep
						JOIN pg_catalog.pg_attribute AS sa
							ON sa.attrelid = dep.refobjid
							AND sa.attnum = dep.refobjsubid
						WHERE dep.classid = 'pg_catalog.pg_class'::regclass
						AND dep.objid = a.attrelid
						AND dep.objsubid = a.attnum
						AND sa.atttypid = ANY(nested_topology_type_oids)
						AND sa.attnum > 0
						AND NOT sa.attisdropped
						AND sa.attnum <> a.attnum
					) AS generated_source_dependency
				)
				AND NOT EXISTS (
					WITH RECURSIVE inherited_relid(attrelid) AS (
						SELECT i.inhrelid
						FROM pg_catalog.pg_inherits AS i
						WHERE i.inhparent = a.attrelid
						UNION ALL
						SELECT i.inhrelid
						FROM pg_catalog.pg_inherits AS i
						JOIN inherited_relid AS ir
							ON ir.attrelid = i.inhparent
					)
					SELECT 1
					FROM inherited_relid AS i
					JOIN pg_catalog.pg_attribute AS ca
						ON ca.attrelid = i.attrelid
						AND ca.attname = a.attname
						AND ca.atttypid = a.atttypid
						AND ca.attnum > 0
						AND NOT ca.attisdropped
					JOIN pg_catalog.pg_depend AS dep
						ON dep.refobjid = ca.attrelid
						AND dep.refobjsubid IN (0, ca.attnum)
					WHERE dep.classid = 'pg_catalog.pg_policy'::regclass
				)
					AND NOT EXISTS (
						WITH RECURSIVE inherited_relid(attrelid) AS (
							SELECT i.inhrelid
							FROM pg_catalog.pg_inherits AS i
							WHERE i.inhparent = a.attrelid
							UNION ALL
							SELECT i.inhrelid
							FROM pg_catalog.pg_inherits AS i
							JOIN inherited_relid AS ir
								ON ir.attrelid = i.inhparent
						)
						SELECT 1
						FROM inherited_relid AS i
						JOIN pg_catalog.pg_attribute AS ca
							ON ca.attrelid = i.attrelid
							AND ca.attname = a.attname
							AND ca.atttypid = a.atttypid
							AND ca.attnum > 0
							AND NOT ca.attisdropped
						JOIN pg_catalog.pg_depend AS dep
							ON dep.refobjid = ca.attrelid
							AND (
								(
									dep.classid IN (
										'pg_catalog.pg_publication_rel'::regclass
									)
									AND dep.refobjsubid = ca.attnum
								)
								OR (
									dep.classid = 'pg_catalog.pg_trigger'::regclass
									AND dep.refobjsubid IN (0, ca.attnum)
								)
								OR (
									dep.classid = 'pg_catalog.pg_proc'::regclass
									AND dep.refobjsubid IN (0, ca.attnum)
								)
							)
					)
					AND NOT EXISTS (
						WITH RECURSIVE inherited_relid(attrelid) AS (
							SELECT i.inhrelid
						FROM pg_catalog.pg_inherits AS i
						WHERE i.inhparent = a.attrelid
						UNION ALL
						SELECT i.inhrelid
						FROM pg_catalog.pg_inherits AS i
						JOIN inherited_relid AS ir
							ON ir.attrelid = i.inhparent
					)
					SELECT 1
					FROM inherited_relid AS i
					JOIN pg_catalog.pg_attribute AS ca
						ON ca.attrelid = i.attrelid
						AND ca.attname = a.attname
						AND ca.atttypid = a.atttypid
						AND ca.attnum > 0
						AND NOT ca.attisdropped
					JOIN pg_catalog.pg_depend AS dep
						ON dep.refobjid = ca.attrelid
						AND dep.refobjsubid IN (0, ca.attnum)
					JOIN pg_catalog.pg_rewrite AS rw
						ON rw.oid = dep.objid
				)
				AND NOT EXISTS (
					WITH RECURSIVE inherited_relid(attrelid) AS (
						SELECT i.inhrelid
						FROM pg_catalog.pg_inherits AS i
						WHERE i.inhparent = a.attrelid
						UNION ALL
						SELECT i.inhrelid
						FROM pg_catalog.pg_inherits AS i
						JOIN inherited_relid AS ir
							ON ir.attrelid = i.inhparent
					)
					SELECT 1
					FROM inherited_relid AS i
					JOIN pg_catalog.pg_attribute AS ca
						ON ca.attrelid = i.attrelid
						AND ca.attname = a.attname
						AND ca.atttypid = a.atttypid
						AND ca.attnum > 0
						AND NOT ca.attisdropped
					JOIN pg_catalog.pg_depend AS dep
						ON dep.refobjid = ca.attrelid
						AND dep.refobjsubid IN (0, ca.attnum)
					JOIN pg_catalog.pg_attrdef AS gd
						ON gd.oid = dep.objid
					JOIN pg_catalog.pg_attribute AS ga
						ON ga.attrelid = gd.adrelid
						AND ga.attnum = gd.adnum
					WHERE dep.classid = 'pg_catalog.pg_attrdef'::regclass
					AND ga.attgenerated <> ''
					UNION ALL
					SELECT 1
					FROM inherited_relid AS i
					JOIN pg_catalog.pg_attribute AS ca
						ON ca.attrelid = i.attrelid
						AND ca.attname = a.attname
						AND ca.atttypid = a.atttypid
						AND ca.attnum > 0
						AND NOT ca.attisdropped
					JOIN pg_catalog.pg_depend AS dep
						ON dep.refobjid = ca.attrelid
						AND dep.refobjsubid IN (0, ca.attnum)
					JOIN pg_catalog.pg_attribute AS ga
						ON ga.attrelid = dep.objid
						AND ga.attnum = dep.objsubid
					WHERE dep.classid = 'pg_catalog.pg_class'::regclass
					AND ga.attgenerated <> ''
				)
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
				IF pg_catalog.current_setting('server_version_num')::int >= 170000 THEN
					sql := pg_catalog.format(
						'ALTER TABLE %s ALTER COLUMN %I SET EXPRESSION AS (%s)',
						domain_generated_column.attrelid::regclass,
						domain_generated_column.attname,
						domain_generated_column.generation_expr
					);
					EXECUTE sql;
				ELSE
					skipped_repair := true;
					RAISE WARNING 'Could not rewrite stored generated column %.% for topology.% on PostgreSQL %, generated expression reset requires PostgreSQL 17 or later',
						domain_generated_column.attrelid::regclass,
						domain_generated_column.attname,
						domain_name,
						pg_catalog.current_setting('server_version');
				END IF;
				END LOOP;

				-- Restore array-of-domain columns before re-adding domain
				-- constraints. Recasting text[] after NOT VALID constraints are
				-- back would validate each element as a new domain value, which
				-- defeats the incomplete-repair path for grandfathered rows.
				CREATE TEMP TABLE IF NOT EXISTS pg_temp._postgis_topology_domain_array_columns (
					attrelid OID,
					attname NAME,
					target_domain_type TEXT,
					default_expr TEXT
				) ON COMMIT DROP;

				FOR domain_column IN
					SELECT *
					FROM pg_temp._postgis_topology_domain_array_columns
					WHERE target_domain_type = pg_catalog.format('%I.%I[]', domain_schema, domain_name)
				LOOP
					restored_domain_array_columns := true;
					sql := pg_catalog.format(
						'ALTER TABLE %s ALTER COLUMN %I TYPE %s USING (%I::%s)',
						domain_column.attrelid::regclass,
						domain_column.attname,
						domain_column.target_domain_type,
						domain_column.attname,
						domain_column.target_domain_type
					);
					EXECUTE sql;

					IF domain_column.default_expr IS NOT NULL THEN
						sql := pg_catalog.format(
							'ALTER TABLE %s ALTER COLUMN %I SET DEFAULT ((%s)::pg_catalog.text[]::%s)',
							domain_column.attrelid::regclass,
							domain_column.attname,
							domain_column.default_expr,
							domain_column.target_domain_type
						);
						EXECUTE sql;
					END IF;
				END LOOP;

				DELETE FROM pg_temp._postgis_topology_domain_array_columns
				WHERE target_domain_type = pg_catalog.format('%I.%I[]', domain_schema, domain_name);

				FOR domain_constraint IN
					SELECT
						value->>'name' AS conname,
						CASE
						WHEN skipped_repair THEN value->>'definition'
						WHEN COALESCE((value->>'not_valid_by_repair')::boolean, false) THEN pg_catalog.regexp_replace(
							value->>'definition',
							'[[:space:]]+NOT[[:space:]]+VALID[[:space:]]*$',
							'',
							'i'
						)
						ELSE pg_catalog.regexp_replace(
							value->>'definition',
							'[[:space:]]*$',
							'',
							'i'
						)
						END AS constraint_def,
						COALESCE((value->>'not_valid_by_repair')::boolean, false) AS not_valid_by_repair
					FROM pg_catalog.jsonb_array_elements(domain_constraint_defs) AS value
				LOOP
					-- If some old-width rows were intentionally skipped, restoring
					-- constraints as validated would scan those rows and can abort
					-- the whole repair. PostgreSQL also rejects validated domain
					-- constraints while array/container columns still depend on the
					-- domain, even after those columns have been rewritten. Mark only
					-- constraints demoted by skipped storage as retry candidates; the
					-- array-dependency case is structural, not evidence of old-width
					-- storage left behind.
					sql := pg_catalog.format(
						'ALTER DOMAIN %I.%I ADD CONSTRAINT %I %s%s',
						domain_schema,
						domain_name,
						domain_constraint.conname,
						domain_constraint.constraint_def,
						CASE
						WHEN (skipped_repair OR restored_domain_array_columns)
							AND domain_constraint.constraint_def !~* '[[:space:]]NOT[[:space:]]+VALID[[:space:]]*$'
						THEN ' NOT VALID'
						ELSE ''
						END
					);
					EXECUTE sql;
					IF skipped_repair
						AND (
							domain_constraint.not_valid_by_repair
							OR domain_constraint.constraint_def !~* '[[:space:]]NOT[[:space:]]+VALID[[:space:]]*$'
						)
					THEN
						sql := pg_catalog.format(
							'COMMENT ON CONSTRAINT %I ON DOMAIN %I.%I IS %L',
							domain_constraint.conname,
							domain_schema,
							domain_name,
							constraint_not_valid_marker
						);
						EXECUTE sql;
					END IF;
				END LOOP;

			IF skipped_repair THEN
				RAISE WARNING 'Topology.% domain storage repair was incomplete; repair marker was not written',
					domain_name;
			ELSE
				sql := pg_catalog.format(
					'COMMENT ON DOMAIN %I.%I IS %L',
					domain_schema,
					domain_name,
					pg_catalog.concat_ws(
						E'\n',
						NULLIF(pg_catalog.obj_description(domain_oid, 'pg_type'), ''),
						repair_marker
					)
				);
				EXECUTE sql;
			END IF;

			RAISE INFO 'Upgraded % from % to %', domain_name, old_domain_type, new_domain_type;
		EXCEPTION
		WHEN others THEN
			GET STACKED DIAGNOSTICS
				context := PG_EXCEPTION_CONTEXT,
				detail := PG_EXCEPTION_DETAIL;
			RAISE WARNING 'Could not modify % from % to %, got % (%)',
				domain_name, old_domain_type, new_domain_type, SQLERRM, SQLSTATE USING DETAIL = detail, HINT = context;
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
