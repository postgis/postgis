
SELECT 'mic-box' AS name,
       st_astext(st_snaptogrid(center, 0.0001)) AS center,
       st_astext(st_snaptogrid(nearest, 0.0001)) AS nearest,
       round(radius::numeric,4) AS radius
FROM ST_MaximumInscribedCircle('Polygon((0 0, 100 0, 99 98, 0 100, 0 0))'::geometry);

SELECT 'mic-empty' AS name,
       st_astext(st_snaptogrid(center, 0.0001)) AS center,
       st_astext(st_snaptogrid(nearest, 0.0001)) AS nearest,
       round(radius::numeric,4) AS radius
FROM ST_MaximumInscribedCircle('Polygon Empty'::geometry);

SELECT 'mic-null' AS name, center, nearest, radius
FROM ST_MaximumInscribedCircle(NULL);

SELECT 'mic-line' AS name,
       st_astext(st_snaptogrid(center, 0.0001)) AS center,
       st_astext(st_snaptogrid(nearest, 0.0001)) AS nearest,
       round(radius::numeric,4) AS radius
FROM ST_MaximumInscribedCircle('LINESTRING(0 0, 100 0, 99 98, 0 100, 0 0)'::geometry);

SELECT 'mic-mpoint' AS name,
       st_astext(st_snaptogrid(center, 0.0001)) AS center,
       st_astext(st_snaptogrid(nearest, 0.0001)) AS nearest,
       round(radius::numeric,4) AS radius
FROM ST_MaximumInscribedCircle('MULTIPOINT(0 0, 100 0, 99 98, 0 100, 0 0)'::geometry);

SELECT 'mic-point' AS name,
       st_astext(st_snaptogrid(center, 0.0001)) AS center,
       st_astext(st_snaptogrid(nearest, 0.0001)) AS nearest,
       round(radius::numeric,4) AS radius
FROM ST_MaximumInscribedCircle('POINT(0 0)'::geometry);

WITH p AS (
	SELECT 'Polygon((0 0, 100 0, 99 98, 0 100, 0 0))'::geometry AS ply
)
SELECT 'mic-cte' AS name, ST_AsText(st_snaptogrid((ST_MaximumInscribedCircle(ply)).center,0.001))
	FROM p;
