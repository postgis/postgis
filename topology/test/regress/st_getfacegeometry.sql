set client_min_messages to WARNING;

SELECT topology.CreateTopology('tt') > 0;

COPY tt.face(face_id, mbr) FROM STDIN;
1	POLYGON((0 0,0 10,10 10,10 0,0 0))
2	POLYGON((2 2,2 8,8 8,8 2,2 2))
3	POLYGON((12 2,12 8,18 8,18 2,12 2))
\.

COPY tt.node(node_id, geom) FROM STDIN;
1	POINT(2 2)
2	POINT(0 0)
3	POINT(12 2)
\.

COPY tt.edge_data(
	edge_id, start_node, end_node,
	abs_next_left_edge, abs_next_right_edge,
	next_left_edge, next_right_edge,
	left_face, right_face, geom) FROM STDIN;
1	1	1	1	1	1	-1	1	2	LINESTRING(2 2, 2  8,  8  8,  8 2, 2 2)
2	2	2	2	2	2	-2	0	1	LINESTRING(0 0, 0 10, 10 10, 10 0, 0 0)
3	3	3	3	3	3	-3	0	3	LINESTRING(12 2, 12 8, 18 8, 18 2, 12 2)
\.

-- F1 should have an hole !
-- See http://trac.osgeo.org/postgis/ticket/726
SELECT 'f1 (with hole)', ST_asText(topology.st_getfacegeometry('tt', 1));
SELECT 'f2 (fill hole)', ST_asText(topology.st_getfacegeometry('tt', 2));

-- Face with multiple holes
-- See http://trac.osgeo.org/postgis/ticket/4966
COPY tt.face(face_id, mbr) FROM STDIN;
10	POLYGON((20 0,20 10,30 10,30 0,20 0))
11	POLYGON((22 2,22 4,24 4,24 2,22 2))
12	POLYGON((26 6,26 8,28 8,28 6,26 6))
\.

COPY tt.node(node_id, geom) FROM STDIN;
10	POINT(20 0)
11	POINT(22 2)
12	POINT(26 6)
\.

COPY tt.edge_data(
	edge_id, start_node, end_node,
	abs_next_left_edge, abs_next_right_edge,
	next_left_edge, next_right_edge,
	left_face, right_face, geom) FROM STDIN;
10	10	10	10	10	10	-10	0	10	LINESTRING(20 0,20 10,30 10,30 0,20 0)
11	11	11	11	11	11	-11	10	11	LINESTRING(22 2,22 4,24 4,24 2,22 2)
12	12	12	12	12	12	-12	10	12	LINESTRING(26 6,26 8,28 8,28 6,26 6)
\.

SELECT 'f10 (two holes)', ST_asText(topology.st_getfacegeometry('tt', 10));

-- Face with a hole larger than the surrounding annulus
-- See http://trac.osgeo.org/postgis/ticket/4966
COPY tt.face(face_id, mbr) FROM STDIN;
20	POLYGON((40 0,40 10,50 10,50 0,40 0))
21	POLYGON((40.5 0.5,40.5 9.5,49.5 9.5,49.5 0.5,40.5 0.5))
\.

COPY tt.node(node_id, geom) FROM STDIN;
20	POINT(40 0)
21	POINT(40.5 0.5)
\.

COPY tt.edge_data(
	edge_id, start_node, end_node,
	abs_next_left_edge, abs_next_right_edge,
	next_left_edge, next_right_edge,
	left_face, right_face, geom) FROM STDIN;
20	20	20	20	20	20	-20	0	20	LINESTRING(40 0,40 10,50 10,50 0,40 0)
21	21	21	21	21	21	-21	20	21	LINESTRING(40.5 0.5,40.5 9.5,49.5 9.5,49.5 0.5,40.5 0.5)
\.

SELECT 'f20 (large hole)', ST_asText(topology.st_getfacegeometry('tt', 20));

-- Face subdivided by a same-face bridge edge should keep the full face.
-- See http://trac.osgeo.org/postgis/ticket/4966
COPY tt.face(face_id, mbr) FROM STDIN;
30	POLYGON((60 0,60 10,70 10,70 0,60 0))
\.

COPY tt.node(node_id, geom) FROM STDIN;
30	POINT(60 0)
31	POINT(60 10)
32	POINT(70 0)
\.

COPY tt.edge_data(
	edge_id, start_node, end_node,
	abs_next_left_edge, abs_next_right_edge,
	next_left_edge, next_right_edge,
	left_face, right_face, geom) FROM STDIN;
30	30	30	30	30	30	-30	0	30	LINESTRING(60 0,60 10,70 10,70 0,60 0)
31	31	32	31	31	31	-31	30	30	LINESTRING(60 10,70 0)
\.

SELECT 'f30 (same-face bridge)', ST_asText(topology.st_getfacegeometry('tt', 30));

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
