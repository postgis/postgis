
\set VERBOSITY terse
set client_min_messages to ERROR;

\i :top_builddir/topology/test/load_large_topology.sql
\i ../load_large_features.sql
\i ../large_hierarchy.sql
--\i ../more_features.sql

SELECT 'valid-start', * FROM topology.ValidateTopologyRelation ('large_city_data');

-- Delete all primitives
DELETE FROM large_city_data.edge_data;
DELETE FROM large_city_data.node;
DELETE FROM large_city_data.face WHERE face_id > 0;

SELECT 'invalid-primitives', * FROM topology.ValidateTopologyRelation ('large_city_data')
ORDER BY 3, 4, 5;

-- Delete features from primitive tables
WITH
deleted_land_parcels AS (
  DELETE FROM large_features.land_parcels
  RETURNING feature
),
deleted_traffic_signs AS (
  DELETE FROM large_features.traffic_signs
  RETURNING feature
),
deleted_city_streets AS (
  DELETE FROM large_features.city_streets
  RETURNING feature
)
SELECT NULL FROM (
  SELECT ClearTopoGeom (feature) FROM deleted_land_parcels
    UNION
  SELECT ClearTopoGeom (feature) FROM deleted_traffic_signs
    UNION
  SELECT ClearTopoGeom (feature) FROM deleted_city_streets
) foo
;

SELECT 'invalid-hierarchical', * FROM topology.ValidateTopologyRelation ('large_city_data')
ORDER BY 3, 4, 5;

--SELECT * FROM topology.layer WHERE child_id IS NOT NULL;

SELECT topology.DropTopology ('large_city_data');
DROP SCHEMA large_features CASCADE;
