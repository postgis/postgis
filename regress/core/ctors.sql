-- Repeat all tests with the new function names.
-- postgis-users/2006-July/012764.html
SELECT ST_SRID(ST_Collect('SRID=32749;POINT(0 0)', 'SRID=32749;POINT(1 1)'));

SELECT ST_Collect('SRID=32749;POINT(0 0)', 'SRID=32740;POINT(1 1)');

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

SELECT 'ST_MakeCurveLine1', ST_AsText(ST_MakeCurveLine(
  ST_MakePoint(1, 2), ST_MakePoint(3, 4), ST_MakePoint(5, 6)));

SELECT 'ST_MakeCurveLine2', ST_AsText(ST_MakeCurveLine(ARRAY[
  ST_MakePoint(1, 2), ST_MakePoint(3, 4), ST_MakePoint(5, 6),
  ST_MakePoint(7, 8), ST_MakePoint(9, 10)]));

SELECT 'ST_MakeCurveLineZM', ST_AsEWKT(ST_MakeCurveLine(ARRAY[
  'POINT ZM (0 0 1 5)'::geometry,
  'POINT ZM (1 1 2 6)'::geometry,
  'POINT ZM (2 0 3 7)'::geometry]));

SELECT 'ST_MakeCurveLineNullArray', ST_MakeCurveLine(ARRAY[NULL]::geometry[]) IS NULL;
SELECT ST_MakeCurveLine(ARRAY['POINT(0 0)'::geometry, 'POINT(1 1)'::geometry]);
SELECT ST_MakeCurveLine(ARRAY[
  'POINT(0 0)'::geometry, 'POINT(1 1)'::geometry,
  'POINT(2 0)'::geometry, 'POINT(3 1)'::geometry]);
SELECT ST_MakeCurveLine(ARRAY['POINT(0 0)'::geometry, 'LINESTRING(1 1, 2 2)'::geometry, 'POINT(3 3)'::geometry]);
SELECT ST_MakeCurveLine(ARRAY['POINT(0 0)'::geometry, 'SRID=3;POINT(1 1)'::geometry, 'POINT(2 0)'::geometry]);

SELECT 'ST_MakeCompoundCurve1', ST_AsText(ST_MakeCompoundCurve(ARRAY[
  'CIRCULARSTRING(0 0,2 0,2 1,2 3,4 3)'::geometry,
  'LINESTRING(4 3,4 5,1 4,0 0)'::geometry]));

SELECT 'ST_MakeCompoundCurveNurbs', ST_AsText(ST_MakeCompoundCurve(ARRAY[
  'LINESTRING(0 0,1 1)'::geometry,
  'NURBSCURVE(2, (1 1, 2 3, 4 1))'::geometry]));

SELECT 'ST_MakeCompoundCurveEmptyArray', ST_MakeCompoundCurve(ARRAY[]::geometry[]) IS NULL;
SELECT ST_MakeCompoundCurve(ARRAY[
  'LINESTRING(0 0,1 1)'::geometry,
  'LINESTRING(2 2,3 3)'::geometry]);
SELECT ST_MakeCompoundCurve(ARRAY[
  'LINESTRING Z (0 0 0,1 1 0)'::geometry,
  'LINESTRING M (1 1 0,2 2 0)'::geometry]);
SELECT ST_MakeCompoundCurve(ARRAY[
  'LINESTRING Z (0 0 0,1 1 0)'::geometry,
  'LINESTRING Z (1 1 1,2 2 0)'::geometry]);
SELECT ST_MakeCompoundCurve(ARRAY[
  'LINESTRING(0 0,1 1)'::geometry,
  'SRID=3;LINESTRING(1 1,2 2)'::geometry]);
SELECT ST_MakeCompoundCurve(ARRAY['POINT(0 0)'::geometry]);
SELECT ST_MakeCompoundCurve(ARRAY[
  'COMPOUNDCURVE((0 0,1 1),(1 1,2 2))'::geometry]);

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
