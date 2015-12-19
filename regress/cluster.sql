-- postgres
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

-- tests for window versions of the above

CREATE TEMPORARY TABLE cluster_intersecting_window_results AS
SELECT id, geom, ST_ClusterIntersectingW(geom) OVER (ORDER BY id) AS cid FROM cluster_inputs ORDER BY id;

-- should be 4 clusters total (including null cluster)
SELECT 't101', 4 = array_length(array_agg(DISTINCT cid), 1) FROM cluster_intersecting_window_results;
-- null geom has null cluster
SELECT 't102', bool_and(cid IS NULL) FROM cluster_intersecting_window_results WHERE geom IS NULL;
-- ids 1, 2, 4, and 7 in same cluster
SELECT 't103', 1 = count(DISTINCT cid) FROM cluster_intersecting_window_results WHERE id IN (1, 2, 4, 7);

CREATE TEMPORARY TABLE cluster_within_window_results AS
SELECT id, geom, ST_ClusterWithinW(geom, 1.4) OVER (ORDER BY id) AS cid FROM cluster_inputs ORDER BY id;
-- should be 4 clusters total (including null cluster)
SELECT 't201', 4 = array_length(array_agg(DISTINCT cid), 1) FROM cluster_within_window_results;
-- null geom has null cluster
SELECT 't202', bool_and(cid IS NULL) FROM cluster_within_window_results WHERE geom IS NULL;
-- ids 1, 2, 4, and 7 in same cluster
SELECT 't203', 1 = count(DISTINCT cid) FROM cluster_within_window_results WHERE id IN (1, 2, 4, 7);

