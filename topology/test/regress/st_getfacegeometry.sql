set client_min_messages to WARNING;

SELECT topology.CreateTopology('tt') > 0;

COPY tt.face(face_id, mbr) FROM STDIN;
1	POLYGON((0 0,0 10,10 10,10 0,0 0))
2	POLYGON((2 2,2 8,8 8,8 2,2 2))
3	POLYGON((12 2,12 8,18 8,18 2,12 2))
5	POLYGON((20 0,20 10,30 10,30 0,20 0))
\.

COPY tt.node(node_id, geom) FROM STDIN;
1	POINT(2 2)
2	POINT(0 0)
3	POINT(12 2)
4	POINT(20 0)
5	POINT(20 10)
6	POINT(30 10)
7	POINT(30 0)
8	POINT(10 10)
9	POINT(10 0)
\.

COPY tt.edge_data(
	edge_id, start_node, end_node,
	abs_next_left_edge, abs_next_right_edge,
	next_left_edge, next_right_edge,
        left_face, right_face, geom) FROM STDIN;
1	1	1	1	1	1	-1	1	2	LINESTRING(2 2, 2  8,  8  8,  8 2, 2 2)
2	2	2	2	2	2	-2	0	1	LINESTRING(0 0, 0 10, 10 10, 10 0, 0 0)
3	3	3	3	3	3	-3	0	3	LINESTRING(12 2, 12 8, 18 8, 18 2, 12 2)
5	4	5	8	11	8	-11	5	5	LINESTRING(20 0, 20 10)
6	7	4	5	7	5	-7	5	0	LINESTRING(30 0, 20 0)
7	6	7	6	8	6	-8	5	0	LINESTRING(30 10, 30 0)
8	5	6	7	5	7	-5	5	0	LINESTRING(20 10, 30 10)
9	5	8	10	5	10	-5	0	0	LINESTRING(20 10, 10 10)
10	8	9	11	9	11	-9	0	0	LINESTRING(10 10, 10 0)
11	9	4	9	10	9	-10	0	0	LINESTRING(10 0, 20 0)
\.

-- F1 should have an hole !
-- See http://trac.osgeo.org/postgis/ticket/726
SELECT 'f1 (with hole)', ST_asText(topology.st_getfacegeometry('tt', 1));
SELECT 'f2 (fill hole)', ST_asText(topology.st_getfacegeometry('tt', 2));
SELECT 'f5 (bad labels)', ST_asText(topology.st_getfacegeometry('tt', 5));

-- Universal face has no geometry
-- See http://trac.osgeo.org/postgis/ticket/973
SELECT topology.st_getfacegeometry('tt', 0);

-- Null arguments
SELECT topology.st_getfacegeometry(null, 1);
SELECT topology.st_getfacegeometry('tt', null);

-- Invalid topology names
SELECT topology.st_getfacegeometry('NonExistent', 1);
SELECT topology.st_getfacegeometry('', 1);

-- Non-existent face
SELECT topology.st_getfacegeometry('tt', 666);

-- Face with partial rings
-- See https://trac.osgeo.org/postgis/ticket/4681
COPY tt.face(face_id, mbr) FROM STDIN;
4	POLYGON((0 0,2 2,2 2,0 0))
\.
COPY tt.edge_data(
	edge_id, start_node, end_node,
	abs_next_left_edge, abs_next_right_edge,
	next_left_edge, next_right_edge,
        left_face, right_face, geom) FROM STDIN;
4	2	1	1	2	1	-2	4	4	LINESTRING(0 0, 2 2)
\.
SET client_min_messages TO NOTICE;
SELECT ST_AsText(topology.st_getfacegeometry('tt', 4));
SET client_min_messages TO WARNING;

SELECT topology.DropTopology('tt');
