-- Node two crossing lines
select 't1', st_asewkt(st_node(
'SRID=10;MULTILINESTRING((0 0, 10 0),(5 -5, 5 5))'
));

-- Node two overlapping lines
select 't2', st_asewkt(st_node(
'SRID=10;MULTILINESTRING((0 0, 10 0, 20 0),(25 0, 15 0, 8 0))'
));

-- Node a self-intersecting line
-- See http://trac.osgeo.org/geos/ticket/482
select 't3', st_asewkt(st_node(
'SRID=10;LINESTRING(0 0, 10 10, 0 10, 10 0)'
));

-- Node two overlapping 3d lines, from documentation
select 't4', st_asewkt(st_node(
'SRID=10;LINESTRINGZ(0 0 0, 10 10 10, 0 10 5, 10 0 3)'
));
