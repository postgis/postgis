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

-- postgis-users/2006-July/012788.html
select ST_makebox2d('SRID=3;POINT(0 0)', 'SRID=3;POINT(1 1)');
select ST_makebox2d('POINT(0 0)', 'SRID=3;POINT(1 1)');

select ST_3DMakeBox('SRID=3;POINT(0 0)', 'SRID=3;POINT(1 1)');
select ST_3DMakeBox('POINT(0 0)', 'SRID=3;POINT(1 1)');
