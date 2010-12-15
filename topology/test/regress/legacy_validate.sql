\i load_topology.sql
\i validate_topology.sql

-- clean up
SELECT topology.DropTopology('city_data');
DROP SCHEMA features CASCADE;
