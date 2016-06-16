CREATE FUNCTION test(geom geometry, wrap float8, amount float8, exp geometry)
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
$$ LANGUAGE plpgsql;

SELECT 'P1', test(
  'POINT(0 0)', 2, 10,
  'POINT(10 0)');

SELECT 'P2', test(
  'POINT(0 0)', 2, -10,
  'POINT(0 0)');

SELECT 'P3', test(
  'POINT(0 0)', -2, -10,
  'POINT(-10 0)');

SELECT 'L1', test(
  'LINESTRING(0 0,10 0)', 2, 10,
  --'LINESTRING(2 0,12 0)');
  'MULTILINESTRING((10 0,12 0),(2 0,10 0))');

SELECT 'L2', test(
  'LINESTRING(0 0,10 0)', 8, -10,
  'MULTILINESTRING((0 0,8 0),(-2 0,0 0))');

SELECT 'L3', test(
  'LINESTRING(0 0,10 0)', 0, 10,
  'LINESTRING(0 0,10 0)');

SELECT 'L4', test(
  'LINESTRING(0 0,10 0)', 10, -10,
  'LINESTRING(0 0,10 0)');

SELECT 'ML1', test(
  'MULTILINESTRING((-10 0,0 0),(0 0,10 0))', 0, 20,
  'MULTILINESTRING((10 0,20 0),(0 0,10 0))');

SELECT 'ML2', test(
  'MULTILINESTRING((-10 0,0 0),(0 0,10 0))', 0, -20,
  'MULTILINESTRING((-10 0,0 0),(-20 0,-10 0))');

SELECT 'ML3', test(
  'MULTILINESTRING((10 0,5 0),(-10 0,0 0),(0 0,5 0))', 0, -20,
  'MULTILINESTRING((-10 0,0 0),(-15 0,-10 0),(-20 0,-15 0))');

SELECT 'A1', test(
  'POLYGON((0 0,10 0,10 10,0 10,0 0),
           (1 2,3 2,3 4,1 4,1 2),
           (4 2,6 2,6 4,4 4,4 2),
           (7 2,9 2,9 4,7 4,7 2))', 5, 10,
  'POLYGON((5 0,5 2,6 2,6 4,5 4,5 10,10 10,15 10,15 4,14 4,14 2,15 2,15 0,10 0,5 0),
           (11 2,13 2,13 4,11 4,11 2),
           (7 2,9 2,9 4,7 4,7 2))');

SELECT 'A2', test(
  'POLYGON((0 0,10 0,10 10,0 10,0 0),
           (1 2,3 2,3 4,1 4,1 2),
           (4 2,6 2,6 4,4 4,4 2),
           (7 2,9 2,9 4,7 4,7 2))', 5, -10,
  'POLYGON((-5 0,-5 2,-4 2,-4 4,-5 4,-5 10,0 10,5 10,5 4,4 4,4 2,5 2,5 0,0 0,-5 0),
           (1 2,3 2,3 4,1 4,1 2),
           (-3 2,-1 2,-1 4,-3 4,-3 2))');

SELECT 'C1', test(
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
  'GEOMETRYCOLLECTION(
    POLYGON((-5 0,-5 2,-4 2,-4 4,-5 4,-5 10,0 10,5 10,5 4,4 4,4 2,5 2,5 0,0 0,-5 0),
            (1 2,3 2,3 4,1 4,1 2),
            (-3 2,-1 2,-1 4,-3 4,-3 2)),
    MULTILINESTRING((0 40,5 40),(-5 40,0 40)),
    POINT(2 20),
    POINT(-3 -20)
  )');

DROP FUNCTION test(geometry, float8, float8, geometry);
