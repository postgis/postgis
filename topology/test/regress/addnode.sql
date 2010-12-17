set client_min_messages to WARNING;

-- Test with zero tolerance

SELECT topology.CreateTopology('nodes') > 0;

-- Check that the same point geometry return the same node id

SELECT 'p1',  topology.addNode('nodes', 'POINT(0 0)');
SELECT 'p1b', topology.addNode('nodes', 'POINT(0 0)');

SELECT 'p2',  topology.addNode('nodes', 'POINT(1 0)');
SELECT 'p2b', topology.addNode('nodes', 'POINT(1 0)');

-- Check we only have two points, both with unknown containing face

SELECT node_id, containing_face, st_astext(geom) from nodes.node
ORDER by node_id;

SELECT topology.DropTopology('nodes');
