set client_min_messages to WARNING;

\i load_topology.sql
\i load_features.sql
\i hierarchy.sql

--- Lineal non-hierarchical 
SELECT feature_name||'-vanilla', topology.AsTopoJSON(feature)
 FROM features.city_streets
 WHERE feature_name IN ('R3', 'R4', 'R1', 'R2' )
 ORDER BY feature_name;

--- Lineal hierarchical 
SELECT feature_name||'-vanilla', topology.AsTopoJSON(feature)
 FROM features.big_streets
 WHERE feature_name IN ('R4', 'R1R2' )
 ORDER BY feature_name;

--- Areal non-hierarchical
SELECT feature_name||'-vanilla', topology.AsTopoJSON(feature)
 FROM features.land_parcels
 WHERE feature_name IN ('P1', 'P2', 'P3', 'P4', 'P5' )
 ORDER BY feature_name;

--- Areal hierarchical
SELECT feature_name||'-vanilla', topology.AsTopoJSON(feature)
 FROM features.big_parcels
 WHERE feature_name IN ('P1P2', 'P3P4')
 ORDER BY feature_name;

-- Now again with edge mapping {
CREATE TEMP TABLE edgemap (arc_id serial, edge_id int unique);

--- Lineal non-hierarchical 
SELECT feature_name||'-vanilla', topology.AsTopoJSON(feature, 'edgemap')
 FROM features.city_streets
 WHERE feature_name IN ('R3', 'R4', 'R1', 'R2' )
 ORDER BY feature_name;

--- Lineal hierarchical 
SELECT feature_name||'-vanilla', topology.AsTopoJSON(feature, 'edgemap')
 FROM features.big_streets
 WHERE feature_name IN ('R4', 'R1R2' )
 ORDER BY feature_name;

--- Areal non-hierarchical
SELECT feature_name||'-vanilla', topology.AsTopoJSON(feature, 'edgemap')
 FROM features.land_parcels
 WHERE feature_name IN ('P1', 'P2', 'P3', 'P4', 'P5' )
 ORDER BY feature_name;

--- Areal hierarchical
SELECT feature_name||'-vanilla', topology.AsTopoJSON(feature, 'edgemap')
 FROM features.big_parcels
 WHERE feature_name IN ('P1P2', 'P3P4')
 ORDER BY feature_name;

DROP TABLE edgemap;
-- End edge mapping }


SELECT topology.DropTopology('city_data');
DROP SCHEMA features CASCADE;
