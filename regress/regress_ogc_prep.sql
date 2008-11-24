---
--- Tests for GEOS/JTS prepared predicates
---
---

CREATE TABLE ogc_prep (
	c VARCHAR,
	g1 GEOMETRY,
	g2 GEOMETRY
);

-- PIP - point within polygon (no cache)
INSERT INTO ogc_prep (c, g1, g2) 
	VALUES ('099', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', 'POINT(5 5)');
-- PIP - point within polygon
INSERT INTO ogc_prep (c, g1, g2) 
	VALUES ('100', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', 'POINT(5 5)');
-- PIP - point on polygon vertex
INSERT INTO ogc_prep (c, g1, g2) 
	VALUES ('101', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', 'POINT(0 0)');
-- PIP - point outside polygon
INSERT INTO ogc_prep (c, g1, g2) 
	VALUES ('102', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', 'POINT(-1 0)');
-- PIP - point on polygon edge
INSERT INTO ogc_prep (c, g1, g2) 
	VALUES ('103', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', 'POINT(0 5)');
-- PIP - point in line with polygon edge
INSERT INTO ogc_prep (c, g1, g2) 
	VALUES ('104', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', 'POINT(0 12)');
	
SELECT 'intersects' || c, ST_Intersects(g1, g2) FROM ogc_prep ORDER BY c;
SELECT 'contains' || c, ST_Contains(g1, g2) FROM ogc_prep ORDER BY c;
SELECT 'covers' || c, ST_Covers(g1, g2) FROM ogc_prep ORDER BY c;
SELECT 'containsproperly' || c, ST_ContainsProperly(g1, g2) FROM ogc_prep ORDER BY c;

-- PIP - point vertically aligned with polygon vertex
INSERT INTO ogc_prep (c, g1, g2)
	VALUES ('105', ST_GeomFromText('POLYGON((521526 5377783, 521481 5377811, 521494 5377832, 521539 5377804, 521526 5377783))', 32631), ST_GeomFromText('POINT(521513 5377804)', 32631) );
INSERT INTO ogc_prep (c, g1, g2)
	VALUES ('105', ST_GeomFromText('POLYGON((521526 5377783, 521481 5377811, 521494 5377832, 521539 5377804, 521526 5377783))', 32631), ST_GeomFromText('POINT(521513 5377804)', 32631) );
INSERT INTO ogc_prep (c, g1, g2)
	VALUES ('105', ST_GeomFromText('POLYGON((521526 5377783, 521481 5377811, 521494 5377832, 521539 5377804, 521526 5377783))', 32631), ST_GeomFromText('POINT(521513 5377804)', 32631) );
-- PIP - repeated vertex, poly first
INSERT INTO ogc_prep (c, g1, g2)
	VALUES ('106', ST_GeomFromText('POLYGON((521526 5377783, 521482 5377811, 521494 5377832, 521539 5377804, 521526 5377783))', 32631), ST_GeomFromText('POINT(521543 5377804)', 32631) );
INSERT INTO ogc_prep (c, g1, g2)
	VALUES ('106', ST_GeomFromText('POLYGON((521526 5377783, 521482 5377811, 521494 5377832, 521539 5377804, 521526 5377783))', 32631), ST_GeomFromText('POINT(521543 5377804)', 32631) );
INSERT INTO ogc_prep (c, g1, g2)
	VALUES ('106', ST_GeomFromText('POLYGON((521526 5377783, 521482 5377811, 521494 5377832, 521539 5377804, 521526 5377783))', 32631), ST_GeomFromText('POINT(521543 5377804)', 32631) );

-- PIP - point vertically aligned with polygon vertex, poly first
SELECT 'intersectspoly' || c, ST_Intersects(g1, g2) FROM ogc_prep WHERE c = '105';
SELECT 'intersectspoint' || c, ST_Intersects(g2, g1) FROM ogc_prep WHERE c = '105';
SELECT 'containspoly' || c, ST_Contains(g1, g2) FROM ogc_prep WHERE c = '105';
SELECT 'containspoint' || c, ST_Contains(g2, g1) FROM ogc_prep WHERE c = '105';
SELECT 'containsproperlypoly' || c, ST_ContainsProperly(g1, g2) FROM ogc_prep WHERE c = '105';
SELECT 'containsproperlypoint' || c, ST_ContainsProperly(g2, g1) FROM ogc_prep WHERE c = '105';
SELECT 'coverspoly' || c, ST_Covers(g1, g2) FROM ogc_prep WHERE c = '105';
SELECT 'coverspoint' || c, ST_Covers(g2, g1) FROM ogc_prep WHERE c = '105';

SELECT 'intersectspoly' || c, ST_Intersects(g1, g2) FROM ogc_prep WHERE c = '106';
SELECT 'intersectspoint' || c, ST_Intersects(g2, g1) FROM ogc_prep WHERE c = '106';
SELECT 'containspoly' || c, ST_Contains(g1, g2) FROM ogc_prep WHERE c = '106';
SELECT 'containspoint' || c, ST_Contains(g2, g1) FROM ogc_prep WHERE c = '106';
SELECT 'containsproperlypoly' || c, ST_ContainsProperly(g1, g2) FROM ogc_prep WHERE c = '106';
SELECT 'containsproperlypoint' || c, ST_ContainsProperly(g2, g1) FROM ogc_prep WHERE c = '106';
SELECT 'coverspoly' || c, ST_Covers(g1, g2) FROM ogc_prep WHERE c = '106';
SELECT 'coverspoint' || c, ST_Covers(g2, g1) FROM ogc_prep WHERE c = '106';


INSERT INTO ogc_prep (c, g1, g2)
	VALUES ('200', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', 'POLYGON((2 2, 2 3, 3 3, 3 2, 2 2))'); 
INSERT INTO ogc_prep (c, g1, g2)
	VALUES ('201', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', 'POLYGON((2 2, 2 3, 3 3, 3 2, 2 2))'); 
INSERT INTO ogc_prep (c, g1, g2)
	VALUES ('202', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', 'POLYGON((0 0, 2 0, 2 2, 0 2, 0 0))');
INSERT INTO ogc_prep (c, g1, g2)
	VALUES ('203', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', 'POLYGON((-5 -5, 5 -5, 5 5, -5 5, -5 -5))');
INSERT INTO ogc_prep (c, g1, g2)
	VALUES ('204', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', 'POLYGON((-2 -2, -2 -3, -3 -3, -3 -2, -2 -2))');
INSERT INTO ogc_prep (c, g1, g2)
	VALUES ('205', 'POLYGON((0 0, 0 10, 10 11, 10 0, 0 0))', 'POLYGON((2 2, 2 3, 3 3, 3 2, 2 2))'); 
INSERT INTO ogc_prep (c, g1, g2)
	VALUES ('206', 'POLYGON((0 0, 0 10, 10 11, 10 0, 0 0))', 'POLYGON((2 2, 2 3, 3 3, 3 2, 2 2))'); 
INSERT INTO ogc_prep (c, g1, g2)
	VALUES ('207', 'POLYGON((0 0, 0 10, 10 11, 10 0, 0 0))', 'POLYGON((0 0, 2 0, 2 2, 0 2, 0 0))');
INSERT INTO ogc_prep (c, g1, g2)
	VALUES ('208', 'POLYGON((0 0, 0 10, 10 11, 10 0, 0 0))', 'POLYGON((-5 -5, 5 -5, 5 5, -5 5, -5 -5))');
INSERT INTO ogc_prep (c, g1, g2)
	VALUES ('209', 'POLYGON((0 0, 0 10, 10 11, 10 0, 0 0))', 'POLYGON((-2 -2, -2 -3, -3 -3, -3 -2, -2 -2))');

SELECT 'intersects' || c, ST_Intersects(g1, g2) AS intersects_g1g2, ST_Intersects(g2, g1) AS intersects_g2g1 
	FROM ogc_prep WHERE c >= '200' ORDER BY c;
SELECT 'contains' || c, ST_Contains(g1, g2) AS contains_g1g2, ST_Contains(g2, g1) AS contains_g2g1 
	FROM ogc_prep WHERE c >= '200' ORDER BY c;
SELECT 'containsproperly' || c, ST_ContainsProperly(g1, g2) AS containsproperly_g1g2, ST_ContainsProperly(g2, g1) AS containsproperly_g2g1 
	FROM ogc_prep WHERE c >= '200' ORDER BY c;
SELECT 'covers' || c, ST_Covers(g1, g2) AS covers_g1g2, ST_Covers(g2, g1) AS covers_g2g1 
	FROM ogc_prep WHERE c >= '200' ORDER BY c;



-- UNEXPECTED GEOMETRY TYPES --

INSERT INTO ogc_prep (c, g1, g2)
	VALUES ('300', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', 'POINT(5 5)');
INSERT INTO ogc_prep (c, g1, g2)
	VALUES ('301', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', 'POINT(5 5)'); 
INSERT INTO ogc_prep (c, g1, g2)
	VALUES ('302', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', 'POINT(5 5)'); 
INSERT INTO ogc_prep (c, g1, g2)
	VALUES ('303', 'LINESTRING(0 0, 0 10, 10 10, 10 0)', 'POINT(5 5)');
INSERT INTO ogc_prep (c, g1, g2)
	VALUES ('304', 'LINESTRING(0 0, 0 10, 10 10, 10 0)', 'POINT(5 5)'); 
INSERT INTO ogc_prep (c, g1, g2)
	VALUES ('305', 'LINESTRING(0 0, 0 10, 10 10, 10 0)', 'POINT(5 5)'); 
INSERT INTO ogc_prep (c, g1, g2)
	VALUES ('306', 'POINT(5 5)', 'POINT(5 5)');
INSERT INTO ogc_prep (c, g1, g2)
	VALUES ('307', 'POINT(5 5)', 'POINT(5 5)');
INSERT INTO ogc_prep (c, g1, g2)
	VALUES ('308', 'POINT(5 5)', 'POINT(5 5)'); 
INSERT INTO ogc_prep (c, g1, g2)
	VALUES ('309', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');
INSERT INTO ogc_prep (c, g1, g2)
	VALUES ('310', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))'); 
INSERT INTO ogc_prep (c, g1, g2)
	VALUES ('311', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))'); 
INSERT INTO ogc_prep (c, g1, g2)
	VALUES ('312', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', 'LINESTRING(0 0, 0 10, 10 10, 10 0)');
INSERT INTO ogc_prep (c, g1, g2)
	VALUES ('313', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', 'LINESTRING(0 0, 0 10, 10 10, 10 0)'); 
INSERT INTO ogc_prep (c, g1, g2)
	VALUES ('314', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', 'LINESTRING(0 0, 0 10, 10 10, 10 0)');

-- UNEXPECTED GEOMETRY TYPES --

SELECT c, ST_Contains(g1, g2) AS contains_g1g2, ST_Contains(g2, g1) AS contains_g2g1, 
          ST_Covers(g1, g2) AS covers_g1g2, ST_Covers(g2, g1) AS covers_g2g1,
          ST_Intersects(g1, g2) AS intersects_g1g2, ST_Intersects(g2, g1) AS intersects_g2g1,
          ST_ContainsProperly(g1, g2) AS containsproper_g1g2, ST_ContainsProperly(g2, g1) AS containsproper_g2g1 
          FROM ogc_prep WHERE c >= '300' ORDER BY c;

INSERT INTO ogc_prep (c, g1, g2)
	VALUES ( '400', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', 'LINESTRING(1 10, 9 10, 9 8)');
INSERT INTO ogc_prep (c, g1, g2)
	VALUES ( '400', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', 'LINESTRING(1 10, 9 10, 9 8)');
INSERT INTO ogc_prep (c, g1, g2)
	VALUES ( '400', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', 'LINESTRING(1 10, 9 10, 9 8)');
INSERT INTO ogc_prep (c, g1, g2)
	VALUES ( '401', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', 'LINESTRING(1 10, 10 10, 10 8)');
INSERT INTO ogc_prep (c, g1, g2)
	VALUES ( '401', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', 'LINESTRING(1 10, 10 10, 10 8)');
INSERT INTO ogc_prep (c, g1, g2)
	VALUES ( '401', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', 'LINESTRING(1 10, 10 10, 10 8)');
	
SELECT 'intersects' || c, ST_Intersects(g1, g2) FROM ogc_prep WHERE c >= '400' ORDER BY c;
SELECT 'contains' || c, ST_Contains(g1, g2) FROM ogc_prep WHERE c >= '400' ORDER BY c;
SELECT 'containsproperly' || c, ST_ContainsProperly(g1, g2) FROM ogc_prep WHERE c >= '400' ORDER BY c;
SELECT 'covers' || c, ST_Covers(g1, g2) FROM ogc_prep WHERE c >= '400' ORDER BY c;

DROP TABLE ogc_prep;
