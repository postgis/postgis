-- tests for ST_ClusterIntersecting

WITH st_clusterintersecting_regr_inputs AS
  (SELECT unnest(ARRAY['LINESTRING (0 0, 1 1)'::geometry,
		       'LINESTRING (5 5, 4 4)'::geometry,
		       'LINESTRING (6 6, 7 7)'::geometry,
		       'LINESTRING (0 0, -1 -1)'::geometry,
		       'POLYGON ((0 0, 4 0, 4 4, 0 4, 0 0))'::geometry]) AS geom)

SELECT 't1', ST_AsText(unnest(ST_ClusterIntersecting(geom))) FROM st_clusterintersecting_regr_inputs;
