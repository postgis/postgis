SELECT * FROM topology.layer;
\d upgrade_test.feature

DO $$
DECLARE
	call_count integer;
BEGIN
	SELECT calls.call_count
	INTO call_count
	FROM public.upgrade_test_pg_get_function_result_calls AS calls;

	IF call_count <> 0 THEN
		RAISE EXCEPTION
			'public.pg_get_function_result(regprocedure) was called % time(s) during extension upgrade',
			call_count;
	END IF;
END;
$$ LANGUAGE plpgsql;

DROP FUNCTION public.pg_get_function_result(regprocedure);
DROP TABLE public.upgrade_test_pg_get_function_result_calls;

DO $test$
BEGIN
	IF EXISTS (
		SELECT 1 FROM public.upgrade_add_column_overload_marker
	) THEN
		RAISE EXCEPTION
			'postgis_topology update called the public text overload';
	END IF;

	IF NOT EXISTS (
		SELECT 1
		FROM information_schema.columns
		WHERE table_schema = 'topology'
		  AND table_name = 'topology'
		  AND column_name = 'useslargeids'
		  AND data_type = 'boolean'
		  AND is_nullable = 'NO'
		  AND column_default = 'false'
	) THEN
		RAISE EXCEPTION
			'postgis_topology update did not add useslargeids safely';
	END IF;
END
$test$;

DROP FUNCTION public._postgis_add_column_to_table(
	text, text, text, boolean, text, text
);
DROP TABLE public.upgrade_add_column_overload_marker;

DO $postgis_upgrade_test$
BEGIN
	IF EXISTS (
		SELECT 1
		FROM upgrade_test.domain_constraint_callback
	) THEN
		RAISE EXCEPTION 'Topology upgrade invoked a non-catalog domain constraint callback';
	END IF;
END
$postgis_upgrade_test$;

DROP FUNCTION public.array_upper(bigint[], integer);
DROP FUNCTION public.array_lower(bigint[], integer);

-- https://trac.osgeo.org/postgis/ticket/5983
DROP INDEX upgrade_test.upgrade_test_feature_tg_id_idx;
SELECT topology.FixCorruptTopoGeometryColumn(schema_name, table_name, feature_column)
    FROM topology.layer;

\d upgrade_test.feature

-- See https://trac.osgeo.org/postgis/ticket/5102
SELECT topology.CopyTopology('upgrade_test', 'upgrade_test_copy');
INSERT INTO upgrade_test.domain_test values (
  '{1,2}'::topology.topoelement,
  '{{2,3}}'::topology.topoelementarray
);

SELECT * FROM topology.layer;

INSERT INTO upgrade_test.domain_test values (
  '{1,2}'::topology.topoelement,
  '{{2,3}}'::topology.topoelementarray
);

SELECT topology.DropTopology('upgrade_test');
SELECT topology.DropTopology('upgrade_test_copy');
