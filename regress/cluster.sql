-- tests for ST_ClusterIntersecting and ST_ClusterWithin

CREATE TEMPORARY TABLE cluster_inputs (id int, geom geometry);
INSERT INTO cluster_inputs VALUES
(1, 'LINESTRING (0 0, 1 1)'),
(2, 'LINESTRING (5 5, 4 4)'),
(3, NULL),
(4, 'LINESTRING (0 0, -1 -1)'),
(5, 'LINESTRING (6 6, 7 7)'),
(6, 'POLYGON EMPTY'),
(7, 'POLYGON ((0 0, 4 0, 4 4, 0 4, 0 0))');

SELECT 't1', ST_AsText(unnest(ST_ClusterIntersecting(geom ORDER BY id))) FROM cluster_inputs;
SELECT 't2', ST_AsText(unnest(ST_ClusterIntersecting(ST_Accum(geom ORDER BY id)))) FROM cluster_inputs;
SELECT 't3', ST_AsText(unnest(ST_ClusterWithin(geom, 1.4 ORDER BY id))) FROM cluster_inputs;
SELECT 't4', ST_AsText(unnest(ST_ClusterWithin(ST_Accum(geom ORDER BY id), 1.5))) FROM cluster_inputs;
