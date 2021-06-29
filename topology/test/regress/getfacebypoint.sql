set client_min_messages to ERROR;

SELECT NULL FROM topology.CreateTopology('topo');

SELECT NULL FROM topology.TopoGeo_addPolygon('topo', 'POLYGON((1 2, 1 5, 10 5, 10 2, 1 2 ))');
SELECT NULL FROM topology.TopoGeo_addPolygon('topo', 'POLYGON((10 2, 10 5, 10 12, 10 14, 10 15, 15 15, 15 2, 10 2))');
SELECT NULL FROM topology.TopoGeo_addPolygon('topo', 'POLYGON((7 12, 7 15, 10 15, 10 14, 8 14, 8 12, 7 12))');
SELECT NULL FROM topology.TopoGeo_addPolygon('topo', 'POLYGON((4 7,6 7,6 10,4 10,4 7))');
SELECT NULL FROM topology.TopoGeo_addPolygon('topo', 'POLYGON((1 5, 1 12, 7 12, 8 12, 10 12, 10 5, 1 5),(4 7, 4 10, 6 10, 6 7, 4 7))');
SELECT NULL FROM topology.TopoGeo_addPolygon('topo', 'POLYGON((8 12,8 14,10 14,10 12,8 12))');

-- Check each computed point-on-surface is found to be in its own face
WITH faces AS (
  SELECT face_id, topology.ST_GetFaceGeometry('topo', face_id) geom
  FROM topo.face WHERE face_id > 0
), faces_and_query_points AS (
  SELECT face_id, geom, ST_PointOnSurface(geom) pos
  FROM faces
)
SELECT 't1',
  'F' || face_id,
  face_id = topology.GetFaceByPoint(
    'topo',
    pos,
    0
  )
FROM faces_and_query_points
ORDER BY face_id;


-- Ask for a point outside a face but with a tolerance sufficient to include one face
SELECT 't2',
  topology.GetFaceByPoint('topo', 'POINT(6.5 13)', 0.5) =
  topology.GetFaceByPoint('topo', 'POINT(7.5 13)', 0)
;

SELECT 't3',
  topology.GetFaceByPoint('topo', 'POINT(3 13)', 1) =
  topology.GetFaceByPoint('topo', 'POINT(3 11)', 0)
;

-- ask for a Point where there isn't a Face
SELECT 't4', topology.GetFaceByPoint('topo','POINT(5 14)', 0);

-- Ask for a point whose closest point on closest edge
-- is an endpoint
SELECT 't5',
  topology.GetFaceByPoint('topo', 'POINT(8 11)', 0) =
  topology.GetFaceByPoint('topo', 'POINT(9 11)', 0)
;

-- Failing cases (should all raise exceptions) -------

-- Ask for Point in a Node (2 or more faces)
SELECT 'e1', topology.GetFaceByPoint('topo','POINT(1 5)', 0);

-- Ask for a Point with a tollerance too high (2 or more faces)
SELECT 'e2', topology.GetFaceByPoint('topo','POINT(6 13)', 1);

SELECT NULL FROM topology.DropTopology('topo');
