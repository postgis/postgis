---
--- Tests for GEOS/JTS prepared predicates
---
---

SELECT 'intersects', ST_intersectsPrepared('LINESTRING(0 10, 0 -10)', p) from ( values 
('LINESTRING(0 0, 1 1)'),('LINESTRING(0 0, 1 1)'),('LINESTRING(0 0, 1 1)')
) as v(p);
-- PIP - point within polygon
SELECT 'intersects100', ST_intersectsPrepared('POINT(5 5)', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');
-- PIP - point on polygon vertex
SELECT 'intersects101', ST_intersectsPrepared('POINT(0 0)', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');
-- PIP - point outside polygon
SELECT 'intersects102', ST_intersectsPrepared('POINT(-1 0)', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');
-- PIP - point on polygon edge
SELECT 'intersects103', ST_intersectsPrepared('POINT(0 5)', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');
-- PIP - point in line with polygon edge
SELECT 'intersects104', ST_intersectsPrepared('POINT(0 12)', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');
-- PIP - point vertically aligned with polygon vertex 
SELECT 'intersects105', ST_intersectsPrepared(ST_GeomFromText('POINT(521513 5377804)', 32631), ST_GeomFromText('POLYGON((521526 5377783, 521481 5377811, 521494 5377832, 521539 5377804, 521526 5377783))', 32631));
-- PIP - repeated vertex
SELECT 'intersects106', ST_intersectsPrepared(ST_GeomFromText('POINT(521543 5377804)', 32631), ST_GeomFromText('POLYGON((521526 5377783, 521482 5377811, 521494 5377832, 521539 5377804, 521526 5377783))', 32631));
-- PIP - point within polygon
SELECT 'intersects150', ST_intersectsPrepared('POINT(5 5)', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');
-- PIP - point on polygon vertex
SELECT 'intersects151', ST_intersectsPrepared('POINT(0 0)', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');
-- PIP - point outside polygon
SELECT 'intersects152', ST_intersectsPrepared('POINT(-1 0)', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');
-- PIP - point on polygon edge
SELECT 'intersects153', ST_intersectsPrepared('POINT(0 5)', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');
-- PIP - point in line with polygon edge
SELECT 'intersects154', ST_intersectsPrepared('POINT(0 12)', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');
-- PIP - point vertically aligned with polygon vertex 
SELECT 'intersects155', ST_intersectsPrepared(ST_GeomFromText('POINT(521513 5377804)', 32631), ST_GeomFromText('POLYGON((521526 5377783, 521481 5377811, 521494 5377832, 521539 5377804, 521526 5377783))', 32631));
-- PIP - repeated vertex
SELECT 'intersects156', ST_intersectsPrepared(ST_GeomFromText('POINT(521543 5377804)', 32631), ST_GeomFromText('POLYGON((521526 5377783, 521482 5377811, 521494 5377832, 521539 5377804, 521526 5377783))', 32631));

SELECT 'intersects200', ST_intersectsPrepared('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', p) from ( values 
('POINT(5 5)'),('POINT(5 5)'),('POINT(5 5)') 
) as v(p);
-- PIP - point on vertex of polygon
SELECT 'intersects201', ST_intersectsPrepared('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', p) from ( values 
('POINT(0 0)'),('POINT(0 0)'),('POINT(0 0)')
) as v(p);
-- PIP - point outside polygon
SELECT 'intersects202', ST_intersectsPrepared('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', p) from ( values 
('POINT(-1 0)'),('POINT(-1 0)'),('POINT(-1 0)')
) as v(p);
-- PIP - point on edge of polygon
SELECT 'intersects203', ST_intersectsPrepared('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', p) from ( values 
('POINT(0 5)'),('POINT(0 5)'),('POINT(0 5)')
) as v(p);
-- PIP - point in line with polygon edge
SELECT 'intersects204', ST_intersectsPrepared('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', p) from ( values 
('POINT(0 12)'),('POINT(0 12)'),('POINT(0 12)')
) as v(p);
-- PIP - point vertically aligned with polygon vertex 
SELECT 'intersects205', ST_intersectsPrepared(ST_GeomFromText('POLYGON((521526 5377783, 521481 5377811, 521494 5377832, 521539 5377804, 521526 5377783))', 32631), p) from ( values 
(ST_GeomFromText('POINT(521513 5377804)', 32631)),(ST_GeomFromText('POINT(521513 5377804)', 32631)),(ST_GeomFromText('POINT(521513 5377804)', 32631))
) as v(p);
-- PIP - repeated vertex 
SELECT 'intersects206', ST_intersectsPrepared(ST_GeomFromText('POLYGON((521526 5377783, 521482 5377811, 521481 5377811, 521494 5377832, 521539 5377804, 521526 5377783))', 32631), p) from ( values 
(ST_GeomFromText('POINT(521513 5377804)', 32631)),(ST_GeomFromText('POINT(521513 5377804)', 32631)),(ST_GeomFromText('POINT(521513 5377804)', 32631))
) as v(p);
SELECT 'intersects210', ST_intersectsPrepared('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', p) from ( values 
('LINESTRING(1 10, 9 10, 9 8)'),('LINESTRING(1 10, 9 10, 9 8)'),('LINESTRING(1 10, 9 10, 9 8)')
) as v(p);
SELECT 'intersects211', ST_intersectsPrepared('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', p) from ( values 
('LINESTRING(1 10, 10 10, 10 8)'),('LINESTRING(1 10, 10 10, 10 8)'),('LINESTRING(1 10, 10 10, 10 8)')
) as v(p);


-- PIP - point within polygon
SELECT 'contains100', ST_ContainsPrepared('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', p) from ( values 
('POINT(5 5)'),('POINT(5 5)'),('POINT(5 5)')
) as v(p);
-- PIP - point on vertex of polygon
SELECT 'contains101', ST_ContainsPrepared('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', p) from ( values 
('POINT(0 0)'),('POINT(0 0)'),('POINT(0 0)')
) as v(p);
-- PIP - point outside polygon
SELECT 'contains102', ST_ContainsPrepared('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', p) from ( values 
('POINT(-1 0)'),('POINT(-1 0)'),('POINT(-1 0)')
) as v(p);
-- PIP - point on edge of polygon
SELECT 'contains103', ST_ContainsPrepared('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', p) from ( values 
('POINT(0 5)'),('POINT(0 5)'),('POINT(0 5)')
) as v(p);
-- PIP - point in line with polygon edge
SELECT 'contains104', ST_ContainsPrepared('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', p) from ( values 
('POINT(0 12)'),('POINT(0 12)'),('POINT(0 12)')
) as v(p);
-- PIP - point vertically aligned with polygon vertex 
SELECT 'contains105', ST_ContainsPrepared(ST_GeomFromText('POLYGON((521526 5377783, 521481 5377811, 521494 5377832, 521539 5377804, 521526 5377783))', 32631), p) from ( values 
(ST_GeomFromText('POINT(521513 5377804)', 32631)),(ST_GeomFromText('POINT(521513 5377804)', 32631)),(ST_GeomFromText('POINT(521513 5377804)', 32631))
) as v(p);
-- PIP - repeated vertex 
SELECT 'contains106', ST_ContainsPrepared(ST_GeomFromText('POLYGON((521526 5377783, 521482 5377811, 521481 5377811, 521494 5377832, 521539 5377804, 521526 5377783))', 32631), p) from ( values 
(ST_GeomFromText('POINT(521513 5377804)', 32631)),(ST_GeomFromText('POINT(521513 5377804)', 32631)),(ST_GeomFromText('POINT(521513 5377804)', 32631))
) as v(p);
SELECT 'contains110', ST_ContainsPrepared('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', p) from ( values 
('LINESTRING(1 10, 9 10, 9 8)'),('LINESTRING(1 10, 9 10, 9 8)'),('LINESTRING(1 10, 9 10, 9 8)')
) as v(p);
SELECT 'contains111', ST_ContainsPrepared('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', p) from ( values 
('LINESTRING(1 10, 10 10, 10 8)'),('LINESTRING(1 10, 10 10, 10 8)'),('LINESTRING(1 10, 10 10, 10 8)')
) as v(p);

-- PIP - point within polygon
SELECT 'containsproperly100', ST_ContainsProperlyPrepared('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', p) from ( values 
('POINT(5 5)'),('POINT(5 5)'),('POINT(5 5)')
) as v(p);
-- PIP - point on vertex of polygon
SELECT 'containsproperly101', ST_ContainsProperlyPrepared('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', p) from ( values 
('POINT(0 0)'),('POINT(0 0)'),('POINT(0 0)')
) as v(p);
-- PIP - point outside polygon
SELECT 'containsproperly102', ST_ContainsProperlyPrepared('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', p) from ( values 
('POINT(-1 0)'),('POINT(-1 0)'),('POINT(-1 0)')
) as v(p);
-- PIP - point on edge of polygon
SELECT 'containsproperly103', ST_ContainsProperlyPrepared('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', p) from ( values 
('POINT(0 5)'),('POINT(0 5)'),('POINT(0 5)')
) as v(p);
-- PIP - point in line with polygon edge
SELECT 'containsproperly104', ST_ContainsProperlyPrepared('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', p) from ( values 
('POINT(0 12)'),('POINT(0 12)'),('POINT(0 12)')
) as v(p);
-- PIP - point vertically aligned with polygon vertex 
SELECT 'containsproperly105', ST_ContainsProperlyPrepared(ST_GeomFromText('POLYGON((521526 5377783, 521481 5377811, 521494 5377832, 521539 5377804, 521526 5377783))', 32631), p) from ( values 
(ST_GeomFromText('POINT(521513 5377804)', 32631)),(ST_GeomFromText('POINT(521513 5377804)', 32631)),(ST_GeomFromText('POINT(521513 5377804)', 32631))
) as v(p);
-- PIP - repeated vertex 
SELECT 'containsproperly106', ST_ContainsProperlyPrepared(ST_GeomFromText('POLYGON((521526 5377783, 521482 5377811, 521481 5377811, 521494 5377832, 521539 5377804, 521526 5377783))', 32631), p) from ( values 
(ST_GeomFromText('POINT(521513 5377804)', 32631)),(ST_GeomFromText('POINT(521513 5377804)', 32631)),(ST_GeomFromText('POINT(521513 5377804)', 32631))
) as v(p);
SELECT 'containsproperly110', ST_ContainsProperlyPrepared('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', p) from ( values 
('LINESTRING(1 10, 9 10, 9 8)'),('LINESTRING(1 10, 9 10, 9 8)'),('LINESTRING(1 10, 9 10, 9 8)')
) as v(p);
SELECT 'containsproperly111', ST_ContainsProperlyPrepared('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', p) from ( values 
('LINESTRING(1 10, 10 10, 10 8)'),('LINESTRING(1 10, 10 10, 10 8)'),('LINESTRING(1 10, 10 10, 10 8)')
) as v(p);

-- Covers cases
SELECT 'covers100', ST_CoversPrepared('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', p) from ( values 
('LINESTRING(1 10, 9 10, 9 8)'),('LINESTRING(1 10, 9 10, 9 8)'),('LINESTRING(1 10, 9 10, 9 8)')
) as v(p);
SELECT 'covers101', ST_CoversPrepared('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', p) from ( values 
('LINESTRING(1 10, 10 10, 10 8)'),('LINESTRING(1 10, 10 10, 10 8)'),('LINESTRING(1 10, 10 10, 10 8)')
) as v(p);
-- PIP - point within polygon
SELECT 'covers102', ST_CoversPrepared('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', p) from ( values 
('POINT(5 5)'),('POINT(5 5)'),('POINT(5 5)')
) as v(p);
-- PIP - point on vertex of polygon
SELECT 'covers103', ST_CoversPrepared('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', p) from ( values 
('POINT(0 0)'),('POINT(0 0)'),('POINT(0 0)')
) as v(p);
-- PIP - point outside polygon
SELECT 'covers104', ST_CoversPrepared('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', p) from ( values 
('POINT(-1 0)'),('POINT(-1 0)'),('POINT(-1 0)')
) as v(p);
-- PIP - point on edge of polygon
SELECT 'covers105', ST_CoversPrepared('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', p) from ( values 
('POINT(0 5)'),('POINT(0 5)'),('POINT(0 5)')
) as v(p);
-- PIP - point in line with polygon edge
SELECT 'covers106', ST_CoversPrepared('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))', p) from ( values 
('POINT(0 12)'),('POINT(0 12)'),('POINT(0 12)')
) as v(p);
-- PIP - point vertically aligned with polygon vertex 
SELECT 'covers107', ST_CoversPrepared(ST_GeomFromText('POLYGON((521526 5377783, 521481 5377811, 521494 5377832, 521539 5377804, 521526 5377783))', 32631), p) from ( values 
(ST_GeomFromText('POINT(521513 5377804)', 32631)),(ST_GeomFromText('POINT(521513 5377804)', 32631)),(ST_GeomFromText('POINT(521513 5377804)', 32631))
) as v(p);
-- PIP - repeated vertex 
SELECT 'covers108', ST_CoversPrepared(ST_GeomFromText('POLYGON((521526 5377783, 521482 5377811, 521481 5377811, 521494 5377832, 521539 5377804, 521526 5377783))', 32631), p) from ( values 
(ST_GeomFromText('POINT(521513 5377804)', 32631)),(ST_GeomFromText('POINT(521513 5377804)', 32631)),(ST_GeomFromText('POINT(521513 5377804)', 32631))
) as v(p);
