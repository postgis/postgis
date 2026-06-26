\set VERBOSITY terse
set client_min_messages to WARNING;

SELECT topology.CreateTopology('2d') > 0;
SELECT topology.CreateTopology('2dLarge', 0, 0, false, 0, true) > 0;
SELECT topology.CreateTopology('edgeTrigger') > 0;

SELECT 'edge-insert-rule', count(*)
FROM pg_rewrite
WHERE ev_class = '"edgeTrigger".edge'::regclass
AND rulename = 'edge_insert_rule';

INSERT INTO "edgeTrigger".node(node_id, containing_face, geom)
VALUES
  (100, NULL, 'POINT(0 0)'::geometry),
  (101, NULL, 'POINT(1 0)'::geometry),
  (102, NULL, 'POINT(2 0)'::geometry);

INSERT INTO "edgeTrigger".edge(edge_id, start_node, end_node, next_left_edge, next_right_edge, left_face, right_face, geom)
VALUES (100, 100, 101, 100, -100, 0, 0, 'LINESTRING(0 0, 1 0)'::geometry);

SELECT 'edge-view-insert', edge_id, next_left_edge, abs_next_left_edge, next_right_edge, abs_next_right_edge
FROM "edgeTrigger".edge_data
WHERE edge_id = 100;

INSERT INTO "edgeTrigger".edge_data(edge_id, start_node, end_node, next_left_edge, abs_next_left_edge, next_right_edge, abs_next_right_edge, left_face, right_face, geom)
VALUES (101, 101, 102, -101, 0, 101, 0, 0, 0, 'LINESTRING(1 0, 2 0)'::geometry);

SELECT 'edge-data-insert', edge_id, next_left_edge, abs_next_left_edge, next_right_edge, abs_next_right_edge
FROM "edgeTrigger".edge_data
WHERE edge_id = 101;

UPDATE "edgeTrigger".edge_data
SET next_left_edge = -100,
  next_right_edge = 100
WHERE edge_id = 101;

SELECT 'edge-data-update', edge_id, next_left_edge, abs_next_left_edge, next_right_edge, abs_next_right_edge
FROM "edgeTrigger".edge_data
WHERE edge_id = 101;

SELECT topology.DropTopology('edgeTrigger');

SELECT topology.CreateTopology('2dAgain', -1, 0, false) > 0;
SELECT topology.CreateTopology('2dLarge.2', -1, 0, false, 0, true) > 0;
SELECT topology.CreateTopology('2dAgain.2', -1, 0, false, 2000) = 2000;
SELECT topology.CreateTopology('2dLarge.3', -1, 0, false, 2001, true) = 2001;

SELECT topology.CreateTopology('3d', -1, 0, true) > 0;
SELECT topology.CreateTopology('3dLarge', -1, 0, true, 0, true) > 0;
SELECT topology.CreateTopology('3d.2', -1, 0, true, 3000) = 3000;
SELECT topology.CreateTopology('3dLarge.2', -1, 0, true, 3001, true) = 3001;

-- Test if sequence is set after user defined topo id
SELECT topology.CreateTopology('2d.2'); -- should be 3002

SELECT topology.CreateTopology('3d'); -- already exists
SELECT topology.CreateTopology('3d.2'); -- already exists

SELECT name,srid,precision,hasz from topology.topology
WHERE name in ('2d', '2dAgain', '2dLarge', '3d', '3dLarge')
ORDER by name;

-- Only 3dZ accepted in 3d topo
SELECT topology.AddNode('3d', 'POINT(0 0)');
SELECT topology.AddNode('3d', 'POINTM(1 1 1)');
SELECT topology.AddNode('3d', 'POINT(2 2 2)');

-- Only 2d accepted in 2d topo
SELECT topology.AddNode('2d', 'POINTM(0 0 0)');
SELECT topology.AddNode('2d', 'POINT(1 1 1)');
SELECT topology.AddNode('2d', 'POINT(2 2)');

SELECT topology.DropTopology('2d');
SELECT topology.DropTopology('2d.2');
SELECT topology.DropTopology('2dLarge');
SELECT topology.DropTopology('2dLarge.2');
SELECT topology.DropTopology('2dLarge.3');
SELECT topology.DropTopology('2dAgain');
SELECT topology.DropTopology('2dAgain.2');
SELECT topology.DropTopology('3d');
SELECT topology.DropTopology('3d.2');
SELECT topology.DropTopology('3dLarge');
SELECT topology.DropTopology('3dLarge.2');

-- Exceptions
SELECT topology.CreateTopology('public');
SELECT topology.CreateTopology('topology');
