DO LANGUAGE plpgsql $$
DECLARE
	expected_oid oid;
	actual_oid oid;
	call_count integer;
BEGIN
	IF to_regclass('issue_009_signature_helper_probe') IS NOT NULL THEN
		SELECT helper_oid, issue_009_signature_helper_probe.call_count
		INTO expected_oid, call_count
		FROM issue_009_signature_helper_probe;

		SELECT '_postgis_drop_function_by_signature(text)'::regprocedure::oid
		INTO actual_oid;

		IF actual_oid <> expected_oid THEN
			RAISE EXCEPTION 'one-argument helper changed: expected OID %, got %',
				expected_oid, actual_oid;
		END IF;

		IF call_count <> 0 THEN
			RAISE EXCEPTION 'one-argument helper executed % times', call_count;
		END IF;
	END IF;
END;
$$;

DROP FUNCTION IF EXISTS _postgis_drop_function_by_signature(text);
DROP TABLE IF EXISTS issue_009_signature_helper_probe;
