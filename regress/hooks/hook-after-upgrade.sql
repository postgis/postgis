-- TODO: move all these views and tables under postgis_upgrade_test_data
DO $upgrade_test_operator$
BEGIN
	IF EXISTS (SELECT 1 FROM public.upgrade_test_operator_calls) THEN
		RAISE EXCEPTION 'generated upgrade selected a non-catalog integer-array operator';
	END IF;
END
$upgrade_test_operator$;

DROP OPERATOR public.< (integer[], integer[]);
DROP FUNCTION public.upgrade_test_integer_array_lt(integer[], integer[]);
DROP TABLE public.upgrade_test_operator_calls;

DROP VIEW IF EXISTS upgrade_view_test_overlay;
DROP VIEW IF EXISTS upgrade_view_test_unaryunion;
DROP VIEW IF EXISTS upgrade_view_test_subdivide;
DROP VIEW IF EXISTS upgrade_view_test_union;
DROP VIEW IF EXISTS upgrade_view_test_force_dims;
DROP VIEW IF EXISTS upgrade_view_test_askml;
DROP VIEW IF EXISTS upgrade_view_test_dwithin;
DROP VIEW IF EXISTS upgrade_view_test_clusterkmeans;
DROP VIEW IF EXISTS upgrade_view_test_distance;
DROP TABLE IF EXISTS upgrade_test;

DO $$
BEGIN
	IF NOT EXISTS (
		SELECT 1
		FROM pg_catalog.pg_proc AS p
		WHERE p.oid = pg_catalog.to_regprocedure(
			'_postgis_drop_function_by_identity(text,text)'
		)
	) THEN
		RAISE EXCEPTION 'identity-helper collision function is absent';
	END IF;

	IF EXISTS (SELECT 1 FROM upgrade_test_identity_helper_call) THEN
		RAISE EXCEPTION 'identity-helper collision function ran';
	END IF;
END;
$$;

DROP FUNCTION _postgis_drop_function_by_identity(text, text);
DROP TABLE upgrade_test_identity_helper_call;

\i :regdir/hooks/issue-009-signature-helper-after.sql

DO LANGUAGE plpgsql $postgis_upgrade_test$
BEGIN
	IF EXISTS (
		SELECT 1
		FROM postgis_upgrade_test_data.issue004_named_argument_operator_calls
	)
	THEN
		RAISE EXCEPTION
			'generated named-argument guard used the test text-array operator';
	END IF;

	-- The replacement must leave the supported function available.
	PERFORM ST_TileEnvelope(0, 0, 0);
END
$postgis_upgrade_test$;

-- Drop any upgrade test data
DROP SCHEMA IF EXISTS postgis_upgrade_test_data CASCADE;

-- Drop deprecated functions
\i :regdir/hooks/drop-deprecated-functions.sql
