-- Returns NULL for NULL geometry
SELECT 't1', ST_QuantizeCoordinates(NULL::geometry, 6) IS NULL;
-- Throws error if no precision specified
SELECT 't2', ST_QuantizeCoordinates('POINT (3 7)', NULL);
-- Preserves SRID
SELECT 't3', ST_SRID(ST_QuantizeCoordinates('SRID=32611;POINT (3 7)', 5)) = 32611;
-- Carries X precision through to other dimensions if not specified
SELECT 't4', ST_X(ST_QuantizeCoordinates('POINT (1.2345678 1.2345678)', 3)) = ST_Y(ST_QuantizeCoordinates('POINT (1.2345678 1.2345678)', 3));
-- Or they can be specified independently
SELECT 't5', ST_X(ST_QuantizeCoordinates('POINT (1.2345678 1.2345678)', 7, 4)) != ST_Y(ST_QuantizeCoordinates('POINT (1.2345678 1.2345678)', 7, 4));
-- Significant digits are preserved
WITH input AS (SELECT ST_MakePoint(1.23456789012345, 0) AS geom)
SELECT 't6', bool_and(abs(ST_X(geom)-ST_X(ST_QuantizeCoordinates(geom, i))) < pow(10, -i))
FROM input, generate_series(1,15) AS i
