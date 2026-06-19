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
