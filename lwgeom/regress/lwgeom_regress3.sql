--- build a larger database
\set ECHO none
\i regress_lots_of_points.sql
\set ECHO all

--- test some of the searching capabilities

-- GiST index

CREATE INDEX quick_gist on test using gist (lwgeom gist_lwgeom_ops);
vacuum analyse test;

 select num from test where lwgeom && lwgeom(envelope('BOX3D(125 125,135 135)'::box3d)) order by num;

set enable_seqscan = off;

 select num from test where lwgeom && lwgeom(envelope('BOX3D(125 125,135 135)'::box3d)) order by num;


