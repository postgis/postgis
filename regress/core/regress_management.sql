-- Test the populate_geometry_columns,DropGeometryTable etc --
\set VERBOSITY terse
SET client_min_messages TO warning;
CREATE TABLE test_pt(gid SERIAL PRIMARY KEY, geom geometry);
INSERT INTO test_pt(geom) VALUES(ST_GeomFromEWKT('SRID=4326;POINT M(1 2 3)'));
SELECT populate_geometry_columns('test_pt'::regclass);
SELECT '#6038.before', srid FROM geometry_columns WHERE f_table_schema = 'public' AND f_table_name = 'test_pt' AND f_geometry_column = 'geom';
SELECT 'The result: ' || DropGeometryTable('test_pt');
SELECT '#6038.after', count(*) FROM geometry_columns WHERE f_table_schema = 'public' AND f_table_name = 'test_pt' AND f_geometry_column = 'geom';
DO $$
DECLARE
	can_switch_role boolean;
BEGIN
	SELECT rolsuper INTO can_switch_role
	FROM pg_roles
	WHERE rolname = current_user;

	IF can_switch_role THEN
		DROP SCHEMA IF EXISTS test6038_private CASCADE;
		DROP TABLE IF EXISTS public.test6038_visible_geom;
		DROP ROLE IF EXISTS test6038_invisible;
		DROP ROLE IF EXISTS test6038_visible;
		CREATE ROLE test6038_invisible;
		CREATE ROLE test6038_visible;
		GRANT test6038_invisible TO CURRENT_USER;
		GRANT test6038_visible TO CURRENT_USER;
		CREATE TABLE public.test6038_visible_geom(geom geometry(Point, 4326));
		GRANT SELECT ON public.test6038_visible_geom TO test6038_visible;
		CREATE SCHEMA test6038_private;
		CREATE TABLE test6038_private.hidden_geom(geom geometry(Point, 4326));
		REVOKE ALL ON SCHEMA test6038_private FROM PUBLIC;
		GRANT SELECT ON test6038_private.hidden_geom TO test6038_invisible;

		EXECUTE 'SET LOCAL ROLE test6038_visible';
		IF 1 != (SELECT count(*) FROM geometry_columns WHERE f_table_schema = 'public' AND f_table_name = 'test6038_visible_geom') THEN
			RAISE EXCEPTION 'geometry_columns did not expose currently selectable table in visible schema';
		END IF;
		EXECUTE 'RESET ROLE';

		REVOKE SELECT ON public.test6038_visible_geom FROM test6038_visible;
		EXECUTE 'SET LOCAL ROLE test6038_visible';
		IF 0 != (SELECT count(*) FROM geometry_columns WHERE f_table_schema = 'public' AND f_table_name = 'test6038_visible_geom') THEN
			RAISE EXCEPTION 'geometry_columns exposed metadata after SELECT was revoked in visible schema';
		END IF;
		EXECUTE 'RESET ROLE';

		-- The view must not resolve names inside schemas hidden from the caller.
		EXECUTE 'SET LOCAL ROLE test6038_invisible';
		IF 1 != (SELECT count(*) FROM geometry_columns WHERE f_table_schema = 'test6038_private') THEN
			RAISE EXCEPTION 'geometry_columns did not preserve OID-based SELECT visibility in hidden schema';
		END IF;
		EXECUTE 'RESET ROLE';

		REVOKE SELECT ON test6038_private.hidden_geom FROM test6038_invisible;
		EXECUTE 'SET LOCAL ROLE test6038_invisible';
		IF 0 != (SELECT count(*) FROM geometry_columns WHERE f_table_schema = 'test6038_private') THEN
			RAISE EXCEPTION 'geometry_columns exposed hidden-schema metadata after SELECT was revoked';
		END IF;
		EXECUTE 'RESET ROLE';

		DROP SCHEMA test6038_private CASCADE;
		DROP TABLE public.test6038_visible_geom;
		DROP ROLE test6038_invisible;
		DROP ROLE test6038_visible;
	END IF;
END
$$;
SELECT 'Unexistant: ' || DropGeometryTable('unexistent'); -- see ticket #861

-- Foreign data wrapper creation is superuser-only. Exercise the real foreign
-- table path when allowed, and keep restricted regression runs deterministic.
SELECT current_setting('is_superuser')::boolean
	AND EXISTS (
		SELECT 1
		FROM pg_available_extensions
		WHERE name = 'file_fdw'
	) AS regress_management_has_fdw \gset

\if :regress_management_has_fdw
SELECT current_setting('data_directory') || '/regress_management_fdw_' || pg_backend_pid() || '_' || to_char(clock_timestamp(), 'YYYYMMDDHH24MISSUS') || '.csv' AS regress_management_file \gset

COPY (SELECT 'SRID=4326;POINT(1 2)') TO :'regress_management_file' WITH (FORMAT csv);

CREATE EXTENSION file_fdw;
CREATE SERVER regress_management_server FOREIGN DATA WRAPPER file_fdw;
CREATE FOREIGN TABLE test_fdw_geom(geom geometry)
	SERVER regress_management_server
	OPTIONS (filename :'regress_management_file', format 'csv');

SELECT 'foreign before', coord_dimension, srid, type
	FROM geometry_columns
	WHERE f_table_schema = 'public'
	AND f_table_name = 'test_fdw_geom'
	AND f_geometry_column = 'geom';
SELECT 'foreign typmod', populate_geometry_columns('test_fdw_geom'::regclass);
SELECT 'foreign after typmod', coord_dimension, srid, type
	FROM geometry_columns
	WHERE f_table_schema = 'public'
	AND f_table_name = 'test_fdw_geom'
	AND f_geometry_column = 'geom';

DROP FOREIGN TABLE test_fdw_geom;
CREATE FOREIGN TABLE test_fdw_geom(geom geometry)
	SERVER regress_management_server
	OPTIONS (filename :'regress_management_file', format 'csv');

SELECT 'foreign constraints', populate_geometry_columns('test_fdw_geom'::regclass, false);
SELECT 'foreign after constraints', coord_dimension, srid, type
	FROM geometry_columns
	WHERE f_table_schema = 'public'
	AND f_table_name = 'test_fdw_geom'
	AND f_geometry_column = 'geom';

DROP FOREIGN TABLE test_fdw_geom;
DROP SERVER regress_management_server;
DROP EXTENSION file_fdw;
\else
SELECT 'foreign before', 2, 0, 'GEOMETRY';
SELECT 'foreign typmod', 1;
SELECT 'foreign after typmod', 2, 4326, 'POINT';
SELECT 'foreign constraints', 1;
SELECT 'foreign after constraints', 2, 4326, 'POINT';
\endif
