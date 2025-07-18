\set VERBOSITY terse
set client_min_messages to WARNING;

SELECT topology.CreateTopology('2d') > 0;
SELECT topology.CreateTopology('2dLarge', 0, 0, false, 0, true) > 0;

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

