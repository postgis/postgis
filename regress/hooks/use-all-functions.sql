DO $BODY$
DECLARE
	rec RECORD;
BEGIN
	DROP SCHEMA IF EXISTS postgis_upgrade_test_data CASCADE;
	CREATE SCHEMA postgis_upgrade_test_data;
	FOR rec IN
		SELECT
			array_agg(
				regexp_replace(
					regexp_replace(
						regexp_replace(
							oid::regprocedure::text,
							'double precision', 'float8'
						),
						E'\\(([^)])',
						E'(NULL::\\1'
					),
					',',
					',NULL::',
					'g'
				)
				||
				CASE WHEN prokind = 'w' THEN ' OVER()' ELSE '' END
				|| '::text as f' || oid
			) f
		FROM pg_catalog.pg_proc
		WHERE
		pronamespace::regnamespace::text !~ '^pg_'
		AND pronamespace::regnamespace::text <> 'information_schema'
		-- We only consider normal and window functions
		-- ( not aggregates or procedures )
		AND prokind in ('f','w')
		-- Skip function known to be problematic
		AND proname NOT IN (
			'st_approxquantile' -- https://trac.osgeo.org/postgis/ticket/5498
		)
		-- Skip functions using VARIADIC or OUTPUT arguments
		AND ( NOT coalesce(proargmodes, '{}') && ARRAY['v', 'o']::"char"[] )
		-- Skip function having arguments of internal types
		AND ( NOT proargtypes::oid[] && ARRAY ['internal'::regtype]::oid[] )
		-- Skip function having anyelement return type, to avoid:
		-- ERROR:  function returning record called in context that cannot accept type record
		AND ( NOT prorettype = ANY( ARRAY ['anyelement'::regtype] ) )
	LOOP
		EXECUTE format(
			'CREATE VIEW postgis_upgrade_test_data.locking_view AS SELECT %s',
			array_to_string(rec.f, ',')
		);
	END LOOP;


END;
$BODY$ LANGUAGE 'plpgsql';


SELECT 'INFO: Most PostGIS functions locked into view usage';
