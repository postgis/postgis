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
						'\(([^)])',
						'(NULL::\1'
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
		-- We use the ST_ prefix as a sign that this function
		-- belongs to PostGIS and is meant for public usage,
		-- but the condition could be improved
		proname like 'st\_%'
		-- We only consider normal and window functions
		-- ( not aggregates or procedures )
		AND prokind in ('f','w')
		-- Skip function known to be problematic
		AND proname NOT IN (
			'fake'
			,'st_approxquantile' -- https://trac.osgeo.org/postgis/ticket/5498
--			,'st_mapalgebrafct' -- https://trac.osgeo.org/postgis/ticket/5500
--			,'st_bandmetadata' -- unreported
--			,'st_bandnodatavalue' -- unreported
--			,'st_setgeoreference' -- unreported
--			,'st_setbandisnodata' -- unreported
--			,'st_setbandnodatavalue' -- unreported
--			,'st_addband' -- unreported
--			,'st_bandisnodata' -- unreported
--			,'st_makeemptyraster' -- unreported
--			,'st_bandpath' -- unreported
--			,'st_bandpixeltype' -- unreported
--			,'st_value' -- unreported
--			,'st_georeference' -- unreported
--			,'st_summarystats' -- unreported
		)
		-- Skip function known to be problematic
		AND oid::regprocedure::text NOT IN (
			'fake'
--			,'st_polygon(raster,integer)' -- unreported
		)
		-- Skip functions taking polymorphic arguments (anyelement)
		-- as those ones would need us to cast to some real type.
		AND proname NOT IN ( 'st_asmvt', 'st_fromflatgeobuf' )
		-- Skip functions using VARIADIC or OUTPUT arguments
		AND ( NOT coalesce(proargmodes, '{}') && ARRAY['v'::"char",'o'] )
	LOOP
		EXECUTE format(
			'CREATE VIEW postgis_upgrade_test_data.locking_view AS SELECT %s',
			array_to_string(rec.f, ',')
		);
	END LOOP;


END;
$BODY$ LANGUAGE 'plpgsql';