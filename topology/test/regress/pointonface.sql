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

SELECT 'f' || face_id,
  ST_AsText(topology.PointOnFace('tt', face_id)),
  ST_Contains(
    topology.ST_GetFaceGeometry('tt', face_id),
    topology.PointOnFace('tt', face_id)
  )
FROM tt.face
WHERE face_id > 0
ORDER BY face_id;

SELECT topology.PointOnFace('tt', 0);

SELECT topology.PointOnFace(null, 1);
SELECT topology.PointOnFace('tt', null);

SELECT topology.PointOnFace('NonExistent', 1);
SELECT topology.PointOnFace('', 1);

SELECT topology.PointOnFace('tt', 666);

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
SELECT ST_AsText(topology.PointOnFace('tt', 4));
SET client_min_messages TO WARNING;

SELECT topology.DropTopology('tt');
