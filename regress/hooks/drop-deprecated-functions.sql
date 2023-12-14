-- Drop functions deprecated on upgrade, if possible (could be possible later)
DO LANGUAGE 'plpgsql' $DROPALL$
DECLARE
	rec pg_catalog.pg_proc;
BEGIN
	FOR rec IN
		SELECT * FROM pg_catalog.pg_proc
		WHERE proname ~ '_deprecated_by_postgis'
	LOOP
		RAISE NOTICE 'Dropping function %', rec.oid::regprocedure;
		BEGIN
			EXECUTE pg_catalog.format('DROP FUNCTION %s', rec.oid::regprocedure);
		EXCEPTION
		WHEN dependent_objects_still_exist THEN
			RAISE NOTICE 'Could not drop function % as dependent objects still exist', rec.oid::regprocedure;
		WHEN OTHERS THEN
			RAISE EXCEPTION 'Got % (%)', SQLERRM, SQLSTATE;
		END;
	END LOOP;
END;
$DROPALL$;

