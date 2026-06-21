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

CREATE OR REPLACE FUNCTION postgis_scripts_released()
RETURNS text
AS $$ SELECT '3.7.0rc1'::text AS version $$
LANGUAGE 'sql' IMMUTABLE;

CREATE OR REPLACE FUNCTION topology.postgis_topology_scripts_installed()
RETURNS text
AS $$ SELECT '1.0.0'::text AS version $$
LANGUAGE 'sql' IMMUTABLE;

-- Older component script versions still mean the installed procs need upgrade.
SELECT 'topology_old_scripts',
	postgis_full_version() ~
		E'TOPOLOGY \\(topology procs from "1\\.0\\.0" need upgrade\\)';

CREATE OR REPLACE FUNCTION topology.postgis_topology_scripts_installed()
RETURNS text
AS $$ SELECT '3.7.0'::text AS version $$
LANGUAGE 'sql' IMMUTABLE;

CREATE OR REPLACE FUNCTION postgis_raster_scripts_installed()
RETURNS text
AS $$ SELECT '3.7.0rc2'::text AS version $$
LANGUAGE 'sql' IMMUTABLE;

CREATE OR REPLACE FUNCTION postgis_sfcgal_full_version()
RETURNS text
AS $$ SELECT '999.0.0dev'::text AS version $$
LANGUAGE 'sql' IMMUTABLE;

CREATE OR REPLACE FUNCTION postgis_sfcgal_scripts_installed()
RETURNS text
AS $$ SELECT '3.7.0beta1'::text AS version $$
LANGUAGE 'sql' IMMUTABLE;

-- Newer component script versions are intentional in mixed-script installs:
-- report them as newer than core, not as something that needs upgrade. This
-- includes final releases newer than release candidates and later RCs.
SELECT 'topology_newer_scripts',
	postgis_full_version() ~
		E'TOPOLOGY \\(topology procs from "3\\.7\\.0" are newer than core procs from "3\\.7\\.0rc1"\\)';

SELECT 'raster_newer_scripts',
	postgis_full_version() ~
		E'\\(raster procs from "3\\.7\\.0rc2" are newer than core procs from "3\\.7\\.0rc1"\\)';

-- A prerelease older than the core prerelease is still reported as needing
-- upgrade, even when its numeric major/minor/micro components match.
SELECT 'sfcgal_older_prerelease',
	postgis_full_version() ~
		E'\\(sfcgal procs from "3\\.7\\.0beta1" need upgrade\\)';

CREATE OR REPLACE FUNCTION postgis_sfcgal_scripts_installed()
RETURNS text
AS $$ SELECT '3.7.0'::text AS version $$
LANGUAGE 'sql' IMMUTABLE;

SELECT 'sfcgal_newer_scripts',
	postgis_full_version() ~
		E'\\(sfcgal procs from "3\\.7\\.0" are newer than core procs from "3\\.7\\.0rc1"\\)';

ROLLBACK;
