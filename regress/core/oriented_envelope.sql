-- It's strict
SELECT 't1', ST_OrientedEnvelope(NULL::geometry);
-- Empty polygon on empty inputs
SELECT 't2', ST_AsText(ST_OrientedEnvelope('POINT EMPTY'));
-- SRID is preserved
SELECT 't3', ST_SRID(ST_OrientedEnvelope('SRID=32611;POINT(4021690.58034526 6040138.01373556)'));
-- Can return Point or LineString on degenerate inputs
SELECT 't4', ST_AsText(ST_OrientedEnvelope('MULTIPOINT ((0 0), (-1 -1), (2 2))'));
SELECT 't5', ST_AsText(ST_OrientedEnvelope('POINT (0.9625 2)'));
-- Also works for normal inputs
SELECT 't6', ST_AsText(ST_Normalize(ST_OrientedEnvelope('MULTIPOINT ((0 0), (-1 -1), (3 2))')), 2);

