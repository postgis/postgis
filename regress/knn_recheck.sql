-- create table
CREATE TABLE knn_recheck_geom(gid serial primary key, geom geometry);
INSERT INTO knn_recheck_geom(gid,geom)
SELECT ROW_NUMBER() OVER(ORDER BY x,y) AS gid, ST_Point(x*0.777,y*0.777) As geom
FROM generate_series(-100,100, 1) AS x CROSS JOIN generate_series(-300,10000,10) As y;

INSERT INTO knn_recheck_geom(gid, geom)
SELECT 500000, 'LINESTRING(100 300, -10 700, 400 123, -300 10000)'::geometry;


INSERT INTO knn_recheck_geom(gid, geom)
SELECT 500001, 'POLYGON((100 3000, -10 700, 400 123, 405 124, 100 3000))'::geometry;

INSERT INTO knn_recheck_geom(gid,geom)
SELECT 600000 + ROW_NUMBER() OVER(ORDER BY gid) AS gid, ST_Buffer(geom,1000,2) As geom
FROM knn_recheck_geom
WHERE gid IN(1000, 10000, 2000, 40000);


-- without index order should match st_distance order --
-- point check
SELECT gid, RANK() OVER(ORDER BY ST_Distance( 'POINT(200 1000)'::geometry, geom) )
FROM knn_recheck_geom
ORDER BY 'POINT(200 1000)'::geometry <-> geom LIMIT 5;

-- linestring check
SELECT gid, RANK() OVER(ORDER BY ST_Distance( 'LINESTRING(200 100, -10 600)'::geometry, geom) )
FROM knn_recheck_geom
ORDER BY 'LINESTRING(200 100, -10 600)'::geometry <-> geom LIMIT 5;

-- lateral check before index
SELECT a.gid, b.gid As match, RANK() OVER(PARTITION BY a.gid ORDER BY ST_Distance(a.geom, b.geom) ) As true_rn, b.rn  As knn_rn
FROM knn_recheck_geom As a 
	LEFT JOIN 
		LATERAL ( SELECT  gid, geom, RANK() OVER(ORDER BY a.geom <-> g.geom) As rn 
			FROM knn_recheck_geom As g WHERE a.gid <> g.gid ORDER BY a.geom <-> g.geom LIMIT 5) As b ON true
	WHERE a.gid IN(50000,50001,70000,61000)
ORDER BY a.gid, b.rn;

-- create index and repeat
CREATE INDEX idx_knn_recheck_geom_gist ON knn_recheck_geom USING gist(geom);

-- point check after index
SELECT gid, RANK() OVER(ORDER BY ST_Distance( 'POINT(200 1000)'::geometry, geom) )
FROM knn_recheck_geom
ORDER BY 'POINT(200 1000)'::geometry <-> geom LIMIT 5;

-- lateral check after index - currently is wrong
SELECT a.gid, b.gid As match, RANK() OVER(PARTITION BY a.gid ORDER BY ST_Distance(a.geom, b.geom) ) As true_rn, b.rn  As knn_rn
FROM knn_recheck_geom As a 
	LEFT JOIN 
		LATERAL ( SELECT  gid, geom, RANK() OVER(ORDER BY a.geom <-> g.geom) As rn 
			FROM knn_recheck_geom As g WHERE a.gid <> g.gid ORDER BY a.geom <-> g.geom LIMIT 5) As b ON true
	WHERE a.gid IN(50000,50001,70000,61000)
ORDER BY a.gid, b.rn;

DROP TABLE knn_recheck_geom;

-- geography tests
DELETE FROM spatial_ref_sys where srid = 4326;
INSERT INTO "spatial_ref_sys" ("srid","auth_name","auth_srid","proj4text") VALUES (4326,'EPSG',4326,'+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs ');
-- create table
CREATE TABLE knn_recheck_geog(gid serial primary key, geog geography);
INSERT INTO knn_recheck_geog(gid,geog)
SELECT ROW_NUMBER() OVER(ORDER BY x,y) AS gid, ST_Point(x*1.11,y*0.95)::geography As geog
FROM generate_series(-100,100, 1) AS x CROSS JOIN generate_series(-90,90,1) As y;

INSERT INTO knn_recheck_geog(gid, geog)
SELECT 500000, 'LINESTRING(-95 -10, -11 65, 5 10, -70 60)'::geography;

INSERT INTO knn_recheck_geog(gid, geog)
SELECT 500001, 'POLYGON((-95 10, -95.6 10.5, -95.9 10.75, -95 10))'::geography;

INSERT INTO knn_recheck_geog(gid,geog)
SELECT 600000 + ROW_NUMBER() OVER(ORDER BY gid) AS gid, ST_Buffer(geog,1000) As geog
FROM knn_recheck_geog
WHERE gid IN(1000, 10000, 2000, 2614, 40000);


SELECT gid, RANK() OVER(ORDER BY ST_Distance( 'POINT(95 10)'::geography, geog,false) )
FROM knn_recheck_geog
ORDER BY 'POINT(95 10)'::geography <-> geog LIMIT 5;

SELECT gid, RANK() OVER(ORDER BY ST_Distance( 'POINT(-95 -10)'::geography, geog, false) )
FROM knn_recheck_geog
ORDER BY 'POINT(-95 -10)'::geography <-> geog LIMIT 5;

-- lateral check before index
SELECT a.gid,  ARRAY(SELECT  gid
			FROM knn_recheck_geog As g WHERE a.gid <> g.gid ORDER BY ST_Distance(a.geog, g.geog, false) LIMIT 5) = ARRAY(SELECT  gid
			FROM knn_recheck_geog As g WHERE a.gid <> g.gid ORDER BY a.geog <-> g.geog LIMIT 5) As dist_order_agree
FROM knn_recheck_geog As a 
	WHERE a.gid IN(500000,500010,1000,2614)
ORDER BY a.gid;


-- create index and repeat
CREATE INDEX idx_knn_recheck_geog_gist ON knn_recheck_geog USING gist(geog);

SELECT gid
FROM knn_recheck_geog
ORDER BY 'POINT(95 10)'::geography <-> geog LIMIT 5;

SELECT gid
FROM knn_recheck_geog
ORDER BY 'POINT(-95 -10)'::geography <-> geog LIMIT 5;

-- check after index
set enable_seqscan = false;  --sometimes doesn't want to use index
SELECT a.gid,  ARRAY(SELECT  gid
			FROM knn_recheck_geog As g WHERE a.gid <> g.gid ORDER BY ST_Distance(a.geog, g.geog, false) LIMIT 5) = ARRAY(SELECT  gid
			FROM knn_recheck_geog As g WHERE a.gid <> g.gid ORDER BY a.geog <-> g.geog LIMIT 5) As dist_order_agree
FROM knn_recheck_geog As a 
	WHERE a.gid IN(500000,500010,1000,2614)
ORDER BY a.gid;

DROP TABLE knn_recheck_geog;

--
-- Delete inserted spatial data
--
DELETE FROM spatial_ref_sys WHERE srid = 4326;

