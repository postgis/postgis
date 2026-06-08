-- create table
CREATE TABLE knn_recheck_geom(gid serial primary key, geom geometry);
INSERT INTO knn_recheck_geom(gid,geom)
SELECT ROW_NUMBER() OVER(ORDER BY x,y) AS gid, ST_Point(x*0.777,y*0.887) As geom
FROM generate_series(-100,1000, 7) AS x CROSS JOIN generate_series(-300,1000,9) As y;

INSERT INTO knn_recheck_geom(gid, geom)
SELECT 500000 + i, ST_Translate('LINESTRING(-100 300, 500 700, 400 123, 500 10000, 1 1)'::geometry, i*2000,0)
FROM generate_series(0,10) i;

INSERT INTO knn_recheck_geom(gid, geom)
SELECT 500100 + i, ST_Translate('POLYGON((100 800, 100 700, 400 123, 405 124, 100 800))'::geometry,0,i*2000)
FROM generate_series(0,3) i;


INSERT INTO knn_recheck_geom(gid,geom)
SELECT 600000 + ROW_NUMBER() OVER(ORDER BY gid) AS gid, ST_Translate(ST_Buffer(geom,8,15 ),100,300) As geom
FROM knn_recheck_geom
WHERE gid IN(1000, 10000, 2000,3000);


-- without index order should match st_distance order --
-- point check

SELECT '#1' As t, gid, ST_Distance( 'POINT(-305 998.5)'::geometry, geom)::numeric(10,2)
FROM knn_recheck_geom
ORDER BY 'POINT(-305 998.5)'::geometry <-> geom LIMIT 5;

-- linestring check
SELECT '#2' As t, gid, ST_Distance( 'MULTILINESTRING((-95 -300, 100 200, 100 323),(-50 2000, 30 6000))'::geometry, geom)::numeric(12,4)
FROM knn_recheck_geom
ORDER BY 'MULTILINESTRING((-95 -300, 100 200, 100 323),(-50 2000, 30 6000))'::geometry <-> geom LIMIT 5;

-- lateral check before index
SELECT '#3' As t, a.gid, b.gid As match, ST_Distance(a.geom, b.geom)::numeric(15,4) As true_rn, b.knn_dist::numeric(15,4)
FROM knn_recheck_geom As a
	LEFT JOIN
		LATERAL ( SELECT  gid, geom, a.geom <-> g.geom As knn_dist
			FROM knn_recheck_geom As g WHERE a.gid <> g.gid ORDER BY a.geom <-> g.geom LIMIT 5) As b ON true
	WHERE a.gid IN(1,500101)
ORDER BY a.gid, true_rn, b.gid;

-- create index and repeat
CREATE INDEX idx_knn_recheck_geom_gist ON knn_recheck_geom USING gist(geom);
vacuum analyze knn_recheck_geom;

set enable_seqscan = false;
SELECT '#1' As t, gid, ST_Distance( 'POINT(-305 998.5)'::geometry, geom)::numeric(10,2)
FROM knn_recheck_geom
ORDER BY 'POINT(-305 998.5)'::geometry <-> geom LIMIT 5;

-- linestring check
SELECT '#2' As t, gid, ST_Distance( 'MULTILINESTRING((-95 -300, 100 200, 100 323),(-50 2000, 30 6000))'::geometry, geom)::numeric(12,4)
FROM knn_recheck_geom
ORDER BY 'MULTILINESTRING((-95 -300, 100 200, 100 323),(-50 2000, 30 6000))'::geometry <-> geom LIMIT 5;

-- lateral check before index
SELECT '#3' As t, a.gid, b.gid As match, ST_Distance(a.geom, b.geom)::numeric(15,4) As true_rn, b.knn_dist::numeric(15,4)
FROM knn_recheck_geom As a
	LEFT JOIN
		LATERAL ( SELECT  gid, geom, a.geom <-> g.geom As knn_dist
			FROM knn_recheck_geom As g WHERE a.gid <> g.gid ORDER BY a.geom <-> g.geom LIMIT 5) As b ON true
	WHERE a.gid IN(1,500101)
ORDER BY a.gid, true_rn, b.gid;

DROP TABLE knn_recheck_geom;

-- create table
CREATE TABLE knn_recheck_geog(gid serial primary key, geog geography);
INSERT INTO knn_recheck_geog(gid,geog)
SELECT ROW_NUMBER() OVER(ORDER BY x,y) AS gid, ST_Point(x*1.11,y*0.95)::geography As geog
FROM generate_series(-100,100, 1) AS x CROSS JOIN generate_series(-90,90,1) As y;

INSERT INTO knn_recheck_geog(gid, geog)
SELECT 500000, 'LINESTRING(-95 -10, -93 -10.5, -90 -10.6, -95 -10.5, -95 -10)'::geography;

