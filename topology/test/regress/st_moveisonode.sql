\set VERBOSITY terse
set client_min_messages to ERROR;

-- Import city_data
\i :top_builddir/topology/test/load_topology.sql

-- Isolated node to invalid location (coincident)
SELECT 'coincident_node', topology.ST_MoveIsoNode('city_data', 4, 'POINT(25 35)');
-- Isolated node to invalid location (edge-crossing)
SELECT 'edge-crossing', topology.ST_MoveIsoNode('city_data', 4, 'POINT(20 40)');

-- Non isolated node (is used by an edge);
SELECT 'not-isolated', topology.ST_MoveIsoNode('city_data', 3, 'POINT(25 36)');

-- Invalid point
SELECT 'invalid-new-point', topology.ST_MoveIsoNode('city_data', 4, 'MULTIPOINT(5 4)');

-- Move outside of original containing face (universal)
SELECT 'change-face-to-universal', topology.ST_MoveIsoNode('city_data', 4, 'POINT(20 41)');
-- Move outside of original containing face (universal)
SELECT 'change-face-to-other', topology.ST_MoveIsoNode('city_data', 4, 'POINT(15 37)');

-- Valid move
SELECT topology.ST_MoveIsoNode('city_data', 4, 'POINT(21 37)');
SELECT topology.ST_MoveIsoNode('city_data', 4, 'POINT(19 37)');

-- TODO: test moving the node in a different face (#3232)


-- Cleanup
SELECT NULL FROM topology.DropTopology('city_data');
