\set VERBOSITY terse
set client_min_messages to WARNING;

-- Create a topology in floating precision model
SELECT NULL FROM topology.CreateTopology('topo', 4326);

-- Add a linestring on a grid of size 2
SELECT NULL FROM topology.TopoGeo_AddLineString('topo',
  ST_Segmentize('SRID=4326;LINESTRING(0 0, 8 0)'::geometry, 2));

-- Add a connected linestring on a grid of size 2
SELECT NULL FROM topology.TopoGeo_AddLineString('topo',
  ST_Segmentize('SRID=4326;LINESTRING(8 0, 8 8)'::geometry, 2));

-- Add an isolated node on a grid of size 1
SELECT NULL FROM topology.TopoGeo_AddPoint('topo',
  'SRID=4326;POINT(-3 -1)'::geometry);

SELECT 'start', 'edge', edge_id, ST_AsEWKT(geom) FROM topo.edge ORDER BY edge_id;
SELECT 'start', 'node', node_id, ST_AsEWKT(geom) FROM topo.node WHERE containing_face IS NOT NULL ORDER BY node_id;

-- Validate as such
SELECT 'topo_float', ST_AsEWKT(ST_Normalize(
  topology.ValidateTopologyPrecision('topo')
));

-- Update topology precision to 4
UPDATE topology.topology SET precision = 4 WHERE name = 'topo';

-- Validate with precision 4
SELECT 'topo_prec4', ST_AsEWKT(ST_Normalize(
  topology.ValidateTopologyPrecision('topo')
));

-- Validate with precision 4 and limited bbox
SELECT 'topo_prec4_bbox', ST_AsEWKT(ST_Normalize(
  topology.ValidateTopologyPrecision(
    'topo',
    'LINESTRING(-1 -1,7 7)'::geometry
  )
));

-- Validate with gridSize 2
SELECT 'topo_prec2', ST_AsEWKT(ST_Normalize(
  topology.ValidateTopologyPrecision('topo', gridSize => 2)
));

-- Validate with too small gridSize
SELECT 'topo_prec_too_small', ST_AsEWKT(ST_Normalize(
  topology.ValidateTopologyPrecision('topo', gridSize => 1e-20)
));

SELECT NULL FROM topology.DropTopology('topo');

