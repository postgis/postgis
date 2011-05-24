set client_min_messages to WARNING;

select topology.CreateTopology('tt', -1) > 0;

select 'ST_AddIsoNode: test NULL exceptions';
--
select topology.ST_AddIsoNode(NULL, 0, 'POINT(1 4)');
select topology.ST_AddIsoNode('tt', 0, NULL);
select topology.ST_AddIsoNode('tt', NULL, NULL);
select topology.ST_AddIsoNode(NULL, NULL, NULL);
--
select 'ST_AddIsoNode: test wrong topology name';
--
select topology.ST_AddIsoNode('wrong_name', 0, 'POINT(1 4)');
--
select 'ST_AddIsoNode: test negative idface';
--
select topology.ST_AddIsoNode('tt', -1, 'POINT(1 4)');
--
select 'ST_AddIsoNode: test wrong idface';
--
select topology.ST_AddIsoNode('tt', 1, 'POINT(1 4)');
--
select 'ST_AddIsoNode: test smart creation ISO Node (without know idface)';
--
select topology.ST_AddIsoNode('tt', NULL, 'POINT(1 4)');
--
select 'ST_AddIsoNode: test coincident nodes';
--
select topology.ST_AddIsoNode('tt', 0, 'POINT(1 4)');
select topology.ST_AddIsoNode('tt', NULL, 'POINT(1 4)');
select topology.ST_AddIsoNode('tt', 1, 'POINT(1 4)');
--
select 'ST_AddIsoNode: test add a node in UniverseFace';
--
select topology.ST_AddIsoNode('tt', 0, 'POINT(2 2)');
--
select 'ST_AddIsoNode - prepare to test the creation inside a face';
--
select topology.DropTopology('tt');
select topology.CreateTopology('tt', -1) > 0;
select topology.ST_AddIsoNode('tt', NULL, 'POINT(1 1)');
select topology.ST_AddIsoNode('tt', NULL, 'POINT(5 2)');
select topology.ST_AddIsoNode('tt', NULL, 'POINT(4 6)');
select topology.ST_AddIsoNode('tt', NULL, 'POINT(0 4)');
select topology.ST_AddIsoNode('tt', NULL, 'POINT(2 2)');
select topology.ST_AddIsoNode('tt', NULL, 'POINT(4 3)');
select topology.ST_AddIsoNode('tt', NULL, 'POINT(3 5)');
select topology.ST_AddIsoNode('tt', NULL, 'POINT(1 3)');
select count(*) from tt.node where containing_face<>0;
select count(*) from tt.node where containing_face is null;
INSERT INTO tt.face (face_id, mbr) VALUES (2, '010300000001000000050000000000000000000000000000000000F03F00000000000000000000000000001840000000000000144000000000000018400000000000001440000000000000F03F0000000000000000000000000000F03F');
INSERT INTO tt.face (face_id, mbr) VALUES (3, '01030000000100000005000000000000000000F03F0000000000000040000000000000F03F00000000000014400000000000001040000000000000144000000000000010400000000000000040000000000000F03F0000000000000040');
BEGIN;
INSERT INTO tt.edge_data (edge_id, start_node, end_node, next_left_edge, abs_next_left_edge, next_right_edge, abs_next_right_edge, left_face, right_face, geom) VALUES (1, 1, 2, 2, 2, -4, 4, 2, 0, '010200000002000000000000000000F03F000000000000F03F00000000000014400000000000000040');
INSERT INTO tt.edge_data (edge_id, start_node, end_node, next_left_edge, abs_next_left_edge, next_right_edge, abs_next_right_edge, left_face, right_face, geom) VALUES (2, 2, 3, 3, 3, -1, 1, 2, 0, '0102000000020000000000000000001440000000000000004000000000000010400000000000001840');
INSERT INTO tt.edge_data (edge_id, start_node, end_node, next_left_edge, abs_next_left_edge, next_right_edge, abs_next_right_edge, left_face, right_face, geom) VALUES (3, 3, 4, 4, 4, -2, 2, 2, 0, '0102000000020000000000000000001040000000000000184000000000000000000000000000001040');
INSERT INTO tt.edge_data (edge_id, start_node, end_node, next_left_edge, abs_next_left_edge, next_right_edge, abs_next_right_edge, left_face, right_face, geom) VALUES (4, 4, 1, 1, 1, -3, 3, 2, 0, '01020000000200000000000000000000000000000000001040000000000000F03F000000000000F03F');
INSERT INTO tt.edge_data (edge_id, start_node, end_node, next_left_edge, abs_next_left_edge, next_right_edge, abs_next_right_edge, left_face, right_face, geom) VALUES (5, 5, 6, 6, 6, -8, 8, 3, 2, '0102000000020000000000000000000040000000000000004000000000000010400000000000000840');
INSERT INTO tt.edge_data (edge_id, start_node, end_node, next_left_edge, abs_next_left_edge, next_right_edge, abs_next_right_edge, left_face, right_face, geom) VALUES (6, 6, 7, 7, 7, -5, 5, 3, 2, '0102000000020000000000000000001040000000000000084000000000000008400000000000001440');
INSERT INTO tt.edge_data (edge_id, start_node, end_node, next_left_edge, abs_next_left_edge, next_right_edge, abs_next_right_edge, left_face, right_face, geom) VALUES (7, 7, 8, 8, 8, -6, 6, 3, 2, '01020000000200000000000000000008400000000000001440000000000000F03F0000000000000840');
INSERT INTO tt.edge_data (edge_id, start_node, end_node, next_left_edge, abs_next_left_edge, next_right_edge, abs_next_right_edge, left_face, right_face, geom) VALUES (8, 8, 5, 5, 5, -7, 7, 3, 2, '010200000002000000000000000000F03F000000000000084000000000000000400000000000000040');
END;
--
select 'ST_AddIsoNode: test a node inside a hole';
--
select topology.ST_AddIsoNode('tt', NULL, 'POINT(3 3)');
select count(*) from tt.node where node_id=9 AND containing_face=3;
--
select 'ST_AddIsoNode: test a node inside a face';
--
select topology.ST_AddIsoNode('tt', NULL, 'POINT(4 5)');
select count(*) from tt.node where node_id=10 AND containing_face=2;
--
select topology.DropTopology('tt');
