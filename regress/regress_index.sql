--- build a larger database
\set ECHO none
\i regress_lots_of_points.sql
\set ECHO all

--- test some of the searching capabilities

-- GiST index

CREATE INDEX quick_gist on test using gist (the_geom gist_geometry_ops) with (islossy);

 select * from test where the_geom && 'BOX3D(125 125,135 135)'::box3d order by num;

set enable_seqscan = off;

 select * from test where the_geom && 'BOX3D(125 125,135 135)'::box3d  order by num;


--- RTree (not recommended)

CREATE INDEX quick_rt on test using rtree (the_geom rt_geometry_ops);
set enable_seqscan = on;
 select * from test where the_geom && 'BOX3D(125 125,135 135)'::box3d order by num;
set enable_seqscan = off;
 select * from test where the_geom && 'BOX3D(125 125,135 135)'::box3d  order by num;


