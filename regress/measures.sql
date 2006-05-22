
select '113', area2d('MULTIPOLYGON( ((0 0, 10 0, 10 10, 0 10, 0 0)),( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5) ) ,( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7, 5 7, 5 5),(1 1,2 1, 2 2, 1 2, 1 1) ) )'::GEOMETRY) as value;

select '114', perimeter2d('MULTIPOLYGON( ((0 0, 10 0, 10 10, 0 10, 0 0)),( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5) ) ,( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7, 5 7, 5 5),(1 1,2 1, 2 2, 1 2, 1 1) ) )'::GEOMETRY) as value;

select '115', perimeter3d('MULTIPOLYGON( ((0 0 0, 10 0 0, 10 10 0, 0 10 0, 0 0 0)),( (0 0 0, 10 0 0, 10 10 0, 0 10 0, 0 0 0),(5 5 0, 7 5 0, 7 7  0, 5 7 0, 5 5 0) ) ,( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7 1, 5 7 1, 5 5 1),(1 1 1,2 1 1, 2 2 1, 1 2 1, 1 1 1) ) )'::GEOMETRY) as value;


select '116', length2d('MULTILINESTRING((0 0, 1 1),(0 0, 1 1, 2 2) )'::GEOMETRY) as value;
select '117', length3d('MULTILINESTRING((0 0, 1 1),(0 0, 1 1, 2 2) )'::GEOMETRY) as value;
select '118', length3d('MULTILINESTRING((0 0 0, 1 1 1),(0 0 0, 1 1 1, 2 2 2) )'::GEOMETRY) as value;

select '134', distance('POINT(1 2)', 'POINT(1 2)');
select '135', distance('POINT(5 0)', 'POINT(10 12)');

select '136', distance('POINT(0 0)', translate('POINT(0 0)', 5, 12, 0));


-- postgis-users/2006-May/012174.html
select 'dist', distance(a,b), distance(b,a) from (
	select 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))'::geometry as a,
		'POLYGON((11 0, 11 10, 20 10, 20 0, 11 0),
			(15 5, 15 8, 17 8, 17 5, 15 5))'::geometry as b
	) as foo;

