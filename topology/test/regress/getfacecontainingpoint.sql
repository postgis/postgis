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

-- Query point on a node incident to multiple faces
SELECT 'e1', topology.GetFaceContainingPoint('city_data', 'POINT(21 14)');

-- Query point on a node incident to universe face and proper face
SELECT 'e2', topology.GetFaceContainingPoint('city_data', 'POINT(9 22)');

-- Query point on a non-dangling edge binding 2 faces
SELECT 'e3', topology.GetFaceContainingPoint('city_data', 'POINT(28 14)');

-- Query point on a non-dangling edge binding universe and proper face
SELECT 'e4', topology.GetFaceContainingPoint('city_data', 'POINT(28 22)');

CREATE TABLE city_data.query_points(id int primary key, g geometry);
INSERT INTO city_data.query_points VALUES
-- Query point near isolated edge in a face
( 1, 'POINT(11 34)' ),
-- Query point near node of closed edge, outside of it
( 2, 'POINT(3.8 30.8)' ),
-- Query point near node of closed edge, inside of it
( 3, 'POINT(4.2 31.2)' ),
-- Query point on a node of degree 1 incident to single face
( 4, 'POINT(25 35)' ),
-- Query point on an dangling edge
( 5, 'POINT(25 33)' ),
-- Query point on a node of degree 2 incident to single face
( 6, 'POINT(57 33)' ),
-- Query point in hole touching shell, near edge having hole on the left
( 7, 'POINT(32 16.9)' ),
-- Query point in hole touching shell, near edge having hole on the right
( 8, 'POINT(24 16.9)' ),
-- Query point collinear with closest incoming segment (right of next one)
( 9, 'POINT(7.1 31)' ),
-- Query point on the right of the closest segment but left of ring
( 10, 'POINT(26 16.8)' ),
-- Query point on the left of the closest segment and left of ring
( 11, 'POINT(26 17.2)' ),
-- Query point on the left of the closest segment but right of ring
( 12, 'POINT(33.05 17.21)' ),
-- Query point on the right of the closest segment and right of ring
( 13, 'POINT(33.1 17.2)' ),
-- Query point near the start of a dangling edge
( 14, 'POINT(8 36)' );

SELECT 't'||id, topology.GetFaceContainingPoint('city_data', g)
FROM city_data.query_points
ORDER BY id;



SELECT NULL FROM topology.DropTopology('city_data');
