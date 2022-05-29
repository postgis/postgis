-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--
----
-- PostGIS - Spatial Types for PostgreSQL
-- http://postgis.net
--
-- Copyright (C) 2011 Regina Obe <lr@pcorp.us>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
--
-- Author: Regina Obe <lr@pcorp.us>
--
-- This is a suite of SQL helper functions for use during a PostGIS extension install/upgrade
-- The functions get uninstalled after the extention install/upgrade process
---------------------------
-- postgis_extension_remove_objects: This function removes objects of a particular class from an extension
-- this is needed because there is no ALTER EXTENSION DROP FUNCTION/AGGREGATE command
-- and we can't CREATE OR REPALCe functions whose signatures have changed and we can drop them if they are part of an extention
-- So we use this to remove it from extension first before we drop
CREATE FUNCTION postgis_extension_remove_objects(param_extension text, param_type text)
  RETURNS boolean AS
$$
DECLARE
	var_sql text := '';
	var_r record;
	var_result boolean := false;
	var_class text := '';
	var_is_aggregate boolean := false;
	var_sql_list text := '';
	var_pgsql_version integer := pg_catalog.current_setting('server_version_num');
BEGIN
		var_class := CASE WHEN pg_catalog.lower(param_type) OPERATOR(pg_catalog.=)'function' OR pg_catalog.lower(param_type) OPERATOR(pg_catalog.=) 'aggregate' THEN 'pg_catalog.pg_proc' ELSE '' END;
		var_is_aggregate := CASE WHEN pg_catalog.lower(param_type) OPERATOR(pg_catalog.=) 'aggregate' THEN true ELSE false END;

		IF var_pgsql_version OPERATOR(pg_catalog.<) 110000 THEN
			var_sql_list := $sql$SELECT 'ALTER EXTENSION ' OPERATOR(pg_catalog.||)  e.extname OPERATOR(pg_catalog.||) ' DROP ' OPERATOR(pg_catalog.||) $3 OPERATOR(pg_catalog.||) ' ' OPERATOR(pg_catalog.||) COALESCE(proc.proname OPERATOR(pg_catalog.||) '(' OPERATOR(pg_catalog.||) oidvectortypes(proc.proargtypes) OPERATOR(pg_catalog.||) ')' ,typ.typname, cd.relname, op.oprname,
					cs.typname OPERATOR(pg_catalog.||) ' AS ' OPERATOR(pg_catalog.||) ct.typname OPERATOR(pg_catalog.||) ') ', opcname, opfname) OPERATOR(pg_catalog.||) ';' AS remove_command
			FROM pg_catalog.pg_depend As d INNER JOIN pg_catalog.pg_extension As e
				ON d.refobjid OPERATOR(pg_catalog.=) e.oid INNER JOIN pg_catalog.pg_class As c ON
					c.oid OPERATOR(pg_catalog.=) d.classid
					LEFT JOIN pg_catalog.pg_proc AS proc ON proc.oid OPERATOR(pg_catalog.=) d.objid
					LEFT JOIN pg_catalog.pg_type AS typ ON typ.oid OPERATOR(pg_catalog.=) d.objid
					LEFT JOIN pg_catalog.pg_class As cd ON cd.oid OPERATOR(pg_catalog.=) d.objid
					LEFT JOIN pg_operator As op ON op.oid OPERATOR(pg_catalog.=) d.objid
					LEFT JOIN pg_catalog.pg_cast AS ca ON ca.oid OPERATOR(pg_catalog.=) d.objid
					LEFT JOIN pg_catalog.pg_type AS cs ON ca.castsource OPERATOR(pg_catalog.=) cs.oid
					LEFT JOIN pg_catalog.pg_type AS ct ON ca.casttarget OPERATOR(pg_catalog.=) ct.oid
					LEFT JOIN pg_opclass As oc ON oc.oid OPERATOR(pg_catalog.=) d.objid
					LEFT JOIN pg_opfamily As ofa ON ofa.oid OPERATOR(pg_catalog.=) d.objid
			WHERE d.deptype OPERATOR(pg_catalog.=) 'e' and e.extname OPERATOR(pg_catalog.=) $1 and c.relname OPERATOR(pg_catalog.=) $2 AND COALESCE(proc.proisagg, false) OPERATOR(pg_catalog.=) $4;$sql$;
		ELSE -- for PostgreSQL 11 and above, they removed proc.proisagg among others and replaced with some func type thing
			var_sql_list := $sql$SELECT 'ALTER EXTENSION ' OPERATOR(pg_catalog.||) e.extname OPERATOR(pg_catalog.||) ' DROP ' OPERATOR(pg_catalog.||) $3 OPERATOR(pg_catalog.||) ' ' OPERATOR(pg_catalog.||) COALESCE(proc.proname OPERATOR(pg_catalog.||) '(' OPERATOR(pg_catalog.||) oidvectortypes(proc.proargtypes) OPERATOR(pg_catalog.||) ')' ,typ.typname, cd.relname, op.oprname,
					cs.typname OPERATOR(pg_catalog.||) ' AS ' OPERATOR(pg_catalog.||) ct.typname OPERATOR(pg_catalog.||) ') ', opcname, opfname) OPERATOR(pg_catalog.||) ';' AS remove_command
			FROM pg_catalog.pg_depend As d INNER JOIN pg_catalog.pg_extension As e
				ON d.refobjid OPERATOR(pg_catalog.=) e.oid INNER JOIN pg_catalog.pg_class As c ON
					c.oid OPERATOR(pg_catalog.=) d.classid
					LEFT JOIN pg_catalog.pg_proc AS proc ON proc.oid OPERATOR(pg_catalog.=) d.objid
					LEFT JOIN pg_catalog.pg_type AS typ ON typ.oid OPERATOR(pg_catalog.=) d.objid
					LEFT JOIN pg_catalog.pg_class As cd ON cd.oid OPERATOR(pg_catalog.=) d.objid
					LEFT JOIN pg_operator As op ON op.oid OPERATOR(pg_catalog.=) d.objid
					LEFT JOIN pg_catalog.pg_cast AS ca ON ca.oid OPERATOR(pg_catalog.=) d.objid
					LEFT JOIN pg_catalog.pg_type AS cs ON ca.castsource OPERATOR(pg_catalog.=) cs.oid
					LEFT JOIN pg_catalog.pg_type AS ct ON ca.casttarget OPERATOR(pg_catalog.=) ct.oid
					LEFT JOIN pg_opclass As oc ON oc.oid OPERATOR(pg_catalog.=) d.objid
					LEFT JOIN pg_opfamily As ofa ON ofa.oid OPERATOR(pg_catalog.=) d.objid
			WHERE d.deptype OPERATOR(pg_catalog.=) 'e' and e.extname OPERATOR(pg_catalog.=) $1 and c.relname OPERATOR(pg_catalog.=) $2 AND (proc.prokind OPERATOR(pg_catalog.=) 'a')  OPERATOR(pg_catalog.=) $4;$sql$;
		END IF;

		FOR var_r IN EXECUTE var_sql_list  USING param_extension, var_class, param_type, var_is_aggregate
		LOOP
			var_sql := var_sql OPERATOR(pg_catalog.||) var_r.remove_command OPERATOR(pg_catalog.||) ';';
		END LOOP;
		IF var_sql > '' THEN
			EXECUTE var_sql;
			var_result := true;
		END IF;

		RETURN var_result;
