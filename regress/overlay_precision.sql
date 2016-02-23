-- postgis_reg

SELECT 't1', ST_AsText(ST_UnaryUnion('LINESTRING (0 0, 1 1, 0 1, 0.22 0)', op_precision := 1e-4));
SELECT 't2', ST_AsText(ST_Intersection('LINESTRING (0 0, 1 0.5)', 'LINESTRING (0 0.6, 1 0)', op_precision := 1e-3));
SELECT 't3', ST_AsText(ST_Difference('LINESTRING (0 0, 1.1 0.9)', 'POLYGON ((0 0, 1 0, 1 0.6, 0 0.37, 0 0))', op_precision := 1e-4));
SELECT 't4', ST_AsText(ST_SymmetricDifference('POLYGON ((0 0, 0.6 0, 0 1, 0 0))', 'POLYGON ((0 0, 1 0, 1 0.6, 0 0.37, 0 0))', op_precision := 1e-3));
SELECT 't5', ST_AsText(ST_Union(geom::geometry, 1e-3)) FROM (VALUES (1, 'LINESTRING (0 0, 1 0.5)'), (1, 'LINESTRING (0 0.6, 1 0)')) sq (grp, geom) GROUP BY grp;
