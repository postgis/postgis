set client_min_messages to WARNING;

\i load_topology.sql
\i validate_topology.sql

-- clean up
SELECT topology.DropTopology('city_data');
