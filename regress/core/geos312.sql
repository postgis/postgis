-- #4011
SELECT '#4011',
ST_GeometryType(ST_LineMerge(geom)) AS linemerge,
ST_AsText(geom) AS geom,
ST_IsEmpty(geom) AS empty,
ST_IsValid(geom) AS valid
FROM (VALUES
('LINESTRING(0 0, 1 1)'),
('MULTILINESTRING((0 0, 1 1), (1 1, 2 2))'),
('MULTILINESTRING((0 0, 1 1), EMPTY)'),
('MULTILINESTRING(EMPTY, EMPTY)'),
(NULL),
('POLYGON((0 0, 1 0, 1 1, 0 0))'),
('GEOMETRYCOLLECTION(LINESTRING(0 0, 1 1))'),
('MULTILINESTRING EMPTY'),
('MULTILINESTRING((0 0, 0 0))')
) as f(geom);
