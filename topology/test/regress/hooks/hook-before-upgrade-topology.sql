SELECT topology.createTopology('upgrade_test');

-- Create some TopoGeometry data
CREATE TABLE upgrade_test.feature(id serial primary key);
SELECT topology.AddTopoGeometryColumn('upgrade_test', 'upgrade_test', 'feature', 'tg', 'linear');
INSERT INTO upgrade_test.feature(tg) SELECT topology.toTopoGeom('LINESTRING(0 0, 10 0)', 'upgrade_test', 1);
CREATE INDEX upgrade_test_feature_tg_id_idx ON upgrade_test.feature ( id(tg) );

-- Create some TopoGeometry data
CREATE TABLE upgrade_test.domain_test(a topology.topoelement, b topology.topoelementarray);
INSERT INTO upgrade_test.domain_test values (
  '{1,2}'::topology.topoelement,
  '{{2,3}}'::topology.topoelementarray
);

-- An unqualified pg_get_function_result(regprocedure) call in an upgrade
-- script can select this exact-match overload instead of the pg_catalog
-- function, whose argument is oid. Keep the overload harmless: record the
-- call and delegate to the intended function.
CREATE TABLE public.upgrade_test_pg_get_function_result_calls (
	call_count integer NOT NULL
);
INSERT INTO public.upgrade_test_pg_get_function_result_calls VALUES (0);

CREATE FUNCTION public.pg_get_function_result(function_oid regprocedure)
RETURNS text
LANGUAGE plpgsql
AS $$
BEGIN
	UPDATE public.upgrade_test_pg_get_function_result_calls
	SET call_count = call_count + 1;
	RETURN pg_catalog.pg_get_function_result(function_oid::oid);
END;
$$;

-- Simulate the pre-3.6.0 catalog state that needs the upgrade helper.
ALTER TABLE topology.topology DROP COLUMN useslargeids;

-- This harmless overload records an unsafe function-resolution result.
CREATE TABLE public.upgrade_add_column_overload_marker (
	called boolean NOT NULL
);

CREATE FUNCTION public._postgis_add_column_to_table(
	table_name text,
	column_name text,
	data_type text,
	is_not_null boolean,
	default_value text,
	deprecated_in_version text
)
RETURNS void
LANGUAGE plpgsql
AS $marker$
BEGIN
	INSERT INTO public.upgrade_add_column_overload_marker VALUES (true);
END
$marker$;

-- Keep the callback harmless. The after-upgrade hook rejects any marker row.
CREATE TABLE upgrade_test.domain_constraint_callback (
	callback_name text NOT NULL
);

CREATE FUNCTION public.array_upper(bigint[], integer)
RETURNS integer
LANGUAGE plpgsql
SECURITY INVOKER
AS $callback$
BEGIN
	INSERT INTO upgrade_test.domain_constraint_callback VALUES ('array_upper');
	RETURN pg_catalog.array_upper($1, $2);
END
$callback$;

CREATE FUNCTION public.array_lower(bigint[], integer)
RETURNS integer
LANGUAGE plpgsql
SECURITY INVOKER
AS $callback$
BEGIN
	INSERT INTO upgrade_test.domain_constraint_callback VALUES ('array_lower');
	RETURN pg_catalog.array_lower($1, $2);
END
$callback$;
