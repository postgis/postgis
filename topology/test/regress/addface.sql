set client_min_messages to WARNING;

-- Test with zero tolerance

SELECT topology.CreateTopology('tt') > 0;

-- Register a face in absence of edges (exception expected)
SELECT 'f*',  topology.addFace('tt', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');

-- Create 4 edges

SELECT 'e1',  topology.addEdge('tt', 'LINESTRING(0 0, 10 0)');
SELECT 'e2',  topology.addEdge('tt', 'LINESTRING(10 0, 10 10)');
SELECT 'e3',  topology.addEdge('tt', 'LINESTRING(0 10, 10 10)');
SELECT 'e4',  topology.addEdge('tt', 'LINESTRING(0 0, 0 10)');

-- Add one edge only incident on a vertex
SELECT 'e5',  topology.addEdge('tt', 'LINESTRING(0 0, 0 -10)');

-- Add 3 more edges closing a squre to the right,
-- all edges with same direction

SELECT 'e6',  topology.addEdge('tt', 'LINESTRING(10 10, 20 10)');
SELECT 'e7',  topology.addEdge('tt', 'LINESTRING(20 10, 20 0)');
SELECT 'e8',  topology.addEdge('tt', 'LINESTRING(20 0, 10 0)');


-- Register a face with no holes
SELECT 'f1',  topology.addFace('tt', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');

-- Register the _same_ face  again
SELECT 'f1*',  topology.addFace('tt', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');

-- Register a face with no holes matching all edges in the same direction
SELECT 'f2',  topology.addFace('tt', 'POLYGON((10 10, 20 10, 20 0, 10 0, 10 10))');

-- Check added faces
SELECT face_id, Box2d(mbr) from tt.face ORDER by face_id;

-- Check linking
SELECT edge_id, left_face, right_face from tt.edge ORDER by edge_id;

SELECT topology.DropTopology('tt');

-- Test topology with MixedCase
SELECT topology.CreateTopology('Ul') > 0;
SELECT 'MiX-e1',  topology.addEdge('Ul', 'LINESTRING(0 0, 10 0)');
SELECT 'MiX-e2',  topology.addEdge('Ul', 'LINESTRING(10 0, 10 10)');
SELECT 'MiX-e3',  topology.addEdge('Ul', 'LINESTRING(0 10, 10 10)');
SELECT 'MiX-e4',  topology.addEdge('Ul', 'LINESTRING(0 0, 0 10)');
SELECT 'MiX-f1',  topology.addFace('Ul', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');
SELECT topology.DropTopology('Ul');

-- Test polygons with holes
SELECT topology.CreateTopology('t2') > 0;

-- Edges forming two squares
SELECT 't2.e1',  topology.addEdge('t2', 'LINESTRING(0 0, 10 0)');
SELECT 't2.e2',  topology.addEdge('t2', 'LINESTRING(10 0, 10 10)');
SELECT 't2.e3',  topology.addEdge('t2', 'LINESTRING(0 10, 10 10)');
SELECT 't2.e4',  topology.addEdge('t2', 'LINESTRING(0 0, 0 10)');
SELECT 't2.e5',  topology.addEdge('t2', 'LINESTRING(10 10, 20 10)');
SELECT 't2.e6',  topology.addEdge('t2', 'LINESTRING(20 10, 20 0)');
SELECT 't2.e7',  topology.addEdge('t2', 'LINESTRING(20 0, 10 0)');

-- Clockwise hole within the square on the left
SELECT 't2.e8',  topology.addEdge('t2', 'LINESTRING(1 1, 1 2, 2 2, 2 1, 1 1)');
-- Counter-clockwise hole within the square on the left
SELECT 't2.e9',  topology.addEdge('t2', 'LINESTRING(3 1,4 1,4 2,3 2,3 1)');

-- Multi-edge hole within the square on the right
SELECT 't2.e10',  topology.addEdge('t2', 'LINESTRING(12 2, 14 2, 14 4)');
SELECT 't2.e11',  topology.addEdge('t2', 'LINESTRING(12 2, 12 4, 14 4)');

-- Register left face with two holes
SELECT 't2.f1',  topology.addFace('t2',
'POLYGON((0 0,10 0,10 10,0 10,0 0),
         (1 1,2 1,2 2,1 2,1 1),
         (3 1,3 2,4 2,4 1,3 1))'
);

-- Register right face with one hole
SELECT 't2.f2',  topology.addFace('t2',
'POLYGON((20 0,10 0,10 10,20 10,20 0),
         (12 2,14 2,14 4,12 4, 12 2))'
);

-- Register left hole in left square
SELECT 't2.f3',  topology.addFace('t2',
'POLYGON((1 1,2 1,2 2,1 2,1 1))'
);

-- Register right hole in left square
SELECT 't2.f4',  topology.addFace('t2',
'POLYGON((3 1,4 1,4 2,3 2,3 1))'
);

-- Register hole in right face 
SELECT 't2.f5',  topology.addFace('t2',
'POLYGON((12 2,12 4,14 4,14 2,12 2))'
);

-- Check added faces
SELECT face_id, Box2d(mbr) from t2.face ORDER by face_id;

-- Check linking
SELECT edge_id, left_face, right_face from t2.edge ORDER by edge_id;

SELECT topology.DropTopology('t2');
