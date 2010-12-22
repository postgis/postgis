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

-- Register a face with no holes
SELECT 'f1',  topology.addFace('tt', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');

-- Register the _same_ face  again
SELECT 'f1*',  topology.addFace('tt', 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))');

-- Check added faces
SELECT face_id, mbr from tt.face ORDER by face_id;

-- Check linking
SELECT edge_id, left_face, right_face from tt.edge ORDER by edge_id;

SELECT topology.DropTopology('tt');
