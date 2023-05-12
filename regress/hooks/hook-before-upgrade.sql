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

-- Add view using ST_DWithin function
-- NOTE: 3.0.0 changed them to add default params
CREATE VIEW upgrade_view_test_dwithin AS
SELECT
	ST_DWithin(g1::text, g1::text, 1) as text_dwithin,
	ST_DWithin(g2, g2, 1) as geography_dwithin
FROM upgrade_test;

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
