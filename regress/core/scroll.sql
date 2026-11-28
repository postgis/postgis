SELECT ST_Scroll('POINT(0 0)', 'POINT(0 0)');
SELECT ST_Scroll('LINESTRING(0 0, 10 0, 5 5,0 0)', 'LINESTRING(0 0,1 0)');
SELECT ST_Scroll('LINESTRING(0 0, 10 0, 5 5)', 'POINT(0 0)');

SELECT ST_AsEWKT(ST_Scroll('SRID=4326;LINESTRING(0 0, 10 0, 5 5,0 0)', 'POINT(0 0)'));
SELECT ST_AsEWKT(ST_Scroll('SRID=4326;LINESTRING(0 0, 10 0, 5 5,0 0)', 'POINT(5 5)'));
SELECT ST_AsEWKT(ST_Scroll('SRID=4326;LINESTRING(0 0, 10 0, 5 5,0 0)', 'POINT(10 0)'));

SELECT ST_AsEWKT(ST_Scroll('SRID=4326;LINESTRINGM(0 0 0, 10 0 2, 5 5 4,0 0 0)', 'POINT(10 0 2)'));

SELECT ST_AsEWKT(ST_Scroll('SRID=4326;LINESTRINGZ(0 0 0, 10 0 2, 5 5 4,0 0 0)', 'POINT(5 5 4)'));

SELECT ST_AsEWKT(ST_Scroll('SRID=4326;LINESTRING(0 0 0 1, 10 0 2 0, 5 5 4 2,0 0 0 1)', 'POINT(5 5 4 2)'));

-- The input geometry datum must remain unchanged when its point array is scrolled.
WITH input AS MATERIALIZED (
  SELECT 'LINESTRING(0 0, 10 0, 5 5, 0 0)'::geometry AS geom
), before AS (
  SELECT geom, ST_AsEWKT(geom) AS wkt FROM input
), call AS (
  SELECT geom, wkt, ST_Scroll(geom, 'POINT(5 5)'::geometry) AS result FROM before
)
SELECT ST_AsEWKT(result), ST_AsEWKT(geom) = wkt FROM call;
