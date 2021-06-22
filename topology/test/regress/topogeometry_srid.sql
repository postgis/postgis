set client_min_messages to WARNING;

\i ../load_topology.sql
\i ../load_features.sql
\i ../more_features.sql
\i ../hierarchy.sql

SELECT DISTINCT 'ST_Srid(traffic_signs)',
	geometrytype(feature) FROM features.traffic_signs;

SELECT DISTINCT 'ST_Srid(city_streets)',
	geometrytype(feature) FROM features.city_streets;

SELECT DISTINCT 'ST_Srid(land_parcels)',
	geometrytype(feature) FROM features.land_parcels;

SELECT DISTINCT 'ST_Srid(big_signs)',
	geometrytype(feature) FROM features.big_signs;

SELECT DISTINCT 'ST_Srid(big_streets)',
	geometrytype(feature) FROM features.big_streets;

SELECT DISTINCT 'ST_Srid(big_parcels)',
	geometrytype(feature) FROM features.big_parcels;

SELECT topology.DropTopology('city_data');
DROP SCHEMA features CASCADE;
