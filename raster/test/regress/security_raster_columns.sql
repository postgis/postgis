-----------------------------------------------------------------------
--
-- Verify that raster_columns does not bind a pre-install text overload.
--
-----------------------------------------------------------------------

SET client_min_messages TO warning;

WITH helper_oids AS (
	SELECT
		'public._raster_constraint_info_scale(name,name,name,text)'::regprocedure::oid AS attacker_oid,
		'public._raster_constraint_info_scale(name,name,name,character)'::regprocedure::oid AS postgis_oid
), view_dependencies AS (
	SELECT d.refobjid
	FROM pg_catalog.pg_depend AS d
	JOIN pg_catalog.pg_rewrite AS rw ON rw.oid = d.objid
	JOIN pg_catalog.pg_class AS v ON v.oid = rw.ev_class
	JOIN pg_catalog.pg_namespace AS n ON n.oid = v.relnamespace
	WHERE d.classid = 'pg_catalog.pg_rewrite'::regclass
		AND d.refclassid = 'pg_catalog.pg_proc'::regclass
		AND n.nspname = 'public'
		AND v.relname = 'raster_columns'
)
SELECT
	NOT EXISTS (
		SELECT 1
		FROM view_dependencies, helper_oids
		WHERE refobjid = attacker_oid
	),
	EXISTS (
		SELECT 1
		FROM view_dependencies, helper_oids
		WHERE refobjid = postgis_oid
	);

CREATE TABLE issue008_raster_columns_source (rast raster);

SELECT scale_x IS NULL AND scale_y IS NULL
FROM raster_columns
WHERE r_table_schema = 'public'
	AND r_table_name = 'issue008_raster_columns_source'
	AND r_raster_column = 'rast';

DROP TABLE issue008_raster_columns_source;
