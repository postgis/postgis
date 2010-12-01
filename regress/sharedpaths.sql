-- SRIDs are checked!
select 't1', st_asewkt(st_sharedpaths(
  'SRID=10;LINESTRING(0 0, 10 0)', 'SRID=5;LINESTRING(0 0, 100 0)'
));

-- SRIDs are retained
select 't2', st_asewkt(st_sharedpaths(
  'SRID=10;LINESTRING(0 0, 10 0)', 'SRID=10;LINESTRING(0 0, 10 0)'
));

-- Opposite direction
select 't3', st_asewkt(st_sharedpaths(
  'LINESTRING(0 0, 10 0)', 'LINESTRING(10 0, 0 0)'
));

-- Disjoint
select 't4', st_asewkt(st_sharedpaths(
  'LINESTRING(0 0, 10 0)', 'LINESTRING(20 0, 30 0)'
));

-- Mixed
select 't5', st_asewkt(st_sharedpaths(
  'LINESTRING(0 0, 100 0)', 'LINESTRING(20 0, 30 0, 30 50, 80 0, 70 0)'
));

