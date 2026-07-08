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

WITH result AS (SELECT ST_Union(
	'LINESTRING(0.5 0, 9 0)'::geometry,
	'LINESTRING(7 0, 13.2 0)'::geometry,
 2) AS geom)
SELECT 'union', ST_AsText(geom) = CASE
    WHEN split_part(postgis_geos_version(), '.', 1)::integer * 100
         + split_part(postgis_geos_version(), '.', 2)::integer >= 315
    THEN 'LINESTRING(0 0,8 0,10 0,14 0)'
    ELSE 'MULTILINESTRING((0 0,8 0),(8 0,10 0),(10 0,14 0))'
END FROM result;

WITH result AS (SELECT ST_UnaryUnion(
	'GEOMETRYCOLLECTION(LINESTRING(0.5 0, 9 0), LINESTRING(7 0, 13.2 0))'::geometry,
2) AS geom)
SELECT 'unaryunion', ST_AsText(geom) = CASE
    WHEN split_part(postgis_geos_version(), '.', 1)::integer * 100
         + split_part(postgis_geos_version(), '.', 2)::integer >= 315
    THEN 'LINESTRING(0 0,8 0,10 0,14 0)'
    ELSE 'MULTILINESTRING((0 0,8 0),(8 0,10 0),(10 0,14 0))'
END FROM result;

WITH n AS (SELECT geom
FROM ST_Subdivide(
	'POLYGON((0 0,0 10,10 10,10 6,100 5.1, 100 10, 110 10, 110 0, 100 0,100 4.9,10 5,10 0,0 0))'::geometry,
6, 1) AS geom)
SELECT 'subdivide', count(geom), ST_AsText(ST_Normalize(ST_Union(geom)))
FROM n;
