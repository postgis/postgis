-- Repeat all tests with the new function names.
-- postgis-users/2006-July/012764.html
SELECT ST_SRID(collect('SRID=32749;POINT(0 0)', 'SRID=32749;POINT(1 1)'));

SELECT ST_collect('SRID=32749;POINT(0 0)', 'SRID=32740;POINT(1 1)');

select ST_asewkt(makeline('SRID=3;POINT(0 0)', 'SRID=3;POINT(1 1)'));
select ST_makeline('POINT(0 0)', 'SRID=3;POINT(1 1)');

-- postgis-users/2006-July/012788.html
select ST_makebox2d('SRID=3;POINT(0 0)', 'SRID=3;POINT(1 1)');
select ST_makebox2d('POINT(0 0)', 'SRID=3;POINT(1 1)');

select ST_makebox3d('SRID=3;POINT(0 0)', 'SRID=3;POINT(1 1)');
select ST_makebox3d('POINT(0 0)', 'SRID=3;POINT(1 1)');
