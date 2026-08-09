set client_min_messages to WARNING;

\i :top_builddir/topology/test/load_topology-4326.sql
\i ../load_features.sql
\i ../more_features.sql
\i ../hierarchy.sql
SELECT topology.FixCorruptTopoGeometryColumn(schema_name, table_name, feature_column)
FROM topology.layer
ORDER BY schema_name, table_name, feature_column;

CREATE TABLE features.mixed_features(id serial primary key);
SELECT 'mixed-layer', topology.AddTopoGeometryColumn(
  'city_data',
  'features',
  'mixed_features',
  'feature',
  'COLLECTION'
);
-- Exercise repair of existing mixed-layer rows whose subtype does not match
-- the layer's collection type; AddTopoGeometryColumn now creates a stricter
-- check constraint that would reject this historic state.
ALTER TABLE features.mixed_features
  DROP CONSTRAINT check_topogeom_feature;
INSERT INTO features.mixed_features(feature)
  SELECT topology.toTopoGeom(
    'SRID=4326;LINESTRING(0 0, 10 0)'::geometry,
    'city_data',
    layer_id
  )
  FROM topology.layer
  WHERE schema_name = 'features'
  AND table_name = 'mixed_features'
  AND feature_column = 'feature';

SELECT 'mixed-before', (feature).type
  FROM features.mixed_features;
WITH repaired AS (
  UPDATE features.mixed_features
    SET feature = (
      (feature).topology_id,
      (feature).layer_id,
      (feature).id,
      l.feature_type
    )::topology.topogeometry
    FROM topology.layer AS l
    WHERE l.topology_id = (feature).topology_id
    AND l.layer_id = (feature).layer_id
    AND (feature).type <> l.feature_type
    AND (
      l.feature_type <> 4
      OR (feature).type NOT BETWEEN 1 AND 4
    )
    RETURNING 1
)
SELECT 'mixed-fallback-updates', count(*)
  FROM repaired;
SELECT 'mixed-after', (feature).type
  FROM features.mixed_features;

SELECT topology.DropTopology('city_data');
DROP SCHEMA features CASCADE;
