set client_min_messages to WARNING;

SELECT topology.CreateTopology('tt') > 0;

COPY tt.face(face_id) FROM STDIN;
1
2
\.

COPY tt.node(node_id, geom) FROM STDIN;
1	POINT(2 2)
2	POINT(0 0)
\.

COPY tt.edge_data(
	edge_id, start_node, end_node,
	abs_next_left_edge, abs_next_right_edge,
	next_left_edge, next_right_edge,
        left_face, right_face, geom) FROM STDIN;
1	1	1	1	1	1	1	1	2	LINESTRING(2 2, 2  8,  8  8,  8 2, 2 2)
2	2	2	2	2	2	2	0	1	LINESTRING(0 0, 0 10, 10 10, 10 0, 0 0)
\.

--INSERT INTO tt.edge(edge_id,
--	start_node, end_node,
--	next_left_edge, next_right_edge,
--	left_face, right_face, geom)
--VALUES(2,2,2,2,2,
--	0,1,'LINESTRING(0 0, 0 10, 10 10,  10 0, 0 0)');

-- SELECT * FROM topology.ValidateTopology('tt');

-- F1 should have an hole !
SELECT 'f1 (with hole)', ST_asText(topology.st_getfacegeometry('tt', 1));
SELECT 'f2 (fill hole)', ST_asText(topology.st_getfacegeometry('tt', 2));

SELECT topology.DropTopology('tt');