INSERT INTO knn_recheck_geog(gid, geog)
SELECT 500001, 'POLYGON((-95 10, -95.6 10.5, -95.9 10.75, -95 10))'::geography;

INSERT INTO knn_recheck_geog(gid,geog)
SELECT 600000 + ROW_NUMBER() OVER(ORDER BY gid) AS gid, ST_Buffer(geog,1000) As geog
FROM knn_recheck_geog
WHERE gid IN(1000, 10000, 2000, 2614, 40000);


SELECT '#1g' As t, gid, ST_Distance( 'POINT(-95 -10)'::geography, geog, false)::numeric(12,4) ,
    ('POINT(-95 -10)'::geography <-> geog )::numeric(12,4)
FROM knn_recheck_geog
ORDER BY 'POINT(-95 -10)'::geography <-> geog LIMIT 5;

SELECT '#2g' As t, gid, ST_Distance( 'LINESTRING(75 10, 75 12, 80 20)'::geography, geog, false)::numeric(12,4),
    ('LINESTRING(75 10, 75 12, 80 20)'::geography <-> geog)::numeric(12,4) As knn_dist
FROM knn_recheck_geog
ORDER BY 'LINESTRING(75 10, 75 12, 80 20)'::geography <-> geog LIMIT 5;

-- lateral check before index
SELECT '#3g' As t, a.gid,  ARRAY(SELECT  gid
			FROM knn_recheck_geog As g WHERE a.gid <> g.gid ORDER BY ST_Distance(a.geog, g.geog, false) LIMIT 5) = ARRAY(SELECT  gid
			FROM knn_recheck_geog As g WHERE a.gid <> g.gid ORDER BY a.geog <-> g.geog LIMIT 5) As dist_order_agree
FROM knn_recheck_geog As a
	WHERE a.gid IN(500000,500010,1000)
ORDER BY a.gid;


-- create index and repeat
CREATE INDEX idx_knn_recheck_geog_gist ON knn_recheck_geog USING gist(geog);
vacuum analyze knn_recheck_geog;
set enable_seqscan = false;

SELECT '#1g' As t, gid, ST_Distance( 'POINT(-95 -10)'::geography, geog, false)::numeric(12,4) ,
    ('POINT(-95 -10)'::geography <-> geog )::numeric(12,4)
FROM knn_recheck_geog
ORDER BY 'POINT(-95 -10)'::geography <-> geog LIMIT 5;

SELECT '#2g' As t, gid, ST_Distance( 'LINESTRING(75 10, 75 12, 80 20)'::geography, geog, false)::numeric(12,4),
    ('LINESTRING(75 10, 75 12, 80 20)'::geography <-> geog)::numeric(12,4) As knn_dist
FROM knn_recheck_geog
ORDER BY 'LINESTRING(75 10, 75 12, 80 20)'::geography <-> geog LIMIT 5;

SELECT '#3g' As t, a.gid,  ARRAY(SELECT  g.gid
			FROM knn_recheck_geog As g WHERE a.gid <> g.gid ORDER BY ST_Distance(a.geog, g.geog, false) LIMIT 5) = ARRAY(SELECT  gid
			FROM knn_recheck_geog As g WHERE a.gid <> g.gid ORDER BY a.geog <-> g.geog LIMIT 5) As dist_order_agree
FROM knn_recheck_geog As a
	WHERE a.gid IN(500000,500010,1000)
ORDER BY a.gid;

DROP TABLE knn_recheck_geog;

--now the nd operator tests
-- create table and load
CREATE TABLE knn_recheck_geom_nd(gid serial primary key, geom geometry);
INSERT INTO knn_recheck_geom_nd(gid,geom)
SELECT ROW_NUMBER() OVER(ORDER BY x,y,z) AS gid, ST_MakePoint(x*0.777,y*0.887,z*1.05) As geom
FROM generate_series(-100,1000, 7) AS x ,
    generate_series(-300,1000,9) As y,
 generate_series(1005,10000,5555) As z ;

 -- 3d lines
INSERT INTO knn_recheck_geom_nd(gid, geom)
SELECT 500000 + i, ST_Translate('LINESTRING(-100 300 500, 500 700 600, 400 123 0, 500 10000 -1234, 1 1 5000)'::geometry, i*2000,0)
FROM generate_series(0,10) i;


-- 3d polygons
INSERT INTO knn_recheck_geom_nd(gid, geom)
SELECT 500100 + i, ST_Translate('POLYGON((100 800 5678, 100 700 5678, 400 123 5678, 405 124 5678, 100 800 5678))'::geometry,0,i*2000)
FROM generate_series(0,3) i;

