--- build a larger database
\i regress_lots_of_points.sql

--- test some of the searching capabilities

-- GiST index

CREATE INDEX quick_gist on test using gist (the_geom);

 select num,ST_astext(the_geom) from test where the_geom && 'BOX3D(125 125,135 135)'::box3d order by num;

set enable_seqscan = off;

 select num,ST_astext(the_geom) from test where the_geom && 'BOX3D(125 125,135 135)'::box3d  order by num;

DROP TABLE test;
