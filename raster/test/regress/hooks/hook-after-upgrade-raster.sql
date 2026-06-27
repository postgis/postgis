DROP VIEW upgrade_test_raster_view_st_value;
DROP VIEW upgrade_test_raster_view_st_clip;
DROP VIEW upgrade_test_raster_view_st_intersects;

DROP TABLE upgrade_test_raster;
DROP TABLE upgrade_test_raster_with_regular_blocking;

DROP VIEW upgrade_test_raster_view_st_slope;
DROP VIEW upgrade_test_raster_view_st_aspect;

-- See https://trac.osgeo.org/postgis/ticket/2823
DO $$
DECLARE
	column_acl_preserved boolean;
	grantor_acl_preserved boolean;
	grantee_dependency_preserved boolean;
	public_has_select boolean;
BEGIN
	IF NOT has_table_privilege('upgrade_test_raster_acl_user', current_schema() || '.raster_columns', 'SELECT') THEN
		RAISE EXCEPTION 'raster_columns SELECT grant was not preserved';
	END IF;

	SELECT EXISTS (
		SELECT 1
		FROM pg_class c
		CROSS JOIN aclexplode(c.relacl) AS acl
		WHERE c.oid = (current_schema() || '.raster_columns')::regclass
			AND acl.grantee = 0
			AND acl.privilege_type = 'SELECT'
	) INTO public_has_select;

	IF public_has_select THEN
		RAISE EXCEPTION 'raster_columns public SELECT grant was unexpectedly restored';
	END IF;

	SELECT EXISTS (
		SELECT 1
		FROM pg_catalog.pg_class c
		CROSS JOIN aclexplode(c.relacl) AS acl
		WHERE c.oid = (current_schema() || '.raster_columns')::regclass
			AND acl.grantor = 'upgrade_test_raster_acl_grantor'::regrole
			AND acl.grantee = 'upgrade_test_raster_acl_user'::regrole
			AND acl.privilege_type = 'SELECT'
	) INTO grantor_acl_preserved;

	IF NOT grantor_acl_preserved THEN
		RAISE EXCEPTION 'raster_columns SELECT grantor was not preserved';
	END IF;

	SELECT EXISTS (
		SELECT 1
		FROM pg_catalog.pg_attribute a
		CROSS JOIN aclexplode(a.attacl) AS acl
		WHERE a.attrelid = (current_schema() || '.raster_columns')::regclass
			AND a.attname = 'r_table_name'
			AND acl.grantor = 'upgrade_test_raster_acl_grantor'::regrole
			AND acl.grantee = 'upgrade_test_raster_acl_user'::regrole
			AND acl.privilege_type = 'SELECT'
	) INTO column_acl_preserved;

	IF NOT column_acl_preserved THEN
		RAISE EXCEPTION 'raster_columns column SELECT grant was not preserved';
	END IF;

	SELECT EXISTS (
		SELECT 1
		FROM pg_catalog.pg_shdepend d
		WHERE d.dbid = (SELECT oid FROM pg_catalog.pg_database WHERE datname = current_database())
			AND d.classid = 'pg_catalog.pg_class'::regclass
			AND d.objid = (current_schema() || '.raster_columns')::regclass
			AND d.refclassid = 'pg_catalog.pg_authid'::regclass
			AND d.refobjid = 'upgrade_test_raster_acl_user'::regrole
			AND d.deptype = 'a'
	) INTO grantee_dependency_preserved;

	IF NOT grantee_dependency_preserved THEN
		RAISE EXCEPTION 'raster_columns ACL dependency was not preserved';
	END IF;

	GRANT SELECT ON TABLE raster_columns TO public;
	REVOKE SELECT (r_table_name) ON TABLE raster_columns FROM upgrade_test_raster_acl_user;
	REVOKE ALL ON TABLE raster_columns FROM upgrade_test_raster_acl_user;
	REVOKE ALL ON TABLE raster_columns FROM upgrade_test_raster_acl_grantor CASCADE;
END
$$;

-- Drop deprecated functions
\i :regdir/hooks/drop-deprecated-functions.sql
