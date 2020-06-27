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
SELECT 't2', ST_AsText(unnest(ST_ClusterIntersecting(array_agg(geom ORDER BY id)))) FROM cluster_inputs;
SELECT 't3', ST_AsText(unnest(ST_ClusterWithin(geom, 1.4 ORDER BY id))) FROM cluster_inputs;
SELECT 't4', ST_AsText(unnest(ST_ClusterWithin(array_agg(geom ORDER BY id), 1.5))) FROM cluster_inputs;

-- tests for ST_DBSCAN

CREATE TEMPORARY TABLE dbscan_inputs (id int, geom geometry);
INSERT INTO dbscan_inputs VALUES
(1, 'POINT (0 0)'),
(2, 'POINT (0 1)'),
(3, 'POINT (-0.5 0.5)'),
(4, 'POINT (1 0)'),
(5, 'POINT (1 1)'),
(6, 'POINT (1.0 0.5)');

/* minpoints = 1, equivalent to ST_ClusterWithin */
SELECT 't101', id, ST_ClusterDBSCAN(geom, eps := 0.8, minpoints := 1) OVER () from dbscan_inputs;

/* minpoints = 4, no clusters */
SELECT 't102', id, ST_ClusterDBSCAN(geom, eps := 0.8, minpoints := 4) OVER () from dbscan_inputs;

/* minpoints = 3, but eps too small to form cluster on left */
SELECT 't103', id, ST_ClusterDBSCAN(geom, eps := 0.6, minpoints := 3) OVER () from dbscan_inputs;

-- #3612
SELECT '#3612a', ST_ClusterDBSCAN(foo1.the_geom, 20.1, 5)OVER()  As result
							FROM ((SELECT geom  As the_geom
									FROM (VALUES ( ST_GeomFromEWKT('SRID=4326;POLYGONM((-71.1319 42.2503 1,-71.132 42.2502 3,-71.1323 42.2504 -2,-71.1322 42.2505 1,-71.1319 42.2503 0))') ),
											( ST_GeomFromEWKT('SRID=4326;POLYGONM((-71.1319 42.2512 0,-71.1318 42.2511 20,-71.1317 42.2511 -20,-71.1317 42.251 5,-71.1317 42.2509 4,-71.132 42.2511 6,-71.1319 42.2512 30))') ) ) As g(geom))) As foo1 LIMIT 3;
SELECT '#3612b', ST_ClusterDBSCAN(ST_Point(1,1), 20.1, 5) OVER();


-- ST_ClusterKMeans
select '#4100a', count(distinct result) from (SELECT ST_ClusterKMeans(foo1.the_geom, 3) OVER()  As result
  FROM ((SELECT ST_Collect(geom)  As the_geom
		FROM (VALUES ( ST_GeomFromEWKT('SRID=4326;MULTIPOLYGON(((-71.0821 42.3036 2,-71.0822 42.3036 2,-71.082 42.3038 2,-71.0819 42.3037 2,-71.0821 42.3036 2)))') ),
	( ST_GeomFromEWKT('SRID=4326;POLYGON((-71.1261 42.2703 1,-71.1257 42.2703 1,-71.1257 42.2701 1,-71.126 42.2701 1,-71.1261 42.2702 1,-71.1261 42.2703 1))') ) ) As g(geom) CROSS JOIN generate_series(1,3) As i GROUP BY i )) As foo1 LIMIT 10) kmeans;

select '#4100b', count(distinct cid) from (select ST_ClusterKMeans(geom,2) over () as cid from (values ('POINT(0 0)'::geometry), ('POINT(0 0)')) g(geom)) kmeans;


select '#4101a', count(distinct result) from (SELECT ST_ClusterKMeans(foo1.the_geom, 3) OVER()  As result
							FROM ((SELECT ST_GeomFromText('POINT EMPTY',4326) As the_geom
			UNION ALL SELECT ST_GeomFromText('MULTIPOINT EMPTY',4326) As the_geom
			UNION ALL SELECT ST_GeomFromText('MULTIPOLYGON EMPTY',4326) As the_geom
			UNION ALL SELECT ST_GeomFromText('LINESTRING EMPTY',4326) As the_geom
			UNION ALL SELECT ST_GeomFromText('MULTILINESTRING EMPTY',4326) As the_geom ) ) As foo1 LIMIT 10) kmeans;

select '#4101b', count(distinct cid) from (select ST_ClusterKMeans(geom,2) over () as cid from (values ('POINT EMPTY'::geometry), ('POINT EMPTY')) g(geom)) kmeans;

select '3d_support-1', count(distinct cid) from (select ST_ClusterKMeans(geom,2) over () as cid from (values ('POINT(0 0 1)'::geometry), ('POINT(0 0 5)'), ('POINT(0 0 7)')) g(geom)) kmeans;
select '3d_support-2', count(distinct cid) from (select ST_ClusterKMeans(geom,2) over () as cid from (values ('LINESTRING(0 0 1, 0 0 -1)'::geometry), ('POINT(0 0 5)'), ('POINT(0 0 7)')) g(geom)) kmeans;
select '3d_support-3', count(distinct cid) from (select ST_ClusterKMeans(geom,2) over () as cid from (values ('LINESTRING(0 0, 0 0)'::geometry), ('POINT(0 0)'), ('POINT(0 0)')) g(geom)) kmeans;

-- check that null and empty is handled in the clustering
select '#4071', count(distinct a), count(distinct b), count(distinct c)  from
(select
	ST_ClusterKMeans(geom, 1) over () a,
	ST_ClusterKMeans(geom, 2) over () b,
	ST_ClusterKMeans(geom, 3) over () c
from (values (null::geometry), ('POINT(1 1)'), ('POINT EMPTY'), ('POINT(0 0)'), ('POINT(4 4)')) as g (geom)) z;
