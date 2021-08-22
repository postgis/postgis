
SELECT 'mic-box' AS name,
       st_astext(center, 4) AS center,
       st_astext(nearest, 4) AS nearest,
       round(radius::numeric,4) AS radius
FROM ST_MaximumInscribedCircle('Polygon((0 0, 100 0, 99 98, 0 100, 0 0))'::geometry);

SELECT 'mic-empty' AS name,
       st_astext(center, 4) AS center,
       st_astext(nearest, 4) AS nearest,
       round(radius::numeric,4) AS radius
FROM ST_MaximumInscribedCircle('Polygon Empty'::geometry);

SELECT 'mic-null' AS name, center, nearest, radius
FROM ST_MaximumInscribedCircle(NULL);

SELECT 'mic-line' AS name,
       st_astext(center, 4) AS center,
       st_astext(nearest, 4) AS nearest,
       round(radius::numeric,4) AS radius
FROM ST_MaximumInscribedCircle('LINESTRING(0 0, 100 0, 99 98, 0 100, 0 0)'::geometry);

SELECT 'mic-mpoint' AS name,
       st_astext(center, 4) AS center,
       st_astext(nearest, 4) AS nearest,
       round(radius::numeric,4) AS radius
FROM ST_MaximumInscribedCircle('MULTIPOINT(0 0, 100 0, 99 98, 0 100, 0 0)'::geometry);

SELECT 'mic-point' AS name,
       st_astext(center, 4) AS center,
       st_astext(nearest, 4) AS nearest,
       round(radius::numeric,4) AS radius
FROM ST_MaximumInscribedCircle('POINT(0 0)'::geometry);

WITH p AS (
	SELECT 'Polygon((0 0, 100 0, 99 98, 0 100, 0 0))'::geometry AS ply
)
SELECT 'mic-cte' AS name, ST_AsText(st_snaptogrid((ST_MaximumInscribedCircle(ply)).center,0.001))
	FROM p;

-- ST_ReducePrecision
SELECT 'rp-1', ST_AsText(ST_ReducePrecision('POINT(1.412 19.323)', 0.1));
SELECT 'rp-2', ST_AsText(ST_ReducePrecision('POINT(1.412 19.323)', 1.0));
SELECT 'rp-3', ST_AsText(ST_ReducePrecision('POINT(1.412 19.323)', 10));
