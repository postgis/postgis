BEGIN;
SELECT NULL FROM topology.CreateTopology('topo');
SELECT 't1', topology.RenameTopology('topo', 'topo with space');
INSERT INTO "topo with space".node(node_id, geom)
VALUES
	(1, 'POINT(0 0)'::geometry),
	(2, 'POINT(1 1)'::geometry);
INSERT INTO "topo with space".edge (
	edge_id, start_node, end_node,
	next_left_edge, next_right_edge,
	left_face, right_face, geom
) VALUES (
	1, 1, 2,
	1, -1,
	0, 0, 'LINESTRING(0 0,1 1)'::geometry
);
SELECT 't-edge', count(*) FROM "topo with space".edge_data;
SELECT 't2', topology.RenameTopology('topo with space', 'topo with MixedCase');
SELECT 't2', topology.RenameTopology('topo with MixedCase', 'topo');
ROLLBACK;
