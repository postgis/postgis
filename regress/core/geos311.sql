
-- ST_ConcaveHull --
WITH hull AS (
	SELECT ST_Buffer(ST_MakePoint(0,0),1) AS geom
)
SELECT 'concavehull01',
	abs(ST_Area(geom) - ST_Area(ST_ConcaveHull(geom, 1.0))) < 0.01 AS area_convex
FROM hull;

WITH hull AS (
	SELECT ST_Difference(
		ST_Buffer(ST_MakePoint(0,0),10),
		ST_Translate(ST_Buffer(ST_MakePoint(0,0),10), 5, 0)) AS geom
)
SELECT 'concavehull02',
	ST_Area(ST_ConcaveHull(geom, 0.5)) > 0.0 AS area_non_zero,
	ST_GeometryType(ST_ConcaveHull(geom, 0.5)) AS type,
	ST_Area(ST_ConcaveHull(geom, 0.5, true)) > 0.0 AS nohole_area_non_zero
FROM hull;
