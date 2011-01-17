set client_min_messages to WARNING;

-- Test with zero tolerance

SELECT topology.CreateTopology('nodes') > 0;

-- Check that the same point geometry return the same node id

SELECT 'p1',  topology.addNode('nodes', 'POINT(0 0)');
SELECT 'p1b', topology.addNode('nodes', 'POINT(0 0)');

SELECT 'p2',  topology.addNode('nodes', 'POINT(1 0)');
SELECT 'p2b', topology.addNode('nodes', 'POINT(1 0)');

-- Check that adding a node in the middle of an existing edge is refused
-- While adding one on the endpoint is fine
INSERT INTO nodes.edge VALUES(1,1,1,1,-1,0,0,'LINESTRING(0 10,10 10)');
SELECT 'p3*1',  topology.addNode('nodes', 'POINT(5 10)'); -- refused
SELECT 'p3',  topology.addNode('nodes', 'POINT(0 10)'); -- good
SELECT 'p4',  topology.addNode('nodes', 'POINT(10 10)'); -- good
-- And same against a closed edge
INSERT INTO nodes.edge VALUES(2,2,2,2,-2,0,0,
 'LINESTRING(0 20,10 20,10 30, 0 30, 0 20)');
SELECT 'p5',  topology.addNode('nodes', 'POINT(0 20)'); -- good

-- Check we only have two points, both with unknown containing face

SELECT node_id, containing_face, st_astext(geom) from nodes.node
ORDER by node_id;

SELECT topology.DropTopology('nodes');
