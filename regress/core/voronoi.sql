-- postgres

-- SRID is preserved
SELECT 1,  32145 = ST_SRID(ST_VoronoiPolygons('SRID=32145;MULTIPOINT (0 0, 1 1, 2 2)'));
-- NULL -> NULL
SELECT 2,  ST_VoronoiPolygons(NULL) IS NULL;
-- NULL tolerance produces error
SELECT 3,  ST_VoronoiPolygons('MULTIPOINT (0 0, 1 1, 2 2)', NULL);
-- Tolerance can't be negative
SELECT 5,  ST_VoronoiPolygons('MULTIPOINT (0 0, 1 1, 2 2)', -2);
-- Output types are correct
SELECT 6,  GeometryType(ST_VoronoiPolygons('MULTIPOINT (0 0, 1 1, 2 2)')) = 'GEOMETRYCOLLECTION';
SELECT 7,  GeometryType(ST_VoronoiLines('MULTIPOINT (0 0, 1 1, 2 2)')) = 'MULTILINESTRING';
-- Clipping extent is handled correctly
SELECT 8,  ST_Equals(ST_Envelope('LINESTRING (-20 -10, 10 10)'::geometry), ST_Envelope(ST_VoronoiPolygons('MULTIPOINT (0 0, 1 1, 2 2)', 0.0, 'MULTIPOINT (-20 -10, 10 10)')));
