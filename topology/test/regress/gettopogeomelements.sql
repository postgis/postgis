set client_min_messages to WARNING;

\i load_topology.sql
\i load_features.sql

SELECT lid, tid, GetTopoGeomElements('city_data', lid, tid)
FROM (
 SELECT DISTINCT layer_id as lid, topogeo_id as tid
 FROM city_data.relation
) as f
order by 1, 2, 3;

SELECT lid, tid, 'ARY', GetTopoGeomElementArray('city_data', lid, tid)
FROM (
 SELECT DISTINCT layer_id as lid, topogeo_id as tid
 FROM city_data.relation
) as f
order by 1, 2;

-- clean up
SELECT topology.DropTopology('city_data');
DROP SCHEMA features CASCADE;
