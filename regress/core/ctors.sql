-- Repeat all tests with the new function names.
-- postgis-users/2006-July/012764.html
SELECT ST_SRID(ST_Collect('SRID=32749;POINT(0 0)', 'SRID=32749;POINT(1 1)'));

SELECT ST_Collect('SRID=32749;POINT(0 0)', 'SRID=32740;POINT(1 1)');

SELECT '#534-1', ST_AsText(ST_AddGeometry('MULTIPOINT((0 0),(1 1))', 'POINT(2 2)'));
SELECT '#534-2', ST_AsText(ST_AddGeometry('MULTIPOLYGON(((0 0,0 1,1 1,0 0)))', 'POLYGON((2 2,2 3,3 3,2 2))'));
SELECT '#534-3', ST_AsText(ST_AddGeometry('MULTIPOINT((0 0),(1 1))', 'LINESTRING(2 2,3 3)'));
SELECT '#534-4', ST_AsEWKT(ST_AddGeometry('SRID=4326;MULTIPOINTZ((0 0 1),(1 1 2))', 'SRID=4326;POINTZ(2 2 3)'));
SELECT '#534-5', ST_AsText(ST_AddGeometry('GEOMETRYCOLLECTION(MULTIPOINT((0 0),(1 1)))', 'POINT(2 2)'));
SELECT '#534-6', ST_AddGeometry('POINT(0 0)', 'POINT(1 1)');
SELECT '#534-7', ST_AddGeometry('SRID=32749;MULTIPOINT((0 0))', 'SRID=32740;POINT(1 1)');
SELECT '#534-8', ST_AddGeometry('MULTIPOINTZ((0 0 1))', 'POINT(1 1)');
SELECT '#534-9', ST_AsText(ST_AddGeometry('COMPOUNDCURVE((0 0,1 1))', 'LINESTRING(1 1,2 2)'));
SELECT '#534-10', ST_AsText(ST_AddGeometry('COMPOUNDCURVE((0 0,1 1))', 'LINESTRING(5 5,6 6)'));
SELECT '#534-11', ST_AsText(ST_AddGeometry('CURVEPOLYGON((0 0,1 0,1 1,0 0))', 'LINESTRING(0.2 0.2,0.3 0.2,0.2 0.3,0.2 0.2)'));
SELECT '#534-12', ST_AsText(ST_AddGeometry('CURVEPOLYGON((0 0,1 0,1 1,0 0))', 'LINESTRING(2 2,3 3)'));
SELECT '#534-13', ST_AsText(ST_AddGeometry('CURVEPOLYGON((0 0,1 0,1 1,0 0))', 'LINESTRING(2 2,3 3,2 2)'));
SELECT '#534-14', ST_AsText(ST_AddGeometry('CURVEPOLYGON((0 0,1 0,1 1,0 0))', 'LINESTRING(2 2,2 2)'));
SELECT '#534-15', ST_AsText(ST_AddGeometry('COMPOUNDCURVE EMPTY', 'LINESTRING EMPTY'));

select ST_asewkt(ST_makeline('SRID=3;POINT(0 0)', 'SRID=3;POINT(1 1)'));
select ST_makeline('POINT(0 0)', 'SRID=3;POINT(1 1)');

select 'ST_MakeLine1', ST_AsText(ST_MakeLine(
 'POINT(0 0)'::geometry,
 'LINESTRING(1 1, 10 0)'::geometry
));

-- postgis-devel/2016-February/025634.html for repeated point background
select 'ST_MakeLine_agg1', ST_AsText(ST_MakeLine(g)) from (
 values ('POINT(0 0)'),
        ('LINESTRING(1 1, 10 0)'),
        ('LINESTRING(10 0, 20 20)'),
        ('POINT(40 4)'),
        ('POINT(40 4)'),
        ('POINT(40 5)'),
        ('MULTIPOINT(40 5, 40 6, 40 6, 40 7)'),
        ('LINESTRING(40 7, 40 8)')
) as foo(g);

select 'ST_MakeLine_agg2', ST_AsText(ST_MakeLine(g)) from (
 values ('POINT(0 0)'),
        ('LINESTRING(0 0, 1 0)'),
        ('POINT(1 0)')
) as foo(g);

-- postgis-users/2006-July/012788.html
select ST_makebox2d('SRID=3;POINT(0 0)', 'SRID=3;POINT(1 1)');
select ST_makebox2d('POINT(0 0)', 'SRID=3;POINT(1 1)');

select ST_3DMakeBox('SRID=3;POINT(0 0)', 'SRID=3;POINT(1 1)');
select ST_3DMakeBox('POINT(0 0)', 'SRID=3;POINT(1 1)');

SELECT 'ST_MakeLine2', ST_AsText(ST_MakeLine(geom))
FROM (
  VALUES
  ('LINESTRING(0 0, 1 1)'),
  ('MULTILINESTRING((1 1, 2 2), (2 2, 3 3))'),
  ('MULTILINESTRING(EMPTY, (4 4, 5 5))')
  ) AS geoms(geom);

SELECT 'ST_MakePolygonLine', ST_AsText(ST_MakePolygon(
  'LINESTRING(0 0, 4 0, 4 4, 0 4, 0 0)'::geometry));

SELECT 'ST_MakePolygonCurveShell', ST_AsText(ST_MakePolygon(
  'CIRCULARSTRING(0 0, 1 1, 2 0, 1 -1, 0 0)'::geometry));

SELECT 'ST_MakePolygonCurveHole', ST_AsText(ST_MakePolygon(
  'LINESTRING(0 0, 4 0, 4 4, 0 4, 0 0)'::geometry,
  ARRAY['CIRCULARSTRING(1 1, 2 2, 3 1, 2 0, 1 1)'::geometry]));

SELECT 'ST_MakePolygonNurbsShell', ST_AsText(ST_MakePolygon(
  'NURBSCURVE(2, (0 0, 5 10, 10 0, 5 -10, 0 0))'::geometry));

SELECT 'ST_MakePolygonNurbsHole', ST_AsText(ST_MakePolygon(
  'LINESTRING(0 0, 4 0, 4 4, 0 4, 0 0)'::geometry,
  ARRAY['NURBSCURVE(2, (1 1, 2 2, 3 1, 2 0, 1 1))'::geometry]));

SELECT ST_MakePolygon('CIRCULARSTRING(0 0, 1 1, 2 0)'::geometry);

SELECT ST_MakePolygon(
  'CIRCULARSTRING Z (0 0 0, 1 1 0, 2 0 0, 1 -1 0, 0 0 0)'::geometry,
  ARRAY['CIRCULARSTRING M (1 1 7, 2 2 7, 3 1 7, 2 0 7, 1 1 7)'::geometry]);

SELECT ST_MakePolygon(
  'CIRCULARSTRING M (0 0 0, 1 1 0, 2 0 0, 1 -1 0, 0 0 0)'::geometry,
  ARRAY['CIRCULARSTRING Z (1 1 7, 2 2 7, 3 1 7, 2 0 7, 1 1 7)'::geometry]);
