
\set VERBOSITY terse
set client_min_messages to WARNING;

\i ../load_topology-4326.sql

-- Add dangling loop
SELECT NULL FROM topology.TopoGeo_addLineString('city_data',
'SRID=4326;LINESTRING(16 38, 13 35)');
SELECT NULL FROM topology.TopoGeo_addLineString('city_data',
'SRID=4326;LINESTRING(9 35, 7 34)');
-- Add another dangling edge
SELECT NULL FROM topology.TopoGeo_addLineString('city_data',
'SRID=4326;LINESTRING(3 38, 4 36)');
-- Add hole touching shell
SELECT NULL FROM topology.TopoGeo_addLineString('city_data',
'SRID=4326;LINESTRING(25 30, 21 35,25 35)');


SELECT
  'F' || face_id,
  ST_AsEWKT(
    topology.GetFaceMaximalOuterRing('city_data', face_id)
  )
FROM city_data.face
WHERE face_id > 0
ORDER BY face_id;

SELECT NULL FROM topology.DropTopology('city_data');
