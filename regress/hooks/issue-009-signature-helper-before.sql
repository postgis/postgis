-- A one-argument overload must not make upgrade cleanup calls ambiguous.
CREATE TABLE IF NOT EXISTS issue_009_signature_helper_probe (
	helper_oid oid NOT NULL,
	call_count integer NOT NULL
);

CREATE OR REPLACE FUNCTION _postgis_drop_function_by_signature(function_signature text)
RETURNS void AS $$
BEGIN
	UPDATE issue_009_signature_helper_probe
	SET call_count = call_count + 1;
END;
$$ LANGUAGE plpgsql;

INSERT INTO issue_009_signature_helper_probe
SELECT '_postgis_drop_function_by_signature(text)'::regprocedure::oid, 0
WHERE NOT EXISTS (SELECT FROM issue_009_signature_helper_probe);
