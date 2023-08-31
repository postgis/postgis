-- Retire old signatures during upgrade while retaining the function OID
-- when a view or other dependent object still uses it.
CREATE FUNCTION _postgis_retire_upgrade_function(old_function pg_catalog.regprocedure)
RETURNS void AS $$
DECLARE
    old_name text;
    extension_name text;
BEGIN
    IF old_function IS NULL THEN
        RETURN;
    END IF;

    SELECT e.extname INTO extension_name
    FROM pg_catalog.pg_depend d
    JOIN pg_catalog.pg_extension e ON e.oid = d.refobjid
    WHERE d.classid = 'pg_catalog.pg_proc'::pg_catalog.regclass
      AND d.objid = old_function
      AND d.refclassid = 'pg_catalog.pg_extension'::pg_catalog.regclass
      AND d.deptype = 'e';

    IF extension_name IS NOT NULL THEN
        EXECUTE pg_catalog.format('ALTER EXTENSION %I DROP FUNCTION %s',
            extension_name, old_function);
    END IF;

    BEGIN
        EXECUTE pg_catalog.format('DROP FUNCTION %s', old_function);
    EXCEPTION WHEN dependent_objects_still_exist THEN
        SELECT proname INTO STRICT old_name FROM pg_catalog.pg_proc
        WHERE oid = old_function;
        EXECUTE pg_catalog.format('ALTER FUNCTION %s RENAME TO %I',
            old_function, old_name || '_deprecated_by_postgis');
        RAISE WARNING 'Preserving retired function % for dependent objects', old_function;
    END;
END
$$ LANGUAGE plpgsql;
