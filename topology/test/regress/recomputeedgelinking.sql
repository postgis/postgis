\set VERBOSITY terse
set client_min_messages to ERROR;
set search_path to public,pg_catalog;

\i :top_builddir/topology/test/load_topology.sql

SELECT 'unexpected_city_data_invalidities', * FROM topology.ValidateTopology('city_data');

-- Error calls

SELECT 'e1', topology.RecomputeEdgeLinking('non-existent'::text);
SELECT 'e2', topology.RecomputeEdgeLinking(NULL::text);

-- Full repair

BEGIN;
UPDATE city_data.edge_data SET next_left_edge = -next_left_edge where edge_id in (9,10,20);
UPDATE city_data.edge_data SET next_right_edge = -next_right_edge where edge_id = 19;
UPDATE city_data.edge_data
SET
  next_left_edge = -next_left_edge,
  next_right_edge = -next_right_edge
where edge_id in (3,25);

SELECT '#4942.full.before', count(*)
FROM topology.ValidateTopology('city_data')
WHERE error LIKE 'invalid next_%';
SELECT '#4942.full.updated', topology.RecomputeEdgeLinking('city_data');
SELECT '#4942.full.after', * FROM topology.ValidateTopology('city_data')
UNION
SELECT '#4942.full.after', '---', null, null
ORDER BY 1,2,3,4;
ROLLBACK;

-- Repair also refreshes the absolute next-edge cache columns.

BEGIN;
UPDATE city_data.edge_data
SET abs_next_left_edge = abs_next_left_edge + 1000
WHERE edge_id = 9;
UPDATE city_data.edge_data
SET abs_next_right_edge = abs_next_right_edge + 1000
WHERE edge_id = 19;

SELECT
  '#4942.abs.before',
  edge_id,
  abs_next_left_edge = abs(next_left_edge),
  abs_next_right_edge = abs(next_right_edge)
FROM city_data.edge_data
WHERE edge_id IN (9, 19)
ORDER BY edge_id;
SELECT '#4942.abs.updated', topology.RecomputeEdgeLinking('city_data');
SELECT
  '#4942.abs.after',
  edge_id,
  abs_next_left_edge = abs(next_left_edge),
  abs_next_right_edge = abs(next_right_edge)
FROM city_data.edge_data
WHERE edge_id IN (9, 19)
ORDER BY edge_id;
ROLLBACK;

-- Bbox repair is scoped to nodes intersecting the supplied bbox.

BEGIN;
UPDATE city_data.edge_data SET next_left_edge = -next_left_edge where edge_id in (9,10,20);
UPDATE city_data.edge_data SET next_right_edge = -next_right_edge where edge_id = 19;
UPDATE city_data.edge_data
SET
  next_left_edge = -next_left_edge,
  next_right_edge = -next_right_edge
where edge_id in (3,25);

SELECT '#4942.bbox.updated', topology.RecomputeEdgeLinking(
  'city_data',
  (SELECT ST_Expand(geom, 0.1) FROM city_data.node WHERE node_id = 14)
);
SELECT '#4942.bbox.remaining', * FROM topology.ValidateTopology('city_data')
UNION
SELECT '#4942.bbox.remaining', '---', null, null
ORDER BY 1,2,3,4;
ROLLBACK;

SELECT NULL FROM topology.DropTopology('city_data');
