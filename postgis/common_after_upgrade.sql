-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--
--
-- PostGIS - Spatial Types for PostgreSQL
-- http://postgis.net
--
-- Copyright (C) 2011-2020 Sandro Santilli <strk@kbt.io>
-- Copyright (C) 2010-2012 Regina Obe <lr@pcorp.us>
-- Copyright (C) 2009      Paul Ramsey <pramsey@cleverelephant.ca>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
--
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--
-- This file will be appended at the very end of every
-- sql upgrade script.
--
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

-- DROP auxiliar function (created by common_before_upgrade.sql)
DROP FUNCTION _postgis_drop_function_by_identity(text, text, text);
DROP FUNCTION _postgis_drop_function_by_signature(text, text);


-- Drop deprecated functions if possible
DO LANGUAGE 'plpgsql'
$POSTGIS_PROC_UPGRADE$
DECLARE
    new_name TEXT;
    rec RECORD;
    extrec RECORD;
    sql TEXT;
    detail TEXT;
    hint TEXT;
BEGIN

    -- Try to drop all deprecated functions,
    -- and report failure to do so as a WARNING
    -- for the user to handle.
    --
    FOR rec IN

        SELECT *, oid::regprocedure as proc
        FROM pg_catalog.pg_proc
        WHERE proname ~ 'deprecated_by_postgis'

    LOOP --{

        RAISE DEBUG 'Handling deprecated function %', rec.proc;

        new_name := pg_catalog.regexp_replace(
            rec.proc::text,
            E'_deprecated_by_postgis[^(]*\\(.*',
            ''
        );

        sql := pg_catalog.format('DROP FUNCTION %s', rec.proc);
        --RAISE DEBUG 'SQL: %', sql;
        BEGIN --{
            EXECUTE sql;
        EXCEPTION
        WHEN OTHERS THEN -- }{
            hint = 'Resolve the issue';
            GET STACKED DIAGNOSTICS detail := PG_EXCEPTION_DETAIL;
            IF detail LIKE '%view % depends%' THEN
                hint = pg_catalog.format(
                    'Replace the view changing all occurrences of %s in its definition with %s',
                    rec.proc,
                    new_name
                );
            END IF;
            hint = hint || ' and upgrade again';

            RAISE WARNING 'Deprecated function % left behind: %',
                rec.proc, SQLERRM
            USING DETAIL = detail, HINT = hint;

            -- Drop the function from any extension it is part of
            -- so dump/reloads still work
            FOR extrec IN
                SELECT e.extname
                FROM
                    pg_catalog.pg_extension e,
                    pg_catalog.pg_depend d
                WHERE
                    d.refclassid = 'pg_catalog.pg_extension'::pg_catalog.regclass AND
                    d.refobjid = e.oid AND
                    d.classid = 'pg_catalog.pg_proc'::pg_catalog.regclass AND
                    d.objid = rec.proc::oid
            LOOP
                RAISE DEBUG 'Unpackaging % from extension %', rec.proc, extrec.extname;
                sql := pg_catalog.format('ALTER EXTENSION %I DROP FUNCTION %s', extrec.extname, rec.proc);
                EXECUTE sql;
            END LOOP;

        END; --}

    END LOOP; --}
END
$POSTGIS_PROC_UPGRADE$;
