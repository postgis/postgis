set client_min_messages to WARNING;

INSERT INTO spatial_ref_sys ( auth_name, auth_srid, srid, proj4text ) VALUES ( 'EPSG', 4326, 4326, '+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs' );

\i load_topology-4326.sql
\i load_features.sql

--- Puntual outputs {

-- Output simple puntual features (single-geom)
SELECT feature_name||'-vanilla', topology.AsGML(feature)
 FROM features.traffic_signs
 WHERE feature_name IN ('S1', 'S2', 'S3', 'S4' )
 ORDER BY feature_name;

-- Output again but with no prefix
SELECT feature_name||'-noprefix', topology.AsGML(feature, '')
 FROM features.traffic_signs WHERE feature_name = 'S1';

-- Output again with custom prefix
SELECT feature_name||'-customprefix', topology.AsGML(feature, 'cstm')
 FROM features.traffic_signs WHERE feature_name = 'S1';

--- } Puntual outputs 

--- Lineal outputs (TODO) {
--- } Lineal outputs 

--- Areal outputs (TODO) {
--- } Areal outputs 

SELECT topology.DropTopology('city_data');
DROP SCHEMA features CASCADE;
DELETE FROM spatial_ref_sys where srid = 4326;
