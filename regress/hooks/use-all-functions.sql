DO $BODY$
DECLARE
	rec RECORD;
BEGIN
	FOR rec IN
		SELECT
			array_agg(
				regexp_replace(
					regexp_replace(
						oid::regprocedure::text,
						'\(([^)])',
						'(NULL::\1'
					),
					',',
					',NULL::',
					'g'
				) || ' as f' || oid
			) f
		FROM pg_catalog.pg_proc
		WHERE proname like 'st\_%'
	LOOP
		EXECUTE format(
			'CREATE VIEW postgis_locking_view AS SELECT %s',
			array_to_string(rec.f, ',')
		);
	END LOOP;


END;
$BODY$ LANGUAGE 'plpgsql';