END;
$$
LANGUAGE plpgsql VOLATILE;

CREATE FUNCTION postgis_extension_drop_if_exists(param_extension text, param_statement text)
  RETURNS boolean AS
$$
DECLARE
	var_sql_ext text := 'ALTER EXTENSION ' OPERATOR(pg_catalog.||) pg_catalog.quote_ident(param_extension) OPERATOR(pg_catalog.||) ' ' OPERATOR(pg_catalog.||) pg_catalog.replace(param_statement, 'IF EXISTS', '');
	var_result boolean := false;
BEGIN
	BEGIN
		EXECUTE var_sql_ext;
		var_result := true;
	EXCEPTION
		WHEN OTHERS THEN
			--this is to allow ignoring if the object does not exist in extension
			var_result := false;
	END;
	RETURN var_result;
END;
$$
LANGUAGE plpgsql VOLATILE;

CREATE FUNCTION postgis_extension_AddToSearchPath(a_schema_name text)
RETURNS text
AS
$$
DECLARE
	var_result text;
	var_cur_search_path text;
BEGIN

	WITH settings AS (
		SELECT pg_catalog.unnest(setconfig) config
		FROM pg_catalog.pg_db_role_setting
		WHERE setdatabase OPERATOR(pg_catalog.=) (
			SELECT oid
			FROM pg_catalog.pg_database
			WHERE datname OPERATOR(pg_catalog.=) pg_catalog.current_database()
		) and setrole OPERATOR(pg_catalog.=) 0
	)
	SELECT pg_catalog.regexp_replace(config, '^search_path=', '')
	FROM settings WHERE config like 'search_path=%'
	INTO var_cur_search_path;

	RAISE NOTICE 'cur_search_path from pg_db_role_setting is %', var_cur_search_path;

	IF var_cur_search_path IS NULL THEN
		SELECT reset_val
		INTO var_cur_search_path
		FROM pg_catalog.pg_settings
		WHERE name OPERATOR(pg_catalog.=) 'search_path';

		RAISE NOTICE 'cur_search_path from pg_settings is %', var_cur_search_path;
	END IF;


	IF var_cur_search_path LIKE '%' OPERATOR(pg_catalog.||) pg_catalog.quote_ident(a_schema_name) OPERATOR(pg_catalog.||) '%' THEN
		var_result := a_schema_name OPERATOR(pg_catalog.||) ' already in database search_path';
	ELSE
		var_cur_search_path := var_cur_search_path OPERATOR(pg_catalog.||) ', '
                       OPERATOR(pg_catalog.||) pg_catalog.quote_ident(a_schema_name);
		EXECUTE 'ALTER DATABASE ' OPERATOR(pg_catalog.||) pg_catalog.quote_ident(pg_catalog.current_database())
                             OPERATOR(pg_catalog.||) ' SET search_path = ' OPERATOR(pg_catalog.||) var_cur_search_path;
		var_result := a_schema_name OPERATOR(pg_catalog.||) ' has been added to end of database search_path ';
	END IF;

	EXECUTE 'SET search_path = ' OPERATOR(pg_catalog.||) var_cur_search_path;

  RETURN var_result;
END
$$
LANGUAGE 'plpgsql' VOLATILE STRICT;
