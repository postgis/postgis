
-- ST_ConcaveHull --
WITH hull AS (
	SELECT ST_Buffer(ST_MakePoint(0,0),1) AS geom
)
SELECT 'concavehull01-poly' as test,
	abs(ST_Area(geom) - ST_Area(ST_ConcaveHull(geom, 1.0))) < 0.01 AS area_convex
FROM hull;

WITH hull AS (
	SELECT ST_Difference(
		ST_Buffer(ST_MakePoint(0,0),10),
		ST_Translate(ST_Buffer(ST_MakePoint(0,0),10), 5, 0)) AS geom
)
SELECT 'concavehull02-poly' as test,
	ST_Area(ST_ConcaveHull(geom, 0.5)) > 0.0 AS area_non_zero,
	ST_GeometryType(ST_ConcaveHull(geom, 0.5)) AS type,
	ST_Area(ST_ConcaveHull(geom, 0.5, true)) > 0.0 AS nohole_area_non_zero
FROM hull;

-- ST_ConcaveHull --
WITH hull AS (
	SELECT ST_Buffer(ST_MakePoint(0,0),1) AS geom
)
SELECT 'concavehull01-points' as test,
	abs(ST_Area(geom) - ST_Area(ST_ConcaveHull(ST_Points(geom), 1.0))) < 0.01 AS area_convex
FROM hull;

WITH hull AS (
	SELECT ST_Difference(
		ST_Buffer(ST_MakePoint(0,0),10),
		ST_Translate(ST_Buffer(ST_MakePoint(0,0),10), 5, 0)) AS geom
)
SELECT 'concavehull02-points' as test,
	ST_Area(ST_ConcaveHull(ST_Points(geom), 0.5)) > 0.0 AS area_non_zero,
	ST_GeometryType(ST_ConcaveHull(ST_Points(geom), 0.5)) AS type,
	ST_Area(ST_ConcaveHull(ST_Points(geom), 0.5, true)) > 0.0 AS nohole_area_non_zero
FROM hull;

-- ST_SimplifyPolygonHull
SELECT 'simplifypolygonhull-1' AS test,
	ST_AsText(ST_SimplifyPolygonHull('POLYGON((0 0, 0 1, 1 1, 1 0, 0 0))'::geometry, 0.5),1);

SELECT 'simplifypolygonhull-2' AS test,
	ST_AsText(ST_SimplifyPolygonHull(
		'POLYGON((0 0, 0 1, 0.5 0.1, 1 1, 1 0, 0 0))'::geometry,
		0.5),1);

SELECT 'simplifypolygonhull-3' AS test,
	ST_AsText(ST_SimplifyPolygonHull(
		'POLYGON((0 0, 0 1, 0.5 0.1, 0.5 0.1, 1 1, 1 0, 0 0))'::geometry,
		0.85),1);

