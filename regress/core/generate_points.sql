-- Check containment and count of generated points
WITH circle AS (
	SELECT ST_Point(5000,5000) AS pt,
	       ST_Buffer(ST_Point(5000,5000),10) AS geom
),
pts AS (
	SELECT (ST_Dump(ST_GeneratePoints(geom, 50))).geom AS geom
	FROM circle
)
SELECT 'generate_points_1', bool_and(ST_Distance(circle.pt, pts.geom) < 10), count(pts.geom)
FROM circle, pts

