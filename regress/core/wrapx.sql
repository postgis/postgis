CREATE FUNCTION test_wrapx(geom geometry, wrap float8, amount float8, exp geometry)
RETURNS text AS $$
DECLARE
	obt geometry;
BEGIN
	obt = ST_Normalize(ST_WrapX(geom, wrap, amount));
	IF ST_OrderingEquals(obt, exp) THEN
		RETURN 'OK';
	ELSE
		RETURN 'KO:' || ST_AsEWKT(obt) || ' != ' || ST_AsEWKT(exp);
	END IF;
END
$$ LANGUAGE 'plpgsql';

CREATE OR REPLACE FUNCTION geos_version()
RETURNS integer AS $$
    SELECT (matches[1]::integer * 100) + matches[2]::integer
    FROM regexp_match(postgis_geos_version(), E'^([0-9]+)\\.([0-9]+)') AS matches;
$$ LANGUAGE 'sql' IMMUTABLE;


SELECT 'P1', test_wrapx(
	'POINT(0 0)', 2, 10,
	'POINT(10 0)');

SELECT 'P2', test_wrapx(
	'POINT(0 0)', 2, -10,
	'POINT(0 0)');

SELECT 'P3', test_wrapx(
	'POINT(0 0)', -2, -10,
	'POINT(-10 0)');

SELECT 'L1', test_wrapx(
	'LINESTRING(0 0,10 0)', 2, 10,
	CASE WHEN geos_version() >= 315
	  THEN 'LINESTRING(2 0,10 0,12 0)'
	  ELSE 'MULTILINESTRING((10 0,12 0),(2 0,10 0))'
	END);

SELECT 'L2', test_wrapx(
	'LINESTRING(0 0,10 0)', 8, -10,
	CASE WHEN geos_version() >= 315
	  THEN 'LINESTRING(-2 0,0 0,8 0)'
	  ELSE 'MULTILINESTRING((0 0,8 0),(-2 0,0 0))'
	END);

SELECT 'L3', test_wrapx(
	'LINESTRING(0 0,10 0)', 0, 10,
	'LINESTRING(0 0,10 0)');

SELECT 'L4', test_wrapx(
	'LINESTRING(0 0,10 0)', 10, -10,
	'LINESTRING(0 0,10 0)');

SELECT 'ML1', test_wrapx(
	'MULTILINESTRING((-10 0,0 0),(0 0,10 0))', 0, 20,
	'MULTILINESTRING((10 0,20 0),(0 0,10 0))');

SELECT 'ML2', test_wrapx(
	'MULTILINESTRING((-10 0,0 0),(0 0,10 0))', 0, -20,
	'MULTILINESTRING((-10 0,0 0),(-20 0,-10 0))');

SELECT 'ML3', test_wrapx(
	'MULTILINESTRING((10 0,5 0),(-10 0,0 0),(0 0,5 0))', 0, -20,
	'MULTILINESTRING((-10 0,0 0),(-15 0,-10 0),(-20 0,-15 0))');

SELECT 'A1', test_wrapx(
	'POLYGON((0 0,10 0,10 10,0 10,0 0),
					 (1 2,3 2,3 4,1 4,1 2),
					 (4 2,6 2,6 4,4 4,4 2),
					 (7 2,9 2,9 4,7 4,7 2))', 5, 10,
	'POLYGON((5 0,5 2,6 2,6 4,5 4,5 10,10 10,15 10,15 4,14 4,14 2,15 2,15 0,10 0,5 0),
					 (11 2,13 2,13 4,11 4,11 2),
					 (7 2,9 2,9 4,7 4,7 2))');

SELECT 'A2', test_wrapx(
	'POLYGON((0 0,10 0,10 10,0 10,0 0),
					 (1 2,3 2,3 4,1 4,1 2),
					 (4 2,6 2,6 4,4 4,4 2),
					 (7 2,9 2,9 4,7 4,7 2))', 5, -10,
	'POLYGON((-5 0,-5 2,-4 2,-4 4,-5 4,-5 10,0 10,5 10,5 4,4 4,4 2,5 2,5 0,0 0,-5 0),
					 (1 2,3 2,3 4,1 4,1 2),
					 (-3 2,-1 2,-1 4,-3 4,-3 2))');

SELECT 'C1', test_wrapx(
	'GEOMETRYCOLLECTION(
		POLYGON((0 0,10 0,10 10,0 10,0 0),
						(1 2,3 2,3 4,1 4,1 2),
						(4 2,6 2,6 4,4 4,4 2),
						(7 2,9 2,9 4,7 4,7 2)),
		POINT(2 20),
		POINT(7 -20),
		LINESTRING(0 40,10 40)
	)',
	5, -10,
	CASE WHEN geos_version() >= 315
	  THEN 'GEOMETRYCOLLECTION(POLYGON((-5 0,-5 2,-4 2,-4 4,-5 4,-5 10,0 10,5 10,5 4,4 4,4 2,5 2,5 0,0 0,-5 0),(1 2,3 2,3 4,1 4,1 2),(-3 2,-1 2,-1 4,-3 4,-3 2)),LINESTRING(-5 40,0 40,5 40),POINT(2 20),POINT(-3 -20))'
	  ELSE 'GEOMETRYCOLLECTION(
		POLYGON((-5 0,-5 2,-4 2,-4 4,-5 4,-5 10,0 10,5 10,5 4,4 4,4 2,5 2,5 0,0 0,-5 0),
						(1 2,3 2,3 4,1 4,1 2),
						(-3 2,-1 2,-1 4,-3 4,-3 2)),
		MULTILINESTRING((0 40,5 40),(-5 40,0 40)),
		POINT(2 20),
		POINT(-3 -20)
	)'
	END);



DROP FUNCTION geos_version();
DROP FUNCTION test_wrapx(geometry, float8, float8, geometry);
