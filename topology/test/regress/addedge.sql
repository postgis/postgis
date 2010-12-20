set client_min_messages to WARNING;

SELECT topology.CreateTopology('tt') > 0;

SELECT 'e1',  topology.addEdge('tt', 'LINESTRING(0 0, 8 0)');

-- Failing cases (should all raise exceptions) -------

-- Equals
SELECT 'e*1', topology.addEdge('tt', 'LINESTRING(0 0, 8 0)');
-- Contained with endpoint contact 
SELECT 'e*2', topology.addEdge('tt', 'LINESTRING(1 0, 8 0)');
-- Contained with no endpoint contact 
SELECT 'e*3', topology.addEdge('tt', 'LINESTRING(1 0, 7 0)');
-- Overlapping 
SELECT 'e*4', topology.addEdge('tt', 'LINESTRING(1 0, 9 0)');
-- Contains with endpoint contact
SELECT 'e*5', topology.addEdge('tt', 'LINESTRING(0 0, 9 0)');
-- Contains with no endpoint contact
SELECT 'e*6', topology.addEdge('tt', 'LINESTRING(-1 0, 9 0)');
-- Touches middle with endpoint
SELECT 'e*7', topology.addEdge('tt', 'LINESTRING(5 0, 5 10)');
-- Crosses 
SELECT 'e*8', topology.addEdge('tt', 'LINESTRING(5 -10, 5 10)');
-- Is touched on the middle by endpoint
SELECT 'e*9', topology.addEdge('tt', 'LINESTRING(0 -10, 0 10)');
-- Touches middle with internal vertex 
SELECT 'e*10', topology.addEdge('tt', 'LINESTRING(0 10, 5 0, 5 10)');

-- Endpoint touching cases (should succeed) ------

SELECT 'e2',  topology.addEdge('tt', 'LINESTRING(8 0, 8 10)');
SELECT 'e3',  topology.addEdge('tt', 'LINESTRING(0 0, 0 10)');
-- this one connects e2-e3
SELECT 'e4',  topology.addEdge('tt', 'LINESTRING(8 10, 0 10)');

-- Disjoint case (should succeed) ------

SELECT 'e5',  topology.addEdge('tt', 'LINESTRING(8 -10, 0 -10)');

-- this one touches the same edge (e5) at both endpoints
SELECT 'e6',  topology.addEdge('tt', 'LINESTRING(8 -10, 4 -20, 0 -10)');

SELECT edge_id, left_face, right_face,
	next_left_edge, next_right_edge,
	st_astext(geom) from tt.edge ORDER by edge_id;

SELECT topology.DropTopology('tt');
