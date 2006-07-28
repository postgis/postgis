-- postgis-users/2006-July/012764.html
SELECT SRID(collect('SRID=32749;POINT(0 0)', 'SRID=32749;POINT(1 1)'));

SELECT collect('SRID=32749;POINT(0 0)', 'SRID=32740;POINT(1 1)');

select asewkt(makeline('SRID=3;POINT(0 0)', 'SRID=3;POINT(1 1)'));
select makeline('POINT(0 0)', 'SRID=3;POINT(1 1)');

-- postgis-users/2006-July/012788.html
select makebox2d('SRID=3;POINT(0 0)', 'SRID=3;POINT(1 1)');
select makebox2d('POINT(0 0)', 'SRID=3;POINT(1 1)');

select makebox3d('SRID=3;POINT(0 0)', 'SRID=3;POINT(1 1)');
select makebox3d('POINT(0 0)', 'SRID=3;POINT(1 1)');
