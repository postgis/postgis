BEGIN;

DO $$
BEGIN
	IF NOT EXISTS (
		SELECT 1
		FROM pg_catalog.pg_namespace
		WHERE nspname = 'topology'
	) THEN
		CREATE SCHEMA topology;
	END IF;
END;
$$;

CREATE OR REPLACE FUNCTION topology.postgis_topology_scripts_installed()
RETURNS text
AS $$ SELECT '1.0.0'::text AS version $$
LANGUAGE 'sql' IMMUTABLE;

SELECT 'topology_old_scripts',
	postgis_full_version() ~
		'TOPOLOGY \(topology procs from "1\.0\.0" need upgrade\)';

CREATE OR REPLACE FUNCTION topology.postgis_topology_scripts_installed()
RETURNS text
AS $$ SELECT '999.0.0dev'::text AS version $$
LANGUAGE 'sql' IMMUTABLE;

SELECT 'topology_newer_scripts',
	postgis_full_version() ~
		'TOPOLOGY \(topology procs from "999\.0\.0dev" are newer than core procs from "[^"]+"\)';

ROLLBACK;
