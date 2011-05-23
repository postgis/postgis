set client_min_messages to WARNING;

SELECT topology.CreateTopology('sqlmm_topology') > 0;

-- Put some points in
INSERT INTO sqlmm_topology.node (containing_face, geom) VALUES
  (0, 'POINT(0 0)') RETURNING 'N' || node_id; -- 1
INSERT INTO sqlmm_topology.node (containing_face, geom) VALUES
  (0, 'POINT(10 0)') RETURNING 'N' || node_id; -- 2
INSERT INTO sqlmm_topology.node (containing_face, geom) VALUES
  (0, 'POINT(5 0)') RETURNING 'N' || node_id; -- 3
INSERT INTO sqlmm_topology.node (containing_face, geom) VALUES
  (0, 'POINT(5 10)') RETURNING 'N' || node_id; -- 4
INSERT INTO sqlmm_topology.node (containing_face, geom) VALUES
  (0, 'POINT(10 10)') RETURNING 'N' || node_id; -- 5
INSERT INTO sqlmm_topology.node (containing_face, geom) VALUES
  (0, 'POINT(20 10)') RETURNING 'N' || node_id; -- 6

-- null input
SELECT topology.ST_AddIsoEdge('sqlmm_topology', 1, 2, NULL);
SELECT topology.ST_AddIsoEdge(NULL, 1, 2, 'LINESTRING(0 0, 1 1)');
SELECT topology.ST_AddIsoEdge('sqlmm_topology', 1, NULL, 'LINESTRING(0 0, 1 1)');
SELECT topology.ST_AddIsoEdge('sqlmm_topology', NULL, 2, 'LINESTRING(0 0, 1 1)');

-- invalid curve
SELECT topology.ST_AddIsoEdge('sqlmm_topology', 1, 2, 'POINT(0 0)');

-- non-simple curve
SELECT topology.ST_AddIsoEdge('sqlmm_topology', 1, 2, 'LINESTRING(0 0, 10 0, 5 5, 5 -5)');

-- non-existing nodes
SELECT topology.ST_AddIsoEdge('sqlmm_topology', 10000, 2, 'LINESTRING(0 0, 1 1)');

-- Curve endpoints mismatch
SELECT topology.ST_AddIsoEdge('sqlmm_topology', 1, 2, 'LINESTRING(0 0, 1 1)');
SELECT topology.ST_AddIsoEdge('sqlmm_topology', 1, 2, 'LINESTRING(0 1, 10 0)');

-- Node crossing 
SELECT topology.ST_AddIsoEdge('sqlmm_topology', 1, 2, 'LINESTRING(0 0, 10 0)');

-- Good one
SELECT 'E' || topology.ST_AddIsoEdge('sqlmm_topology',
  4, 5, 'LINESTRING(5 10, 5 9, 10 10)');

-- Another good one
SELECT 'E' || topology.ST_AddIsoEdge('sqlmm_topology',
  1, 2, 'LINESTRING(0 0, 2 1, 10 5, 10 0)');

-- Check that containing_face became NULL for the nodes which are
-- not isolated anymore (#976)
SELECT 'N' || node_id, containing_face FROM sqlmm_topology.node
  ORDER BY node_id;

-- Not isolated edge (shares endpoint with previous)
SELECT topology.ST_AddIsoEdge('sqlmm_topology',
  4, 6, 'LINESTRING(5 10, 10 9, 20 10)');
SELECT topology.ST_AddIsoEdge('sqlmm_topology',
  5, 6, 'LINESTRING(10 10, 20 10)');

-- Edge intersection (geometry intersects an edge)
SELECT topology.ST_AddIsoEdge('sqlmm_topology',
  1, 2, 'LINESTRING(0 0, 2 20, 10 0)');

-- TODO: check closed edge (not-isolated I guess...)
-- on different faces (TODO req. nodes contained in face)


SELECT topology.DropTopology('sqlmm_topology');
