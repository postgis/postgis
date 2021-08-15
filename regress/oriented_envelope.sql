-- It's strict
SELECT 't1', ST_OrientedEnvelope(NULL::geometry) IS NULL;
-- Empty polygon on empty inputs
SELECT 't2', ST_Equals(ST_OrientedEnvelope('POINT EMPTY'), 'POLYGON EMPTY'::geometry);
-- SRID is preserved
SELECT 't3', ST_SRID(ST_OrientedEnvelope('SRID=32611;POINT(4021690.58034526 6040138.01373556)')) = 32611;
-- Can return Point or LineString on degenerate inputs
SELECT 't4', ST_Equals('LINESTRING (-1 -1, 2 2)', ST_OrientedEnvelope('MULTIPOINT ((0 0), (-1 -1), (2 2))'));
SELECT 't5', ST_Equals('POINT (0.9625 2)', ST_OrientedEnvelope('POINT (0.9625 2)'));
-- Also works for normal inputs
-- Check using text to avoid precision difference between various GEOS versions
SELECT 't6', ST_AsText(ST_OrientedEnvelope('MULTIPOINT ((0 0), (-1 -1), (3 2))')) = 'POLYGON((3 2,2.88 2.16,-1.12 -0.84,-1 -1,3 2))';

SELECT '#4863', st_contains(geometry, st_scale(st_orientedenvelope(geometry),
 'SRID=3857; POINT(0.8 0.8)', st_centroid(geometry))) from (select
 'SRID=3857; POLYGON((-141972.789895508 6755731.24770785,-141935.49511986
 6755733.56891884,-141934.403428904 6755716.1146343,-141971.698204552
 6755713.77835553,-141972.789895508 6755731.24770785))'::geometry as
 geometry) x;

