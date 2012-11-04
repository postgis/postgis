set client_min_messages to WARNING;

INSERT INTO spatial_ref_sys ( auth_name, auth_srid, srid, proj4text ) VALUES ( 'EPSG', 4326, 4326, '+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs' );
\i load_topology-4326.sql
\i load_features.sql
\i more_features.sql

SELECT topology.CopyTopology('city_data', 'CITY_data_UP_down') > 0;

SELECT srid,precision FROM topology.topology WHERE name = 'CITY_data_UP_down';

SELECT 'nodes', count(node_id) FROM "CITY_data_UP_down".node;
SELECT * FROM "CITY_data_UP_down".node EXCEPT
SELECT * FROM "city_data".node;

SELECT 'edges', count(edge_id) FROM "CITY_data_UP_down".edge_data;
SELECT * FROM "CITY_data_UP_down".edge EXCEPT
SELECT * FROM "city_data".edge;

SELECT 'faces', count(face_id) FROM "CITY_data_UP_down".face;
SELECT * FROM "CITY_data_UP_down".face EXCEPT
SELECT * FROM "city_data".face;

SELECT 'relations', count(*) FROM "CITY_data_UP_down".relation;
SELECT * FROM "CITY_data_UP_down".relation EXCEPT
SELECT * FROM "city_data".relation;

SELECT 'layers', count(l.*) FROM topology.layer l, topology.topology t
WHERE l.topology_id = t.id and t.name = 'CITY_data_UP_down';
SELECT l.layer_id, l.feature_type, l.level FROM topology.layer l,
topology.topology t where l.topology_id = t.id and t.name = 'CITY_data_UP_down'
EXCEPT
SELECT l.layer_id, l.feature_type, l.level FROM topology.layer l,
topology.topology t where l.topology_id = t.id and t.name = 'city_data';

SELECT l.layer_id, l.schema_name, l.table_name, l.feature_column
FROM topology.layer l, topology.topology t
WHERE l.topology_id = t.id and t.name = 'CITY_data_UP_down'
ORDER BY l.layer_id;

-- Check sequences
SELECT * from "CITY_data_UP_down".node_node_id_seq;
SELECT * from "CITY_data_UP_down".edge_data_edge_id_seq;
SELECT * from "CITY_data_UP_down".face_face_id_seq;
SELECT * from "CITY_data_UP_down".layer_id_seq;
SELECT * from "CITY_data_UP_down".topogeo_s_1;
SELECT * from "CITY_data_UP_down".topogeo_s_2;
SELECT * from "CITY_data_UP_down".topogeo_s_3;

SELECT topology.DropTopology('CITY_data_UP_down');
SELECT topology.DropTopology('city_data');
DROP SCHEMA features CASCADE;
DELETE FROM spatial_ref_sys where srid = 4326;
