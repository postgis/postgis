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
		SELECT unnest(setconfig) config
		FROM pg_db_role_setting
		WHERE setdatabase = (
			SELECT oid
			FROM pg_database
			WHERE datname = current_database()
		) and setrole = 0
	)
	SELECT regexp_replace(config, '^search_path=', '')
	FROM settings WHERE config like 'search_path=%'
	INTO var_cur_search_path;

	RAISE NOTICE 'cur_search_path from pg_db_role_setting is %', var_cur_search_path;

	-- only run this test if person creating the extension is a super user
	IF var_cur_search_path IS NULL AND (SELECT rolsuper FROM pg_roles where rolname = CURRENT_USER) THEN
		SELECT setting
		INTO var_cur_search_path
		FROM pg_file_settings
		WHERE name = 'search_path' AND applied;

		RAISE NOTICE 'cur_search_path from pg_file_settings is %', var_cur_search_path;
	END IF;

	IF var_cur_search_path IS NULL THEN
		SELECT boot_val
		INTO var_cur_search_path
		FROM pg_settings
		WHERE name = 'search_path';

		RAISE NOTICE 'cur_search_path from pg_settings is %', var_cur_search_path;
	END IF;

	IF var_cur_search_path LIKE '%' || quote_ident(a_schema_name) || '%' THEN
		var_result := a_schema_name || ' already in database search_path';
	ELSE
		var_cur_search_path := var_cur_search_path || ', '
                       || quote_ident(a_schema_name);
		EXECUTE 'ALTER DATABASE ' || quote_ident(current_database())
                             || ' SET search_path = ' || var_cur_search_path;
		var_result := a_schema_name || ' has been added to end of database search_path ';
	END IF;

	EXECUTE 'SET search_path = ' || var_cur_search_path;

  RETURN var_result;
END
$BODY$
SET search_path = pg_catalog -- make safe
LANGUAGE 'plpgsql' VOLATILE STRICT
;

