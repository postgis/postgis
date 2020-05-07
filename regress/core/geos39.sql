
SELECT 'mic-box' AS name, st_astext(center) AS center,
       st_astext(nearest) AS nearest,
       round(radius::numeric,4) AS radius
FROM ST_MaximumInscribedCircle('Polygon((0 0, 100 0, 100 100, 0 100, 0 0))'::geometry);

SELECT 'mic-empty' AS name, st_astext(center) AS center,
       st_astext(nearest) AS nearest,
       round(radius::numeric,4) AS radius
FROM ST_MaximumInscribedCircle('Polygon Empty'::geometry);

SELECT 'mic-null' AS name, center, nearest, radius
FROM ST_MaximumInscribedCircle(NULL);

SELECT 'mic-line' AS name, st_astext(center) AS center,
       st_astext(nearest) AS nearest,
       round(radius::numeric,4) AS radius
FROM ST_MaximumInscribedCircle('LINESTRING(0 0, 100 0, 100 100, 0 100, 0 0)'::geometry);

SELECT 'mic-mpoint' AS name, st_astext(center) AS center,
       st_astext(nearest) AS nearest,
       round(radius::numeric,4) AS radius
FROM ST_MaximumInscribedCircle('MULTIPOINT(0 0, 100 0, 100 100, 0 100, 0 0)'::geometry);

SELECT 'mic-point' AS name, st_astext(center) AS center,
       st_astext(nearest) AS nearest,
       round(radius::numeric,4) AS radius
FROM ST_MaximumInscribedCircle('POINT(0 0)'::geometry);
