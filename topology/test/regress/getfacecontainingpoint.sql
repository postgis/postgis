\set VERBOSITY terse
set client_min_messages to WARNING;

\i ../load_topology.sql

-- Add holes touching shell defined in CW order
SELECT NULL FROM topology.TopoGeo_addLineString('city_data', 'LINESTRING(24 14, 23 17,25 17, 24 14)');

-- Add holes touching shell defined in CCW order
SELECT NULL FROM topology.TopoGeo_addLineString('city_data', 'LINESTRING(32 14, 33 17,31 17, 32 14)');


-- Get face containing the "point on surface" of each face's geometry
SELECT 'pos', face_id, topology.GetFaceContainingPoint(
  'city_data',
  ST_PointOnSurface(
    ST_GetFaceGeometry(
      'city_data',
      face_id
    )
  )
)
FROM city_data.face
WHERE face_id > 0
ORDER BY face_id;

-- Query point near isolated edge in a face
SELECT 't1', topology.GetFaceContainingPoint('city_data', 'POINT(11 34)');

-- Query point near node of closed edge, outside of it
SELECT 't2', topology.GetFaceContainingPoint('city_data', 'POINT(3.8 30.8)');

-- Query point near node of closed edge, inside of it
SELECT 't3', topology.GetFaceContainingPoint('city_data', 'POINT(4.2 31.2)');

-- Query point on a node of degree 1 incident to single face
SELECT 't4', topology.GetFaceContainingPoint('city_data', 'POINT(25 35)');

-- Query point on an dangling edge
SELECT 't5', topology.GetFaceContainingPoint('city_data', 'POINT(25 33)');

-- Query point on a node of degree 2 incident to single face
SELECT 't6', topology.GetFaceContainingPoint('city_data', 'POINT(57 33)');

-- Query point in hole touching shell, near edge having hole on the left
SELECT 't7', topology.GetFaceContainingPoint('city_data', 'POINT(32 16.9)');

-- Query point in hole touching shell, near edge having hole on the right
SELECT 't8', topology.GetFaceContainingPoint('city_data', 'POINT(24 16.9)');

-- Query point on a node incident to multiple faces
SELECT 'e1', topology.GetFaceContainingPoint('city_data', 'POINT(21 14)');

-- Query point on a node incident to universe face and proper face
SELECT 'e2', topology.GetFaceContainingPoint('city_data', 'POINT(9 22)');

-- Query point on a non-dangling edge binding 2 faces
SELECT 'e3', topology.GetFaceContainingPoint('city_data', 'POINT(28 14)');

-- Query point on a non-dangling edge binding universe and proper face
SELECT 'e4', topology.GetFaceContainingPoint('city_data', 'POINT(28 22)');


SELECT NULL FROM topology.DropTopology('city_data');
