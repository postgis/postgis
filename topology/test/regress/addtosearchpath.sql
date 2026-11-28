SET client_min_messages = error;

-- The pre-install overload records a call instead of changing privileges.
SELECT 'addtosearchpath-install-safe',
       NOT EXISTS (
           SELECT 1 FROM public.addtosearchpath_overload_marker
       )
       AND EXISTS (
           SELECT 1
           FROM pg_catalog.pg_db_role_setting AS settings,
                pg_catalog.unnest(settings.setconfig) AS config(setting)
           WHERE settings.setdatabase = (
                     SELECT oid
                     FROM pg_catalog.pg_database
                     WHERE datname = pg_catalog.current_database()
                 )
             AND settings.setrole = 0
             AND pg_catalog.regexp_replace(config.setting, '^search_path=', '')
                 ~ '(^|,[[:space:]]*)topology([[:space:]]*,|$)'
       );

DROP FUNCTION topology.AddToSearchPath(text);
DROP TABLE public.addtosearchpath_overload_marker;

-- DEFAULT is also a valid schema identifier in search_path. Re-parsing it as
-- an ALTER DATABASE value treats it as the DEFAULT keyword instead.
SELECT pg_catalog.set_config('search_path', 'default, public', false);
SELECT pg_catalog.format(
    'ALTER DATABASE %I SET search_path FROM CURRENT',
    pg_catalog.current_database()
) \gexec
SELECT pg_catalog.rtrim(topology.AddToSearchPath('topology'::varchar));
SELECT 'addtosearchpath-default-schema',
       pg_catalog.current_setting('search_path') = 'default, public, topology';
SELECT pg_catalog.format(
    'ALTER DATABASE %I SET search_path = "$user", public, topology',
    pg_catalog.current_database()
) \gexec
SELECT pg_catalog.set_config('search_path', '"$user", public, topology', false) IS NOT NULL;