-- polyhedral surface --
INSERT INTO knn_recheck_geom_nd(gid,geom)
SELECT 600000 + row_number() over(), ST_Translate(the_geom,100, 450,1000) As the_geom
		FROM (VALUES ( ST_GeomFromText(
'PolyhedralSurface(
((0 0 0, 0 0 1, 0 1 1, 0 1 0, 0 0 0)),
((0 0 0, 0 1 0, 1 1 0, 1 0 0, 0 0 0)), ((0 0 0, 1 0 0, 1 0 1, 0 0 1, 0 0 0)),  ((1 1 0, 1 1 1, 1 0 1, 1 0 0, 1 1 0)),
((0 1 0, 0 1 1, 1 1 1, 1 1 0, 0 1 0)),  ((0 0 1, 1 0 1, 1 1 1, 0 1 1, 0 0 1))
)') ) ,
( ST_GeomFromText(
'PolyhedralSurface(
((0 0 0, 0 0 1, 0 1 1, 0 1 0, 0 0 0)),
((0 0 0, 0 1 0, 1 1 0, 1 0 0, 0 0 0)) )') ) )
As foo(the_geom) ;

-- without index order should match st_3ddistance order --
-- point check
SELECT '#1nd-3' As t, gid, ST_3DDistance( 'POINT(-305 998.5 1000)'::geometry, geom)::numeric(12,4) As dist3d,
('POINT(-305 998.5 1000)'::geometry <<->> geom)::numeric(12,4) As dist_knn
FROM knn_recheck_geom_nd
ORDER BY 'POINT(-305 998.5 1000)'::geometry <<->> geom LIMIT 5;

-- linestring check
SELECT '#2nd-3' As t, gid, ST_3DDistance( 'MULTILINESTRING((-95 -300 5000, 105 451 1000, 100 323 200),(-50 2000 456, 30 6000 789))'::geometry::geometry, geom)::numeric(12,4),
 ('MULTILINESTRING((-95 -300 5000, 105 451 1000, 100 323 200),(-50 2000 456, 30 6000 789))'::geometry <<->> geom)::numeric(12,4) As knn_dist
FROM knn_recheck_geom_nd
ORDER BY 'MULTILINESTRING((-95 -300 5000, 105 451 1000, 100 323 200),(-50 2000 456, 30 6000 789))'::geometry <<->> geom LIMIT 5;

-- lateral test
SELECT '#3nd-3' As t, a.gid, b.gid As match, ST_3DDistance(a.geom, b.geom)::numeric(15,4) As true_rn, b.knn_dist::numeric(15,4)
FROM knn_recheck_geom_nd As a
	LEFT JOIN
		LATERAL ( SELECT  gid, geom, a.geom <<->> g.geom As knn_dist
			FROM knn_recheck_geom_nd As g WHERE a.gid <> g.gid ORDER BY a.geom <<->> g.geom LIMIT 5) As b ON true
	WHERE a.gid IN(1,600001)
ORDER BY a.gid, true_rn, b.gid;

-- create index and repeat
CREATE INDEX idx_knn_recheck_geom_nd_gist ON knn_recheck_geom_nd USING gist(geom gist_geometry_ops_nd);
vacuum analyze knn_recheck_geom_nd;
set enable_seqscan = false;
-- point check
SELECT '#1nd-3' As t, gid, ST_3DDistance( 'POINT(-305 998.5 1000)'::geometry, geom)::numeric(12,4) As dist3d,
('POINT(-305 998.5 1000)'::geometry <<->> geom)::numeric(12,4) As dist_knn
FROM knn_recheck_geom_nd
ORDER BY 'POINT(-305 998.5 1000)'::geometry <<->> geom LIMIT 5;

-- linestring check
SELECT '#2nd-3' As t, gid, ST_3DDistance( 'MULTILINESTRING((-95 -300 5000, 105 451 1000, 100 323 200),(-50 2000 456, 30 6000 789))'::geometry::geometry, geom)::numeric(12,4),
 ('MULTILINESTRING((-95 -300 5000, 105 451 1000, 100 323 200),(-50 2000 456, 30 6000 789))'::geometry <<->> geom)::numeric(12,4) As knn_dist
FROM knn_recheck_geom_nd
ORDER BY 'MULTILINESTRING((-95 -300 5000, 105 451 1000, 100 323 200),(-50 2000 456, 30 6000 789))'::geometry <<->> geom LIMIT 5;

-- lateral test
SELECT '#3nd-3' As t, a.gid, b.gid As match, ST_3DDistance(a.geom, b.geom)::numeric(15,4) As true_rn, b.knn_dist::numeric(15,4)
FROM knn_recheck_geom_nd As a
	LEFT JOIN
		LATERAL ( SELECT  gid, geom, a.geom <<->> g.geom As knn_dist
			FROM knn_recheck_geom_nd As g WHERE a.gid <> g.gid ORDER BY a.geom <<->> g.geom LIMIT 5) As b ON true
	WHERE a.gid IN(1,600001)
ORDER BY a.gid, true_rn, b.gid;


DROP TABLE knn_recheck_geom_nd;

-- #3573
SELECT '#3573', 'POINT M (0 0 13)'::geometry <<->> 'LINESTRING M (0 0 5, 0 1 6)'::geometry;

-- #3418
CREATE TABLE test_wo (geo geometry);
INSERT INTO test_wo VALUES
  ('0101000020E61000007D91D0967329E4BF6631B1F9B8D64A40'::geometry),
  ('0101000020E6100000E2AFC91AF510C1BFCDCCCCCCCCAC4A40'::geometry);
CREATE INDEX ON TEST_WO USING GIST (GEO);
analyze test_wo;
SET enable_seqscan = false;
SELECT '#3418' As ticket, ('0101000020E610000092054CE0D6DDE5BFCDCCCCCCCCAC4A40'::geometry <-> geo)::numeric(14,7), ST_Distance('0101000020E610000092054CE0D6DDE5BFCDCCCCCCCCAC4A40'::geometry, geo)::numeric(14,7)
FROM test_wo ORDER BY geo <->
('0101000020E610000092054CE0D6DDE5BFCDCCCCCCCCAC4A40'::geometry);
DROP TABLE test_wo;
set enable_seqscan to default;

-- #5782
CREATE TABLE t5782 ( l text, g geometry );
INSERT INTO t5782 VALUES
	('A', 'LINESTRING(18.00678831099686 69.0404811833497,18.006784630996860 69.04045431334970,18.00677727099686 69.0404005833497)'),
	('B', 'LINESTRING(18.00677727099686 69.0404005833497,18.006780950996863 69.04042744334969,18.00678831099686 69.0404811833497)');
CREATE INDEX ON t5782 USING GIST(g);
ANALYZE t5782;
SET enable_seqscan = false;
SELECT '#5782', l FROM t5782 ORDER BY g <-> 'POINT(18.006691126034692 69.04048768506776)'::geometry;
DROP TABLE t5782;
SET enable_seqscan to default;

--
-- https://trac.osgeo.org/postgis/ticket/6026
--
CREATE TABLE reproduce_6026 (
    id SERIAL PRIMARY KEY,
    geometry GEOMETRY(POLYGON, 28992)
);
INSERT INTO reproduce_6026 (geometry) VALUES
    (ST_GeomFromText('POLYGON ((133894.531 490000, 132700 490000, 132460 490000, 131703.958 489011.71, 131695 489000, 132880 489000, 132880 488416.725, 133206.605 488583.011, 134029.81 489022.36, 133917.695 490000, 133894.531 490000))', 28992)),
    (ST_GeomFromText('POLYGON ((138000 492000, 141276.49 492000, 141050 492550, 140967.11 492751.305, 139951.651 495217.42, 139490.072 495217.42, 136358.892 495217.42, 135900 495217.42, 135301.1 495217.42, 135118.887 495296.573, 135115.873 495264.338, 135106.301 495164.456, 135098.485 495065.802, 135090.476 494967.014, 135085.764 494917.296, 135078.439 494866.459, 135070.381 494817.483, 135060.353 494767.172, 135036.351 494674.319, 135017.779 494615.505, 134991.438 494544.049, 134972.66 494501.055, 134948.148 494446.666, 134922.544 494398.914, 134872.859 494311.273, 134859.929 494292.444, 134843.402 494264.587, 134815.107 494200.454, 134770.855 494120.756, 134728.422 494053.048, 134932.74 493923.77, 133708.3 491825.92, 133917.695 490000, 134029.81 489022.36, 138000 492000))', 28992)),
    (ST_GeomFromText('POLYGON ((131215.201 487105.005, 134029.81 486992.15, 134029.81 489022.36, 133206.605 488583.011, 132880 488416.725, 132198.685 488069.845, 131375.721 487651.916, 131199.012 487560.733, 131198.76 487560.603, 131215.201 487105.005))', 28992)),
    (ST_GeomFromText('POLYGON ((134029.81 489022.36, 136889.1 488581.7, 138000 492000, 134029.81 489022.36))', 28992)),
    (ST_GeomFromText('POLYGON ((133917.695 490000, 133708.3 491825.92, 133541.312 491951.805, 133460 491840, 132820 491000, 132210 490190, 132460 490000, 132700 490000, 133894.531 490000, 133917.695 490000))', 28992));

-- create index
CREATE INDEX reproduce_6026_x
ON reproduce_6026 USING GIST(geometry);
-- force index usage
SET enable_seqscan = OFF;
-- Reproduce error
SELECT '#6026', id
  FROM reproduce_6026
  ORDER BY geometry <-> ST_SetSRID(ST_MakePoint(133718, 489222), 28992)
  LIMIT 4;

DROP TABLE reproduce_6026;

