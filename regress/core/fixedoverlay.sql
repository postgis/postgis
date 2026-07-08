CREATE OR REPLACE FUNCTION geos_version()
RETURNS integer AS $$
    SELECT (matches[1]::integer * 100) + matches[2]::integer
    FROM regexp_match(postgis_geos_version(), E'^([0-9]+)]\\.([0-9]+)') AS matches;
$$ LANGUAGE 'sql' IMMUTABLE;

SELECT 'intersection', ST_AsText(ST_Intersection(
	'LINESTRING(0 0, 9 0)'::geometry,
	'LINESTRING(7 0, 13.2 0)'::geometry,
2));

SELECT 'difference', ST_AsText(ST_Difference(
	'LINESTRING(0 0, 9 0)'::geometry,
	'LINESTRING(7 0, 13.2 0)'::geometry,
2));

SELECT 'symdifference', ST_AsText(ST_SymDifference(
	'LINESTRING(0 0, 9 0)'::geometry,
	'LINESTRING(7 0, 13.2 0)'::geometry,
2));



WITH n AS (SELECT geom
FROM ST_Subdivide(
	'POLYGON((0 0,0 10,10 10,10 6,100 5.1, 100 10, 110 10, 110 0, 100 0,100 4.9,10 5,10 0,0 0))'::geometry,
6, 1) AS geom)
SELECT 'subdivide', count(geom), ST_AsText(ST_Normalize(ST_Union(geom)))
FROM n;


DROP FUNCTION geos_version();
