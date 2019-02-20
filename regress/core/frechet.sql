-- tests for Frechet distances

-- linestring and linestring
SELECT 'frechet_ls_ls', st_frechetdistance(
	'LINESTRING (0 0, 2 1)'::geometry
	, 'LINESTRING (0 0, 2 0)'::geometry);
-- 1.0

-- other linestrings
SELECT 'frechet_ls_ls_2', st_frechetdistance(
	'LINESTRING (0 0, 2 0)'::geometry,
	'LINESTRING (0 1, 1 2, 2 1)'::geometry)::numeric(12,6);
-- 2.23606797749979

-- linestring and multipoint
SELECT 'frechet_ls_mp', st_frechetdistance(
	'LINESTRING (0 0, 2 0)'::geometry,
	'MULTIPOINT (0 1, 1 0, 2 1)'::geometry);
-- 1.0

-- another linestring and linestring
SELECT 'frechet_ls_ls_3', st_frechetdistance(
	'LINESTRING (0 0, 100 0)'::geometry,
	'LINESTRING (0 0, 50 50, 100 0)'::geometry)::numeric(12,6);
-- 70.7106781186548

-- rechet with densification
SELECT 'frechetdensify_ls_ls', st_frechetdistance(
	'LINESTRING (0 0, 100 0)'::geometry,
	'LINESTRING (0 0, 50 50, 100 0)'::geometry, 0.5);
-- 50.0
