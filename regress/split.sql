-- Split line by point of different SRID
select st_splitgeometry('SRID=10;LINESTRING(0 0, 10 0)', 'SRID=5;POINT(5 1)');

-- Split line by point on the line interior
select '1',st_asewkt(st_splitgeometry('SRID=10;LINESTRING(0 0, 10 0)', 'SRID=10;POINT(5 0)'));
select '1.1',st_asewkt(st_splitgeometry('SRID=10;LINESTRING(10 0, 0 0)', 'SRID=10;POINT(5 0)'));

-- Split line by point on the line boundary
select '2',st_asewkt(st_splitgeometry('SRID=10;LINESTRING(0 0, 10 0)', 'SRID=10;POINT(10 0)'));

-- Split line by point on the line exterior
select '3',st_asewkt(st_splitgeometry('SRID=10;LINESTRING(0 0, 10 0)', 'SRID=10;POINT(5 1)'));

-- Split line by line of different SRID
select st_splitgeometry('SRID=10;LINESTRING(0 0, 10 0)', 'SRID=5;LINESTRING(5 1, 10 1)');

-- Split line by disjoint line 
select '4', st_asewkt(ST_SplitGeometry('SRID=10;LINESTRING(0 0, 10 0)', 'SRID=10;LINESTRING(20 0, 20 20)'));

-- Split line by touching line
select '5', st_asewkt(ST_SplitGeometry('SRID=10;LINESTRING(0 0, 10 0)', 'SRID=10;LINESTRING(10 -5, 10 5)'));

-- Split line by crossing line
select '6', st_asewkt(ST_SplitGeometry('SRID=10;LINESTRING(0 0, 10 0)', 'SRID=10;LINESTRING(5 -5, 5 5)'));

-- Split line by multiply-crossing line
select '7', st_asewkt(ST_SplitGeometry('SRID=10;LINESTRING(0 0, 10 0, 10 10, 0 10, 0 20, 10 20)', 'SRID=10;LINESTRING(5 -5, 5 25)'));

-- Split line by overlapping line (1)
select '8.1', st_asewkt(ST_SplitGeometry('SRID=10;LINESTRING(0 0, 10 0)', 'SRID=10;LINESTRING(5 0, 20 0)'));
-- Split line by overlapping line (2)
select '8.2', st_asewkt(ST_SplitGeometry('SRID=10;LINESTRING(0 0, 10 0)', 'SRID=10;LINESTRING(5 0, 8 0)'));
