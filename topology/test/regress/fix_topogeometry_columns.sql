set client_min_messages to WARNING;

\i :top_builddir/topology/test/load_topology-4326.sql
\i ../load_features.sql
\i ../more_features.sql
\i ../hierarchy.sql
SELECT topology.FixCorruptTopoGeometryColumn(schema_name, table_name, feature_column)
FROM topology.layer
ORDER BY schema_name, table_name, feature_column;

SELECT topology.DropTopology('city_data');
DROP SCHEMA features CASCADE;
