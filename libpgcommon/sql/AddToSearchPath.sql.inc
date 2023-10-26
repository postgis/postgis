/*

  Common body for a function to add a schema to the search path
  Use as follows:

    CREATE OR REPLACE FUNCTION someName(someARg text)
    #include "libpgcommon/sql/AddToSearchPath.sql.inc"
*/
RETURNS text
AS
$BODY$
DECLARE
	var_result text;
	var_cur_search_path text;
	a_schema_name text := $1;
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
		SELECT setting
		INTO var_cur_search_path
		FROM pg_catalog.pg_file_settings
		WHERE name OPERATOR(pg_catalog.=) 'search_path' AND applied;

		RAISE NOTICE 'cur_search_path from pg_file_settings is %', var_cur_search_path;
	END IF;

	IF var_cur_search_path IS NULL THEN
		SELECT boot_val
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
$BODY$
LANGUAGE 'plpgsql' VOLATILE STRICT;

