-- These assertions require the fixtures created by hook-before-upgrade.sql.
DO $$
BEGIN
	IF EXISTS (SELECT 1 FROM upgrade_test_helper_overload_calls) THEN
		RAISE EXCEPTION 'upgrade helper overload was called';
	END IF;
END;
$$;

DROP FUNCTION _postgis_drop_function_by_identity(text, text);
DROP FUNCTION _postgis_drop_function_by_signature(text);
DROP TABLE upgrade_test_helper_overload_calls;

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
