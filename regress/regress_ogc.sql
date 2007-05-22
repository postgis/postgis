---
--- Tests for GEOS/JTS implemented functions
---
---

-- Ouput is snapped to grid to account for small floating numbers
-- differences between architectures
SELECT 'buffer', astext(SnapToGrid(buffer('POINT(0 0)', 1, 2), 1.0e-6));

SELECT 'geomunion', astext(geomunion('POINT(0 0)', 'POINT(1 1)'));
SELECT 'unite_garray', equals(unite_garray(geom_accum('{POINT(0 0)}', 'POINT(2 3)')), 'MULTIPOINT(2 3,0 0)');
SELECT 'convexhull', asewkt(convexhull('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0),(2 2, 2 4, 4 4, 4 2, 2 2))'));
SELECT 'relate', relate('POINT(0 0)', 'LINESTRING(0 0, 1 1)');
SELECT 'relate', relate('POINT(0 0)', 'LINESTRING(0 0, 1 1)', 'F0FFFF*02');
SELECT 'relate', relate('POINT(0 0)', 'LINESTRING(0 0, 1 1)', 'F0FFF0*02');
SELECT 'disjoint', disjoint('POINT(0 0)', 'LINESTRING(0 0, 1 1)');
SELECT 'touches', touches('LINESTRING(0 10, 0 -10)', 'LINESTRING(0 0, 1 1)');
SELECT 'intersects', intersects('LINESTRING(0 10, 0 -10)', 'LINESTRING(0 0, 1 1)');
SELECT 'crosses', crosses('LINESTRING(0 10, 0 -10)', 'LINESTRING(0 0, 1 1)');
SELECT 'crosses', crosses('LINESTRING(0 10, 0 -10)', 'LINESTRING(-4 0, 1 1)');
SELECT 'within100', within('POINT(5 5)', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');
SELECT 'within101', within('POINT(0 0)', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');
SELECT 'within102', within('POINT(-1 0)', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');
SELECT 'within103', within('POINT(0 5)', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');
SELECT 'within104', within(GeomFromText('POINT(521513 5377804)', 32631), GeomFromText('POLYGON((521526 5377783, 521481 5377811, 521494 5377832, 521539 5377804, 521526 5377783))', 32631));
SELECT 'within105', within(GeomFromText('POINT(521513 5377804)', 32631), GeomFromText('POLYGON((521526 5377783, 521482 5377811, 521481 5377811, 521494 5377832, 521539 5377804, 521526 5377783))', 32631));
SELECT 'disjoint100', disjoint('POINT(5 5)', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');
SELECT 'disjoint101', disjoint('POINT(0 0)', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');
SELECT 'disjoint102', disjoint('POINT(-1 0)', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');
SELECT 'disjoint103', disjoint('POINT(0 5)', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');
SELECT 'disjoint104', disjoint(GeomFromText('POINT(521513 5377804)', 32631), GeomFromText('POLYGON((521526 5377783, 521481 5377811, 521494 5377832, 521539 5377804, 521526 5377783))', 32631));
SELECT 'disjoint105', disjoint(GeomFromText('POINT(521543 5377804)', 32631), GeomFromText('POLYGON((521526 5377783, 521482 5377811, 521494 5377832, 521539 5377804, 521526 5377783))', 32631));
SELECT 'disjoint106', disjoint('POINT(5 5)', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');
SELECT 'disjoint107', disjoint('POINT(0 0)', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');
SELECT 'disjoint108', disjoint('POINT(-1 0)', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');
SELECT 'disjoint109', disjoint('POINT(0 5)', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');
SELECT 'disjoint110', disjoint(GeomFromText('POINT(521513 5377804)', 32631), GeomFromText('POLYGON((521526 5377783, 521481 5377811, 521494 5377832, 521539 5377804, 521526 5377783))', 32631));
SELECT 'disjoint111', disjoint(GeomFromText('POINT(521543 5377804)', 32631), GeomFromText('POLYGON((521526 5377783, 521482 5377811, 521494 5377832, 521539 5377804, 521526 5377783))', 32631));
SELECT 'intersects100', intersects('POINT(5 5)', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');
SELECT 'intersects101', intersects('POINT(0 0)', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');
SELECT 'intersects102', intersects('POINT(-1 0)', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');
SELECT 'intersects103', intersects('POINT(0 5)', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');
SELECT 'intersects104', intersects(GeomFromText('POINT(521513 5377804)', 32631), GeomFromText('POLYGON((521526 5377783, 521481 5377811, 521494 5377832, 521539 5377804, 521526 5377783))', 32631));
SELECT 'intersects105', intersects(GeomFromText('POINT(521543 5377804)', 32631), GeomFromText('POLYGON((521526 5377783, 521482 5377811, 521494 5377832, 521539 5377804, 521526 5377783))', 32631));
SELECT 'intersects106', intersects('POINT(5 5)', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');
SELECT 'intersects107', intersects('POINT(0 0)', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');
SELECT 'intersects108', intersects('POINT(-1 0)', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');
SELECT 'intersects109', intersects('POINT(0 5)', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');
SELECT 'intersects110', intersects(GeomFromText('POINT(521513 5377804)', 32631), GeomFromText('POLYGON((521526 5377783, 521481 5377811, 521494 5377832, 521539 5377804, 521526 5377783))', 32631));
SELECT 'intersects111', intersects(GeomFromText('POINT(521543 5377804)', 32631), GeomFromText('POLYGON((521526 5377783, 521482 5377811, 521494 5377832, 521539 5377804, 521526 5377783))', 32631));
-- moved here from regress.sql
select 'within119', within('LINESTRING(-1 -1, -1 101, 101 101, 101 -1)'::GEOMETRY,'BOX3D(0 0, 100 100)'::BOX3D);
select 'within120', within('LINESTRING(-1 -1, -1 100, 101 100, 101 -1)'::GEOMETRY,'BOX3D(0 0, 100 100)'::BOX3D);
SELECT 'contains100', contains('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))','POINT(-1 0)');
SELECT 'contains101', contains('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))','POINT(0 0)');
SELECT 'contains102', contains('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))','POINT(5 5)');
SELECT 'overlaps', overlaps('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))','POINT(5 5)');
SELECT 'isvalid', isvalid('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');
SELECT 'isvalid', isvalid('POLYGON((0 0, 0 10, 10 10, -5 10, 10 0, 0 0))');
SELECT 'isvalid', isvalid('GEOMETRYCOLLECTION EMPTY');
SELECT 'intersection', astext(intersection('LINESTRING(0 10, 0 -10)', 'LINESTRING(0 0, 1 1)'));
SELECT 'difference', astext(difference('LINESTRING(0 10, 0 -10)'::GEOMETRY, 'LINESTRING(0 2, 0 -2)'::GEOMETRY));
SELECT 'boundary', astext(boundary('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0),(2 2, 2 4, 4 4, 4 2, 2 2))'));
SELECT 'symdifference', astext(symdifference('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0),(2 2, 2 4, 4 4, 4 2, 2 2))', 'LINESTRING(0 0, 20 20)'));
SELECT 'issimple', issimple('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0),(2 2, 2 4, 4 4, 4 2, 2 2))');
SELECT 'equals', equals('LINESTRING(0 0, 1 1)', 'LINESTRING(1 1, 0 0)');
SELECT 'pointonsurface', astext(pointonsurface('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0),(2 2, 2 4, 4 4, 4 2, 2 2))'));
SELECT 'centroid', astext(centroid('POLYGON((0 0, 0 10, 10 10, 10 0, 0 0),(2 2, 2 4, 4 4, 4 2, 2 2))'));
SELECT 'exteriorring', astext(exteriorring(PolygonFromText('POLYGON((52 18,66 23,73 9,48 6,52 18),(59 18,67 18,67 13,59 13,59 18))')));
SELECT 'polygonize_garray', astext(polygonize_garray('{0102000000020000000000000000000000000000000000000000000000000024400000000000000000:0102000000020000000000000000002440000000000000000000000000000000000000000000000000:0102000000020000000000000000002440000000000000244000000000000000000000000000000000:0102000000020000000000000000002440000000000000244000000000000024400000000000000000:0102000000020000000000000000002440000000000000244000000000000000000000000000002440:0102000000020000000000000000000000000000000000244000000000000000000000000000002440:0102000000020000000000000000000000000000000000244000000000000024400000000000002440:0102000000020000000000000000000000000000000000244000000000000000000000000000000000:0102000000020000000000000000000000000000000000244000000000000024400000000000000000}'));

SELECT 'polygonize_garray', astext(geometryn(polygonize_garray('{LINESTRING(0 0, 10 0):LINESTRING(10 0, 10 10):LINESTRING(10 10, 0 10):LINESTRING(0 10, 0 0)}'), 1));

select 'linemerge149', asewkt(linemerge('GEOMETRYCOLLECTION(LINESTRING(0 0, 1 1), LINESTRING(4 4, 1 1), LINESTRING(-5 -5, 0 0))'::geometry));

--- postgis-devel/2005-December/001784.html
select 'intersects', intersects(
   polygonfromtext('POLYGON((0.0 0.0,1.0 0.0,1.0 1.0,1.0 0.0,0.0 0.0))'),
      polygonfromtext('POLYGON((0.0 2.0,1.0 2.0,1.0 3.0,0.0 3.0,0.0 2.0))')
      );

select '130', geosnoop('POLYGON((0 0, 1 1, 0 0))');

