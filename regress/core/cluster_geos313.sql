CREATE TEMPORARY TABLE cluster_geos313_inputs (id int, geom geometry);
INSERT INTO cluster_geos313_inputs VALUES
(1, 'LINESTRING (0 0, 1 1)'),
(2, 'LINESTRING (5 5, 4 4)');

SELECT 'cluster_relate_null_matrix', ST_ClusterRelateWin(geom, NULL) OVER ()
FROM cluster_geos313_inputs
LIMIT 1;
