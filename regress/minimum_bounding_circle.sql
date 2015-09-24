SELECT 't1', ST_MinimumBoundingCircle(NULL::geometry) IS NULL;
SELECT 't2', ST_Equals(ST_MinimumBoundingCircle('POINT EMPTY'), 'POLYGON EMPTY'::geometry);
SELECT 't3', ST_SRID(ST_MinimumBoundingCircle('SRID=32611;POINT(4021690.58034526 6040138.01373556)')) = 32611;
SELECT 't4', ST_SRID(center) = 32611 FROM ST_MinimumBoundingRadius('SRID=32611;POINT(4021690.58034526 6040138.01373556)');
SELECT 't5', ST_Equals(center, 'POINT EMPTY') AND radius = 0 FROM ST_MinimumBoundingRadius('GEOMETRYCOLLECTION EMPTY');
SELECT 't6', ST_Equals(center, 'POINT (0 0.5)') AND radius = 0.5 FROM ST_MinimumBoundingRadius('MULTIPOINT ((0 0 0), (0 1 1))');
