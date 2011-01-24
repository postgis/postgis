set client_min_messages to WARNING;

INSERT INTO spatial_ref_sys ( auth_name, auth_srid, srid, proj4text ) VALUES ( 'EPSG', 4326, 4326, '+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs' );

\i load_topology-4326.sql
\i load_features.sql

--- Puntual single element {

-- Output simple puntual features (composed by single topo-element)
SELECT feature_name||'-vanilla', topology.AsGML(feature)
 FROM features.traffic_signs
 WHERE feature_name IN ('S1', 'S2', 'S3', 'S4' )
 ORDER BY feature_name;

-- Output again but with no prefix
SELECT feature_name||'-noprefix', topology.AsGML(feature, '')
 FROM features.traffic_signs 
 WHERE feature_name IN ('S1', 'S2', 'S3', 'S4' )
 ORDER BY feature_name;

-- Output again with custom prefix
SELECT feature_name||'-customprefix', topology.AsGML(feature, 'cstm')
 FROM features.traffic_signs 
 WHERE feature_name IN ('S1', 'S2', 'S3', 'S4' )
 ORDER BY feature_name;

--- } Puntual single-element 

--- Puntual multi element (TODO) {
--- } Puntual multi-element 

--- Lineal single element {

-- Output simple lineal features (composed by single topo element)
SELECT feature_name||'-vanilla', topology.AsGML(feature)
 FROM features.city_streets
 WHERE feature_name IN ('R3', 'R4' )
 ORDER BY feature_name;

-- Output again but with no prefix
SELECT feature_name||'-noprefix', topology.AsGML(feature, '')
 FROM features.city_streets 
 WHERE feature_name IN ('R3', 'R4' )
 ORDER BY feature_name;

-- Output again with custom prefix
SELECT feature_name||'-customprefix', topology.AsGML(feature, 'cstm')
 FROM features.city_streets 
 WHERE feature_name IN ('R3', 'R4' )
 ORDER BY feature_name;

--- } Lineal single-element

--- Lineal multi-element (TODO) {
--- } Lineal multi-element

--- Areal single-element {

-- Output simple lineal features (composed by single topo element)
SELECT feature_name||'-vanilla', topology.AsGML(feature)
 FROM features.land_parcels
 WHERE feature_name IN ('P4', 'P5' )
 ORDER BY feature_name;

-- Output again but with no prefix
SELECT feature_name||'-noprefix', topology.AsGML(feature, '')
 FROM features.land_parcels WHERE feature_name IN ('P4', 'P5')
 ORDER BY feature_name;

-- Output again with custom prefix
SELECT feature_name||'-customprefix', topology.AsGML(feature, 'cstm')
 FROM features.land_parcels WHERE feature_name IN ('P4', 'P5')
 ORDER BY feature_name;

--- } Areal single-element 

--- Areal multi-element (TODO) {
--- } Areal multi-element 

SELECT topology.DropTopology('city_data');
DROP SCHEMA features CASCADE;
DELETE FROM spatial_ref_sys where srid = 4326;
