-- Point on line
select '1',st_asewkt(st_splitgeometry('SRID=10;LINESTRING(0 0, 10 0)', 'POINT(5 0)'));

select '1.1',st_asewkt(st_splitgeometry('SRID=10;LINESTRING(10 0, 0 0)', 'POINT(5 0)'));

-- Point on line boundary
select '2',st_asewkt(st_splitgeometry('SRID=10;LINESTRING(0 0, 10 0)', 'POINT(10 0)'));

-- Point off line
select '3',st_asewkt(st_splitgeometry('SRID=10;LINESTRING(0 0, 10 0)', 'POINT(5 1)'));

