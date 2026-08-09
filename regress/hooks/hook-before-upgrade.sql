CREATE TABLE upgrade_test(g1 geometry, g2 geography);
INSERT INTO upgrade_test(g1,g2) VALUES
('POINT(0 0)', 'LINESTRING(0 0, 1 1)'),
('POINT(1 0)', 'LINESTRING(0 1, 1 1)');

-- Add view using ST_Union aggregate
-- See https://trac.osgeo.org/postgis/ticket/4386
CREATE VIEW upgrade_view_test_union AS
SELECT ST_Union(g1) FROM upgrade_test;

-- Add view using overlay functions
CREATE VIEW upgrade_view_test_overlay AS
SELECT
	ST_Intersection(g1, g1) as geometry_intersection,
	ST_Intersection(g2, g2) as geography_intersection,
	ST_Difference(g1, g1) as geometry_difference,
	ST_SymDifference(g1, g1) as geometry_symdifference
FROM upgrade_test;

-- Add view using unaryunion function
-- NOTE: 2.0.0 introduced ST_UnaryUnion
CREATE VIEW upgrade_view_test_unaryunion AS
SELECT
	ST_UnaryUnion(g1) as geometry_unaryunion
FROM upgrade_test;

-- Add view using unaryunion function
-- NOTE: 2.2.0 introduced ST_Subdivide
CREATE VIEW upgrade_view_test_subdivide AS
SELECT
	ST_Subdivide(g1, 256) as geometry_subdivide
FROM upgrade_test;

-- Add view using ST_ForceX function
-- NOTE: 3.1.0 changed them from taking only geometry
--       to also take optional zvalue/mvalue params
CREATE VIEW upgrade_view_test_force_dims AS
SELECT
	ST_Force3D(g1) as geometry_force3d,
	ST_Force3DZ(g1) as geometry_force3dz,
	ST_Force3DM(g1) as geometry_force3dm,
	ST_Force4D(g1) as geometry_force4d
FROM upgrade_test;

-- Add view using ST_AsKML function
-- NOTE: 2.0.0 changed them to add default params
CREATE VIEW upgrade_view_test_askml AS
SELECT
	ST_AsKML(g1) as geometry_askml,
	ST_AsKML(g2) as geography_askml
FROM upgrade_test;

-- Add view using ST_DWithin functions
-- See https://trac.osgeo.org/postgis/ticket/5494
CREATE VIEW upgrade_view_test_dwithin AS
SELECT
	ST_DWithin(NULL::text, NULL::text, NULL::float8) as text_dwithin,
	-- Available since 1.5.0, changed in 3.0.0 to add optional 4th use_spheroid param
	ST_DWithin(NULL::geography, NULL::geography, NULL::float8) as geography_dwithin,
	-- Available since 1.3.0
	ST_DWithin(NULL::geometry, NULL::geometry, NULL::float8) as geometry_dwithin
;

-- Add view using ST_ClusterKMeans windowing function
-- NOTE: 3.2.0 changed it to add max_radius parameter
CREATE VIEW upgrade_view_test_clusterkmeans AS
SELECT
	ST_ClusterKMeans(g1, 1) OVER ()
FROM upgrade_test;

-- This view uses ST_Distance signatures, available since 1.5.0
-- NOTE: 3.0.0 changed them to use default arguments
-- See https://trac.osgeo.org/postgis/ticket/5380
CREATE VIEW upgrade_view_test_distance AS
SELECT
	ST_Distance(g2, g2) geog_dist1,
	ST_Distance(g2, g2, true) geog_dist2
FROM upgrade_test;

-- Break probin of all postgis functions, as we expect
-- the upgrade procedure to replace them all
UPDATE pg_proc SET probin = probin || '-uninstalled'
WHERE probin like '%postgis%';


-- Change SECURITY of postgis_version() to DEFINER
-- to verify the bit is reset upon upgrade
--
-- NOTE: we pick postgis_version as one of the oldest
--       function names
--
ALTER FUNCTION postgis_version() SECURITY DEFINER;

-- Shorter overloads must not make default-argument upgrade helper calls
-- ambiguous. The callbacks only record that they ran.
CREATE TABLE upgrade_test_helper_overload_calls (
	helper_name text NOT NULL
);

CREATE FUNCTION _postgis_drop_function_by_identity(
	function_name text,
	function_arguments text
)
RETURNS void
LANGUAGE plpgsql
AS $$
BEGIN
	INSERT INTO upgrade_test_helper_overload_calls
	VALUES ('_postgis_drop_function_by_identity');
END;
$$;

CREATE FUNCTION _postgis_drop_function_by_signature(
	function_signature text
)
RETURNS void
LANGUAGE plpgsql
AS $$
BEGIN
	INSERT INTO upgrade_test_helper_overload_calls
	VALUES ('_postgis_drop_function_by_signature');
END;
$$;

-- Verify generated named-argument guards do not use an exact operator from
-- the extension schema. The callback only records that it ran.
CREATE SCHEMA IF NOT EXISTS postgis_upgrade_test_data;

CREATE TABLE postgis_upgrade_test_data.issue004_named_argument_operator_calls (
	called boolean NOT NULL
);

CREATE FUNCTION postgis_upgrade_test_data.issue004_named_argument_operator_callback(
	text[], text[]
)
RETURNS boolean
LANGUAGE plpgsql
AS $postgis_upgrade_test$
BEGIN
	INSERT INTO postgis_upgrade_test_data.issue004_named_argument_operator_calls
	VALUES (true);
	RETURN false;
END
$postgis_upgrade_test$;

CREATE OPERATOR <> (
	FUNCTION = postgis_upgrade_test_data.issue004_named_argument_operator_callback,
	LEFTARG = text[],
	RIGHTARG = text[]
);

-- Add the deprecated shape that makes the generated replacement guard run.
CREATE FUNCTION ST_TileEnvelope(
	zoom integer,
	x integer,
	y integer,
	bounds geometry
)
RETURNS geometry
LANGUAGE sql
IMMUTABLE STRICT
AS 'SELECT bounds';
