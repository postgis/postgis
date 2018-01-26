set client_min_messages to WARNING;

--
-- ST_InitTopoGeo
--

SELECT regexp_replace(ST_InitTopoGeo('sqlmm_topology'), 'id:[0-9]*', 'id:x');

-------------------------------------------------------------
-- ST_AddIsoNode (1)
-------------------------------------------------------------

SELECT '-- ST_AddIsoNode ------------------------';

-- null input
SELECT topology.ST_AddIsoNode('sqlmm_topology', NULL, NULL);
SELECT topology.ST_AddIsoNode(NULL, NULL, 'POINT(0 0)');
SELECT topology.ST_AddIsoNode(NULL, 1, NULL);

-- good nodes on the 'world' face
SELECT topology.ST_AddIsoNode('sqlmm_topology', NULL, 'POINT(0 0)');
SELECT topology.ST_AddIsoNode('sqlmm_topology', NULL, 'POINT(10 0)');
SELECT topology.ST_AddIsoNode('sqlmm_topology', NULL, 'POINT(5 0)');
SELECT topology.ST_AddIsoNode('sqlmm_topology', NULL, 'POINT(5 10)');
SELECT topology.ST_AddIsoNode('sqlmm_topology', NULL, 'POINT(10 10)');
SELECT topology.ST_AddIsoNode('sqlmm_topology', NULL, 'POINT(20 10)');

-- existing nodes
SELECT topology.ST_AddIsoNode('sqlmm_topology', NULL, 'POINT(0 0)');
SELECT topology.ST_AddIsoNode('sqlmm_topology', NULL, 'POINT(10 0)');

-- other good ones (add another 0 to be detected as coincident)
SELECT topology.ST_AddIsoNode('sqlmm_topology', NULL, 'POINT(10.000000000000001 0)');
SELECT topology.ST_AddIsoNode('sqlmm_topology', NULL, 'POINT(7 10)');

-- non-existent face specification
SELECT topology.ST_AddIsoNode('sqlmm_topology', 1, 'POINT(20 0)');

-- using other then point
SELECT topology.ST_AddIsoNode('sqlmm_topology', 1, 'MULTIPOINT(20 0)');

-- coincident nodes
SELECT topology.ST_AddIsoNode('sqlmm_topology', NULL, 'POINT(10.000000000000001 0)');
SELECT topology.ST_AddIsoNode('sqlmm_topology', NULL, 'POINT(0 0)');
SELECT topology.ST_AddIsoNode('sqlmm_topology', NULL, 'POINT(10 0)');

-- ST_AddIsoNode not within face (TODO when ST_GetFaceGeometry is done)

------------------------------------------
-- ST_AddIsoEdge (1)
------------------------------------------

SELECT '-- ST_AddIsoEdge ------------------------';

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

-- Good ones
SELECT topology.ST_AddIsoEdge('sqlmm_topology', 4, 5, 'LINESTRING(5 10, 5 9, 10 10)');
SELECT topology.ST_AddIsoEdge('sqlmm_topology', 1, 2, 'LINESTRING(0 0, 2 1, 10 5, 10 0)');

-- Not isolated edge (shares endpoint with previous)
SELECT topology.ST_AddIsoEdge('sqlmm_topology', 4, 6, 'LINESTRING(5 10, 10 9, 20 10)');
SELECT topology.ST_AddIsoEdge('sqlmm_topology', 5, 6, 'LINESTRING(10 10, 20 10)');

-- Edge intersection (geometry intersects an edge)
SELECT topology.ST_AddIsoEdge('sqlmm_topology', 1, 2, 'LINESTRING(0 0, 2 20, 10 0)');

-- on different faces (TODO req. nodes contained in face)

-------------------------------------------------------------
-- ST_AddIsoNode (2)
-------------------------------------------------------------

SELECT '-- ST_AddIsoNode(2) ------------------------';

-- ST_AddIsoNode edge-crossing node
SELECT topology.ST_AddIsoNode('sqlmm_topology', NULL, 'POINT(5 9.5)');

-------------------------------------------------------------
-- ST_RemoveIsoNode
-------------------------------------------------------------

SELECT '-- ST_RemoveIsoNode  ------------------------';

-- Isolated node
SELECT topology.ST_RemoveIsoNode('sqlmm_topology', 3);

-- Non isolated node (is used by an edge);
SELECT topology.ST_RemoveIsoNode('sqlmm_topology', 4);

-------------------------------------------------------------
-- ST_MoveIsoNode
-------------------------------------------------------------

SELECT '-- ST_MoveIsoNode  ------------------------';

-- Isolated node to invalid location (coincident)
SELECT topology.ST_MoveIsoNode('sqlmm_topology', 8, 'POINT(5 10)');
-- Isolated node to invalid location (edge-crossing)
SELECT topology.ST_MoveIsoNode('sqlmm_topology', 8, 'POINT(5 9)');

-- Non isolated node (is used by an edge);
SELECT topology.ST_MoveIsoNode('sqlmm_topology', 4, 'POINT(5 4)');

-- Invalid point
SELECT topology.ST_MoveIsoNode('sqlmm_topology', 2, 'MULTIPOINT(5 4)');

-- Valid move
SELECT topology.ST_MoveIsoNode('sqlmm_topology', 8, 'POINT(7 11)');
SELECT topology.ST_MoveIsoNode('sqlmm_topology', 8, 'POINT(7 10)');

-------------------------------------------------------------
-- ST_RemoveIsoEdge
-------------------------------------------------------------
SELECT '-- ST_RemoveIsoEdge ---------------------';

CREATE TEMP TABLE edge1_endnodes AS
  WITH edge AS (
    SELECT start_node,end_node
    FROM sqlmm_topology.edge_data
    WHERE edge_id = 1
  )
  SELECT start_node id FROM edge UNION
  SELECT end_node FROM edge;
SELECT '#3351.1', node_id, containing_face
 FROM sqlmm_topology.node where node_id in (
    SELECT id FROM edge1_endnodes
  )
 ORDER BY node_id;
SELECT topology.ST_RemoveIsoEdge('sqlmm_topology', 1);
SELECT '#3351.2', node_id, containing_face
 FROM sqlmm_topology.node where node_id in (
    SELECT id FROM edge1_endnodes
  )
 ORDER BY node_id;
DROP TABLE edge1_endnodes;

-------------------------------------------------------------
-- ST_NewEdgesSplit
-------------------------------------------------------------

SELECT '-- ST_NewEdgesSplit  ---------------------';
SELECT topology.ST_NewEdgesSplit('sqlmm_topology', 2, 'POINT(10 2)');

SELECT topology.DropTopology('sqlmm_topology');
