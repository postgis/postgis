set client_min_messages to WARNING;

\i :top_builddir/topology/test/load_topology.sql
\i :regdir/../topology/test/load_features.sql
\i :regdir/../topology/test/more_features.sql
\i :regdir/../topology/test/hierarchy.sql
\i :top_builddir/topology/test/topo_predicates.sql

-- clean up
SELECT topology.DropTopology('city_data');
DROP SCHEMA features CASCADE;
