DO $BODY$
DECLARE
	rec RECORD;
BEGIN
	FOR rec IN
		SELECT
			array_agg(
				regexp_replace(
					regexp_replace(
						regexp_replace(
							oid::regprocedure::text,
							'double precision', 'float8'
						),
						'\(([^)])',
						'(NULL::\1'
					),
					',',
					',NULL::',
					'g'
				)
				||
				CASE WHEN prokind = 'w' THEN ' OVER()' ELSE '' END
				|| ' as f' || oid
			) f
		FROM pg_catalog.pg_proc
		WHERE proname like 'st\_%'
		AND prokind in ('f','w')
		-- See https://trac.osgeo.org/postgis/ticket/5498 and
		-- remove when fixed
		AND proname != 'st_approxquantile'
		-- We skip functions taking anyelement for now
		-- as those ones would need us to cast to some
		-- real type
		AND proname in ( 'st_asmvt' )
	LOOP
		EXECUTE format(
			'CREATE VIEW postgis_locking_view AS SELECT %s',
			array_to_string(rec.f, ',')
		);
	END LOOP;


END;
$BODY$ LANGUAGE 'plpgsql';
