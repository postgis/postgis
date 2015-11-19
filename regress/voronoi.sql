-- postgres

-- SRID is preserved
SELECT 1,  32145 = ST_SRID(ST_Voronoi('SRID=32145;MULTIPOINT (0 0, 1 1, 2 2)'));
-- NULL -> NULL
SELECT 2,  ST_Voronoi(NULL) IS NULL;
SELECT 3,  ST_Voronoi('MULTIPOINT (0 0, 1 1, 2 2)', NULL, 0, NULL) IS NULL;
SELECT 4,  ST_Voronoi('MULTIPOINT (0 0, 1 1, 2 2)', NULL, NULL, true) IS NULL;
-- Tolerance can't be negative
SELECT 5,  ST_Voronoi('MULTIPOINT (0 0, 1 1, 2 2)', NULL, -2);
-- Output types are correct
SELECT 6,  GeometryType(ST_Voronoi('MULTIPOINT (0 0, 1 1, 2 2)')) = 'GEOMETRYCOLLECTION';
SELECT 7,  GeometryType(ST_Voronoi('MULTIPOINT (0 0, 1 1, 2 2)', NULL, 0, false)) = 'MULTILINESTRING';
-- Clipping extent is handled correctly
SELECT 8,  ST_Equals(ST_Envelope('LINESTRING (-20 -10, 10 10)'::geometry), ST_Envelope(ST_Voronoi('MULTIPOINT (0 0, 1 1, 2 2)', 'MULTIPOINT (-20 -10, 10 10)')));
